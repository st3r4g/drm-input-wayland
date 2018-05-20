#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libudev.h>
#include <linux/input.h>

struct input {
	int key_fd;
};

struct key_dev {
	char *devnode;
	char *name;
};

struct key_dev *find_keyboard_devices(int *count) {
	struct udev *udev = udev_new();
	struct udev_enumerate *enu = udev_enumerate_new(udev);
	udev_enumerate_add_match_property(enu, "ID_INPUT_KEYBOARD", "1");
	udev_enumerate_add_match_sysname(enu, "event*");
	udev_enumerate_scan_devices(enu);
	struct udev_list_entry *cur;
	*count = 0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		(*count)++;
	}
	struct key_dev *key_devs = calloc(*count, sizeof(struct key_dev));
	int i=0;
	udev_list_entry_foreach(cur, udev_enumerate_get_list_entry(enu)) {
		struct udev_device *dev = udev_device_new_from_syspath(udev,
		udev_list_entry_get_name(cur));
		const char *devnode = udev_device_get_devnode(dev);
		key_devs[i].devnode = malloc((strlen(devnode)+1)*sizeof(char));
		strcpy(key_devs[i].devnode, devnode);
		i++;
	}
	return key_devs;
}

void free_keyboard_devices(struct key_dev *key_devs, int count) {
	for (int i=0; i<count; i++) {
		free(key_devs[i].devnode);
	}
	free(key_devs);
}

struct input *input_setup() {
	int count;
	struct key_dev *key_devs = find_keyboard_devices(&count);
	int n;
	if (count > 1) {
		printf("Found multiple keyboards:\n");
		for (int i=0; i<count; i++) {
			printf("(%d) [%s]\n", i, key_devs[i].devnode);
		}
		printf("Choose one: ");
		scanf("%d", &n);
	} else {
		// Handle count == 0
		n = 0;
	}

	struct input *S = calloc(1, sizeof(struct input));
	S->key_fd = open(key_devs[n].devnode, O_RDWR|O_CLOEXEC|O_NOCTTY|O_NONBLOCK);
	if (S->key_fd < 0) {
		perror("open");
		return 0;
	}
	free_keyboard_devices(key_devs, count);

	ioctl(S->key_fd, EVIOCGRAB, 1);
	return S;
}

int input_get_key_fd(struct input *S) {
	return S->key_fd;
}

_Bool input_handle_event(struct input *S, unsigned int *key, unsigned int *state) {
	struct input_event ev;
	read(S->key_fd, &ev, sizeof(struct input_event));
	*key = ev.code;
	*state = ev.value > 0 ? 1 : 0;
	return ev.type == EV_KEY;
}

void input_release(struct input *S) {
	close(S->key_fd);
	free(S);
}
