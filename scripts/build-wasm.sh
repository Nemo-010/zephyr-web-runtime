#!/usr/bin/env bash
# Build LVGL and the application to WebAssembly.
#
# SPDX-License-Identifier: Apache-2.0
#
# Needs emcc on PATH and LVGL_SRC pointing at an LVGL tree. Nothing else: the
# runtime is deliberately independent of Zephyr, because Zephyr's kernel cannot
# currently be built for wasm (its heap-size constants use absolute symbols,
# which wasm has no concept of). See docs/why-not-zephyr.md.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
: "${LVGL_SRC:?set LVGL_SRC to an LVGL source tree}"
out="${OUT:-$here/web/dist}"
build="$here/build"

mkdir -p "$out" "$build"

# Only the LVGL sources the UI needs. Compiling all of LVGL for wasm works but
# takes minutes and produces a much larger module for no benefit.
#
# The stdlib/core sources are added separately below, because exactly one
# implementation has to be compiled in and which one depends on lv_conf.h.
sources=(
  "$here/app/app.c"
  "$here/app/main.c"
)

# The widgets and core LVGL files. LVGL's own build system can be used instead,
# but a flat list keeps this reproducible and fast.
# Collected into an array via a plain command substitution, because process
# substitution is not available in every shell this may be run from.
lvgl_sources="$(
  find "$LVGL_SRC/src" -name '*.c' \
    ! -path '*/osal/lv_cmsis_rtos2.c' \
    ! -path '*/osal/lv_freertos.c' \
    ! -path '*/osal/lv_linux.c' \
    ! -path '*/osal/lv_mqx.c' \
    ! -path '*/osal/lv_os_none.c' \
    ! -path '*/osal/lv_pthread.c' \
    ! -path '*/osal/lv_rtthread.c' \
    ! -path '*/osal/lv_sdl2.c' \
    ! -path '*/osal/lv_windows.c' \
    ! -path '*/drivers/*' \
    ! -path '*/libs/gltf/*' \
    ! -path '*/stdlib/builtin/*' \
    ! -path '*/stdlib/clib/*' \
    ! -path '*/stdlib/rtthread/*' \
    ! -path '*/stdlib/micropython/*' \
    ! -path '*/stdlib/uefi/*' \
    ! -path '*/debugging/test/*'
)"

while IFS= read -r f; do
  [ -n "$f" ] && sources+=("$f")
done <<< "$lvgl_sources"

# lv_conf.h selects the C library for malloc, string and sprintf.
sources+=(
  "$LVGL_SRC/src/stdlib/clib/lv_mem_core_clib.c"
  "$LVGL_SRC/src/stdlib/clib/lv_string_clib.c"
  "$LVGL_SRC/src/stdlib/clib/lv_sprintf_clib.c"
)

emcc \
  -I"$LVGL_SRC" \
  -I"$LVGL_SRC/src" \
  -DLV_CONF_INCLUDE_SIMPLE=1 \
  -DLV_CONF_PATH="\"$here/app/lv_conf.h\"" \
  "${sources[@]}" \
  -o "$out/zephyr-runtime.js" \
  -sMODULARIZE=1 \
  -sEXPORT_NAME=createZephyrRuntime \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_malloc","_free","_web_init","_web_tick","_web_key","_web_framebuffer","_web_framebuffer_size","_web_width","_web_height"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8"]' \
  -sENVIRONMENT=web,worker,node \
  -O1

echo "built $out/zephyr-runtime.js"
ls -la "$out"
