# File: arcmsr/Makefile
# Makefile for the Areca ARC-11XX/12XX/16XX SAS/SATA RAID Controller
# Copyright (C) 2009 - 2012 Areca Technology Corporation <support@areca.com.tw>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, version 2
# of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
#
# Main targets:
#    all (the default) : make all
#    clean             : clean files
#    install           : make all + install
#    uninstall         : uninstall
#
# Notes :
#    - install and uninstall must be made as root

ifeq ($(KVER),)
  ifeq ($(KDIR),)
    KVER = $(shell uname -r)
    KDIR := /lib/modules/$(KVER)/build
  endif
else
  KDIR := /lib/modules/$(KVER)/build
endif

MODULE_NAME = arcmsr
INSTALL_DIR := /lib/modules/$(shell uname -r)/extra
arcmsr-objs := arcmsr_attr.o arcmsr_hba.o
obj-m := arcmsr.o

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd) BUILD_INI=m
install: all
	$(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd) BUILD_INI=m modules_install
	-depmod -a $(KVER)
uninstall:
	rm -f $(INSTALL_DIR)/$(MODULE_NAME).ko
	-/sbin/depmod -a $(KVER)
	rmmod $(MODULE_NAME)
clean:
	rm -f *.o *.ko .*.cmd *.mod.c .*.d .depend *~ Modules.symvers Module.symvers Module.markers modules.order
	rm -rf .tmp_versions
