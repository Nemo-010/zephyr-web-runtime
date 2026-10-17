/*
 * Headless test for the WebAssembly runtime.
 *
 * Asserts the things that actually matter for a faithful preview: the geometry
 * the runtime reports, that it renders something non-trivial, and that a button
 * press changes the framebuffer. A runtime that renders nothing, or that
 * ignores input, is exactly the failure this test exists to catch.
 */

import { createRequire } from "module";
import { dirname, join } from "path";
import { fileURLToPath } from "url";

const here = dirname(fileURLToPath(import.meta.url));
const modulePath = join(here, "..", "web", "dist", "zephyr-runtime.js");

const require = createRequire(import.meta.url);
const factory = require(modulePath);

const WIDTH = 128;
const HEIGHT = 64;
const FB_SIZE = (WIDTH * HEIGHT) / 8;

let failures = 0;

function check(name, condition, detail = "") {
  if (condition) {
    console.log(`ok   ${name}`);
  } else {
    console.log(`FAIL ${name} ${detail}`);
    failures++;
  }
}

const mod = await factory();
mod.ccall("web_init", null, [], []);

check("width", mod.ccall("web_width", "number", [], []) === WIDTH);
check("height", mod.ccall("web_height", "number", [], []) === HEIGHT);
check("framebuffer size", mod.ccall("web_framebuffer_size", "number", [], []) === FB_SIZE);

const ptr = mod.ccall("web_framebuffer", "number", [], []);
const snapshot = () => Buffer.from(mod.HEAPU8.slice(ptr, ptr + FB_SIZE));

const step = async (count, delayMs = 16) => {
  for (let i = 0; i < count; i++) {
    mod.ccall("web_tick", null, [], []);
    await new Promise((resolve) => setTimeout(resolve, delayMs));
  }
};

await step(8);

const idle = snapshot();
let lit = 0;

for (const byte of idle) {
  for (let bit = 0; bit < 8; bit++) {
    if ((byte >> bit) & 1) {
      lit++;
    }
  }
}

/* A blank panel is the failure mode worth catching, so require real content. */
check("renders content", lit > 200, `(only ${lit} lit pixels)`);
check("not fully lit", lit < WIDTH * HEIGHT * 0.9, `(${lit} lit pixels)`);

/* The border is drawn at the panel edge, so the first byte must not be empty. */
check("top-left corner is drawn", idle[0] !== 0, `(byte 0 is ${idle[0]})`);

mod.ccall("web_key", null, ["number"], [11]);
await step(8);

const after = snapshot();
let changed = 0;

for (let i = 0; i < FB_SIZE; i++) {
  if (idle[i] !== after[i]) {
    changed++;
  }
}

check("a key press redraws the panel", changed > 0, `(${changed} bytes changed)`);

/* And a second press must keep working, not just the first. */
mod.ccall("web_key", null, ["number"], [11]);
await step(8);
const after2 = snapshot();
let changed2 = 0;

for (let i = 0; i < FB_SIZE; i++) {
  if (after[i] !== after2[i]) {
    changed2++;
  }
}

check("a second key press redraws again", changed2 > 0, `(${changed2} bytes changed)`);

console.log(failures === 0 ? "\nall runtime tests passed" : `\n${failures} test(s) failed`);
process.exit(failures === 0 ? 0 : 1);
