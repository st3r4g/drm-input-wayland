#include <wayland-server-core.h>

struct xdg_surface {
	struct wl_resource *surface;
};

struct xdg_shell {
	struct wl_list surface_list;
};

void xdg_shell_new(struct wl_resource *resource);
