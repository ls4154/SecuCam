#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define PWM_PERIOD 20000000 /* 20 ms */
#define MIN_DUTY_CYCLE (1000000 - 300000)
#define MAX_DUTY_CYCLE (2000000 + 300000)

#define GPIO_SERVO_PAN 22
#define GPIO_SERVO_TILT 10

struct servo_device {
	int state;
	int angle;
	int gpio_pin;
	struct hrtimer htimer;
	ktime_t kt0, kt1;
};

static ssize_t servo_pan_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t servo_pan_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count);
static ssize_t servo_tilt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t servo_tilt_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count);
static void set_angle(struct servo_device *dev, unsigned int angle);

static struct servo_device servo_pan = {
	.state = 0,
	.angle = 90,
	.gpio_pin = GPIO_SERVO_PAN
};
static struct servo_device servo_tilt = {
	.state = 0,
	.angle = 90,
	.gpio_pin = GPIO_SERVO_TILT
};
static struct kobject *servo_kobj;

static struct kobj_attribute servo_attr_p = 
__ATTR(pan, 0660, servo_pan_show, servo_pan_store);

static struct kobj_attribute servo_attr_t = 
__ATTR(tilt, 0660, servo_tilt_show, servo_tilt_store);

static struct attribute *servo_attrs[] = {
	&servo_attr_p.attr,
	&servo_attr_t.attr,
	NULL
};

static struct attribute_group servo_attr_group = {
	.attrs = servo_attrs
};

static enum hrtimer_restart f_timer_pan(struct hrtimer *unused)
{
	if (servo_pan.state) {
		gpio_set_value(servo_pan.gpio_pin, 0);
		hrtimer_forward_now(&servo_pan.htimer, servo_pan.kt0);
	} else {
		gpio_set_value(servo_pan.gpio_pin, 1);
		hrtimer_forward_now(&servo_pan.htimer, servo_pan.kt1);
	}
	servo_pan.state = !servo_pan.state;
	return HRTIMER_RESTART;
}

static enum hrtimer_restart f_timer_tilt(struct hrtimer *unused)
{
	if (servo_tilt.state) {
		gpio_set_value(servo_tilt.gpio_pin, 0);
		hrtimer_forward_now(&servo_tilt.htimer, servo_tilt.kt0);
	} else {
		gpio_set_value(servo_tilt.gpio_pin, 1);
		hrtimer_forward_now(&servo_tilt.htimer, servo_tilt.kt1);
	}
	servo_tilt.state = !servo_tilt.state;
	return HRTIMER_RESTART;
}


static int __init servo_init(void)
{
	printk("servo : init\n");

	/* register sysfs */
	servo_kobj = kobject_create_and_add("secucam_servo", NULL);
	if (!servo_kobj)
		goto err;
	if (sysfs_create_group(servo_kobj, &servo_attr_group)) {
		kobject_put(servo_kobj);
		goto err;
	}

	/* init gpio */
	gpio_request(GPIO_SERVO_PAN, "pan");
	gpio_direction_output(GPIO_SERVO_PAN, 0);
	gpio_request(GPIO_SERVO_TILT, "tilt");
	gpio_direction_output(GPIO_SERVO_TILT, 0);

	/* set ktime */
	set_angle(&servo_pan, 90);
	set_angle(&servo_tilt, 90);

	/* init hrtimer */
	hrtimer_init(&servo_pan.htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	servo_pan.htimer.function = f_timer_pan;
	hrtimer_start(&servo_pan.htimer, servo_pan.kt0, HRTIMER_MODE_REL);
	hrtimer_init(&servo_tilt.htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	servo_tilt.htimer.function = f_timer_tilt;
	hrtimer_start(&servo_tilt.htimer, servo_tilt.kt0, HRTIMER_MODE_REL);

	return 0;

err:
	printk("init error");
	return -1;
}

static void __exit servo_exit(void)
{
	gpio_set_value(GPIO_SERVO_PAN, 0);
	gpio_free(GPIO_SERVO_PAN);
	gpio_set_value(GPIO_SERVO_TILT, 0);
	gpio_free(GPIO_SERVO_TILT);

	hrtimer_cancel(&servo_pan.htimer);
	hrtimer_cancel(&servo_tilt.htimer);

	kobject_put(servo_kobj);

	printk(KERN_INFO "servo : exit\n");
}

static ssize_t servo_pan_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", servo_pan.angle);
}

static ssize_t servo_pan_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	unsigned int val;
	sscanf(buf, "%u", &val);
	set_angle(&servo_pan, val);
	return count;
}

static ssize_t servo_tilt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", servo_tilt.angle);
}

static ssize_t servo_tilt_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	unsigned int val;
	sscanf(buf, "%u", &val);
	set_angle(&servo_tilt, val);
	return count;
}

static void set_cycle(struct servo_device *dev, unsigned int cycle)
{
	if (cycle > MAX_DUTY_CYCLE)
		cycle = MAX_DUTY_CYCLE;
	if (cycle < MIN_DUTY_CYCLE)
		cycle = MIN_DUTY_CYCLE;

	dev->kt0 = ktime_set(0, PWM_PERIOD - cycle);
	dev->kt1 = ktime_set(0, cycle);
}

static void set_angle(struct servo_device *dev, unsigned int angle)
{
	int cycle;

	if (angle > 180)
		angle = 180;

	dev->angle = angle;

	cycle = angle * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) / 180 + MIN_DUTY_CYCLE;
	
	set_cycle(dev, cycle);
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Servo Driver");
MODULE_AUTHOR("Youngjae Lee");

