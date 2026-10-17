/*
 * WebAssembly entry point.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Exposes the smallest possible surface to JavaScript: create the UI, advance
 * LVGL by one frame, hand back a pointer to the 1bpp framebuffer, and feed a
 * key in. Everything else (widgets, fonts, rendering) is LVGL itself, compiled
 * to wasm, so the browser shows exactly what the device would.
 *
 * The framebuffer is returned as the panel's own bytes: 1 bit per pixel, eight
 * vertical pixels per byte, bit 7 first. Converting that to something a browser
 * likes is the page's job, not the runtime's, because that is what keeps the
 * runtime honest about what the panel receives.
 */

#include <lvgl.h>
#include <stdint.h>
#include <string.h>

#include <emscripten.h>

#define OLED_W 128
#define OLED_H 64
/* Eight vertical pixels per byte, one byte per column per page. */
#define FB_STRIDE OLED_W
#define FB_PAGES  (OLED_H / 8)
#define FB_SIZE   (FB_STRIDE * FB_PAGES)

lv_obj_t *app_create(void);
void app_tick(void);
void app_on_key(uint32_t key);

static uint8_t framebuffer[FB_SIZE];

static uint32_t web_tick_get(void)
{
	return (uint32_t)emscripten_get_now();
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
	/*
	 * LVGL hands over an I1 buffer with an 8 byte palette header, in
	 * horizontal bit order. The panel wants vertical pages, so the bits are
	 * transposed here, once, in the same place the real display driver does it.
	 */
	const uint8_t *src = px_map + 8;

	/*
	 * LVGL's I1 buffer is a packed *bit* array: `width` bits per row, with bit
	 * 7 of each byte first. The stride here is therefore in pixels, not bytes.
	 * This matches what Zephyr's own LVGL monochrome glue does
	 * (modules/lvgl/lvgl_display_mono.c), which is the reference for how LVGL
	 * lays these buffers out.
	 */
	const uint32_t stride_bits = OLED_W;

	for (int32_t y = area->y1; y <= area->y2; y++) {
		for (int32_t x = area->x1; x <= area->x2; x++) {
			uint32_t bit = (uint32_t)x + (uint32_t)y * stride_bits;
			uint8_t set = (src[bit / 8] >> (7 - (bit % 8))) & 1;

			/* MONO10 with hardware inversion: a set bit is a lit pixel. */
			uint8_t *dst = &framebuffer[(y / 8) * FB_STRIDE + x];
			uint8_t mask = 1u << (y % 8);

			if (set) {
				*dst |= mask;
			} else {
				*dst &= (uint8_t)~mask;
			}
		}
	}

	lv_display_flush_ready(disp);
}

EMSCRIPTEN_KEEPALIVE
uint8_t *web_framebuffer(void)
{
	return framebuffer;
}

EMSCRIPTEN_KEEPALIVE
int web_framebuffer_size(void)
{
	return FB_SIZE;
}

EMSCRIPTEN_KEEPALIVE
int web_width(void)
{
	return OLED_W;
}

EMSCRIPTEN_KEEPALIVE
int web_height(void)
{
	return OLED_H;
}

EMSCRIPTEN_KEEPALIVE
void web_init(void)
{
	lv_init();

	/*
	 * LVGL's tick callback returns uint32_t milliseconds. Casting
	 * emscripten_get_now() straight to that type would pass a double, which is
	 * a signature mismatch in wasm (it traps rather than converting), so the
	 * conversion is done explicitly in a real function.
	 */
	lv_tick_set_cb(web_tick_get);

	lv_display_t *disp = lv_display_create(OLED_W, OLED_H);

	lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
	lv_display_set_flush_cb(disp, flush_cb);

	static uint8_t draw_buf[FB_SIZE + 8];

	lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
			       LV_DISPLAY_RENDER_MODE_FULL);

	memset(framebuffer, 0, sizeof(framebuffer));

	lv_group_t *group = lv_group_create();

	lv_group_set_default(group);

	app_create();
}

EMSCRIPTEN_KEEPALIVE
void web_tick(void)
{
	app_tick();
	lv_timer_handler();
}

EMSCRIPTEN_KEEPALIVE
void web_key(uint32_t code)
{
	app_on_key(code);
}
