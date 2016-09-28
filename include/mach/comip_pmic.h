#ifndef __COMIP_PMIC_H__
#define __COMIP_PMIC_H__

#define PMIC_RTC_ALARM_NORMAL                           (0)
#define PMIC_RTC_ALARM_POWEROFF                         (1)
#define PMIC_RTC_ALARM_REBOOT                           (PMIC_RTC_ALARM_NORMAL)

enum {
	PU_REASON_HARDWARE_RESET,
	PU_REASON_WATCHDOG,
	PU_REASON_RTC_ALARM,
	PU_REASON_PWR_KEY_PRESS,
	PU_REASON_USB_CABLE,
	PU_REASON_USB_FACTORY,
	PU_REASON_USB_CHARGER,
	PU_REASON_REBOOT_NORMAL,
	PU_REASON_REBOOT_FACTORY,
	PU_REASON_REBOOT_RECOVERY,
	PU_REASON_REBOOT_FOTA,
	PU_REASON_REBOOT_CRITICAL,
	PU_REASON_REBOOT_UNKNOWN,
	PU_REASON_LOW_PWR_RESET,
	PU_REASON_MAX
};

enum {
	REBOOT_NONE,
	REBOOT_NORMAL,
	REBOOT_POWER_KEY,	/* Power key reboot in power off charge mode. */
	REBOOT_RTC_ALARM,	/* RTC alarm reboot in power off charge mode. */
	REBOOT_RECOVERY,
	REBOOT_FOTA,
	REBOOT_CRITICAL,
	REBOOT_FACTORY,
	REBOOT_UNKNOWN,
	REBOOT_MAX
};

enum {
	BOOT_MODE_NORMAL,
	BOOT_MODE_AMT1,
	BOOT_MODE_AMT3,
	BOOT_MODE_RECOVERY,
	BOOT_MODE_MAX
};

#define REBOOT_REASON_MASK		(0x1f)
#define REBOOT_USER_FLAG		(0x20)

struct pmic_info {
	int buck1_vout0;
	int buck1_vout1;
	int buck2_vout0;
	int buck2_vout1;
	int buck2_vout2;
	int buck2_vout3;
	int buck3_vout0;
	int buck3_vout1;
	int buck3_vout2;
	int buck3_vout3;
	int buck4_vout0;
	int buck4_vout1;
	int buck5_vout0;
	int buck5_vout1;
	int buck6_vout0;
	int buck6_vout1;
	int startup_type;
	int shutdown_type;
	int power_on_type;
	int reboot_type;
	int (*init)(struct pmic_info *info);
	void (*reboot_reason_set)(u8 type);
	int (*power_on_key_check)(struct pmic_info *info);
	int (*battery_voltage_check)(struct pmic_info *info);
	int (*vibrator_enable)(struct pmic_info *info);
	int (*lcdio_enable)(struct pmic_info *info, int enable);
	int (*lcdcore_enable)(struct pmic_info *info, int enable);
};

extern void pmic_reboot(void);
extern void pmic_power_off(void);
extern int pmic_reg_read(u8 slave_addr, u8 reg, u8 *value);
extern int pmic_reg_write(u8 slave_addr, u8 reg, u8 value);
extern int pmic_reg_bit_write(u8 slave_addr, u8 reg, u8 mask, u8 value);
extern int pmic_init(void);
extern int pmic_power_up_reason_get(void);
extern int pmic_reboot_reason_get(void);
extern void pmic_reboot_reason_set(u8 type);
extern int pmic_power_on_key_check(void);
extern int pmic_vibrator_enable_set(void);
extern int pmic_lcdio_enable(int enable);
extern int pmic_lcdcore_enable(int enable);

/*extern special charger attr*/
#if defined(CONFIG_CHARGER_BQ24158)||defined(CONFIG_CHARGER_NCP1851)||defined(CONFIG_CHARGER_BQ24296)
#define CONFIG_EXTERN_CHARGER
#endif

#ifdef CONFIG_EXTERN_CHARGER
extern int special_charger_charging_init(u8 flag);
#else
static inline int special_charger_charging_init(u8 flag){return -1;}
#endif


#endif /* __COMIP_PMIC_H__ */
