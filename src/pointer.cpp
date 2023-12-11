#include <assert.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-util.h>
#include "common.h"
#include "pointer.h"
#include "util.h"

#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"


static enum pointer_action
parse_action(const char *action)
{
	static const struct token actions[] = {
		{"click",  POINTER_ACTION_CLICK },
		{"motion", POINTER_ACTION_MOTION},
		{"move",   POINTER_ACTION_MOTION},
		{"scroll", POINTER_ACTION_SCROLL},
		{NULL, POINTER_ACTION_UNSPEC}
	};

	return matchtok(actions, action);
}

static uint32_t
parse_button(const char *button)
{
	static const struct token buttons[] = {
		{"left",    BTN_LEFT   },
		{"right",   BTN_RIGHT  },
		{"middle",  BTN_MIDDLE },
		{"extra",   BTN_EXTRA  }, 
		{"side",    BTN_SIDE   },
		{"forward", BTN_FORWARD},
		{"back",    BTN_BACK   },
		{NULL, 0}
	};

	return matchtok(buttons, button);
}

static void
parse_fixed(const char *d, wl_fixed_t *fixed)
{
	char *end;
	int val = strtod(d, &end);
	if (end == d || *end){
		die("Bad value: '%s'\n", d);
	} else {
		*fixed = wl_fixed_from_double(val);
	}
}


void
prepare_pointer(struct wlrctl *state, int argc, char *argv[])
{
	struct wlrctl_pointer_command *cmd = calloc(1, sizeof (struct wlrctl_pointer_command));
	assert(cmd);

	const char *action = argv[0];
	cmd->action = parse_action(action);
	switch (cmd->action) {
	case POINTER_ACTION_CLICK:
		if (argc < 2) {
			cmd->button = BTN_LEFT;
		} else {
			const char *button = argv[1];
			cmd->button = parse_button(button);
			if (!cmd->button) {
				die("Unknown button: '%s'\n", button);
			}
		}
		break;
	case POINTER_ACTION_MOTION:
		switch (argc) {
		case 1:
			break;
		case 2:
			parse_fixed(argv[1], &cmd->dx);
			break;
		case 3:
			parse_fixed(argv[1], &cmd->dx);
			parse_fixed(argv[2], &cmd->dy);
			break;
		default:
			die("Extra argument: '%s'\n", argv[3]);
		}
		break;
	case POINTER_ACTION_SCROLL:
		switch (argc) {
		case 1:
			break;
		case 2:
			parse_fixed(argv[1], &cmd->dy);
			break;
		case 3:
			parse_fixed(argv[1], &cmd->dy);
			parse_fixed(argv[2], &cmd->dx);
			break;
		default:
			die("Extra argument: '%s'\n", argv[3]);
		}
		break;
	case POINTER_ACTION_UNSPEC:
		die("Unknown pointer action: '%s'\n", action);
	}

	state->cmd = cmd;
	cmd->state = state;
}

static void
complete_pointer(void *data, struct wl_callback *callback, uint32_t serial)
{
	struct wlrctl *state = data;
	wl_callback_destroy(callback);
	state->running = false;
	destroy_pointer(state);
}

static struct wl_callback_listener completed_listener = {
	.done = complete_pointer
};

static void
pointer_press(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button)
{
	zwlr_virtual_pointer_v1_button(vptr, timestamp(), button, WL_POINTER_BUTTON_STATE_PRESSED);
	zwlr_virtual_pointer_v1_frame(vptr);
}

static void
pointer_release(struct zwlr_virtual_pointer_v1 *vptr, uint32_t button)
{
	zwlr_virtual_pointer_v1_button(vptr, timestamp(), button, WL_POINTER_BUTTON_STATE_RELEASED);
	zwlr_virtual_pointer_v1_frame(vptr);
}

static void
pointer_move(struct zwlr_virtual_pointer_v1 *vptr, wl_fixed_t dx, wl_fixed_t dy)
{
	if (!dx && !dy) {
		return;
	} else {
		zwlr_virtual_pointer_v1_motion(vptr, timestamp(), dx, dy);
		zwlr_virtual_pointer_v1_frame(vptr);
	}
}

static void
pointer_scroll(struct zwlr_virtual_pointer_v1 *vptr, wl_fixed_t dy, wl_fixed_t dx)
{
	if (!dx && !dy) {
		return;
	}
	if (dx) {
		zwlr_virtual_pointer_v1_axis_source(vptr, WL_POINTER_AXIS_SOURCE_FINGER);
		zwlr_virtual_pointer_v1_axis(vptr, timestamp(), WL_POINTER_AXIS_HORIZONTAL_SCROLL, dx);
	}
	if (dy) {
		zwlr_virtual_pointer_v1_axis_source(vptr, WL_POINTER_AXIS_SOURCE_FINGER);
		zwlr_virtual_pointer_v1_axis(vptr, timestamp(), WL_POINTER_AXIS_VERTICAL_SCROLL, dy);
	}
	zwlr_virtual_pointer_v1_frame(vptr);
	if (dx) {
		zwlr_virtual_pointer_v1_axis_source(vptr, WL_POINTER_AXIS_SOURCE_FINGER);
		zwlr_virtual_pointer_v1_axis_stop(vptr, timestamp(), WL_POINTER_AXIS_HORIZONTAL_SCROLL);
	}
	if (dy) {
		zwlr_virtual_pointer_v1_axis_source(vptr, WL_POINTER_AXIS_SOURCE_FINGER);
		zwlr_virtual_pointer_v1_axis_stop(vptr, timestamp(), WL_POINTER_AXIS_VERTICAL_SCROLL);
	}
	zwlr_virtual_pointer_v1_frame(vptr);
}

void
run_pointer(struct wlrctl *state)
{
	struct wlrctl_pointer_command *cmd = state->cmd;
	cmd->device =
	zwlr_virtual_pointer_manager_v1_create_virtual_pointer(
		state->vp_mgr, state->seat
	);
	switch (cmd->action) {
	case POINTER_ACTION_CLICK:
		pointer_press(cmd->device, cmd->button);
		pointer_release(cmd->device, cmd->button);
		break;
	case POINTER_ACTION_MOTION:
		pointer_move(cmd->device, cmd->dx, cmd->dy);
		break;
	case POINTER_ACTION_SCROLL:
		pointer_scroll(cmd->device, cmd->dy, cmd->dx);
		break;
	case POINTER_ACTION_UNSPEC:
		// Unreachable
		assert(false);
	}
	struct wl_callback *callback = wl_display_sync(state->display);
	wl_callback_add_listener(callback, &completed_listener, state);
}

void
destroy_pointer(struct wlrctl *state)
{
	struct wlrctl_pointer_command *cmd = state->cmd;
	zwlr_virtual_pointer_v1_destroy(cmd->device);
	free(cmd);
}
