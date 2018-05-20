#ifndef MYSCREEN_H
#define MYSCREEN_H

struct screen;

/*
 * The screen interface represents a physical screen where the rendering done by
 * the Wayland compositor is presented.
 */

struct screen *screen_setup();

void drm_handle_event(int fd, void (*)(int,unsigned
int,unsigned int, unsigned int, void*));

void screen_post(struct screen *, void *user_data);
int screen_get_gpu_fd(struct screen *);
struct gbm_device *screen_get_gbm_device(struct screen *);
struct gbm_surface *screen_get_gbm_surface(struct screen *);

void screen_release(struct screen *);

#endif
