#include <wayland-server-core.h>

struct surface_state {
	struct wl_resource *buffer;
};

struct surface {
	struct surface_state *current, *pending;
	struct texture *texture;
	struct wl_resource *frame;

	struct wl_list link;
};

struct surface *surface_new(struct wl_resource *resource);
