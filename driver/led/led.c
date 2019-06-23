#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_LED 18
#define GPIO_SEN 23

static unsigned int num_irq;
static int led_on;

static irqreturn_t led_irq_handler(int irq, void *unuse)
{
	printk("irq start");
	led_on = !led_on;
	gpio_set_value(GPIO_LED, led_on);
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	int ret;
	printk("led : init\n");

	gpio_request(GPIO_LED, "ledred");
	led_on = 0;
	gpio_direction_output(GPIO_LED, led_on);

	gpio_request(GPIO_SEN, "sensor");
	gpio_direction_input(GPIO_SEN);
	gpio_set_debounce(GPIO_SEN, 300);

	num_irq = gpio_to_irq(GPIO_SEN);
	ret = request_irq(num_irq, led_irq_handler,
			IRQF_TRIGGER_RISING, "led_irq", NULL);

	return 0;
}

static void __exit led_exit(void)
{
	gpio_set_value(GPIO_LED, 0);
	gpio_free(GPIO_LED);

	gpio_free(GPIO_SEN);

	free_irq(num_irq, NULL);

	printk(KERN_INFO "led : exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Driver");
MODULE_AUTHOR("Youngjae Lee");
