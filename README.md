# DRM-INPUT-WAYLAND
My attempt at learning Wayland by writing simple and minimal programs

note: requires the DRM atomic commit feature, no support for the legacy interface

WARNING: hardcoded gpu/keyboard path for now

Folders explained:
* 01 -> minimal color/image rendering (dumb-buffer) for 3 seconds
* 02 -> minimal color rendering (dumb-buffer), exit on keypress
* compositor -> Wayland protocol implementation [in progress]
* 99 -> Headless compositor [for debugging]

the compositor has now a GLES3 renderer!

TODO: manage GBM surfaces properly (add double buffering and remove nanosleep)

TODO: reorganize code and get rid of 99 folder
