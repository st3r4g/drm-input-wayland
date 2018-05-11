# DRM-INPUT-WAYLAND
My attempt at learning Wayland by writing simple and minimal programs

note: requires the DRM atomic commit feature, no support for the legacy interface

WARNING: hardcoded gpu/keyboard path for now

Folders explained:
* 01 -> minimal color/image rendering (dumb-buffer) for 3 seconds
* 02 -> minimal color rendering (dumb-buffer), exit on keypress
* compositor -> Wayland protocol implementation [in progress]

Rendering supports both wl_shm and wl_drm and should display fine, but without
any optimizations (e.g.: full redraw on every VBLANK, 1 frame of latency for
fast clients...) at the moment.

Bugs:
* CPU usage grows linearly (seen with weston-terminal/top)
* sometimes simple-egl fails at egl_init
* sometimes simple-egl fails and the process survives the death of the
compositor with full CPU usage (related to keyboard_enter_surface code)
