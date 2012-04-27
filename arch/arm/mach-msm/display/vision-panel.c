/* linux/arch/arm/mach-msm/board-vision-panel.c
 *
 * Copyright (C) 2008 HTC Corporation.
 * Author: Jay Tu <jay_tu@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <mach/msm_fb-7x30.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <mach/panel_id.h>
#include <mach/debug_display.h>

#include "../board-vision.h"
#include "../devices.h"
#include "../proc_comm.h"

static struct vreg *V_LCMIO_1V8;
static struct vreg *V_LCMIO_2V8;

//static struct clk *axi_clk;

#define PWM_USER_DEF	 	142
#define PWM_USER_MIN		30
#define PWM_USER_DIM		9
#define PWM_USER_MAX		255

#define PWM_HITACHI_DEF		174
#define PWM_HITACHI_MIN		10
#define PWM_HITACHI_MAX		255

#define DEFAULT_BRIGHTNESS      PWM_USER_DEF

struct vreg *vreg_ldo19, *vreg_ldo20;
struct mddi_cmd {
        unsigned char cmd;
        unsigned delay;
        unsigned char *vals;
        unsigned len;
};

#define LCM_CMD(_cmd, _delay, ...)                              \
{                                                               \
        .cmd = _cmd,                                            \
        .delay = _delay,                                        \
        .vals = (u8 []){__VA_ARGS__},                           \
        .len = sizeof((u8 []){__VA_ARGS__}) / sizeof(u8)        \
}

static struct renesas_t {
	struct led_classdev lcd_backlight;
	struct msm_mddi_client_data *client_data;
	struct mutex lock;
	unsigned long status;
	int last_shrink_br;
} renesas;

static struct mddi_cmd tear[] = {
	LCM_CMD(0x44, 0, 0x01, 0x00, 0x00, 0x00,
                         0x90, 0x00, 0x00, 0x00)
};

static struct mddi_cmd vision_renesas_cmd[] = {
	LCM_CMD(0x2A, 0, 0x00, 0x00, 0x01, 0xDF),
	LCM_CMD(0x2B, 0, 0x00, 0x00, 0x03, 0x1F),
	LCM_CMD(0x36, 0, 0x00, 0x00, 0x00, 0x00),
	LCM_CMD(0x3A, 0, 0x55, 0x00, 0x00, 0x00),
};

static struct mddi_cmd vision_renesas_backlight_blank_cmd[] = {
	LCM_CMD(0xB9, 0, 0x00, 0x00, 0x00, 0x00,
			 0xff, 0x00, 0x00, 0x00,
			 0x04, 0x00, 0x00, 0x00,
			 0x08, 0x00, 0x00, 0x00,),
};

static struct mddi_cmd gama[] = {
           LCM_CMD(0xB0, 0, 0x04, 0x00, 0x00, 0x00),
           LCM_CMD(0xC1, 0, 0x43, 0x00, 0x00, 0x00,
                                 0x31, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00,
                                 0x21, 0x00, 0x00, 0x00,
                                 0x21, 0x00, 0x00, 0x00,
                                 0x32, 0x00, 0x00, 0x00,
                                 0x12, 0x00, 0x00, 0x00,
                                 0x28, 0x00, 0x00, 0x00,
                                 0x4A, 0x00, 0x00, 0x00,
                                 0x1E, 0x00, 0x00, 0x00,
                                 0xA5, 0x00, 0x00, 0x00,
                                 0x0F, 0x00, 0x00, 0x00,
                                 0x58, 0x00, 0x00, 0x00,
                                 0x21, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00
           ),
           LCM_CMD(0xC8, 0, 0x2D, 0x00, 0x00, 0x00,
                                 0x2F, 0x00, 0x00, 0x00,
                                 0x31, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x3E, 0x00, 0x00, 0x00,
                                 0x51, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x23, 0x00, 0x00, 0x00,
                                 0x16, 0x00, 0x00, 0x00,
                                 0x0B, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00,
                                 0x2D, 0x00, 0x00, 0x00,
                                 0x2F, 0x00, 0x00, 0x00,
                                 0x31, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x3E, 0x00, 0x00, 0x00,
                                 0x51, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x23, 0x00, 0x00, 0x00,
                                 0x16, 0x00, 0x00, 0x00,
                                 0x0B, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00
           ),
           LCM_CMD(0xC9, 0, 0x00, 0x00, 0x00, 0x00,
                                 0x0F, 0x00, 0x00, 0x00,
                                 0x18, 0x00, 0x00, 0x00,
                                 0x25, 0x00, 0x00, 0x00,
                                 0x33, 0x00, 0x00, 0x00,
                                 0x4D, 0x00, 0x00, 0x00,
                                 0x38, 0x00, 0x00, 0x00,
                                 0x25, 0x00, 0x00, 0x00,
                                 0x18, 0x00, 0x00, 0x00,
                                 0x11, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00,
                                 0x0F, 0x00, 0x00, 0x00,
                                 0x18, 0x00, 0x00, 0x00,
                                 0x25, 0x00, 0x00, 0x00,
                                 0x33, 0x00, 0x00, 0x00,
                                 0x4D, 0x00, 0x00, 0x00,
                                 0x38, 0x00, 0x00, 0x00,
                                 0x25, 0x00, 0x00, 0x00,
                                 0x18, 0x00, 0x00, 0x00,
                                 0x11, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00
           ),
           LCM_CMD(0xCA, 0, 0x27, 0x00, 0x00, 0x00,
                                 0x2A, 0x00, 0x00, 0x00,
                                 0x2E, 0x00, 0x00, 0x00,
                                 0x34, 0x00, 0x00, 0x00,
                                 0x3C, 0x00, 0x00, 0x00,
                                 0x51, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x24, 0x00, 0x00, 0x00,
                                 0x16, 0x00, 0x00, 0x00,
                                 0x0C, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00,
                                 0x27, 0x00, 0x00, 0x00,
                                 0x2A, 0x00, 0x00, 0x00,
                                 0x2E, 0x00, 0x00, 0x00,
                                 0x34, 0x00, 0x00, 0x00,
                                 0x3C, 0x00, 0x00, 0x00,
                                 0x51, 0x00, 0x00, 0x00,
                                 0x36, 0x00, 0x00, 0x00,
                                 0x24, 0x00, 0x00, 0x00,
                                 0x16, 0x00, 0x00, 0x00,
                                 0x0C, 0x00, 0x00, 0x00,
                                 0x02, 0x00, 0x00, 0x00,
                                 0x01, 0x00, 0x00, 0x00 ),
           LCM_CMD(0xD5, 0, 0x14, 0x00, 0x00, 0x00,
                                 0x14, 0x00, 0x00, 0x00
           ),
           LCM_CMD(0xB0, 0, 0x03, 0x00, 0x00, 0x00),
};

static void
do_renesas_cmd(struct msm_mddi_client_data *client_data, struct mddi_cmd *cmd_table, ssize_t size)
{
	struct mddi_cmd *pcmd = NULL;
	for (pcmd = cmd_table; pcmd < cmd_table + size; pcmd++) {
		client_data->remote_write_vals(client_data, pcmd->vals,
			pcmd->cmd, pcmd->len);
		if (pcmd->delay)
			hr_msleep(pcmd->delay);
	}
}

//int color_enhance_switch = 1;

enum {
	GATE_ON = 1 << 0,
};

enum led_brightness brightness_value = DEFAULT_BRIGHTNESS;

extern unsigned long msm_fb_base;

static int
vision_shrink_pwm(int brightness, int user_def,
		int user_min, int user_max, int panel_def,
		int panel_min, int panel_max)
{
	if (brightness < PWM_USER_DIM)
		return 0;

	if (brightness < user_min)
		return panel_min;

	if (brightness > user_def) {
		brightness = (panel_max - panel_def) *
			(brightness - user_def) /
			(user_max - user_def) +
			panel_def;
	} else {
			brightness = (panel_def - panel_min) *
			(brightness - user_min) /
			(user_def - user_min) +
			panel_min;
	}

	return brightness;
}

static void
vision_set_brightness(struct led_classdev *led_cdev,
				enum led_brightness val)
{
	static int enable_fade_on = 1;
	struct msm_mddi_client_data *client = renesas.client_data;
	unsigned int shrink_br = val;

	if (test_bit(GATE_ON, &renesas.status) == 0)
		return;

	shrink_br = vision_shrink_pwm(val, PWM_USER_DEF,
				PWM_USER_MIN, PWM_USER_MAX, PWM_HITACHI_DEF,
				PWM_HITACHI_MIN, PWM_HITACHI_MAX);

	if (!client) {
		PR_DISP_INFO("null mddi client");
		return;
	}

	if (renesas.last_shrink_br == shrink_br) {
		PR_DISP_INFO("[BKL] identical shrink_br");
		return;
	}

	mutex_lock(&renesas.lock);

	if (enable_fade_on) {
		client->remote_write(client, 0x2C, 0x5300);
		/* Trigger it once only */
		enable_fade_on = 0;
	}
	client->remote_write(client, shrink_br, 0x5100);

	/* Update the last brightness */
	if (renesas.last_shrink_br==0 && shrink_br) enable_fade_on = 1;
	renesas.last_shrink_br = shrink_br;
	brightness_value = val;
	mutex_unlock(&renesas.lock);

	PR_DISP_INFO("[BKL] set brightness to %d(enable_fade_on=%d)\n", shrink_br, enable_fade_on);
}

static enum led_brightness
vision_get_brightness(struct led_classdev *led_cdev)
{

	return brightness_value;
}

static void
vision_backlight_switch(struct msm_mddi_client_data *client_data, int on)
{
	enum led_brightness val;

	if (on) {
		PR_DISP_DEBUG("[BKL] turn on backlight\n");
		set_bit(GATE_ON, &renesas.status);
		val = renesas.lcd_backlight.brightness;
		/*LED core uses get_brightness for default value
		If the physical layer is not ready, we should not count on it*/
		if (val == 0)
			val = brightness_value;
		vision_set_brightness(&renesas.lcd_backlight, val);
	} else {
		do_renesas_cmd(client_data, vision_renesas_backlight_blank_cmd, ARRAY_SIZE(vision_renesas_backlight_blank_cmd));
		vision_set_brightness(&renesas.lcd_backlight, 0);
		clear_bit(GATE_ON, &renesas.status);
		renesas.last_shrink_br = 0;
	}
}
/*
static int
vision_ce_switch(int on)
{
	struct msm_mddi_client_data *client = cabc.client_data;

	if (on) {
		printk(KERN_DEBUG "turn on color enhancement\n");
		client->remote_write(client, 0x10, 0xb400);
	} else {
		printk(KERN_DEBUG "turn off color enhancement\n");
		client->remote_write(client, 0x00, 0xb400);
	}
	color_enhance_switch = on;
	return 1;
}

static ssize_t
ce_switch_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t
ce_switch_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count);
#define Color_enhance_ATTR(name) __ATTR(name, 0644, ce_switch_show, ce_switch_store)
static struct device_attribute ce_attr = Color_enhance_ATTR(color_enhance);

static ssize_t
ce_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - 1, "%d\n",
				color_enhance_switch);
	return i;
}

static ssize_t
ce_switch_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	int rc;
	unsigned long res;

	rc = strict_strtoul(buf, 10, &res);
	if (rc) {
		printk(KERN_ERR "invalid parameter, %s %d\n", buf, rc);
		count = -EINVAL;
		goto err_out;
	}

	if (vision_ce_switch(!!res))
		count = -EIO;

err_out:
	return count;
}
*/
static int
vision_backlight_probe(struct platform_device *pdev)
{
	int err = -EIO;
	PR_DISP_DEBUG("%s\n", __func__);

	mutex_init(&renesas.lock);
	renesas.last_shrink_br = 0;
	renesas.client_data = pdev->dev.platform_data;
	renesas.lcd_backlight.name = "lcd-backlight";
	renesas.lcd_backlight.brightness_set = vision_set_brightness;
	renesas.lcd_backlight.brightness_get = vision_get_brightness;
	err = led_classdev_register(&pdev->dev, &renesas.lcd_backlight);
	if (err)
		goto err_register_lcd_bl;

/*	err = device_create_file(cabc.lcd_backlight.dev, &ce_attr);
	if (err)
		goto err_out;

	return 0;
#if 0
	err = device_create_file(cabc.lcd_backlight.dev, &auto_attr);
	if (err)
		goto err_out;

	return 0;
#endif
err_out:
		device_remove_file(&pdev->dev, &ce_attr);
*/
err_register_lcd_bl:
	led_classdev_unregister(&renesas.lcd_backlight);
	return err;
}

/* ------------------------------------------------------------------- */

static struct resource resources_msm_fb[] = {
	{
		.flags = IORESOURCE_MEM,
	},
};
/*
#define REG_WAIT (0xffff)
struct nov_regs {
	unsigned reg;
	unsigned val;
};

struct nov_regs pro_lgd_init_seq[] = {
	{0x1100, 0x00},
	{REG_WAIT, 20},
	{0xf000, 0x55},
	{0xf001, 0xaa},
	{0xf002, 0x52},
	{0xf003, 0x08},
	{0xf004, 0x01},

	{0xb000, 0x0A},
	{0xb001, 0x0A},
	{0xb002, 0x0A},
	{0xb600, 0x43},
	{0xb601, 0x43},
	{0xb602, 0x43},
	{0xb100, 0x0A},
	{0xb101, 0x0A},
	{0xb102, 0x0A},
	{0xb700, 0x33},
	{0xb701, 0x33},
	{0xb702, 0x33},
	{0xb200, 0x01},
	{0xb201, 0x01},
	{0xb202, 0x01},
	{0xb800, 0x24},
	{0xb801, 0x24},
	{0xb802, 0x24},
	{0xb300, 0x08},
	{0xb301, 0x08},
	{0xb302, 0x08},
	{0xb900, 0x22},
	{0xb901, 0x22},
	{0xb902, 0x22},
	{0xbf00, 0x00},
	{0xba00, 0x22},
	{0xba01, 0x22},
	{0xba02, 0x22},
	{0xc200, 0x02},
	{0xbc00, 0x00},
	{0xbc01, 0xA0},
	{0xbc02, 0x00},
	{0xbd00, 0x00},
	{0xbd01, 0xA0},
	{0xbd02, 0x00},
	{0xd000, 0x0f},
	{0xd001, 0x0f},
	{0xd002, 0x10},
	{0xd003, 0x10},
	{0xd100, 0x00},
	{0xd101, 0x5D},
	{0xd102, 0x00},
	{0xd103, 0x76},
	{0xd104, 0x00},
	{0xd105, 0xa1},
	{0xd106, 0x00},
	{0xd107, 0xc2},
	{0xd108, 0x00},
	{0xd109, 0xdc},
	{0xd10a, 0x00},
	{0xd10b, 0xfb},
	{0xd10c, 0x01},
	{0xd10d, 0x11},
	{0xd10e, 0x01},
	{0xd10f, 0x41},
	{0xd110, 0x01},
	{0xd111, 0x6d},
	{0xd112, 0x01},
	{0xd113, 0x9e},
	{0xd114, 0x01},
	{0xd115, 0xc8},
	{0xd116, 0x02},
	{0xd117, 0x16},
	{0xd118, 0x02},
	{0xd119, 0x54},
	{0xd11a, 0x02},
	{0xd11b, 0x55},
	{0xd11c, 0x02},
	{0xd11d, 0x8e},
	{0xd11e, 0x02},
	{0xd11f, 0xc9},
	{0xd120, 0x02},
	{0xd121, 0xf3},
	{0xd122, 0x03},
	{0xd123, 0x22},
	{0xd124, 0x03},
	{0xd125, 0x45},
	{0xd126, 0x03},
	{0xd127, 0x73},
	{0xd128, 0x03},
	{0xd129, 0x8e},
	{0xd12a, 0x03},
	{0xd12b, 0xaa},
	{0xd12c, 0x03},
	{0xd12d, 0xba},
	{0xd12e, 0x03},
	{0xd12f, 0xcb},
	{0xd130, 0x03},
	{0xd131, 0xde},
	{0xd132, 0x03},
	{0xd133, 0xea},
	{0xd200, 0x00},
	{0xd201, 0x5d},
	{0xd202, 0x00},
	{0xd203, 0x76},
	{0xd204, 0x00},
	{0xd205, 0xa1},
	{0xd206, 0x00},
	{0xd207, 0xc2},
	{0xd208, 0x00},
	{0xd209, 0xdc},
	{0xd20a, 0x00},
	{0xd20b, 0xfb},
	{0xd20c, 0x01},
	{0xd20d, 0x11},
	{0xd20e, 0x01},
	{0xd20f, 0x41},
	{0xd210, 0x01},
	{0xd211, 0x6d},
	{0xd212, 0x01},
	{0xd213, 0x9e},
	{0xd214, 0x01},
	{0xd215, 0xc8},
	{0xd216, 0x02},
	{0xd217, 0x16},
	{0xd218, 0x02},
	{0xd219, 0x54},
	{0xd21a, 0x02},
	{0xd21b, 0x55},
	{0xd21c, 0x02},
	{0xd21d, 0x8e},
	{0xd21e, 0x02},
	{0xd21f, 0xc9},
	{0xd220, 0x02},
	{0xd221, 0xf3},
	{0xd222, 0x03},
	{0xd223, 0x22},
	{0xd224, 0x03},
	{0xd225, 0x45},
	{0xd226, 0x03},
	{0xd227, 0x73},
	{0xd228, 0x03},
	{0xd229, 0x8e},
	{0xd22a, 0x03},
	{0xd22b, 0xaa},
	{0xd22c, 0x03},
	{0xd22d, 0xba},
	{0xd22e, 0x03},
	{0xd22f, 0xcb},
	{0xd230, 0x03},
	{0xd231, 0xde},
	{0xd232, 0x03},
	{0xd233, 0xea},
	{0xd300, 0x00},
	{0xd301, 0x5d},
	{0xd302, 0x00},
	{0xd303, 0x76},
	{0xd304, 0x00},
	{0xd305, 0xa1},
	{0xd306, 0x00},
	{0xd307, 0xc2},
	{0xd308, 0x00},
	{0xd309, 0xdc},
	{0xd30a, 0x00},
	{0xd30b, 0xfb},
	{0xd30c, 0x01},
	{0xd30d, 0x11},
	{0xd30e, 0x01},
	{0xd30f, 0x41},
	{0xd310, 0x01},
	{0xd311, 0x6d},
	{0xd312, 0x01},
	{0xd313, 0x9e},
	{0xd314, 0x01},
	{0xd315, 0xc8},
	{0xd316, 0x02},
	{0xd317, 0x16},
	{0xd318, 0x02},
	{0xd319, 0x54},
	{0xd31a, 0x02},
	{0xd31b, 0x55},
	{0xd31c, 0x02},
	{0xd31d, 0x8e},
	{0xd31e, 0x02},
	{0xd31f, 0xc9},
	{0xd320, 0x02},
	{0xd321, 0xf3},
	{0xd322, 0x03},
	{0xd323, 0x22},
	{0xd324, 0x03},
	{0xd325, 0x45},
	{0xd326, 0x03},
	{0xd327, 0x73},
	{0xd328, 0x03},
	{0xd329, 0x8e},
	{0xd32a, 0x03},
	{0xd32b, 0xaa},
	{0xd32c, 0x03},
	{0xd32d, 0xba},
	{0xd32e, 0x03},
	{0xd32f, 0xcb},
	{0xd330, 0x03},
	{0xd331, 0xde},
	{0xd332, 0x03},
	{0xd333, 0xea},
	{0xd400, 0x00},
	{0xd401, 0x05},
	{0xd402, 0x00},
	{0xd403, 0x22},
	{0xd404, 0x00},
	{0xd405, 0x51},
	{0xd406, 0x00},
	{0xd407, 0x73},
	{0xd408, 0x00},
	{0xd409, 0x8d},
	{0xd40a, 0x00},
	{0xd40b, 0xb2},
	{0xd40c, 0x00},
	{0xd40d, 0xc6},
	{0xd40e, 0x00},
	{0xd40f, 0xf0},
	{0xd410, 0x01},
	{0xd411, 0x15},
	{0xd412, 0x01},
	{0xd413, 0x53},
	{0xd414, 0x01},
	{0xd415, 0x89},
	{0xd416, 0x01},
	{0xd417, 0xcf},
	{0xd418, 0x02},
	{0xd419, 0x09},
	{0xd41a, 0x02},
	{0xd41b, 0x0a},
	{0xd41c, 0x02},
	{0xd41d, 0x48},
	{0xd41e, 0x02},
	{0xd41f, 0x98},
	{0xd420, 0x02},
	{0xd421, 0xd1},
	{0xd422, 0x03},
	{0xd423, 0x18},
	{0xd424, 0x03},
	{0xd425, 0x47},
	{0xd426, 0x03},
	{0xd427, 0x73},
	{0xd428, 0x03},
	{0xd429, 0x8e},
	{0xd42a, 0x03},
	{0xd42b, 0xaa},
	{0xd42c, 0x03},
	{0xd42d, 0xba},
	{0xd42e, 0x03},
	{0xd42f, 0xcb},
	{0xd430, 0x03},
	{0xd431, 0xde},
	{0xd432, 0x03},
	{0xd433, 0xea},
	{0xd500, 0x00},
	{0xd501, 0x05},
	{0xd502, 0x00},
	{0xd503, 0x22},
	{0xd504, 0x00},
	{0xd505, 0x51},
	{0xd506, 0x00},
	{0xd507, 0x73},
	{0xd508, 0x00},
	{0xd509, 0x8d},
	{0xd50a, 0x00},
	{0xd50b, 0xb2},
	{0xd50c, 0x00},
	{0xd50d, 0xc6},
	{0xd50e, 0x00},
	{0xd50f, 0xf0},
	{0xd510, 0x01},
	{0xd511, 0x15},
	{0xd512, 0x01},
	{0xd513, 0x53},
	{0xd514, 0x01},
	{0xd515, 0x89},
	{0xd516, 0x01},
	{0xd517, 0xcf},
	{0xd518, 0x02},
	{0xd519, 0x09},
	{0xd51a, 0x02},
	{0xd51b, 0x0a},
	{0xd51c, 0x02},
	{0xd51d, 0x48},
	{0xd51e, 0x02},
	{0xd51f, 0x98},
	{0xd520, 0x02},
	{0xd521, 0xd1},
	{0xd522, 0x03},
	{0xd523, 0x18},
	{0xd524, 0x03},
	{0xd525, 0x47},
	{0xd526, 0x03},
	{0xd527, 0x73},
	{0xd528, 0x03},
	{0xd529, 0x8e},
	{0xd52a, 0x03},
	{0xd52b, 0xaa},
	{0xd52c, 0x03},
	{0xd52d, 0xba},
	{0xd52e, 0x03},
	{0xd52f, 0xcb},
	{0xd530, 0x03},
	{0xd531, 0xde},
	{0xd532, 0x03},
	{0xd533, 0xea},
	{0xd600, 0x00},
	{0xd601, 0x05},
	{0xd602, 0x00},
	{0xd603, 0x22},
	{0xd604, 0x00},
	{0xd605, 0x51},
	{0xd606, 0x00},
	{0xd607, 0x73},
	{0xd608, 0x00},
	{0xd609, 0x8d},
	{0xd60a, 0x00},
	{0xd60b, 0xb2},
	{0xd60c, 0x00},
	{0xd60d, 0xc6},
	{0xd60e, 0x00},
	{0xd60f, 0xf0},
	{0xd610, 0x01},
	{0xd611, 0x15},
	{0xd612, 0x01},
	{0xd613, 0x53},
	{0xd614, 0x01},
	{0xd615, 0x89},
	{0xd616, 0x01},
	{0xd617, 0xcf},
	{0xd618, 0x02},
	{0xd619, 0x09},
	{0xd61a, 0x02},
	{0xd61b, 0x0a},
	{0xd61c, 0x02},
	{0xd61d, 0x48},
	{0xd61e, 0x02},
	{0xd61f, 0x98},
	{0xd620, 0x02},
	{0xd621, 0xd1},
	{0xd622, 0x03},
	{0xd623, 0x18},
	{0xd624, 0x03},
	{0xd625, 0x47},
	{0xd626, 0x03},
	{0xd627, 0x73},
	{0xd628, 0x03},
	{0xd629, 0x8e},
	{0xd62a, 0x03},
	{0xd62b, 0xaa},
	{0xd62c, 0x03},
	{0xd62d, 0xba},
	{0xd62e, 0x03},
	{0xd62f, 0xcb},
	{0xd630, 0x03},
	{0xd631, 0xde},
	{0xd632, 0x03},
	{0xd633, 0xea},
	//pwm control
	{0xf000, 0x55},
	{0xf001, 0xaa},
	{0xf002, 0x52},
	{0xf003, 0x08},
	{0xf004, 0x00},
	{0xB400, 0x10},
	{0xe000, 0x01},
	{0xe001, 0x03},
	//pwm control
	{0xb100, 0xc8},
	{0xb101, 0x00},
	{0xb600, 0x05},
	{0xb700, 0x71},
	{0xb701, 0x71},
	{0xb800, 0x01},
	{0xb801, 0x05},
	{0xb802, 0x05},
	{0xb803, 0x05},
	{0xb900, 0x00},
	{0xb901, 0x40},
	{0xba00, 0x05},
	{0xbc00, 0x00},
	{0xbc01, 0x00},
	{0xbc02, 0x00},
	{0xbd00, 0x01},
	{0xbd01, 0x8c},
	{0xbd02, 0x14},
	{0xbd03, 0x14},
	{0xbd04, 0x00},
	{0xbe00, 0x01},
	{0xbe01, 0x8c},
	{0xbe02, 0x14},
	{0xbe03, 0x14},
	{0xbe04, 0x00},
	{0xbf00, 0x01},
	{0xbf01, 0x8c},
	{0xbf02, 0x14},
	{0xbf03, 0x14},
	{0xbf04, 0x00},
	{0xc900, 0xc2},
	{0xc901, 0x02},
	{0xc902, 0x50},
	{0xc903, 0x50},
	{0xc904, 0x50},
	//CABC
	{0xD400, 0x05},
	{0xDD00, 0x55},

	{0xE400, 0xFF},
	{0xE401, 0xF7},
	{0xE402, 0xEF},
	{0xE403, 0xE7},
	{0xE404, 0xDF},
	{0xE405, 0xD7},
	{0xE406, 0xCF},
	{0xE407, 0xC7},
	{0xE408, 0xBF},
	{0xE409, 0xB7},
	//CABC END
	//Page Enbale
	{0xFF00, 0xAA},
	{0xFF01, 0x55},
	{0xFF02, 0x25},
	{0xFF03, 0x01},
	//saturation off
	{0xF50C, 0x03},
	// Values for Vivid Color start
	{0xF900, 0x0a},
	{0xF901, 0x00},
	{0xF902, 0x0e},
	{0xF903, 0x1f},
	{0xF904, 0x37},
	{0xF905, 0x55},
	{0xF906, 0x6e},
	{0xF907, 0x6e},
	{0xF908, 0x46},
	{0xF909, 0x28},
	{0xF90A, 0x0e},
	// Vivid Color end
	{0x5300, 0x24},
	{0x3600, 0x00},
	{0x4400, 0x01},
	{0x4401, 0x77},
	//CABC Mode Selection
	{0x5500, 0x03},
	{0x5E00, 0x06},
	{0x3500, 0x00},
	{REG_WAIT, 160},
	{0x2900, 0x00},
};

static struct nov_regs sony_init_seq[] = {
	{0x1100, 0x00},
	{REG_WAIT, 120},
	{0xF000, 0x55},
	{0xF001, 0xaa},
	{0xF002, 0x52},
	{0xF003, 0x08},
	{0xF004, 0x00},

	{0xB400, 0x10},
	{0xE000, 0x01},
	{0xE001, 0x03},
	{0xB800, 0x00},
	{0xB801, 0x00},
	{0xB802, 0x00},
	{0xB803, 0x00},
	{0xBC00, 0x00},
	{0xBC01, 0x00},
	{0xBC02, 0x00},
	//CABC
	{0xD400, 0x05},
	{0xDD00, 0x55},

	{0xE400, 0xFF},
	{0xE401, 0xF7},
	{0xE402, 0xEF},
	{0xE403, 0xE7},
	{0xE404, 0xDF},
	{0xE405, 0xD7},
	{0xE406, 0xCF},
	{0xE407, 0xC7},
	{0xE408, 0xBF},
	{0xE409, 0xB7},

	//CABC END
	//Page Enbale
	{0xFF00, 0xAA},
	{0xFF01, 0x55},
	{0xFF02, 0x25},
	{0xFF03, 0x01},
	//saturation off
	{0xF50C, 0x03},

	// Values for Vivid Color start
	{0xF900, 0x0a},
	{0xF901, 0x00},
	{0xF902, 0x0e},
	{0xF903, 0x1f},
	{0xF904, 0x37},
	{0xF905, 0x55},
	{0xF906, 0x6e},
	{0xF907, 0x6e},
	{0xF908, 0x46},
	{0xF909, 0x28},
	{0xF90A, 0x0e},
	// Vivid Color end
	{0x3500, 0x00},
	{0x4400, 0x01},
	{0x4401, 0xB3},
	{0x2900, 0x00},
	{REG_WAIT, 40},
	{0x5500, 0x03},
	{0x5E00, 0x06},
	{0x5300, 0x24},
};
*/
static int
vision_mddi_init(struct msm_mddi_bridge_platform_data *bridge_data,
		     struct msm_mddi_client_data *client_data)
{
/*	int i = 0, array_size = 0;
	unsigned reg, val;
	struct nov_regs *init_seq = NULL;

	PR_DISP_INFO("%s(0x%x)\n", __func__, panel_type);
	client_data->auto_hibernate(client_data, 0);

	switch (panel_type) {
	case PANEL_ID_VISION_SONY:
		init_seq = sony_init_seq;
		array_size = ARRAY_SIZE(sony_init_seq);
		break;
	case PANEL_ID_VISION_LG:
	default:
		init_seq = pro_lgd_init_seq;
		array_size = ARRAY_SIZE(pro_lgd_init_seq);
		break;
	}

	for (i = 0; i < array_size; i++) {
		reg = cpu_to_le32(init_seq[i].reg);
		val = cpu_to_le32(init_seq[i].val);
		if (reg == REG_WAIT)
			msleep(val);
		else
			client_data->remote_write(client_data, val, reg);
	}
	if (color_enhance_switch == 0)
		client_data->remote_write(client_data, 0x00, 0xb400);
	client_data->auto_hibernate(client_data, 1);

	if (axi_clk)
		clk_set_rate(axi_clk, 0);
*/
	do_renesas_cmd(client_data, vision_renesas_cmd, ARRAY_SIZE(vision_renesas_cmd));
	return 0;
}

static int
vision_mddi_uninit(struct msm_mddi_bridge_platform_data *bridge_data,
			struct msm_mddi_client_data *client_data)
{
	PR_DISP_DEBUG("%s\n", __func__);

	client_data->auto_hibernate(client_data, 0);
	client_data->remote_write(client_data, 0x0, 0x10);
	hr_msleep(72);
	client_data->auto_hibernate(client_data, 1);

	return 0;
}

static int
vision_panel_blank(struct msm_mddi_bridge_platform_data *bridge_data,
			struct msm_mddi_client_data *client_data)
{
	PR_DISP_DEBUG("%s\n", __func__);

/*	client_data->auto_hibernate(client_data, 0);

	if (panel_type == PANEL_ID_VISION_SONY) {
		client_data->remote_write(client_data, 0x0, 0x5300);
		vision_backlight_switch(LED_OFF);
		client_data->remote_write(client_data, 0, 0x2800);
		client_data->remote_write(client_data, 0, 0x1000);
	} else {
		client_data->remote_write(client_data, 0x0, 0x5300);
		vision_backlight_switch(LED_OFF);
		client_data->remote_write(client_data, 0, 0x2800);
		hr_msleep(10);
		client_data->remote_write(client_data, 0, 0x1000);
	}
	client_data->auto_hibernate(client_data, 1);*/

	client_data->remote_write(client_data, 0x04, 0xB0);
	client_data->remote_write(client_data, 0x0, 0x28);
	vision_backlight_switch(client_data,LED_OFF);
	client_data->remote_write(client_data, 0x0, 0xB8);
	client_data->remote_write(client_data, 0x03, 0xB0);
	hr_msleep(72);

	return 0;
}

static int
vision_panel_unblank(struct msm_mddi_bridge_platform_data *bridge_data,
			struct msm_mddi_client_data *client_data)
{
	PR_DISP_DEBUG("%s\n", __func__);
	client_data->auto_hibernate(client_data, 0);
	/* HTC, Add 50 ms delay for stability of driver IC at high temperature */
/*	hr_msleep(50);
	if (panel_type == PANEL_ID_VISION_SONY) {
		client_data->remote_write(client_data, 0x00, 0x3600);
		client_data->remote_write(client_data, 0x24, 0x5300);
	} else {
		client_data->remote_write(client_data, 0x24, 0x5300);
	}
	vision_backlight_switch(LED_FULL);*/

	client_data->remote_write(client_data, 0x04, 0xB0);

	client_data->remote_write(client_data, 0x0, 0x11);
	hr_msleep(125);
	do_renesas_cmd(client_data, gama, ARRAY_SIZE(gama));
	vision_backlight_switch(client_data, LED_FULL);
	client_data->remote_write(client_data, 0x0, 0x29);
	do_renesas_cmd(client_data, tear, ARRAY_SIZE(tear));
	client_data->remote_write(client_data, 0x0, 0x35);
	client_data->remote_write(client_data, 0x03, 0xB0);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

/*static struct msm_mddi_bridge_platform_data novatec_client_data = {
	.init = vision_mddi_init,
	.uninit = vision_mddi_uninit,
	.blank = vision_panel_blank,
	.unblank = vision_panel_unblank,
	.fb_data = {
		.xres = 480,
		.yres = 800,
		.width = 52,
		.height = 88,
		.output_format = 0,
	},
	.panel_conf = {
		.caps = MSMFB_CAP_renesas,
		.vsync_gpio = 30,
	},
};*/

static struct msm_mddi_bridge_platform_data renesas_client_data = {
	.init = vision_mddi_init,
	.uninit = vision_mddi_uninit,
	.blank = vision_panel_blank,
	.unblank = vision_panel_unblank,
	.fb_data = {
		.xres = 480,
		.yres = 800,
		.width = 48,
		.height = 80,
		.output_format = 0,
	},
};

static void
mddi_power(struct msm_mddi_client_data *client_data, int on)
{
	if (panel_type == PANEL_ID_SAG_HITACHI) {
		if (on) {
			vreg_enable(vreg_ldo19);
			gpio_set_value(VISION_MDDI_RSTz,0);
			vreg_enable(vreg_ldo20);
			hr_msleep(1);
			gpio_set_value(VISION_MDDI_RSTz,1);
			hr_msleep(5);
		}
		else {
			vreg_disable(vreg_ldo19);
			vreg_disable(vreg_ldo20);
		}
	}
/*	int rc;
	unsigned config;
	PR_DISP_DEBUG("%s(%s)\n", __func__, on?"on":"off");

	if (on) {
		if (axi_clk)
			clk_set_rate(axi_clk, 192000000);

		config = PCOM_GPIO_CFG(VISION_MDDI_TE, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(VISION_LCD_ID0, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);

		if (panel_type == PANEL_ID_VISION_SONY) {
			config = PCOM_GPIO_CFG(VISION_LCD_ID1, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);
			rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
			vreg_enable(V_LCMIO_1V8);
			hr_msleep(3);
			vreg_enable(V_LCMIO_2V8);
			hr_msleep(5);

			gpio_set_value(VISION_LCD_RSTz, 1);
			hr_msleep(1);
			gpio_set_value(VISION_LCD_RSTz, 0);
			hr_msleep(1);
			gpio_set_value(VISION_LCD_RSTz, 1);
			hr_msleep(15);
		} else {
			config = PCOM_GPIO_CFG(VISION_LCD_ID1, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA);
			rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
			vreg_enable(V_LCMIO_1V8);
			hr_msleep(3);
			vreg_enable(V_LCMIO_2V8);
			hr_msleep(5);

			gpio_set_value(VISION_LCD_RSTz, 1);
			hr_msleep(1);
			gpio_set_value(VISION_LCD_RSTz, 0);
			hr_msleep(1);
			gpio_set_value(VISION_LCD_RSTz, 1);
			hr_msleep(20);
		}
	} else {
		if (panel_type == PANEL_ID_VISION_SONY) {
			hr_msleep(80);
			gpio_set_value(VISION_LCD_RSTz, 0);
			hr_msleep(10);
			vreg_disable(V_LCMIO_1V8);
			vreg_disable(V_LCMIO_2V8);
		} else {
			hr_msleep(20);
			gpio_set_value(VISION_LCD_RSTz, 0);
			vreg_disable(V_LCMIO_2V8);
			vreg_disable(V_LCMIO_1V8);
		}

		config = PCOM_GPIO_CFG(VISION_MDDI_TE, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(VISION_LCD_ID1, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
		config = PCOM_GPIO_CFG(VISION_LCD_ID0, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA);
		rc = msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, 0);
	}
*/
}

static void
panel_mddi_fixup(uint16_t *mfr_name, uint16_t *product_code)
{
	PR_DISP_DEBUG("mddi fixup\n");
	*mfr_name = 0xb9f6;
	*product_code = 0x1408;
}

static struct msm_mddi_platform_data mddi_pdata = {
	.fixup = panel_mddi_fixup,
	.power_client = mddi_power,
	.fb_resource = resources_msm_fb,
	.num_clients = 1,
	.client_platform_data = {
		{
			.product_id = (0xb9f6 << 16 | 0x1408),
			.name = "mddi_c_b9f6_1408",
			.id = 0,
			.client_data = &renesas_client_data,
			.clk_rate = 0,
		},
	},
};

static struct platform_driver vision_backlight_driver = {
	.probe = vision_backlight_probe,
	.driver = {
		.name = "renesas_backlight",
		.owner = THIS_MODULE,
	},
};

static struct msm_mdp_platform_data mdp_pdata = {
	.overrides = MSM_MDP4_MDDI_DMA_SWITCH,
	.color_format = MSM_MDP_OUT_IF_FMT_RGB888,
/*#ifdef CONFIG_MDP4_HW_VSYNC
       .xres = 480,
       .yres = 800,
       .back_porch = 20,
       .front_porch = 20,
       .pulse_width = 40,
#endif*/
};

/*static struct msm_mdp_platform_data mdp_pdata_sony = {
	.overrides = MSM_MDP_PANEL_FLIP_UD | MSM_MDP_PANEL_FLIP_LR,
	.color_format = MSM_MDP_OUT_IF_FMT_RGB888,
#ifdef CONFIG_MDP4_HW_VSYNC
       .xres = 480,
       .yres = 800,
       .back_porch = 4,
       .front_porch = 2,
       .pulse_width = 4,
#endif
};*/

int __init vision_init_panel(void)
{
	int rc;

	PR_DISP_INFO("%s: enter.\n", __func__);

	/* lcd panel power */
	V_LCMIO_1V8 = vreg_get(NULL, "wlan2");

	if (IS_ERR(V_LCMIO_1V8)) {
		PR_DISP_ERR("%s: LCMIO_1V8 get failed (%ld)\n",
		       __func__, PTR_ERR(V_LCMIO_1V8));
		return -1;
	}

	V_LCMIO_2V8 = vreg_get(NULL, "gp13");

	if (IS_ERR(V_LCMIO_2V8)) {
		PR_DISP_ERR("%s: LCMIO_2V8 get failed (%ld)\n",
		       __func__, PTR_ERR(V_LCMIO_2V8));
		return -1;
	}

	rc = vreg_set_level(vreg_ldo19, 1800);
	if (rc) {
		pr_err("%s: vreg LDO19 set level failed (%d)\n",
		       __func__, rc);
		return -1;
	}

	resources_msm_fb[0].start = msm_fb_base;
	resources_msm_fb[0].end = msm_fb_base + MSM_FB_SIZE - 1;

	if (panel_type == PANEL_ID_SAG_HITACHI) {
		rc = vreg_set_level(vreg_ldo20, 2850);
		if (rc) {
			pr_err("%s: vreg LDO20 set level failed (%d)\n",
				__func__, rc);
			return -1;
		}
		msm_device_mdp.dev.platform_data = &mdp_pdata;
		rc = platform_device_register(&msm_device_mdp);
		if (rc)
			return rc;

		mddi_pdata.clk_rate = 384000000;
		mddi_pdata.type = MSM_MDP_MDDI_TYPE_II;

/*	axi_clk = clk_get(NULL, "ebi1_mddi_clk");
	if (IS_ERR(axi_clk)) {
		PR_DISP_ERR("%s: failed to get axi clock\n", __func__);
		return PTR_ERR(axi_clk);
	}
*/
		msm_device_mddi0.dev.platform_data = &mddi_pdata;
		rc = platform_device_register(&msm_device_mddi0);
		if (rc)
			return rc;

		rc = platform_driver_register(&vision_backlight_driver);
		if (rc)
			return rc;
	}
	return 0;
}
