/*
 * Copyright (C) 2025 Yahia Shamaa <yehiashamaa987@gmail.com>
 */


// Note: Only framebuffer is tested to work, pixel clk is a bit incorrect  

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>

#include <mach/jzfb.h>
#include <mach/jz_dsim.h>
#include "../board_base.h"

extern struct regulator_dev *regulator_to_rdev(struct regulator *regulator);
extern int ricoh61x_regulator_set_sleep_mode_power(struct regulator_dev *rdev, int power_on);

static struct regulator *lcd_vcc_reg = NULL;
static bool is_init = 0;


int truly_am013rn90044_reset(struct lcd_device *lcd)
{
	gpio_direction_output(GPIO_MIPI_RST_N, 1);
	msleep(50);
	gpio_direction_output(GPIO_MIPI_RST_N, 0);
	msleep(10);
	gpio_direction_output(GPIO_MIPI_RST_N, 1);
	mdelay(10);

	return 0;
}

void truly_am013rn90044_exit(void)
{
  return;
}


int truly_am013rn90044_init(struct lcd_device *lcd)
{
	int ret = 0;

	lcd_vcc_reg = regulator_get(NULL, VCC_LCD_2V8_NAME);
	if (IS_ERR(lcd_vcc_reg)) {
		dev_err(&lcd->dev, "failed to get regulator %s\n", VCC_LCD_2V8_NAME);
		return PTR_ERR(lcd_vcc_reg);
	}

	ret = gpio_request(98, "vdd en pin");
	if (ret) {
		dev_err(&lcd->dev,"can't request power enable pin\n");
		return ret;
	}
	ret = gpio_request(99, "mipi reset pin");
	if (ret) {
		dev_err(&lcd->dev,"can't request mipi reset pin\n");
		return ret;
	}


	is_init = 1;

	return ret;
}

int truly_am013rn90044_power_on(struct lcd_device *lcd, int enable)
{
	int ret;

	if(!is_init && truly_am013rn90044_init(lcd))
		return -EFAULT;

	if (enable == 1) {
		gpio_direction_output(99, 0);
		mdelay(50);
		ret = regulator_enable(lcd_vcc_reg);
		if (ret)
			printk(KERN_ERR "failed to enable lcd vcc reg\n");
			return 0;
		mdelay(50);
		gpio_direction_output(98, 1);
		mdelay(200);
		gpio_direction_output(99, 1);
		mdelay(100);
	} 
	else {
		/* quick action buttons to lcd display abnormal */

		if ( enable != 2 )
		{
			gpio_direction_output(99, 0);
			udelay(1000);
			gpio_direction_output(98, 0);
			udelay(5000);
			regulator_disable(lcd_vcc_reg);
		}
    	return 0;
	}

	return 0;
}

struct lcd_platform_data truly_am013rn90044_data = {
	.reset    = truly_am013rn90044_reset,
	.power_on = truly_am013rn90044_power_on,
	.lcd_enabled = 1, // lcd panel was enabled from uboot
};

struct mipi_dsim_lcd_device truly_am013rn90044_device={
	.name = "truly_am013rn90044-lcd",
	.id   = 0,
	.platform_data = &truly_am013rn90044_data,
};

unsigned long truly_am013rn90044_cmd_buf[]= {
	0x2C2C2C2C,
};

struct fb_videomode jzfb_videomode = {
	.name = "truly_am013rn90044-lcd",
	.refresh = 60,
	.xres = 360,
	.yres = 360,
	// TODO: Fix pixelclk
	.pixclock = KHZ2PICOS(10376),
	.left_margin  = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzdsi_data jzdsi_pdata = {
	.modes = &jzfb_videomode,
	.video_config.no_of_lanes = 1,
	.video_config.virtual_channel = 0,
	.video_config.color_coding = COLOR_CODE_24BIT,
	.video_config.video_mode = VIDEO_BURST_WITH_SYNC_PULSES,
	.video_config.receive_ack_packets = 0,	/* enable receiving of ack packets */
	.video_config.is_18_loosely = 0, /*loosely: R0R1R2R3R4R5__G0G1G2G3G4G5G6__B0B1B2B3B4B5B6, not loosely: R0R1R2R3R4R5G0G1G2G3G4G5B0B1B2B3B4B5*/
	.video_config.data_en_polarity = 1,

	.dsi_config.max_lanes = 1,
	.dsi_config.max_hs_to_lp_cycles = 100,
	.dsi_config.max_lp_to_hs_cycles = 40,
	.dsi_config.max_bta_cycles = 4095,
	.dsi_config.max_bps = 500, /* 500Mbps */
	.dsi_config.color_mode_polarity = 1,
	.dsi_config.shut_down_polarity = 1,
	.dsi_config.te_gpio = DSI_TE_GPIO,
	.dsi_config.te_irq_level = IRQF_TRIGGER_RISING,
};

struct jzfb_platform_data jzfb_pdata = {
	.name = "truly_am013rn90044-lcd",
	.num_modes = 1,
	.modes = &jzfb_videomode,
	.dsi_pdata = &jzdsi_pdata,

	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 24, /* Actually is 32 but driver maps 32 -> 24 for some reason */
	.width = 31,
	.height = 31,

	.smart_config.clkply_active_rising = 0,
	.smart_config.rsply_cmd_high = 0,
	.smart_config.csply_active_high = 0,
	.smart_config.write_gram_cmd = truly_am013rn90044_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(truly_am013rn90044_cmd_buf),
	.smart_config.bus_width = 8,
	.dither_enable = 1,
	.dither.dither_red   = 1,	/* 6bit */
	.dither.dither_green = 1,	/* 6bit */
	.dither.dither_blue  = 1,	/* 6bit */
};
