#include <wayland-server-core.h>

struct surface_state {
	uint32_t width, height;
	void *data;
};

struct surface {
	struct surface_state *current, *pending;
	struct wl_list link;
};

struct surface *surface_new(struct wl_client *client, uint32_t id);
void surface_free(struct surface *surface);
