/*
******************************************************************************************
**	O.S		: Linux
**	FILE NAME	: arcmsr.c
**	BY		: Erich Chen   
**	Description	: SCSI RAID Device Driver for Areca RAID Host Bus Adapter 
************************************************************************
** Copyright (C) 2002 - 2005, Areca Technology Corporation All rights reserved.
**
**	Web site: www.areca.com.tw
**	E-mail: support@areca.com.tw
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public	License version 2 as
** published by the Free Software Foundation.
** This	program	is distributed in the hope that	it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS	IS'' AND ANY EXPRESS OR
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
** REV#		DATE		NAME				DESCRIPTION
** 1.00.00.00	03/31/2004	Erich Chen			First release
** 1.10.00.04	07/28/2004	Erich Chen			modify for ioctl
** 1.10.00.06	08/28/2004	Erich Chen			modify for 2.6.x
** 1.10.00.08	09/28/2004	Erich Chen			modify for x86_64 
** 1.10.00.10	10/10/2004	Erich Chen			bug fix for SMP& ioctl
** 1.20.00.00	11/29/2004	Erich Chen			bug fix with arcmsr_bus_reset when PHY error
** 1.20.00.02	12/09/2004	Erich Chen			bug fix with over 2T bytes RAIDVolume
** 1.20.00.04	01/09/2005	Erich Chen			bug fix fits for Debian linux kernel version 2.2.xx 
** 1.20.0X.07	03/28/2005	Erich Chen			bug fix sync for 1.20.00.07 (linux.org version)
**											remove some unused function
**											--.--.0X.-- is  for old style kernel compatibility
** 1.20.0X.08 	06/23/2005	Erich Chen			
**											1.bug fix with abort command, in case of heavy loading 
**											when sata cable working on low quality connection
** 1.20.0X.09	9/12/2005	Erich Chen			
**											1.bug fix with abort command handling,
**											and firmware version check and FW update notify 
**											for HW bug fix
** 1.20.0X.10	9/23/2005	Erich Chen			
**											1.enhance sysfs function for change driver's max tag Q number.
**											2.add DMA_64BIT_MASK for backward compatible with all 2.6.x
**											3.add some useful message for abort command
**											4.add ioctl code 'ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE'
**											customer can send this command for sync raid volume data
** 1.20.0X.11	9/29/2005	Erich Chen
**											1.by comment of Arjan van de Ven fix incorrect msleep redefine
**											2.cast off sizeof(dma_addr_t) condition for 64bit pci_set_dma_mask
** 1.20.0X.12	9/30/2005	Erich Chen
**											1.bug fix with 64bit platform's ccbs using 
**											if over 4G system memory
**											2.change 64bit pci_set_consistent_dma_mask into 32bit
**											3.increcct adapter count if adapter initialize fail.
**											4.miss-edit psge += sizeof(struct SG64ENTRY *) as 
**											psge += sizeof(struct SG64ENTRY) at arcmsr_build_ccb
**											64 bits sg entry would be incorrectly calculated
**											thanks to Kornel Wieliczek's great help
** 1.20.0X.13	11/15/2005	Erich Chen
**											1.scheduling pending ccb with 'first in first out'
**											new firmware update notify
** 1.20.0X.13	11/07/2006	Erich Chen
**											1.remove #include config.h and devfs_fs_kernel.h
**											2.enlarge the timeout duration for each scsi command 
**											it could aviod the vibration factor with sata disk
**											on some bad machine
** 1.20.0X.14	05/02/2007	Erich Chen & Nick Cheng
**											1.add PCI-Express error recovery function and AER capability
**											2.add the selection of ARCMSR_MAX_XFER_SECTORS_B=4096 
**											if firmware version is newer than 1.41
**											3.modify arcmsr_iop_reset to improve the stability
**											4.delect arcmsr_modify_timeout routine 
**											because it would malfunction as removal and recovery the lun
**											if somebody needs to adjust the scsi command timeout value, 
**											the script could be available on Areca FTP site 
**											or contact Areca support team 
**											5.modify the ISR, arcmsr_interrupt routine, 
**											to prevent the inconsistent with sg_mod driver 
**											if application directly calls the arcmsr driver 
**											w/o passing through scsi mid layer		
**											6.delect the callback function, arcmsr_ioctl
**
**
** 1.20.0X.15	23/08/2007	Erich Chen & Nick Cheng
**											1.support ARC1200/1201/1202
** 1.20.0X.15	01/10/2007	Erich Chen & Nick Cheng
**											1.add arcmsr_enable_eoi_mode()
** 1.20.0X.15	04/12/2007	Erich Chen & Nick Cheng		
**											1.delete the limit of if dev_aborts[id][lun]>1, then
**											acb->devstate[id][lun] = ARECA_RAID_GONE in arcmsr_abort()
**											to wait for OS recovery on delicate HW
**											2.modify arcmsr_drain_donequeue() to ignore unknown command
**											and let kernel process command timeout.
**											This could handle IO request violating max. segments 
**											while Linux XFS over DM-CRYPT. 
**											Thanks to Milan Broz's comments <mbroz@redhat.com>
**
** 1.20.0X.15	24/12/2007	Erich Chen & Nick Cheng
**											1.fix the portability problems
**											2.fix type B where we should _not_ iounmap() acb->pmu;
**											it's not ioremapped.
**											3.add return -ENOMEM if ioremap() fails
**											4.transfer IS_SG64_ADDR w/ cpu_to_le32() in arcmsr_build_ccb
**											5.modify acb->devstate[i][j] as ARECA_RAID_GONE 
**											instead of ARECA_RAID_GOOD in arcmsr_alloc_ccb_pool()
**											6.fix arcmsr_cdb->Context as (unsigned long)arcmsr_cdb
**											7.add the checking state of (outbound_intstatus &
**											ARCMSR_MU_OUTBOUND_HANDLE_INT) == 0 in arcmsr_handle_hba_isr
**											8.replace pci_alloc_consistent()/pci_free_consistent() 
**											with kmalloc()/kfree() in arcmsr_iop_message_xfer()
**											9. fix the release of dma memory for type B in arcmsr_free_ccb_pool()
**											10.fix the arcmsr_polling_hbb_ccbdone()
** 1.20.0X.15	02/27/2008	Erich Chen & Nick Cheng		
**											1.arcmsr_iop_message_xfer() is called from 
**											atomic context under the queuecommand scsi_host_template
**											handler. James Bottomley pointed out that the current
**											GFP_KERNEL|GFP_DMA flags are wrong: firstly we are in
**											atomic context, secondly this memory is not used for DMA.
**											Also removed some unneeded casts.
**											Thanks to Daniel Drake <dsd@gentoo.org>
** 1.20.0X.15	04/07/2008	Erich Chen & Nick Cheng
**											1. add the function of HW reset for Type_A for kernel version greater than 2.6.0
**											2. add the function to automatic scan as the volume added or delected for kernel version greater than 2.6.0
**											3. support the notification of the FW status to the AP layer for kernel version greater than 2.6.0
** 1.20.0X.15	03/06/2008	Erich Chen & Nick Cheng					
**											1. support SG-related functions after kernel-2.6.2x
******************************************************************************************
*/
#define	ARCMSR_DEBUG			0
#define	ARCMSR_PP_RESET		0	/*the machanism to increase or not the waiting period for scsi device reset*/
#define	ARCMSR_FW_POLLING		1	/*the machanism to poll the FW status and discover the hot-plug and hot-remove scsi device*/
/*****************************************************************************************/
#if defined __KERNEL__
	#include <linux/version.h>
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif
	/* modversions.h should be before should be before module.h */
    	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		#if defined( MODVERSIONS )
			#include <config/modversions.h>
		#endif
    	#endif

	#if defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
		int sleeptime = 20;
		int retrycount = 12;
	#endif

	#include <linux/module.h>
	#include <asm/dma.h>
	#include <asm/io.h>
	#include <asm/system.h>
	#include <asm/uaccess.h>
	#include <linux/delay.h>
	#include <linux/signal.h>
	#include <linux/errno.h>
	#include <linux/kernel.h>
	#include <linux/ioport.h>
	#include <linux/pci.h>
	#include <linux/proc_fs.h>
	#include <linux/string.h>
	#include <linux/ctype.h>
	#include <linux/interrupt.h>
	#include <linux/smp_lock.h>
	#include <linux/stddef.h>

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,5,0)
		#include <linux/pci_ids.h>
		#include <linux/moduleparam.h>
		#include <linux/blkdev.h>
	#else
		#include <linux/blk.h>
	#endif
	#include <linux/timer.h>
	#include <linux/reboot.h>
	#include <linux/sched.h>
	#include <linux/init.h>
	#include <linux/highmem.h>
	
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
		# include <linux/spinlock.h>
	#else
		# include <asm/spinlock.h>
	#endif	/* 2,3,30 */

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,19)
		# include <linux/aer.h>
		# include <linux/pci_regs.h>
	#endif  

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,5,0)
		#include <scsi/scsi.h>
		#include <scsi/scsi_host.h>
		#include <scsi/scsi_cmnd.h>
		#include <scsi/scsi_tcq.h>
		#include <scsi/scsi_device.h>
		#include <scsi/scsicam.h>
	#else
		#include "/usr/src/linux/drivers/scsi/scsi.h"
		#include "/usr/src/linux/drivers/scsi/hosts.h"
		#include "/usr/src/linux/drivers/scsi/constants.h"
		#include "/usr/src/linux/drivers/scsi/sd.h"
	#endif

	#include "arcmsr.h"
#endif

MODULE_AUTHOR("Erich Chen <support@areca.com.tw>");
MODULE_DESCRIPTION("Areca (ARC11xx/12xx/13xx/16xx) SATA/SAS RAID Host Bus Adapter");
#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,0)
	MODULE_VERSION(ARCMSR_DRIVER_VERSION);
#endif

#ifdef MODULE_LICENSE
	MODULE_LICENSE("Dual BSD/GPL");
#endif
/*
**********************************************************************************
**********************************************************************************
*/
static u_int8_t arcmsr_adapterCnt=0;
static struct HCBARC arcmsr_host_control_block;
static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb);
static void arcmsr_iop_init(struct AdapterControlBlock *acb);
static void arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb);
static void arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb);
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock	*acb);
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb);
static void *arcmsr_get_firmware_spec(struct AdapterControlBlock *acb, int mode);
static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb);
static void arcmsr_define_adapter_type(struct AdapterControlBlock *acb);
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static void arcmsr_request_device_map(unsigned long pacb);
	static void arcmsr_request_hba_device_map(struct AdapterControlBlock *acb);
	static void arcmsr_request_hbb_device_map(struct AdapterControlBlock *acb);
#endif
static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb);
static void arcmsr_enable_outbound_ints(struct AdapterControlBlock *acb, u32 orig_mask);
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	static void arcmsr_message_isr_bh_fn(struct work_struct *work);
#elif (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static void arcmsr_message_isr_bh_fn(void *acb);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	#define	arcmsr_detect NULL
    	static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb);
    	static int arcmsr_probe(struct pci_dev *pdev,const struct pci_device_id *id);
	static void arcmsr_remove(struct pci_dev *pdev);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
		static void arcmsr_shutdown(struct pci_dev *pdev);
	#endif
#else
    	static void arcmsr_interrupt(struct AdapterControlBlock *acb);
    	int arcmsr_schedule_command(struct scsi_cmnd *pcmd);
    	int arcmsr_detect(Scsi_Host_Template *);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    	static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf);
	static struct notifier_block arcmsr_event_notifier={arcmsr_halt_notify,NULL,0};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	#ifdef CONFIG_SCSI_ARCMSR_AER
		static pci_ers_result_t arcmsr_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state);
       		static pci_ers_result_t arcmsr_pci_slot_reset(struct pci_dev *pdev);
	#endif	   
#endif
/*
**********************************************************************************
**********************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	#ifdef CONFIG_SCSI_ARCMSR_AER
		static struct pci_error_handlers arcmsr_pci_error_handlers=
		{ 
			.error_detected			= arcmsr_pci_error_detected,
 			.slot_reset 			= arcmsr_pci_slot_reset,
		};
	#endif
#endif
/*
**********************************************************************************
**********************************************************************************
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,30)
  struct proc_dir_entry	arcmsr_proc_scsi=
	{
		PROC_SCSI_ARCMSR,
		8,
		"arcmsr",
		S_IFDIR	| S_IRUGO | S_IXUGO,
		2
	};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	/* We do our own ID filtering. So, grab all SCSI storage class devices. */
	static struct pci_device_id arcmsr_device_id_table[] = {
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1110)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1120)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1130)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1160)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1170)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1200)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1201)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1202)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1210)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1220)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1230)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1260)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1270)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1280)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1380)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1381)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1680)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1681)},
		{0, 0},	/* Terminating entry */
	};
	MODULE_DEVICE_TABLE(pci, arcmsr_device_id_table);
	struct pci_driver arcmsr_pci_driver = 
	{
		.name		      				= "arcmsr",
		.id_table					= arcmsr_device_id_table,
		.probe		      				= arcmsr_probe,
		.remove	     	      				= arcmsr_remove,
        		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	    		.shutdown				= arcmsr_shutdown,
	    	#endif               	
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
			#ifdef CONFIG_SCSI_ARCMSR_AER
				.err_handler			= &arcmsr_pci_error_handlers,
			#endif	
        		#endif
	};
	/*
	*********************************************************************
	*********************************************************************
	*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	static irqreturn_t arcmsr_do_interrupt(int irq,void *dev_id)
	{
		irqreturn_t handle_state;
        		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
	    	struct AdapterControlBlock *acb;
	    	struct AdapterControlBlock *acbtmp;
		unsigned long flags;
		int i = 0;

		acb = (struct AdapterControlBlock *)dev_id;
		acbtmp = pHCBARC->acb[i];
		while((acb != acbtmp) && acbtmp && (i <ARCMSR_MAX_ADAPTER) ) {
			i++;
			acbtmp = pHCBARC->acb[i];
		}
		if(!acbtmp) {
			#if ARCMSR_DEBUG
				printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
	    	    	#endif
	    	    	return IRQ_NONE;
		}
		spin_lock_irqsave(acb->host->host_lock, flags);
		handle_state = arcmsr_interrupt(acb);
		spin_unlock_irqrestore(acb->host->host_lock, flags);
		return handle_state;
	}
	#else
	/*
	*********************************************************************
	*********************************************************************
	*/
	static irqreturn_t arcmsr_do_interrupt(int irq,void *dev_id, struct pt_regs *regs)
	{
		irqreturn_t handle_state;
        		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
	    	struct AdapterControlBlock *acb;
	    	struct AdapterControlBlock *acbtmp;
		unsigned long flags;
		int i = 0;

		acb = (struct AdapterControlBlock *)dev_id;
		acbtmp = pHCBARC->acb[i];
		while((acb != acbtmp) && acbtmp && (i <ARCMSR_MAX_ADAPTER) ) {
			i++;
			acbtmp = pHCBARC->acb[i];
		}
		if(!acbtmp) {
			#if ARCMSR_DEBUG
				printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
	    	    	#endif
	    	    	return IRQ_NONE;
		}
		spin_lock_irqsave(acb->host->host_lock, flags);
		handle_state = arcmsr_interrupt(acb);
		spin_unlock_irqrestore(acb->host->host_lock, flags);
		return handle_state;
	}
	#endif
	/*
	**********************************************************************************
	**********************************************************************************
	*/
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	static void arcmsr_message_isr_bh_fn(struct work_struct *work) 
	{
		struct AdapterControlBlock *acb = container_of(work,struct AdapterControlBlock, arcmsr_do_message_isr_bh);
		#if ARCMSR_DEBUG
			printk("arcmsr_message_isr_bh_fn..................................\n");
		#endif

		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {

				struct MessageUnit_A __iomem *reg  = acb->pmuA;
				char *acb_dev_map = (char *)acb->device_map;
				uint32_t __iomem *signature = (uint32_t __iomem*) (&reg->msgcode_rwbuffer[0]);
				char __iomem *devicemap = (char __iomem*) (&reg->msgcode_rwbuffer[21]);
				int target, lun;
				struct scsi_device *psdev;
				char diff;

				atomic_inc(&acb->rq_map_token);
				if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
					for(target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
						diff = (*acb_dev_map)^readb(devicemap);
						if (diff != 0) {
							char temp;
							*acb_dev_map = readb(devicemap);
							temp =*acb_dev_map;
							for(lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
								if((temp & 0x01)==1 && (diff & 0x01) == 1) {	
										#if ARCMSR_DEBUG
											printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
									scsi_add_device(acb->host, 0, target, lun);
								}else if((temp & 0x01) == 0 && (diff & 0x01) == 1) {
									psdev = scsi_device_lookup(acb->host, 0, target, lun);
									if (psdev != NULL ) {
										#if ARCMSR_DEBUG
											printk("REMOVE A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
										scsi_remove_device(psdev);
										scsi_device_put(psdev);
									}
								}
								temp >>= 1;
								diff >>= 1;
							}
						}
						devicemap++;
						acb_dev_map++;
					}
				}
				break;
			}

			case ACB_ADAPTER_TYPE_B: {
				struct MessageUnit_B *reg  = acb->pmuB;
				char *acb_dev_map = (char *)acb->device_map;
				uint32_t __iomem *signature = (uint32_t __iomem*)(&reg->msgcode_rwbuffer[0]);
				char __iomem *devicemap = (char __iomem*)(&reg->msgcode_rwbuffer[21]);
				int target, lun;
				struct scsi_device *psdev;
				char diff;

				atomic_inc(&acb->rq_map_token);
				if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
					for(target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
						diff = (*acb_dev_map)^readb(devicemap);
						if (diff != 0) {
							char temp;
							*acb_dev_map = readb(devicemap);
							temp =*acb_dev_map;
							for(lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
								if((temp & 0x01)==1 && (diff & 0x01) == 1) {	
										#if ARCMSR_DEBUG
											printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
									scsi_add_device(acb->host, 0, target, lun);
								}else if((temp & 0x01) == 0 && (diff & 0x01) == 1) {
									psdev = scsi_device_lookup(acb->host, 0, target, lun);
									if (psdev != NULL ) {
										#if ARCMSR_DEBUG
											printk("REMOVE A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
										scsi_remove_device(psdev);
										scsi_device_put(psdev);
									}
								}
								temp >>= 1;
								diff >>= 1;
							}
						}
						devicemap++;
						acb_dev_map++;
					}
				}
			}
		}
	}

	#elif (defined(ARCMSR_FW_POLLING) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static void arcmsr_message_isr_bh_fn(void *pacb) 
	{
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;
		#if ARCMSR_DEBUG
			printk("arcmsr_message_isr_bh_fn..................................\n");
		#endif

		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {

				struct MessageUnit_A __iomem *reg  = acb->pmuA;
				char *acb_dev_map = (char *)acb->device_map;
				uint32_t __iomem *signature = (uint32_t __iomem*) (&reg->msgcode_rwbuffer[0]);
				char __iomem *devicemap = (char __iomem*) (&reg->msgcode_rwbuffer[21]);
				int target, lun;
				struct scsi_device *psdev;
				char diff;

				atomic_inc(&acb->rq_map_token);
				if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
					for(target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
						diff = (*acb_dev_map)^readb(devicemap);
						if (diff != 0) {
							char temp;
							*acb_dev_map = readb(devicemap);
							temp =*acb_dev_map;
							for(lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
								if((temp & 0x01)==1 && (diff & 0x01) == 1) {	
										#if ARCMSR_DEBUG
											printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
									scsi_add_device(acb->host, 0, target, lun);
								}else if((temp & 0x01) == 0 && (diff & 0x01) == 1) {
									psdev = scsi_device_lookup(acb->host, 0, target, lun);
									if (psdev != NULL ) {
										#if ARCMSR_DEBUG
											printk("REMOVE A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
										scsi_remove_device(psdev);
										scsi_device_put(psdev);
									}
								}
								temp >>= 1;
								diff >>= 1;
							}
						}
						devicemap++;
						acb_dev_map++;
					}
				}
				break;
			}

			case ACB_ADAPTER_TYPE_B: {
				struct MessageUnit_B *reg  = acb->pmuB;
				char *acb_dev_map = (char *)acb->device_map;
				uint32_t __iomem *signature = (uint32_t __iomem*)(&reg->msgcode_rwbuffer[0]);
				char __iomem *devicemap = (char __iomem*)(&reg->msgcode_rwbuffer[21]);
				int target, lun;
				struct scsi_device *psdev;
				char diff;

				atomic_inc(&acb->rq_map_token);
				if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
					for(target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
						diff = (*acb_dev_map)^readb(devicemap);
						if (diff != 0) {
							char temp;
							*acb_dev_map = readb(devicemap);
							temp =*acb_dev_map;
							for(lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
								if((temp & 0x01)==1 && (diff & 0x01) == 1) {	
										#if ARCMSR_DEBUG
											printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
									scsi_add_device(acb->host, 0, target, lun);
								}else if((temp & 0x01) == 0 && (diff & 0x01) == 1) {
									psdev = scsi_device_lookup(acb->host, 0, target, lun);
									if (psdev != NULL ) {
										#if ARCMSR_DEBUG
											printk("REMOVE A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
										#endif
										scsi_remove_device(psdev);
										scsi_device_put(psdev);
									}
								}
								temp >>= 1;
								diff >>= 1;
							}
						}
						devicemap++;
						acb_dev_map++;
					}
				}
			}
		}
	}
	#endif
	/*
	*********************************************************************
	*********************************************************************
	*/
	int arcmsr_bios_param(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int *geom)
	{
		int ret,heads,sectors,cylinders, total_capacity;
		unsigned char *buffer;/* return copy of block device's partition table */

		#if ARCMSR_DEBUG
			printk("arcmsr_bios_param.................. \n");
		#endif

		buffer = scsi_bios_ptable(bdev);
		if(buffer) 
		{
			ret = scsi_partsize(buffer, capacity, &geom[2], &geom[0], &geom[1]);
			kfree(buffer);
			if (ret != -1)
			{
				return(ret);
			}
		}
		total_capacity = capacity;
		heads=64;
		sectors=32;
		cylinders = sector_div(capacity, heads * sectors);
		if(cylinders >= 1024)
		{
			heads=255;
			sectors=63;
				cylinders = sector_div(capacity, heads * sectors);
		}
		geom[0]=heads;
		geom[1]=sectors;
		geom[2]=cylinders;
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_probe(struct pci_dev *pdev,const struct pci_device_id *id)
	{
		struct Scsi_Host *host;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		uint8_t bus,dev_fun;		
		#if ARCMSR_DEBUG
			printk("arcmsr_probe............................\n");
		#endif
		
		if(pci_enable_device(pdev)) {
			printk("arcmsr%d: adapter probe: pci_enable_device error \n", arcmsr_adapterCnt);
			return -ENODEV;
		}

		if((host=scsi_host_alloc(&arcmsr_scsi_host_template,sizeof(struct AdapterControlBlock)))==0)	{
			printk("arcmsr%d: adapter probe: scsi_host_alloc error \n", arcmsr_adapterCnt);
	    		return -ENODEV;
		}

		if(!pci_set_dma_mask(pdev, DMA_64BIT_MASK)) {
			printk("ARECA RAID ADAPTER%d: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n", arcmsr_adapterCnt);
		}
		else if(!pci_set_dma_mask(pdev, DMA_32BIT_MASK)) {
			printk("ARECA RAID ADAPTER%d: 32BITS PCI BUS DMA ADDRESSING SUPPORTED\n", arcmsr_adapterCnt);
		} 
		else {
			printk("ARECA RAID ADAPTER%d: No suitable DMA available.\n", arcmsr_adapterCnt);
			return -ENOMEM;
		}

		if (pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK)) {
			printk("ARECA RAID ADAPTER%d: No 32BIT coherent DMA adressing available.\n", arcmsr_adapterCnt);
			return -ENOMEM;
		}

		bus = pdev->bus->number;
		dev_fun = pdev->devfn;
		acb = (struct AdapterControlBlock *) host->hostdata;
		memset(acb,0,sizeof(struct AdapterControlBlock));
		acb->pdev = pdev;
		acb->host = host;
		host->max_sectors = ARCMSR_MAX_XFER_SECTORS;
		host->max_lun = ARCMSR_MAX_TARGETLUN;
		host->max_id = ARCMSR_MAX_TARGETID;/*16:8*/
		host->max_cmd_len = 16;	 /*this is issue of 64bit LBA ,over 2T byte*/
		host->sg_tablesize = ARCMSR_MAX_SG_ENTRIES;
		host->can_queue = ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
		host->cmd_per_lun = ARCMSR_MAX_CMD_PERLUN;	    
		host->this_id=ARCMSR_SCSI_INITIATOR_ID;	
		host->unique_id = (bus << 8) | dev_fun;
		host->io_port = 0;
		host->n_io_port = 0;
		host->irq = pdev->irq;

		/**
 		* pci_set_master - enables bus-mastering for device dev
 		* @dev: the PCI device to enable
 		* Enables bus-mastering on the device and calls pcibios_set_master() to do the needed arch specific settings.
		*/
		pci_set_master(pdev);

		 if (pci_request_regions(pdev, "arcmsr")) {
			printk("arcmsr%d: pci_request_regions failed \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt = arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}

		arcmsr_define_adapter_type(acb);	

		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
		acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
		INIT_LIST_HEAD(&acb->ccb_free_list);

		if(arcmsr_alloc_ccb_pool(acb)) {
			printk("arcmsr%d: arcmsr_alloc_ccb_pool got error \n", arcmsr_adapterCnt);
			pHCBARC->adapterCnt = arcmsr_adapterCnt;
			pHCBARC->acb[arcmsr_adapterCnt] = NULL;
			scsi_remove_host(host);
			scsi_host_put(host);
			return -ENODEV;
		}

		arcmsr_iop_init(acb);

		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
				INIT_WORK (&acb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn);
			#else
				INIT_WORK (&acb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn, acb);
			#endif
		#endif

		if(request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb)) {
			printk("arcmsr%d: request_irq =%d failed!\n", arcmsr_adapterCnt--, pdev->irq);
			pHCBARC->adapterCnt = arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}

		if(strncmp(acb->firm_version,"V1.42",5) >= 0)
            		host->max_sectors = ARCMSR_MAX_XFER_SECTORS_B;

		if(scsi_add_host(host, &pdev->dev)) {
			printk("arcmsr%d: scsi_add_host got error \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt = arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}

	    	pHCBARC->adapterCnt = arcmsr_adapterCnt;
	   	pci_set_drvdata(pdev, host);
	    	scsi_scan_host(host);

		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			atomic_set(&acb->rq_map_token, 16);
			acb->fw_state = TRUE;
			init_timer(&acb->eternal_timer);
			acb->eternal_timer.expires = jiffies + msecs_to_jiffies(10*HZ);
			acb->eternal_timer.data = (unsigned long) acb;
			acb->eternal_timer.function = &arcmsr_request_device_map;
			add_timer(&acb->eternal_timer);
		#endif

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
			#ifdef CONFIG_SCSI_ARCMSR_AER
				pci_enable_pcie_error_reporting(pdev);
			#endif
	    	#endif
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_remove(struct pci_dev *pdev)
	{
	    	struct Scsi_Host *host = pci_get_drvdata(pdev);
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
	    	struct AdapterControlBlock *acb = (struct AdapterControlBlock *) host->hostdata;
		int i;

		#if ARCMSR_DEBUG
			printk("arcmsr_remove............................\n");
		#endif

		arcmsr_pcidev_disattach(acb);
		/*if this is last acb */
		for(i = 0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			if(pHCBARC->acb[i])
			{ 
				return;/* this is not last adapter's release */
			}
		}

		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
			unregister_reboot_notifier(&arcmsr_event_notifier);
		#endif
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_scsi_host_template_init(struct scsi_host_template *host_template)
	{
		int error;
		struct HCBARC *pHCBARC= &arcmsr_host_control_block;

		#if ARCMSR_DEBUG
			printk("arcmsr_scsi_host_template_init..............\n");
		#endif
		/* 
		** register as a PCI hot-plug driver module 
		*/
		memset(pHCBARC,0,sizeof(struct HCBARC));
		error = pci_register_driver(&arcmsr_pci_driver);
		if(pHCBARC->acb[0])
		{
			host_template->proc_name="arcmsr";
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
			register_reboot_notifier(&arcmsr_event_notifier);
		#endif
		}
		return error;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
		static void arcmsr_shutdown(struct pci_dev *pdev)
		{
			struct Scsi_Host *host = pci_get_drvdata(pdev);
			struct AdapterControlBlock *acb= (struct AdapterControlBlock *)host->hostdata;
			
			#if ARCMSR_DEBUG
				printk("arcmsr_shutdown............................\n");
			#endif
			arcmsr_disable_outbound_ints(acb);

			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				del_timer_sync(&acb->eternal_timer);
				flush_scheduled_work();
			#endif

			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);
		}
    	#endif
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_module_init(void)
	{
		return (arcmsr_scsi_host_template_init(&arcmsr_scsi_host_template));
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_module_exit(void)
	{
		pci_unregister_driver(&arcmsr_pci_driver);
	}
	/*
	************************************************************************
	************************************************************************
	*/
	module_init(arcmsr_module_init);
	module_exit(arcmsr_module_exit);
#else
	/*
	*************************************************************************
	*************************************************************************
	*/
	static void arcmsr_internal_done(struct scsi_cmnd *pcmd)
	{
		pcmd->SCp.Status++;
	}
	/*
	***************************************************************
	*	arcmsr_schedule_command
	*	Description:	Process a command from the SCSI manager(A.P)
	*	Parameters:	cmd - Pointer to SCSI command structure.
	*	Returns:		Status code.
	***************************************************************
	*/
	int arcmsr_schedule_command(struct scsi_cmnd *pcmd)
	{
		unsigned long timeout;
		#if ARCMSR_DEBUG
			printk(" arcmsr_schedule_command................ \n");
		#endif
		pcmd->SCp.Status=0;
		arcmsr_queue_command(pcmd,arcmsr_internal_done);
		timeout=jiffies + 60 * HZ; 
		while(time_before(jiffies,timeout) && !pcmd->SCp.Status) 
		{
			schedule();
		}
		if(!pcmd->SCp.Status) 
		{
			pcmd->result=(DID_ERROR<<16);
		}
		return pcmd->result;
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
	void arcmsr_do_interrupt(int irq,void *dev_id,struct pt_regs *regs)
	{
		struct HCBARC *pHCBARC= &arcmsr_host_control_block;
		struct AdapterControlBlock *acb;
		struct AdapterControlBlock *acbtmp;
		int i=0;

		#if ARCMSR_DEBUG
			printk("arcmsr_do_interrupt.................. \n");
		#endif
		
		acb=(struct AdapterControlBlock *)dev_id;
		acbtmp=pHCBARC->acb[i];
		while((acb != acbtmp) && acbtmp && (i <ARCMSR_MAX_ADAPTER) ) {
			i++;
			acbtmp=pHCBARC->acb[i];
		}
		if(!acbtmp) {
			#if ARCMSR_DEBUG
				printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
			#endif
			return;
		}
		spin_lock_irq(&acb->isr_lock);
		arcmsr_interrupt(acb);
		spin_unlock_irq(&acb->isr_lock);
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
	int arcmsr_bios_param(Disk *disk,kdev_t dev,int geom[])
	{
		int heads,sectors,cylinders,total_capacity;

		#if ARCMSR_DEBUG
			printk("arcmsr_bios_param.................. \n");
		#endif
		
		total_capacity=disk->capacity;
		heads=64;
		sectors=32;
		cylinders=total_capacity / (heads * sectors);
		if(cylinders >= 1024)
		{
			heads=255;
			sectors=63;
			cylinders=total_capacity / (heads * sectors);
		}
		geom[0]=heads;
		geom[1]=sectors;
		geom[2]=cylinders;
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	int arcmsr_detect(Scsi_Host_Template * host_template)
	{
		struct
		{
			unsigned int   vendor_id;
			unsigned int   device_id;
		} const	arcmsr_devices[] = {
			{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1110 }
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1120}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1130}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1160}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1170}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1200}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1201}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1202}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1210}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1220}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1230}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1260}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1270}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1280}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1380}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1381}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1680}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1681}};
		
		struct pci_dev *pdev=NULL;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC= &arcmsr_host_control_block;
		struct Scsi_Host *host;
		static u_int8_t	i;
		
		#if ARCMSR_DEBUG
			printk("arcmsr_detect............................\n");
		#endif
		
		memset(pHCBARC,0,sizeof(struct HCBARC));
		for(i=0; i < (sizeof(arcmsr_devices)/sizeof(arcmsr_devices[0])); ++i) {
			pdev=NULL;
			while((pdev=pci_find_device(arcmsr_devices[i].vendor_id,arcmsr_devices[i].device_id,pdev))) {
				if((host=scsi_register(host_template,sizeof(struct AdapterControlBlock)))==0) {
					printk("arcmsr_detect: scsi_register error . . . . . . . . . . .\n");
					continue;
				}
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					if(pci_enable_device(pdev)) {
						printk("arcmsr_detect: pci_enable_device ERROR..................................\n");
						scsi_unregister(host);
						continue;
					}
					if(!pci_set_dma_mask(pdev,(dma_addr_t)0xffffffffffffffffULL)) {
						/*64bit*/
						printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
					}
					else if(pci_set_dma_mask(pdev,(dma_addr_t)0x00000000ffffffffULL)) {
						/*32bit*/
						printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED (ERROR)\n");
						scsi_unregister(host);
						continue;
					}
				#endif
				acb=(struct AdapterControlBlock	*) host->hostdata;
				memset(acb,0,sizeof(struct AdapterControlBlock));
				spin_lock_init(&acb->isr_lock);
				acb->pdev=pdev;
				acb->host=host;
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,7)
					host->max_sectors=ARCMSR_MAX_XFER_SECTORS;
					if(strncmp(acb->firm_version,"V1.42",5) >= 0)
                                		host->max_sectors=ARCMSR_MAX_XFER_SECTORS_B;
				#endif			
				host->max_lun=ARCMSR_MAX_TARGETLUN;
				host->max_id=ARCMSR_MAX_TARGETID;/*16:8*/

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
				host->max_cmd_len=16;		 /*this is issue of 64bit LBA ,over 2T byte*/
	    			#endif

				host->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
				host->can_queue=ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
				host->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;	    
				host->this_id=ARCMSR_SCSI_INITIATOR_ID;	
				host->io_port=0;
				host->n_io_port=0;
				host->irq=pdev->irq;
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,4)
					scsi_set_pci_device(host,pdev);
				#endif
				arcmsr_define_adapter_type(acb);
				acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
				acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
				INIT_LIST_HEAD(&acb->ccb_free_list);
				if(!arcmsr_alloc_ccb_pool(acb)) {
					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
						pci_set_drvdata(pdev,acb); /*set driver_data*/
					#endif
					pci_set_master(pdev);
					if(request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb)) {
						printk("arcmsr_detect:	request_irq got ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->acb[acb->adapter_index]=NULL;
						arcmsr_free_ccb_pool(acb);
					    	scsi_unregister(host);
						goto next_areca;
					}
					
					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
						if (pci_request_regions(pdev, "arcmsr")) {
							printk("arcmsr_detect:	pci_request_regions got	ERROR...................\n");
							arcmsr_adapterCnt--;
							pHCBARC->acb[acb->adapter_index]=NULL;
							arcmsr_free_ccb_pool(acb);
					    		scsi_unregister(host);
							goto next_areca;
						}
		            		#endif
					arcmsr_iop_init(acb);/*	on kernel 2.4.21 driver's iop read/write must after request_irq	*/
				}else {
					printk("arcmsr: arcmsr_alloc_ccb_pool got ERROR...................\n");
					scsi_unregister(host);
				}
		next_areca: ;
			}
		}
		if(arcmsr_adapterCnt) {
			#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
				host_template->proc_name="arcmsr";
			#else		  
				host_template->proc_dir= &arcmsr_proc_scsi;
			#endif
			register_reboot_notifier(&arcmsr_event_notifier);
		} else {
			printk("arcmsr_detect:...............NO	ARECA RAID ADAPTER FOUND...........\n");
			return(arcmsr_adapterCnt);
		}
		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		return(arcmsr_adapterCnt);
	}

#endif 
/*
**********************************************************************
**********************************************************************
*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb) {
		struct scsi_cmnd *pcmd = ccb->pcmd;
		scsi_dma_unmap(pcmd);
	 }
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb) {
		struct AdapterControlBlock *acb=ccb->acb;
		struct scsi_cmnd *pcmd=ccb->pcmd;

		#if ARCMSR_DEBUG
			printk("arcmsr_pci_unmap_dma............................\n");
		#endif
		
		if(pcmd->use_sg != 0) {
			struct scatterlist *sl;

			sl = (struct scatterlist *)pcmd->request_buffer;
			pci_unmap_sg(acb->pdev,	sl, pcmd->use_sg, pcmd->sc_data_direction);
		} else if(pcmd->request_bufflen != 0){
			pci_unmap_single(acb->pdev,(dma_addr_t)(unsigned long)pcmd->SCp.ptr,pcmd->request_bufflen, pcmd->sc_data_direction);
		}
	}
#endif
 	/*
	**********************************************************************************
	**********************************************************************************
	*/
	#if 0
	static void arcmsr_modify_timeout(struct scsi_cmnd *cmd)
	{
		/* 
		**********************************************************************
		** int mod_timer(struct timer_list *timer, unsigned long expires)
		** The function returns whether it has modified a pending timer or not.
		** (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
		** active timer returns 1.)
		** mod_timer(timer, expires) is equivalent to:
		** del_timer(timer); timer->expires = expires; add_timer(timer);
		**********************************************************************
		*/
		cmd->timeout_per_command = ARCMSR_SD_TIMEOUT*HZ;
		mod_timer(&cmd->eh_timeout, jiffies + ARCMSR_SD_TIMEOUT*HZ);
	}
	#endif
	/*
	************************************************************************
	*Dapendent on the PCI_DEVICE_ID, to define its adapter type
	************************************************************************
	*/
	static void arcmsr_define_adapter_type(struct AdapterControlBlock *acb)
	{
		struct pci_dev *pdev = acb->pdev;
		u16 dev_id;
		
		#if ARCMSR_DEBUG
			printk("arcmsr_define_adapter_type....................................... \n");
		#endif
		
		pci_read_config_word(pdev, PCI_DEVICE_ID, &dev_id);
		switch(dev_id) {
		case 0x1201 : {
			acb->adapter_type = ACB_ADAPTER_TYPE_B;
			}
			break;

		default : acb->adapter_type = ACB_ADAPTER_TYPE_A;
		}
	}
/*
**********************************************************************************
**this is the replacement of arcmsr_disable_allintr() in the former version
**********************************************************************************
*/
static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb)
{
	
	u32 orig_mask = 0;

	#if ARCMSR_DEBUG
		printk("arcmsr_disable_outbound_ints....................................... \n");
	#endif
	
	switch(acb->adapter_type) {
		
	case ACB_ADAPTER_TYPE_A : {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		orig_mask = readl(&reg->outbound_intmask);
		writel(orig_mask|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE, &reg->outbound_intmask);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B : {
		struct MessageUnit_B *reg = acb->pmuB;
		orig_mask = readl(reg->iop2drv_doorbell_mask);
		writel(0, reg->iop2drv_doorbell_mask);
		}
	break;
	}
	return orig_mask;	
}
/*
**********************************************************************************
**this is the replacement of arcmsr_enable_allintr() in the former version
**********************************************************************************
*/
static void arcmsr_enable_outbound_ints(struct AdapterControlBlock *acb, u32 orig_mask)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_enable_outbound_ints...................................\n");
	#endif
	
	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A : {
		uint32_t mask;
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		mask = orig_mask & ~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
		writel(mask, &reg->outbound_intmask);
		acb->outbound_int_enable = ~(orig_mask & mask) & 0x000000ff;
		}
		break;
		
	case ACB_ADAPTER_TYPE_B : {
		uint32_t mask;
		struct MessageUnit_B *reg = acb->pmuB;
		mask = orig_mask | (ARCMSR_IOP2DRV_DATA_WRITE_OK|ARCMSR_IOP2DRV_DATA_READ_OK|ARCMSR_IOP2DRV_CDB_DONE|ARCMSR_IOP2DRV_MESSAGE_CMD_DONE);
		writel(mask, reg->iop2drv_doorbell_mask);
		acb->outbound_int_enable = (orig_mask | mask) & 0x0000000f;
		}
	}
}

/*
**********************************************************************
**********************************************************************
*/
static uint8_t arcmsr_hba_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	uint32_t Index;
	uint8_t Retries = 0x00;
	
	do {
		for (Index = 0; Index < 100; Index++) {
			if (readl(&reg->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) {
				writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);
				return 0x00;
			}
			arc_mdelay_int(10);
		}/*max 1 seconds*/
		
	} while (Retries++ < 20);/*max 20 sec*/
	return 0xff;
}
/*
**********************************************************************
**********************************************************************
*/
static uint8_t arcmsr_hbb_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	uint32_t Index;
	uint8_t Retries = 0x00;

	do {
		for (Index = 0; Index < 100; Index++) {
			if (readl(reg->iop2drv_doorbell)
				& ARCMSR_IOP2DRV_MESSAGE_CMD_DONE) {
				writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);
				writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
				return 0x00;
			}
			arc_mdelay_int(10);
		}/*max 1 seconds*/
		
	} while (Retries++ < 20);/*max 20 sec*/
	return 0xff;
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_hba_cache(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	int retry_count = 10;
	
	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, &reg->inbound_msgaddr0);
	do {
		if(!arcmsr_hba_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down=%d \n", acb->host->host_no, retry_count);
		}
	} while(retry_count != 0);
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_hbb_cache(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	int retry_count = 10;

	writel(ARCMSR_MESSAGE_FLUSH_CACHE, reg->drv2iop_doorbell);

	do {
		if(!arcmsr_hbb_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down=%d \n", acb->host->host_no, retry_count);
		}
	} while(retry_count != 0);
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb)
{
    	#if ARCMSR_DEBUG
    		printk("arcmsr_flush_adapter_cache..............\n");
    	#endif
		
	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		arcmsr_flush_hba_cache(acb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		arcmsr_flush_hbb_cache(acb);
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_ccb_complete(struct CommandControlBlock *ccb, int stand_flag)
{
	struct AdapterControlBlock *acb=ccb->acb;
	struct scsi_cmnd *pcmd=ccb->pcmd;

	#if ARCMSR_DEBUG
		printk("arcmsr_ccb_complete......................\n");
	#endif
	
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
    		arcmsr_pci_unmap_dma(ccb);
	#endif
	if(stand_flag==1)
	{
	    atomic_dec(&acb->ccboutstandingcount);
	}
	ccb->startdone=ARCMSR_CCB_DONE;
	ccb->ccb_flags=0;
	list_add_tail(&ccb->list, &acb->ccb_free_list);

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		pcmd->scsi_done(pcmd);
	#else
		unsigned long flags;
		spin_lock_irqsave(&io_request_lock, flags);
		pcmd->scsi_done(pcmd);
		spin_unlock_irqrestore(&io_request_lock, flags);
	#endif
}
/*
**********************************************************************
**	 if scsi error do auto request sense
**********************************************************************
*/
void arcmsr_report_sense_info(struct CommandControlBlock *ccb)
{
	struct scsi_cmnd *pcmd=ccb->pcmd;

	#if ARCMSR_DEBUG
    		printk("arcmsr_report_sense_info...........\n");
	#endif

	if(pcmd->sense_buffer) {
		memset(pcmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
		memcpy(pcmd->sense_buffer,ccb->arcmsr_cdb.SenseData,SCSI_SENSE_BUFFERSIZE);
	    	pcmd->sense_buffer[0] = (0x1 << 7 | 0x70); /* Valid,ErrorCode */
	}
}
/*
*********************************************************************
*********************************************************************
*/
static uint8_t arcmsr_abort_hba_allcmd(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, &reg->inbound_msgaddr0);
	if (arcmsr_hba_wait_msgint_ready(acb)){
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, acb->host->host_no);
		return 0xff;
	}
	return 0x00;
}
/*
*********************************************************************
*********************************************************************
*/
static uint8_t arcmsr_abort_hbb_allcmd(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;

	writel(ARCMSR_MESSAGE_ABORT_CMD, reg->drv2iop_doorbell);
	if (arcmsr_hbb_wait_msgint_ready(acb)){
		printk(KERN_NOTICE "arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, acb->host->host_no);
		return 0xff;
	}
	return 0x00;
}
/*
*********************************************************************
** If return value of arcmsr_abort_allcmd() is equal to zero, it means all commands  
** are aborted SUCCESSFULLY.
*********************************************************************
*/
static uint8_t arcmsr_abort_allcmd(struct AdapterControlBlock *acb)
{
	uint8_t rtnval = 0;
	#if ARCMSR_DEBUG
		printk("arcmsr_abort_allcmd........................... \n");
	#endif	
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		rtnval = arcmsr_abort_hba_allcmd(acb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		rtnval = arcmsr_abort_hbb_allcmd(acb);
		}
	}
	return rtnval;
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_build_ccb(struct AdapterControlBlock *acb,struct CommandControlBlock *ccb,struct scsi_cmnd *pcmd)
{
	struct ARCMSR_CDB *arcmsr_cdb = &ccb->arcmsr_cdb;
	uint8_t *psge = (uint8_t * )&arcmsr_cdb->u;
	__le32 address_lo,address_hi;
	int arccdbsize=0x30, sgcount = 0;
	unsigned short use_sg;
	unsigned request_bufflen;
	#if ARCMSR_DEBUG
		printk("arcmsr_build_ccb........................... \n");
	#endif
	
    	ccb->pcmd = pcmd;
	memset(arcmsr_cdb,0,sizeof(struct ARCMSR_CDB));
    	arcmsr_cdb->Bus = 0;
   	arcmsr_cdb->TargetID = pcmd->device->id;
    	arcmsr_cdb->LUN = pcmd->device->lun;
    	arcmsr_cdb->Function = 1;
	arcmsr_cdb->CdbLength = (uint8_t)pcmd->cmd_len;
    	arcmsr_cdb->Context = (unsigned long)arcmsr_cdb;
    	memcpy(arcmsr_cdb->Cdb, pcmd->cmnd, pcmd->cmd_len);

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,6,25)
		use_sg = scsi_sg_count(pcmd);
		request_bufflen = scsi_bufflen(pcmd);
	#else
		use_sg = pcmd->use_sg;
		request_bufflen = pcmd->request_bufflen;
	#endif

	if(use_sg) {
		__le32 length;
		int i,cdb_sgcount = 0;
		struct scatterlist *sl;

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,23)
			sl = scsi_sglist(pcmd);
			sgcount = scsi_dma_map(pcmd);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
			sl = (struct scatterlist *) pcmd->request_buffer;
			sgcount=pci_map_sg(acb->pdev, sl, pcmd->use_sg, pcmd->sc_data_direction);
    		#else
			sl=(struct scatterlist *) pcmd->request_buffer;
			sgcount = pcmd->use_sg;
    		#endif

		if(sgcount > ARCMSR_MAX_SG_ENTRIES) {
			return FAILED;
		}
		/* map stor port SG list to our iop SG List.*/

		if (sgcount) {
			for(i = 0; i < sgcount; i++) {
				/* Get the physical address of the current data	pointer	*/
				#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
					length=cpu_to_le32(sg_dma_len(sl));
	    				address_lo=cpu_to_le32(dma_addr_lo32(sg_dma_address(sl)));
					address_hi=cpu_to_le32(dma_addr_hi32(sg_dma_address(sl)));
				#else
	    				length=cpu_to_le32(sl->length);
					address_lo=cpu_to_le32(virt_to_bus(sl->address));
	    				address_hi=0;
				#endif

				if(address_hi==0) {
					struct SG32ENTRY* pdma_sg=(struct SG32ENTRY*)psge;

					pdma_sg->address=address_lo;
					pdma_sg->length=length;
					psge +=	sizeof(struct SG32ENTRY);
					arccdbsize += sizeof(struct SG32ENTRY);
				} else {
					struct SG64ENTRY *pdma_sg=(struct SG64ENTRY *)psge;

					pdma_sg->addresshigh=address_hi;
					pdma_sg->address=address_lo;
					pdma_sg->length=length|cpu_to_le32(IS_SG64_ADDR);
					psge +=sizeof(struct SG64ENTRY);
					arccdbsize +=sizeof(struct SG64ENTRY);
				}
				sl++;
				cdb_sgcount++;
			}
			arcmsr_cdb->sgcount = (uint8_t)cdb_sgcount;
			arcmsr_cdb->DataLength = request_bufflen;
			if( arccdbsize > 256) 
				arcmsr_cdb->Flags|= ARCMSR_CDB_FLAG_SGL_BSIZE;
		}
	}else if(request_bufflen) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
			dma_addr_t dma_addr;
			dma_addr = pci_map_single(acb->pdev, scsi_sglist(pcmd),scsi_bufflen(pcmd), pcmd->sc_data_direction);
			/* We hide it here for later unmap. */
			pcmd->SCp.ptr =	(char *)(unsigned long)dma_addr;
			address_lo = cpu_to_le32(dma_addr_lo32(dma_addr));
	    		address_hi = cpu_to_le32(dma_addr_hi32(dma_addr));
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
			dma_addr_t dma_addr;
			dma_addr = pci_map_single(acb->pdev, pcmd->request_buffer, pcmd->request_bufflen, pcmd->sc_data_direction);
			/* We hide it here for later unmap. */
			pcmd->SCp.ptr =	(char *)(unsigned long)dma_addr;
			address_lo = cpu_to_le32(dma_addr_lo32(dma_addr));
	    		address_hi = cpu_to_le32(dma_addr_hi32(dma_addr));
    		#else
 			address_lo = cpu_to_le32(virt_to_bus(pcmd->request_buffer));/* Actual requested buffer */
 	    		address_hi = 0;
    		#endif
		
		if(address_hi==0) {
			struct SG32ENTRY* pdma_sg = (struct SG32ENTRY*)psge;
			pdma_sg->address = address_lo;
	    		pdma_sg->length = request_bufflen;
		} else{
			struct SG64ENTRY* pdma_sg=(struct SG64ENTRY*)psge;
			pdma_sg->addresshigh = address_hi;
			pdma_sg->address = address_lo;
	    		pdma_sg->length = request_bufflen|cpu_to_le32(IS_SG64_ADDR);
		}
		arcmsr_cdb->sgcount=1;
		arcmsr_cdb->DataLength = request_bufflen;
	}
	if (pcmd->cmnd[0]|WRITE_6 || pcmd->cmnd[0]|WRITE_10 || pcmd->cmnd[0]|WRITE_12 )	 {
		arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_WRITE;
		ccb->ccb_flags |= CCB_FLAG_WRITE;
	}

    	return SUCCESS;
}
/*
**************************************************************************
**	arcmsr_post_ccb: Send a protocol specific ARC send postcard to a AIOC .
**	handle: Handle of registered ARC protocol driver
**	adapter_id: AIOC unique identifier(integer)
**	pPOSTCARD_SEND: Pointer to ARC send postcard
**
**	This routine posts a ARC send postcard to the request post FIFO of a
**	specific ARC adapter.
**************************************************************************
*/ 
static void arcmsr_post_ccb(struct AdapterControlBlock *acb,struct CommandControlBlock *ccb)
{
	uint32_t cdb_shifted_phyaddr = ccb->cdb_shifted_phyaddr;
	struct ARCMSR_CDB *arcmsr_cdb = (struct ARCMSR_CDB *)&ccb->arcmsr_cdb;
	atomic_inc(&acb->ccboutstandingcount);
	ccb->startdone = ARCMSR_CCB_START;

	#if ARCMSR_DEBUG
		printk("arcmsr_post_ccb...................................\n");
	#endif

	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A *reg = acb->pmuA;

		if (arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE)
			writel(cdb_shifted_phyaddr | ARCMSR_CCBPOST_FLAG_SGL_BSIZE, &reg->inbound_queueport);
		else {
				writel(cdb_shifted_phyaddr, &reg->inbound_queueport);
		}
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		int32_t ending_index, index = reg->postq_index;

		ending_index = ((index+1) % ARCMSR_MAX_HBB_POSTQUEUE);
		reg->post_qbuffer[ending_index] = 0;
		if(arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE) {
			reg->post_qbuffer[index] = (cdb_shifted_phyaddr|ARCMSR_CCBPOST_FLAG_SGL_BSIZE);
		}
		else {
			reg->post_qbuffer[index] = cdb_shifted_phyaddr;
		}
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;     /*if last index number set it to 0 */
		reg->postq_index = index;
		writel(ARCMSR_DRV2IOP_CDB_POSTED, reg->drv2iop_doorbell);
		}
		break;
	}
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_iop_message_read(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_iop_message_read..............\n");
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		writel(ARCMSR_DRV2IOP_DATA_READ_OK, reg->drv2iop_doorbell);
		}
		break;
	}
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_iop_message_wrote(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_iop_message_wrote..............\n");
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		/*
		** push inbound doorbell tell iop, driver data write ok 
		** and wait reply on next hwinterrupt for next Qbuffer post
		*/
		writel(ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK, &reg->inbound_doorbell);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		/*
		** push inbound doorbell tell iop, driver data write ok 
		** and wait reply on next hwinterrupt for next Qbuffer post
		*/
		writel(ARCMSR_DRV2IOP_DATA_WRITE_OK, reg->drv2iop_doorbell);
		}
		break;
	}
}
/*
**********************************************************************
** get the Qbuffer for message 
**********************************************************************
*/
static struct QBUFFER __iomem  *arcmsr_get_iop_rqbuffer(struct AdapterControlBlock *acb)
{
	struct QBUFFER __iomem *qbuffer = NULL;

	#if ARCMSR_DEBUG
		printk("arcmsr_get_iop_rqbuffer..............\n");
	#endif

	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		qbuffer = (struct QBUFFER __iomem *) &reg->message_rbuffer;
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		qbuffer = (struct QBUFFER __iomem *) reg->message_rbuffer;
		}
		break;
	}
	return qbuffer;
}
/*
**********************************************************************
**
**********************************************************************
*/
static struct QBUFFER __iomem  *arcmsr_get_iop_wqbuffer(struct AdapterControlBlock *acb)
{
	struct QBUFFER __iomem *pqbuffer = NULL;

	#if ARCMSR_DEBUG
		printk("arcmsr_get_iop_wqbuffer..............\n");
	#endif

	switch (acb->adapter_type) {
		
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		pqbuffer = (struct QBUFFER __iomem *)&reg->message_wbuffer;
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B  *reg = acb->pmuB;
		pqbuffer = (struct QBUFFER __iomem *)reg->message_wbuffer;
		}
		break;
	}
	return pqbuffer;
}
/*
**********************************************************************
** the replacement for arcmsr_post_Qbuffer in former version
**********************************************************************
*/
static void arcmsr_post_ioctldata2iop(struct AdapterControlBlock *acb)
{
	int32_t wqbuf_firstindex, wqbuf_lastindex;
	uint8_t *pQbuffer;
	struct QBUFFER __iomem *pwbuffer;
	uint8_t __iomem *iop_data;
	int32_t allxfer_len = 0;

	#if ARCMSR_DEBUG
		printk("arcmsr_post_ioctldata2iop..............\n");
	#endif

	pwbuffer = arcmsr_get_iop_wqbuffer(acb);
	iop_data = (uint8_t __iomem *)pwbuffer->data;
    	if(acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_READED) {
		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		wqbuf_firstindex = acb->wqbuf_firstindex;
		wqbuf_lastindex = acb->wqbuf_lastindex;
		while((wqbuf_firstindex != wqbuf_lastindex) && (allxfer_len<124)) {
			pQbuffer = &acb->wqbuffer[wqbuf_firstindex];
			memcpy(iop_data, pQbuffer, 1);
			wqbuf_firstindex++;
			wqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
			iop_data++;
			allxfer_len++;
		}
		acb->wqbuf_firstindex = wqbuf_firstindex;
		pwbuffer->data_len = allxfer_len;
		arcmsr_iop_message_wrote(acb);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_hba_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, &reg->inbound_msgaddr0);

	if(arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter background rebulid' timeout \n", acb->host->host_no);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_hbb_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_MESSAGE_STOP_BGRB, reg->drv2iop_doorbell);

	if(arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter background rebulid' timeout \n", acb->host->host_no);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock *acb)
{
	 #if	ARCMSR_DEBUG
		printk("arcmsr_stop_adapter_bgrb..............\n");
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		arcmsr_stop_hba_bgrb(acb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		arcmsr_stop_hbb_bgrb(acb);
		}
		break;
	}	
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb)
{
	 #if	ARCMSR_DEBUG
		printk("arcmsr_free_ccb_pool..............\n");
	#endif

	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		iounmap(acb->pmuA);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0) 
			dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM)+0x20), acb->dma_coherent, acb->dma_coherent_handle);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
			pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM)+0x20), acb->dma_coherent, acb->dma_coherent_handle);
		#else
			kfree(acb->dma_coherent);
		#endif
		}
		break;

	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		iounmap((u8 *)reg->drv2iop_doorbell - ARCMSR_DRV2IOP_DOORBELL);
		iounmap((u8 *)reg->message_wbuffer - ARCMSR_MESSAGE_WBUFFER);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0) 
			dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
			pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#else
			kfree(acb->dma_coherent);
		#endif
		}
		}

}
/*
**********************************************************************
** Function:	arcmsr_interrupt
** Output:	void
** The Byte 2 of the 32-bit 'result' variable in struct scsi_cmnd, which is defined in Linux/include/scsi/scsi_cmnd.h, is defined in Linux/include/scsi/scsi.h: 
** DID_OK	   0x00			// NO error				   
** DID_NO_CONNECT  0x01		// Couldn't connect before timeout period  
** DID_BUS_BUSY	   0x02		// BUS stayed busy through time	out period 
** DID_TIME_OUT	   0x03		// TIMED OUT for other reason		   
** DID_BAD_TARGET  0x04		// BAD target.				   
** DID_ABORT	   0x05		// Told	to abort for some other	reason	   
** DID_PARITY	   0x06		// Parity error				   
** DID_ERROR	   0x07		// Internal error			   
** DID_RESET	   0x08		// Reset by somebody.			   
** DID_BAD_INTR	   0x09		// Got an interrupt we weren't expecting.  
** DID_PASSTHROUGH 0x0a	// Force command past mid-layer		   
** DID_SOFT_ERROR  0x0b		// The low level driver	just wish a retry  
** DRIVER_OK	   0x00		// Driver status	
**********************************************************************
*/
/*
**********************************************************************
**handle the interrupt from IOP that there is data in IOP waiting to be written to the driver
**********************************************************************
*/
static void arcmsr_iop2drv_data_wrote_handle(struct AdapterControlBlock *acb)
{
	struct QBUFFER __iomem  *prbuffer;
	struct QBUFFER *pQbuffer;
	uint8_t __iomem *iop_data;
	int32_t my_empty_len, iop_len, rqbuf_firstindex, rqbuf_lastindex;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_iop2drv_data_wrote_handle...................................\n");
	#endif

   	rqbuf_lastindex = acb->rqbuf_lastindex;
	rqbuf_firstindex = acb->rqbuf_firstindex;
	prbuffer = arcmsr_get_iop_rqbuffer(acb);		/*get the Qbuffer on IOP for data read by the driver*/
	iop_data = (uint8_t __iomem *)prbuffer->data;
	iop_len = prbuffer->data_len;
    	my_empty_len = (rqbuf_firstindex - rqbuf_lastindex -1)&(ARCMSR_MAX_QBUFFER -1);

	if(my_empty_len >= iop_len) {
		while(iop_len > 0) {
			pQbuffer = (struct QBUFFER *)&acb->rqbuffer[rqbuf_lastindex];
			memcpy(pQbuffer,iop_data,1);	/*from IOP tp driver buffer*/
			rqbuf_lastindex++;
			rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
			iop_data++;
			iop_len--;
		}
        		acb->rqbuf_lastindex = rqbuf_lastindex;
		arcmsr_iop_message_read(acb);		/*notice IOP the message is read*/
	}else {
		acb->acb_flags |= ACB_F_IOPDATA_OVERFLOW;
	}
}
/*
*******************************************************************************
**process the interrupt from IOP after IOP notices driver that IOP is ready to receive the data
*******************************************************************************
*/
static void arcmsr_iop2drv_data_read_handle(struct AdapterControlBlock *acb)
{
	acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_READED;
	if(acb->wqbuf_firstindex!= acb->wqbuf_lastindex) {
		uint8_t * pQbuffer;
		struct QBUFFER __iomem *pwbuffer;
		uint8_t __iomem *iop_data;
		int32_t allxfer_len = 0;

		#if ARCMSR_DEBUG
			printk("arcmsr_iop2drv_data_read_handle...................................\n");
		#endif

		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		pwbuffer = arcmsr_get_iop_wqbuffer(acb);    /*get the Qbuffer on IOP for data which be written to by the driver, i.e. read by IOP*/
		iop_data = (uint8_t __iomem *)pwbuffer->data;

		while((acb->wqbuf_firstindex != acb->wqbuf_lastindex) && (allxfer_len<124)) {
			pQbuffer = &acb->wqbuffer[acb->wqbuf_firstindex];
   			memcpy(iop_data, pQbuffer,1);
			acb->wqbuf_firstindex++;
			acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; 
			iop_data++;
			allxfer_len++;
		}
		pwbuffer->data_len = allxfer_len;

		arcmsr_iop_message_wrote(acb);
 	}

	if(acb->wqbuf_firstindex == acb->wqbuf_lastindex) {
		acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_CLEARED;
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_report_ccb_state(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb, uint32_t flag_ccb)
{
	uint8_t id, lun;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_report_ccb_state...................................\n");
	#endif

	id = ccb->pcmd->device->id;
	lun = ccb->pcmd->device->lun;
	/* FW replies ccb w/o errors */
	if (!(flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)) {
		if (acb->devstate[id][lun] == ARECA_RAID_GONE) {
			/* FW will automatically recover the taget, 
			hence the driver directly return to the initial value */
			acb->devstate[id][lun] = ARECA_RAID_GOOD;
			acb->dev_aborts[id][lun]=0;
		}
		ccb->pcmd->result = DID_OK << 16;
		arcmsr_ccb_complete(ccb, 1);
	} else {
		/* FW replies ccb w/ errors */
		switch(ccb->arcmsr_cdb.DeviceStatus) {
		case ARCMSR_DEV_SELECT_TIMEOUT: {
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_NO_CONNECT << 16;
			arcmsr_ccb_complete(ccb, 1);
			}
			break;

		case ARCMSR_DEV_ABORTED: 

		case ARCMSR_DEV_INIT_FAIL: {
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_BAD_TARGET << 16;
			arcmsr_ccb_complete(ccb, 1);
			}
			break;

		case ARCMSR_DEV_CHECK_CONDITION: {
			acb->devstate[id][lun] = ARECA_RAID_GOOD;
			ccb->pcmd->result = (DID_OK << 16 |CHECK_CONDITION << 1);
			arcmsr_report_sense_info(ccb);
			arcmsr_ccb_complete(ccb, 1);
			}
			break;

		default:
			printk(KERN_EMERG "arcmsr%d: scsi id = %d lun = %d get command error done, but got unknown DeviceStatus = 0x%x \n"
				, acb->host->host_no
				, id
				, lun
				, ccb->arcmsr_cdb.DeviceStatus);
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_BAD_TARGET << 16;
			arcmsr_ccb_complete(ccb, 1);
			break;
		}
	}	
}
/*
*******************************************************************************
* Finish the flag_ccb in the done_queue which is the buffer queues the answered ccb from adapter processor 
*******************************************************************************
*/
static void arcmsr_drain_donequeue(struct AdapterControlBlock *acb, uint32_t flag_ccb)
{
	struct CommandControlBlock *ccb;
	int id, lun;

	#if ARCMSR_DEBUG
		printk("arcmsr_drain_donequeue...................................\n");
	#endif

	ccb = (struct CommandControlBlock *)(acb->vir2phy_offset + (flag_ccb << 5));
	/*In the normal state, the ccb startdone should be ARCMSR_CCB_START*/
	if ((ccb->acb != acb) || (ccb->startdone != ARCMSR_CCB_START)) {
		if (ccb->startdone == ARCMSR_CCB_ABORTED) {
			struct scsi_cmnd *abortcmd = ccb->pcmd;
			if (abortcmd) {
				id = abortcmd->device->id;
				lun = abortcmd->device->lun;
				if(acb->dev_aborts[id][lun] >= 4) {
					acb->devstate[id][lun] = ARECA_RAID_GONE;
					abortcmd->result = DID_NO_CONNECT << 16;
				}
				abortcmd->result |= DID_ABORT << 16;
				arcmsr_ccb_complete(ccb,1);
				printk(KERN_EMERG "arcmsr%d: ccb ='0x%p' got aborted command \n", acb->host->host_no, ccb);
			}
		}
		printk(KERN_EMERG "arcmsr%d: get an illegal ccb command done acb = '0x%p'"
				"ccb = '0x%p' ccbacb = '0x%p' startdone = 0x%x"
				" ccboutstandingcount = %d \n"
				, acb->host->host_no
				, acb
				, ccb
				, ccb->acb
				, ccb->startdone
				, atomic_read(&acb->ccboutstandingcount));
		}else
	arcmsr_report_ccb_state(acb, ccb, flag_ccb);
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hba_doorbell_isr(struct AdapterControlBlock *acb) 
{
    	uint32_t outbound_doorbell;
	struct MessageUnit_A __iomem *reg  = acb->pmuA;

	#if ARCMSR_DEBUG
		printk("arcmsr_hba_doorbell_isr..................................\n");
	#endif

	outbound_doorbell = readl(&reg->outbound_doorbell);
	writel(outbound_doorbell, &reg->outbound_doorbell); 
	if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK) {
		arcmsr_iop2drv_data_wrote_handle(acb);
	}
	
	if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK) 	{
		arcmsr_iop2drv_data_read_handle(acb);
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hba_postqueue_isr(struct AdapterControlBlock *acb)
{
    	uint32_t flag_ccb;
	struct MessageUnit_A __iomem *reg  = acb->pmuA;

	while((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) {
		arcmsr_drain_donequeue(acb, flag_ccb);
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hbb_postqueue_isr(struct AdapterControlBlock *acb)
{
	int32_t index;
    	uint32_t flag_ccb;
	struct MessageUnit_B *reg  = acb->pmuB;
	#if ARCMSR_DEBUG
		printk("arcmsr_hbb_postqueue_isr..................................\n");
	#endif

	index = reg->doneq_index;
	
	while((flag_ccb = reg->done_qbuffer[index]) != 0) {
		reg->done_qbuffer[index] = 0;
		arcmsr_drain_donequeue(acb, flag_ccb);
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;
		reg->doneq_index = index;
	}
}
/*
**********************************************************************************
** Handle a message interrupt
**
** The only message interrupt we expect is in response to a query for the current adapter config.  
** We want this in order to compare the drivemap so that we can detect newly-attached drives.
**********************************************************************************
*/
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static void arcmsr_hba_message_isr(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_A *reg  = acb->pmuA;

		#if ARCMSR_DEBUG
			printk("arcmsr_hba_message_isr..................................\n");
		#endif

		writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);/*clear interrupt and message state*/
		schedule_work(&acb->arcmsr_do_message_isr_bh);
	}

	/*
	**********************************************************************************
	** Handle a message interrupt
	**
	** The only message interrupt we expect is in response to a query for the current adapter config.  
	** We want this in order to compare the drivemap so that we can detect newly-attached drives.
	**********************************************************************************
	*/
	static void arcmsr_hbb_message_isr(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_B *reg  = acb->pmuB;
		#if ARCMSR_DEBUG
			printk("arcmsr_hbb_message_isr..................................\n");
		#endif

		writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
		schedule_work(&acb->arcmsr_do_message_isr_bh);
	}
#endif
/*
*******************************************************************************
*******************************************************************************
*/
static int arcmsr_handle_hba_isr(struct AdapterControlBlock *acb)
{
	uint32_t outbound_intstatus;
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	#if ARCMSR_DEBUG
		printk("arcmsr_handle_hba_isr..................................\n");
	#endif
	
	outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	if(!(outbound_intstatus & ARCMSR_MU_OUTBOUND_HANDLE_INT))	{
        	return 1;
	}
	writel(outbound_intstatus, &reg->outbound_intstatus);
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_DOORBELL_INT)	{
		arcmsr_hba_doorbell_isr(acb);
	}
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_POSTQUEUE_INT) {
		arcmsr_hba_postqueue_isr(acb);
	}
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		if(outbound_intstatus & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) 	{
			arcmsr_hba_message_isr(acb);		/* messenger of "driver to iop commands" */
		}
	#endif
	return 0;	
}
/*
*******************************************************************************
*******************************************************************************
*/
static int arcmsr_handle_hbb_isr(struct AdapterControlBlock *acb)
{
	uint32_t outbound_doorbell;
	struct MessageUnit_B *reg = acb->pmuB;

	#if ARCMSR_DEBUG
		printk("arcmsr_handle_hbb_isr..................................\n");
	#endif
	
	outbound_doorbell = readl(reg->iop2drv_doorbell) & acb->outbound_int_enable;
	if(!outbound_doorbell)
	        return 1;
	writel(~outbound_doorbell, reg->iop2drv_doorbell);	/* clear doorbell interrupt */
	readl(reg->iop2drv_doorbell);			/* in case the last action of doorbell interrupt clearance is cached, this action can push HW to write down the clear bit*/
	writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
	if(outbound_doorbell & ARCMSR_IOP2DRV_DATA_WRITE_OK) {
		arcmsr_iop2drv_data_wrote_handle(acb);
	}
	if(outbound_doorbell & ARCMSR_IOP2DRV_DATA_READ_OK) {
		arcmsr_iop2drv_data_read_handle(acb);
	}
	if(outbound_doorbell & ARCMSR_IOP2DRV_CDB_DONE) {
		arcmsr_hbb_postqueue_isr(acb);
	}
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		if(outbound_doorbell & ARCMSR_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbb_message_isr(acb);		/* messenger of "driver to iop commands" */
		}
	#endif
	return 0;
}
/*
*******************************************************************************
*******************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb)
#else
	static void arcmsr_interrupt(struct AdapterControlBlock *acb)
#endif
	{
			
		switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			if(arcmsr_handle_hba_isr(acb)) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
					return IRQ_NONE;
				#else
					return;
				#endif
			}
			} 
			break;
		
		case ACB_ADAPTER_TYPE_B: {
			if(arcmsr_handle_hbb_isr(acb)) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
					return IRQ_NONE;
				#else
					return;
				#endif
			}
			} 
			break;
		}
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			return IRQ_HANDLED;
		#else
			return;
		#endif
	}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_iop_parking(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_iop_parking.......................................\n");
	#endif
	
	if (acb) {
		/* stop adapter background rebuild */
		if (acb->acb_flags & ACB_F_MSG_START_BGRB) {
			uint32_t intmask_org;
			acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
			intmask_org = arcmsr_disable_outbound_ints(acb);
			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);
			arcmsr_enable_outbound_ints(acb, intmask_org);
		}
	}
}
/*
************************************************************************
************************************************************************
*/
static int arcmsr_iop_message_xfer(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	struct CMD_MESSAGE_FIELD *pcmdmessagefld;
	int retvalue = 0, transfer_len = 0;
	char *buffer;
	struct scatterlist *sg;
	unsigned short use_sg;
	uint32_t controlcode = (uint32_t )cmd->cmnd[5] << 24 |
				(uint32_t )cmd->cmnd[6] << 16 |
				(uint32_t )cmd->cmnd[7] << 8 |
				(uint32_t )cmd->cmnd[8]; 
					/* 4 bytes: Areca io control code */

	#if ARCMSR_DEBUG
		printk("arcmsr_iop_message_xfer.......................................\n");
	#endif

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
		use_sg = scsi_sg_count(cmd);
	#else
		use_sg = cmd->use_sg;
	#endif

	if (use_sg) {

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
			sg = scsi_sglist(cmd);
		#else
			sg = (struct scatterlist *)cmd->request_buffer;
		#endif

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
			buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			buffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
		#else
			buffer = sg->address;
		#endif

		if (use_sg > 1) {
			retvalue = ARCMSR_MESSAGE_FAIL;
			goto message_out;
		}
		transfer_len += sg->length;
	} else {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25) 
			sg = scsi_sglist(cmd);
			buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
			transfer_len = scsi_bufflen(cmd);
		#else
			buffer = cmd->request_buffer;
			transfer_len = cmd->request_bufflen;
		#endif
	}

	if (transfer_len > sizeof(struct CMD_MESSAGE_FIELD)) {
		retvalue = ARCMSR_MESSAGE_FAIL;
		goto message_out;
	}

	pcmdmessagefld = (struct CMD_MESSAGE_FIELD *) buffer;

	switch(controlcode) {
	case ARCMSR_MESSAGE_READ_RQBUFFER: {
			unsigned char *ver_addr;
			uint8_t *pQbuffer, *ptmpQbuffer;
			int32_t allxfer_len = 0;

			ver_addr = kmalloc(1032, GFP_ATOMIC);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				goto message_out;
			}

			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif

			ptmpQbuffer = ver_addr;
			while ((acb->rqbuf_firstindex != acb->rqbuf_lastindex)	&& (allxfer_len < 1031)) {
				pQbuffer = &acb->rqbuffer[acb->rqbuf_firstindex];
				memcpy(ptmpQbuffer, pQbuffer, 1);
				acb->rqbuf_firstindex++;
				acb->rqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
				ptmpQbuffer++;
				allxfer_len++;
			}
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {

				struct QBUFFER __iomem *prbuffer; 
				uint8_t __iomem *iop_data;
                			int32_t iop_len;

                			acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				prbuffer = arcmsr_get_iop_rqbuffer(acb);
				iop_data = prbuffer->data;
				iop_len = readl(&prbuffer->data_len);
				while (iop_len > 0) {
					acb->rqbuffer[acb->rqbuf_lastindex] = readb(iop_data);
					acb->rqbuf_lastindex++;
					acb->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
					iop_data++;
					iop_len--;
				}
				arcmsr_iop_message_read(acb);
			}
			memcpy(pcmdmessagefld->messagedatabuffer, ver_addr, allxfer_len);
			pcmdmessagefld->cmdmessage.Length = allxfer_len;
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			kfree(ver_addr);
		}
		break;
	case ARCMSR_MESSAGE_WRITE_WQBUFFER: {
			unsigned char *ver_addr;
			int32_t	my_empty_len, user_len,	wqbuf_firstindex, wqbuf_lastindex;
			uint8_t	*pQbuffer, *ptmpuserbuffer;

			ver_addr = kmalloc(1032, GFP_ATOMIC);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				goto message_out;
			}
			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			ptmpuserbuffer = ver_addr;
			user_len = pcmdmessagefld->cmdmessage.Length;
			memcpy(ptmpuserbuffer, pcmdmessagefld->messagedatabuffer, user_len);
			wqbuf_lastindex = acb->wqbuf_lastindex;
			wqbuf_firstindex = acb->wqbuf_firstindex;
			if (wqbuf_lastindex != wqbuf_firstindex) {
				struct SENSE_DATA *sensebuffer = (struct SENSE_DATA *)cmd->sense_buffer;
				arcmsr_post_ioctldata2iop(acb);
				/* has error report sensedata */
				cmd->sense_buffer[0] = (0x1 << 7 | 0x70); /* Valid bit and 'current errors' */
				cmd->sense_buffer[2] = ILLEGAL_REQUEST; /* SenseKey */
				cmd->sense_buffer[7] = 0x0A; /* AdditionalSenseLength*/
				cmd->sense_buffer[12] = 0x20; /* AdditionalSenseCode, i.e. ASC */
				sensebuffer->Valid = 1;
				retvalue = ARCMSR_MESSAGE_FAIL;
			} else {
				my_empty_len = (wqbuf_firstindex-wqbuf_lastindex - 1) &(ARCMSR_MAX_QBUFFER - 1);
				if (my_empty_len >= user_len) {
					while (user_len > 0) {
						pQbuffer = &acb->wqbuffer[acb->wqbuf_lastindex];
						memcpy(pQbuffer, ptmpuserbuffer, 1);
						acb->wqbuf_lastindex++;
						acb->wqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
						ptmpuserbuffer++;
						user_len--;
					}
					if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_CLEARED) {
						acb->acb_flags &= ~ACB_F_MESSAGE_WQBUFFER_CLEARED;
						arcmsr_post_ioctldata2iop(acb);
					}
				} else {
					/* has error report sensedata */
					cmd->sense_buffer[0] = (0x1 << 7 | 0x70); /* Valid,ErrorCode */
					cmd->sense_buffer[2] = ILLEGAL_REQUEST; /* SenseKey */
					cmd->sense_buffer[7] = 0x0A; /* AdditionalSenseLength */
					cmd->sense_buffer[12] = 0x20; /* AdditionalSenseCode, i.e. ASC */
					retvalue = ARCMSR_MESSAGE_FAIL;
				}
			}
			kfree(ver_addr);
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_RQBUFFER: {
			uint8_t *pQbuffer = acb->rqbuffer;
			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				arcmsr_iop_message_read(acb);
			}
			acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
			acb->rqbuf_firstindex = 0;
			acb->rqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_WQBUFFER: {
			uint8_t *pQbuffer = acb->wqbuffer;
 			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				arcmsr_iop_message_read(acb);
			}
			acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED|ACB_F_MESSAGE_WQBUFFER_READED);
			acb->wqbuf_firstindex = 0;
			acb->wqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_ALLQBUFFER: {
			uint8_t *pQbuffer;
 			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				arcmsr_iop_message_read(acb);
			}
			acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED | ACB_F_MESSAGE_WQBUFFER_READED);
			acb->rqbuf_firstindex = 0;
			acb->rqbuf_lastindex = 0;
			acb->wqbuf_firstindex = 0;
			acb->wqbuf_lastindex = 0;
			pQbuffer = acb->rqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			pQbuffer = acb->wqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_RETURN_CODE_3F: {
			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_3F;
		}
		break;
	case ARCMSR_MESSAGE_SAY_HELLO: {
			int8_t * hello_string = "Hello! I am ARCMSR";
			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
				if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
			#endif
			memcpy(pcmdmessagefld->messagedatabuffer, hello_string, (int16_t)strlen(hello_string));
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_SAY_GOODBYE: {
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
		#endif
		arcmsr_iop_parking(acb);
		break;
		}
	case ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE: {
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			if(acb->fw_state == FALSE) {
					pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
					goto message_out;
				}
		#endif
		arcmsr_flush_adapter_cache(acb);
		break;
		}
	default:
		retvalue = ARCMSR_MESSAGE_FAIL;
	}
	message_out:
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		if (use_sg) {
			kunmap_atomic(buffer - sg->offset, KM_IRQ0);
		}
	#endif
	return retvalue;
}
/*
**************************************************************************
**************************************************************************
*/
static struct CommandControlBlock * arcmsr_get_freeccb(struct AdapterControlBlock *acb)
{
	struct list_head *head = &acb->ccb_free_list;
	struct CommandControlBlock *ccb=NULL;

	#if ARCMSR_DEBUG
		printk("arcmsr_get_freeccb......................................\n");
	#endif

	if (!list_empty(head)) 
	{
		ccb = list_entry(head->next, struct CommandControlBlock, list);
		list_del(head->next);
	}
	return ccb;
}
/*
***********************************************************************
***********************************************************************
*/
static void arcmsr_handle_virtual_command(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_handle_virtual_command......................................\n");
	#endif
	
	switch (cmd->cmnd[0]) {
	case INQUIRY: {
		unsigned char inqdata[36];
		char *buffer;
		unsigned short use_sg;
		if (cmd->device->lun) {
			cmd->result = (DID_TIME_OUT << 16);
			cmd->scsi_done(cmd);
			return;
		}
		inqdata[0] = TYPE_PROCESSOR;
		/* Periph Qualifier & Periph Dev Type */
		inqdata[1] = 0;
		/* rem media bit & Dev Type Modifier */
		inqdata[2] = 0;
		/* ISO,ECMA,& ANSI versions */
		inqdata[4] = 31;
		/* length of additional data */
		strncpy(&inqdata[8], "Areca   ", 8);
		/* Vendor Identification */
		strncpy(&inqdata[16], "RAID controller ", 16);
		/* Product Identification */
		strncpy(&inqdata[32], "R001", 4); /* Product Revision */

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
			use_sg = scsi_sg_count(cmd);
		#else
			use_sg = cmd->use_sg;
		#endif
		
		if (use_sg) {
			struct scatterlist *sg;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
				sg = scsi_sglist(cmd);
			#else
				sg = (struct scatterlist *) cmd->request_buffer;
			#endif

			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
				buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
			#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				buffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
			#else
				buffer = sg->address;
			#endif
		} else {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
				struct scatterlist *sg = scsi_sglist(cmd);
				buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
			#else
				buffer = cmd->request_buffer;
			#endif

		}
		memcpy(buffer, inqdata, sizeof(inqdata));
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
			if (scsi_sg_count(cmd)) {
				struct scatterlist *sg;

				sg = (struct scatterlist *) scsi_sglist(cmd);
				kunmap_atomic(buffer - sg->offset, KM_IRQ0);
			}
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
			if (cmd->use_sg) {
				struct scatterlist *sg;

				sg = (struct scatterlist *) cmd->request_buffer;
				kunmap_atomic(buffer - sg->offset, KM_IRQ0);
			}
		#endif
		cmd->scsi_done(cmd);
	}
	break;
	case WRITE_BUFFER:
	case READ_BUFFER: {
		if (arcmsr_iop_message_xfer(acb, cmd))
			cmd->result = (DID_ERROR << 16);
		cmd->scsi_done(cmd);
	}
	break;
	default:
		cmd->scsi_done(cmd);
	}
}
/*
***********************************************************************
***********************************************************************
*/
int arcmsr_queue_command(struct scsi_cmnd *cmd, void (* done)(struct scsi_cmnd *))
{
	struct Scsi_Host *host = cmd->device->host;
	struct AdapterControlBlock *acb=(struct AdapterControlBlock *) host->hostdata;
	struct CommandControlBlock *ccb;
	int target=cmd->device->id;
	int lun=cmd->device->lun;
	uint8_t scsicmd	= cmd->cmnd[0];

	#if ARCMSR_DEBUG
    		printk("arcmsr_queue_command: Cmd=%2x,TargetId=%d,Lun=%d \n",scsicmd,target,lun);
	#endif
	
	cmd->scsi_done=done;
	cmd->host_scribble=NULL;
	cmd->result=0;
	
    	/* 
	*******************************************************
	** Enlarge the timeout duration of each scsi command, it could
	** aviod  the vibration factor with sata disk on some bad machine
	*******************************************************
	*/
	if((scsicmd == SYNCHRONIZE_CACHE) || (scsicmd == SEND_DIAGNOSTIC)) {
		if(acb->devstate[target][lun] == ARECA_RAID_GONE) {
	    		cmd->result = (DID_NO_CONNECT << 16);
		}
		cmd->scsi_done(cmd);
		return 0;
	}

	if (acb->acb_flags & ACB_F_BUS_RESET) {
		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {
				struct MessageUnit_A __iomem *reg = acb->pmuA;
				uint32_t intmask_org,outbound_doorbell;

				if((readl(&reg->outbound_msgaddr1) & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0 ) {
					printk(KERN_NOTICE "arcmsr%d: bus reset and return busy \n", acb->host->host_no);
					return SCSI_MLQUEUE_HOST_BUSY;
				}

				acb->acb_flags &= ~ACB_F_FIRMWARE_TRAP;
				printk(KERN_NOTICE "arcmsr%d: hardware bus reset and reset ok \n", acb->host->host_no);
				/* disable all outbound interrupt */
				intmask_org = arcmsr_disable_outbound_ints(acb);
 				arcmsr_get_firmware_spec(acb,1);
				/*start background rebuild*/
				arcmsr_start_adapter_bgrb(acb);
				/* clear Qbuffer if door bell ringed */
				outbound_doorbell = readl(&reg->outbound_doorbell);
				writel(outbound_doorbell, &reg->outbound_doorbell);/*clear interrupt */
   				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
				/* enable outbound Post Queue,outbound doorbell Interrupt */
				arcmsr_enable_outbound_ints(acb, intmask_org);
				acb->acb_flags |= ACB_F_IOP_INITED;
				acb->acb_flags &= ~ACB_F_BUS_RESET;
			}
			break;
			case ACB_ADAPTER_TYPE_B: {
			}
		}
	}

	if(target == 16) {
		/* virtual device for iop message transfer */
		arcmsr_handle_virtual_command(acb, cmd);
		return 0;
	}

	if(acb->devstate[target][lun]==ARECA_RAID_GONE) {
		uint8_t block_cmd;

		block_cmd=scsicmd & 0x0f;
		if(block_cmd==0x08 || block_cmd==0x0a) {
			cmd->result=(DID_NO_CONNECT << 16);
			cmd->scsi_done(cmd);
			return 0;
		}
	}

	if (atomic_read(&acb->ccboutstandingcount) >= ARCMSR_MAX_OUTSTANDING_CMD)
		return SCSI_MLQUEUE_HOST_BUSY;

    	ccb = arcmsr_get_freeccb(acb);
	if (!ccb)
		return SCSI_MLQUEUE_HOST_BUSY;
	
	if(arcmsr_build_ccb(acb, ccb, cmd)==FAILED){
		cmd->result=(DID_ERROR << 16) |(RESERVATION_CONFLICT << 1);
		cmd->scsi_done(cmd);
		return 0;
	}

	arcmsr_post_ccb(acb, ccb);
	return 0;
}
/*
**********************************************************************
**********************************************************************
*/
static void *arcmsr_get_hba_config(struct AdapterControlBlock *acb, int mode)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model = (char __iomem *) (&reg->msgcode_rwbuffer[15]);
	char __iomem *iop_firm_version = (char __iomem *) (&reg->msgcode_rwbuffer[17]);
	char __iomem *iop_device_map = (char __iomem *) (&reg->msgcode_rwbuffer[21]);
	int count;

	#if ARCMSR_DEBUG
		printk("arcmsr_get_hba_config......................................\n");
	#endif

    	writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
	if(arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'get adapter firmware miscellaneous data' timeout \n", acb->host->host_no);
		return NULL;
	}
	if(mode == 1){
		count = 8;
		while(count) {
        			*acb_firm_model = readb(iop_firm_model);
        			acb_firm_model++;
			iop_firm_model++;
			count--;
		}

		count = 16;	
		while(count) {
        			*acb_firm_version = readb(iop_firm_version);
        			acb_firm_version++;
			iop_firm_version++;
			count--;		
		}

		count=16;
		while(count) {
        			*acb_device_map=readb(iop_device_map);
        			acb_device_map++;
			iop_device_map++;
			count--;
 		}

		printk(KERN_INFO "ARECA RAID ADAPTER%d: FIRMWARE VERSION %s \n" , acb->host->host_no , acb->firm_version);
		acb->signature = readl(&reg->msgcode_rwbuffer[0]);
		acb->firm_request_len = readl(&reg->msgcode_rwbuffer[1]);
		acb->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[2]);
		acb->firm_sdram_size = readl(&reg->msgcode_rwbuffer[3]);
		acb->firm_ide_channels = readl(&reg->msgcode_rwbuffer[4]);
	}
	return (&reg->msgcode_rwbuffer[0]);
}
/*
**********************************************************************
**********************************************************************
*/
static void __iomem *arcmsr_get_hbb_config(struct AdapterControlBlock *acb, int mode)
{
 	struct MessageUnit_B *reg = acb->pmuB;
	uint32_t __iomem *lrwbuffer = reg->msgcode_rwbuffer;
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model = (char __iomem *) (&lrwbuffer[15]);	/*firm_model,15,60-67*/
	char __iomem *iop_firm_version = (char __iomem *) (&lrwbuffer[17]);	/*firm_version,17,68-83*/
	char __iomem *iop_device_map = (char __iomem *) (&lrwbuffer[21]);	/*firm_version,21,84-99*/
 	int count;

	#if ARCMSR_DEBUG
		printk("arcmsr_get_hbb_config......................................\n");
	#endif

	writel(ARCMSR_MESSAGE_GET_CONFIG, reg->drv2iop_doorbell);
	if(arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait get adapter firmware miscellaneous data timeout \n", acb->host->host_no);
		return NULL;
	}
	if(mode == 1) {
 		count=8;
 		while(count) {
			*acb_firm_model=readb(iop_firm_model);
        			acb_firm_model++;
  			iop_firm_model++;
  			count--;
 		}
	
 		count=16;
		while(count) {
        			*acb_firm_version=readb(iop_firm_version);
        			acb_firm_version++;
			iop_firm_version++;
			count--;
 		}

		count=16;
		while(count) {
        			*acb_device_map=readb(iop_device_map);
        			acb_device_map++;
			iop_device_map++;
			count--;
 		}

		printk(KERN_INFO "ARECA RAID ADAPTER%d: FIRMWARE VERSION %s \n", acb->host->host_no, acb->firm_version);
	
		acb->signature = readl(lrwbuffer++);
		/*firm_signature,1,00-03*/
		acb->firm_request_len=readl(lrwbuffer++); 
		/*firm_request_len,1,04-07*/
 		acb->firm_numbers_queue=readl(lrwbuffer++); 
		/*firm_numbers_queue,2,08-11*/
 		acb->firm_sdram_size=readl(lrwbuffer++); 
		/*firm_sdram_size,3,12-15*/
		acb->firm_ide_channels=readl(lrwbuffer++); 
		/*firm_ide_channels,4,16-19*/
	}
	return (reg->msgcode_rwbuffer);
}
/*
**********************************************************************
**********************************************************************
*/
static void *arcmsr_get_firmware_spec(struct AdapterControlBlock *acb, int mode)
{
	void *rtnval = 0;
	#if ARCMSR_DEBUG
		printk("arcmsr_get_firmware_spec.................................. \n");
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			rtnval = arcmsr_get_hba_config(acb, mode);
		}
		break;
		
		case ACB_ADAPTER_TYPE_B: {
			rtnval = arcmsr_get_hbb_config(acb, mode);
		}
		break;
	}
	return rtnval;
}
/*
**********************************************************************************
**********************************************************************************
*/
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static void arcmsr_request_hba_device_map(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		
		#if ARCMSR_DEBUG
			printk("arcmsr_request_hba_device_map...............................\n");
		#endif

		if(unlikely(atomic_read(&acb->rq_map_token) == 0)) {
			acb->fw_state = FALSE;
		} else {
		/*to prevent rq_map_token from changing by other interrupt,
		then avoid the dead-lock*/
			acb->fw_state = TRUE;
			atomic_dec(&acb->rq_map_token);
			if((acb->fw_state == FALSE)||(acb->ante_token_value==atomic_read(&acb->rq_map_token))){

				atomic_set(&acb->rq_map_token,16);
			}
			acb->ante_token_value=atomic_read(&acb->rq_map_token);
			writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
		}
		mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6000));
		return;
	}
	/*
	**********************************************************************
	**  
	**********************************************************************
	*/
	static void arcmsr_request_hbb_device_map(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_B __iomem *reg = acb->pmuB;

		#if ARCMSR_DEBUG
			printk("arcmsr_request_hbb_device_map..............................\n");
		#endif

		if(unlikely(atomic_read(&acb->rq_map_token) == 0)) {
			acb->fw_state = FALSE;
		} else {
		/*
		**to prevent rq_map_token from changing by other interrupt,
		**then avoid the dead-lock
		*/
			acb->fw_state = TRUE;
			atomic_dec(&acb->rq_map_token);
			if((acb->fw_state == FALSE)||(acb->ante_token_value==atomic_read(&acb->rq_map_token))){
				atomic_set(&acb->rq_map_token,16);
			}
			acb->ante_token_value=atomic_read(&acb->rq_map_token);
			writel(ARCMSR_MESSAGE_GET_CONFIG, reg->drv2iop_doorbell);
		}
		mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6000));
		return;
	}
	/*
	**********************************************************************
	**********************************************************************
	*/
	static void arcmsr_request_device_map(unsigned long pacb)
	{

		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;

		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {
				arcmsr_request_hba_device_map(acb);
			}
			break;
			case ACB_ADAPTER_TYPE_B: {
				arcmsr_request_hbb_device_map(acb);
			}
			break;
		}
	}
#endif
/*
************************************************************************
************************************************************************
*/
static void arcmsr_start_hba_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_start_hba_bgrb.................................. \n");
	#endif

	acb->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, &reg->inbound_msgaddr0);
	if(arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", acb->host->host_no);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_start_hbb_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_start_hbb_bgrb.................................. \n");
	#endif
	
	acb->acb_flags |= ACB_F_MSG_START_BGRB;
    	writel(ARCMSR_MESSAGE_START_BGRB, reg->drv2iop_doorbell);
	if(arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n",acb->host->host_no);
	}
}
/*
************************************************************************
**  start background rebulid
************************************************************************
*/
static void arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_start_adapter_bgrb.................................. \n");
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A:
		arcmsr_start_hba_bgrb(acb);
		break;
	case ACB_ADAPTER_TYPE_B:
		arcmsr_start_hbb_bgrb(acb);
		break;
	}
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_polling_hba_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	struct CommandControlBlock *ccb;
	uint32_t flag_ccb, outbound_intstatus, poll_ccb_done = 0, poll_count = 0;
	int id, lun;	

	#if ARCMSR_DEBUG
		printk("arcmsr_polling_hba_ccbdone.................................. \n");
	#endif

 	polling_hba_ccb_retry:
	poll_count++;
	outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
	while (1) {
		/*if the outbound postqueue is empty, the value of -1 is returned.*/
		if ((flag_ccb = readl(&reg->outbound_queueport)) == 0xFFFFFFFF) {
			if (poll_ccb_done)
				break;
			else {
				arc_mdelay(25);
				if (poll_count > 100)
					break;
				goto polling_hba_ccb_retry;
			}
		}
		ccb = (struct CommandControlBlock *)(acb->vir2phy_offset + (flag_ccb << 5));
		poll_ccb_done = ((ccb == poll_ccb)|| (poll_ccb_done == 1)) ? 1:0;
		if ((ccb->acb != acb) || (ccb->startdone != ARCMSR_CCB_START)) {
			if ((ccb->startdone == ARCMSR_CCB_ABORTED) || (ccb == poll_ccb)) {
				id = ccb->pcmd->device->id;
				lun = ccb->pcmd->device->lun;
				printk(KERN_WARNING "arcmsr%d: scsi id = %d lun = %d ccb = '0x%p'"
					" poll command abort successfully \n", acb->host->host_no, ccb->pcmd->device->id, ccb->pcmd->device->lun, ccb);
				if(acb->dev_aborts[id][lun] >= 4) {
					acb->devstate[id][lun] = ARECA_RAID_GONE;
					ccb->pcmd->result = DID_NO_CONNECT << 16;
				}
			ccb->pcmd->result = DID_ABORT << 16;
			arcmsr_ccb_complete(ccb, 1);
			continue;
		}
			printk(KERN_NOTICE "arcmsr%d: polling get an illegal ccb"
				" command done ccb = '0x%p'"
				"ccboutstandingcount = %d \n"
				, acb->host->host_no
				, ccb
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		else
		arcmsr_report_ccb_state(acb, ccb, flag_ccb);
	}
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_polling_hbb_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	struct CommandControlBlock *ccb;
	uint32_t flag_ccb, poll_ccb_done = 0, poll_count = 0;
	int32_t index;
	int id, lun;

	#if ARCMSR_DEBUG
		printk("arcmsr_polling_hbb_ccbdone.................................. \n");
	#endif

	polling_hbb_ccb_retry:
	poll_count++;
	writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell); /* clear doorbell interrupt */

		while(1) 	{
			index = reg->doneq_index;
			if((flag_ccb = reg->done_qbuffer[index]) == 0) {
				if(poll_ccb_done) 
				{
					break;
				} 
				else 
				{
					arc_mdelay(25);
					if(poll_count > 100)
					{
						break;
					}
					goto polling_hbb_ccb_retry;
				}
			}
			reg->done_qbuffer[index] = 0;
			index++;
			index %= ARCMSR_MAX_HBB_POSTQUEUE;     /*if last index number set it to 0 */
			reg->doneq_index = index;
			/* check ifcommand done with no error*/
			ccb = (struct CommandControlBlock *)(acb->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
			poll_ccb_done = ((ccb == poll_ccb)|| (poll_ccb_done == 1)) ? 1:0;
			if((ccb->acb!=acb) || (ccb->startdone != ARCMSR_CCB_START)) {
				if((ccb->startdone == ARCMSR_CCB_ABORTED) || (ccb == poll_ccb)) {
					id = ccb->pcmd->device->id;
					lun = ccb->pcmd->device->lun;
					printk(KERN_NOTICE "arcmsr%d: scsi id=%d lun=%d ccb='0x%p' poll command abort successfully \n"
						,acb->host->host_no
						,ccb->pcmd->device->id
						,ccb->pcmd->device->lun
						,ccb);
				if(acb->dev_aborts[id][lun] >= 4) {
					acb->devstate[id][lun]=ARECA_RAID_GONE;
					ccb->pcmd->result = DID_NO_CONNECT << 16;
				}
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb, 1);
				continue;
				}
				printk(KERN_NOTICE "arcmsr%d: polling get an illegal ccb"
					" command done ccb = '0x%p'"
					"ccboutstandingcount = %d \n"
					, acb->host->host_no
					, ccb
					, atomic_read(&acb->ccboutstandingcount));
				continue;
			}
			else
			arcmsr_report_ccb_state(acb, ccb, flag_ccb);
		}	/*drain reply FIFO*/
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_polling_ccbdone.................................. \n");
	#endif

	switch (acb->adapter_type) {
		
	case ACB_ADAPTER_TYPE_A: {
		arcmsr_polling_hba_ccbdone(acb,poll_ccb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		arcmsr_polling_hbb_ccbdone(acb,poll_ccb);
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb)
{
	int i = 0;
	uint32_t flag_ccb;

	#if ARCMSR_DEBUG
		printk("arcmsr_done4abort_postqueue................................. \n");
	#endif

	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		uint32_t outbound_intstatus;
		outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
		/*clear and abort all outbound posted Q*/
		writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
		while(((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < ARCMSR_MAX_OUTSTANDING_CMD)) {
			arcmsr_drain_donequeue(acb, flag_ccb);
		}
		}
		break;

	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		/*clear all outbound posted Q*/
		writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);
		for(i = 0; i < ARCMSR_MAX_HBB_POSTQUEUE; i++) {
			if((flag_ccb = reg->done_qbuffer[i]) != 0) {
				reg->done_qbuffer[i] = 0;
				arcmsr_drain_donequeue(acb,flag_ccb);
			}
			reg->post_qbuffer[i] = 0;
		}
		reg->doneq_index = 0;
		reg->postq_index = 0;
		}
		break;
	}
}
/*
**********************************************************************
**********************************************************************
*/	
static void arcmsr_iop_confirm(struct AdapterControlBlock *acb)
{
	uint32_t cdb_phyaddr, ccb_phyaddr_hi32;
	dma_addr_t dma_coherent_handle;
	/*
	********************************************************************
	** here we need to tell iop 331 our freeccb.HighPart 
	** if freeccb.HighPart is not zero
	********************************************************************
	*/
	dma_coherent_handle = acb->dma_coherent_handle;
	cdb_phyaddr = (uint32_t)(dma_coherent_handle);
	ccb_phyaddr_hi32 = (uint32_t)((cdb_phyaddr >> 16) >> 16);
	/*
	***********************************************************************
	**    if adapter type B, set window of "post command Q" 
	***********
	************************************************************
	*/	
	#if ARCMSR_DEBUG
		printk("arcmsr_iop_confirm.................................. \n");
	#endif
	
	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		if(ccb_phyaddr_hi32 != 0) {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			uint32_t intmask_org;
			intmask_org = arcmsr_disable_outbound_ints(acb);
			writel(ARCMSR_SIGNATURE_SET_CONFIG, &reg->msgcode_rwbuffer[0]);
			writel(ccb_phyaddr_hi32, &reg->msgcode_rwbuffer[1]);
			writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, &reg->inbound_msgaddr0);
			if(arcmsr_hba_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d: ""'set ccb high part physical address' timeout\n",
				acb->host->host_no);
			}
			arcmsr_enable_outbound_ints(acb, intmask_org);
		}
		}
		break;
	
	case ACB_ADAPTER_TYPE_B: {
		dma_addr_t post_queue_phyaddr;
		uint32_t __iomem *rwbuffer;
		struct MessageUnit_B *reg = acb->pmuB;
		uint32_t intmask_org;
		intmask_org = arcmsr_disable_outbound_ints(acb);
		reg->postq_index = 0;
		reg->doneq_index = 0;
		writel(ARCMSR_MESSAGE_SET_POST_WINDOW, reg->drv2iop_doorbell);
		if(arcmsr_hbb_wait_msgint_ready(acb)) {
			printk(KERN_NOTICE "arcmsr%d:can not set diver mode\n", acb->host->host_no);
		}
		post_queue_phyaddr = cdb_phyaddr + ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + offsetof(struct MessageUnit_B, post_qbuffer);
		rwbuffer = reg->msgcode_rwbuffer;
		writel(ARCMSR_SIGNATURE_SET_CONFIG, rwbuffer++);			/* driver "set config" signature				*/
		writel(ccb_phyaddr_hi32, rwbuffer++);						/* normal should be zero					*/
		writel(post_queue_phyaddr, rwbuffer++);						/* postQ size (256+8)*4					*/
		writel(post_queue_phyaddr + 1056, rwbuffer++);				/* doneQ size (256+8)*4					*/
		writel(1056, rwbuffer);										/* ccb maxQ size must be --> [(256+8)*4]	*/
		writel(ARCMSR_MESSAGE_SET_CONFIG, reg->drv2iop_doorbell);
		if(arcmsr_hbb_wait_msgint_ready(acb)) {
			printk(KERN_NOTICE "arcmsr%d: 'set command Q window' timeout \n",acb->host->host_no);
		}

		writel(ARCMSR_MESSAGE_START_DRIVER_MODE, reg->drv2iop_doorbell);
		if(arcmsr_hbb_wait_msgint_ready(acb)) {
			printk(KERN_NOTICE "arcmsr%d: 'can not set diver mode \n",acb->host->host_no);
		}
		arcmsr_enable_outbound_ints(acb, intmask_org);
		}
		break;
	}
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_wait_firmware_ready(struct AdapterControlBlock *acb)
{
	uint32_t firmware_state = 0;
	#if ARCMSR_DEBUG
		printk("arcmsr_wait_firmware_ready............................\n");
	#endif

	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		do {
			firmware_state = readl(&reg->outbound_msgaddr1);
		}while((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		do {
			firmware_state = readl(reg->iop2drv_doorbell);
		}while((firmware_state & ARCMSR_MESSAGE_FIRMWARE_OK) == 0);
		writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
		}
		break;
	}
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_clear_doorbell_queue_buffer(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG
		printk("arcmsr_clear_doorbell_queue_buffer................................. \n");
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A *reg = acb->pmuA;
		uint32_t outbound_doorbell;
		/* empty doorbell Qbuffer if door bell ringed */
		outbound_doorbell = readl(&reg->outbound_doorbell);
		writel(outbound_doorbell, &reg->outbound_doorbell);/*clear doorbell interrupt */
		writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
		}
		break;
	
	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
		writel(ARCMSR_DRV2IOP_DATA_READ_OK, reg->drv2iop_doorbell);
		/* let IOP know data has been read */
		}
		break;
	}
}
/*
************************************************************************
**
************************************************************************
*/
static void arcmsr_enable_eoi_mode(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type)
	{
	case ACB_ADAPTER_TYPE_A:
		return;
	case ACB_ADAPTER_TYPE_B:
		{
	        	struct MessageUnit_B *reg = acb->pmuB;
			writel(ARCMSR_MESSAGE_ACTIVE_EOI_MODE, reg->drv2iop_doorbell);
			if(arcmsr_hbb_wait_msgint_ready(acb))
			{
				printk(KERN_NOTICE "ARCMSR IOP enables EOI_MODE TIMEOUT");
				return;
			}
		}
		break;
	}
	return;
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_hardware_reset(struct AdapterControlBlock *acb)
{
	uint8_t value[64];
	int i;

	/* backup pci config data */
	for (i=0; i<64; i++) {
		pci_read_config_byte(acb->pdev, i, &value[i]);
	}
	/* hardware reset signal */
	pci_write_config_byte(acb->pdev, 0x84, 0x20);
	arc_mdelay(1000);
	/* write back pci config data */
	for (i=0;i<64;i++) {
		pci_write_config_byte(acb->pdev, i, value[i]);
	}
	arc_mdelay(1000);
	return;
}
/*
****************************************************************************
****************************************************************************
*/
#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	int arcmsr_sleep_for_bus_reset(struct scsi_cmnd *cmd)
	{
			struct Scsi_Host *shost = NULL;
			spinlock_t *host_lock = NULL;
			int i, isleep;

			shost = cmd->device->host;
			host_lock = shost->host_lock;

			printk("Host %d bus reset over, sleep %d seconds (busy %d, can queue %d) ...........\n", 
					shost->host_no, sleeptime, shost->host_busy, shost->can_queue);
			isleep = sleeptime / 10;
			spin_unlock_irq(host_lock);
			if (isleep > 0) {
				for (i = 0; i < isleep; i ++) {
					arc_mdelay(10000);
					printk("^%d^\n", i);
				}
			}
			
			isleep = sleeptime % 10;
			if (isleep > 0) {
				arc_mdelay(isleep*1000);
				printk("^v^\n");
			}
			spin_lock_irq(host_lock);	        
			printk("***** wake up *****\n");
			return 0;
	}
#endif
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_iop_init(struct AdapterControlBlock *acb)
{

    	uint32_t intmask_org;

	#if ARCMSR_DEBUG
		printk("arcmsr_iop_init.................................. \n");
	#endif

	 intmask_org = arcmsr_disable_outbound_ints(acb);		/* disable all outbound interrupt */
	arcmsr_wait_firmware_ready(acb);
	arcmsr_iop_confirm(acb);
	arcmsr_get_firmware_spec(acb, 1);	
	arcmsr_start_adapter_bgrb(acb);				/*start background rebuild*/
	arcmsr_clear_doorbell_queue_buffer(acb);			/* empty doorbell Qbuffer if door bell ringed */
	arcmsr_enable_eoi_mode(acb);	
	arcmsr_enable_outbound_ints(acb,intmask_org);		/* enable outbound Post Queue,outbound doorbell Interrupt */
	acb->acb_flags |= ACB_F_IOP_INITED;
}
/*
****************************************************************************
****************************************************************************
*/
static uint8_t arcmsr_iop_reset(struct AdapterControlBlock *acb)
{
	struct CommandControlBlock *ccb;
	uint32_t intmask_org;
	uint8_t rtnval = 0x00;
	int i = 0;

	#if ARCMSR_DEBUG
		printk("arcmsr_iop_reset: reset iop controller......................................\n");
	#endif

	if (atomic_read(&acb->ccboutstandingcount) != 0) {
		#if ARCMSR_DEBUG
			printk("arcmsr_iop_reset: ccboutstandingcount=%d ...\n",atomic_read(&acb->ccboutstandingcount));
		#endif
	
		/* disable all outbound interrupt */
		intmask_org = arcmsr_disable_outbound_ints(acb);
		/* talk to iop 331 outstanding command aborted */
		rtnval = arcmsr_abort_allcmd(acb);
		/* clear all outbound posted Q */
		arcmsr_done4abort_postqueue(acb);
		for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
			ccb = acb->pccb_pool[i];
			if (ccb->startdone == ARCMSR_CCB_START) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					arcmsr_pci_unmap_dma(ccb);
				#endif
				ccb->startdone = ARCMSR_CCB_DONE;
				ccb->ccb_flags = 0;
				list_add_tail(&ccb->list, &acb->ccb_free_list);
			}
		}
		atomic_set(&acb->ccboutstandingcount, 0);
		/* enable all outbound interrupt */
		arcmsr_enable_outbound_ints(acb, intmask_org);
		return rtnval;
	}
	return rtnval;
}
/*
****************************************************************************
****************************************************************************
*/
int arcmsr_bus_reset(struct scsi_cmnd *cmd) {
	struct AdapterControlBlock *acb;
    	int retry = 0;

	acb = (struct AdapterControlBlock *) cmd->device->host->hostdata;
	if(acb->acb_flags & ACB_F_BUS_RESET)
		return SUCCESS;

	printk("arcmsr%d: bus reset ..... \n", acb->adapter_index);
	acb->acb_flags |= ACB_F_BUS_RESET;
	acb->num_resets++;
	while(atomic_read(&acb->ccboutstandingcount) != 0 && retry < 4) {
		arcmsr_interrupt(acb);
		retry++;
	}

	if(arcmsr_iop_reset(acb)) {
		switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			uint32_t intmask_org, outbound_doorbell;
			int retry_count = 0;

			printk("arcmsr%d: do hardware bus reset .....num_resets = %d num_aborts = %d \n", acb->adapter_index, acb->num_resets, acb->num_aborts);
			arcmsr_hardware_reset(acb);
			acb->acb_flags |= ACB_F_FIRMWARE_TRAP;
			acb->acb_flags &= ~ACB_F_IOP_INITED;
			#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			sleep_again:
				arcmsr_sleep_for_bus_reset(cmd);
				if(( readl(&reg->outbound_msgaddr1) & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0 ) {
					printk(KERN_NOTICE "arcmsr%d: hardware bus reset and return busy, retry=%d \n", acb->host->host_no, retry_count);
					if(retry_count > retrycount) {
						printk(KERN_NOTICE "arcmsr%d: hardware bus reset and return busy, retry aborted \n", acb->host->host_no);
						return SUCCESS;
					}
					retry_count++;
					goto sleep_again;
				}
				acb->acb_flags &= ~ACB_F_FIRMWARE_TRAP;
				acb->acb_flags |= ACB_F_IOP_INITED;
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				printk(KERN_NOTICE "arcmsr%d: hardware bus reset and reset ok \n", acb->host->host_no);
				/* disable all outbound interrupt */
				intmask_org=arcmsr_disable_outbound_ints(acb);
 				arcmsr_get_firmware_spec(acb,1);
				/*start	background rebuild*/
				arcmsr_start_adapter_bgrb(acb);
				/* clear Qbuffer if door bell ringed */
				outbound_doorbell=readl(&reg->outbound_doorbell);
				writel(outbound_doorbell, &reg->outbound_doorbell); /*clear interrupt */
   				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
				/* enable outbound Post Queue,outbound doorbell Interrupt */
				arcmsr_enable_outbound_ints(acb, intmask_org);
				#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
					atomic_set(&acb->rq_map_token, 16);
					init_timer(&acb->eternal_timer);
					acb->eternal_timer.expires = jiffies + msecs_to_jiffies(20*HZ);
					acb->eternal_timer.data = (unsigned long) acb;
					acb->eternal_timer.function = &arcmsr_request_device_map;
					add_timer(&acb->eternal_timer);
				#endif
			#endif
		}
		break;
		case ACB_ADAPTER_TYPE_B: {
		}
		}
	}
	else {
       		 acb->acb_flags &= ~ACB_F_BUS_RESET;
	}	

	arcmsr_iop_reset(acb);
	acb->acb_flags &= ~ACB_F_BUS_RESET;
	return SUCCESS;
}
/*
*****************************************************************************************
*****************************************************************************************
*/
static void arcmsr_abort_one_cmd(struct AdapterControlBlock *acb,	struct CommandControlBlock *ccb)
{
	u32 intmask;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_abort_one_cmd................................. \n");
	#endif

	/*
	** Wait for 3 sec for all command done.
	*/
	arc_mdelay_int(3000);
	intmask = arcmsr_disable_outbound_ints(acb);
	arcmsr_polling_ccbdone(acb, ccb);
	arcmsr_enable_outbound_ints(acb, intmask);
}
/*
*****************************************************************************************
*****************************************************************************************
*/
int arcmsr_abort(struct scsi_cmnd *cmd)
{
	struct AdapterControlBlock *acb	 = (struct AdapterControlBlock *)cmd->device->host->hostdata;
	int i;	
	#if ARCMSR_DEBUG
		printk("arcmsr_abort................................. \n");
	#endif

	acb->num_aborts++;

	if (!atomic_read(&acb->ccboutstandingcount))
		return SUCCESS;
	
	for (i = 0; i <	ARCMSR_MAX_FREECCB_NUM; i++) {
		struct CommandControlBlock *ccb = acb->pccb_pool[i];
		if (ccb->startdone == ARCMSR_CCB_START && ccb->pcmd == cmd) {
			ccb->startdone = ARCMSR_CCB_ABORTED;
			arcmsr_abort_one_cmd(acb, ccb);
			break;
		}
	}
		return SUCCESS;
}
/*
*********************************************************************
*********************************************************************
*/
const char *arcmsr_info(struct Scsi_Host *host)
{
	struct AdapterControlBlock *acb	= (struct AdapterControlBlock *) host->hostdata;
	static char buf[256];
	char *type;
	int raid6 = 1;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_info........................................ \n");
	#endif

	switch (acb->pdev->device) {
	case PCI_DEVICE_ID_ARECA_1110:
	case PCI_DEVICE_ID_ARECA_1200:
	case PCI_DEVICE_ID_ARECA_1202:
	case PCI_DEVICE_ID_ARECA_1210:
		raid6 =	0;
		/*FALLTHRU*/
	case PCI_DEVICE_ID_ARECA_1120:
	case PCI_DEVICE_ID_ARECA_1130:
	case PCI_DEVICE_ID_ARECA_1160:
	case PCI_DEVICE_ID_ARECA_1170:
	case PCI_DEVICE_ID_ARECA_1201:
	case PCI_DEVICE_ID_ARECA_1220:
	case PCI_DEVICE_ID_ARECA_1230:
	case PCI_DEVICE_ID_ARECA_1260:
	case PCI_DEVICE_ID_ARECA_1270:
	case PCI_DEVICE_ID_ARECA_1280:
		type = "SATA";
		break;
	case PCI_DEVICE_ID_ARECA_1380:
	case PCI_DEVICE_ID_ARECA_1381:
	case PCI_DEVICE_ID_ARECA_1680:
	case PCI_DEVICE_ID_ARECA_1681:
		type = "SAS";
		break;
	default:
		type = "X-TYPE";
		break;
	}
	sprintf(buf, "Areca %s Host Adapter RAID Controller%s\n	       %s",
			type, raid6 ? "( RAID6 capable)" : "",
			ARCMSR_DRIVER_VERSION);
	return buf;
}
/*
*********************************************************************
**  duplicate arcmsr_initialize() to arcmsr_alloc_ccb_pool(), in which add a line of 
**  INIT_LIST_HEAD() that originally been put in arcmsr_probe() of kernel.org version
*********************************************************************
*/
static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb)
{
	struct HCBARC *pHCBARC =& arcmsr_host_control_block;
	struct pci_dev *pdev = acb->pdev;
	
	#if ARCMSR_DEBUG
		printk("arcmsr_alloc_ccb_pool....................................\n");
	#endif

	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		
		void *dma_coherent;
		dma_addr_t dma_coherent_handle, dma_addr;
		struct CommandControlBlock *ccb_tmp;
		//uint32_t intmask_org;
		int i, j;

		acb->pmuA = ioremap(pci_resource_start(pdev,0), pci_resource_len(pdev,0));
		if (!acb->pmuA) {
			printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
			return -ENOMEM;
		}

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)	
			dma_coherent = dma_alloc_coherent(&pdev->dev,ARCMSR_MAX_FREECCB_NUM *sizeof (struct CommandControlBlock) + 0x20,	&dma_coherent_handle, GFP_KERNEL);
		#else
			dma_coherent = pci_alloc_consistent(pdev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) +	0x20, &dma_coherent_handle);
		#endif
		if (!dma_coherent) {
			printk("arcmsr%d: dma_alloc_coherent got error \n",arcmsr_adapterCnt);
			iounmap(acb->pmuA);
			return -ENOMEM;
		}

		acb->dma_coherent = dma_coherent;
		acb->dma_coherent_handle = dma_coherent_handle;
		memset(dma_coherent, 0, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock)+0x20);
		if (((unsigned long)dma_coherent & 0x1F)!=0) {
			dma_coherent = dma_coherent + (0x20 - ((unsigned long)dma_coherent & 0x1F));
			dma_coherent_handle = dma_coherent_handle + (0x20 - ((unsigned long)dma_coherent_handle & 0x1F));
		}

		dma_addr = dma_coherent_handle;
		ccb_tmp = (struct CommandControlBlock *)dma_coherent;
		for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
			ccb_tmp->cdb_shifted_phyaddr = dma_addr >> 5;
			ccb_tmp->acb = acb;
			acb->pccb_pool[i] = ccb_tmp;
			list_add_tail(&ccb_tmp->list, &acb->ccb_free_list);
			dma_addr = dma_addr + sizeof (struct CommandControlBlock);
			ccb_tmp++;
		}

		acb->vir2phy_offset = (unsigned long)ccb_tmp -(unsigned long)dma_addr;
		for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
			for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
				acb->devstate[i][j] = ARECA_RAID_GONE;
    			//intmask_org = arcmsr_disable_outbound_ints(acb);	

		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		
		struct MessageUnit_B *reg;
		void __iomem *mem_base0, *mem_base1;
		void *dma_coherent;
		dma_addr_t dma_coherent_handle, dma_addr;
		//uint32_t intmask_org;
		struct CommandControlBlock *ccb_tmp;
		int i, j;

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)		
			dma_coherent = dma_alloc_coherent(&pdev->dev,ARCMSR_MAX_FREECCB_NUM *sizeof (struct CommandControlBlock) + sizeof(struct MessageUnit_B) + 0x20, &dma_coherent_handle, GFP_KERNEL);
		#else
			dma_coherent = pci_alloc_consistent(pdev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) +	sizeof(struct MessageUnit_B) + 0x20, &dma_coherent_handle);
		#endif

		if (!dma_coherent) {
			printk(KERN_NOTICE "DMA allocation failed...........................\n");
			return -ENOMEM;
		}

		acb->dma_coherent = dma_coherent;
		acb->dma_coherent_handle = dma_coherent_handle;
		memset(dma_coherent, 0, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + sizeof(struct MessageUnit_B) + 0x20);

		if (((unsigned long)dma_coherent	& 0x1F)!=0) {/*ccb address must 32 (0x20) boundary*/
			dma_coherent = dma_coherent + (0x20 - ((unsigned long)dma_coherent & 0x1F));
			dma_coherent_handle = dma_coherent_handle + (0x20 - ((unsigned long)dma_coherent_handle & 0x1F));
		}

		dma_addr = dma_coherent_handle;
		ccb_tmp = (struct CommandControlBlock *)dma_coherent;

		for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
			ccb_tmp->cdb_shifted_phyaddr = dma_addr >> 5;
			ccb_tmp->acb = acb;
			acb->pccb_pool[i] = ccb_tmp;
			list_add_tail(&ccb_tmp->list, &acb->ccb_free_list);
			dma_addr = dma_addr + sizeof (struct CommandControlBlock);
			ccb_tmp++;
		}

		acb->vir2phy_offset = (unsigned long)ccb_tmp -(unsigned long)dma_addr;
		reg = (struct MessageUnit_B *)(dma_coherent + ARCMSR_MAX_FREECCB_NUM * sizeof (struct CommandControlBlock));
		acb->pmuB = reg;
		mem_base0 = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
		if (!mem_base0)
			goto out;
		mem_base1 = ioremap(pci_resource_start(pdev, 2), pci_resource_len(pdev, 2));
		if (!mem_base1) {
			iounmap(mem_base0);
			goto out;
		}
		reg->drv2iop_doorbell = mem_base0 + ARCMSR_DRV2IOP_DOORBELL;
		reg->drv2iop_doorbell_mask = mem_base0 + ARCMSR_DRV2IOP_DOORBELL_MASK;
		reg->iop2drv_doorbell = mem_base0 + ARCMSR_IOP2DRV_DOORBELL;
		reg->iop2drv_doorbell_mask = mem_base0 + ARCMSR_IOP2DRV_DOORBELL_MASK;
		reg->message_wbuffer = mem_base1 + ARCMSR_MESSAGE_WBUFFER;
		reg->message_rbuffer =  mem_base1 + ARCMSR_MESSAGE_RBUFFER;
		reg->msgcode_rwbuffer = mem_base1 + ARCMSR_MSGCODE_RWBUFFER;
		for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
			for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
				acb->devstate[i][j] = ARECA_RAID_GONE;
		    		//intmask_org = arcmsr_disable_outbound_ints(acb);	
		}
		break;
	}
	acb->adapter_index = arcmsr_adapterCnt;
	pHCBARC->acb[arcmsr_adapterCnt] = acb;
	arcmsr_adapterCnt++;
	return 0;
	
	out:
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0) 
		dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
	#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
		pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
	#else
		kfree(acb->dma_coherent);
	#endif
	return -ENOMEM;
}
/*
*********************************************************************
** modify bus reset sleep time:
**		echo "arcmsr sleeptime XXX" >/proc/scsi/arcmsr/0
**		XXX(secs): time for wait firmware ready while bus reset 
**		for example: echo "arcmsr sleeptime 120" >/proc/scsi/arcmsr/0
** modify wait firmware ready retry count:
**		echo "arcmsr retrycount XXX" >/proc/scsi/arcmsr/0
**		XXX: retry times for wait firmware ready  
**		for example: echo "arcmsr retrycount 12" >/proc/scsi/arcmsr/0
** modify max commands queue host adapter supported:
**		echo "arcmsr devqueue XXX" >/proc/scsi/arcmsr/0
** check pending command on Areca controller:
**		cat /proc/scsi/arcmsr/0
*********************************************************************
*/
#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
	static int arcmsr_set_info(char *buffer, int length, struct Scsi_Host *host) {
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *) host->hostdata;
		struct scsi_device *pstSDev, *pstTmp;
		char *p;
		int iTmp;
		unsigned long flags;

		printk("arcmsr_set_info: %s\n", buffer);

		if (!strncmp("arcmsr ", buffer, 7)) {
			printk("arcmsr_set_info: arcmsr\n");
			if (!strncmp("devqueue", buffer + 7, 8)) {
				p = buffer + 16;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify dev queue from %d to %d\n",host->cmd_per_lun, iTmp);

				host->cmd_per_lun = iTmp;
	            
				spin_lock_irqsave(acb->host->host_lock, flags);
				list_for_each_entry_safe(pstSDev, pstTmp, &host->__devices, siblings) {
					pstSDev->queue_depth = iTmp;
				}
				spin_unlock_irqrestore(host->host_lock, flags);
			}
			else if (!strncmp("hostqueue", buffer + 7, 9)) {
				p = buffer + 17;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify host queue from %d to %d\n", host->can_queue, iTmp);
	            		host->can_queue = iTmp;
			}
			else if (!strncmp("sleeptime", buffer + 7, 9)) {
				p = buffer + 17;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify sleep time after bus reset from %d to %d\n", sleeptime, iTmp);
				sleeptime = iTmp;
			}
			else if (!strncmp("retrycount", buffer + 7, 10)) {
				p = buffer + 18;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify retry value after bus reset from %d to %d\n", retrycount, iTmp);
				retrycount = iTmp;
			}
		}
		return (length);
	}
#else
	static int arcmsr_set_info(char *buffer,int length) {
		#if ARCMSR_DEBUG
		printk("arcmsr_set_info.............\n");
		#endif
		return 0;
	}
#endif
/*
*********************************************************************
*********************************************************************
*/
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb)
{
       struct pci_dev *pdev;
	struct CommandControlBlock *ccb;
	struct HCBARC *pHCBARC = &arcmsr_host_control_block;
	struct Scsi_Host *host;
	int i = 0,poll_count = 0;
    	#if	ARCMSR_DEBUG
    		printk("arcmsr_pcidev_disattach.................. \n");
    	#endif
	/* disable iop all outbound interrupt */
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		flush_scheduled_work();
		del_timer_sync(&acb->eternal_timer);
	#endif
    	arcmsr_disable_outbound_ints(acb);
	arcmsr_stop_adapter_bgrb(acb);
	arcmsr_flush_adapter_cache(acb);
	acb->acb_flags |= ACB_F_SCSISTOPADAPTER;
	acb->acb_flags &= ~ACB_F_IOP_INITED;
	if(atomic_read(&acb->ccboutstandingcount)!= 0) {  
		while(atomic_read(&acb->ccboutstandingcount)!= 0 && (poll_count < 6)) {
	    		arcmsr_interrupt(acb);
	   		poll_count++;
		}
		if(atomic_read(&acb->ccboutstandingcount)!= 0) {  
			arcmsr_abort_allcmd(acb);
			arcmsr_done4abort_postqueue(acb);
			for(i = 0;i<ARCMSR_MAX_FREECCB_NUM;i++) {
				ccb = acb->pccb_pool[i];
				if(ccb->startdone == ARCMSR_CCB_START) {
					ccb->startdone = ARCMSR_CCB_ABORTED;
				}
			}
		}
	}

	host = acb->host;
    	pdev = acb->pdev;
    	arcmsr_free_ccb_pool(acb);
	pHCBARC->acb[acb->adapter_index] = NULL; /* clear record */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		scsi_remove_host(host);
		scsi_host_put(host);
	#else
		scsi_unregister(host);
	#endif
		free_irq(pdev->irq,acb);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
		pci_release_regions(pdev);
		pci_disable_device(pdev);
		pci_set_drvdata(pdev, NULL);
	#endif
}
/*
***************************************************************
***************************************************************
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
	static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf)
	{
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		struct Scsi_Host *host;
		int i;

		#if ARCMSR_DEBUG
			printk("arcmsr_halt_notify.............................\n");
		#endif
		if((event != SYS_RESTART) && (event != SYS_HALT) && (event != SYS_POWER_OFF))
		{
			return NOTIFY_DONE;
		}
		for(i = 0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			acb = pHCBARC->acb[i];
			if(acb == NULL) 
			{
				continue;
			}
			/* Flush cache to disk */
			/* Free	irq,otherwise extra interrupt is generated	 */
			/* Issue a blocking(interrupts disabled) command to the	card */
			host = acb->host;
			arcmsr_pcidev_disattach(acb);
		}
		unregister_reboot_notifier(&arcmsr_event_notifier);
		return NOTIFY_OK;
	}
#endif
/*
*********************************************************************
*********************************************************************
*/
#undef	SPRINTF
#define	SPRINTF(args...) pos +=sprintf(pos,## args)
#define	YESNO(YN)\
if(YN) SPRINTF(" Yes ");\
else SPRINTF(" No ")
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
#else
	int arcmsr_proc_info(char *buffer, char ** start, off_t offset, int length, int hostno, int inout)
#endif
{
	uint8_t i;
	char * pos = buffer;
	struct AdapterControlBlock *acb;
	struct HCBARC *pHCBARC = &arcmsr_host_control_block;

	#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		struct scsi_device *pstSDev, *pstTmp;
		unsigned long flags;
	#endif
	
	#if ARCMSR_DEBUG
		printk("arcmsr_proc_info.............\n");
	#endif
	
   	if(inout) {
		#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
			return(arcmsr_set_info(buffer, length, host));
		#else
			return(arcmsr_set_info(buffer, length));
		#endif
	}
	
	for(i = 0;i<ARCMSR_MAX_ADAPTER;i++) {
		acb = pHCBARC->acb[i];
	    if(!acb) 
			continue;
		SPRINTF("ARECA SATA/SAS RAID Mass Storage Host Adadpter \n");
		SPRINTF("Driver Version %s ",ARCMSR_DRIVER_VERSION);
		SPRINTF("IRQ%d \n",acb->pdev->irq);
		SPRINTF("===========================\n"); 
	}

	#if (defined(ARCMSR_PP_RESET) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
		SPRINTF("**********************************\n"); 
		SPRINTF("host busy %d, failed %d, queue %d\n", host->host_busy, host->host_failed, host->can_queue);
        	printk("host busy %d, failed %d, queue %d\n", host->host_busy, host->host_failed, host->can_queue);
		spin_lock_irqsave(host->host_lock, flags);
		list_for_each_entry_safe(pstSDev, pstTmp, &host->__devices, siblings) {
		SPRINTF("device %d:%d:%d  busy %d, queue depth %d, blocked %d\n", 
				pstSDev->id, pstSDev->lun, pstSDev->channel,
				pstSDev->device_busy, pstSDev->queue_depth,
				pstSDev->device_blocked);
		printk("device %d:%d:%d  busy %d, queue depth %d, blocked %d\n", 
				pstSDev->id, pstSDev->lun, pstSDev->channel,
				pstSDev->device_busy, pstSDev->queue_depth,
				pstSDev->device_blocked);
		}
		spin_unlock_irqrestore(host->host_lock, flags);
	#endif

	*start = buffer + offset;
	if(pos - buffer < offset) {
	    return 0;
	}
	else if(pos - buffer - offset < length) {
	    return (pos - buffer - offset);
	}
	else {
	    return length;
	}
}
/*
************************************************************************
************************************************************************
*/
int arcmsr_release(struct Scsi_Host *host)
{
	struct AdapterControlBlock *acb;
	struct HCBARC *pHCBARC = &arcmsr_host_control_block;
    	uint8_t match = 0xff, i;

	#if ARCMSR_DEBUG
		printk("arcmsr_release...........................\n");
	#endif
	if(!host) {
	return -ENXIO;
	}
	acb = (struct AdapterControlBlock *)host->hostdata;
	if(!acb) {
	return -ENXIO;
	}
	for(i = 0;i<ARCMSR_MAX_ADAPTER;i++) {
		if(pHCBARC->acb[i] == acb) 	{  
			match = i;
		}
	}
	if(match == 0xff) {
		return -ENODEV;
	}
	arcmsr_pcidev_disattach(acb);
	for(i = 0;i<ARCMSR_MAX_ADAPTER;i++) {
		if(pHCBARC->acb[i]) { 
			return 0;
		}
	}
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
#ifdef CONFIG_SCSI_ARCMSR_AER

      	static pci_ers_result_t arcmsr_pci_slot_reset(struct pci_dev *pdev)
      	{
		struct Scsi_Host *host = pci_get_drvdata(pdev);
        		struct AdapterControlBlock *acb =
              	(struct AdapterControlBlock *) host->hostdata;
		uint32_t intmask_org;
		int i, j;
		#if ARCMSR_DEBUG
			printk("arcmsr_pci_slot_reset............................\n");
		#endif

		if (pci_enable_device(pdev)) {
			return PCI_ERS_RESULT_DISCONNECT;
		}
		pci_set_master(pdev);
      		intmask_org = arcmsr_disable_outbound_ints(acb);
		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
		acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;

		for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
			for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
				acb->devstate[i][j] = ARECA_RAID_GONE;

		arcmsr_wait_firmware_ready(acb);
		arcmsr_iop_confirm(acb);
       		/* disable all outbound interrupt */
		arcmsr_get_firmware_spec(acb,1);
		/*start background rebuild*/
		arcmsr_start_adapter_bgrb(acb);
		/* empty doorbell Qbuffer if door bell ringed */
		arcmsr_clear_doorbell_queue_buffer(acb);
		arcmsr_enable_eoi_mode(acb);
		/* enable outbound Post Queue,outbound doorbell Interrupt */
		arcmsr_enable_outbound_ints(acb,intmask_org);
		acb->acb_flags |= ACB_F_IOP_INITED;
		pci_enable_pcie_error_reporting(pdev);
 		return PCI_ERS_RESULT_RECOVERED;

	}
      
      	static void arcmsr_pci_ers_need_reset_forepart(struct pci_dev *pdev)
     	{
     		struct Scsi_Host *host = pci_get_drvdata(pdev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
		struct CommandControlBlock *ccb;
		uint32_t intmask_org;
		int i = 0;

		#if ARCMSR_DEBUG
			printk("arcmsr_pci_ers_need_reset_forepart............................\n");
		#endif

		if (atomic_read(&acb->ccboutstandingcount) != 0) {

			/* disable all outbound interrupt */
			intmask_org = arcmsr_disable_outbound_ints(acb);
			/* talk to iop 331 outstanding command aborted */
			arcmsr_abort_allcmd(acb);
			/* wait for 3 sec for all command aborted*/
			arc_mdelay_int(3000);
			/* clear all outbound posted Q */
			arcmsr_done4abort_postqueue(acb);
			for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
				ccb = acb->pccb_pool[i];
				if (ccb->startdone == ARCMSR_CCB_START) {
					ccb->startdone = ARCMSR_CCB_ABORTED;
					arcmsr_ccb_complete(ccb,1);
				}
			}		
			/* enable all outbound interrupt */
			arcmsr_enable_outbound_ints(acb, intmask_org);
		}
		pci_disable_device(pdev);
	}   

      
	static void arcmsr_pci_ers_disconnect_forepart(struct pci_dev *pdev)
      	{

			struct Scsi_Host *host = pci_get_drvdata(pdev);
			struct AdapterControlBlock *acb	= (struct AdapterControlBlock *)host->hostdata;
			
			#if ARCMSR_DEBUG
				printk("arcmsr_pci_ers_disconnect_forepart............................\n");
			#endif
	
			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);

	}
      
      	static pci_ers_result_t arcmsr_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
      	{
		#if ARCMSR_DEBUG
			printk("arcmsr_pci_error_detected............................\n");
		#endif
		
         	switch (state){
               		case pci_channel_io_frozen:
	                       	 arcmsr_pci_ers_need_reset_forepart(pdev);
        	               	 return PCI_ERS_RESULT_NEED_RESET;
               		case pci_channel_io_perm_failure:
                	       	 arcmsr_pci_ers_disconnect_forepart(pdev);
                       		 return PCI_ERS_RESULT_DISCONNECT;
                       		 break;
               		default:
                       		 return PCI_ERS_RESULT_NEED_RESET;
            }
	}
#endif
#endif

