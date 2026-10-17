/*
 * LVGL configuration for the browser runtime.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Kept minimal on purpose: 1bpp monochrome, the UNSCII 8 font (which is what
 * looks right on a 128x64 OLED), and no features the panel cannot show.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 1

#define LV_USE_OS LV_OS_NONE

#define LV_MEM_SIZE (256 * 1024)
/* Use the C library for string and printf: Emscripten provides both, and
 * LVGL's builtin implementations are a separate source tree that has to be
 * compiled in explicitly. malloc stays LVGL's own so memory can be measured. */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Widgets. Only what the UI uses. */
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_LIST 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_OBJ 1

/* Fonts. UNSCII 8 is the standard choice for 128x64 monochrome. */
#define LV_FONT_UNSCII_8 1
#define LV_FONT_DEFAULT &lv_font_unscii_8
#define LV_FONT_MONTSERRAT_14 0

#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE 1

#define LV_DPI_DEF 116

#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0

#endif /* LV_CONF_H */
