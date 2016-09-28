
include $(TOPDIR)/config.mk

LIB	:= $(obj)libpmic.o

COBJS-$(CONFIG_COMIP_PMIC) += comip_pmic.o
COBJS-$(CONFIG_CHARGER_BQ24158) += bq24158_charger.o
COBJS-$(CONFIG_CHARGER_BQ24296) += bq24296_charger.o
COBJS-$(CONFIG_CHARGER_NCP1851) += ncp1851_charger.o
COBJS-$(CONFIG_PMIC_LC1100HT) += comip_lc1100ht.o
COBJS-$(CONFIG_PMIC_LC1100H) += comip_lc1100h.o
COBJS-$(CONFIG_PMIC_LC1132) += comip_lc1132.o
COBJS-$(CONFIG_PMIC_LC1160) += comip_lc1160.o
COBJS-$(CONFIG_NCP6335) += ncp6335.o
COBJS-$(CONFIG_FAN53555) += fan53555.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
