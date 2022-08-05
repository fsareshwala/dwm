dwm - dynamic window manager
----------------------------
This repository houses my fork of `dwm`, a dynamic tiling window manager for X.

Patches
-------
I have applied the following patches in this repository:

- [fullgaps](https://dwm.suckless.org/patches/fullgaps): adds gaps between client windows
- [scratchpad](https://dwm.suckless.org/patches/scratchpad): spawn or restore a floating terminal
  window
- [swallow](https://dwm.suckless.org/patches/swallow): adds window swallowing to `dwm` as known from
  Plan 9's windowing system rio

I'd like to implement the following features at some point in the future:
- Shift+J / Shift+K: move the currently focused client up and down the stack without losing focus

Dependencies
------------
On debian based systems, the following libraries are required in order to build `dwm`:

- `libx11-xcb-dev`
- `libxft-dev`
- `libxinerama-dev`
- `libxcb-res0-dev`

Upstream Project
----------------
For more information about `dwm`, see the [upstream project homepage](https://dwm.suckless.org).
