#ifndef DRIVERS_IMX_FB_H
#define DRIVERS_IMX_FB_H

#include <stdint.h>
#include <stdbool.h>

struct fb_info {
	uint8_t *buffer;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t prev_params_ch0[16];
	uint32_t prev_params_ch1[16];
};

struct image {
	const uint8_t *buffer;
	uint32_t width;
	uint32_t height;
};

struct blit {
	uint32_t x;
	uint32_t y;
	struct image image;
};

bool fb_acquire(struct fb_info *info, uint8_t r, uint8_t g, uint8_t b);
void fb_release(struct fb_info *info);

void fb_clear(struct fb_info *info, uint8_t r, uint8_t g, uint8_t b);
void fb_blit(struct fb_info *info, uint32_t x, uint32_t y, const uint8_t *buffer, uint32_t width, uint32_t height);
void fb_blit_image(struct fb_info *info, uint32_t x, uint32_t y, const struct image *image);
void fb_blit_all(struct fb_info *info, const struct blit *blits, int num_blits);

#endif

