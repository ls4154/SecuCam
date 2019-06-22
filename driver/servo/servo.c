#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define PWM_PERIOD 20000000 /* 20 ms */
#define MIN_DUTY_CYCLE 1000000
#define MAX_DUTY_CYCLE 2000000

#define GPIO_SERVO 22

static void set_angle(unsigned int angle);

static struct hrtimer htimer;
static ktime_t kt0, kt1;

static int state = 0;
static unsigned int value_angle;

static ssize_t servo_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", value_angle);
}

static ssize_t servo_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	int val;
	sscanf(buf, "%u", &val);
	set_angle(val);
	return count;
}


static struct kobject *servo_kobj;

static struct kobj_attribute servo_attr = 
__ATTR(servo0, 0660, servo_show, servo_store);

static struct attribute *servo_attrs[] = {
	&servo_attr.attr,
	NULL
};

static struct attribute_group servo_attr_group = {
	.attrs = servo_attrs
};

static enum hrtimer_restart function_timer(struct hrtimer *unused)
{
	if (state) {
		gpio_set_value(GPIO_SERVO, 0);
		hrtimer_forward_now(&htimer, kt0);
	} else {
		gpio_set_value(GPIO_SERVO, 1);
		hrtimer_forward_now(&htimer, kt1);
	}
	state = !state;
	return HRTIMER_RESTART;
}

static void set_cycle(unsigned int cycle)
{
	if (cycle > MAX_DUTY_CYCLE)
		cycle = MAX_DUTY_CYCLE;
	if (cycle < MIN_DUTY_CYCLE)
		cycle = MIN_DUTY_CYCLE;

	kt0 = ktime_set(0, PWM_PERIOD - cycle);
	kt1 = ktime_set(0, cycle);
}

static void set_angle(unsigned int angle)
{
	int cycle;


	if (angle > 180)
		angle = 180;

	value_angle = angle;

	cycle = angle * (MAX_DUTY_CYCLE - MIN_DUTY_CYCLE) / 180 + MIN_DUTY_CYCLE;
	
	set_cycle(cycle);
}

static int __init servo_init(void)
{
	printk("servo : init\n");

	state = 0;

	/* register */
	servo_kobj = kobject_create_and_add("servo_kobj", kernel_kobj);
	if (!servo_kobj)
		goto err;
	if (sysfs_create_group(servo_kobj, &servo_attr_group)) {
		kobject_put(servo_kobj);
		goto err;
	}

	/* init gpio */
	gpio_request(GPIO_SERVO, "servo");
	gpio_direction_output(GPIO_SERVO, 0);

	/* set ktime */
	set_angle(90);

	/* init hrtimer */
	hrtimer_init(&htimer, CLOCK_REALTIME, HRTIMER_MODE_REL);
	htimer.function = function_timer;
	hrtimer_start(&htimer, kt0, HRTIMER_MODE_REL);

	return 0;

err:
	printk("init error");
	return -1;
}

static void __exit servo_exit(void)
{
	gpio_set_value(GPIO_SERVO, 0);
	gpio_free(GPIO_SERVO);

	hrtimer_cancel(&htimer);

	kobject_put(servo_kobj);

	printk(KERN_INFO "servo : exit\n");
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Servo Driver");
MODULE_AUTHOR("Youngjae Lee");

