#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input-polldev.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/mod_devicetable.h>
#include <linux/slab.h>

#define WIINUNCHUK_JOY_FUZZ 4
#define WIINUNCHUK_JOY_FLAT 8

struct wiinunchuk_device {
	struct input_polled_dev *polled_input;
	struct i2c_client *i2c_client;
	int state;
};

static void wiinunchuk_poll(struct input_polled_dev *polled_input) {
	struct wiinunchuk_device *wiinunchuk = polled_input->private;
	struct i2c_client *i2c = wiinunchuk->i2c_client;
	uint8_t b[6];
	int jx, jy, ax, ay, az;
	bool c, z;

	switch (wiinunchuk->state) {
	case 0:
		i2c_smbus_write_byte(i2c, 0x00);
		msleep(10);
		wiinunchuk->state = 1;
		break;
	case 1:
		b[0] = i2c_smbus_read_byte(i2c);
		b[1] = i2c_smbus_read_byte(i2c);
		b[2] = i2c_smbus_read_byte(i2c);
		b[3] = i2c_smbus_read_byte(i2c);
		b[4] = i2c_smbus_read_byte(i2c);
		b[5] = i2c_smbus_read_byte(i2c);

		jx = b[0];
		jy = b[1];
		ax = (((int)b[2] << 2) | ((b[5] & 0xc) >> 2));
		ay = (((int)b[3] << 2) | ((b[5] & 0x30) >> 4));
		az = (((int)b[4] << 2) | ((b[5] & 0xc0) >> 6));
		c = !((b[5] & 0x2) >> 1);
		z = !(b[5] & 0x1);

		input_report_abs(polled_input->input, ABS_X, jx);
		input_report_abs(polled_input->input, ABS_Y, jy);
		/* input_report_abs(polled_input->input, ABS_RX, ax); */
		/* input_report_abs(polled_input->input, ABS_RY, ay); */
		/* input_report_abs(polled_input->input, ABS_RZ, az); */
		input_report_key(polled_input->input, BTN_C, c);
		input_report_key(polled_input->input, BTN_Z, z);
		input_sync(polled_input->input);

		msleep(10);

		wiinunchuk->state = 0;
		break;
	default:
		wiinunchuk->state = 0;
	}
}

static void wiinunchuk_open(struct input_polled_dev *polled_input) {
	struct wiinunchuk_device *wiinunchuk = polled_input->private;
	struct i2c_client *i2c = wiinunchuk->i2c_client;
	static uint8_t data1[2] = { 0xf0, 0x55 };
	static uint8_t data2[2] = { 0xfb, 0x00 };
	struct i2c_msg msg1 = { .addr = i2c->addr, .len = 2, .buf = data1 };
	struct i2c_msg msg2 = { .addr = i2c->addr, .len = 2, .buf = data2 };

	i2c_transfer(i2c->adapter, &msg1, 1);
	msleep(10);
	i2c_transfer(i2c->adapter, &msg2, 1);
	msleep(10);

	wiinunchuk->state = 0;
}

static int wiinunchuk_probe(struct i2c_client *client,
		const struct i2c_device_id *id) {
	struct wiinunchuk_device *wiinunchuk;
	struct input_polled_dev *polled_input;
	struct input_dev *input;
	int rc;

	printk("nunchuk probe");

	wiinunchuk = devm_kzalloc(&client->dev, sizeof(struct wiinunchuk_device), GFP_KERNEL);
	if (!wiinunchuk)
		return -ENOMEM;

	polled_input = input_allocate_polled_device();
	if (!polled_input) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	wiinunchuk->i2c_client = client;
	wiinunchuk->polled_input = polled_input;

	polled_input->private = wiinunchuk;
	polled_input->poll = wiinunchuk_poll;
	polled_input->poll_interval = 50;
	polled_input->open = wiinunchuk_open;

	input = polled_input->input;
	input->name = "Nintendo WiiNunchuk";
	input->id.bustype = BUS_I2C;
	input->dev.parent = &client->dev;

	set_bit(EV_ABS, input->evbit);
	set_bit(ABS_X, input->absbit);
	set_bit(ABS_Y, input->absbit);
	set_bit(ABS_RX, input->absbit);
	set_bit(ABS_RY, input->absbit);
	set_bit(ABS_RZ, input->absbit);

	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);

	input_set_abs_params(input, ABS_X, 30, 220,
			WIINUNCHUK_JOY_FUZZ, WIINUNCHUK_JOY_FLAT);
	input_set_abs_params(input, ABS_Y, 40, 200,
			WIINUNCHUK_JOY_FUZZ, WIINUNCHUK_JOY_FLAT);
	input_set_abs_params(input, ABS_RX, 0, 0x3ff,
			WIINUNCHUK_JOY_FUZZ, WIINUNCHUK_JOY_FLAT);
	input_set_abs_params(input, ABS_RY, 0, 0x3ff,
			WIINUNCHUK_JOY_FUZZ, WIINUNCHUK_JOY_FLAT);
	input_set_abs_params(input, ABS_RZ, 0, 0x3ff,
			WIINUNCHUK_JOY_FUZZ, WIINUNCHUK_JOY_FLAT);

	rc = input_register_polled_device(wiinunchuk->polled_input);
	if (rc) {
		printk("wiinunchuk: Failed to register input device\n");
		goto err_register;
	}

	i2c_set_clientdata(client, wiinunchuk);

	return 0;

err_register:
	input_free_polled_device(polled_input);
err_alloc:
	devm_kfree(&client->dev, wiinunchuk);

	return rc;
}

static int wiinunchuk_remove(struct i2c_client *client) {
	struct wiinunchuk_device *wiinunchuk = i2c_get_clientdata(client);

	i2c_set_clientdata(client, NULL);
	input_unregister_polled_device(wiinunchuk->polled_input);
	input_free_polled_device(wiinunchuk->polled_input);
	devm_kfree(&client->dev, wiinunchuk);

	return 0;
}

static const struct i2c_device_id wiinunchuk_id[] = {
	{"nunchuk", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, wiinunchuk_id);

#ifdef CONFIG_OF
static const struct of_device_id wiinunchuk_dt_ids[] = {
	{ .compatible = "nintendo,nunchuk", },
	{ }
};
MODULE_DEVICE_TABLE(of, wiinunchuk_dt_ids);
#endif

static struct i2c_driver wiinunchuk_driver = {
	.probe = wiinunchuk_probe,
	.remove = wiinunchuk_remove,
	.id_table = wiinunchuk_id,
	.driver = {
		.name = "nunchuk",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wiinunchuk_dt_ids),
	},
};

static int __init wiinunchuk_init(void) {
	printk("nunchuk init");
	return i2c_add_driver(&wiinunchuk_driver);
}

static void __exit wiinunchuk_exit(void) {
	printk("nunchuk exit");
	i2c_del_driver(&wiinunchuk_driver);
}

module_init(wiinunchuk_init);
module_exit(wiinunchuk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WiiNunchuk Driver");
MODULE_AUTHOR("Lee Youngjae");
