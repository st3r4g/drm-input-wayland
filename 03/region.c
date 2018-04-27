#define _POSIX_C_SOURCE 200809L
#include "region.h"
#include <wayland-server-protocol.h>

static void region_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void region_add(struct wl_client *client, struct wl_resource *resource,
int32_t x, int32_t y, int32_t width, int32_t height) {

}

static void region_subtract(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static const struct wl_region_interface impl = {
	.destroy = region_destroy,
	.add = region_add,
	.subtract = region_subtract
};

void region_new(struct wl_client *client, uint32_t id) {
	struct wl_resource *res = wl_resource_create(client,
	&wl_region_interface, 1, id);
	wl_resource_set_implementation(res, &impl, 0, 0);
}
