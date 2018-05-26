#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

#include <backend/egl.h>
#include <backend/input.h>
#include <backend/screen.h>
#include <util/log.h>
#include <protocols/xdg-shell-unstable-v6-server-protocol.h>
#include <renderer.h>
#include <wl/compositor.h>
#include <wl/data_device_manager.h>
#include <wl/output.h>
#include <wl/seat.h>
#include <wl/keyboard.h>
#include <wl/surface.h> // da togliere
#include <xdg/xdg_shell.h>

struct server {
	struct wl_display *display;
	struct egl *egl;
	struct renderer *renderer;

	struct input *input;
	struct screen *screen;

	struct wl_list xdg_shell_list;
	struct wl_list seat_list;

	struct xdg_surface *focused;
};

void server_focus(struct xdg_surface *new) {
	struct server *S = new->server;
	if (S->focused)
		wl_keyboard_send_leave(S->focused->keyboard, 0, S->focused->surface);
	struct wl_array array;
	wl_array_init(&array);
	wl_keyboard_send_enter(new->keyboard, 0, new->surface, &array);
	S->focused = new;
}

void render(struct server *server) {
	renderer_clear();
	
	struct xdg_shell *xdg_shell;
	wl_list_for_each(xdg_shell, &server->xdg_shell_list, link) {
		struct xdg_surface *xdg_surface;
		wl_list_for_each(xdg_surface, &xdg_shell->xdg_surface_list, link) {
			struct surface *surface =
			wl_resource_get_user_data(xdg_surface->surface);
			renderer_tex_draw(server->renderer, surface->texture);
		}
	}
}

// IMPORTANTE: all'entrata il vblank è iniziato, non finito

static void page_flip_handler(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
	struct server *server = user_data;
/*	struct timespec tq;
	tq.tv_sec = 0;
	tq.tv_nsec = 3000000L;
	// Aspettare qualche millisecondo rimuove il tearing perchè sto usando
	// un solo buffer e stavo disegnando mentre ero ancora nel periodo di
	// vblank/scanout
	nanosleep(&tq, &tp);*/

	struct xdg_shell *xdg_shell;
	wl_list_for_each(xdg_shell, &server->xdg_shell_list, link) {
/*		TODO: IDEA -> liste con gli elementi da trattare!
		struct frame_callback *fc, *tmp;
		wl_list_for_each_safe(fc, tmp, &compositor->frame_callback_list, link) {
			unsigned int ms = tv_sec * 1000 + tv_usec / 1000;
			wl_callback_send_done(fc->frame, (uint32_t)ms);
			wl_resource_destroy(fc->frame);
			wl_list_remove(fc);
		}*/
		struct xdg_surface *xdg_surface;
		wl_list_for_each(xdg_surface, &xdg_shell->xdg_surface_list, link) {
			struct surface *surface =
			wl_resource_get_user_data(xdg_surface->surface);
			if (surface->frame) {
				unsigned int ms = tv_sec * 1000 + tv_usec / 1000;
				wl_callback_send_done(surface->frame, (uint32_t)ms);
				wl_resource_destroy(surface->frame);
				surface->frame = 0;
			}
		}
	}

	render(server);

	if (egl_swap_buffers(server->egl) == EGL_FALSE) {
		errlog("eglSwapBuffers failed");
	}

	screen_post(server->screen, server);

}

static int gpu_ev_handler(int fd, uint32_t mask, void *data) {
	drm_handle_event(fd, page_flip_handler);
	return 0;
}

static int key_ev_handler(int key_fd, uint32_t mask, void *data) {
	struct server *server = data;
	struct aaa aaa;
	if (input_handle_event(server->input, &aaa)) {
		struct seat *seat;
		wl_list_for_each(seat, &server->seat_list, link) {
			if (seat->keyb)
				keyboard_send(seat->keyb, &aaa);
		}
		if (aaa.key == 59) //F1
			wl_display_terminate(server->display);
	}
	return 0;
}

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	compositor_new(resource, server->egl);
}

static void data_device_manager_bind(struct wl_client *client, void *data,
uint32_t version, uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	data_device_manager_new(resource);
}


static void seat_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_seat_interface, version, id);
	struct seat *seat = seat_new(resource, server->input);
	wl_list_insert(&server->seat_list, &seat->link);
}

static void output_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct wl_resource *resource = wl_resource_create(client,
	&wl_output_interface, version, id);
	output_new(resource);
}

static void xdg_shell_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&zxdg_shell_v6_interface, version, id);
	struct xdg_shell *xdg_shell = xdg_shell_new(resource, server);
	wl_list_insert(&server->xdg_shell_list, &xdg_shell->link);
}

int main(int argc, char *argv[]) {
	struct server *server = calloc(1, sizeof(struct server));
	wl_list_init(&server->seat_list);
	wl_list_init(&server->xdg_shell_list);

	server->display = wl_display_create();
	struct wl_display *D = server->display;
	wl_display_add_socket_auto(D);

	wl_global_create(D, &wl_compositor_interface, 4, server,
	compositor_bind);
	wl_global_create(D, &wl_data_device_manager_interface, 1, 0,
	data_device_manager_bind);
	wl_global_create(D, &wl_seat_interface, 5, server, seat_bind);
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &zxdg_shell_v6_interface, 1, server,
	xdg_shell_bind);

	server->screen = screen_setup();
	if (!server->screen)
		return EXIT_FAILURE;
	server->input = input_setup();
	if (!server->input)
		return EXIT_FAILURE;
	server->egl = egl_setup(screen_get_gbm_device(server->screen),
	screen_get_gbm_surface(server->screen), D);
	if (!server->egl)
		return EXIT_FAILURE;
	server->renderer = renderer_setup();
	if (!server->renderer)
		return EXIT_FAILURE;

	render(server);
	
	if (egl_swap_buffers(server->egl) == EGL_FALSE) {
		errlog("eglSwapBuffers failed");
	}

	screen_post(server->screen, server);
	
	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, screen_get_gpu_fd(server->screen),
	WL_EVENT_READABLE, gpu_ev_handler, 0);
	wl_event_loop_add_fd(el, input_get_key_fd(server->input),
	WL_EVENT_READABLE, key_ev_handler, server);

	if (argc > 1) {
		pid_t pid = fork();
		if (!pid) {
			char client[64];
			sprintf(client, "/bin/%s", argv[1]);
			execl(client, argv[1], (char*)0);
		}
	}

	wl_display_run(D);
	
	wl_display_destroy(D);
	input_release(server->input);
	screen_release(server->screen);
	free(server);
	return 0;
}
