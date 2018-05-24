#include <wayland-server-core.h>

struct keyboard {
	struct wl_resource *resource;
	struct input *input;
};

struct keyboard *keyboard_new(struct wl_resource *resource, struct input *input);
