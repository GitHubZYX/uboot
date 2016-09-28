#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).o

SOBJS	:= lowlevel_init.o
COBJS	:= lc1860.o

COBJS	+= dqs_training.o
COBJS	+= ddr_zq_select.o
COBJS	+= comip_memctl_lc1860.o

ifdef CONFIG_SAVE_DDR_ON_FLASH
COBJS	+= comip_ddr_on_flash.o
endif

ifdef CONFIG_MULTI_DDR_SUPPORT
COBJS   += memctl_multi_ddr.o

ifdef CONFIG_SAMSUNG_KMR310001M
COBJS   += memctl_inits_KMR310001M.o
endif
ifdef CONFIG_HYNIX_H9TQ65A8GTMCUR
COBJS   += memctl_inits_H9TQ65A8GTMCUR.o
endif
ifdef CONFIG_MICRON_MT29TZZZ5D6YKFAH
COBJS   += memctl_inits_MT29TZZZ5D6YKFAH.o
endif
endif

ifdef CONFIG_BOARD_INFO_SUPPORT
COBJS   += comip_board_info.o
endif

ifndef CONFIG_COMIP_TARGETLOADER
ifdef CONFIG_DDR_MEMTEST
COBJS	+= comip_memorytest.o
endif

SOBJS	+= tl420_bin.o
COBJS	+= tl420.o
COBJS	+= comip_lp.o
endif

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
