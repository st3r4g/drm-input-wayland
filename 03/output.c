#define _POSIX_C_SOURCE 200809L
#include "output.h"
#include <wayland-server-protocol.h>

static void output_release(struct wl_client *client, struct wl_resource
*resource) {

}

static const struct wl_output_interface impl = {
	.release = output_release
};

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *res = wl_resource_create(client,
	&wl_output_interface, version, id);
	wl_resource_set_implementation(res, &impl, 0, 0);
}

void output_new(struct wl_display *D) {
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
}
