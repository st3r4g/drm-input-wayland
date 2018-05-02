#define _POSIX_C_SOURCE 200809L
#include "output.h"
#include <wayland-server-protocol.h>

static void output_release(struct wl_client *client, struct wl_resource
*resource) {

}

static const struct wl_output_interface impl = {
	.release = output_release
};

void output_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
