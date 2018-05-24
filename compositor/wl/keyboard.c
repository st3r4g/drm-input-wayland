#define _POSIX_C_SOURCE 200809L
#include <backend/input.h>
#include <util/log.h>
#include <wl/keyboard.h>
#include <xdg/xdg_shell.h>

#include <stdlib.h>
#include <wayland-server-protocol.h>

static void keyboard_release(struct wl_client *client, struct wl_resource
*resource) {
}

static const struct wl_keyboard_interface impl = {
	.release = keyboard_release
};

struct keyboard *keyboard_new(struct wl_resource *resource, struct input *input) {
	struct keyboard *keyboard = calloc(1, sizeof(struct keyboard));
	keyboard->resource = resource;
	keyboard->input = input;
	wl_resource_set_implementation(resource, &impl, keyboard, 0);
	
	wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
	input_get_keymap_fd(input), input_get_keymap_size(input));
	if (wl_resource_get_version(resource) >=
	WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
		wl_keyboard_send_repeat_info(resource, 25, 600);
	return keyboard;
}
