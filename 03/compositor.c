#define _POSIX_C_SOURCE 200809L
#include "compositor.h"
#include "surface.h"
#include "region.h"
#include <wayland-server-protocol.h>

#include <stdlib.h>

static void compositor_create_surface(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct wl_resource *surface_resource = wl_resource_create(client,
	&wl_surface_interface, 4, id);
	surface_new(surface_resource);
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

void compositor_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}

