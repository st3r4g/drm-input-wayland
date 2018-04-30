#define _POSIX_C_SOURCE 200809L
#include "surface.h"
#include <wayland-server-protocol.h>

#include <stdio.h>
#include <stdlib.h>

static void surface_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void surface_attach(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *buffer, int32_t x, int32_t y) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer = buffer;
}

static void surface_damage(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static void surface_frame(struct wl_client *client, struct wl_resource
*resource, uint32_t callback) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->frame = wl_resource_create(client, &wl_callback_interface, 1, callback);
}

static void surface_set_opaque_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {

}

static void surface_set_input_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {

}

static void surface_commit(struct wl_client *client, struct wl_resource
*resource) {
	struct surface *surface = wl_resource_get_user_data(resource);
	// apply all pending state
	surface->current->buffer = surface->pending->buffer;
}

static void surface_set_buffer_transform(struct wl_client *client, struct
wl_resource *resource, int32_t transform) {

}

static void surface_set_buffer_scale(struct wl_client *client, struct
wl_resource *resource, int32_t scale) {

}

static void surface_damage_buffer(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static const struct wl_surface_interface impl = {
	.destroy = surface_destroy,
	.attach = surface_attach,
	.damage = surface_damage,
	.frame = surface_frame,
	.set_opaque_region = surface_set_opaque_region,
	.set_input_region = surface_set_input_region,
	.commit = surface_commit,
	.set_buffer_transform = surface_set_buffer_transform,
	.set_buffer_scale = surface_set_buffer_scale,
	.damage_buffer = surface_damage_buffer
};

void surface_free(struct wl_resource *resource) {
	struct surface *surface = wl_resource_get_user_data(resource);
	free(surface->pending);
	free(surface->current);
	free(surface);
}

void surface_new(struct wl_resource *resource) {
	struct surface *surface = calloc(1, sizeof(struct surface));
	surface->current = calloc(1, sizeof(struct surface_state));
	surface->pending = calloc(1, sizeof(struct surface_state));
	wl_resource_set_implementation(resource, &impl, surface, surface_free);
}
