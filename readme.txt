******************************************************************************************
**        O.S   : Linux
**   FILE NAME  : arcmsr.c
**   BY    : Erich Chen   
**   Description: SCSI RAID Device Driver for 
**                ARCMSR RAID Host adapter 
************************************************************************
** Copyright (C) 2002 - 2005, Areca Technology Corporation All rights reserved.
**
**     Web site: www.areca.com.tw
**       E-mail: support@areca.com.tw
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
************************************************************************
** Redistribution and use in source and binary forms,with or without
** modification,are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice,this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice,this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES,INCLUDING,BUT NOT LIMITED TO,THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,INDIRECT,
** INCIDENTAL,SPECIAL,EXEMPLARY,OR CONSEQUENTIAL DAMAGES(INCLUDING,BUT
** NOT LIMITED TO,PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA,OR PROFITS; OR BUSINESS INTERRUPTION)HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY,WHETHER IN CONTRACT,STRICT LIABILITY,OR TORT
**(INCLUDING NEGLIGENCE OR OTHERWISE)ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE,EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**************************************************************************

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
       Thanks to: 
                 Tamas TEVESZ <ice@extreme.hu> kindness comment for 
                       Out-of-tree build
                 Doug Goldstein <doug@monetra.com> kindness comment for 
                       Kconfig.arcmsr file create
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

There are two ways to build the arcmsr driver.

1. In-tree build
================

Copy the `arcmsr' directory to $KSRC/drivers/scsi, then manually edit the
following two files to add the options necessary to build the driver:
--------------------------------------------------------------------------------------------
Add the following line to $KSRC/drivers/scsi/Makefile:

obj-$(CONFIG_SCSI_ARCMSR)       += arcmsr/

--------------------------------------------------------------------------------------------
Add the following block to $KSRC/drivers/scsi/Kconfig:

source "drivers/scsi/arcmsr/Kconfig.arcmsr"

--------------------------------------------------------------------------------------------
Now configure and build your kernel as you usually do, paying attention to
select the Device Drivers -> SCSI device support -> SCSI low-level drivers -> ARECA SATA/SAS RAID Host Controller option.

2. Out-of-tree build
====================

This option is particularly useful if you are running a stock kernel supplied
by your Linux distributor, or for other reason do not want to perform a full
kernel build. Please note that this way you will only be able to build a module
version of the arcmsr driver; also note that you will need at least the headers
and configuration file your current kernel was built with.

Provided that you meet all the above prerequisites, building the arcmsr driver
module is done with the following command:

# cd arcmsr 
# make -C /lib/modules/`uname -r`/build CONFIG_SCSI_ARCMSR=m SUBDIRS=$PWD modules 

3. For kernel 2.4 users
======================

Copy the `arcmsr' directory to $KSRC/drivers/scsi, then manually edit the
following two files to add the options necessary to build the driver:
--------------------------------------------------------------------------------------------
Add the following line to $KSRC/drivers/scsi/Makefile:

subdir-$(CONFIG_SCSI_ARCMSR)	+= arcmsr
obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/arcmsr.o
--------------------------------------------------------------------------------------------
Add the following block to $KSRC/drivers/scsi/Config.in:

if [ "$CONFIG_PCI" = "y" ]; then
   dep_tristate 'ARECA ARC11X0[PCI-X]/ARC12X0[PCI-EXPRESS] SATA-RAID support' CONFIG_SCSI_ARCMSR $CONFIG_SCSI
 fi

--------------------------------------------------------------------------------------------
Now configure and build your kernel as you usually do, paying attention to
select the Device Drivers -> SCSI device support -> SCSI low-level drivers -> ARECA SATA/SAS RAID Host Controller option.

---------------------------------------------------------------------------------------------
Notice :

if the kernel have arcmsr driver buildin but the version is older than 1.20.00.15, 
you have to added new controller's defination to avoid compile error...

Add the following block to $KSRC/include/linux/pci_ids.h
#define PCI_DEVICE_ID_ARECA_1200    0x1200
#define PCI_DEVICE_ID_ARECA_1201    0x1201
#define PCI_DEVICE_ID_ARECA_1202    0x1202

