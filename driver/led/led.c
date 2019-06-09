#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>

static int __init led_init(void) {
	printk(KERN_INFO "led : init\n");
	return 0;
}

static void __exit led_exit(void) {
	printk(KERN_INFO "led : exit\n");
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Driver");
MODULE_AUTHOR("Youngjae Lee");
