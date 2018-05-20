#include <backend/egl.h>

#include <stdlib.h>
#include <stdio.h>

#include <EGL/eglext.h>
#include <EGL/eglmesaext.h>
#include <gbm.h>

struct egl {
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
};

static PFNEGLQUERYWAYLANDBUFFERWL eglQueryWaylandBufferWL = 0;
static PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL = 0;

void load_pointers() {
	eglBindWaylandDisplayWL = (PFNEGLBINDWAYLANDDISPLAYWL)
	eglGetProcAddress("eglBindWaylandDisplayWL");
	eglQueryWaylandBufferWL = (PFNEGLQUERYWAYLANDBUFFERWL)
	eglGetProcAddress("eglQueryWaylandBufferWL");
}

struct egl *egl_setup(struct gbm_device *gbm_device, struct gbm_surface
*gbm_surface, struct wl_display *D) {
	struct egl *egl = calloc(1, sizeof(struct egl));
	load_pointers();
	egl->display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA,
	gbm_device, NULL);
	egl->context = EGL_NO_CONTEXT;
	if (egl->display == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetPlatformDisplay failed\n");
	}

	EGLint major, minor;
	if (eglInitialize(egl->display, &major, &minor) == EGL_FALSE) {
		fprintf(stderr, "eglInitialize failed\n");
	}
	printf("EGL %i.%i initialized\n", major, minor);

	if (eglBindWaylandDisplayWL(egl->display, D) == EGL_FALSE) {
		fprintf(stderr, "eglBindWaylandDisplayWL failed\n");
	}
	
	if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
		fprintf(stderr, "eglBindAPI failed\n");
	}

// 	`size` is the upper value of the possible values of `matching`
	const int size = 1;
	int matching;
	const EGLint attrib_required[] = {
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_NATIVE_RENDERABLE, EGL_TRUE,
		EGL_NATIVE_VISUAL_ID, GBM_FORMAT_XRGB8888,
	//	EGL_NATIVE_VISUAL_TYPE, ? 
	EGL_NONE};

	EGLConfig *config = malloc(size*sizeof(EGLConfig));
	eglChooseConfig(egl->display, attrib_required, config, size, &matching);
	//printf("EGLConfig matching: %i (requested: %i)\n", matching, size);
	
	const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	
	egl->context = eglCreateContext(egl->display, *config, EGL_NO_CONTEXT, attribs);
	if (egl->context == EGL_NO_CONTEXT) {
		fprintf(stderr, "eglGetCreateContext failed\n");
	}
	egl->surface = eglCreatePlatformWindowSurface(egl->display, *config,
	gbm_surface, NULL);
	if (egl->surface == EGL_NO_SURFACE) {
		fprintf(stderr, "eglCreatePlatformWindowSurface failed\n");
	}

	if (eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context) == EGL_FALSE) {
		fprintf(stderr, "eglMakeCurrent failed\n");
	}

	return egl;
}

const char *GetError(EGLint error_code) {
	switch (error_code) {
	case EGL_SUCCESS:             return "EGL_SUCCESS";
	case EGL_NOT_INITIALIZED:     return "EGL_NOT_INITITALIZED";
	case EGL_BAD_ACCESS:          return "EGL_BAD_ACCESS";
	case EGL_BAD_ALLOC:           return "EGL_BAD_ALLOC";
	case EGL_BAD_ATTRIBUTE:       return "EGL_BAD_ATTRIBUTE";
	case EGL_BAD_CONTEXT:         return "EGL_BAD_CONTEXT";
	case EGL_BAD_CONFIG:          return "EGL_BAD_CONFIG";
	case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
	case EGL_BAD_DISPLAY:         return "EGL_BAD_DISPLAY";
	case EGL_BAD_SURFACE:         return "EGL_BAD_SURFACE";
	case EGL_BAD_MATCH:           return "EGL_BAD_MATCH";
	case EGL_BAD_PARAMETER:       return "EGL_BAD_PARAMETER";
	case EGL_BAD_NATIVE_PIXMAP:   return "EGL_BAD_NATIVE_PIXMAP";
	case EGL_BAD_NATIVE_WINDOW:   return "EGL_BAD_NATIVE_WINDOW";
	case EGL_CONTEXT_LOST:        return "EGL_CONTEXT_LOST";
	default:                      return "Unknown error";
	}
}

int egl_wl_buffer_has_egl(struct egl *egl, struct wl_resource *buffer) {
	EGLint texture_format;
	return eglQueryWaylandBufferWL(egl->display, buffer, EGL_TEXTURE_FORMAT,
	&texture_format);
}

EGLImage egl_create_image(struct egl *egl, struct wl_resource *buffer, EGLint
*width, EGLint *height) {
	eglQueryWaylandBufferWL(egl->display, buffer, EGL_WIDTH, width);
	eglQueryWaylandBufferWL(egl->display, buffer, EGL_WIDTH, height);
	EGLAttrib attribs = EGL_NONE;
	EGLImage image = eglCreateImage(egl->display, EGL_NO_CONTEXT,
	EGL_WAYLAND_BUFFER_WL, buffer, &attribs);
	if (image == EGL_NO_IMAGE) {
		printf("AHIAHAH\n");
	}
	return image;
}

void egl_destroy_image(struct egl *egl, EGLImage image) {
	eglDestroyImage(egl->display, image);
}

EGLBoolean egl_swap_buffers(struct egl *egl) {
	return eglSwapBuffers(egl->display, egl->surface);
}
