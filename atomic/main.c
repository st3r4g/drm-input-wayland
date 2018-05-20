#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

struct props {
	struct {
		uint32_t fb_id;
		uint32_t crtc_id;
	} plane;

	struct {
		uint32_t crtc_id;
	} conn;
};

int main() {
	uint32_t plane_id, old_fb_id;
	struct props props;
	int fd = open("/dev/dri/card0", O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	
	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);
	for (size_t i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[i]);
		if (plane->crtc_id) {
			plane_id = plane->plane_id;
			old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);

	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(fd, plane_id,
	DRM_MODE_OBJECT_PLANE);
	for (size_t i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(fd,
		obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			props.plane.fb_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	for (;;) {
		drmModeAtomicReq *req = drmModeAtomicAlloc();
		printf("cursor: %d\n", drmModeAtomicGetCursor(req));
		if (drmModeAtomicAddProperty(req, plane_id, props.plane.fb_id,
		old_fb_id) < 0)
			perror("drmModeAtomicAddProperty");
		else
			printf("add ok\n");
		printf("cursor: %d\n", drmModeAtomicGetCursor(req));

		if (drmModeAtomicCommit(fd, req, DRM_MODE_PAGE_FLIP_EVENT, 0))
			perror("drmModeAtomicCommit");
		else
			printf("ok\n");
//		sleep(1);
//		drmModeAtomicSetCursor(req, 0);
		drmModeAtomicFree(req);
	}

	close(fd);
	return 0;
}
