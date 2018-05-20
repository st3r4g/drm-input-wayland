struct input;

struct input *input_setup();
int input_get_key_fd(struct input *S);
_Bool input_handle_event(struct input *S, unsigned int *key, unsigned int *state);
void input_release(struct input *S);
