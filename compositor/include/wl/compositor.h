#include <wayland-server-core.h>

struct compositor {
	struct egl *egl;
};

struct compositor *compositor_new(struct wl_resource *resource, struct egl *egl);
