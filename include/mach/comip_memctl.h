
#ifndef __COMIP_MEMCTL_H__
#define __COMIP_MEMCTL_H__

struct memctl_data {
	unsigned int reg;
	unsigned int val;
};

extern int memctl_init(void);

#endif /* __COMIP_MEMCTL_H__ */
