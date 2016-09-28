
#include <common.h>

/* I2C_CON. */
#define I2C_CON_NOMEANING			(1 << 6)
#define I2C_CON_RESTART_EN			(1 << 5)
#define I2C_CON_10BIT_ADDR			(1 << 4)
#define I2C_CON_7BIT_ADDR			(0 << 4)
#define I2C_CON_STANDARD_MODE			(1 << 1)
#define I2C_CON_FAST_MODE			(2 << 1)
#define I2C_CON_MASTER_MODE			(1 << 0)

/* I2C_STATUS. */
#define I2C_STATUS_RX_FIFO_FULL			(1 << 4)
#define I2C_STATUS_RX_FIFO_NOT_EMPTY		(1 << 3)
#define I2C_STATUS_TX_FIFO_EMPTY		(1 << 2)
#define I2C_STATUS_TX_FIFO_NOT_FULL		(1 << 1)
#define I2C_STATUS_ACTIVITY			(1 << 0)

/* I2C_DATA_CMD. */
#define I2C_DATA_CMD_RESTART			(1 << 10)
#define I2C_DATA_CMD_STOP			(1 << 9)
#define I2C_DATA_CMD_READ			(1 << 8)

/* I2C_INTR_EN & I2C_INTR_STAT & I2C_RAW_INTR_STAT. */
#define I2C_INTR_GEN_CALL			(1 << 11)
#define I2C_INTR_START_DET			(1 << 10)
#define I2C_INTR_STOP_DET			(1 << 9)
#define I2C_INTR_ACTIVITY			(1 << 8)
#define I2C_TX_ABORT				(1 << 6)
#define I2C_TX_EMPTY				(1 << 4)
#define I2C_TX_OVER				(1 << 3)
#define I2C_RX_FULL				(1 << 2)
#define I2C_RX_OVER				(1 << 1)
#define I2C_RX_UNDER				(1 << 0)

/* Timeout values. */
#define I2C_BUSY_TIMEOUT			(100000) /* us. */
#define I2C_RX_FIFO_WAIT_TIMEOUT		(1000) /* us. */
#define I2C_TX_FIFO_WAIT_TIMEOUT		(1000) /* us. */
#define I2C_SEND_STOP_TIMEOUT			(1000) /* us. */

/* Retry times. */
#define I2C_TRIES				(3)

static unsigned int comip_i2c_base(uchar id)
{
	unsigned int base = COM_I2C_BASE;

	switch (id) {
	case I2C0:
		base = I2C0_BASE;
		break;
	case I2C1:
		base = I2C1_BASE;
		break;
	case I2C2:
		base = I2C2_BASE;
		break;
	case I2C3:
		base = I2C3_BASE;
		break;
	case COM_I2C:
		base = COM_I2C_BASE;
		break;
	}

	return base;
}

static void comip_i2c_write_reg(uchar id, unsigned int value, unsigned int offset)
{
	unsigned int base = comip_i2c_base(id);

	__raw_writel(value, base + offset);
}

static unsigned int comip_i2c_read_reg(uchar id, unsigned int offset)
{
	unsigned int base = comip_i2c_base(id);

	return __raw_readl(base + offset);
}

static int comip_i2c_check_status(uchar id, uint32_t flag)
{
	return !!(comip_i2c_read_reg(id, I2C_STATUS) & flag);
}

static int comip_i2c_wait_for_bb(uchar id, uint32_t timeout)
{
	while (timeout-- && comip_i2c_check_status(id, I2C_STATUS_ACTIVITY))
		udelay(1);

	if (comip_i2c_check_status(id, I2C_STATUS_ACTIVITY))
		return -1;

	return 0;
}

static void comip_i2c_start_message(uchar id, uchar slave_addr)
{
	/* Clr all INTR. */
	comip_i2c_read_reg(id, I2C_TX_ABRT_SOURCE);
	comip_i2c_read_reg(id, I2C_CLR_INTR);

	/* Set slave address. */
	comip_i2c_write_reg(id, slave_addr & 0x7f, I2C_TAR);

	/* Enable I2c module. */
	comip_i2c_write_reg(id, 1, I2C_ENABLE);
}

static void comip_i2c_stop_message(uchar id)
{
	/* Disable I2c module. */
	comip_i2c_write_reg(id, 0, I2C_ENABLE);
}

static int comip_i2c_wait_rx_fifo_not_empty(uchar id, uint32_t timeout)
{
	while (timeout-- && !comip_i2c_check_status(id, I2C_STATUS_RX_FIFO_NOT_EMPTY))
		udelay(1);

	if (!comip_i2c_check_status(id, I2C_STATUS_RX_FIFO_NOT_EMPTY))
		return -1;

	return 0;
}

static int comip_i2c_wait_tx_fifo_empty(uchar id, uint32_t timeout)
{
	while (timeout-- && !comip_i2c_check_status(id, I2C_STATUS_TX_FIFO_EMPTY))
		udelay(1);

	if (!comip_i2c_check_status(id, I2C_STATUS_TX_FIFO_EMPTY))
		return -1;

	return 0;
}

static int comip_i2c_wait_stop_flag(uchar id, uint32_t timeout)
{
	while (timeout-- && !(comip_i2c_read_reg(id, I2C_RAW_INTR_STAT) & I2C_INTR_STOP_DET))
		udelay(1);

	if (!(comip_i2c_read_reg(id, I2C_RAW_INTR_STAT) & I2C_INTR_STOP_DET))
		return -1;

	return 0;
}

static int __comip_i2c_write(uchar id, uchar slave_addr, uchar addr, uchar data)
{
	int ret;

	ret = comip_i2c_wait_for_bb(id, I2C_BUSY_TIMEOUT);
	if (ret)
		return ret;

	comip_i2c_start_message(id, slave_addr);

	comip_i2c_write_reg(id, addr, I2C_DATA_CMD);
	comip_i2c_write_reg(id, data | I2C_DATA_CMD_STOP, I2C_DATA_CMD);

	ret = comip_i2c_wait_tx_fifo_empty(id, I2C_TX_FIFO_WAIT_TIMEOUT);
	if (ret)
		goto out;

	ret = comip_i2c_wait_stop_flag(id, I2C_SEND_STOP_TIMEOUT);

out:
	comip_i2c_stop_message(id);

	return ret;
}

static int __comip_i2c_read(uchar id, uchar slave_addr, uchar addr, uchar *data)
{
	int ret;

	ret = comip_i2c_wait_for_bb(id, I2C_BUSY_TIMEOUT);
	if (ret)
		return ret;

	comip_i2c_start_message(id, slave_addr);

	comip_i2c_write_reg(id, addr, I2C_DATA_CMD);
	comip_i2c_write_reg(id, I2C_DATA_CMD_READ | I2C_DATA_CMD_STOP, I2C_DATA_CMD);

	ret = comip_i2c_wait_rx_fifo_not_empty(id, I2C_RX_FIFO_WAIT_TIMEOUT);
	if (ret)
		goto out;

	*data = (uchar)comip_i2c_read_reg(id, I2C_DATA_CMD);

	ret = comip_i2c_wait_stop_flag(id, I2C_SEND_STOP_TIMEOUT);

out:
	comip_i2c_stop_message(id);

	return ret;
}

int comip_i2c_init(uchar id, uchar fast_mode)
{
	unsigned char con = I2C_CON_NOMEANING
					| I2C_CON_RESTART_EN
					| I2C_CON_7BIT_ADDR
					| I2C_CON_MASTER_MODE;

	if (!fast_mode)
		con |= I2C_CON_STANDARD_MODE; /* Standard mode. */
	else
		con |= I2C_CON_FAST_MODE; /* Fast mode. */

	/* I2C config. */
	comip_i2c_write_reg(id, 0, I2C_ENABLE);
	comip_i2c_write_reg(id, con, I2C_CON);
	comip_i2c_write_reg(id, 54, I2C_SS_SCL_HCNT);
	comip_i2c_write_reg(id, 62, I2C_SS_SCL_LCNT);
	comip_i2c_write_reg(id, 10, I2C_FS_SCL_HCNT);
	comip_i2c_write_reg(id, 18, I2C_FS_SCL_LCNT);

	/* Set the FIFO Rx/Tx triggle level. */
	comip_i2c_write_reg(id, 0, I2C_RX_TL);
	comip_i2c_write_reg(id, 0, I2C_TX_TL);

	/* Disable all intr. */
	comip_i2c_write_reg(id, 0, I2C_INTR_EN);

	return 0;
}

int comip_i2c_write(uchar id, uchar slave_addr, uchar addr, uchar data)
{
	int ret;
	int i;

	for (i = I2C_TRIES; i > 0; i--) {
		ret = __comip_i2c_write(id, slave_addr, addr, data);
		if (!ret)
			return ret;

		udelay(1000);
	}

	return ret;
}

int comip_i2c_read(uchar id, uchar slave_addr, uchar addr, uchar *data)
{
	int ret;
	int i;

	for (i = I2C_TRIES; i > 0; i--) {
		ret = __comip_i2c_read(id, slave_addr, addr, data);
		if (!ret)
			return ret;

		udelay(1000);
	}

	return ret;
}

