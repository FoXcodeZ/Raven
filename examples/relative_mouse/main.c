/**
 * Raven example - relative_mouse
 * The relative mouse mode (Rwindow_set_relative_mouse): the pointer is locked to
 * the window, the cursor disappears, and the motion comes as
 * R_EVENT_MOUSE_RELATIVE to the event hook of the window, with no bounds of the
 * screen. A camera turns with those deltas; here two labels show the mode and
 * the sum of the deltas received.
 *
 *   M      turns the mode on and off
 *   Esc    turns it off
 *
 * The window system gives the cursor back when the window loses the focus, and
 * the motion resumes when it comes back, the mode staying on.
 */

#include "../common/example.h"

static Rid g_window;
static Rid g_mode_label;
static Rid g_sum_label;
static float g_sum_x, g_sum_y;
static int g_unsupported;

static void update_labels(void) {
	char text[128];
	if (g_unsupported) {
		snprintf(text, sizeof(text), "Mode: unsupported by this window system");
	} else {
		snprintf(text, sizeof(text), "Mode: %s", Rwindow_get_relative_mouse(g_window) ? "on" : "off");
	}
	Rlabel_set_text(g_mode_label, text);
	snprintf(text, sizeof(text), "Sum: %.1f, %.1f", (double)g_sum_x, (double)g_sum_y);
	Rlabel_set_text(g_sum_label, text);
}

static void set_mode(int enabled) {
	/* R_FALSE: the window system cannot lock the pointer, the mode stays off */
	g_unsupported = Rwindow_set_relative_mouse(g_window, enabled) ? R_FALSE : R_TRUE;
	update_labels();
	Rwindow_request_redraw(g_window);
}

static int on_event(Rid window, const REvent* event, void* user_data) {
	(void)window;
	(void)user_data;
	switch (event->type) {
		case R_EVENT_MOUSE_RELATIVE:
			/* what a camera would use: the movement since the previous event */
			g_sum_x += event->delta_x;
			g_sum_y += event->delta_y;
			update_labels();
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			if (event->key == R_KEY_M) {
				set_mode(!Rwindow_get_relative_mouse(window));
				return R_TRUE;
			}
			if (event->key == R_KEY_ESCAPE) {
				set_mode(R_FALSE);
				return R_TRUE;
			}
			break;
		default:
			break;
	}
	return R_FALSE;
}

int main(void) {
	Rid box, title, hint;

	if (!example_init("relative_mouse")) return 1;

	g_window = Rwindow_create("Relative mouse", 460, 240, R_WINDOW_DEFAULT);

	/* a vertical box centered in the window */
	box = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(box, 10.0f);
	Rslot_set_halign(box, R_HALIGN_CENTER);
	Rslot_set_valign(box, R_VALIGN_CENTER);
	Rwindow_set_content(g_window, box);

	title = Rwidget_add_child(box, Rlabel_create("Relative mouse mode"));
	Rlabel_set_text_size(title, 22.0f);
	Rslot_set_halign(title, R_HALIGN_CENTER);

	g_mode_label = Rwidget_add_child(box, Rlabel_create("Mode: off"));
	Rslot_set_halign(g_mode_label, R_HALIGN_CENTER);

	g_sum_label = Rwidget_add_child(box, Rlabel_create("Sum: 0.0, 0.0"));
	Rslot_set_halign(g_sum_label, R_HALIGN_CENTER);

	hint = Rwidget_add_child(box, Rlabel_create("M toggles the mode, Esc turns it off"));
	Rslot_set_halign(hint, R_HALIGN_CENTER);

	/* R_EVENT_MOUSE_RELATIVE only comes here: no widget is under a locked pointer */
	Rwindow_set_event_hook(g_window, on_event, NULL);
	update_labels();

	/* the loop belongs to the application: Raven draws one frame per call,
	 * and Rwait_frame sleeps until there is something to draw */
	if (!example_automated(g_window)) {
		while (Rwait_frame(-1.0)) {
		}
	}

	/* releases every window, widget and resource that is still alive */
	Rshutdown();
	return 0;
}
