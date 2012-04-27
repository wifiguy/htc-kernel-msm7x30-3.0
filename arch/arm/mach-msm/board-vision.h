/* linux/arch/arm/mach-msm/board-vision.h
 * Copyright (C) 2007-2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#ifndef __ARCH_ARM_MACH_MSM_BOARD_VISION_H
#define __ARCH_ARM_MACH_MSM_BOARD_VISION_H

#include <mach/board.h>

#define MSM_LINUX_BASE1			0x04400000
#define MSM_LINUX_SIZE1			0x0BC00000
#define MSM_LINUX_BASE2			0x20000000
#define MSM_LINUX_SIZE2			0x0A400000
#define MSM_MEM_256MB_OFFSET	0x10000000

#define MSM_GPU_MEM_BASE		0x00100000
#define MSM_GPU_MEM_SIZE		0x00300000

#define MSM_RAM_CONSOLE_BASE	0x00500000
#define MSM_RAM_CONSOLE_SIZE	0x00100000

#define MSM_PMEM_ADSP_BASE     	0x2A400000
#define MSM_PMEM_ADSP_SIZE      0x03200000
#define PMEM_KERNEL_EBI1_BASE   0x2D600000
#define PMEM_KERNEL_EBI1_SIZE   0x00700000

#define MSM_PMEM_CAMERA_BASE	0x2DD00000
#define MSM_PMEM_CAMERA_SIZE	0x00000000

#define MSM_PMEM_MDP_BASE		0x2DD00000
#define MSM_PMEM_MDP_SIZE		0x02000000

#define MSM_FB_BASE				0x2FD00000
#define MSM_FB_SIZE				0x00300000

#define VISION_GPIO_WIFI_IRQ             147
#define VISION_GPIO_WIFI_SHUTDOWN_N       39

#define VISION_GPIO_UART2_RX 				51
#define VISION_GPIO_UART2_TX 				52

#define VISION_GPIO_KEYPAD_POWER_KEY		46


/* Macros assume PMIC GPIOs start at 0 */
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)     (pm_gpio + NR_GPIO_IRQS)
#define PM8058_GPIO_SYS_TO_PM(sys_gpio)    (sys_gpio - NR_GPIO_IRQS)

/* Proximity */
#define VISION_GPIO_PROXIMITY_INT_N		18

#define VISION_GPIO_COMPASS_INT			42
#define VISION_GPIO_GSENSOR_INT_N			180
#define VISION_LAYOUTS_XA_XB		{ \
		{ { 0,  1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ { 1,  0, 0}, { 0, -1,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}
#define VISION_LAYOUTS_XC			{ \
		{ { 0,  1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 0, -1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0, -1,  0}, {0, 0,  1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
					}

#define VISION_PMIC_GPIO_INT			(27)

#define VISION_GPIO_UP_INT_N          (142)

#define VISION_GPIO_PS_HOLD			(29)

#define VISION_MDDI_RSTz				(162)
#define VISION_LCD_ID0				(124)
#define VISION_LCD_RSTz_ID1			(126)
#define VISION_LCD_PCLK_1             (90)
#define VISION_LCD_DE                 (91)
#define VISION_LCD_VSYNC              (92)
#define VISION_LCD_HSYNC              (93)
#define VISION_LCD_G0                 (94)
#define VISION_LCD_G1                 (95)
#define VISION_LCD_G2                 (96)
#define VISION_LCD_G3                 (97)
#define VISION_LCD_G4                 (98)
#define VISION_LCD_G5                 (99)
#define VISION_LCD_B0					(22)
#define VISION_LCD_B1                 (100)
#define VISION_LCD_B2                 (101)
#define VISION_LCD_B3                 (102)
#define VISION_LCD_B4                 (103)
#define VISION_LCD_B5                 (104)
#define VISION_LCD_R0					(25)
#define VISION_LCD_R1                 (105)
#define VISION_LCD_R2                 (106)
#define VISION_LCD_R3                 (107)
#define VISION_LCD_R4                 (108)
#define VISION_LCD_R5                 (109)

#define VISION_LCD_SPI_CSz			(87)

/* Audio */
#define VISION_AUD_SPI_CSz            (89)
#define VISION_AUD_MICPATH_SEL_XA     (121)

/* Flashlight */
#define VISION_GPIO_FLASHLIGHT_FLASH	128
#define VISION_GPIO_FLASHLIGHT_TORCH	129

/* BT */
#define VISION_GPIO_BT_UART1_RTS      (134)
#define VISION_GPIO_BT_UART1_CTS      (135)
#define VISION_GPIO_BT_UART1_RX       (136)
#define VISION_GPIO_BT_UART1_TX       (137)
#define VISION_GPIO_BT_PCM_OUT        (138)
#define VISION_GPIO_BT_PCM_IN         (139)
#define VISION_GPIO_BT_PCM_SYNC       (140)
#define VISION_GPIO_BT_PCM_CLK        (141)
#define VISION_GPIO_BT_RESET_N        (41)
#define VISION_GPIO_BT_HOST_WAKE      (26)
#define VISION_GPIO_BT_CHIP_WAKE      (50)
#define VISION_GPIO_BT_SHUTDOWN_N     (38)

#define VISION_GPIO_USB_ID_PIN		(145)

#define VISION_VCM_PD					(34)
#define VISION_CAM1_PD                (31)
#define VISION_CLK_SEL                (23) /* camera select pin */
#define VISION_CAM2_PD			    (24)
#define VISION_CAM2_RSTz				(21)

#define VISION_CODEC_3V_EN			(36)
#define VISION_GPIO_TP_ATT_N			(20)

/* PMIC 8058 GPIO */
#define PMGPIO(x) (x-1)
#define VISION_AUD_MICPATH_SEL_XB		PMGPIO(10)
#define VISION_AUD_EP_EN				PMGPIO(13)
#define VISION_VOL_UP				PMGPIO(14)
#define VISION_VOL_DN				PMGPIO(15)
#define VISION_GPIO_CHG_INT		PMGPIO(16)
#define VISION_AUD_HP_DETz		PMGPIO(17)
#define VISION_AUD_SPK_EN			PMGPIO(18)
#define VISION_AUD_HP_EN			PMGPIO(19)
#define VISION_PS_SHDN			PMGPIO(20)
#define VISION_TP_RSTz			PMGPIO(21)
#define VISION_LED_3V3_EN			PMGPIO(22)
#define VISION_SDMC_CD_N			PMGPIO(23)
#define VISION_GREEN_LED			PMGPIO(24)
#define VISION_AMBER_LED			PMGPIO(25)
#define VISION_AUD_A3254_RSTz		PMGPIO(36)
#define VISION_WIFI_SLOW_CLK		PMGPIO(38)
#define VISION_SLEEP_CLK2			PMGPIO(39)

/*display*/
extern struct platform_device msm_device_mdp;
extern struct platform_device msm_device_mddi0;
extern int panel_type;

unsigned int vision_get_engineerid(void);
int vision_init_mmc(unsigned int sys_rev);
void __init vision_audio_init(void);
int vision_init_keypad(void);
int __init vision_wifi_init(void);
int __init vision_init_panel(void);
int __init vision_rfkill_init(void);
#endif /* __ARCH_ARM_MACH_MSM_BOARD_VISION_H */
