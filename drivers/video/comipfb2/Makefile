#
# (C) Copyright 2000-2007
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)libvideo.o

COBJS-$(CONFIG_FB_COMIP2) += comipfb.o comipfb_dev.o comipfb_if.o
COBJS-$(CONFIG_FB_COMIP_LCDC2) += comip_lcdc2.o
COBJS-$(CONFIG_FB_COMIP_MIPI) += mipi_interface.o mipi_cmd.o

COBJS-$(CONFIG_LCD_AUO_OTM1285A_OTP) += lcd_auo_otm1285a_otp.o
COBJS-$(CONFIG_LCD_AUO_R61308OTP) +=  lcd_auo_r61308otp.o
COBJS-$(CONFIG_LCD_AUO_NT35521) += lcd_auo_nt35521.o
COBJS-$(CONFIG_LCD_SHARP_R69431) += lcd_sharp_eR69431.o

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
