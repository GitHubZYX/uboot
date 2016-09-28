#ifndef _COMIPFB_IF_H__
#define _COMIPFB_IF_H__

#define ENTER_ULPS	0x01
#define EXIT_ULPS	0x02

struct comipfb_info;
struct comipfb_dev_cmds;

struct comipfb_if {
	int (*init)(struct comipfb_info *fbi);
	int (*exit)(struct comipfb_info *fbi);
	int (*dev_cmd)(struct comipfb_info *fbi, struct comipfb_dev_cmds *cmds);
	void (*bl_change)(struct comipfb_info *fbi, int val);
	void (*te_trigger)(struct comipfb_info *fbi);	//for mipi only when te source is dsi control
};

extern struct comipfb_if* comipfb_if_get(struct comipfb_info *fbi);
extern int comipfb_if_mipi_dev_cmds(struct comipfb_info *fbi, struct comipfb_dev_cmds *cmds);
extern void comipfb_if_mipi_reset(struct comipfb_info *fbi);
extern int comipfb_read_lcm_id(struct comipfb_info *fbi, void *dev);

#endif /*_COMIPFB_IF_H__*/
