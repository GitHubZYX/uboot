#ifndef __MFP_H
#define __MFP_H

#define mfp_to_gpio(m)		((m) % 256)
#define gpio_to_mfp(x)		(x)

#define MFP_PIN_GPIO(x)		(x)
#define MFP_CONFIG_ARRAY(x)	x, sizeof(x) / sizeof(x[0])

#if defined(CONFIG_COMIP_MFP)

enum {
	MFP_PIN_MODE_0 = 0,
	MFP_PIN_MODE_1,
	MFP_PIN_MODE_2,
	MFP_PIN_MODE_3,
	MFP_PIN_MODE_GPIO,
	MFP_PIN_MODE_MAX
};

enum {
	MFP_PULL_DISABLE = 0,
	MFP_PULL_UP,
	MFP_PULL_DOWN,
	MFP_PULL_MODE_MAX,
};

typedef unsigned int mfp_pin;
typedef unsigned int mfp_pin_mode;
typedef unsigned int mfp_pull_id;
typedef unsigned int mfp_pull_mode;

struct mfp_pin_cfg {
	mfp_pin id;
	mfp_pin_mode mode;
};

struct mfp_pull_cfg {
	mfp_pull_id id;
	mfp_pull_mode mode;
};

extern int comip_mfp_config(mfp_pin id, mfp_pin_mode mode);
extern int comip_mfp_config_pull(mfp_pin id, mfp_pull_mode mode);
extern int comip_mfp_config_array(struct mfp_pin_cfg *pin, int size);
extern int comip_mfp_config_pull_array(struct mfp_pull_cfg *pull, int size);

#elif defined(CONFIG_COMIP_MFP2)

enum {
	MFP_PIN_MODE_0 = 0,
	MFP_PIN_MODE_1,
	MFP_PIN_MODE_2,
	MFP_PIN_MODE_GPIO = MFP_PIN_MODE_2,
	MFP_PIN_MODE_3,
	MFP_PIN_MODE_MAX
};

enum {
	MFP_PULL_DISABLE = 0,
	MFP_PULL_DOWN,
	MFP_PULL_UP,
	MFP_PULL_BOTH,
	MFP_PULL_MODE_MAX,
};

enum {
	MFP_DS_2MA = 1,
	MFP_DS_4MA,
	MFP_DS_6MA,
	MFP_DS_8MA,
	MFP_DS_10MA,
	MFP_DS_12MA,
	MFP_DS_MODE_MAX,
};

typedef unsigned int mfp_pin;
typedef unsigned int mfp_pin_mode;
typedef unsigned int mfp_pull_mode;
typedef unsigned int mfp_ds_mode;

struct mfp_pin_cfg {
	mfp_pin id;
	mfp_pin_mode mode;
};

struct mfp_pull_cfg {
	mfp_pin id;
	mfp_pull_mode mode;
};

struct mfp_ds_cfg {
	mfp_pin id;
	mfp_ds_mode mode;
};

extern int comip_mfp_config(mfp_pin id, mfp_pin_mode mode);
extern int comip_mfp_config_pull(mfp_pin id, mfp_pull_mode mode);
extern int comip_mfp_config_ds(mfp_pin id, mfp_ds_mode mode);
extern int comip_mfp_config_array(struct mfp_pin_cfg *pin, int size);
extern int comip_mfp_config_pull_array(struct mfp_pull_cfg *pull, int size);
extern int comip_mfp_config_ds_array(struct mfp_ds_cfg *pull, int size);

#else

#define comip_mfp_config(id, mode)
#define comip_mfp_config_pull(id, mode)
#define comip_mfp_config_ds(id, mode)
#define comip_mfp_config_array(pin, size)
#define comip_mfp_config_pull_array(pull, size)
#define comip_mfp_config_ds_array(ds, size)

#endif

#endif /* __MFP_H */
