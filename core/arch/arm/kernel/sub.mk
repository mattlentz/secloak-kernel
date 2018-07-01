srcs-$(CFG_ARM32_core) += proc_a32.S
srcs-$(CFG_ARM32_core) += spin_lock_a32.S
srcs-$(CFG_TEE_CORE_DEBUG) += spin_lock_debug.c
srcs-$(CFG_ARM32_core) += ssvce_a32.S
srcs-$(CFG_PL310) += tz_ssvce_pl310_a32.S

srcs-$(CFG_ARM32_core) += thread_a32.S
srcs-y += thread.c
srcs-y += abort.c
srcs-y += trace_ext.c
srcs-$(CFG_ARM32_core) += misc_a32.S
srcs-$(CFG_PM_STUBS) += pm_stubs.c
cflags-pm_stubs.c-y += -Wno-suggest-attribute=noreturn

srcs-$(CFG_GENERIC_BOOT) += generic_boot.c
ifeq ($(CFG_GENERIC_BOOT),y)
srcs-$(CFG_ARM32_core) += generic_entry_a32.S
endif

ifeq ($(CFG_UNWIND),y)
srcs-y += unwind_arm32.c
endif
