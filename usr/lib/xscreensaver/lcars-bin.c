#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

Display *dpy = NULL;
pid_t child_pid = 0;
Window ruffle_window = 0;

void cleanup(int sig) {
	if (child_pid > 0) {
		kill(child_pid, SIGTERM);
		waitpid(child_pid, NULL, 0);
	}
	if (dpy) {
		XCloseDisplay(dpy);
	}
	exit(0);
}

Window find_window_by_pid(Window root, pid_t pid) {
	Atom pid_atom = XInternAtom(dpy, "_NET_WM_PID", True);
	if (!pid_atom) return 0;
	
	Window root_return, parent_return;
	Window *children;
	unsigned int nchildren;
	
	if (!XQueryTree(dpy, root, &root_return, &parent_return, &children, &nchildren)) {
		return 0;
	}
	
	for (unsigned int i = 0; i < nchildren; i++) {
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytes_after;
		unsigned char *prop = NULL;
		
		if (XGetWindowProperty(dpy, children[i], pid_atom, 0, 1, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop) {
			if (*(pid_t *)prop == pid) {
				Window result = children[i];
				XFree(prop);
				XFree(children);
				return result;
			}
			XFree(prop);
		}
		
		// Recurse into child windows
		Window w = find_window_by_pid(children[i], pid);
		if (w) {
			XFree(children);
			return w;
		}
	}
	
	if (children) {	
		XFree(children);
		return 0;
	}
}

int main(int argc, char **argv) {
	fprintf(stderr, "[INFO] Starting XScreenSaver LCARS b1.0\n");
	
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	
	char *parent_str = getenv("XSCREENSAVER_WINDOW");
	if (!parent_str) {
		fprintf(stderr, "[ERROR] XSCREENSAVER_WINDOW not set\n");
		return 1;
	}
	
	Window parent = strtol(parent_str, NULL, 0);
	dpy = XOpenDisplay(NULL);
	
	if (!dpy) {
		fprintf(stderr, "[ERROR] Cannot open X display\n");
		return 1;
	}
	
	XWindowAttributes xwa;
	int width = 0, height = 0;
	
	for (int i = 0; i < 50; i++) { // ~2.5 seconds
		if (XGetWindowAttributes(dpy, parent, &xwa)) {
			fprintf(stderr, "[INFO] Attempt %d: width=%d, height=%d, map_state=%d\n", i, xwa.width, xwa.height, xwa.map_state);
			if (xwa.map_state == IsViewable && xwa.width > 100 && xwa.height > 100) {
				width = xwa.width;
				height = xwa.height;
				break;
			}
		} else {
			fprintf(stderr, "[ERROR] Attempt %d: Failed to get window attributes\n", i);
		}
		usleep(1); // 50ms
	}
	
	// Launch ruffle
	child_pid = fork();
	if (child_pid == 0) {
		char width_str[16], height_str[16];
		snprintf(width_str, sizeof(width_str), "%d", width);
		snprintf(height_str, sizeof(height_str), "%d", height);
		setenv("NO_COLOR", "1", 1);
		char *args[] = {"ruffle", "--no-gui", "--graphics", "gl", "--width", width_str, "--height", height_str, "--scale", "exact-fit", "/usr/share/lcars/lcars.swf", NULL};
		execvp("ruffle", args);
		perror("[ERROR] Failed to exec ruffle");
		exit(1);
	}
	
	// Wait for ruffle window
	for (int i = 0; i < 100; i++) {
		ruffle_window = find_window_by_pid(DefaultRootWindow(dpy), child_pid);
		if (ruffle_window) break;
		usleep(1); // 100ms
	}
	
	if (!ruffle_window) {
		fprintf(stderr, "[ERROR] Could not find ruffle window\n");
		cleanup(0);
	}
    
	// Reparent and show the window
	XReparentWindow(dpy, ruffle_window, parent, 0, 0);
	XMapRaised(dpy, ruffle_window);
	XFlush(dpy);
	
	int paused = 0;
	XWindowAttributes attr;
	
	// Monitor screensaver window
	while (1) {
		if (!XGetWindowAttributes(dpy, parent, &attr)) {
			fprintf(stderr, "[INFO] Screensaver window is gone. Exiting.\n");
			break;
		}
		
		static int last_width = 0;
		static int last_height = 0;
		
		if (attr.width != last_width || attr.height != last_height) {
			last_width = attr.width;
			last_height = attr.height;
			
			// Resize ruffle window to match new parent size
			XResizeWindow(dpy, ruffle_window, last_width, last_height);
			XFlush(dpy);
		}
		
		if (attr.map_state == IsUnmapped) {
			// Screensaver password dialog timeout
			if (!paused) {
				XReparentWindow(dpy, ruffle_window, parent, 0, 0);
				XMapRaised(dpy, ruffle_window);
				XFlush(dpy);
			}
		} else {
			if (paused) {
				paused = 0;
				// Ensure it’s remapped in case it was unmapped
				XReparentWindow(dpy, ruffle_window, parent, 0, 0);
				XMapRaised(dpy, ruffle_window);
				XFlush(dpy);
			}
		}
		usleep(50000);
	}
	cleanup(0);
	return 0;
}
