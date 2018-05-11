#define _POSIX_C_SOURCE 200809L
#include <wl/keyboard.h>
#include <xdg/xdg_shell.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

static const struct wl_keyboard_interface impl;

int create_file(off_t size) {
	static const char template[] = "/compositor-XXXXXX";
	const char *path;
	char *name;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template));
	if (!name)
		return -1;

	strcpy(name, path);
	strcat(name, template);

	int fd = mkstemp(name);
	if (fd >= 0) {
		long flags = fcntl(fd, F_GETFD);
		fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
		unlink(name);
	}

	free(name);

	if (fd < 0)
		return -1;

	do {
		ret = posix_fallocate(fd, 0, size);
	} while (ret == EINTR);
	if (ret != 0) {
		close(fd);
		errno = ret;
		return -1;
	}
	
	return fd;

}

int32_t create_xkb_keymap_fd(uint32_t *size) {
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!context) {
		printf("Cannot create XKB context\n");
	}
	
	struct xkb_rule_names rules = { 0 };
/*	rules.rules = config->rules;
	rules.model = config->model;
	rules.layout = config->layout;
	rules.variant = config->variant;
	rules.options = config->options;*/

	struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
	XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (keymap == NULL) {
		xkb_context_unref(context);
		printf("Cannot create XKB keymap\n");
		return -1;
	}

	char *keymap_str = NULL;
	keymap_str = xkb_keymap_get_as_string(keymap,
	XKB_KEYMAP_FORMAT_TEXT_V1);
	size_t keymap_size = strlen(keymap_str) + 1;
	int keymap_fd = create_file(keymap_size);
	if (keymap_fd < 0) {
		printf("creating a keymap file for %zu bytes failed\n", keymap_size);
		return -1;
	}
	void *ptr = mmap(NULL, keymap_size,
		PROT_READ | PROT_WRITE, MAP_SHARED, keymap_fd, 0);
	if (ptr == (void*)-1) {
		printf("failed to mmap() %zu bytes", keymap_size);
		return -1;
	}
	strcpy(ptr, keymap_str);
	free(keymap_str);

	xkb_keymap_unref(keymap);
	xkb_context_unref(context);

	*size = keymap_size;
	return keymap_fd;
}

void keyboard_new(struct wl_resource *resource, struct wl_client *client) {
	printf("aaa\n");
	wl_resource_set_implementation(resource, &impl, 0, 0);
	uint32_t size;
	int32_t fd = create_xkb_keymap_fd(&size);
	wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
	fd, size);
	if (wl_resource_get_version(resource) >=
	WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
		wl_keyboard_send_repeat_info(resource, 25, 600);
	printf("bbb\n");
}
