
struct screen;
struct screen *screen_setup();

void drm_handle_event(int fd, void (*page_flip_handler)(int,unsigned
int,unsigned int, unsigned int, void*));

void screen_post(struct screen *, void *user_data);
int screen_get_fd(struct screen *);
struct gbm_device *screen_get_gbm_device(struct screen *);
struct gbm_surface *screen_get_gbm_surface(struct screen *);

void screen_release(struct screen *);
