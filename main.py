from threading import Thread
import threading

import evdev
import sys
import time
import picamera
import datetime
import select
import os


angle_pan = 90
angle_tilt = 90

abs_x = 127
abs_y = 127

recording = False
lock = None
cam = None

def thread_pantilt(val):
    global angle_pan
    global angle_tilt
    print("pantilt start")

    cnt_x = 0
    cnt_y = 0

    while True:
        if abs_x < 60:
            cnt_x = 0
            angle_pan = min(angle_pan + 2, 180)
        elif abs_x < 100:
            cnt_x = 0
            angle_pan = min(angle_pan + 1, 180)
        elif abs_x < 160:
            cnt_x += 1
            angle_pan = angle_pan
        elif abs_x < 190:
            cnt_x = 0
            angle_pan = max(angle_pan - 1, 0)
        else:
            cnt_x = 0
            angle_pan = max(angle_pan - 2, 0)

        f = open("/sys/secucam_servo/pan","w")
        if cnt_x >= 20:
            f.write(str(-1))
        else:
            f.write(str(angle_pan))
        f.close()

        if abs_y < 60:
            cnt_y = 0
            angle_tilt = min(angle_tilt + 2, 180)
        elif abs_y < 100:
            cnt_y = 0
            angle_tilt = min(angle_tilt + 1, 180)
        elif abs_y < 160:
            cnt_y += 1
            angle_tilt = angle_tilt
        elif abs_y < 190:
            cnt_y = 0
            angle_tilt = max(angle_tilt - 1, 0)
        else:
            cnt_y = 0
            angle_tilt = max(angle_tilt - 2, 0)

        f = open("/sys/secucam_servo/tilt","w")
        if cnt_y >= 20:
            f.write(str(-1))
        else:
            f.write(str(angle_tilt))
        f.close()

        time.sleep(0.02)

def thread_sensor():
    global lock
    global recording
    print("sensor start")
    fd = os.open('/sys/secucam_sensor/sensor', os.O_RDONLY)
    while True:
        poller = select.poll()
        poller.register(fd, select.POLLPRI | select.POLLERR)

        os.read(fd, 100)
        os.lseek(fd, 0, os.SEEK_SET)

        ret = poller.poll(5000)
        if len(ret) > 0:
            desc,ev = ret[0]
            os.read(desc, 100)
            os.lseek(fd, 0, os.SEEK_SET)
            print("sensor")
            lock.acquire()
            if not recording:
                print("record start")
                now = datetime.datetime.now()
                now_str = now.strftime('%Y-%m-%d-%H:%M:%S')
                cam.resolution = (640, 480)
                cam.start_recording(output = now_str + '.h264')
                recording = True
                lock.release()
                time.sleep(10)
                lock.acquire()
                recording = False
                cam.stop_recording()
                lock.release()
                print("record stop")


def main():
    global lock
    global recording

    global abs_x
    global abs_y

    global cam

    t1 = Thread(target = thread_pantilt, args = (1,))
    t1.start()

    t2 = Thread(target = thread_sensor)
    t2.start()

    cam = picamera.PiCamera()

    dev = evdev.InputDevice('/dev/input/event4')

    lock = threading.Lock()

    for event in dev.read_loop():
        if event.type == evdev.ecodes.EV_KEY:
            #print(evdev.categorize(event))
            if event.value == 1:
                if event.code == 306: # C
                    pass
                elif event.code == 309: # Z
                    print("Z")
                    lock.acquire()
                    if recording:
                        print("record fin")
                        cam.stop_recording()
                        recording = False
                    else:
                        print("record start")
                        now = datetime.datetime.now()
                        now_str = now.strftime('%Y-%m-%d-%H:%M:%S')
                        cam.resolution = (640, 480)
                        cam.start_recording(output = now_str + '.h264')
                        recording = True
                    lock.release()

        elif event.type == evdev.ecodes.EV_ABS:
            if event.code == 0:
                abs_x = event.value
            else:
                abs_y = event.value

if __name__ == "__main__":
    main()
