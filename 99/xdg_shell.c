#define _POSIX_C_SOURCE 200809L
#include "xdg_shell.h"
#include "protocols/xdg-shell-unstable-v6-server-protocol.h"

#include <stdlib.h>

/* XDG TOPLEVEL */

static void xdg_toplevel_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void xdg_toplevel_set_parent(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *parent) {

}

static void xdg_toplevel_set_title(struct wl_client *client, struct wl_resource
*resource, const char *title) {

}

static void xdg_toplevel_set_app_id(struct wl_client *client, struct wl_resource
*resource, const char *app_id) {

}

static void xdg_toplevel_show_window_menu(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *seat, uint32_t serial, int32_t x,
int32_t y) {

}

static void xdg_toplevel_move(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *seat, uint32_t serial) {

}

static void xdg_toplevel_resize(struct wl_client *client, struct wl_resource
*resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {

}

static void xdg_toplevel_set_max_size(struct wl_client *client, struct
wl_resource *resource, int32_t width, int32_t height) {

}

static void xdg_toplevel_set_min_size(struct wl_client *client, struct
wl_resource *resource, int32_t width, int32_t height) {

}

static void xdg_toplevel_set_maximized(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_unset_maximized(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_set_fullscreen(struct wl_client *client, struct
wl_resource *resource, struct wl_resource *output) {

}

static void xdg_toplevel_unset_fullscreen(struct wl_client *client, struct
wl_resource *resource) {

}

static void xdg_toplevel_set_minimized(struct wl_client *client, struct
wl_resource *resource) {

}

static const struct zxdg_toplevel_v6_interface toplevel_impl = {
	.destroy = xdg_toplevel_destroy,
	.set_parent = xdg_toplevel_set_parent,
	.set_title = xdg_toplevel_set_title,
	.set_app_id = xdg_toplevel_set_app_id,
	.show_window_menu = xdg_toplevel_show_window_menu,
	.move = xdg_toplevel_move,
	.resize = xdg_toplevel_resize,
	.set_max_size = xdg_toplevel_set_max_size,
	.set_min_size = xdg_toplevel_set_min_size,
	.set_maximized = xdg_toplevel_set_maximized,
	.unset_maximized = xdg_toplevel_unset_maximized,
	.set_fullscreen = xdg_toplevel_set_fullscreen,
	.unset_fullscreen = xdg_toplevel_unset_fullscreen,
	.set_minimized = xdg_toplevel_set_minimized
};

/* XDG SURFACE */

static void xdg_surface_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void xdg_surface_get_toplevel(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {
	struct wl_resource *res = wl_resource_create(client, &zxdg_toplevel_v6_interface, 1, id);
	wl_resource_set_implementation(res, &toplevel_impl, 0, 0);
}

static void xdg_surface_get_popup(struct wl_client *client, struct wl_resource
*resource, uint32_t id, struct wl_resource *parent, struct wl_resource
*positioner) {

}

static void xdg_surface_set_window_geometry(struct wl_client *client, struct
wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height) {

}

static void xdg_surface_ack_configure(struct wl_client *client, struct
wl_resource *resource, uint32_t serial) {

}

static const struct zxdg_surface_v6_interface surface_impl = {
	.destroy = xdg_surface_destroy,
	.get_toplevel = xdg_surface_get_toplevel,
	.get_popup = xdg_surface_get_popup,
	.set_window_geometry = xdg_surface_set_window_geometry,
	.ack_configure = xdg_surface_ack_configure
};

/* XDG SHELL */

static void xdg_shell_destroy(struct wl_client *client, struct wl_resource
*resource) {

}

static void xdg_shell_create_positioner(struct wl_client *client, struct
wl_resource *resource, uint32_t id) {

}

static void xdg_shell_get_xdg_surface(struct wl_client *client, struct
wl_resource *resource, uint32_t id, struct wl_resource *surface) {
	struct wl_resource *res = wl_resource_create(client, &zxdg_surface_v6_interface, 1, id);
	wl_resource_set_implementation(res, &surface_impl, 0, 0);
	zxdg_surface_v6_send_configure(res, 0);
}

static void xdg_shell_pong(struct wl_client *client, struct wl_resource
*resource, uint32_t serial) {

}

static const struct zxdg_shell_v6_interface impl = {
	.destroy = xdg_shell_destroy,
	.create_positioner = xdg_shell_create_positioner,
	.get_xdg_surface = xdg_shell_get_xdg_surface,
	.pong = xdg_shell_pong
};

void xdg_shell_new(struct wl_resource *resource) {
	wl_resource_set_implementation(resource, &impl, 0, 0);
}
