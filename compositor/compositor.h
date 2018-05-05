#include <wayland-server-core.h>

struct compositor {
	struct wl_list surface_list;

	struct wl_list link;
};

struct compositor *compositor_new(struct wl_resource *resource);
