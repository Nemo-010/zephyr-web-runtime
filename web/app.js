/*
 * Browser side of the runtime.
 *
 * The module hands back a pointer to the panel's own bytes, so everything here
 * is about turning 1bpp vertically-tiled pages into pixels and getting input
 * back in. Keeping the conversion on this side is deliberate: it is the only
 * place that could "improve" the image, and it must not.
 */

"use strict";

const WIDTH = 128;
const HEIGHT = 64;
const PAGES = HEIGHT / 8;
const FB_SIZE = WIDTH * PAGES;

/* Palette. A real SSD1306 is white-on-black; these match what it looks like. */
const LIT = [0x9d, 0xf5, 0xd8];
const DARK = [0x07, 0x0a, 0x0c];

const canvas = document.getElementById("screen");
const ctx = canvas.getContext("2d", { alpha: false });
const framesEl = document.getElementById("frames");
const rateEl = document.getElementById("rate");
const statusEl = document.getElementById("status");

let runtime = null;
let fbPtr = 0;
let imageData = null;
let frames = 0;
let lastRateAt = performance.now();
let rateFrames = 0;

function paint(heap) {
  const data = imageData.data;

  for (let page = 0; page < PAGES; page++) {
    for (let x = 0; x < WIDTH; x++) {
      const byte = heap[fbPtr + page * WIDTH + x];

      for (let bit = 0; bit < 8; bit++) {
        const y = page * 8 + bit;
        const lit = (byte >> bit) & 1;
        const colour = lit ? LIT : DARK;
        const off = (y * WIDTH + x) * 4;

        data[off] = colour[0];
        data[off + 1] = colour[1];
        data[off + 2] = colour[2];
        data[off + 3] = 255;
      }
    }
  }

  ctx.putImageData(imageData, 0, 0);
}

function loop() {
  if (!runtime) {
    return;
  }

  runtime.ccall("web_tick", null, [], []);
  paint(runtime.HEAPU8);
  frames++;

  rateFrames++;
  const now = performance.now();

  if (now - lastRateAt >= 500) {
    rateEl.textContent = Math.round((rateFrames * 1000) / (now - lastRateAt)) + " fps";
    framesEl.textContent = String(frames);
    rateFrames = 0;
    lastRateAt = now;
  }

  requestAnimationFrame(loop);
}

/* --- input ------------------------------------------------------------- */

function sendKey(code) {
  if (!runtime) {
    return;
  }
  runtime.ccall("web_key", null, ["number"], [code]);
}

function bindButton(el) {
  const code = Number(el.dataset.code);

  const down = (event) => {
    event.preventDefault();
    if (el.dataset.down === "true") {
      return;
    }
    el.dataset.down = "true";
    sendKey(code);
  };

  const up = (event) => {
    if (event) {
      event.preventDefault();
    }
    el.dataset.down = "false";
  };

  el.addEventListener("pointerdown", down);
  el.addEventListener("pointerup", up);
  el.addEventListener("pointerleave", up);
  el.addEventListener("pointercancel", up);

  /* Keyboard access on the real button element. */
  el.addEventListener("keydown", (event) => {
    if (event.key === " " || event.key === "Enter") {
      down(event);
    }
  });
  el.addEventListener("keyup", (event) => {
    if (event.key === " " || event.key === "Enter") {
      up(event);
    }
  });
}

const KEYMAP = {
  "0": 11,
  "1": 2,
  Enter: 28,
};

function bindKeyboard() {
  window.addEventListener("keydown", (event) => {
    if (event.repeat) {
      return;
    }

    const code = KEYMAP[event.key];

    if (code === undefined) {
      return;
    }

    event.preventDefault();
    sendKey(code);

    const el = document.querySelector(`.key[data-code="${code}"]`);

    if (el) {
      el.dataset.down = "true";
    }
  });

  window.addEventListener("keyup", (event) => {
    const code = KEYMAP[event.key];

    if (code === undefined) {
      return;
    }

    const el = document.querySelector(`.key[data-code="${code}"]`);

    if (el) {
      el.dataset.down = "false";
    }
  });
}

/* --- boot -------------------------------------------------------------- */

async function main() {
  imageData = ctx.createImageData(WIDTH, HEIGHT);

  statusEl.textContent = "loading";

  try {
    runtime = await createZephyrRuntime();
  } catch (error) {
    statusEl.textContent = "failed to load";
    console.error(error);
    return;
  }

  runtime.ccall("web_init", null, [], []);

  fbPtr = runtime.ccall("web_framebuffer", "number", [], []);

  const reportedSize = runtime.ccall("web_framebuffer_size", "number", [], []);
  const reportedW = runtime.ccall("web_width", "number", [], []);
  const reportedH = runtime.ccall("web_height", "number", [], []);

  if (reportedSize !== FB_SIZE || reportedW !== WIDTH || reportedH !== HEIGHT) {
    /* Refuse to show something that does not match the panel the page claims to
     * be. A silent mismatch here is exactly the failure this whole project
     * exists to make impossible. */
    statusEl.textContent =
      `geometry mismatch: runtime says ${reportedW}x${reportedH} ` +
      `(${reportedSize} bytes), page expects ${WIDTH}x${HEIGHT} (${FB_SIZE})`;
    return;
  }

  statusEl.textContent = "running";

  document.querySelectorAll(".key").forEach(bindButton);
  bindKeyboard();

  requestAnimationFrame(loop);
}

main();
