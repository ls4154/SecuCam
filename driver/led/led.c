#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define GPIO_LED 18
#define GPIO_BTN 23

static unsigned int num_irq;

static irqreturn_t led_irq_handler(int irq, void *unuse)
{
	printk("irq handler");
	return IRQ_HANDLED;
}

static int __init led_init(void)
{
	int ret;
	printk("led : init\n");

	gpio_request(GPIO_LED, "ledred");
	gpio_direction_output(GPIO_LED, 0);

	gpio_request(GPIO_BTN, "button");
	gpio_direction_input(GPIO_BTN);
	gpio_set_debounce(GPIO_BTN, 200);

	num_irq = gpio_to_irq(GPIO_BTN);
	ret = request_irq(num_irq, led_irq_handler,
			IRQF_TRIGGER_RISING, "led_irq", NULL);

	return 0;
}

static void __exit led_exit(void)
{
	gpio_direction_output(GPIO_LED, 0);
	gpio_free(GPIO_LED);

	gpio_free(GPIO_BTN);

	free_irq(num_irq, NULL);

	printk(KERN_INFO "led : exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Driver");
MODULE_AUTHOR("Youngjae Lee");
