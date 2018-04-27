#include <wayland-server-core.h>

struct xdg_shell {
	struct wl_global *global;
};

struct xdg_shell *xdg_shell_new(struct wl_display *D);
void xdg_shell_free(struct xdg_shell *xdg_shell);
