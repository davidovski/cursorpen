#!/usr/bin/env python
#
#  cursorpen.py
#
#  this is the original prototype I made for this idea, taking code from 
#  https://github.com/Wazzaps/fingerpaint and modifying to work across
#  the whole desktop. 
#  
#  Use of this is not recommended, as this program is slow and inefficient
#  compared to the C program, however it should still work.
#  

import sys
from pynput.mouse import Button, Controller
from screeninfo import get_monitors
import subprocess as sp
import evdev
import contextlib
import pyudev
import screeninfo

scale = 1.5

@contextlib.contextmanager
def lock_pointer(devname):
    sp.call(['xinput', 'disable', devname])
    try:
        yield
    finally:
        sp.call(['xinput', 'enable', devname])

def get_touchpad(udev):
    for device in get_touchpads(udev):
        dev_name = get_device_name(device).strip('"')
        print('Using touchpad:', dev_name, file=sys.stderr)
        try:
            return evdev.InputDevice(device.device_node), dev_name
        except PermissionError:
            permission_error()
    return None, None

def get_touchpads(udev):
    for device in udev.list_devices(ID_INPUT_TOUCHPAD='1'):
        if device.device_node is not None and device.device_node.rpartition('/')[2].startswith('event'):
            yield device


def get_device_name(dev):
    while dev is not None:
        name = dev.properties.get('NAME')
        if name:
            return name
        else:
            dev = next(dev.ancestors, None)



def move_mouse(posx, posy):
    x = (monitor.width * posx - monitor.width*0.4) * scale
    y = (monitor.height * posy - monitor.height*0.1) * scale
    #x = (monitor.width * posx) 
    #y = (monitor.height * posy)
    mouse.position = (x, y)

def loop(touchpad):
    x_absinfo = touchpad.absinfo(evdev.ecodes.ABS_X)
    y_absinfo = touchpad.absinfo(evdev.ecodes.ABS_Y)
    val_range = (x_absinfo.max - x_absinfo.min, y_absinfo.max - y_absinfo.min)

    posx, posy = 0, 0
    while True:
        event = touchpad.read_one()
        if event:
            #print(posx, posy)
            if event.type == evdev.ecodes.EV_ABS:
                if event.code == evdev.ecodes.ABS_X:
                    posx = (event.value - x_absinfo.min) / val_range[0]
                if event.code == evdev.ecodes.ABS_Y:
                    posy = (event.value - y_absinfo.min) / val_range[1]
            move_mouse(posx, posy)

            if event.type == evdev.ecodes.EV_KEY:
                if event.code == evdev.ecodes.BTN_TOUCH and event.value == 0:
                    mouse.click(Button.left, 1)
                if event.code == evdev.ecodes.BTN_LEFT:
                    if event.value:
                        mouse.press(Button.left)
                    else:
                        mouse.release(Button.left)
                if event.code == evdev.ecodes.BTN_RIGHT:
                    if event.value:
                        mouse.press(Button.right)
                    else:
                        mouse.release(Button.right)



mouse = Controller()
monitor = get_monitors()[0]
udev = pyudev.Context()
touchpad, devname = get_touchpad(udev)
print(touchpad.path)
if touchpad is None:
    print('No touchpad found', file=sys.stderr)
    exit(1)

with lock_pointer(devname):
    loop(touchpad)
