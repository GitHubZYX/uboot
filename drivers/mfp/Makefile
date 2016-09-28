
include $(TOPDIR)/config.mk

LIB	:= $(obj)libmfp.o

COBJS-$(CONFIG_COMIP_MFP) += mfp.o
COBJS-$(CONFIG_COMIP_MFP2) += mfp2.o

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
