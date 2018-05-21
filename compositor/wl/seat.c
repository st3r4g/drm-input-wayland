#define _POSIX_C_SOURCE 200809L
#include <util/log.h>
#include <wl/seat.h>
#include <wl/keyboard.h>
#include <wayland-server-protocol.h>
#include <stdlib.h>

static void seat_release(struct wl_client *client, struct wl_resource *resource)
{

}

static void seat_get_keyboard(struct wl_client *client, struct wl_resource
*resource, uint32_t id) {
	struct seat *seat = wl_resource_get_user_data(resource);
	struct wl_resource *keyboard_resource = wl_resource_create(client,
	&wl_keyboard_interface, wl_resource_get_version(resource), id);
	seat->keyb = keyboard_resource;
	keyboard_new(keyboard_resource, &seat->keyb);
}

static const struct wl_seat_interface impl = {
	.get_pointer = 0,
	.get_keyboard = seat_get_keyboard,
	.get_touch = 0,
	.release = seat_release
};

void seat_free(struct wl_resource *resource) {
	struct seat *seat = wl_resource_get_user_data(resource);
	wl_list_remove(&seat->link);
	errlog("seat_free");
}

struct seat *seat_new(struct wl_resource *resource) {
	struct seat *seat = calloc(1, sizeof(struct seat));
	wl_resource_set_implementation(resource, &impl, seat, seat_free);
	wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_KEYBOARD);
	return seat;
}
