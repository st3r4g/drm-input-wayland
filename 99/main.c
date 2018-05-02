#define _POSIX_C_SOURCE 200809L

#include <unistd.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include "protocols/xdg-shell-unstable-v6-server-protocol.h"

#include "compositor.h"
#include "output.h"
#include "surface.h"
#include "xdg_shell.h"

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	compositor_new(resource);
}

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_output_interface, version, id);
	output_new(resource);
}

static void xdg_shell_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&zxdg_shell_v6_interface, version, id);
	xdg_shell_new(resource);
}

int main(int argc, char *argv[]) {
	struct wl_display *D = wl_display_create();
	wl_display_add_socket_auto(D);

	wl_global_create(D, &wl_compositor_interface, 4, 0, compositor_bind);
//	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &zxdg_shell_v6_interface, 1, 0, xdg_shell_bind);

	pid_t pid = fork();
	if (!pid)
		execl("/home/stefano/git-repos/weston/weston-simple-shm", "weston-simple-shm", (char*)0);

	wl_display_run(D);
	wl_display_destroy(D);
	return 0;
}
