#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#define GPIO_LED 18
#define GPIO_BTN 23

static unsigned int num_irq;
static int status;
static int led_on;
static struct timer_list timer;

static irqreturn_t led_irq_handler(int irq, void *unuse)
{
	printk("button pressed");
	status = !status;
	return IRQ_HANDLED;
}

void timer_callback(unsigned long data)
{
	printk("timer");
	if (status)
		led_on = !led_on;
	else
		led_on = 0;
	gpio_set_value(GPIO_LED, led_on);
}

static int __init led_init(void)
{
	int ret;
	printk("led : init\n");

	status = 0;

	gpio_request(GPIO_LED, "ledred");
	led_on = 0;
	gpio_direction_output(GPIO_LED, led_on);

	gpio_request(GPIO_BTN, "button");
	gpio_direction_input(GPIO_BTN);
	gpio_set_debounce(GPIO_BTN, 200);

	num_irq = gpio_to_irq(GPIO_BTN);
	ret = request_irq(num_irq, led_irq_handler,
			IRQF_TRIGGER_RISING, "led_irq", NULL);

	setup_timer(&timer, timer_callback, 0);
	mod_timer(&timer, jiffies + msecs_to_jiffies(500));

	return 0;
}

static void __exit led_exit(void)
{
	gpio_set_value(GPIO_LED, 0);
	gpio_free(GPIO_LED);

	gpio_free(GPIO_BTN);

	free_irq(num_irq, NULL);

	del_timer(&timer);

	printk(KERN_INFO "led : exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Driver");
MODULE_AUTHOR("Youngjae Lee");
