README file for the arcmsr RAID controller SCSI driver
======================================================
History

        REV#         DATE	       NAME	         DESCRIPTION
     1.00.00.00    3/31/2004         Erich Chen        First release
     1.10.00.04    7/28/2004         Erich Chen        modify for ioctl
     1.10.00.06    8/28/2004         Erich Chen        modify for 2.6.x
     1.10.00.08    9/28/2004         Erich Chen        modify for x86_64 
     1.10.00.10   10/10/2004         Erich Chen        bug fix for SMP & ioctl
     1.20.00.00   11/29/2004         Erich Chen        bug fix with arcmsr_bus_reset when PHY error
     1.20.00.02   12/09/2004         Erich Chen        bug fix with over 2T bytes RAID Volume
     1.20.00.04    1/09/2005         Erich Chen        fits for Debian linux kernel version 2.2.xx 
     1.20.00.05    2/20/2005         Erich Chen        cleanly as look like a Linux driver at 2.6.x
                                                       thanks for peoples kindness comment
                                                                Kornel Wieliczek
                                                                Christoph Hellwig 
                                                                Adrian Bunk 
                                                                Andrew Morton 
                                                                Christoph Hellwig 
                                                                James Bottomley 
                                                                Arjan van de Ven
     1.20.00.06    3/12/2005         Erich Chen        fix with arcmsr_pci_unmap_dma "unsigned long" cast,
                                                       modify PCCB POOL allocated by "dma_alloc_coherent"
                                                       (Kornel Wieliczek's comment)
                                                       
     1.20.00.07    3/23/2005         Erich Chen        bug fix with arcmsr_scsi_host_template_init ocur segmentation fault,
                                                       if RAID adapter does not on PCI slot and modprobe/rmmod this driver twice.
                                                       bug fix enormous stack usage (Adrian Bunk's comment)
=====================================================================================================                                                       
Support:
      
        Linux SCSI RAID driver technical support 

        mail address: erich@areca.com.tw
                 Tel: 886-2-8797-4060 Ext.203
                 Fax: 886-2-8797-5970
            Web site: http//:www.areca.com.tw (We support Linux RAID config tools)
======================================================================================================
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     /usr/src/linux/drivers/scsi/Makefile
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	  ...
	  ....
         
	  obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/
         
	  ....
	  ...
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         
     /usr/src/linux/drivers/scsi/Kconfig
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	  ...
	  ....
config SCSI_ARCMSR
	tristate "ARECA ARC11X0[PCI-X]/ARC12X0[PCI-EXPRESS] SATA-RAID support"
	depends on  PCI && SCSI
	help
	  This driver supports all of ARECA's SATA RAID controllers cards. 
	  This is an ARECA maintained driver by Erich Chen. 
	  If you have any problems, please mail to: < erich@areca.com.tw >
	  Areca have suport Linux RAID config tools
  
	  < http://www.areca.com.tw >

	  To compile this driver as a module, choose M here: the
	  module will be called arcmsr (modprobe arcmsr) .
         
	  ....
	  ...
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

###############################################################################

Make new Linux kernel......


Copyright
---------
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
