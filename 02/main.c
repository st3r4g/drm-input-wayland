#include <drm_fourcc.h>
#include <inttypes.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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

bool create_dumb_framebuffer(int drm_fd, uint32_t width, uint32_t height, struct dumb_framebuffer *fb);
bool print_image_on_dumb_framebuffer(struct dumb_framebuffer *fb, char *img_file);

int main(int argc, char *argv[]) {
	int status = 0;
	// 1) open gpu file descriptor
	int gpu_fd = open("/dev/dri/card0", O_RDWR);
	if (gpu_fd < 0) {
		perror("open");
		return 1;
	}

	int key_fd = open("/dev/input/event4", O_RDONLY);
	if (key_fd < 0) {
		perror("open");
		return 1;
	}

	// 2) turn on ATOMIC capability
	if(drmSetClientCap(gpu_fd, DRM_CLIENT_CAP_ATOMIC, 1)) {
	// note: automatically turns on UNIVERSAL PLANES too
		fprintf(stderr, "ATOMIC MODESETTING UNSUPPORTED :C\n");
		status = 1;
		goto clean1;
	}
	// 3) retrieve current plane_id and plane_props_id.fb_id
	// note: also retrieve old_fb_id to restore it when we quit
	uint32_t plane_id, old_fb_id;

	drmModePlaneRes *plane_res = drmModeGetPlaneResources(gpu_fd);
	for (int i=0; i<plane_res->count_planes; i++) {
		drmModePlane *plane = drmModeGetPlane(gpu_fd, plane_res->planes[i]);
		if (plane->crtc_id) {
			plane_id = plane->plane_id;
			old_fb_id = plane->fb_id;
		}
		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(plane_res);

	drmModeObjectProperties *obj_props;
	obj_props = drmModeObjectGetProperties(gpu_fd, plane_id, DRM_MODE_OBJECT_PLANE);
	for (int i=0; i<obj_props->count_props; i++) {
		drmModePropertyRes *prop = drmModeGetProperty(gpu_fd, obj_props->props[i]);
		if (!strcmp(prop->name, "FB_ID"))
			plane_props_id.fb_id = prop->prop_id;
		drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(obj_props);

	// 4) get screen resolution from old_fb_id and create a new fb
	drmModeFB *old_fb = drmModeGetFB(gpu_fd, old_fb_id);
	uint32_t width = old_fb->width, height = old_fb->height;
	drmModeFreeFB(old_fb);

	struct dumb_framebuffer fb;
	create_dumb_framebuffer(gpu_fd, width, height, &fb);
	if (argc > 1 && argv[1])
		print_image_on_dumb_framebuffer(&fb, argv[1]);

	// 5) perform the modesetting
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	if (!req)
		fprintf(stderr, "atomic allocation failed\n");
	if (drmModeAtomicAddProperty(req, plane_id, plane_props_id.fb_id, fb.id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if (drmModeAtomicCommit(gpu_fd, req, 0, 0)) {
		fprintf(stderr, "atomic commit failed\n");
		status = 1;
		goto clean2;
	}

	ioctl(key_fd, EVIOCGRAB, 1);
	struct input_event ev;
	for (;;) {
		read(key_fd, &ev, sizeof(struct input_event));
		if (ev.type == EV_KEY && ev.value != 0)
			break;
	}

	// 6) we are done, restore the old_fb_id
	if (drmModeAtomicAddProperty(req, plane_id, plane_props_id.fb_id, old_fb_id) < 0)
		fprintf(stderr, "atomic add property failed\n");
	if(drmModeAtomicCommit(gpu_fd, req, 0, 0))
		fprintf(stderr, "atomic commit failed\n");
	
clean2:
	drmModeAtomicFree(req);
	munmap(fb.data, fb.size);
	drmModeRmFB(gpu_fd, fb.id);
	;
	struct drm_mode_destroy_dumb destroy = { .handle = fb.handle };
	drmIoctl(gpu_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
clean1:
	close(key_fd);
	close(gpu_fd);
	return status;
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

bool print_image_on_dumb_framebuffer(struct dumb_framebuffer *fb, char *img_file) {
	if (access(img_file, F_OK) == -1) {
		perror("print_image");
		return false;
	}
	char cmd[512];
	sprintf(cmd, "magick identify -format '%%[w]x%%[h]' \"%s\"", img_file);
	FILE *pf = popen(cmd, "r");
	char res[16];
	fgets(res, 16, pf);
	pclose(pf);
	char *w = strtok(res, "x"), *h = strtok(0, "x");
	uint32_t img_w = atoi(w), img_h = atoi(h);
	uint32_t width = img_w > fb->width ? fb->width : img_w;
	uint32_t height = img_h > fb->height ? fb->height : img_h;

	uint8_t *img = malloc(img_w*img_h*3*sizeof(uint8_t));
	sprintf(cmd, "magick stream -map rgb -depth 8 \"%s\" tmpimg.dat", img_file);
	system(cmd);
	FILE *ptr = fopen("tmpimg.dat", "r");
	fread(img, img_w*img_h*3*sizeof(uint8_t), 1, ptr);
	fclose(ptr);
	system("rm tmpimg.dat");

	for (int i=0; i<height; i++) {
		for (int j=0; j<width; j++) {
			fb->data[i*fb->stride+j*4+0] = img[i*img_w*3+j*3+2]; //B
			fb->data[i*fb->stride+j*4+1] = img[i*img_w*3+j*3+1]; //G
			fb->data[i*fb->stride+j*4+2] = img[i*img_w*3+j*3+0]; //R
			fb->data[i*fb->stride+j*4+3] = 0x00; //X
		}
	}

	free(img);
	return true;
}
