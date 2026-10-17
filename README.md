# zephyr-web-runtime

LVGL compiled to WebAssembly, rendering a 128x64 monochrome OLED in the browser,
with the panel's real button codes wired to on-screen keys.

**[Try it](https://nemo-010.github.io/zephyr-web-runtime/)** - the page is built
and tested by CI on every push.

```
+------------------------+   +-----------------------------+
| PREVIEW                |   |  Controls                   |
|                        |   |                             |
| [====================] |   |  SW2 (11)   SW3 (2)         |
|                        |   |                             |
|  one                   |   |  State                      |
|  two                   |   |  Frames   1234              |
|                        |   |  Render   60 fps             |
| presses: 0             |   |                             |
+------------------------+   +-----------------------------+
```

## What this is, and is not

It is LVGL, not the Zephyr kernel. Zephyr's kernel cannot currently be built for
WebAssembly: its heap-size constants are emitted as absolute symbols, which wasm
has no representation for. [docs/why-not-zephyr.md](docs/why-not-zephyr.md)
explains the blocker, what was tried, and what stays faithful.

For judging a UI on a monochrome OLED, the faithful parts are the ones that
matter: the same LVGL version, the same 1bpp packing, the same bit order, the
same geometry, the same font, and the same button codes. If a screen depends on
the kernel (sensors, drivers, `k_uptime_get()`), use the desktop preview, which
runs the real thing.

## The framebuffer is the panel's bytes

The runtime hands JavaScript a pointer to the panel buffer exactly as an SSD1306
would receive it: one byte per column per page, eight vertical pixels per byte,
bit 7 first. The page does the conversion to pixels, and deliberately does not
"improve" it. If the packing were wrong, the page would show the same garbage
the panel would.

## Build

```bash
# emcc and node on PATH
export LVGL_SRC=/path/to/lvgl        # any LVGL 9.x tree
./scripts/build-wasm.sh              # -> web/dist/zephyr-runtime.{js,wasm}
node tests/test_runtime.mjs          # headless: geometry, content, input
```

The runtime is `web/`. Serve it over HTTP (wasm will not load from `file://`):

```bash
python3 -m http.server -d web 8000   # then open http://localhost:8000
```

With pixi, `pixi run -e web bash scripts/build-wasm.sh` provides emcc and node.

## Input

| Key | Button code | Why that number |
| --- | --- | --- |
| `0` | 11 | `INPUT_KEY_0`, the MCXA156's User SW2 |
| `1` | 2 | `INPUT_KEY_1`, the MCXA156's User SW3 |
| `Enter` | 28 | `INPUT_KEY_ENTER`, a conventional select |

The on-screen buttons use the same codes. These are the values the board's own
devicetree declares, not invented ones.

## Layout

```
app/app.c            the UI, plain LVGL, shared with the desktop preview
app/main.c           wasm entry points: init, tick, key, framebuffer pointer
app/lv_conf.h        1bpp, UNSCII 8, no features the panel cannot show
scripts/build-wasm.sh
web/                 the page: canvas, buttons, and the 1bpp decoder
tests/test_runtime.mjs
docs/why-not-zephyr.md
```

## Licence

Apache-2.0.
