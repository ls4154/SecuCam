#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define GPIO_LED 18
#define GPIO_SEN 23

static unsigned int num_irq;
static int led_on;

static struct kobject *kobj;

static ssize_t sensor_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf);
static ssize_t sensor_store(struct kobject *kobj, struct kobj_attribute *attr, char * buf, size_t count);

static struct kobj_attribute attr =
__ATTR(sensor, 0660, sensor_show, sensor_store);

static struct attribute *attrs[] = {
	&attr.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = attrs
};

static ssize_t sensor_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	return sprintf(buf, "%d\n", led_on);
}
static ssize_t sensor_store(struct kobject *kobj, struct kobj_attribute *attr, char * buf, size_t count)
{
	return count;
}

static irqreturn_t led_irq_top(int irq, void *unuse)
{
	printk("irq top\n");
	if (led_on)
		return IRQ_HANDLED;
	led_on = !led_on;
	gpio_set_value(GPIO_LED, led_on);
	return IRQ_WAKE_THREAD;
}
static irqreturn_t led_irq_bottom(int irq, void *unuse)
{
	msleep(5000);
	printk("irq bottom\n");
	led_on = !led_on;
	gpio_set_value(GPIO_LED, led_on);
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	int ret;
	printk("led : init\n");

	kobj = kobject_create_and_add("secucam_sensor", NULL);
	if (!kobj)
		goto err;
	if (sysfs_create_group(kobj, &attr_group)) {
		kobject_put(kobj);
		goto err;
	}

	gpio_request(GPIO_LED, "ledred");
	led_on = 0;
	gpio_direction_output(GPIO_LED, led_on);

	gpio_request(GPIO_SEN, "sensor");
	gpio_direction_input(GPIO_SEN);
	gpio_set_debounce(GPIO_SEN, 300);

	num_irq = gpio_to_irq(GPIO_SEN);
	ret = request_threaded_irq(num_irq, led_irq_top, led_irq_bottom,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, "led_irq", NULL);

	return 0;

err:
	printk("led : init error\n");
	return -1;
}

static void __exit led_exit(void)
{
	gpio_set_value(GPIO_LED, 0);
	gpio_free(GPIO_LED);

	gpio_free(GPIO_SEN);

	free_irq(num_irq, NULL);

	kobject_put(kobj);

	printk(KERN_INFO "led : exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Driver");
MODULE_AUTHOR("Youngjae Lee");
