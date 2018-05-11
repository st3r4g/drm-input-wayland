#define _POSIX_C_SOURCE 200809L
#include <backend/screen.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// GBM formats are either GBM_BO_FORMAT_XRGB8888 or GBM_BO_FORMAT_ARGB8888
static const int COLOR_DEPTH = 24;
static const int BIT_PER_PIXEL = 32;

struct {
	uint32_t fb_id;
} plane_props_id;

struct screen {
	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t old_fb_id;
	uint32_t fb_id[2];
	drmModeAtomicReq *req;

	// GBM
	struct gbm_device *gbm_device;
	struct gbm_surface *gbm_surface;
	struct gbm_bo *gbm_bo;
};

int drm_setup(struct screen *);
int gbm_setup(struct screen *);

struct screen *screen_setup() {
	struct screen *screen = calloc(1, sizeof(struct screen));
	drm_setup(screen);
	gbm_setup(screen);
	return screen;
}

int drm_setup(struct screen *S) {
	S->gpu_fd = open("/dev/dri/card0", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (S->gpu_fd < 0) {
		perror("open /dev/dri/card0");
		return 1;
	}

	if(drmSetClientCap(S->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return 1;
	}

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(S->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(S->gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id) {
			S->plane_id = plane->plane_id;
			S->old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);

	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(S->gpu_fd, S->plane_id, DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(S->gpu_fd, obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			plane_props_id.fb_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	S->req = drmModeAtomicAlloc();
	if (!S->req)
		fprintf(stderr, "atomic allocation failed\n");

	return 0;
}

int gbm_setup(struct screen *S) {
	S->gbm_device = gbm_create_device(S->gpu_fd);
	if (!S->gbm_device) {
		fprintf(stderr, "gbm_create_device failed\n");
		return 0;
	}
	printf("\ngbm_create_device successful\n");

	int ret = gbm_device_is_format_supported(S->gbm_device,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!ret) {
		fprintf(stderr, "format unsupported\n");
		return 0;
	}
	printf("format supported\n");

	drmModeFB *old_fb = drmModeGetFB(S->gpu_fd, S->old_fb_id);
	uint32_t width = old_fb->width, height = old_fb->height;
	drmModeFreeFB(old_fb);

	S->gbm_surface = gbm_surface_create(S->gbm_device, width, height,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!S->gbm_surface) {
		fprintf(stderr, "gbm_surface_create failed\n");
		return 0;
	}
	printf("gbm_surface_create successful\n");
	
	return 0;
}

void drm_handle_event(int fd, void (*page_flip_handler)(int,unsigned
int,unsigned int, unsigned int, void*)) {
	drmEventContext ev_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = page_flip_handler,
	};
	drmHandleEvent(fd, &ev_context);
}

void screen_post(struct screen *S, void *user_data) {
	static int i = 0;

	gbm_surface_release_buffer(S->gbm_surface, S->gbm_bo);

	S->gbm_bo = gbm_surface_lock_front_buffer(S->gbm_surface);
	if (!S->gbm_bo) printf("bad bo\n");

/*	struct timespec tp;
	unsigned int time;
	clock_gettime(CLOCK_REALTIME, &tp);
	time = (tp.tv_sec * 1000000L) + (tp.tv_nsec / 1000);
	printf("[%10.3f] done drawing\n", time / 1000.0);*/

	uint32_t width = gbm_bo_get_width(S->gbm_bo);
	uint32_t height = gbm_bo_get_height(S->gbm_bo);
	uint32_t stride = gbm_bo_get_stride(S->gbm_bo);
	uint32_t handle = gbm_bo_get_handle(S->gbm_bo).u32;

	if (!S->fb_id[i]) {
		drmModeAddFB(S->gpu_fd, width, height, COLOR_DEPTH,
		BIT_PER_PIXEL, stride, handle, &S->fb_id[i]);
	}

	if (drmModeAtomicAddProperty(S->req, S->plane_id,
	plane_props_id.fb_id, S->fb_id[i]) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if (drmModeAtomicCommit(S->gpu_fd, S->req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, user_data))
		fprintf(stderr, "atomic commit failed\n");
	
	i = !i;
}

int screen_get_fd(struct screen *S) {
	return S->gpu_fd;
}

struct gbm_device *screen_get_gbm_device(struct screen *S) {
	return S->gbm_device;
}

struct gbm_surface *screen_get_gbm_surface(struct screen *S) {
	return S->gbm_surface;
}

void screen_release(struct screen *S) {
	if (drmModeAtomicAddProperty(S->req, S->plane_id, plane_props_id.fb_id, S->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(S->gpu_fd, S->req, 0, 0))
		fprintf(stderr, "atomic commit failed\n");
	
	drmModeAtomicFree(S->req);
//	drmModeRmFB(S->gpu_fd, S->fb.id);
	close(S->gpu_fd);
}
