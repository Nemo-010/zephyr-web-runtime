# Why this is LVGL and not the Zephyr kernel

This runtime renders with LVGL compiled to WebAssembly. It does **not** run the
Zephyr kernel. That is a deliberate limitation, and here is exactly why.

## What blocks it

Zephyr computes some of its heap sizing at build time. `lib/heap/heap_constants.c`
emits sizes as **absolute symbols** with `GEN_ABS_SYM_BEGIN` / `GEN_ABS_SYM_END`,
which `scripts/build/gen_offset_header.py` turns into a generated header. The
purpose is to avoid magic numbers: `kernel.h` derives heap sizes from real
`sizeof()` values instead of hardcoding them.

WebAssembly has no representation for an absolute symbol. A symbol in wasm is an
address within linear memory, resolved at instantiation, and there is no
link-time-constant address space. So the build fails at the object stage:

```
fatal error: error in backend: ___z_heap_struct_SIZEOF: absolute addressing not supported!
```

This is not a flag or a version issue. `zephyr_constants_library()` is called
unconditionally from `lib/heap/CMakeLists.txt` and from the top-level
`CMakeLists.txt`, so it is not something a project can opt out of, and patching
it would mean maintaining a Zephyr fork.

## What was tried

The whole path was tested, not assumed:

1. `native_sim/native/64` configures under Emscripten. The devicetree resolves,
   Kconfig runs, and `emcc` is found as the compiler.
2. `CONFIG_SDL_DISPLAY=n` removes the only hard external dependency (SDL), since
   this project supplies its own display.
3. The build then fails at `heap_constants.c`, as above, before any kernel code
   is compiled.

## What is faithful anyway

For the purpose of judging a UI on a 128x64 OLED, the parts that matter are
shared with the device build:

| Property | Device build | This runtime |
| --- | --- | --- |
| Widget tree | LVGL | LVGL, same version |
| Colour depth | 1bpp (I1) | 1bpp (I1) |
| Panel packing | 8 vertical pixels/byte, bit 7 first | same |
| Geometry | from the board's devicetree | 128x64, from the same devicetree |
| Font | UNSCII 8 | UNSCII 8 |
| Button codes | from `zephyr,code` | the same numbers |

What is *not* faithful is everything the kernel does: scheduling, drivers,
sensor reads, and anything time-dependent that depends on `k_uptime_get()`
rather than LVGL's tick. If a screen depends on those, use the desktop preview
(`lvgl-preview run`), which does run the real kernel.

## If this becomes possible

The blocker is one file. If Zephyr ever guards `zephyr_constants_library()` on
`CONFIG_ARCH_POSIX` or offers a non-absolute-symbol mode, this runtime can be
rebuilt against the real kernel and the desktop preview's capture sink can be
reused unchanged: it is a display driver, and wasm needs a display driver just
as much as Linux does.
