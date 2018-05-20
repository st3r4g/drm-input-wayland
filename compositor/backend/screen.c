#define _POSIX_C_SOURCE 200809L
#include <backend/screen.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// GBM formats are either GBM_BO_FORMAT_XRGB8888 or GBM_BO_FORMAT_ARGB8888
static const int COLOR_DEPTH = 24;
static const int BIT_PER_PIXEL = 32;

struct props {
	struct {
		uint32_t fb_id;
		uint32_t crtc_id;
	} plane;

	struct {
		uint32_t crtc_id;
	} conn;
};

struct screen {
	// DRM
	int gpu_fd;
	uint32_t plane_id;
	uint32_t props_plane_fb_id;
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

char *boot_gpu_devpath() {
	char *devpath;
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_sysattr(enu, "boot_vga", "1");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		udev_enumerate_unref(enu);
		enu = udev_enumerate_new(udev);
		udev_enumerate_add_match_parent(enu, dev);
		udev_enumerate_add_match_sysname(enu, "card[0-9]");
		udev_enumerate_scan_devices(enu);
		udev_device_unref(dev);
		udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
			dev = udev_device_new_from_syspath(udev,
			udev_list_entry_get_name(cur));
			const char *str = udev_device_get_devnode(dev);
			devpath = malloc((strlen(str)+1)*sizeof(char));
			strcpy(devpath, str);
			udev_device_unref(dev);
			udev_enumerate_unref(enu);
			break;
		}
		break;
	}
	return devpath;
}

void simple_modeset(struct screen *S, uint32_t crtc_id) {
	drmModePlaneRes *plane_res = drmModeGetPlaneResources(S->gpu_fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(S->gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id == crtc_id) {
			S->plane_id = plane->plane_id;
			S->old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);
}

static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn);
struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id);

drmModeCrtc *get_crtc_from_conn(int fd, drmModeConnector *conn) {
	if (conn->encoder_id) {
		drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoder_id);
		drmModeCrtc *crtc = drmModeGetCrtc(fd, enc->crtc_id);
		drmModeFreeEncoder(enc);
		return crtc;
	} else
		return 0;
}

int drm_setup(struct screen *S) {
	char *devpath = boot_gpu_devpath();
	S->gpu_fd = open(devpath, O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (S->gpu_fd < 0) {
		perror("open /dev/dri/card0");
		return 1;
	}
	free(devpath);

	if(drmSetClientCap(S->gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		return 1;
	}
	
	drmModeRes *res = drmModeGetResources(S->gpu_fd);
	int count = 0;
	for (int i=0; i<res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(S->gpu_fd,
		res->connectors[i]);
		if (conn->connection == DRM_MODE_CONNECTED) {
			count++;
		}
		drmModeFreeConnector(conn);
	}
	drmModeConnector **connected = malloc(count*sizeof(drmModeConnector));
	for (int i=0,j=0; i<res->count_connectors; i++) {
		drmModeConnector *conn = drmModeGetConnector(S->gpu_fd,
		res->connectors[i]);
		if (conn->connection == DRM_MODE_CONNECTED) {
			connected[j] = conn;
			j++;
		} else
			drmModeFreeConnector(conn);
	}
	drmModeFreeResources(res);
	int n;
	if (count > 1) {
		printf("Found multiple displays:\n");
		for (int i=0; i<count; i++) {
			printf("(%d) [%d]\n", i, connected[i]->connector_id);
		}
		printf("Choose one: ");
		scanf("%d", &n);
	} else {
		n = 0;
	}
	drmModeCrtc *crtc = get_crtc_from_conn(S->gpu_fd, connected[n]);

	simple_modeset(S, crtc->crtc_id);
	struct props props = find_prop_ids(S->gpu_fd, S->plane_id, connected[n]->connector_id);
	S->props_plane_fb_id = props.plane.fb_id;

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

	int ret = gbm_device_is_format_supported(S->gbm_device,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!ret) {
		fprintf(stderr, "format unsupported\n");
		return 0;
	}

	drmModeFB *old_fb = drmModeGetFB(S->gpu_fd, S->old_fb_id);
	uint32_t width = old_fb->width, height = old_fb->height;
	drmModeFreeFB(old_fb);

	S->gbm_surface = gbm_surface_create(S->gbm_device, width, height,
	GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!S->gbm_surface) {
		fprintf(stderr, "gbm_surface_create failed\n");
		return 0;
	}

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

	uint32_t width = gbm_bo_get_width(S->gbm_bo);
	uint32_t height = gbm_bo_get_height(S->gbm_bo);
	uint32_t stride = gbm_bo_get_stride(S->gbm_bo);
	uint32_t handle = gbm_bo_get_handle(S->gbm_bo).u32;

	if (!S->fb_id[i]) {
		drmModeAddFB(S->gpu_fd, width, height, COLOR_DEPTH,
		BIT_PER_PIXEL, stride, handle, &S->fb_id[i]);
	}
	if (drmModeAtomicAddProperty(S->req, S->plane_id,
	S->props_plane_fb_id, S->fb_id[i]) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if (drmModeAtomicCommit(S->gpu_fd, S->req, DRM_MODE_PAGE_FLIP_EVENT |
	DRM_MODE_ATOMIC_NONBLOCK, user_data))
		fprintf(stderr, "atomic commit failed\n");
	drmModeAtomicSetCursor(S->req, 0);
	
	i = !i;
}

int screen_get_gpu_fd(struct screen *S) {
	return S->gpu_fd;
}

struct gbm_device *screen_get_gbm_device(struct screen *S) {
	return S->gbm_device;
}

struct gbm_surface *screen_get_gbm_surface(struct screen *S) {
	return S->gbm_surface;
}

void screen_release(struct screen *S) {
	if (drmModeAtomicAddProperty(S->req, S->plane_id, S->props_plane_fb_id, S->old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(S->gpu_fd, S->req, 0, 0))
		fprintf(stderr, "atomic commit failed\n");
	
	drmModeAtomicFree(S->req);
//	drmModeRmFB(S->gpu_fd, S->fb.id);
	close(S->gpu_fd);
	free(S);
}

/*
 * from drm-kms(7)
 */
static int modeset_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn) {
	drmModeEncoder *enc;

	/* iterate all encoders of this connector */
	for (int i = 0; i < conn->count_encoders; ++i) {
		enc = drmModeGetEncoder(fd, conn->encoders[i]);
		if (!enc) {
			/* cannot retrieve encoder, ignoring... */
			continue;
		}

		/* iterate all global CRTCs */
		for (int j = 0; j < res->count_crtcs; ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->possible_crtcs & (1 << j)))
				continue;

			/* Here you need to check that no other connector
			* currently uses the CRTC with id "crtc". If you intend
			* to drive one connector only, then you can skip this
			* step. Otherwise, simply scan your list of configured
			* connectors and CRTCs whether this CRTC is already
			* used. If it is, then simply continue the search here. */
			drmModeFreeEncoder(enc);
			return res->crtcs[j];
		}
		drmModeFreeEncoder(enc);
	}
	/* cannot find a suitable CRTC */
	return -1;
}

struct props find_prop_ids(int fd, uint32_t plane_id, uint32_t conn_id) {
	struct props props;
	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(fd, plane_id,
	DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			props.plane.fb_id = prop->prop_id;
		if (!strcmp(prop->name, "CRTC_ID"))
			props.plane.crtc_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);
	obj_props = drmModeObjectGetProperties(fd, conn_id,
	DRM_MODE_OBJECT_CONNECTOR);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "CRTC_ID"))
			props.conn.crtc_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);
	return props;
}
