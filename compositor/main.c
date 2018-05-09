#define _POSIX_C_SOURCE 200809L
#include <drm_fourcc.h>
#include <inttypes.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include "protocols/xdg-shell-unstable-v6-server-protocol.h"

#include "algebra.h"
#include "egl.h"
#include "renderer.h"

#include "compositor.h"
#include "output.h"
#include "seat.h"
#include "surface.h"
#include "xdg_shell.h"

// GBM formats are either GBM_BO_FORMAT_XRGB8888 or GBM_BO_FORMAT_ARGB8888
static const int COLOR_DEPTH = 24;
static const int BIT_PER_PIXEL = 32;
const char *GetError(EGLint error_code);

struct {
	uint32_t fb_id;
} plane_props_id;

struct backend {
	// INPUT
	int key_fd;

	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t old_fb_id;
	uint32_t fb_id[2];
	drmModeAtomicReq *req;

	// GBM
	struct gbm_device *gbm_device;
	struct gbm_surface *gbm_surface;
	struct gbm_bo *gbm_old_bo, *gbm_cur_bo;
	struct gbm_bo *gbm_bo;
};

struct server {
	struct wl_display *display;
	struct backend *backend;
	struct egl *egl;
	struct renderer *renderer;
	struct wl_list compositor_list;
	struct wl_list seat_list;
};

void render(struct server *server) {
	struct backend *B = server->backend;
	struct egl *egl = server->egl;
	struct renderer *R = server->renderer;
	static int i = 0;

/*	drmModeRes *res = drmModeGetResources(B->gpu_fd);
	printf("%d\n", res->count_fbs);
	for (int i=0; i<res->count_fbs; i++)
		printf("%d\n", res->fbs[i]);
	drmModeFreeResources(res);*/

	renderer_clear();
	
	struct compositor *compositor;
	wl_list_for_each(compositor, &server->compositor_list, link) {
		struct surface *surface;
		wl_list_for_each(surface, &compositor->surface_list, link) {
			renderer_tex_draw(R, surface->texture);
		}
	}
	if (egl_swap_buffers(egl) == EGL_FALSE) {
		fprintf(stderr, "eglSwapBuffers failed\n");
	}

	gbm_surface_release_buffer(B->gbm_surface, B->gbm_bo);

	B->gbm_bo = gbm_surface_lock_front_buffer(B->gbm_surface);
	if (!B->gbm_bo) printf("bad bo\n");

	struct timespec tp;
	unsigned int time;
	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);
//	printf("[%10.3f] done drawing\n", time / 1000.0);

	uint32_t width = gbm_bo_get_width(B->gbm_bo);
	uint32_t height = gbm_bo_get_height(B->gbm_bo);
	uint32_t stride = gbm_bo_get_stride(B->gbm_bo);
	uint32_t handle = gbm_bo_get_handle(B->gbm_bo).u32;

	if (!B->fb_id[i]) {
		drmModeAddFB(B->gpu_fd, width, height, COLOR_DEPTH,
		BIT_PER_PIXEL, stride, handle, &B->fb_id[i]);
	}

	if (drmModeAtomicAddProperty(B->req, B->plane_id,
	plane_props_id.fb_id, B->fb_id[i]) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if (drmModeAtomicCommit(B->gpu_fd, B->req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, server))
		fprintf(stderr, "atomic commit failed\n");
	
	i = !i;
}

// IMPORTANTE: all'entrata il vblank è iniziato, non finito

static void page_flip_handler(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
	struct server *server = user_data;
/*	struct timespec tp;
	unsigned int time;
	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);
	printf("[%10.3f] VBLANK\n", time / 1000.0);*/

/*	struct timespec tq;
	tq.tv_sec = 0;
	tq.tv_nsec = 3000000L;
	// Aspettare qualche millisecondo rimuove il tearing perchè sto usando
	// un solo buffer e stavo disegnando mentre ero ancora nel periodo di
	// vblank/scanout
	nanosleep(&tq, &tp);*/

	struct compositor *compositor;
	wl_list_for_each(compositor, &server->compositor_list, link) {
/*		TODO: IDEA -> liste con gli elementi da trattare!
		struct frame_callback *fc, *tmp;
		wl_list_for_each_safe(fc, tmp, &compositor->frame_callback_list, link) {
			unsigned int ms = tv_sec * 1000 + tv_usec / 1000;
			wl_callback_send_done(fc->frame, (uint32_t)ms);
			wl_resource_destroy(fc->frame);
			wl_list_remove(fc);
		}*/
		struct surface *surface;
		wl_list_for_each(surface, &compositor->surface_list, link) {
			if (surface->frame) {
				unsigned int ms = tv_sec * 1000 + tv_usec / 1000;
				wl_callback_send_done(surface->frame, (uint32_t)ms);
				wl_resource_destroy(surface->frame);
				surface->frame = 0;
			}
		}
	}

	render(server);
}

static int gpu_ev_handler(int gpu_fd, uint32_t mask, void *data) {
	drmEventContext ev_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = page_flip_handler,
	};
	drmHandleEvent(gpu_fd, &ev_context);
	return 0;
}

static int key_ev_handler(int key_fd, uint32_t mask, void *data) {
	struct server *server = data;
	struct input_event ev;
	read(key_fd, &ev, sizeof(struct input_event));
	if (ev.type == EV_KEY) {
		uint32_t state = ev.value > 0 ? 1 : 0;
		struct seat *seat;
		wl_list_for_each(seat, &server->seat_list, link) {
			wl_keyboard_send_key(seat->keyb, 0, 0, ev.code, state);
		}
		if (ev.code == KEY_ESC)
			wl_display_terminate(server->display);
	}
	return 0;
}

int drm_setup(struct backend *B);
int gbm_setup(struct backend *B);
int end(struct backend *B);

static void compositor_bind(struct wl_client *client, void *data, uint32_t
version, uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_compositor_interface, version, id);
	struct compositor *compositor = compositor_new(resource, server->egl);
	wl_list_insert(&server->compositor_list, &compositor->link);
}

static void seat_bind(struct wl_client *client, void *data, uint32_t version,
uint32_t id) {
	struct server *server = data;
	struct wl_resource *resource = wl_resource_create(client,
	&wl_seat_interface, version, id);
	struct seat *seat = seat_new(resource);
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
	struct wl_resource *resource = wl_resource_create(client,
	&zxdg_shell_v6_interface, version, id);
	xdg_shell_new(resource);
}

int main(int argc, char *argv[]) {
	struct server *server = calloc(1, sizeof(struct server));
	wl_list_init(&server->compositor_list);
	wl_list_init(&server->seat_list);

	server->display = wl_display_create();
	struct wl_display *D = server->display;
	wl_display_add_socket_auto(D);

	wl_global_create(D, &wl_compositor_interface, 4, server,
	compositor_bind);
	wl_global_create(D, &wl_seat_interface, 1, server, seat_bind);
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &zxdg_shell_v6_interface, 1, 0, xdg_shell_bind);

	server->backend = calloc(1, sizeof(struct backend));
	struct backend *B = server->backend;
	if (drm_setup(B))
		return 1;
	gbm_setup(B);
	
	server->egl = egl_setup(B->gbm_device, B->gbm_surface, D);

	server->renderer = renderer_setup();
	
	render(server);
	
	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, B->gpu_fd, WL_EVENT_READABLE, gpu_ev_handler, 0);
	wl_event_loop_add_fd(el, B->key_fd, WL_EVENT_READABLE, key_ev_handler,
	server);

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

	end(B);

	return 0;
}

int end(struct backend *B) {
	if (drmModeAtomicAddProperty(B->req, B->plane_id, plane_props_id.fb_id, B->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(B->gpu_fd, B->req, 0, 0))
		fprintf(stderr, "atomic commit failed\n");
	
	drmModeAtomicFree(B->req);
//	drmModeRmFB(B->gpu_fd, B->fb.id);
	close(B->key_fd);
	close(B->gpu_fd);
	return 0;
}

int drm_setup(struct backend *B) {
	B->gpu_fd = open("/dev/dri/card0", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (B->gpu_fd < 0) {
		perror("open /dev/dri/card0");
		return 1;
	}

	B->key_fd = open("/dev/input/event4", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (B->key_fd < 0) {
		perror("open /dev/input/event4");
		return 1;
	}

	if(drmSetClientCap(B->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return 1;
	}

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(B->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(B->gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id) {
			B->plane_id = plane->plane_id;
			B->old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);

	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(B->gpu_fd, B->plane_id, DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(B->gpu_fd, obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			plane_props_id.fb_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	B->req = drmModeAtomicAlloc();
	if (!B->req)
		fprintf(stderr, "atomic allocation failed\n");

	ioctl(B->key_fd, EVIOCGRAB, 1);
	return 0;
}

int gbm_setup(struct backend *B) {
	B->gbm_device = gbm_create_device(B->gpu_fd);
	if (!B->gbm_device) {
		fprintf(stderr, "gbm_create_device failed\n");
		return 0;
	}
	printf("\ngbm_create_device successful\n");

	int ret = gbm_device_is_format_supported(B->gbm_device,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!ret) {
		fprintf(stderr, "format unsupported\n");
		return 0;
	}
	printf("format supported\n");

	drmModeFB *old_fb = drmModeGetFB(B->gpu_fd, B->old_fb_id);
	uint32_t width = old_fb->width, height = old_fb->height;
	drmModeFreeFB(old_fb);

	B->gbm_surface = gbm_surface_create(B->gbm_device, width, height,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!B->gbm_surface) {
		fprintf(stderr, "gbm_surface_create failed\n");
		return 0;
	}
	printf("gbm_surface_create successful\n");
	
	return 0;
}
