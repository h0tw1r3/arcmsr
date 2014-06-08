******************************************************************************************
**        O.S   : Linux
**   FILE NAME  : arcmsr.c
**        BY    : Erich Chen   
**   Description: SCSI RAID Device Driver for 
**                ARCMSR RAID Host adapter 
************************************************************************
** Copyright (C) 2002 - 2005, Areca Technology Corporation All rights reserved.
**
**     Web site: www.areca.com.tw
**       E-mail: erich@areca.com.tw
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
** History
**
**        REV#         DATE	            NAME	         DESCRIPTION
**     1.00.00.00    3/31/2004	       Erich Chen	     First release
**     1.10.00.04    7/28/2004         Erich Chen        modify for ioctl
**     1.10.00.06    8/28/2004         Erich Chen        modify for 2.6.x
**     1.10.00.08    9/28/2004         Erich Chen        modify for x86_64 
**     1.10.00.10   10/10/2004         Erich Chen        bug fix for SMP & ioctl
**     1.20.00.00   11/29/2004         Erich Chen        bug fix with arcmsr_bus_reset when PHY error
**     1.20.00.02   12/09/2004         Erich Chen        bug fix with over 2T bytes RAID Volume
**     1.20.00.04    1/09/2005         Erich Chen        fits for Debian linux kernel version 2.2.xx 
**     1.20.0X.07    3/28/2005         Erich Chen        sync for 1.20.00.07 (linux.org version)
**                                                       remove some unused function
**                                                       --.--.0X.-- is for old style kernel compatibility
**     1.20.0X.08    6/23/2005         Erich Chen        bug fix with abort command,in case of heavy loading when sata cable
**                                                       working on low quality connection
**     1.20.0X.09    9/12/2005         Erich Chen        bug fix with abort command handling,and firmware version check 
**                                                       and firmware update notify for hardware bug fix
**     1.20.0X.10    9/23/2005         Erich Chen        enhance sysfs function for change driver's max tag Q number.
**                                                       add DMA_64BIT_MASK for backward compatible with all 2.6.x
**                                                       add some useful message for abort command
**                                                       add ioctl code 'ARCMSR_IOCTL_FLUSH_ADAPTER_CACHE'
**                                                       customer can send this command for sync raid volume data
**     1.20.0X.11    9/29/2005         Erich Chen        by comment of Arjan van de Ven fix incorrect msleep redefine
**                                                       cast off sizeof(dma_addr_t) condition for 64bit pci_set_dma_mask
**     1.20.0X.12    9/30/2005         Erich Chen        bug fix with 64bit platform's ccbs using if over 4G system memory
**                                                       change 64bit pci_set_consistent_dma_mask into 32bit
**                                                       increcct adapter count if adapter initialize fail.
**                                                       miss edit at arcmsr_build_ccb....
**                                                       psge += sizeof(struct _SG64ENTRY *) =>  psge += sizeof(struct _SG64ENTRY)
**                                                       64 bits sg entry would be incorrectly calculated
**                                                       thanks Kornel Wieliczek give me kindly notify and detail description
**     1.20.0X.13   11/15/2005         Erich Chen        scheduling pending ccb with 'first in first out'
**                                                       new firmware update notify
**                  11/07/2006         Erich Chen        1.remove #include config.h and devfs_fs_kernel.h
**                                                       2.enlarge the timeout duration of each scsi command 
**                                                         it could aviod the vibration factor 
**                                                         with sata disk on some bad machine 
*********************************************************************************************

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


