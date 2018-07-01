#include <secloak/entry.h>

#include <assert.h>
#include <compiler.h>
#include <drivers/dt.h>
#include <drivers/gic.h>
#include <drivers/imx_fb.h>
#include <drivers/imx_gpio.h>
#include <drivers/imx_gpio_keys.h>
#include <initcall.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <kernel/tee_misc.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <sm/optee_smc.h>
#include <secloak/image_headers.h>
#include <secloak/settings.h>
#include <string.h>
#include <util.h>

struct blit display_static[] = {
	{ 0, 0, { header_rotated, 128, 800 } },
	{ 148, 0, { bluerect_rotated, 64, 800 } },
	{ 1051, 80, { footer_rotated, 128, 640 } },
	{ 230, 608, { group_networking_rotated, 64, 192 } },
	{ 502, 608, { group_multimedia_rotated, 64, 192 } },
	{ 776, 608, { group_sensor_rotated, 64, 192 } },
	{ 148, 593, { mode_none_rotated, 64, 96 } },
	{ 148, 402, { mode_airplane_rotated, 64, 128 } },
	{ 148, 272, { mode_movie_rotated, 64, 96 } },
	{ 148, 113, { mode_stealth_rotated, 64, 96 } },
	{ 358, 192, { bt_rotated, 64, 416 } },
	{ 566, 192, { camera_rotated, 64, 416 } },
	{ 422, 192, { cellular_rotated, 64, 416 } },
	{ 840, 192, { gps_rotated, 64, 416 } },
	{ 694, 192, { mic_rotated, 64, 416 } },
	{ 904, 192, { sensor_rotated, 64, 416 } },
	{ 630, 192, { speaker_rotated, 64, 416 } },
	{ 294, 192, { wifi_rotated, 64, 416 } },
};

struct image display_switches_white[] = {
	{ switch_dis_off_w_rotated, 64, 192 },
	{ switch_dis_on_w_rotated, 64, 192 },
	{ switch_en_off_w_rotated, 64, 192 },
	{ switch_en_on_w_rotated, 64, 192 },
};

struct image display_switches_blue[] = {
	{ switch_dis_off_rotated, 64, 192 },
	{ switch_dis_on_rotated, 64, 192 },
	{ switch_en_off_rotated, 64, 192 },
	{ switch_en_on_rotated, 64, 192 },
};

struct image display_icons_wifi[] = {
	{ icon_nowifi_rotated, 64, 192 },
	{ icon_wifi_rotated, 64, 192 },
};

struct image display_icons_bt[] = {
	{ icon_nobt_rotated, 64, 192 },
	{ icon_bt_rotated, 64, 192 },
};

struct image display_icons_camera[] = {
	{ icon_nocamera_rotated, 64, 192 },
	{ icon_camera_rotated, 64, 192 },
};

struct image display_icons_cellular[] = {
	{ icon_nocellular_rotated, 64, 192 },
	{ icon_cellular_rotated, 64, 192 },
};

struct image display_icons_speaker[] = {
	{ icon_nospeaker_rotated, 64, 192 },
	{ icon_speaker_rotated, 64, 192 },
};

struct image display_icons_mic[] = {
	{ icon_nomic_rotated, 64, 192 },
	{ icon_mic_rotated, 64, 192 },
};

struct image display_icons_gps[] = {
	{ icon_nogps_rotated, 64, 192 },
	{ icon_gps_rotated, 64, 192 },
};

struct image display_icons_sensor[] = {
	{ icon_nosensor_rotated, 64, 192 },
	{ icon_sensor_rotated, 64, 192 },
};

struct image display_group_on[] = {
	{ group_dis_on_rotated, 64, 64 },
	{ group_en_on_rotated, 64, 64 },
};

struct image display_group_off[] = {
	{ group_dis_off_rotated, 64, 64 },
	{ group_en_off_rotated, 64, 64 },
};

struct image display_group_custom[] = {
	{ group_dis_custom_rotated, 64, 128 },
	{ group_en_custom_rotated, 64, 128 },
};

struct image display_group_radio[] = {
	{ group_dis_deselect_rotated, 64, 32 },
	{ group_dis_select_rotated, 64, 32 },
	{ group_en_deselect_rotated, 64, 32 },
	{ group_en_select_rotated, 64, 32 },
};

struct image display_mode[] = {
	{ mode_deselect_rotated, 64, 32 },
	{ mode_select_rotated, 64, 32 },
};

static bool cloak_blit_device(struct fb_info *fb, uint32_t bits, int x, struct image *display_switches, struct image *display_icons) {
	bool allow = ((bits & 0x2) != 0);
	fb_blit_image(fb, x, 0, &display_switches[bits]);
	fb_blit_image(fb, x, 608, &display_icons[1]);
	return allow;
}

static inline bool cloak_is_group_enabled(uint32_t group) {
	switch(group) {
		case GROUP_DIS_OFF:
		case GROUP_DIS_CUSTOM:
			return false;
		case GROUP_EN_OFF:
		case GROUP_EN_ON:
		case GROUP_EN_CUSTOM:
			return true;
		default:
			EMSG("[SeCloak] Invalid group value of %u", group);
			panic();
	}
}

static void cloak_blit_group(struct fb_info *fb, uint32_t bits, int x) {
	bool is_enabled = cloak_is_group_enabled(bits);
	fb_blit_image(fb, x, 507, &display_group_on[is_enabled]);
	fb_blit_image(fb, x, 348, &display_group_off[is_enabled]);
	fb_blit_image(fb, x, 123, &display_group_custom[is_enabled]);

	bool is_on = (bits == GROUP_EN_ON);
	bool is_off = (bits == GROUP_DIS_OFF || bits == GROUP_EN_OFF);
	bool is_custom = (bits == GROUP_DIS_CUSTOM || bits == GROUP_EN_CUSTOM);
	fb_blit_image(fb, x, 573, &display_group_radio[(is_enabled << 1) | is_on]);
	fb_blit_image(fb, x, 414, &display_group_radio[(is_enabled << 1) | is_off]);
	fb_blit_image(fb, x, 253, &display_group_radio[(is_enabled << 1) | is_custom]);
}

#define CLOAK_CONFIRM_NONE 0
#define CLOAK_CONFIRM_ALLOW 1
#define CLOAK_CONFIRM_DENY 2

static volatile int cloak_confirm_status = CLOAK_CONFIRM_NONE;

static bool cloak_button_press_handler(int code) {
	switch (code) {
		case KEY_HOMEPAGE:
			cloak_confirm_status = CLOAK_CONFIRM_ALLOW;
			break;
		case KEY_BACK:
			cloak_confirm_status = CLOAK_CONFIRM_DENY;
			break;
		default:
			break;
	}

	// Do not pass these presses back to the non-secure world
	return true;
}

static struct button_handler cloak_button_handler = {
	.on_press = cloak_button_press_handler,
};

static unsigned int cloak_lock = SPINLOCK_UNLOCK;
static unsigned long cloak_prev_settings = 0;

struct cloak_class {
	const char *name;
	bool allow;
};

static void cloak_entry_set(struct thread_smc_args *args) {
	int error = OPTEE_SMC_RETURN_OK;

	if (!cpu_spin_trylock(&cloak_lock)) {
		EMSG("[SeCloak] Could not acquire the lock");
		error = OPTEE_SMC_RETURN_EBUSY;
		goto err_lock;
	}

	struct fb_info fb;
	if (!fb_acquire(&fb, 0xFF, 0xFF, 0xFF)) {
		EMSG("[SeCloak] Could not acquire the frame buffer");
		error = OPTEE_SMC_RETURN_EBADCMD;
		goto err_fb_acq;
	}

	if (!gpio_keys_acquire(&cloak_button_handler)) {
		EMSG("[SeCloak] Could not acquire the GPIO keys");
		error = OPTEE_SMC_RETURN_EBADCMD;
		goto err_gpio_keys_acq;
	}

	EMSG("[SeCloak] Bit Vector = %08x", args->a1);

	// Static content
	fb_blit_all(&fb, display_static, sizeof(display_static) / sizeof(display_static[0]));

	// Individual
	struct cloak_class classes[8] = {
		{ "wifi", cloak_blit_device(&fb, (args->a1 >> WIFI_SHIFT) & 0x3, 294, display_switches_blue, display_icons_wifi) },
		{ "bluetooth", cloak_blit_device(&fb, (args->a1 >> BT_SHIFT) & 0x3, 358, display_switches_white, display_icons_bt) },
		{ "cellular", cloak_blit_device(&fb, (args->a1 >> CELLULAR_SHIFT) & 0x3, 422, display_switches_blue, display_icons_cellular) },
		{ "camera", cloak_blit_device(&fb, (args->a1 >> CAMERA_SHIFT) & 0x3, 566, display_switches_blue, display_icons_camera) },
		{ "audio-out", cloak_blit_device(&fb, (args->a1 >> SPEAKER_SHIFT) & 0x3, 630, display_switches_white, display_icons_speaker) },
		{ "audio-in", cloak_blit_device(&fb, (args->a1 >> MIC_SHIFT) & 0x3, 694, display_switches_blue, display_icons_mic) },
		{ "gps", cloak_blit_device(&fb, (args->a1 >> GPS_SHIFT) & 0x3, 840, display_switches_blue, display_icons_gps) },
		{ "inertial", cloak_blit_device(&fb, (args->a1 >> INERTIAL_SHIFT) & 0x3, 904, display_switches_white, display_icons_sensor) }
	};

	// Groups
	cloak_blit_group(&fb, (args->a1 >> NETWORK_SHIFT) & 0x7, 230);
	cloak_blit_group(&fb, (args->a1 >> MULTIMEDIA_SHIFT) & 0x7, 502);
	cloak_blit_group(&fb, (args->a1 >> SENSOR_SHIFT) & 0x7, 776);

	// Modes
	uint32_t mode = args->a1 & 0x3;
	fb_blit_image(&fb, 148, 693, &display_mode[mode == MODE_NONE]);
	fb_blit_image(&fb, 148, 534, &display_mode[mode == MODE_AIRPLANE]);
	fb_blit_image(&fb, 148, 372, &display_mode[mode == MODE_MOVIE]);
	fb_blit_image(&fb, 148, 213, &display_mode[mode == MODE_STEALTH]);

	// Wait for the confirmation button to be pressed
	DMSG("[SeCloak] Waiting for confirmation...");

	cloak_confirm_status = CLOAK_CONFIRM_NONE;
	while (cloak_confirm_status == CLOAK_CONFIRM_NONE) { }

	if (cloak_confirm_status == CLOAK_CONFIRM_ALLOW) {
		IMSG("[SeCloak] Confirmed 'Allow'");

		for (unsigned int c = 0; c < sizeof(classes) / sizeof(classes[0]); c++) {
			IMSG("\tClass '%s' = %s", classes[c].name, classes[c].allow ? "Enabled" : "Disabled");
		}
		
		// TODO: Clean this up. Right now, it is possible for a device to belong to
		// two classes. If one of those classes is enabled, and comes after the
		// one that is disabled, then the device will remain enabled.
		// To fix this, we set all classes as enabled and then only call
		// dt_enable_class when we want to disable the class.
		for (unsigned int c = 0; c < sizeof(classes) / sizeof(classes[0]); c++) {
			dt_enable_class(classes[c].name, true);
		}
		for (unsigned int c = 0; c < sizeof(classes) / sizeof(classes[0]); c++) {
			if (!classes[c].allow) {
				dt_enable_class(classes[c].name, false);
			}
		}

		cloak_prev_settings = args->a1;

		fb_clear(&fb, 0x00, 0xFF, 0x00);
	} else {
		DMSG("[SeCloak] Confirmed 'Deny'");
		fb_clear(&fb, 0xFF, 0x00, 0x00);
	}

	gpio_keys_release(&cloak_button_handler);

err_gpio_keys_acq:
	fb_release(&fb);

err_fb_acq:
err_lock:
	cpu_spin_unlock(&cloak_lock);
	args->a0 = error;
}

static void cloak_entry_get(struct thread_smc_args *args) {
	int error = OPTEE_SMC_RETURN_OK;

	if (!cpu_spin_trylock(&cloak_lock)) {
		EMSG("[SeCloak] Could not acquire the lock");
		error = OPTEE_SMC_RETURN_EBUSY;
		goto err_lock;
	}

	args->a1 = cloak_prev_settings;

err_lock:
	cpu_spin_unlock(&cloak_lock);
	args->a0 = error;
}

void cloak_entry(struct thread_smc_args *smc_args)
{
	if (smc_args->a0 == OPTEE_SMC_CLOAK_SET) {
		cloak_entry_set(smc_args);
	} else if (smc_args->a1 == OPTEE_SMC_CLOAK_GET) {
		cloak_entry_get(smc_args);
	} else {
		smc_args->a0 = OPTEE_SMC_RETURN_EBADCMD;
	}
}

