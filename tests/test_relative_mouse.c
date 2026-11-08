/*
 * The relative mouse mode (Rwindow_set_relative_mouse): the pointer is locked
 * to the window, the cursor is hidden and the motion of the pointer comes as
 * R_EVENT_MOUSE_RELATIVE to the event hook of the window, never to a widget.
 */
#include "test.h"

/*
 * Not public API: the host platform, told that the window system cannot lock
 * the pointer (as the null platform, whose table entry is NULL). It makes the
 * platform table the core reads have no window_set_relative_mouse, which is
 * the path Rwindow_set_relative_mouse answers R_FALSE on.
 */
extern int rvn_host_platform_set_pointer_lock(int capable);

static struct {
	int relative_events;
	float delta_x, delta_y;
	int move_events;
	RCursor cursor;
	int cursor_window;
} g_seen;

static int observe(Rid window, const REvent* event, void* user) {
	(void)user;
	switch (event->type) {
		case R_EVENT_MOUSE_RELATIVE:
			g_seen.relative_events++;
			g_seen.delta_x = event->delta_x;
			g_seen.delta_y = event->delta_y;
			break;
		case R_EVENT_MOUSE_MOVE:
			g_seen.move_events++;
			break;
		default:
			break;
	}
	(void)window;
	return R_FALSE;
}

static void host_set_cursor(Rid window, RCursor cursor, void* user) {
	(void)user;
	g_seen.cursor = cursor;
	g_seen.cursor_window = window;
}

static void forget_seen(void) {
	g_seen.relative_events = 0;
	g_seen.delta_x = 0.0f;
	g_seen.delta_y = 0.0f;
	g_seen.move_events = 0;
}

/* A host window with the focus, laid out, whose events the test watches. */
static Rid relative_window(void) {
	Rid window = Rwindow_create("relative", 200, 100, 0);
	Rwindow_set_content(window, Rwidget_create(R_WIDGET_VBOX));
	Rwindow_set_event_hook(window, observe, NULL);
	Rwindow_show(window);
	Rhost_window_set_size(window, 200, 100, 1.0f);
	Rhost_inject_focus(window, R_TRUE);
	Rdraw_frame();
	forget_seen();
	return window;
}

static void relative_mouse_set_get(void) {
	Rid window = relative_window();
	CHECK(Rwindow_get_relative_mouse(window) == R_FALSE);
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_TRUE);
	CHECK(Rwindow_get_relative_mouse(window) == R_TRUE);
	/* the cursor is hidden while the mode is on */
	CHECK(g_seen.cursor == R_CURSOR_NONE);
	CHECK(g_seen.cursor_window == window);
	CHECK(Rwindow_set_relative_mouse(window, R_FALSE) == R_TRUE);
	CHECK(Rwindow_get_relative_mouse(window) == R_FALSE);
	/* and it comes back */
	CHECK(g_seen.cursor != R_CURSOR_NONE);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void relative_mouse_injection_reaches_hook(void) {
	Rid window = relative_window();
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_TRUE);
	Rwindow_inject_relative_motion(window, 3.0f, -2.0f);
	CHECK(g_seen.relative_events == 1);
	CHECK_NEAR(g_seen.delta_x, 3.0f);
	CHECK_NEAR(g_seen.delta_y, -2.0f);
	/* and it is not a pointer move */
	CHECK(g_seen.move_events == 0);
	Rwindow_inject_relative_motion(window, 0.5f, 1.5f);
	CHECK(g_seen.relative_events == 2);
	CHECK_NEAR(g_seen.delta_x, 0.5f);
	CHECK_NEAR(g_seen.delta_y, 1.5f);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void relative_mouse_disabled_ignores_injection(void) {
	Rid window = relative_window();
	Rwindow_inject_relative_motion(window, 3.0f, -2.0f);
	CHECK(g_seen.relative_events == 0);
	/* the same after turning the mode off again */
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_TRUE);
	CHECK(Rwindow_set_relative_mouse(window, R_FALSE) == R_TRUE);
	Rwindow_inject_relative_motion(window, 3.0f, -2.0f);
	CHECK(g_seen.relative_events == 0);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void relative_mouse_focus_loss_suspends(void) {
	Rid window = relative_window();
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_TRUE);
	/* another window of the user takes the keyboard: the pointer goes back to it */
	Rhost_inject_focus(window, R_FALSE);
	CHECK(Rwindow_get_relative_mouse(window) == R_TRUE);
	CHECK(g_seen.cursor != R_CURSOR_NONE);
	Rwindow_inject_relative_motion(window, 3.0f, -2.0f);
	CHECK(g_seen.relative_events == 0);
	/* back: the mode resumes on its own */
	Rhost_inject_focus(window, R_TRUE);
	CHECK(Rwindow_get_relative_mouse(window) == R_TRUE);
	CHECK(g_seen.cursor == R_CURSOR_NONE);
	Rwindow_inject_relative_motion(window, 3.0f, -2.0f);
	CHECK(g_seen.relative_events == 1);
	CHECK_NEAR(g_seen.delta_x, 3.0f);
	CHECK_NEAR(g_seen.delta_y, -2.0f);
	Rwindow_destroy(window);
	Rdraw_frame();
}

static void relative_mouse_disable_restores_pointer_events(void) {
	Rid window = relative_window();
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_TRUE);
	Rhost_inject_mouse_move(window, 40, 30, 0);
	CHECK(g_seen.move_events == 0); /* the pointer is locked: it has no place in the window */
	CHECK(Rwindow_get_relative_mouse(window) == R_TRUE);
	CHECK(Rwindow_set_relative_mouse(window, R_FALSE) == R_TRUE);
	Rhost_inject_mouse_move(window, 40, 30, 0);
	CHECK(g_seen.move_events == 1);
	CHECK(g_seen.relative_events == 0);
	Rdraw_frame();
	Rwindow_destroy(window);
	Rdraw_frame();
}

int main(void) {
	RHostPlatformDesc host;
	Rid window;
	memset(&host, 0, sizeof(host));
	host.set_cursor = host_set_cursor;
	Rhost_platform_register(&host);
	if (!test_init(4)) {
		printf("Raven did not start with the host platform\n");
		return 1;
	}
	RUN(relative_mouse_set_get);
	RUN(relative_mouse_injection_reaches_hook);
	RUN(relative_mouse_disabled_ignores_injection);
	RUN(relative_mouse_focus_loss_suspends);
	RUN(relative_mouse_disable_restores_pointer_events);
	Rshutdown();
	/* a window system without pointer lock: no window_set_relative_mouse in its table */
	rvn_host_platform_set_pointer_lock(R_FALSE);
	CHECK(test_init(4));
	window = Rwindow_create("locked out", 200, 100, 0);
	CHECK(Rwindow_set_relative_mouse(window, R_TRUE) == R_FALSE);
	CHECK(Rwindow_get_relative_mouse(window) == R_FALSE);
	Rwindow_destroy(window);
	Rshutdown();
	rvn_host_platform_set_pointer_lock(R_TRUE);
	Rhost_platform_register(NULL);
	return test_report("test_relative_mouse");
}
