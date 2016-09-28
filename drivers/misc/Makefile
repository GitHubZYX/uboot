
include $(TOPDIR)/config.mk

LIB	:= $(obj)libmisc.o

ifdef CONFIG_COMIP_TARGETLOADER
COBJS-y += comip_flashex.o
else
COBJS-y	+= comip_setup.o
COBJS-y	+= comip_recovery.o
COBJS-y	+= comip_flash.o
endif

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
