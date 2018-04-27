#define _POSIX_C_SOURCE 200809L
#include "compositor.h"
#include "surface.h"
#include "region.h"
#include <wayland-server-protocol.h>

#include <stdlib.h>

static void compositor_create_surface(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct compositor *compositor = wl_resource_get_user_data(resource);
	struct surface *surface = surface_new(client, id);
	wl_list_insert(&compositor->surfaces, &surface->link);
}

static void compositor_create_region(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct compositor *compositor = wl_resource_get_user_data(resource);
	region_new(client, id);
}

static const struct wl_compositor_interface impl = {
	.create_surface = compositor_create_surface,
	.create_region = compositor_create_region
};

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct compositor *compositor = data;
	struct wl_resource *res = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	wl_resource_set_implementation(res, &impl, compositor, 0);
}

struct compositor *compositor_new(struct wl_display *D) {
	struct compositor *compositor = calloc(1, sizeof(struct compositor));
	wl_global_create(D, &wl_compositor_interface, 4, compositor,
	compositor_bind);
	wl_list_init(&compositor->surfaces);
	return compositor;
}

void compositor_free(struct compositor *compositor) {
	free(compositor);
}
