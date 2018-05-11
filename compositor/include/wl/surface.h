#ifndef MYSURFACE_H
#define MYSURFACE_H

#include <wayland-server-core.h>

#include <util/box.h>

/*
 * The double-buffered state
 */

struct surface_state {
	struct wl_resource *buffer;
	struct box damage;
	struct box buffer_damage;
	struct wl_resource *opaque_region;
	struct wl_resource *input_region;
};

struct surface {
	struct surface_state *current, *pending;

	_Bool pending_buffer;

	struct egl *egl;
	struct texture *texture;

	struct wl_resource *frame;

	struct wl_signal commit;
};

struct surface *surface_new(struct wl_resource *resource, struct egl *egl);

#endif
