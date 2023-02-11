#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#define SCALE 1.35

struct abs_range {
    int x_min;
    int x_max;
    int x_range;
    int y_min;
    int y_max;
    int y_range;
};

int click_mouse(Display *dpy, struct input_event *ev) {
    int btn;

    if (ev->code == BTN_LEFT) {
        btn = 1;
    } else if (ev->code == BTN_RIGHT) {
        btn = 2;
    } else {
        return 1;
    }

    if (ev->value < 2) {
        printf("Click with value %d  button: %d\n", ev->value, btn);
        XTestFakeButtonEvent(dpy, btn, ev->value, CurrentTime);
    }
}

int move_mouse(Display *dpy, float posx, float posy, int screenw, int screenh) {
    float x, y;
    //x = (screenw * posx) * SCALE;
    //y = (screenh * posy) * SCALE;
    x = ((screenw * posx) - (screenw*0.3)) * SCALE;
    y = ((screenh * posy) - (screenh*0.1)) * SCALE;

    Window root = DefaultRootWindow(dpy);
    XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
    XFlush(dpy);
}

int handle_event(struct input_event *ev, Display *dpy, struct abs_range abs_range, float *posx, float *posy, int sw, int sh) {
    if (ev->type == EV_ABS) {
        if (ev->code == ABS_X) {
            *posx = (float) (ev->value - abs_range.x_min) / abs_range.x_range;
        } else if (ev->code == ABS_Y) {
            *posy = (float) (ev->value - abs_range.y_min) / abs_range.y_range;
        } 
    } else if (ev->type == EV_KEY) {
        click_mouse(dpy, ev);
    }
    move_mouse(dpy, *posx, *posy, sw, sh);

	return 0;
}


Display *get_resolution(int *width, int *height) {
    Display *dpy; int snum;
    if(!(dpy = XOpenDisplay(0)))
            fprintf(stderr, "cannot open display '%s'", XDisplayName(0));
    snum = DefaultScreen(dpy);
    *width = DisplayWidth(dpy, snum);
    *height = DisplayHeight(dpy, snum);
    return dpy;
}


XDevice *get_trackpad(Display *dpy) {
    XDeviceInfo *devices;
    XDevice *device; 

    Atom touchpad_type = 0;
    touchpad_type = XInternAtom(dpy, XI_TOUCHPAD, True);

    int ndevices = 10;
    devices = XListInputDevices(dpy, &ndevices);

    while (ndevices--) {
        if (devices[ndevices].type == touchpad_type) {
            device = XOpenDevice(dpy, devices[ndevices].id);
            if (!device) {
                fprintf(stderr, "Failed to open device '%s'.\n", devices[ndevices].name);
                break;
            }
            printf("Opened device '%s'.\n", devices[ndevices].name);
        }
    }

    XFreeDeviceList(devices);
    return device;
}

int main() {
    Display *dpy;
    XDevice *device;
    struct libevdev *dev;
    struct input_event ev;
    struct abs_range abs_range;

    float posx, posy;
    int sw, sh;

    int fd, rc;

    fd = open("/dev/input/event17", O_RDONLY|O_NONBLOCK);
    if (fd < 0)
       fprintf(stderr, "error: %d %s\n", errno, strerror(errno));
    rc = libevdev_new_from_fd(fd, &dev);
    if (rc < 0)
       fprintf(stderr, "error: %d %s\n", -rc, strerror(-rc));

    abs_range.x_min = libevdev_get_abs_minimum(dev, ABS_X);
    abs_range.x_max = libevdev_get_abs_maximum(dev, ABS_X);
    abs_range.y_min = libevdev_get_abs_minimum(dev, ABS_Y);
    abs_range.y_max = libevdev_get_abs_maximum(dev, ABS_Y);

    abs_range.x_range = abs_range.x_max - abs_range.x_min;
    abs_range.y_range = abs_range.y_max - abs_range.y_min;

    dpy = get_resolution(&sw, &sh);

    device = get_trackpad(dpy);

    do {
		struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ev);
		if (rc == LIBEVDEV_READ_STATUS_SYNC) {
			//while (rc == LIBEVDEV_READ_STATUS_SYNC) {
				//handle_event(&ev, dpy, abs_range, &posx, &posy, sw, sh);
				//rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
			//}
		} else if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            handle_event(&ev, dpy, abs_range, &posx, &posy, sw, sh);
        }
	} while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);
;
    return 0;
}
