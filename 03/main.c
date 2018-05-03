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
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include "protocols/xdg-shell-unstable-v6-server-protocol.h"

#include "compositor.h"
#include "output.h"
#include "surface.h"
#include "xdg_shell.h"

struct {
	uint32_t fb_id;
} plane_props_id;

struct dumb_framebuffer {
	uint32_t id;     // DRM object ID
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t handle; // driver-specific handle
	uint64_t size;   // size of mapping

	uint8_t *data;   // mmapped data we can write to
};

struct drm {
	int gpu_fd;
	uint32_t plane_id, old_fb_id;
	struct dumb_framebuffer fb;
	drmModeAtomicReq *req;
};

struct input {
	int key_fd;
};

static enum wl_iterator_result for_each_resource_func(struct wl_resource *resource, void *user_data) {
	struct drm *drm = user_data;
	const char *class = wl_resource_get_class(resource);
	if (!strcmp(class, "wl_surface")) {
		struct surface *surface = wl_resource_get_user_data(resource);
		if (surface->current->buffer) {
			struct wl_shm_buffer *shm_buffer = wl_shm_buffer_get(surface->current->buffer);
			uint32_t img_w = wl_shm_buffer_get_width(shm_buffer);
			uint32_t img_h = wl_shm_buffer_get_height(shm_buffer);
			uint8_t *img = wl_shm_buffer_get_data(shm_buffer);
			for (size_t i=0; i<img_h; i++) {
				for (size_t j=0; j<img_w; j++) {
					drm->fb.data[i*drm->fb.stride+j*4+0] =
					img[i*img_w*4+j*4+0];
					drm->fb.data[i*drm->fb.stride+j*4+1] =
					img[i*img_w*4+j*4+1];
					drm->fb.data[i*drm->fb.stride+j*4+2] =
					img[i*img_w*4+j*4+2];
					drm->fb.data[i*drm->fb.stride+j*4+3] =
					img[i*img_w*4+j*4+3];
				}
			}
			wl_buffer_send_release(surface->current->buffer);
		}
		if (surface->frame) {
			struct timespec tp;
			clock_gettime(CLOCK_REALTIME, &tp);
			long msec = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
			wl_callback_send_done(surface->frame, (uint32_t)msec);
			wl_resource_destroy(surface->frame);
			surface->frame = 0;
		}
	}
	return WL_ITERATOR_CONTINUE;
}

unsigned int old_tv_sec, old_tv_usec, count = 0;

static void page_flip_handler(int gpu_fd, unsigned int sequence, unsigned int
tv_sec, unsigned int tv_usec, void *user_data) {
/*	struct timespec tp;
	unsigned int time;
	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);
	printf("[%10.3f] FRAME\n", time / 1000.0);*/

	void **user_data_array = user_data;
	struct wl_display *D = user_data_array[0];
	struct drm *drm = user_data_array[1];

	struct wl_client *client;
	struct wl_list *client_list = wl_display_get_client_list(D);
	wl_client_for_each(client, client_list) {
		wl_client_for_each_resource(client, for_each_resource_func, drm);
	}

	if (drmModeAtomicCommit(gpu_fd, drm->req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, user_data))
		fprintf(stderr, "atomic commit failed\n");
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
	struct wl_display *D = data;
	struct input_event ev;
	read(key_fd, &ev, sizeof(struct input_event));
	if (ev.type == EV_KEY && ev.value != 0)
		wl_display_terminate(D);
	return 0;
}

int start(struct drm *drm, struct input *input);
int end(struct drm *drm, struct input *input);
bool create_dumb_framebuffer(int drm_fd, uint32_t width, uint32_t height, struct dumb_framebuffer *fb);

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
	wl_global_create(D, &wl_output_interface, 3, 0, output_bind);
	wl_display_init_shm(D);
	wl_global_create(D, &zxdg_shell_v6_interface, 1, 0, xdg_shell_bind);

	struct drm drm;
	struct input input;

	if (start(&drm, &input))
		return 1;
	
	drm.req = drmModeAtomicAlloc();
	if (!drm.req)
		fprintf(stderr, "atomic allocation failed\n");
	if (drmModeAtomicAddProperty(drm.req, drm.plane_id, plane_props_id.fb_id, drm.fb.id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	void *user_data_array[] = {D, &drm};
	if (drmModeAtomicCommit(drm.gpu_fd, drm.req, DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, user_data_array)) {
		fprintf(stderr, "atomic commit failed\n");
		return 1;
	}

	struct wl_event_loop *el = wl_display_get_event_loop(D);
	wl_event_loop_add_fd(el, drm.gpu_fd, WL_EVENT_READABLE, gpu_ev_handler, 0);
	wl_event_loop_add_fd(el, input.key_fd, WL_EVENT_READABLE, key_ev_handler, D);

	pid_t pid = fork();
	if (!pid)
		execl("/bin/weston-simple-shm", "weston-simple-shm", (char*)0);

	wl_display_run(D);

	wl_display_destroy(D);

	if (end(&drm, &input))
		return 1;

	return 0;
}

int end(struct drm *drm, struct input *input) {
	if (drmModeAtomicAddProperty(drm->req, drm->plane_id, plane_props_id.fb_id, drm->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(drm->gpu_fd, drm->req, 0, 0))
		fprintf(stderr, "atomic commit failed\n");
	
	drmModeAtomicFree(drm->req);
	munmap(drm->fb.data, drm->fb.size);
	drmModeRmFB(drm->gpu_fd, drm->fb.id);
	struct drm_mode_destroy_dumb destroy = { .handle = drm->fb.handle };
	drmIoctl(drm->gpu_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	close(input->key_fd);
	close(drm->gpu_fd);
	return 0;
}

int start(struct drm *drm, struct input *input) {
	drm->gpu_fd = open("/dev/dri/card0", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (drm->gpu_fd < 0) {
		perror("open /dev/dri/card0");
		return 1;
	}

	input->key_fd = open("/dev/input/event4", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (input->key_fd < 0) {
		perror("open /dev/input/event4");
		return 1;
	}

	if(drmSetClientCap(drm->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return 1;
	}

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(drm->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(drm->gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id) {
			drm->plane_id = plane->plane_id;
			drm->old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);

	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(drm->gpu_fd, drm->plane_id, DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(drm->gpu_fd, obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			plane_props_id.fb_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	drmModeFB *old_fb = drmModeGetFB(drm->gpu_fd, drm->old_fb_id);
	uint32_t width = old_fb->width, height = old_fb->height;
	drmModeFreeFB(old_fb);

	create_dumb_framebuffer(drm->gpu_fd, width, height, &drm->fb);

	ioctl(input->key_fd, EVIOCGRAB, 1);
	return 0;
}


bool create_dumb_framebuffer(int drm_fd, uint32_t width, uint32_t height,
struct dumb_framebuffer *fb) {
	int ret;

	struct drm_mode_create_dumb create = {
		.width = width,
		.height = height,
		.bpp = 32,
	};

	ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
	if (ret < 0) {
		perror("DRM_IOCTL_MODE_CREATE_DUMB");
		return false;
	}

	fb->height = height;
	fb->width = width;
	fb->stride = create.pitch;
	fb->handle = create.handle;
	fb->size = create.size;

	uint32_t handles[4] = { fb->handle };
	uint32_t strides[4] = { fb->stride };
	uint32_t offsets[4] = { 0 };

	ret = drmModeAddFB2(drm_fd, width, height, DRM_FORMAT_XRGB8888,
		handles, strides, offsets, &fb->id, 0);
	if (ret < 0) {
		perror("drmModeAddFB2");
		goto error_dumb;
	}

	struct drm_mode_map_dumb map = { .handle = fb->handle };
	ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
	if (ret < 0) {
		perror("DRM_IOCTL_MODE_MAP_DUMB");
		goto error_fb;
	}

	fb->data = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		drm_fd, map.offset);
	if (!fb->data) {
		perror("mmap");
		goto error_fb;
	}

	memset(fb->data, 0x33, fb->size);
	return true;

error_fb:
	munmap(fb->data, fb->size);
	drmModeRmFB(drm_fd, fb->id);
error_dumb:
	;
	struct drm_mode_destroy_dumb destroy = { .handle = fb->handle };
	drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	return false;
}
