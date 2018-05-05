#define _POSIX_C_SOURCE 200809L
#include "compositor.h"
#include "surface.h"
#include "region.h"
#include <wayland-server-protocol.h>

#include <stdlib.h>

static void compositor_create_surface(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct compositor *compositor = wl_resource_get_user_data(resource);
	struct wl_resource *surface_resource = wl_resource_create(client,
	&wl_surface_interface, 4, id);
	struct surface *surface = surface_new(surface_resource);
	compositor->surface_list;
	surface->link;
	wl_list_insert(&compositor->surface_list, &surface->link);
}

static void compositor_create_region(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct wl_resource *region_resource = wl_resource_create(client,
	&wl_region_interface, 1, id);
	region_new(region_resource);
}

static const struct wl_compositor_interface impl = {
	.create_surface = compositor_create_surface,
	.create_region = compositor_create_region
};

struct compositor *compositor_new(struct wl_resource *resource) {
	struct compositor *compositor = calloc(1, sizeof(struct compositor));
	wl_list_init(&compositor->surface_list);
	wl_resource_set_implementation(resource, &impl, compositor, 0);
	return compositor;
}

