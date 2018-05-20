#ifndef MYEGL_H
#define MYEGL_H

#include <EGL/egl.h>

struct egl;
struct gbm_device;
struct gbm_surface;
struct backend;
struct wl_display;
struct wl_resource;

/*
 * The egl interface sets up an EGL context from GBM specific information, and
 * then acts as a wrapper for some EGL calls used by the Wayland compositor.
 */

struct egl *egl_setup(struct gbm_device *gbm_device, struct gbm_surface
*gbm_surface, struct wl_display *D);
int egl_wl_buffer_has_egl(struct egl *egl, struct wl_resource *buffer);
EGLImage egl_create_image(struct egl *egl, struct wl_resource *buffer, EGLint
*width, EGLint *height);
void egl_destroy_image(struct egl *egl, EGLImage image);
EGLBoolean egl_swap_buffers(struct egl *egl);

#endif
