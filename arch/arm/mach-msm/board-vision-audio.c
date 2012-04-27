/* linux/arch/arm/mach-msm/board-vision-audio.c
 *
 * Copyright (C) 2010-2011 HTC Corporation.
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

#include <linux/android_pmem.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/marimba.h>
//#include <linux/mfd/msm-adie-codec.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <mach/dal.h>
#include "board-vision.h"
#include <mach/qdsp5v2_2x/snddev_icodec.h>
#include <mach/qdsp5v2_2x/snddev_ecodec.h>
#include <mach/qdsp5v2_2x/audio_def.h>
#include <mach/qdsp5v2_2x/voice.h>
#include <mach/htc_acoustic_7x30.h>
#include <mach/htc_acdb_7x30.h>
#include <linux/spi/spi_aic3254.h>

static struct mutex bt_sco_lock;
static int curr_rx_mode;
static atomic_t aic3254_ctl = ATOMIC_INIT(0);

#define BIT_SPEAKER	(1 << 0)
#define BIT_HEADSET	(1 << 1)
#define BIT_RECEIVER	(1 << 2)
#define BIT_FM_SPK	(1 << 3)
#define BIT_FM_HS	(1 << 4)

#define PMGPIO(x) (x-1)
#define VISION_ACDB_SMEM_SIZE        (0xE000)
#define VISION_ACDB_RADIO_BUFFER_SIZE (1024 * 3072)

static struct vreg *vreg_audio_n1v8;

static struct q5v2_hw_info q5v2_audio_hw[Q5V2_HW_COUNT] = {
	[Q5V2_HW_HANDSET] = {
		.max_gain[VOC_NB_INDEX] = 200,
		.min_gain[VOC_NB_INDEX] = -1800,
		.max_gain[VOC_WB_INDEX] = -200,
		.min_gain[VOC_WB_INDEX] = -1700,
	},
	[Q5V2_HW_HEADSET] = {
		.max_gain[VOC_NB_INDEX] = 400,
		.min_gain[VOC_NB_INDEX] = -1600,
		.max_gain[VOC_WB_INDEX] = 200,
		.min_gain[VOC_WB_INDEX] = -1800,
	},
	[Q5V2_HW_SPEAKER] = {
		.max_gain[VOC_NB_INDEX] = -100,
		.min_gain[VOC_NB_INDEX] = -1600,
		.max_gain[VOC_WB_INDEX] = -100,
		.min_gain[VOC_WB_INDEX] = -1600,
	},
	[Q5V2_HW_BT_SCO] = {
		.max_gain[VOC_NB_INDEX] = 0,
		.min_gain[VOC_NB_INDEX] = -1500,
		.max_gain[VOC_WB_INDEX] = 0,
		.min_gain[VOC_WB_INDEX] = -1500,
	},
	[Q5V2_HW_TTY] = {
		.max_gain[VOC_NB_INDEX] = 0,
		.min_gain[VOC_NB_INDEX] = 0,
		.max_gain[VOC_WB_INDEX] = 0,
		.min_gain[VOC_WB_INDEX] = 0,
	},
	[Q5V2_HW_HS_SPKR] = {
		.max_gain[VOC_NB_INDEX] = -500,
		.min_gain[VOC_NB_INDEX] = -2000,
		.max_gain[VOC_WB_INDEX] = -500,
		.min_gain[VOC_WB_INDEX] = -2000,
	},
	[Q5V2_HW_USB_HS] = {
		.max_gain[VOC_NB_INDEX] = 1000,
		.min_gain[VOC_NB_INDEX] = -500,
		.max_gain[VOC_WB_INDEX] = 1000,
		.min_gain[VOC_WB_INDEX] = -500,
	},
	[Q5V2_HW_HAC] = {
		.max_gain[VOC_NB_INDEX] = 1000,
		.min_gain[VOC_NB_INDEX] = -500,
		.max_gain[VOC_WB_INDEX] = 1000,
		.min_gain[VOC_WB_INDEX] = -500,
	},
};

static unsigned aux_pcm_gpio_off[] = {
	GPIO_CFG(VISION_GPIO_BT_PCM_OUT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(VISION_GPIO_BT_PCM_IN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_DIN  */
	GPIO_CFG(VISION_GPIO_BT_PCM_SYNC, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(VISION_GPIO_BT_PCM_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	/* PCM_CLK  */
};


static unsigned aux_pcm_gpio_on[] = {
	GPIO_CFG(VISION_GPIO_BT_PCM_OUT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DOUT */
	GPIO_CFG(VISION_GPIO_BT_PCM_IN, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_DIN  */
	GPIO_CFG(VISION_GPIO_BT_PCM_SYNC, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_SYNC */
	GPIO_CFG(VISION_GPIO_BT_PCM_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),	/* PCM_CLK  */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[CAM] %s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

void vision_hs_n1v8_enable(int en)
{
	int rc = 0;

	pr_aud_info("%s: %d\n", __func__, en);

	if (!vreg_audio_n1v8) {

		vreg_audio_n1v8 = vreg_get(NULL, "ncp");

		if (IS_ERR(vreg_audio_n1v8)) {
			printk(KERN_ERR "%s: vreg_get() failed (%ld)\n",
				__func__, PTR_ERR(vreg_audio_n1v8));
			rc = PTR_ERR(vreg_audio_n1v8);
			goto  vreg_aud_hp_1v8_faill;
		}
	}

	if (en) {
		rc = vreg_enable(vreg_audio_n1v8);
		if (rc) {
			printk(KERN_ERR "%s: vreg_enable() = %d \n",
					__func__, rc);
			goto vreg_aud_hp_1v8_faill;
		}
	} else {
		rc = vreg_disable(vreg_audio_n1v8);
		if (rc) {
			printk(KERN_ERR "%s: vreg_disable() = %d \n",
					__func__, rc);
			goto vreg_aud_hp_1v8_faill;
		}
	}
	return;
vreg_aud_hp_1v8_faill:
	printk(KERN_ERR "%s: failed (%ld)\n",
		__func__, PTR_ERR(vreg_audio_n1v8));
}

void vision_snddev_poweramp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_SPK_EN), 1);
		mdelay(30);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_SPEAKER;
		mdelay(5);
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_SPK_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_SPEAKER;
	}
}

void vision_snddev_hsed_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_HP_EN), 1);
		mdelay(30);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_HEADSET;
		mdelay(5);
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_HP_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_HEADSET;
	}
}
/*
void vision_snddev_pre_hsed_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		vision_hs_n1v8_enable(1);
	} else {
		vision_hs_n1v8_enable(0);
	}
}
*/
void vision_snddev_hs_spk_pamp_on(int en)
{
	vision_snddev_poweramp_on(en);
	vision_snddev_hsed_pamp_on(en);
}

void vision_snddev_receiver_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_EP_EN), 1);
		mdelay(5);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_RECEIVER;
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_EP_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_RECEIVER;
	}
}

void vision_snddev_bt_sco_pamp_on(int en)
{
	static int bt_sco_refcount;
	pr_aud_info("%s %d\n", __func__, en);
	mutex_lock(&bt_sco_lock);
	if (en) {
		if (++bt_sco_refcount == 1)
			config_gpio_table(aux_pcm_gpio_on,
					ARRAY_SIZE(aux_pcm_gpio_on));
	} else {
		if (--bt_sco_refcount == 0) {
			config_gpio_table(aux_pcm_gpio_off,
					ARRAY_SIZE(aux_pcm_gpio_off));

			gpio_set_value(VISION_GPIO_BT_PCM_OUT, 0);
			gpio_set_value(VISION_GPIO_BT_PCM_SYNC, 0);
			gpio_set_value(VISION_GPIO_BT_PCM_CLK, 0);

		}
	}
	mutex_unlock(&bt_sco_lock);
}

/* power up internal/externnal mic shared GPIO */
void vision_mic_bias_enable(int en, int shift)
{
	pr_aud_info("%s: %d\n", __func__, en);

	if (en) {
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_ALWAYS);
	} else {
		pmic_hsed_enable(PM_HSED_CONTROLLER_1, PM_HSED_ENABLE_OFF);
	}
}

void vision_snddev_imic_pamp_on(int en)
{
	pr_aud_info("%s: %d\n", __func__, en);

	if (en) {
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_ALWAYS);
		pmic_hsed_enable(PM_HSED_CONTROLLER_2, PM_HSED_ENABLE_ALWAYS);
	} else {
		pmic_hsed_enable(PM_HSED_CONTROLLER_0, PM_HSED_ENABLE_OFF);
		pmic_hsed_enable(PM_HSED_CONTROLLER_2, PM_HSED_ENABLE_OFF);
	}
}

void vision_snddev_emic_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
		/* change MICSELECT to pmic gpio 10 after XB */
		if (system_rev > 0)
			gpio_set_value(
				PM8058_GPIO_PM_TO_SYS(VISION_AUD_MICPATH_SEL_XB)
				, 1);
		else
			gpio_set_value(VISION_AUD_MICPATH_SEL_XA, 1);
	} else {
		if (system_rev > 0)
			gpio_set_value(
				PM8058_GPIO_PM_TO_SYS(VISION_AUD_MICPATH_SEL_XB)
				, 1);
		else
			gpio_set_value(VISION_AUD_MICPATH_SEL_XA, 0);
	}
}

void vision_snddev_fmspk_pamp_on(int en)
{
	if (en) {
		msleep(30);
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_SPK_EN), 1);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_FM_SPK;
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_SPK_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_FM_SPK;
	}
}

void vision_snddev_fmhs_pamp_on(int en)
{
	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_HP_EN), 1);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_FM_HS;
		mdelay(5);
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VISION_AUD_HP_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_FM_HS;
	}
}

int vision_get_rx_vol(uint8_t hw, int network, int level)
{
	struct q5v2_hw_info *info;
	int vol, maxv, minv;

	info = &q5v2_audio_hw[hw];
	maxv = info->max_gain[network];
	minv = info->min_gain[network];
	vol = minv + ((maxv - minv) * level) / 100;
	pr_aud_info("%s(%d, %d, %d) => %d\n", __func__, hw, network, level, vol);
	return vol;
}

void vision_rx_amp_enable(int en)
{
	if (curr_rx_mode != 0) {
		atomic_set(&aic3254_ctl, 1);
		pr_aud_info("%s: curr_rx_mode 0x%x, en %d\n",
			__func__, curr_rx_mode, en);
		if (curr_rx_mode & BIT_SPEAKER)
			vision_snddev_poweramp_on(en);
		if (curr_rx_mode & BIT_HEADSET)
			vision_snddev_hsed_pamp_on(en);
		if (curr_rx_mode & BIT_RECEIVER)
			vision_snddev_receiver_pamp_on(en);
		if (curr_rx_mode & BIT_FM_SPK)
			vision_snddev_fmspk_pamp_on(en);
		if (curr_rx_mode & BIT_FM_HS)
			vision_snddev_fmhs_pamp_on(en);
		atomic_set(&aic3254_ctl, 0);
	}
}

uint32_t vision_get_smem_size(void)
{
	return VISION_ACDB_SMEM_SIZE;
}

uint32_t vision_get_acdb_radio_buffer_size(void)
{
	return VISION_ACDB_RADIO_BUFFER_SIZE;
}

int vision_support_aic3254(void)
{
	return 1;
}

int vision_support_adie(void)
{
	return 1;
}

int vision_support_back_mic(void)
{
	return 1;
}

void vision_get_acoustic_tables(struct acoustic_tables *tb)
{
	switch (system_rev) {
	case 0:
		strcpy(tb->aic3254, "AIC3254_REG_DualMic.csv");
		break;
	case 1:
		strcpy(tb->aic3254, "AIC3254_REG_DualMic_XB.csv");
		break;
	default:
		strcpy(tb->aic3254, "AIC3254_REG_DualMic.txt");
	}
}

static struct acdb_ops acdb = {
	.get_acdb_radio_buffer_size = vision_get_acdb_radio_buffer_size,
};

static struct q5v2audio_analog_ops ops = {
	.speaker_enable	= vision_snddev_poweramp_on,
	.headset_enable	= vision_snddev_hsed_pamp_on,
	.bt_sco_enable = vision_snddev_bt_sco_pamp_on,
	.headset_speaker_enable = vision_snddev_hs_spk_pamp_on,
	.int_mic_enable = vision_snddev_imic_pamp_on,
	.ext_mic_enable = vision_snddev_emic_pamp_on,
	.fm_headset_enable = vision_snddev_hsed_pamp_on,
	.fm_speaker_enable = vision_snddev_poweramp_on,
//	.qtr_headset_enable = vision_snddev_pre_hsed_pamp_on
};

static struct q5v2audio_icodec_ops iops = {
	.support_aic3254 = vision_support_aic3254,
//	.support_adie = vision_support_adie,
};

static struct q5v2audio_ecodec_ops eops = {
	.bt_sco_enable  = vision_snddev_bt_sco_pamp_on,
};

static struct q5v2voice_ops vops = {
	.get_rx_vol = vision_get_rx_vol,
};

static struct acoustic_ops acoustic = {
	.enable_mic_bias = vision_mic_bias_enable,
	.support_aic3254 = vision_support_aic3254,
	.support_back_mic = vision_support_back_mic,
//	.support_beats = vision_support_beats,
	.get_acoustic_tables = vision_get_acoustic_tables
//	.enable_beats = vision_enable_beats
};

int vision_panel_sleep_in(void)
{
	return 0;
}
static struct aic3254_ctl_ops cops = {
	.rx_amp_enable = vision_rx_amp_enable,
	.panel_sleep_in = vision_panel_sleep_in
};

void __init vision_audio_init(void)
{
	struct pm_gpio audio_pwr_28 = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.vin_sel        = 6,
	};

	struct pm_gpio audio_pwr_18 = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_NORMAL,
		.vin_sel        = 4,
	};

	mutex_init(&bt_sco_lock);

#ifdef CONFIG_MSM7KV2_AUDIO
	htc_7x30_register_analog_ops(&ops);
	htc_7x30_register_icodec_ops(&iops);
	htc_7x30_register_ecodec_ops(&eops);
	htc_7x30_register_voice_ops(&vops);
	acoustic_register_ops(&acoustic);
	acdb_register_ops(&acdb);
#endif

	aic3254_register_ctl_ops(&cops);

	pm8xxx_gpio_config(VISION_AUD_HP_EN, &audio_pwr_28);
	pm8xxx_gpio_config(VISION_AUD_EP_EN, &audio_pwr_28);
	pm8xxx_gpio_config(VISION_AUD_SPK_EN, &audio_pwr_28);
	pm8xxx_gpio_config(VISION_AUD_MICPATH_SEL_XB, &audio_pwr_28);

	pm8xxx_gpio_config(VISION_AUD_A3254_RSTz, &audio_pwr_18);
	mdelay(1);
	audio_pwr_18.output_value = 1;
	pm8xxx_gpio_config(VISION_AUD_A3254_RSTz, &audio_pwr_18);
	audio_pwr_18.output_value = 0;

	mutex_lock(&bt_sco_lock);
	config_gpio_table(aux_pcm_gpio_off, ARRAY_SIZE(aux_pcm_gpio_off));
	gpio_set_value(VISION_GPIO_BT_PCM_OUT, 0);
	gpio_set_value(VISION_GPIO_BT_PCM_SYNC, 0);
	gpio_set_value(VISION_GPIO_BT_PCM_CLK, 0);
	gpio_set_value(36, 1);
	mutex_unlock(&bt_sco_lock);
}
