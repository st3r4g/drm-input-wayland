#define _POSIX_C_SOURCE 200809L
#include <wayland-server-protocol.h>

#include <backend/egl.h>
#include <backend/renderer.h>
#include <wl/surface.h>

#include <stdlib.h>
#include <stdio.h>

static void surface_destroy(struct wl_client *client, struct wl_resource
*resource) {
	wl_resource_destroy(resource);
}

static void surface_attach(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *buffer, int32_t x, int32_t y) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending_buffer = 1;
	surface->pending->buffer = buffer;
}

static void surface_damage(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->damage.x = x;
	surface->pending->damage.y = y;
	surface->pending->damage.width = width;
	surface->pending->damage.height = height;
}

static void surface_frame(struct wl_client *client, struct wl_resource
*resource, uint32_t callback) {
	struct surface *surface = wl_resource_get_user_data(resource);
	if (!surface->frame)
		surface->frame = wl_resource_create(client, &wl_callback_interface, 1, callback);
}

static void surface_set_opaque_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->opaque_region = region;

}

static void surface_set_input_region(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *region) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->input_region = region;
}

struct texture *tex_from_buffer(struct egl *egl, struct wl_resource *buffer,
uint32_t *buf_w, uint32_t *buf_h) {
	struct texture *tex;
	if (egl_wl_buffer_has_egl(egl, buffer)) {
		EGLint width, height;
		EGLImage image = egl_create_image(egl, buffer, &width,
		&height);
		*buf_w = (uint32_t)width;
		*buf_h = (uint32_t)height;
		tex = renderer_tex_from_egl_image(width, height, image);
		egl_destroy_image(egl, image);
	} else {
		struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(buffer);
		*buf_w = wl_shm_buffer_get_width(shm_buffer);
		*buf_h = wl_shm_buffer_get_height(shm_buffer);
		uint8_t *data = wl_shm_buffer_get_data(shm_buffer);
		tex = renderer_tex_from_data(*buf_w, *buf_h, data);
	}
	wl_buffer_send_release(buffer);
	return tex;
}

static void surface_commit(struct wl_client *client, struct wl_resource
*resource) {
	struct surface *surface = wl_resource_get_user_data(resource);
	
	if (surface->pending_buffer) {
		surface->pending_buffer = 0;
		surface->current->buffer = surface->pending->buffer;
		renderer_delete_tex(surface->texture);
		uint32_t buf_w, buf_h;
		if (surface->current->buffer) {
			surface->texture = tex_from_buffer(surface->egl,
			surface->current->buffer, &buf_w, &buf_h);
		} else {
			surface->texture = 0, buf_w = 0, buf_h = 0;
		}
	}

	surface->current->damage = surface->pending->damage;
	surface->current->buffer_damage = surface->pending->buffer_damage;

	surface->current->opaque_region = surface->pending->opaque_region;
	surface->current->input_region = surface->pending->input_region;
	
	wl_signal_emit(&surface->commit, 0);
}

static void surface_set_buffer_transform(struct wl_client *client, struct
wl_resource *resource, int32_t transform) {

}

static void surface_set_buffer_scale(struct wl_client *client, struct
wl_resource *resource, int32_t scale) {

}

static void surface_damage_buffer(struct wl_client *client, struct wl_resource
*resource, int32_t x, int32_t y, int32_t width, int32_t height) {
	struct surface *surface = wl_resource_get_user_data(resource);
	surface->pending->buffer_damage.x = x;
	surface->pending->buffer_damage.y = y;
	surface->pending->buffer_damage.width = width;
	surface->pending->buffer_damage.height = height;
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

struct surface *surface_new(struct wl_resource *resource, struct egl *egl) {
	struct surface *surface = calloc(1, sizeof(struct surface));
	surface->pending = calloc(1, sizeof(struct surface_state));
	surface->current = calloc(1, sizeof(struct surface_state));
	surface->egl = egl;
	wl_signal_init(&surface->commit);
	wl_resource_set_implementation(resource, &impl, surface, surface_free);
	return surface;
}
