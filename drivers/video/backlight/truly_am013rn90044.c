/*
 * Copyright (C) 2025 Yahia Shamaa <yehiashamaa987@gmail.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>

#include <video/mipi_display.h>
#include <mach/jz_dsim.h>

#define POWER_IS_ON(pwr)	((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)	((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)	((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

struct truly_am013rn90044_dev {
	struct device *dev;
	unsigned int id;
	unsigned int power;

	struct lcd_device *ld;
	struct backlight_device *bd;
	struct mipi_dsim_lcd_device *dsim_dev;
	struct lcd_platform_data    *ddi_pd;
	struct mutex lock;
};

#ifdef CONFIG_PM
static void truly_am013rn90044_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power);
#endif

// static void truly_am013rn90044_brightness_setting(struct truly_am013rn90044_dev *lcd, int brightness)
// {
// 	
// }

static int truly_am013rn90044_update_status(struct backlight_device *bd)
{
    struct truly_am013rn90044_dev *lcd = dev_get_drvdata(&bd->dev);
	int brightness = bd->props.brightness;
    
    unsigned char brightness_cmd_table[] = {
        0x15, 0x51, brightness, 0x00, 0x00, 0x00
    };

    int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	array_size = ARRAY_SIZE(brightness_cmd_table);
	ops->cmd_write(lcd_to_master(lcd), brightness_cmd_table, array_size);
	
    return 0;
}

static struct backlight_ops truly_am013rn90044_backlight_ops = {
	.update_status = truly_am013rn90044_update_status,
};

void truly_am013rn90044_nop(struct truly_am013rn90044_dev *lcd) /* nop */
{
	unsigned char data_to_send[] = {0x15, 0x00, 0x00, 0x00, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void truly_am013rn90044_sleep_in(struct truly_am013rn90044_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x10, 0x00, 0x00, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void truly_am013rn90044_sleep_out(struct truly_am013rn90044_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x11, 0x00, 0x00, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void truly_am013rn90044_display_on(struct truly_am013rn90044_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x29, 0x00, 0x00, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void truly_am013rn90044_display_off(struct truly_am013rn90044_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x28, 0x00, 0x00, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}


unsigned char truly_am013rn90044_cmd_list[] = {
    0x15, 0xFE, 0x05, 0x00, 0x00, 0x00,
    0x15, 0x05, 0x03, 0x00, 0x00, 0x00,
    0x15, 0xFE, 0x0A, 0x00, 0x00, 0x00,
    0x15, 0x29, 0x10, 0x00, 0x00, 0x00,
    0x15, 0xFE, 0x00, 0x00, 0x00, 0x00,
    0x15, 0x35, 0x00, 0x00, 0x00, 0x00,
    0x39, 0x05, 0x00, 0x2A, 0x00, 0x00,
    0x39, 0x05, 0x00, 0x2B, 0x00, 0x00,
};

static void truly_am013rn90044_panel_condition_setting(struct truly_am013rn90044_dev *lcd)
{
	int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);

	array_size = ARRAY_SIZE(truly_am013rn90044_cmd_list);
	ops->cmd_write(dsi, truly_am013rn90044_cmd_list, array_size);
	msleep(120);
	
    truly_am013rn90044_sleep_out(lcd);
    mdelay(200);
 	
	truly_am013rn90044_display_on(lcd);
	udelay(3000);
    truly_am013rn90044_display_on(lcd);

	return;
}

static void truly_am013rn90044_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_am013rn90044_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		mutex_lock(&lcd->lock);
		truly_am013rn90044_panel_condition_setting(lcd);
		lcd->power = FB_BLANK_UNBLANK;
		mutex_unlock(&lcd->lock);
	}

	return;
}

static int truly_am013rn90044_ioctl(struct mipi_dsim_lcd_device *dsim_dev, int cmd)
{
	struct truly_am013rn90044_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd) {
		pr_err(" truly_am013rn90044_ioctl get drv failed\n");
		return -EFAULT;
	}

	mutex_lock(&lcd->lock);
	switch (cmd) {
	case CMD_MIPI_DISPLAY_ON:
		truly_am013rn90044_display_on(lcd);
        udelay(120000);
#ifdef CONFIG_PM
		truly_am013rn90044_power_on(dsim_dev, POWER_ON_BL);
#endif
		break;
	default:
		break;
	}
	mutex_unlock(&lcd->lock);

	return 0;
}

static int truly_am013rn90044_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_am013rn90044_dev *lcd = NULL;
	struct backlight_properties props;

	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct truly_am013rn90044_dev), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate truly_am013rn90044_dev structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->ld = lcd_device_register("truly_am013rn90044_dev", lcd->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;
	lcd->bd = backlight_device_register("pwm-backlight.0", lcd->dev, lcd,
										&truly_am013rn90044_backlight_ops, &props);
	if (IS_ERR(lcd->bd)) {
		dev_err(lcd->dev, "failed to register 'pwm-backlight.0'.\n");
		return PTR_ERR(lcd->bd);
	}
	dev_set_drvdata(&dsim_dev->dev, lcd);
	dev_dbg(lcd->dev, "probed truly_am013rn90044_dev panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static void truly_am013rn90044_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct truly_am013rn90044_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		/* lcd power on */
		if (lcd->ddi_pd->power_on) {
			return lcd->ddi_pd->power_on(lcd->ld, power);
		}

		// if (power != POWER_ON_BL) {
		// 	/* lcd reset */
		// 	if (lcd->ddi_pd->reset) {
		// 		lcd->ddi_pd->reset(lcd->ld);
		// 	}
		// }
	}

	return;
}

static int truly_am013rn90044_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct truly_am013rn90044_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	lcd->ddi_pd->lcd_enabled = 0;

	mutex_lock(&lcd->lock);

	truly_am013rn90044_display_off(lcd);
	truly_am013rn90044_sleep_in(lcd);

    udelay(5000);
    truly_am013rn90044_power_on(dsim_dev, 0);

	mutex_unlock(&lcd->lock);

	return 0;
}
#else
#define truly_am013rn90044_suspend		NULL
#define truly_am013rn90044_resume			NULL
#define truly_am013rn90044_power_on		NULL
#endif


static struct mipi_dsim_lcd_driver truly_am013rn90044_dsim_ddi_driver = {
	.name = "truly_am013rn90044-lcd",
	.id   = 0,
	.set_sequence = truly_am013rn90044_set_sequence,
	.ioctl    = truly_am013rn90044_ioctl,
	.probe    = truly_am013rn90044_probe,
	.suspend  = truly_am013rn90044_suspend,
	.power_on = truly_am013rn90044_power_on,
};

static int truly_am013rn90044_init(void)
{
	mipi_dsi_register_lcd_driver(&truly_am013rn90044_dsim_ddi_driver);
	return 0;
}

static void truly_am013rn90044_exit(void)
{
	return;
}

module_init(truly_am013rn90044_init);
module_exit(truly_am013rn90044_exit);

MODULE_DESCRIPTION("Truley 1.7 360*360 MIPI LCD Driver");
MODULE_LICENSE("GPL");
