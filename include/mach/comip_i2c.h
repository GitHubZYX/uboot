
#ifndef __COMIP_I2C_H__
#define __COMIP_I2C_H__

enum {
	I2C0,
	I2C1,
	I2C2,
	I2C3,
	COM_I2C,
};

extern int comip_i2c_init(uchar id, uchar fast_mode);
extern int comip_i2c_write(uchar id, uchar slave_addr, uchar addr, uchar data);
extern int comip_i2c_read(uchar id, uchar slave_addr, uchar addr, uchar *data);

#endif /* __COMIP_I2C_H__ */
