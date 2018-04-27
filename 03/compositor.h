#include <wayland-server-core.h>

struct compositor {
	struct wl_list surfaces;
};

struct compositor *compositor_new(struct wl_display *D);
void compositor_free(struct compositor *compositor);
