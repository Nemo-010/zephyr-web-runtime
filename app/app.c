/*
 * The application, shared by the native preview and the browser runtime.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Deliberately written against plain LVGL with no Zephyr dependency, so the
 * same source compiles both into a Zephyr application (for the desktop preview)
 * and into a WebAssembly module (for the browser). What the browser shows is
 * therefore the same widget tree, not a mock-up of it.
 *
 * The layout is designed for a 128x64 monochrome OLED: every size is in whole
 * text lines, because that is the unit an OLED actually has.
 */

#include <lvgl.h>
#include <stdio.h>

#define OLED_W 128
#define OLED_H 64
#define LINE   10

static lv_obj_t *status;
static lv_obj_t *bar;
static int presses;

void app_on_key(uint32_t key);
void app_tick(void);
lv_obj_t *app_create(void);

static void set_status(const char *fmt, int value)
{
	lv_label_set_text_fmt(status, fmt, value);
}

void app_on_key(uint32_t key)
{
	presses++;

	/* Echo the raw code: a real board's buttons are whatever its devicetree
	 * says they are, and on the MCXA156 they are '0' and '1'. */
	lv_label_set_text_fmt(status, "key %u (%d)", (unsigned)key, presses);

	/*
	 * Redraw explicitly.
	 *
	 * lv_label_set_text marks the label dirty, but on a full-refresh
	 * monochrome display LVGL still only flushes when something asks it to.
	 * An input event arriving outside a widget's own handling does not do that,
	 * so without this the key is received and the screen never changes.
	 */
	lv_obj_invalidate(lv_screen_active());
}

void app_tick(void)
{
	static int value = 50;
	static int step = 7;

	value += step;
	if (value >= 100) {
		value = 100;
		step = -step;
	} else if (value <= 0) {
		value = 0;
		step = -step;
	}
	lv_bar_set_value(bar, value, LV_ANIM_OFF);
}

lv_obj_t *app_create(void)
{
	lv_obj_t *screen = lv_screen_active();
	lv_obj_t *label;
	lv_obj_t *list;

	lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_text_color(screen, lv_color_white(), 0);
	lv_obj_set_style_border_color(screen, lv_color_white(), 0);
	lv_obj_set_style_border_width(screen, 1, 0);

	label = lv_label_create(screen);
	lv_label_set_text(label, "PREVIEW");
	lv_obj_set_pos(label, 3, 3);

	bar = lv_bar_create(screen);
	lv_obj_set_size(bar, OLED_W - 6, 8);
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 50, LV_ANIM_OFF);
	lv_obj_set_pos(bar, 3, 3 + LINE + 2);

	list = lv_list_create(screen);
	lv_obj_set_size(list, OLED_W - 6, 22);
	lv_obj_set_pos(list, 3, 3 + 2 * LINE + 4);
	lv_obj_set_style_bg_color(list, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(list, lv_color_white(), 0);
	lv_obj_set_style_border_width(list, 1, 0);
	lv_obj_set_style_text_color(list, lv_color_white(), 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_style_pad_row(list, 0, 0);
	lv_obj_set_style_radius(list, 0, 0);

	lv_list_add_button(list, NULL, "one");
	lv_list_add_button(list, NULL, "two");

	uint32_t count = lv_obj_get_child_count(list);

	for (uint32_t i = 0; i < count; i++) {
		lv_obj_t *child = lv_obj_get_child(list, (int32_t)i);

		lv_obj_set_style_bg_color(child, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(child, LV_OPA_COVER, 0);
		lv_obj_set_style_text_color(child, lv_color_white(), 0);
		lv_obj_set_style_border_width(child, 0, 0);
		lv_obj_set_style_pad_all(child, 1, 0);
		lv_obj_set_style_radius(child, 0, 0);
	}

	status = lv_label_create(screen);
	set_status("presses: %d", 0);
	lv_obj_set_pos(status, 3, OLED_H - LINE - 2);

	return screen;
}
