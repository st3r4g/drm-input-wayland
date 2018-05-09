#include <wayland-server-core.h>

struct seat {
	struct wl_resource *keyb;

	struct wl_list link;
};

struct seat *seat_new(struct wl_resource *resource);
