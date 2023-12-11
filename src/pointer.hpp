#ifndef WLRCTL_DEV_POINTER_H
#define WLRCTL_DEV_POINTER_H

enum pointer_action {
	POINTER_ACTION_UNSPEC = 0,
	POINTER_ACTION_CLICK,
	POINTER_ACTION_MOTION,
	POINTER_ACTION_SCROLL,
};

struct wlrctl_pointer_command {
	enum pointer_action action;
	uint32_t button;
	wl_fixed_t dx, dy;

	struct zwlr_virtual_pointer_v1 *device;
	struct wlrctl *state;
};

void prepare_pointer(struct wlrctl *state, int argc, char *argv[]);
void run_pointer(struct wlrctl *state);
void destroy_pointer(struct wlrctl *state);

#endif
