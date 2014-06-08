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
**								remove some unused function
**								--.--.0X.-- is  for old style kernel compatibility
** 1.20.0X.08 	06/23/2005	Erich Chen			
**								1.bug fix with abort command, in case of heavy loading 
**								when sata cable working on low quality connection
** 1.20.0X.09	9/12/2005	Erich Chen			
**								1.bug fix with abort command handling,
**								and firmware version check and FW update notify 
**								for HW bug fix
** 1.20.0X.10	9/23/2005	Erich Chen			
**								1.enhance sysfs function for change driver's max tag Q number.
**								2.add DMA_64BIT_MASK for backward compatible with all 2.6.x
**								3.add some useful message for abort command
**								4.add ioctl code 'ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE'
**								customer can send this command for sync raid volume data
** 1.20.0X.11	9/29/2005	Erich Chen
**								1.by comment of Arjan van de Ven fix incorrect msleep redefine
**								2.cast off sizeof(dma_addr_t) condition for 64bit pci_set_dma_mask
** 1.20.0X.12	9/30/2005	Erich Chen
**								1.bug fix with 64bit platform's ccbs using 
**								if over 4G system memory
**								2.change 64bit pci_set_consistent_dma_mask into 32bit
**								3.increcct adapter count if adapter initialize fail.
**								4.miss-edit psge += sizeof(struct SG64ENTRY *) as 
**								psge += sizeof(struct SG64ENTRY) at arcmsr_build_ccb
**								64 bits sg entry would be incorrectly calculated
**								thanks to Kornel Wieliczek's great help
** 1.20.0X.13	11/15/2005	Erich Chen
**								1.scheduling pending ccb with 'first in first out'
**								new firmware update notify
** 1.20.0X.13	11/07/2006	Erich Chen
**								1.remove #include config.h and devfs_fs_kernel.h
**								2.enlarge the timeout duration for each scsi command 
**								it could aviod the vibration factor with sata disk
**								on some bad machine
** 1.20.0X.14	05/02/2007	Erich Chen & Nick Cheng
**								1.add PCI-Express error recovery function and AER capability
**								2.add the selection of ARCMSR_MAX_XFER_SECTORS_B=4096 
**								if firmware version is newer than 1.41
**								3.modify arcmsr_iop_reset to improve the stability
**								4.delect arcmsr_modify_timeout routine 
**								because it would malfunction as removal and recovery the lun
**								if somebody needs to adjust the scsi command timeout value, 
**								the script could be available on Areca FTP site 
**								or contact Areca support team 
**								5.modify the ISR, arcmsr_interrupt routine, 
**								to prevent the inconsistent with sg_mod driver 
**								if application directly calls the arcmsr driver 
**								w/o passing through scsi mid layer		
**								6.delect the callback function, arcmsr_ioctl
**
**
** 1.20.0X.15	23/08/2007	Erich Chen & Nick Cheng
**								1.support ARC1200/1201/1202
** 1.20.0X.15	01/10/2007	Erich Chen & Nick Cheng
**								1.add arcmsr_enable_eoi_mode()
** 1.20.0X.15	04/12/2007	Erich Chen & Nick Cheng		
**								1.delete the limit of if dev_aborts[id][lun]>1, then
**								acb->devstate[id][lun] = ARECA_RAID_GONE in arcmsr_abort()
**								to wait for OS recovery on delicate HW
**								2.modify arcmsr_drain_donequeue() to ignore unknown command
**								and let kernel process command timeout.
**								This could handle IO request violating max. segments 
**								while Linux XFS over DM-CRYPT. 
**								Thanks to Milan Broz's comments <mbroz@redhat.com>
**
** 1.20.0X.15	24/12/2007	Erich Chen & Nick Cheng
**								1.fix the portability problems
**								2.fix type B where we should _not_ iounmap() acb->pmu;
**								it's not ioremapped.
**								3.add return -ENOMEM if ioremap() fails
**								4.transfer IS_SG64_ADDR w/ cpu_to_le32() in arcmsr_build_ccb
**								5.modify acb->devstate[i][j] as ARECA_RAID_GONE 
**								instead of ARECA_RAID_GOOD in arcmsr_alloc_ccb_pool()
**								6.fix arcmsr_cdb->Context as (unsigned long)arcmsr_cdb
**								7.add the checking state of (outbound_intstatus &
**								ARCMSR_MU_OUTBOUND_HANDLE_INT) == 0 in arcmsr_handle_hba_isr
**								8.replace pci_alloc_consistent()/pci_free_consistent() 
**								with kmalloc()/kfree() in arcmsr_iop_message_xfer()
**								9. fix the release of dma memory for type B in arcmsr_free_ccb_pool()
**								10.fix the arcmsr_polling_hbb_ccbdone()
** 1.20.0X.15	27/02/2008	Erich Chen & Nick Cheng		
**								1.arcmsr_iop_message_xfer() is called from 
**								atomic context under the queuecommand scsi_host_template
**								handler. James Bottomley pointed out that the current
**								GFP_KERNEL|GFP_DMA flags are wrong: firstly we are in
**								atomic context, secondly this memory is not used for DMA.
**								Also removed some unneeded casts.
**								Thanks to Daniel Drake <dsd@gentoo.org>
** 1.20.0X.15	07/04/2008	Erich Chen & Nick Cheng
**								1. add the function of HW reset for Type_A for kernel version greater than 2.6.0
**								2. add the function to automatic scan as the volume added or delected for kernel version greater than 2.6.0
**								3. support the notification of the FW status to the AP layer for kernel version greater than 2.6.0
** 1.20.0X.15	03/06/2008	Erich Chen & Nick Cheng					
** 								1. support SG-related functions after kernel-2.6.2x
** 1.20.0X.15	03/11/2008	Nick Cheng
**								1. fix the syntax error
** 								2. compatible to kernel-2.6.26
** 1.20.0X.15	06/05/2009	Erich Chen & Nick Cheng		1. improve SCSI EH mechanism for ARC-1680 series
** 1.20.0X.15	02/06/2009	Erich Chen & Nick Cheng		1. fix cli access unavailably issue on ARC-1200/1201 while a certain HD is unstable
** 1.20.0X.15	05/06/2009	Erich Chen & Nick Cheng		1. suport the maximum transfer size to 4M 
** 1.20.0X.15	09/12/2009	Erich Chen & Nick Cheng		1. change the "for loop" for manipulating sg list to scsi_for_each_sg. There is 127 entries per sg list. 
**								If the sg list entry is larget than 127, it will allocate the rest of entries in the other sg list on other page.
**								The old for-loop type could hit the bugs if the second sg list is not ine the subsequent page.
** 1.20.0X.15	05/10/2010	Nick Cheng			1. support ARC-1880 series on kernel 2.6.X
** 1.20.0X.15	07/27/2010	Nick Cheng			1. fix the system lock-up on Intel Xeon 55XX series, x86_64
** 1.20.0X.15	07/29/2010	Nick Cheng			1. revise the codes for scatter-gather applicable to RHEL5.0 to RHEL5.3
** 1.20.0X.15	10/08/2010	Nick Cheng			1. fix DMA memory allocation failure in Xen
**								2. use command->sc_data_direction instead of trying (incorrectly) to
**								figure it out from the command itself in arcsas_build_ccb()
** 1.20.0X.15	03/30/2011	Nick Cheng			1. increase the timeout value for every kind of scsi commands during driver modules installation if needed it
** 1.20.0X.15	06/22/2011	Nick Cheng			1. change debug print
******************************************************************************************
*/
#define ARCMSR_DEBUG_EXTDFW		0
#define ARCMSR_DEBUG_FUNCTION		0
#define ARCMSR_DEBUG_SP		1
#define ARCMSR_DEBUG_TTV		0
#define ARCMSR_PP_RESET			1	/*the machanism to increase or not the waiting period for scsi device reset*/
#define ARCMSR_FW_POLLING		1	/*the machanism to poll the FW status and discover the hot-plug and hot-remove scsi device*/
/*****************************************************************************************/
#if defined __KERNEL__
	#include <linux/version.h>
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif
	/* modversions.h should be before should be before module.h */
    	#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
		#if defined( MODVERSIONS )
			#include <config/modversions.h>
		#endif
    	#endif

	#if(ARCMSR_PP_RESET && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	#define ARCMSR_SLEEPTIME	10
	#endif
	#define ARCMSR_RETRYCOUNT	12
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
	#include <linux/stddef.h>
	#include <linux/nmi.h>
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 5, 0)
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

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 3, 30)
		# include <linux/spinlock.h>
	#else
		# include <asm/spinlock.h>
	#endif	/* 2,3,30 */

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 5, 0)
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

MODULE_AUTHOR("Nick Cheng<support@areca.com.tw>");
MODULE_DESCRIPTION("Areca (ARC11xx/12xx/16xx/18XX) SATA/SAS RAID Controller Driver");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	MODULE_VERSION(ARCMSR_DRIVER_VERSION);
#endif

#ifdef MODULE_LICENSE
	MODULE_LICENSE("Dual BSD/GPL");
#endif
/*
**********************************************************************************
**********************************************************************************
*/
static u8 arcmsr_adapterNo = 0;
static struct HCBARC arcmsr_host_control_block;
wait_queue_head_t wait_q;
static int timeout = 0;
module_param(timeout, int, 0644);
MODULE_PARM_DESC(timeout, "scsi cmd timeout value(0 - 120)");
static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb);
static void arcmsr_iop_init(struct AdapterControlBlock *acb);
static int arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb);
static void arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb);
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock *acb);
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb);
static bool arcmsr_get_firmware_spec(struct AdapterControlBlock *acb);
static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb);
static bool arcmsr_define_adapter_type(struct AdapterControlBlock *acb);
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_request_device_map(unsigned long pacb);
	static void arcmsr_request_hba_device_map(struct AdapterControlBlock *acb);
	static void arcmsr_request_hbb_device_map(struct AdapterControlBlock *acb);
	static void arcmsr_request_hbc_device_map(struct AdapterControlBlock *acb);
	static void arcmsr_hbc_message_isr(struct AdapterControlBlock *acb);
#endif
static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb);
static void arcmsr_enable_outbound_ints(struct AdapterControlBlock *acb, u32 orig_mask);
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20))
	static void arcmsr_message_isr_bh_fn(struct work_struct *work);
#elif (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_message_isr_bh_fn(void *pacb);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
	#define	arcmsr_detect NULL
    	static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb);
    	static int arcmsr_probe(struct pci_dev *pdev,const struct pci_device_id *id);
	static void arcmsr_remove(struct pci_dev *pdev);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
		static void arcmsr_shutdown(struct pci_dev *pdev);
	#endif
#else
    	static void arcmsr_interrupt(struct AdapterControlBlock *acb);
    	int arcmsr_schedule_command(struct scsi_cmnd *pcmd);
    	int arcmsr_detect(Scsi_Host_Template *);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
    	static int arcmsr_halt_notify(struct notifier_block *nb, unsigned long event, void *buf);
	static struct notifier_block arcmsr_event_notifier = {arcmsr_halt_notify, NULL, 0};
#endif
/*
**********************************************************************************
**********************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static void arcmsr_free_mu(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_C:
			break;
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B *reg = acb->pmuB;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) 
				dma_free_coherent(&acb->pdev->dev, sizeof(struct MessageUnit_B), reg, acb->dma_coherent_handle_hbb_mu);
			#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
				pci_free_consistent(acb->pdev, sizeof(struct MessageUnit_B), reg, acb->dma_coherent_handle_hbb_mu);
			#else
				kfree(reg);
			#endif
		}
	}
}
#endif
/*
****************************************************************************
****************************************************************************
*/
static bool arcmsr_remap_pciregion(struct AdapterControlBlock *acb)
{
	struct pci_dev *pdev = acb->pdev;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A: {
			acb->pmuA = ioremap(pci_resource_start(pdev,0), pci_resource_len(pdev,0));
			if (!acb->pmuA) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", arcmsr_adapterNo);
				return false;
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			void __iomem *mem_base0, *mem_base1;
			mem_base0 = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
			if (!mem_base0) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", arcmsr_adapterNo);
				return false;
			}
			mem_base1 = ioremap(pci_resource_start(pdev, 2), pci_resource_len(pdev, 2));
			if (!mem_base1) {
				iounmap(mem_base0);
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", arcmsr_adapterNo);
				return false;
			}
			acb->mem_base0 = mem_base0;
			acb->mem_base1 = mem_base1;
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			acb->pmuC = ioremap(pci_resource_start(pdev, 1), pci_resource_len(pdev, 1));
			if (!acb->pmuC) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
				return false;
			}
			if (readl(&acb->pmuC->outbound_doorbell) & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
				writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &acb->pmuC->outbound_doorbell_clear);/*clear interrupt*/
				return true;
			}
			break;
		}
	}
	return true;
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_unmap_pciregion(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			iounmap(acb->pmuA);
		}
		break;
		case ACB_ADAPTER_TYPE_B: {
			iounmap(acb->mem_base0);
			iounmap(acb->mem_base1);
		}
		break;
		case ACB_ADAPTER_TYPE_C: {
			iounmap(acb->pmuC);
		}	
	}
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_wait_firmware_ready(struct AdapterControlBlock *acb)
{
	uint32_t firmware_state = 0;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			do {
				firmware_state = readl(&reg->outbound_msgaddr1);
			} while ((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B *reg = acb->pmuB;
			do {
				firmware_state = readl(reg->iop2drv_doorbell);
			} while ((firmware_state & ARCMSR_MESSAGE_FIRMWARE_OK) == 0);
			writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
			do {
				firmware_state = readl(&reg->outbound_msgaddr1);
			} while ((firmware_state & ARCMSR_HBCMU_MESSAGE_FIRMWARE_OK) == 0);
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/
static bool arcmsr_hba_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	int i;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	for (i = 0; i < 2000; i++) {
		if (readl(&reg->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) {
			writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}
/*
**********************************************************************
**********************************************************************
*/
static bool arcmsr_hbb_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	int i;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	for (i = 0; i < 2000; i++) {
		if (readl(reg->iop2drv_doorbell) & ARCMSR_IOP2DRV_MESSAGE_CMD_DONE) {
			writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);
			writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}
/*
**********************************************************************
**********************************************************************
*/
static bool arcmsr_hbc_wait_msgint_ready(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C *phbcmu = (struct MessageUnit_C *)pACB->pmuC;
	int i;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	for (i = 0; i < 2000; i++) {
		if (readl(&phbcmu->outbound_doorbell) & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
			writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &phbcmu->outbound_doorbell_clear); /*clear interrupt*/
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}
/*
**********************************************************************************
**********************************************************************************
*/
static bool arcmsr_hbb_enable_driver_mode(struct AdapterControlBlock *pacb)
{
	struct MessageUnit_B *reg = pacb->pmuB;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	writel(ARCMSR_MESSAGE_START_DRIVER_MODE, reg->drv2iop_doorbell);
	if (!arcmsr_hbb_wait_msgint_ready(pacb)) {
		printk(KERN_ERR "arcmsr%d: can't set driver mode. \n", pacb->host->host_no);
		return false;
	}
    	return true;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_touch_nmi_watchdog(void)
{
	#ifdef CONFIG_X86_64
		touch_nmi_watchdog();
	#endif /* CONFIG_X86_64*/

	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13)
		touch_softlockup_watchdog();
	#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13) */
}
/*
**********************************************************************************
**
**********************************************************************************
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 3, 30)
	struct proc_dir_entry arcmsr_proc_scsi=
	{
		PROC_SCSI_ARCMSR,
		8,
		"arcmsr",
		S_IFDIR	| S_IRUGO | S_IXUGO,
		2
	};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
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
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1680)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1681)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1880)},
		{0, 0},	/* Terminating entry */
	};
	MODULE_DEVICE_TABLE(pci, arcmsr_device_id_table);
	struct pci_driver arcmsr_pci_driver = 
	{
		.name		      				= "arcmsr",
		.id_table						= arcmsr_device_id_table,
		.probe		      				= arcmsr_probe,
		.remove	     	      				= arcmsr_remove,
        		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	    		.shutdown				= arcmsr_shutdown,
	    	#endif
	};
	/*
	*********************************************************************
	*********************************************************************
	*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
		static irqreturn_t arcmsr_do_interrupt(int irq,void *dev_id)
		{
			irqreturn_t handle_state;
		    	struct AdapterControlBlock *acb;

			acb = (struct AdapterControlBlock *)dev_id;
			handle_state = arcmsr_interrupt(acb);
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
			//unsigned long flags;
			int i = 0;

			acb = (struct AdapterControlBlock *)dev_id;
			acbtmp = pHCBARC->acb[i];
			while ((acb != acbtmp) && acbtmp && (i <ARCMSR_MAX_ADAPTER) ) {
				i++;
				acbtmp = pHCBARC->acb[i];
			}
			if (!acbtmp) {
				#if ARCMSR_DEBUG_FUNCTION
					printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
		    	    	#endif
		    	    	return IRQ_NONE;
			}
			//spin_lock_irqsave(&acb->host_lock, flags);
			handle_state = arcmsr_interrupt(acb);
			//spin_unlock_irqrestore(&acb->host_lock, flags);
			return handle_state;
		}
	#endif
	/*
	**********************************************************************************
	**********************************************************************************
	*/
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20))
		static void arcmsr_message_isr_bh_fn(struct work_struct *work) 
		{
			struct AdapterControlBlock *acb = container_of(work,struct AdapterControlBlock, arcmsr_do_message_isr_bh);
			#if ARCMSR_DEBUG_FUNCTION
				printk("%s:\n", __func__);
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
						for (target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
							diff = (*acb_dev_map)^readb(devicemap);
							if (diff != 0) {
								char temp;
								*acb_dev_map = readb(devicemap);
								temp =*acb_dev_map;
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01)==1 && (diff & 0x01) == 1) {	
											#if ARCMSR_DEBUG_FUNCTION
												printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
											#endif
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL ) {
											#if ARCMSR_DEBUG_FUNCTION
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
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01)==1 && (diff & 0x01) == 1) {	
											#if ARCMSR_DEBUG_FUNCTION
												printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
											#endif
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL ) {
											#if ARCMSR_DEBUG_FUNCTION
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
					//spin_unlock_irqrestore(&acb->host_lock, flags);
					break;
				}
				case ACB_ADAPTER_TYPE_C: {
					struct MessageUnit_C *reg  = acb->pmuC;
					char *acb_dev_map = (char *)acb->device_map;
					uint32_t __iomem *signature = (uint32_t __iomem *)(&reg->msgcode_rwbuffer[0]);
					char __iomem *devicemap = (char __iomem *)(&reg->msgcode_rwbuffer[21]);
					int target, lun;
					struct scsi_device *psdev;
					char diff;

					atomic_inc(&acb->rq_map_token);
					if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
						for (target = 0; target < ARCMSR_MAX_TARGETID - 1; target++) {
							diff = (*acb_dev_map)^readb(devicemap);
							if (diff != 0) {
								char temp;
								*acb_dev_map = readb(devicemap);
								temp = *acb_dev_map;
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01) == 1 && (diff & 0x01) == 1) {
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL) {
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
	#elif (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))

		static void arcmsr_message_isr_bh_fn(void *pacb) 
		{
			struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;
			#if ARCMSR_DEBUG_FUNCTION
				printk("%s:\n", __func__);
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
					//unsigned long flags;
					
					//spin_lock_irqsave(&acb->host_lock, flags);
					atomic_inc(&acb->rq_map_token);
					if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
						for (target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
							diff = (*acb_dev_map) ^ readb(devicemap);
							if (diff != 0) {
								char temp;
								*acb_dev_map = readb(devicemap);
								temp = *acb_dev_map;
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01)==1 && (diff & 0x01) == 1) {	
											#if ARCMSR_DEBUG_FUNCTION
												printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
											#endif
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL ) {
											#if ARCMSR_DEBUG_FUNCTION
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
					//spin_unlock_irqrestore(&acb->host_lock, flags);
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
					//unsigned long flags;

					//spin_lock_irqsave(&acb->host_lock, flags);
					atomic_inc(&acb->rq_map_token);
					if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
						for(target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
							diff = (*acb_dev_map) ^ readb(devicemap);
							if (diff != 0) {
								char temp;
								*acb_dev_map = readb(devicemap);
								temp = *acb_dev_map;
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01) == 1 && (diff & 0x01) == 1) {	
											#if ARCMSR_DEBUG_FUNCTION
												printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
											#endif
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL ) {
											#if ARCMSR_DEBUG_FUNCTION
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
					//spin_unlock_irqrestore(&acb->host_lock, flags);
					break;
					}	
				case ACB_ADAPTER_TYPE_C: {

					struct MessageUnit_C __iomem *reg  = acb->pmuC;
					char *acb_dev_map = (char *)acb->device_map;
					uint32_t __iomem *signature = (uint32_t __iomem*) (&reg->msgcode_rwbuffer[0]);
					char __iomem *devicemap = (char __iomem*) (&reg->msgcode_rwbuffer[21]);
					int target, lun;
					struct scsi_device *psdev;
					char diff;
					//unsigned long flags;

					//spin_lock_irqsave(&acb->host_lock, flags);
					atomic_inc(&acb->rq_map_token);
					if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
						for (target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
							diff = (*acb_dev_map) ^ readb(devicemap);
							if (diff != 0) {
								char temp;
								*acb_dev_map = readb(devicemap);
								temp = *acb_dev_map;
								for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
									if ((temp & 0x01) == 1 && (diff & 0x01) == 1) {	
											#if ARCMSR_DEBUG_FUNCTION
												printk("ADD A SCSI DEVICE, TARGET_ID=%d, LUN=%d.............\n", target, lun);
											#endif
										scsi_add_device(acb->host, 0, target, lun);
									} else if ((temp & 0x01) == 0 && (diff & 0x01) == 1) {
										psdev = scsi_device_lookup(acb->host, 0, target, lun);
										if (psdev != NULL ) {
											#if ARCMSR_DEBUG_FUNCTION
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
					//spin_unlock_irqrestore(&acb->host_lock, flags);
					break;
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
		int heads, sectors, cylinders;
		heads = 255;
		sectors = 56;
		cylinders = sector_div(capacity, heads * sectors);
		geom[0] = heads;
		geom[1] = sectors;
		geom[2] = cylinders;
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_probe(struct pci_dev *pdev, const struct pci_device_id *id)
	{
		struct Scsi_Host *host;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		uint8_t bus,dev_fun;
		int error;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		pHCBARC->adapterCnt = arcmsr_adapterNo + 1;
		if (pci_enable_device(pdev)) {
			printk("arcmsr%d: adapter probe: pci_enable_device error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			return -ENODEV;
		}
		if ((host = scsi_host_alloc(&arcmsr_scsi_host_template, sizeof(struct AdapterControlBlock))) == 0) {
			printk("arcmsr%d: adapter probe: scsi_host_alloc error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
	    		return -ENODEV;
		}
		error = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
		if (error) {
			error = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
			if (error) {
				printk(KERN_WARNING
				       "scsi%d: No suitable DMA mask available\n",
				       host->host_no);
				goto scsi_host_release;
			}
		}
		init_waitqueue_head(&wait_q);
		bus = pdev->bus->number;
		dev_fun = pdev->devfn;
		acb = (struct AdapterControlBlock *) host->hostdata;
		memset(acb,0,sizeof(struct AdapterControlBlock));
		acb->pdev = pdev;
		acb->host = host;
		acb->adapter_index = arcmsr_adapterNo;	
		pHCBARC->acb[arcmsr_adapterNo] = acb;
		host->max_lun = ARCMSR_MAX_TARGETLUN;
		host->max_id = ARCMSR_MAX_TARGETID;		/*16:8*/
		host->max_cmd_len = 16;	 			/*this is issue of 64bit LBA ,over 2T byte*/
		host->can_queue = ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
		host->cmd_per_lun = ARCMSR_MAX_CMD_PERLUN;	    
		host->this_id = ARCMSR_SCSI_INITIATOR_ID;
		host->unique_id = (bus << 8) | dev_fun;
		/**
 		* pci_set_master - enables bus-mastering for device dev
 		* @dev: the PCI device to enable
 		* Enables bus-mastering on the device and calls pcibios_set_master() to do the needed arch specific settings.
		*/
		pci_set_drvdata(pdev, host);
		pci_set_master(pdev);
		if (pci_request_regions(pdev, "arcmsr")) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: pci_request_regions failed \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto pci_disable_dev;
		}
		spin_lock_init(&acb->eh_lock);
		spin_lock_init(&acb->ccblist_lock);
		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
		acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
		INIT_LIST_HEAD(&acb->ccb_free_list);
		if (!arcmsr_define_adapter_type(acb)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: arcmsr_remap_pciregion got error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto pci_release_regs;
		}
		if (!arcmsr_remap_pciregion(acb)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: arcmsr_remap_pciregion got error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto pci_release_regs;
		}

		if (!arcmsr_get_firmware_spec(acb)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: arcmsr_get_hbb_config got error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto unmap_pci_region;
		}
		if (arcmsr_alloc_ccb_pool(acb)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: arcmsr_alloc_ccb_pool got error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto free_mu;
		}
		if (scsi_add_host(host, &pdev->dev)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: scsi_add_host got error \n", arcmsr_adapterNo);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto RAID_controller_stop;
		}
		arcmsr_iop_init(acb);
		if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", acb)) {
			pHCBARC->acb[arcmsr_adapterNo] = NULL;
			printk("arcmsr%d: request_irq =%d failed!\n", arcmsr_adapterNo, pdev->irq);
			pHCBARC->adapterCnt = arcmsr_adapterNo -1;
			goto scsi_host_remove;
		}
		host->irq = pdev->irq;
		scsi_scan_host(host);
		arcmsr_adapterNo++;
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
				INIT_WORK(&acb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn);
			#else
				INIT_WORK(&acb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn, acb);
			#endif
			atomic_set(&acb->rq_map_token, 16);
			atomic_set(&acb->ante_token_value, 16);
			acb->fw_flag = FW_NORMAL;
			init_timer(&acb->eternal_timer);
			acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6 * HZ);
			acb->eternal_timer.data = (unsigned long) acb;
			acb->eternal_timer.function = &arcmsr_request_device_map;
			add_timer(&acb->eternal_timer);
		#endif
		return 0;
		scsi_host_remove:
			scsi_remove_host(host);
		RAID_controller_stop:
			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);
			arcmsr_free_ccb_pool(acb);
 		free_mu:
			arcmsr_free_mu(acb);
		unmap_pci_region:
			arcmsr_unmap_pciregion(acb);
		pci_release_regs:
			pci_release_regions(pdev);
		scsi_host_release:
			scsi_host_put(host);	
		pci_disable_dev:
			pci_disable_device(pdev);
		return -ENODEV;
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

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		arcmsr_pcidev_disattach(acb);
		/*if this is last acb */
		for (i = 0; i < ARCMSR_MAX_ADAPTER; i++) {
			if (pHCBARC->acb[i]) { 
				return;/* this is not last adapter's release */
			}
		}

		#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
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
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		/* 
		** register as a PCI hot-plug driver module 
		*/
		memset(pHCBARC, 0, sizeof(struct HCBARC));
		error = pci_register_driver(&arcmsr_pci_driver);
		if (pHCBARC->acb[0]) {
			host_template->proc_name="arcmsr";
			#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
				register_reboot_notifier(&arcmsr_event_notifier);
			#endif
		}
		return error;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
		static void arcmsr_shutdown(struct pci_dev *pdev)
		{
			struct Scsi_Host *host = pci_get_drvdata(pdev);
			struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
			
			#if ARCMSR_DEBUG_FUNCTION
				printk("%s:\n", __func__);
			#endif
			arcmsr_disable_outbound_ints(acb);

			#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
				del_timer_sync(&acb->eternal_timer);
				//tasklet_kill(&acb->arcmsr_do_message_isr_bh);
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
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		return (arcmsr_scsi_host_template_init(&arcmsr_scsi_host_template));
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_module_exit(void)
	{
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
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
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		pcmd->SCp.Status = 0;
		arcmsr_queue_command(pcmd,arcmsr_internal_done);
		timeout = jiffies + 60 * HZ; 
		while (time_before(jiffies, timeout) && !pcmd->SCp.Status) {
			schedule();
		}
		if (!pcmd->SCp.Status) {
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
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		struct AdapterControlBlock *acb;
		struct AdapterControlBlock *acbtmp;
		int i = 0;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		
		acb = (struct AdapterControlBlock *)dev_id;
		acbtmp = pHCBARC->acb[i];
		while ((acb != acbtmp) && acbtmp && (i < ARCMSR_MAX_ADAPTER)) {
			i++;
			acbtmp=pHCBARC->acb[i];
		}
		if (!acbtmp) {
			#if ARCMSR_DEBUG_FUNCTION
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
		int heads, sectors, cylinders, total_capacity;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		total_capacity = disk->capacity;
		heads = 255;
		sectors = 56;
		cylinders = total_capacity/heads * sectors;
		geom[0] = heads;
		geom[1] = sectors;
		geom[2] = cylinders;
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	int arcmsr_detect(Scsi_Host_Template * host_template)
	{
		struct {
			unsigned int   vendor_id;
			unsigned int   device_id;
		} const arcmsr_devices[] = {
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
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1680}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1681}
		};

		struct pci_dev *pdev = NULL;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		struct Scsi_Host *host;
		static u_int8_t	i;
		
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		
		memset(pHCBARC, 0, sizeof(struct HCBARC));
		for (i = 0; i < (sizeof(arcmsr_devices) / sizeof(arcmsr_devices[0])); ++i) {
			pdev = NULL;
			while ((pdev = pci_find_device(arcmsr_devices[i].vendor_id,arcmsr_devices[i].device_id,pdev))) {
				if ((host = scsi_register(host_template, sizeof(struct AdapterControlBlock))) == 0) {
					printk("arcmsr_detect: scsi_register error . . . . . . . . . . .\n");
					continue;
				}
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
					if (pci_enable_device(pdev)) {
						printk("arcmsr_detect: pci_enable_device ERROR..................................\n");
						scsi_unregister(host);
						continue;
					}
					if (!pci_set_dma_mask(pdev, (dma_addr_t)0xffffffffffffffffULL)) {
						/*64bit*/
						printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
					} else if (pci_set_dma_mask(pdev,(dma_addr_t)0x00000000ffffffffULL)) {
						/*32bit*/
						printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED (ERROR)\n");
						scsi_unregister(host);
						continue;
					}
				#endif
				acb = (struct AdapterControlBlock *) host->hostdata;
				memset(acb, 0, sizeof(struct AdapterControlBlock));
				spin_lock_init(&acb->isr_lock);
				spin_lock_init(&acb->ccblist_lock);
				acb->pdev = pdev;
				acb->host = host;
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 7)
					host->max_sectors = ARCMSR_MAX_XFER_SECTORS;
					if (strncmp(acb->firm_version, "V1.42",5) >= 0)
                                				host->max_sectors=ARCMSR_MAX_XFER_SECTORS_B;
				#endif			
				host->max_lun = ARCMSR_MAX_TARGETLUN;
				host->max_id = ARCMSR_MAX_TARGETID;/*16:8*/

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
					host->max_cmd_len = 16;		 /*this is issue of 64bit LBA ,over 2T byte*/
	    			#endif

				host->sg_tablesize = ARCMSR_DEFAULT_SG_ENTRIES;
				host->can_queue = ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
				host->cmd_per_lun = ARCMSR_MAX_CMD_PERLUN;	    
				host->this_id = ARCMSR_SCSI_INITIATOR_ID;	
				host->io_port = 0;
				host->n_io_port = 0;
				host->irq = pdev->irq;
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 4)
					scsi_set_pci_device(host,pdev);
				#endif
				arcmsr_define_adapter_type(acb);
				acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
				acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
				INIT_LIST_HEAD(&acb->ccb_free_list);
				if (!arcmsr_alloc_ccb_pool(acb)) {
					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
						pci_set_drvdata(pdev,acb); /*set driver_data*/
					#endif
					acb->adapter_index = arcmsr_adapterNo;
					pHCBARC->acb[arcmsr_adapterNo] = acb;
					pci_set_master(pdev);
					if (request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb)) {
						printk("arcmsr_detect: request_irq got ERROR...................\n");
						arcmsr_adapterNo--;
						pHCBARC->acb[acb->adapter_index] = NULL;
						arcmsr_free_ccb_pool(acb);
					    	scsi_unregister(host);
						goto next_areca;
					}

					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
						if (pci_request_regions(pdev, "arcmsr")) {
							printk("arcmsr_detect: pci_request_regions got	ERROR...................\n");
							arcmsr_adapterNo--;
							pHCBARC->acb[acb->adapter_index] = NULL;
							arcmsr_free_ccb_pool(acb);
					    		scsi_unregister(host);
							goto next_areca;
						}
		            			#endif
					arcmsr_iop_init(acb);/*	on kernel 2.4.21 driver's iop read/write must after request_irq	*/
				} else {
					printk("arcmsr: arcmsr_alloc_ccb_pool got ERROR...................\n");
					scsi_unregister(host);
				}
		next_areca:
			}
		}
		if (arcmsr_adapterNo) {
			#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 3, 30)
				host_template->proc_name = "arcmsr";
			#else		  
				host_template->proc_dir = &arcmsr_proc_scsi;
			#endif
			register_reboot_notifier(&arcmsr_event_notifier);
		} else {
			printk("arcmsr_detect:...............NO	ARECA RAID ADAPTER FOUND...........\n");
			return (arcmsr_adapterNo);
		}
		pHCBARC->adapterCnt = arcmsr_adapterNo;
		return(arcmsr_adapterNo);
	}

#endif 
/*
**********************************************************************
**********************************************************************
*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb) {
		struct scsi_cmnd *pcmd = ccb->pcmd;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		scsi_dma_unmap(pcmd);
	 }
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb) {
		struct AdapterControlBlock *acb = ccb->acb;
		struct scsi_cmnd *pcmd = ccb->pcmd;
		struct scatterlist *sl;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)
			sl = scsi_sglist(pcmd);

		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
			sl = (struct scatterlist *) pcmd->request_buffer;
    		#else
			sl = (struct scatterlist *) pcmd->request_buffer;
    		#endif

		if (pcmd->use_sg != 0) {
			pci_unmap_sg(acb->pdev, sl, pcmd->use_sg, pcmd->sc_data_direction);
		} else if (pcmd->request_bufflen != 0){
			pci_unmap_single(acb->pdev, (dma_addr_t)(unsigned long)pcmd->SCp.ptr, pcmd->request_bufflen, pcmd->sc_data_direction);
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
	static bool arcmsr_define_adapter_type(struct AdapterControlBlock *acb)
	{
		struct pci_dev *pdev = acb->pdev;
		u16 dev_id;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		pci_read_config_word(pdev, PCI_DEVICE_ID, &dev_id);
		acb->dev_id = dev_id;
		switch(dev_id) {
		case 0x1880: {
			printk("Found ARC-%x\n", dev_id);
			acb->adapter_type = ACB_ADAPTER_TYPE_C;
			}
			break;
		case 0x1200:
		case 0x1201: {
			printk("Found ARC-%x\n", dev_id);
			acb->adapter_type = ACB_ADAPTER_TYPE_B;
			}
			break;
		case 0x1110:
		case 0x1120:
		case 0x1130:
		case 0x1160:
		case 0x1170:
		case 0x1210:
		case 0x1220:
		case 0x1230:
		case 0x1260:
		case 0x1280:
		case 0x1680: {
			printk("Found ARC-%x\n", dev_id);
			acb->adapter_type = ACB_ADAPTER_TYPE_A;
			}
			break;
		default: {
			printk("Unknown device ID = 0x%x\n", dev_id);
			return false;
		}
		}
		return true;
	}
/*
**********************************************************************************
**this is the replacement of arcmsr_disable_allintr() in the former version
**********************************************************************************
*/
static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb)
{
	u32 orig_mask = 0;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
		/* disable all outbound interrupt */
		orig_mask = readl(&reg->host_int_mask); /* disable outbound message0 int */
		writel(orig_mask|ARCMSR_HBCMU_ALL_INTMASKENABLE, &reg->host_int_mask);
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
	uint32_t mask;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A : {
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		mask = orig_mask & ~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE | ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE | ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
		writel(mask, &reg->outbound_intmask);
		acb->outbound_int_enable = ~(orig_mask & mask) & 0x000000ff;
		}
		break;
	case ACB_ADAPTER_TYPE_B : {
		struct MessageUnit_B *reg = acb->pmuB;
		mask = orig_mask | (ARCMSR_IOP2DRV_DATA_WRITE_OK | ARCMSR_IOP2DRV_DATA_READ_OK | ARCMSR_IOP2DRV_CDB_DONE | ARCMSR_IOP2DRV_MESSAGE_CMD_DONE);
		writel(mask, reg->iop2drv_doorbell_mask);
		acb->outbound_int_enable = (orig_mask | mask) & 0x0000000f;
		}
		break;
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C *reg = acb->pmuC;
		mask = ~(ARCMSR_HBCMU_UTILITY_A_ISR_MASK | ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR_MASK | ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR_MASK);
		writel(orig_mask & mask, &reg->host_int_mask);
		acb->outbound_int_enable = ~(orig_mask & mask) & 0x0000000f;
		}
	}
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_hba_cache(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	int retry_count = 10;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, &reg->inbound_msgaddr0);
	do {
		if (arcmsr_hba_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down=%d \n", arcmsr_adapterNo, retry_count);
		}
	} while (retry_count != 0);
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_hbb_cache(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	int retry_count = 10;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	writel(ARCMSR_MESSAGE_FLUSH_CACHE, reg->drv2iop_doorbell);
	do {
		if (arcmsr_hbb_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down=%d \n", arcmsr_adapterNo, retry_count);
		}
	} while (retry_count != 0);
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_hbc_cache(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C *reg = (struct MessageUnit_C *)pACB->pmuC;
	int retry_count = 30;/* enlarge wait flush adapter cache time: 10 minute */
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	do {
		if (arcmsr_hbc_wait_msgint_ready(pACB)) {
			break;
		} else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' \
			timeout,retry count down = %d \n", pACB->host->host_no, retry_count);
		}
	} while (retry_count != 0);
	return;
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
		
	switch (acb->adapter_type) {

	case ACB_ADAPTER_TYPE_A: {
		arcmsr_flush_hba_cache(acb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		arcmsr_flush_hbb_cache(acb);
		}
		break;
	case ACB_ADAPTER_TYPE_C: {
		arcmsr_flush_hbc_cache(acb);
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_ccb_complete(struct CommandControlBlock *ccb)
{
	struct AdapterControlBlock *acb=ccb->acb;
	struct scsi_cmnd *pcmd=ccb->pcmd;
	unsigned long flags;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	

	atomic_dec(&acb->ccboutstandingcount);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
    		arcmsr_pci_unmap_dma(ccb);
	#endif
	ccb->startdone = ARCMSR_CCB_DONE;
	ccb->ccb_flags = 0;
	spin_lock_irqsave(&acb->ccblist_lock, flags);
	list_add_tail(&ccb->list, &acb->ccb_free_list);
	spin_unlock_irqrestore(&acb->ccblist_lock, flags);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
		pcmd->scsi_done(pcmd);
	#else
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
static void arcmsr_report_sense_info(struct CommandControlBlock *ccb)
{
	struct scsi_cmnd *pcmd = ccb->pcmd;
	struct SENSE_DATA *sensebuffer = (struct SENSE_DATA *)pcmd->sense_buffer;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	pcmd->result = DID_OK << 16;
	if (sensebuffer) {
		int sense_data_length =
			sizeof(struct SENSE_DATA) < SCSI_SENSE_BUFFERSIZE
			? sizeof(struct SENSE_DATA) : SCSI_SENSE_BUFFERSIZE;
		memset(sensebuffer, 0, SCSI_SENSE_BUFFERSIZE);
		memcpy(sensebuffer, ccb->arcmsr_cdb.SenseData, sense_data_length);
		sensebuffer->ErrorCode = SCSI_SENSE_CURRENT_ERRORS;
		sensebuffer->Valid = 1;
	}
}
/*
*********************************************************************
*********************************************************************
*/
static uint8_t arcmsr_abort_hba_allcmd(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, &reg->inbound_msgaddr0);
	if (arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, arcmsr_adapterNo);
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	writel(ARCMSR_MESSAGE_ABORT_CMD, reg->drv2iop_doorbell);
	if (arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, arcmsr_adapterNo);
		return 0xff;
	}
	return 0x00;
}
/*
*********************************************************************
*********************************************************************
*/
static uint8_t arcmsr_abort_hbc_allcmd(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C *reg = (struct MessageUnit_C *)pACB->pmuC;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	if (!arcmsr_hbc_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, pACB->host->host_no);
		return false;
	}
	return true;
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		rtnval = arcmsr_abort_hba_allcmd(acb);
		}
		break;
		
	case ACB_ADAPTER_TYPE_B: {
		rtnval = arcmsr_abort_hbb_allcmd(acb);
		}
		break;
	case ACB_ADAPTER_TYPE_C: {
		rtnval = arcmsr_abort_hbc_allcmd(acb);
		}
	}
	return rtnval;
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_build_ccb(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb, struct scsi_cmnd *pcmd)
{
	struct ARCMSR_CDB *arcmsr_cdb = &ccb->arcmsr_cdb;
	uint8_t *psge = (uint8_t *)&arcmsr_cdb->u;
	__le32 address_lo, address_hi;
	int arccdbsize = 0x30, sgcount = 0;
	unsigned request_bufflen;
	struct scatterlist *sl, *sg;
	unsigned short use_sg;
	__le32 length = 0, length_sum = 0;
	u64 temp;
	int i;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
    	ccb->pcmd = pcmd;
	memset(arcmsr_cdb, 0, sizeof(struct ARCMSR_CDB));
    	arcmsr_cdb->Bus = 0;
   	arcmsr_cdb->TargetID = pcmd->device->id;
    	arcmsr_cdb->LUN = pcmd->device->lun;
    	arcmsr_cdb->Function = 1;
	arcmsr_cdb->CdbLength = (uint8_t)pcmd->cmd_len;
    	arcmsr_cdb->Context = 0;
    	memcpy(arcmsr_cdb->Cdb, pcmd->cmnd, pcmd->cmd_len);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
		request_bufflen = scsi_bufflen(pcmd);
		use_sg = scsi_sg_count(pcmd);
	#else
		request_bufflen = pcmd->request_bufflen;
		use_sg = pcmd->use_sg;
	#endif
	if (use_sg) {
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)
			sl = scsi_sglist(pcmd);
			sgcount = scsi_dma_map(pcmd);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
			sl = (struct scatterlist *) pcmd->request_buffer;
			sgcount = pci_map_sg(acb->pdev, sl, pcmd->use_sg, pcmd->sc_data_direction);
		#else
			sl = (struct scatterlist *) pcmd->request_buffer;
			sgcount = pcmd->use_sg;
		#endif
		scsi_for_each_sg(pcmd, sg, sgcount, i) {
			#if LINUX_VERSION_CODE >=KERNEL_VERSION(2, 3, 30)
				length = cpu_to_le32(sg_dma_len(sg));
				address_lo = cpu_to_le32(dma_addr_lo32(sg_dma_address(sg)));
				address_hi = cpu_to_le32(dma_addr_hi32(sg_dma_address(sg)));
			#else
				length = cpu_to_le32(sg->length);
				address_lo = cpu_to_le32(virt_to_bus(sg->address));
				address_hi = 0;
			#endif
			length_sum += length;
			if (address_hi == 0) {
				struct SG32ENTRY *pdma_sg = (struct SG32ENTRY *)psge;
				pdma_sg->address = address_lo;
				pdma_sg->length = length;
				psge +=	sizeof(struct SG32ENTRY);
				arccdbsize += sizeof(struct SG32ENTRY);
			} else {
				struct SG64ENTRY *pdma_sg = (struct SG64ENTRY *)psge;

				pdma_sg->addresshigh = address_hi;
				pdma_sg->address = address_lo;
				pdma_sg->length = length|cpu_to_le32(IS_SG64_ADDR);
				psge += sizeof(struct SG64ENTRY);
				arccdbsize += sizeof(struct SG64ENTRY);
			}
		}
		arcmsr_cdb->sgcount = sgcount;
		arcmsr_cdb->DataLength = request_bufflen;
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6 ,0)
			arcmsr_cdb->msgPages = arccdbsize/0x100 + (arccdbsize % 0x100 ? 1 : 0);
		#endif
		if ( arccdbsize > 256)
			arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_SGL_BSIZE;
	} else if (request_bufflen) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
			dma_addr_t dma_addr;
			dma_addr = pci_map_single(acb->pdev, scsi_sglist(pcmd), scsi_bufflen(pcmd), pcmd->sc_data_direction);			
			pcmd->SCp.ptr =	(char *)(unsigned long)dma_addr;/* We hide it here for later unmap. */
			address_lo = cpu_to_le32(dma_addr_lo32(dma_addr));
	    		address_hi = cpu_to_le32(dma_addr_hi32(dma_addr));
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
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
		
		if (address_hi == 0) {
			struct SG32ENTRY* pdma_sg = (struct SG32ENTRY*)psge;
			pdma_sg->address = address_lo;
	    		pdma_sg->length = request_bufflen;
		} else {
			struct SG64ENTRY* pdma_sg = (struct SG64ENTRY*)psge;
			pdma_sg->addresshigh = address_hi;
			pdma_sg->address = address_lo;
	    		pdma_sg->length = request_bufflen|cpu_to_le32(IS_SG64_ADDR);
		}
		arcmsr_cdb->sgcount = 1;
		arcmsr_cdb->DataLength = request_bufflen;
	}	
	if (pcmd->sc_data_direction == DMA_TO_DEVICE) {
		arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_WRITE;
		ccb->ccb_flags |= CCB_FLAG_WRITE;
	}
	ccb->arc_cdb_size = arccdbsize;
	if (timeout) {
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 27)
			temp = pcmd->request->timeout / HZ;//timeout value in struct request is in jiffy
			pcmd->request->deadline = jiffies + timeout * HZ;
			if (pcmd->device->request_queue->timeout.function) {
				mod_timer(&pcmd->device->request_queue->timeout, pcmd->request->deadline);			
			}
			#if ARCMSR_DEBUG_TTV
				osm_printk("%s: Original SCSI command Timeout = %lld, Modified SCSI command Timeout = %ld!\n", __func__, temp, pcmd->request->deadline);
			#endif
		#else
			temp = pcmd->timeout_per_command / HZ;//timeout value in struct request is in jiffy
			if (pcmd->eh_timeout.function) {
				mod_timer(&pcmd->eh_timeout, jiffies + timeout  * HZ );
			}
			#if ARCMSR_DEBUG_TTV
				osm_printk("%s: Original SCSI command Timeout = %lld Modified SCSI command Timeout = %d!\n", __func__, temp, pcmd->timeout_per_command);
			#endif
		#endif
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
	uint32_t shifted_cdb_phyaddr = ccb->shifted_cdb_phyaddr;
	struct ARCMSR_CDB *arcmsr_cdb = (struct ARCMSR_CDB *)&ccb->arcmsr_cdb;
	struct scsi_cmnd *cmd;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	atomic_inc(&acb->ccboutstandingcount);
	ccb->startdone = ARCMSR_CCB_START;
	cmd = (struct scsi_cmnd *)ccb->pcmd;
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		struct MessageUnit_A *reg = acb->pmuA;

		if (arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE)
			writel(shifted_cdb_phyaddr | ARCMSR_CCBPOST_FLAG_SGL_BSIZE, &reg->inbound_queueport);
		else {
			writel(shifted_cdb_phyaddr, &reg->inbound_queueport);
		}
	}
	break;

	case ACB_ADAPTER_TYPE_B: {
		struct MessageUnit_B *reg = acb->pmuB;
		int32_t ending_index, index = reg->postq_index;

		ending_index = ((index + 1) % ARCMSR_MAX_HBB_POSTQUEUE);
		reg->post_qbuffer[ending_index] = 0;
		if (arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE) {
			reg->post_qbuffer[index] = (shifted_cdb_phyaddr | ARCMSR_CCBPOST_FLAG_SGL_BSIZE);
		} else {
			reg->post_qbuffer[index] = shifted_cdb_phyaddr;
		}
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;     /*if last index number set it to 0 */
		reg->postq_index = index;
		writel(ARCMSR_DRV2IOP_CDB_POSTED, reg->drv2iop_doorbell);
	}
	break;

	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C *phbcmu = (struct MessageUnit_C *)acb->pmuC;
		uint32_t ccb_post_stamp, arc_cdb_size;

		arc_cdb_size = (ccb->arc_cdb_size > 0x300) ? 0x300 : ccb->arc_cdb_size;
		ccb_post_stamp = (shifted_cdb_phyaddr | ((arc_cdb_size - 1) >> 6) | 1);
		if (acb->cdb_phyaddr_hi32) {
			writel(acb->cdb_phyaddr_hi32, &phbcmu->inbound_queueport_high);
			writel(ccb_post_stamp, &phbcmu->inbound_queueport_low);
		} else {
			writel(ccb_post_stamp, &phbcmu->inbound_queueport_low);
		}
	}
	}
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_iop_message_read(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C __iomem *reg = acb->pmuC;
		writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_iop_message_wrote(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C __iomem *reg = acb->pmuC;
		/*
		** push inbound doorbell tell iop, driver data write ok
		** and wait reply on next hwinterrupt for next Qbuffer post
		*/
		writel(ARCMSR_HBCMU_DRV2IOP_DATA_WRITE_OK, &reg->inbound_doorbell);
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

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C *phbcmu = (struct MessageUnit_C *)acb->pmuC;
		qbuffer = (struct QBUFFER __iomem *)&phbcmu->message_rbuffer;
		}
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

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
		pqbuffer = (struct QBUFFER __iomem *)&reg->message_wbuffer;
		}
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

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	pwbuffer = arcmsr_get_iop_wqbuffer(acb);
	iop_data = (uint8_t __iomem *)pwbuffer->data;
    	if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_READED) {
		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		wqbuf_firstindex = acb->wqbuf_firstindex;
		wqbuf_lastindex = acb->wqbuf_lastindex;
		while ((wqbuf_firstindex != wqbuf_lastindex) && (allxfer_len < 124)) {
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, &reg->inbound_msgaddr0);

	if (!arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter background rebulid' timeout \n", arcmsr_adapterNo);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_hbb_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_MESSAGE_STOP_BGRB, reg->drv2iop_doorbell);

	if (!arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter background rebulid' timeout \n", arcmsr_adapterNo);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_hbc_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C *reg = (struct MessageUnit_C *)pACB->pmuC;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	if (!arcmsr_hbc_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'stop adapter background rebulid' timeout \n"
			, pACB->host->host_no);
	}
	return;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case ACB_ADAPTER_TYPE_C: {
		arcmsr_stop_hbc_bgrb(acb);
		}
	}	
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
/*
************************************************************************
************************************************************************
*/
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) 
		dma_free_coherent(&acb->pdev->dev, acb->uncache_size, acb->dma_coherent, acb->dma_coherent_handle);
	#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
		pci_free_consistent(acb->pdev, acb->uncache_size, acb->dma_coherent, acb->dma_coherent_handle);
	#else
		kfree(acb->dma_coherent);
	#endif
}
#else
/*
************************************************************************
************************************************************************
*/
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A: {
		iounmap(acb->pmuA);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) 
			dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM)+0x20), acb->dma_coherent, acb->dma_coherent_handle);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
			pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM)+0x20), acb->dma_coherent, acb->dma_coherent_handle);
		#else
			kfree(acb->dma_coherent);
		#endif
		}
		break;

	case ACB_ADAPTER_TYPE_B: {
		iounmap(acb->mem_base0);
		iounmap(acb->mem_base1);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) 
			dma_free_coherent(&acb->pdev->dev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
			pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#else
			kfree(acb->dma_coherent);
		#endif
		}
		}

}
#endif
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
	
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

   	rqbuf_lastindex = acb->rqbuf_lastindex;
	rqbuf_firstindex = acb->rqbuf_firstindex;
	prbuffer = arcmsr_get_iop_rqbuffer(acb);		/*get the Qbuffer on IOP for data read by the driver*/
	iop_data = (uint8_t __iomem *)prbuffer->data;
	iop_len = prbuffer->data_len;
    	my_empty_len = (rqbuf_firstindex - rqbuf_lastindex -1) & (ARCMSR_MAX_QBUFFER -1);

	if (my_empty_len >= iop_len) {
		while (iop_len > 0) {
			pQbuffer = (struct QBUFFER *)&acb->rqbuffer[rqbuf_lastindex];
			memcpy(pQbuffer, iop_data, 1);	/*from IOP tp driver buffer*/
			rqbuf_lastindex++;
			rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
			iop_data++;
			iop_len--;
		}
        		acb->rqbuf_lastindex = rqbuf_lastindex;
		arcmsr_iop_message_read(acb);		/*notice IOP the message is read*/
	} else {
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_READED;
	if (acb->wqbuf_firstindex != acb->wqbuf_lastindex) {
		uint8_t * pQbuffer;
		struct QBUFFER __iomem *pwbuffer;
		uint8_t __iomem *iop_data;
		int32_t allxfer_len = 0;
		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		pwbuffer = arcmsr_get_iop_wqbuffer(acb);    /*get the Qbuffer on IOP for data which be written to by the driver, i.e. read by IOP*/
		iop_data = (uint8_t __iomem *)pwbuffer->data;

		while ((acb->wqbuf_firstindex != acb->wqbuf_lastindex) && (allxfer_len < 124)) {
			pQbuffer = &acb->wqbuffer[acb->wqbuf_firstindex];
   			memcpy(iop_data, pQbuffer, 1);
			acb->wqbuf_firstindex++;
			acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; 
			iop_data++;
			allxfer_len++;
		}
		pwbuffer->data_len = allxfer_len;

		arcmsr_iop_message_wrote(acb);
 	}

	if (acb->wqbuf_firstindex == acb->wqbuf_lastindex) {
		acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_CLEARED;
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_report_ccb_state(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb, bool error)
{
	uint8_t id, lun;	
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	id = ccb->pcmd->device->id;
	lun = ccb->pcmd->device->lun;
	/* FW replies ccb w/o errors */
	if (!error) {
		if (acb->devstate[id][lun] == ARECA_RAID_GONE)
			acb->devstate[id][lun] = ARECA_RAID_GOOD;
		ccb->pcmd->result = DID_OK << 16;
		arcmsr_ccb_complete(ccb);
	} else {
		/* FW replies ccb w/ errors */
		switch(ccb->arcmsr_cdb.DeviceStatus) {
		case ARCMSR_DEV_SELECT_TIMEOUT: {
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_NO_CONNECT << 16;
			arcmsr_ccb_complete(ccb);
			}
			break;

		case ARCMSR_DEV_ABORTED: 

		case ARCMSR_DEV_INIT_FAIL: {
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_BAD_TARGET << 16;
			arcmsr_ccb_complete(ccb);
			}
			break;

		case ARCMSR_DEV_CHECK_CONDITION: {
			acb->devstate[id][lun] = ARECA_RAID_GOOD;
			ccb->pcmd->result = (DID_OK << 16 |CHECK_CONDITION << 1);
			arcmsr_report_sense_info(ccb);
			arcmsr_ccb_complete(ccb);
			}
			break;

		default:
			printk(KERN_EMERG "arcmsr%d: scsi id = %d lun = %d get command error done, but got unknown DeviceStatus = 0x%x \n"
				, arcmsr_adapterNo
				, id
				, lun
				, ccb->arcmsr_cdb.DeviceStatus);
			acb->devstate[id][lun] = ARECA_RAID_GONE;
			ccb->pcmd->result = DID_BAD_TARGET << 16;
			arcmsr_ccb_complete(ccb);
			break;
		}
	}	
}
/*
*******************************************************************************
* Finish the flag_ccb in the done_queue which is the buffer queues the answered ccb from adapter processor 
*******************************************************************************
*/
static void arcmsr_drain_donequeue(struct AdapterControlBlock *acb, struct CommandControlBlock *pCCB, bool error)
{
	int id, lun;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	if ((pCCB->acb != acb) || (pCCB->startdone != ARCMSR_CCB_START)) {
		if (pCCB->startdone == ARCMSR_CCB_ABORTED) {
			struct scsi_cmnd *abortcmd = pCCB->pcmd;
			if (abortcmd) {
				id = abortcmd->device->id;
				lun = abortcmd->device->lun;				
				abortcmd->result |= DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
				printk(KERN_NOTICE "arcmsr%d: pCCB ='0x%p' isr got aborted command \n",
				acb->host->host_no, pCCB);
			}
			return;
		}
		printk(KERN_NOTICE "arcmsr%d: isr get an illegal ccb command"
				"done acb = 0x%p,"
				"ccb = 0x%p,"
				"ccbacb = 0x%p,"
				"startdone = 0x%x"
				"ccboutstandingcount = %d \n"
				, acb->host->host_no
				, acb
				, pCCB
				, pCCB->acb
				, pCCB->startdone
				, atomic_read(&acb->ccboutstandingcount));
		return;
	}
	arcmsr_report_ccb_state(acb, pCCB, error);
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hba_doorbell_isr(struct AdapterControlBlock *acb) 
{
    	uint32_t outbound_doorbell;
	struct MessageUnit_A __iomem *reg  = acb->pmuA;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	outbound_doorbell = readl(&reg->outbound_doorbell);
	writel(outbound_doorbell, &reg->outbound_doorbell); 
	if (outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK) {
		arcmsr_iop2drv_data_wrote_handle(acb);
	}

	if (outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK) {
		arcmsr_iop2drv_data_read_handle(acb);
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hbc_doorbell_isr(struct AdapterControlBlock *pACB)
{
	uint32_t outbound_doorbell;
	struct MessageUnit_C *reg = (struct MessageUnit_C *)pACB->pmuC;
	/*
	*******************************************************************
	**  Maybe here we need to check wrqbuffer_lock is lock or not
	**  DOORBELL: din! don!
	**  check if there are any mail need to pack from firmware
	*******************************************************************
	*/
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	outbound_doorbell = readl(&reg->outbound_doorbell);
	writel(outbound_doorbell, &reg->outbound_doorbell_clear);/*clear interrupt*/
	if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_OK) {
		arcmsr_iop2drv_data_wrote_handle(pACB);
	}
	if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_DATA_READ_OK) {
		arcmsr_iop2drv_data_read_handle(pACB);
	}
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbc_message_isr(pACB);    /* messenger of "driver to iop commands" */
		}
	#endif
	return;
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hba_postqueue_isr(struct AdapterControlBlock *acb)
{
	uint32_t flag_ccb;
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	struct ARCMSR_CDB *pARCMSR_CDB;
	struct CommandControlBlock *pCCB;
	bool error;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	while ((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) {
		pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
		pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_drain_donequeue(acb, pCCB, error);
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hbb_postqueue_isr(struct AdapterControlBlock *acb)
{
	uint32_t index;
	uint32_t flag_ccb;
	struct MessageUnit_B *reg = acb->pmuB;
	struct ARCMSR_CDB *pARCMSR_CDB;
	struct CommandControlBlock *pCCB;
	bool error;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	index = reg->doneq_index;
	while ((flag_ccb = readl(&reg->done_qbuffer[index])) != 0) {
		writel(0, &reg->done_qbuffer[index]);
		pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
		pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_drain_donequeue(acb, pCCB, error);
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;
		reg->doneq_index = index;
	}
}
/*
*******************************************************************************
*******************************************************************************
*/
static void arcmsr_hbc_postqueue_isr(struct AdapterControlBlock *acb)
{
	struct MessageUnit_C *phbcmu;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *ccb;
	uint32_t flag_ccb, ccb_cdb_phy, throttling = 0;
	int error;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	phbcmu = (struct MessageUnit_C *)acb->pmuC;
	/* areca cdb command done */
	/* Use correct offset and size for syncing */
	while (readl(&phbcmu->host_int_status) &
	ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) {
		/* check if command done with no error*/
		flag_ccb = readl(&phbcmu->outbound_queueport_low);
		ccb_cdb_phy = (flag_ccb & 0xFFFFFFF0);/*frame must be 32 bytes aligned*/
		arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);
		ccb = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
		/* check if command done with no error */
		arcmsr_drain_donequeue(acb, ccb, error);
		if (throttling == ARCMSR_HBC_ISR_THROTTLING_LEVEL) {
			writel(ARCMSR_HBCMU_DRV2IOP_POSTQUEUE_THROTTLING, &phbcmu->inbound_doorbell);
			break;
		}
		throttling++;
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
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_hba_message_isr(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_A *reg  = acb->pmuA;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		
		writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);/*clear interrupt and message state*/
		schedule_work(&acb->arcmsr_do_message_isr_bh);
	}
	static void arcmsr_hbb_message_isr(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_B *reg  = acb->pmuB;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
		schedule_work(&acb->arcmsr_do_message_isr_bh);
	}

	static void arcmsr_hbc_message_isr(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_C *reg  = acb->pmuC;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		/*clear interrupt and message state*/
		writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &reg->outbound_doorbell_clear);
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	
	outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	if (!(outbound_intstatus & ARCMSR_MU_OUTBOUND_HANDLE_INT)) {
        		return 1;
	}
	writel(outbound_intstatus, &reg->outbound_intstatus);
	if (outbound_intstatus & ARCMSR_MU_OUTBOUND_DOORBELL_INT) {
		arcmsr_hba_doorbell_isr(acb);
	}
	if (outbound_intstatus & ARCMSR_MU_OUTBOUND_POSTQUEUE_INT) {
		arcmsr_hba_postqueue_isr(acb);
	}
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		if (outbound_intstatus & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) {
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif	
	outbound_doorbell = readl(reg->iop2drv_doorbell) & acb->outbound_int_enable;
	if (!outbound_doorbell)
		return 1;
	writel(~outbound_doorbell, reg->iop2drv_doorbell);	/* clear doorbell interrupt */
	readl(reg->iop2drv_doorbell);			/* in case the last action of doorbell interrupt clearance is cached, this action can push HW to write down the clear bit*/
	writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
	if (outbound_doorbell & ARCMSR_IOP2DRV_DATA_WRITE_OK) {
		arcmsr_iop2drv_data_wrote_handle(acb);
	}
	if (outbound_doorbell & ARCMSR_IOP2DRV_DATA_READ_OK) {
		arcmsr_iop2drv_data_read_handle(acb);
	}
	if (outbound_doorbell & ARCMSR_IOP2DRV_CDB_DONE) {
		arcmsr_hbb_postqueue_isr(acb);
	}
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		if (outbound_doorbell & ARCMSR_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbb_message_isr(acb);		/* messenger of "driver to iop commands" */
		}
	#endif
	return 0;
}
/*
*******************************************************************************
*******************************************************************************
*/
static int arcmsr_handle_hbc_isr(struct AdapterControlBlock *pACB)
{
	uint32_t host_interrupt_status;
	struct MessageUnit_C *phbcmu = (struct MessageUnit_C *)pACB->pmuC;
	/*
	*********************************************
	**   check outbound intstatus
	*********************************************
	*/
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	host_interrupt_status = readl(&phbcmu->host_int_status);
	if (!host_interrupt_status) {
		/*it must be share irq*/
		#if ARCMSR_DEBUG_FUNCTION
			printk("share irq..................................\n");
		#endif
		return 1;
	}
	/* MU ioctl transfer doorbell interrupts*/
	if (host_interrupt_status & ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR) {
		arcmsr_hbc_doorbell_isr(pACB);   /* messenger of "ioctl message read write" */
	}
	/* MU post queue interrupts*/
	if (host_interrupt_status & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) {
		arcmsr_hbc_postqueue_isr(pACB);  /* messenger of "scsi commands" */
	}
	return 0;
}
/*
*******************************************************************************
*******************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
	static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb)
#else
	static void arcmsr_interrupt(struct AdapterControlBlock *acb)
#endif
	{
		switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			if (arcmsr_handle_hba_isr(acb)) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
					return IRQ_NONE;
				#else
					return;
				#endif
			}
		} 
			break;
		
		case ACB_ADAPTER_TYPE_B: {
			if (arcmsr_handle_hbb_isr(acb)) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
					return IRQ_NONE;
				#else
					return;
				#endif
			}
		} 
			break;
		case ACB_ADAPTER_TYPE_C: {
			if (arcmsr_handle_hbc_isr(acb)) {
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
					return IRQ_NONE;
				#else
					return;
				#endif
			}
		}
		}
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
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
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
		use_sg = scsi_sg_count(cmd);
	#else
		use_sg = cmd->use_sg;
	#endif

	if (use_sg) {

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
			sg = scsi_sglist(cmd);
		#else
			sg = (struct scatterlist *)cmd->request_buffer;
		#endif

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
			buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
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
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25) 
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
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if(acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}else{
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		#endif
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
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if(acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}else{
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
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

		if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
			acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
			arcmsr_iop_message_read(acb);
		}
		acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
		acb->rqbuf_firstindex = 0;
		acb->rqbuf_lastindex = 0;
		memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if(acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}else{
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		#endif
	}
	break;
	case ARCMSR_MESSAGE_CLEAR_WQBUFFER: {
		uint8_t *pQbuffer = acb->rqbuffer;
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if(acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}else{
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		#endif

		if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
			acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
			arcmsr_iop_message_read(acb);
		}
		acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED|ACB_F_MESSAGE_WQBUFFER_READED);
		acb->wqbuf_firstindex = 0;
		acb->wqbuf_lastindex = 0;
		memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
	}
	break;
	case ARCMSR_MESSAGE_CLEAR_ALLQBUFFER: {
		uint8_t *pQbuffer;

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
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		#endif
	}
	break;
	case ARCMSR_MESSAGE_RETURN_CODE_3F: {
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_3F;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_3F;
		#endif
	}
	break;
	case ARCMSR_MESSAGE_SAY_HELLO: {
		int8_t * hello_string = "Hello! I am ARCMSR";

		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
		#else
			pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
		#endif
		memcpy(pcmdmessagefld->messagedatabuffer, hello_string, (int16_t)strlen(hello_string));
	}
	break;
	case ARCMSR_MESSAGE_SAY_GOODBYE: {
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}
		#endif
		arcmsr_iop_parking(acb);
	}
	break;
	case ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE: {
		#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			}
		#endif
		arcmsr_flush_adapter_cache(acb);
		break;
		}
	default:
		retvalue = ARCMSR_MESSAGE_FAIL;
	}
	message_out:
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
		if (use_sg) {
			struct scatterlist *sg;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
				sg = scsi_sglist(cmd);
			#else
				sg = (struct scatterlist *)cmd->request_buffer;
			#endif
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
	struct CommandControlBlock *ccb;
	unsigned long flags;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	spin_lock_irqsave(&acb->ccblist_lock, flags);
	if (!list_empty(head)) {
		ccb = list_entry(head->next, struct CommandControlBlock, list);
		list_del_init(&ccb->list);
	} else {
		spin_unlock_irqrestore(&acb->ccblist_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore(&acb->ccblist_lock, flags);
	return ccb;
}
/*
***********************************************************************
***********************************************************************
*/
static void arcmsr_handle_virtual_command(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
			use_sg = scsi_sg_count(cmd);
		#else
			use_sg = cmd->use_sg;
		#endif
		
		if (use_sg) {
			struct scatterlist *sg;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
				sg = scsi_sglist(cmd);
			#else
				sg = (struct scatterlist *) cmd->request_buffer;
			#endif

			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
				buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
			#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
				buffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
			#else
				buffer = sg->address;
			#endif
		} else {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
				struct scatterlist *sg = scsi_sglist(cmd);
				buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
			#else
				buffer = cmd->request_buffer;
			#endif
		}
		memcpy(buffer, inqdata, sizeof(inqdata));
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
			if (scsi_sg_count(cmd)) {
				struct scatterlist *sg;

				sg = (struct scatterlist *) scsi_sglist(cmd);
				kunmap_atomic(buffer - sg->offset, KM_IRQ0);
			}
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
int arcmsr_queue_command_lck(struct scsi_cmnd *cmd, void (* done)(struct scsi_cmnd *))
{
	struct Scsi_Host *host = cmd->device->host;
	struct AdapterControlBlock *acb=(struct AdapterControlBlock *) host->hostdata;
	struct CommandControlBlock *ccb;
	int target=cmd->device->id;
	int lun = cmd->device->lun;
	uint8_t scsicmd = cmd->cmnd[0];

	#if ARCMSR_DEBUG_FUNCTION
    		printk(KERN_NOTICE "arcmsr_queue_command, scsi_cmnd(0x%p), cmnd[0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x], scsi_id = 0x%x, scsi_lun = 0x%x.\n",
			cmd, 
			cmd->cmnd[0],
			cmd->cmnd[1],
			cmd->cmnd[2],
			cmd->cmnd[3],
			cmd->cmnd[4],
			cmd->cmnd[5],
			cmd->cmnd[6], 
			cmd->cmnd[7],
			cmd->cmnd[8],
			cmd->cmnd[9],
			cmd->cmnd[10], 
			cmd->cmnd[11],
			cmd->device->id,
			cmd->device->lun
		);
	#endif
	cmd->scsi_done = done;
	cmd->host_scribble = NULL;
	cmd->result = 0;

 	if ((scsicmd == SYNCHRONIZE_CACHE) || (scsicmd == SEND_DIAGNOSTIC)) {
		if (acb->devstate[target][lun] == ARECA_RAID_GONE) {
	    		cmd->result = (DID_NO_CONNECT << 16);
		}
		cmd->scsi_done(cmd);
		return 0;
	}
	if (target == 16) {
		/* virtual device for iop message transfer */
		arcmsr_handle_virtual_command(acb, cmd);
		return 0;
	}
	if (atomic_read(&acb->ccboutstandingcount) >= ARCMSR_MAX_OUTSTANDING_CMD){
		return SCSI_MLQUEUE_HOST_BUSY;
	}
    	ccb = arcmsr_get_freeccb(acb);
	if (!ccb){
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	if (arcmsr_build_ccb(acb, ccb, cmd) == FAILED) {
		cmd->result = (DID_ERROR << 16) |(RESERVATION_CONFLICT << 1);
		cmd->scsi_done(cmd);
		return 0;
	}
	arcmsr_post_ccb(acb, ccb);
	return 0;
}

DEF_SCSI_QCMD(arcmsr_queue_command)

#else

int arcmsr_queue_command(struct scsi_cmnd *cmd, void (* done)(struct scsi_cmnd *))
{
	struct Scsi_Host *host = cmd->device->host;
	struct AdapterControlBlock *acb=(struct AdapterControlBlock *) host->hostdata;
	struct CommandControlBlock *ccb;
	int target=cmd->device->id;
	int lun = cmd->device->lun;
	uint8_t scsicmd = cmd->cmnd[0];

	#if ARCMSR_DEBUG_FUNCTION
    		printk(KERN_NOTICE "arcmsr_queue_command, scsi_cmnd(0x%p), cmnd[0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x], scsi_id = 0x%x, scsi_lun = 0x%x.\n",
			cmd, 
			cmd->cmnd[0],
			cmd->cmnd[1],
			cmd->cmnd[2],
			cmd->cmnd[3],
			cmd->cmnd[4],
			cmd->cmnd[5],
			cmd->cmnd[6], 
			cmd->cmnd[7],
			cmd->cmnd[8],
			cmd->cmnd[9],
			cmd->cmnd[10], 
			cmd->cmnd[11],
			cmd->device->id,
			cmd->device->lun
		);
	#endif
	cmd->scsi_done = done;
	cmd->host_scribble = NULL;
	cmd->result = 0;

 	if ((scsicmd == SYNCHRONIZE_CACHE) || (scsicmd == SEND_DIAGNOSTIC)) {
		if (acb->devstate[target][lun] == ARECA_RAID_GONE) {
	    		cmd->result = (DID_NO_CONNECT << 16);
		}
		cmd->scsi_done(cmd);
		return 0;
	}
	if (target == 16) {
		/* virtual device for iop message transfer */
		arcmsr_handle_virtual_command(acb, cmd);
		return 0;
	}
	if (atomic_read(&acb->ccboutstandingcount) >= ARCMSR_MAX_OUTSTANDING_CMD){
		return SCSI_MLQUEUE_HOST_BUSY;
	}
    	ccb = arcmsr_get_freeccb(acb);
	if (!ccb){
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	if (arcmsr_build_ccb(acb, ccb, cmd) == FAILED) {
		cmd->result = (DID_ERROR << 16) |(RESERVATION_CONFLICT << 1);
		cmd->scsi_done(cmd);
		return 0;
	}
	arcmsr_post_ccb(acb, ccb);
	return 0;
}
#endif
/*
**********************************************************************
**********************************************************************
*/
bool arcmsr_get_hba_config(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model = (char __iomem *) (&reg->msgcode_rwbuffer[15]);
	char __iomem *iop_firm_version = (char __iomem *) (&reg->msgcode_rwbuffer[17]);
	char __iomem *iop_device_map = (char __iomem *) (&reg->msgcode_rwbuffer[21]);
	int count;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

    	writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
	if (!arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'get adapter firmware miscellaneous data' timeout \n", arcmsr_adapterNo);
		return false;
	}
	count = 8;
	while (count) {
		*acb_firm_model = readb(iop_firm_model);
		acb_firm_model++;
		iop_firm_model++;
		count--;
	}
	count = 16;	
	while (count) {
        		*acb_firm_version = readb(iop_firm_version);
        		acb_firm_version++;
		iop_firm_version++;
		count--;		
	}
	count=16;
	while (count) {
        		*acb_device_map=readb(iop_device_map);
        		acb_device_map++;
		iop_device_map++;
		count--;
 	}
	printk(KERN_NOTICE "Areca RAID Controller%d: F/W %s & Model %s\n", arcmsr_adapterNo, acb->firm_version, acb->firm_model);
	acb->signature = readl(&reg->msgcode_rwbuffer[0]);
	acb->firm_request_len = readl(&reg->msgcode_rwbuffer[1]);
	acb->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[2]);
	acb->firm_sdram_size = readl(&reg->msgcode_rwbuffer[3]);
	acb->firm_hd_channels = readl(&reg->msgcode_rwbuffer[4]);
	acb->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	return true;
}
/*
**********************************************************************
**********************************************************************
*/
bool arcmsr_get_hbb_config(struct AdapterControlBlock *acb)
{
 	struct MessageUnit_B *reg = acb->pmuB;
	struct pci_dev *pdev = acb->pdev;
	void *dma_coherent;
	dma_addr_t dma_coherent_handle;
	uint32_t __iomem *lrwbuffer;
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model;	/*firm_model,15,60-67*/
	char __iomem *iop_firm_version;	/*firm_version,17,68-83*/
	char __iomem *iop_device_map;	/*firm_version,21,84-99*/
 	int count;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)		
		dma_coherent = dma_alloc_coherent(&pdev->dev, sizeof(struct MessageUnit_B), &dma_coherent_handle, GFP_KERNEL);
	#else
		dma_coherent = pci_alloc_consistent(pdev, sizeof(struct MessageUnit_B), &dma_coherent_handle);
	#endif
	if (!dma_coherent) {
		printk("arcmsr%d: dma_alloc_coherent got error for hbb mu\n", arcmsr_adapterNo);
		return false;
	}
	acb->dma_coherent_handle_hbb_mu = dma_coherent_handle;
	reg = (struct MessageUnit_B *)dma_coherent;
	acb->pmuB = reg;
	reg->drv2iop_doorbell = (uint32_t __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_DRV2IOP_DOORBELL);
	reg->drv2iop_doorbell_mask = (uint32_t __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_DRV2IOP_DOORBELL_MASK);
	reg->iop2drv_doorbell = (uint32_t __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_IOP2DRV_DOORBELL);
	reg->iop2drv_doorbell_mask = (uint32_t __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_IOP2DRV_DOORBELL_MASK);
	reg->message_wbuffer = (uint32_t __iomem *)((unsigned long)acb->mem_base1 + ARCMSR_MESSAGE_WBUFFER);
	reg->message_rbuffer =  (uint32_t __iomem *)((unsigned long)acb->mem_base1 + ARCMSR_MESSAGE_RBUFFER);
	reg->msgcode_rwbuffer = (uint32_t __iomem *)((unsigned long)acb->mem_base1 + ARCMSR_MESSAGE_RWBUFFER);
	lrwbuffer = reg->msgcode_rwbuffer;
	iop_firm_model = (char __iomem *)(&reg->msgcode_rwbuffer[15]);	/*firm_model,15,60-67*/
	iop_firm_version = (char __iomem *)(&reg->msgcode_rwbuffer[17]);	/*firm_version,17,68-83*/
	iop_device_map = (char __iomem *)(&reg->msgcode_rwbuffer[21]);	/*firm_version,21,84-99*/
	writel(ARCMSR_MESSAGE_GET_CONFIG, reg->drv2iop_doorbell);
	if (!arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait get adapter firmware miscellaneous data timeout \n", arcmsr_adapterNo);
		return false;
	}
	count = 8;
	while (count) {
		*acb_firm_model=readb(iop_firm_model);
		acb_firm_model++;
		iop_firm_model++;
		count--;
	}
	count = 16;
	while (count) {
		*acb_firm_version=readb(iop_firm_version);
		acb_firm_version++;
		iop_firm_version++;
		count--;
	}
	count = 16;
	while (count) {
		*acb_device_map=readb(iop_device_map);
		acb_device_map++;
		iop_device_map++;
		count--;
	}
	printk(KERN_NOTICE "Areca RAID Controller%d: F/W %s & Model %s\n", arcmsr_adapterNo, acb->firm_version, acb->firm_model);
	acb->signature = readl(&reg->msgcode_rwbuffer[1]);
	/*firm_signature,1,00-03*/
	acb->firm_request_len=readl(&reg->msgcode_rwbuffer[2]); 
	/*firm_request_len,1,04-07*/
	acb->firm_numbers_queue=readl(&reg->msgcode_rwbuffer[3]); 
	/*firm_numbers_queue,2,08-11*/
	acb->firm_sdram_size=readl(&reg->msgcode_rwbuffer[4]); 
	/*firm_sdram_size,3,12-15*/
	acb->firm_hd_channels=readl(&reg->msgcode_rwbuffer[5]); 
	/*firm_hd_channels,4,16-19*/
	acb->firm_cfg_version=readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	return true;
}
/*
**********************************************************************
**********************************************************************
*/
static bool arcmsr_get_hbc_config(struct AdapterControlBlock *pACB)
{
	uint32_t intmask_org, Index, firmware_state = 0;
	struct MessageUnit_C *reg = pACB->pmuC;
	char *acb_firm_model = pACB->firm_model;
	char *acb_firm_version = pACB->firm_version;
	char *iop_firm_model = (char *)(&reg->msgcode_rwbuffer[15]);    /*firm_model,15,60-67*/
	char *iop_firm_version = (char *)(&reg->msgcode_rwbuffer[17]);  /*firm_version,17,68-83*/
	int count;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	/* disable all outbound interrupt */
	intmask_org = readl(&reg->host_int_mask); /* disable outbound message0 int */
	writel(intmask_org | ARCMSR_HBCMU_ALL_INTMASKENABLE, &reg->host_int_mask);
	/* wait firmware ready */
	do {
		firmware_state = readl(&reg->outbound_msgaddr1);
	} while ((firmware_state & ARCMSR_HBCMU_MESSAGE_FIRMWARE_OK) == 0);
	mdelay(20);	
	/* post "get config" instruction */
	writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	/* wait message ready */
	for (Index = 0; Index < 2000; Index++) {
		if (readl(&reg->outbound_doorbell) & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
			writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &reg->outbound_doorbell_clear);/*clear interrupt*/
			break;
		}
		udelay(10);
	} /*max 1 seconds*/
	if (Index >= 2000) {
		printk(KERN_NOTICE "arcmsr%d: wait 'get adapter firmware \
			miscellaneous data' timeout \n", pACB->host->host_no);
		return false;
	}
	count = 8;
	while (count) {
		*acb_firm_model = readb(iop_firm_model);
		acb_firm_model++;
		iop_firm_model++;
		count--;
	}
	count = 16;
	while (count) {
		*acb_firm_version = readb(iop_firm_version);
		acb_firm_version++;
		iop_firm_version++;
		count--;
	}
	printk("Areca RAID Controller%d: F/W %s & Model %s\n",
		pACB->host->host_no,
		pACB->firm_version,
		pACB->firm_model);
	pACB->firm_request_len = readl(&reg->msgcode_rwbuffer[1]);   /*firm_request_len,1,04-07*/
	pACB->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[2]); /*firm_numbers_queue,2,08-11*/
	pACB->firm_sdram_size = readl(&reg->msgcode_rwbuffer[3]);    /*firm_sdram_size,3,12-15*/
	pACB->firm_hd_channels = readl(&reg->msgcode_rwbuffer[4]);  /*firm_ide_channels,4,16-19*/
	pACB->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	/*all interrupt service will be enable at arcmsr_iop_init*/
	return true;
}
/*
**********************************************************************************
**********************************************************************************
*/
static bool arcmsr_get_firmware_spec(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	if (acb->adapter_type == ACB_ADAPTER_TYPE_A)
		return arcmsr_get_hba_config(acb);
	else if (acb->adapter_type == ACB_ADAPTER_TYPE_B)
		return arcmsr_get_hbb_config(acb);
	else
		return arcmsr_get_hbc_config(acb);
}
/*
**********************************************************************************
**********************************************************************************
*/
#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_request_hba_device_map(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		if (unlikely(atomic_read(&acb->rq_map_token) == 0)|| ((acb->acb_flags & ACB_F_BUS_RESET) != 0 ) || ((acb->acb_flags & ACB_F_ABORT) != 0 )) {
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		} else {
			acb->fw_flag = FW_NORMAL;
			if (atomic_read(&acb->ante_token_value) == atomic_read(&acb->rq_map_token)) {
				atomic_set(&acb->rq_map_token, 16);
			}
			atomic_set(&acb->ante_token_value, atomic_read(&acb->rq_map_token));
			if (atomic_dec_and_test(&acb->rq_map_token)) {
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				return;
			}
			writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		}
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
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		if (unlikely(atomic_read(&acb->rq_map_token) == 0)|| ((acb->acb_flags & ACB_F_BUS_RESET) != 0 ) || ((acb->acb_flags & ACB_F_ABORT) != 0 )) {
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		} else {
			acb->fw_flag = FW_NORMAL;
			if (atomic_read(&acb->ante_token_value) == atomic_read(&acb->rq_map_token)) {
				atomic_set(&acb->rq_map_token, 16);
			}
			atomic_set(&acb->ante_token_value, atomic_read(&acb->rq_map_token));
			if (atomic_dec_and_test(&acb->rq_map_token)) {
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				return;
			}
			writel(ARCMSR_MESSAGE_GET_CONFIG, reg->drv2iop_doorbell);
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		}
		return;
	}
	/*
	**********************************************************************
	**********************************************************************
	*/
	static void arcmsr_request_hbc_device_map(struct AdapterControlBlock *acb)
	{
		struct MessageUnit_C __iomem *reg = acb->pmuC;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		if (unlikely(atomic_read(&acb->rq_map_token) == 0) || ((acb->acb_flags & ACB_F_BUS_RESET) != 0) || ((acb->acb_flags & ACB_F_ABORT) != 0)) {
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		} else {
			acb->fw_flag = FW_NORMAL;
			if (atomic_read(&acb->ante_token_value) == atomic_read(&acb->rq_map_token)) {
				atomic_set(&acb->rq_map_token, 16);
			}
			atomic_set(&acb->ante_token_value, atomic_read(&acb->rq_map_token));
			if (atomic_dec_and_test(&acb->rq_map_token)) {
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				return;
			}
			writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
			writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
			mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
		}
		return;
	}
	/*
	**********************************************************************
	**********************************************************************
	*/
	static void arcmsr_request_device_map(unsigned long pacb)
	{

		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {
				arcmsr_request_hba_device_map(acb);
			}
			break;
			case ACB_ADAPTER_TYPE_B: {
				arcmsr_request_hbb_device_map(acb);
			}
			break;
			case ACB_ADAPTER_TYPE_C: {
				arcmsr_request_hbc_device_map(acb);
			}
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
	
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	acb->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, &reg->inbound_msgaddr0);
	if (!arcmsr_hba_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", arcmsr_adapterNo);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_start_hbb_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	
	acb->acb_flags |= ACB_F_MSG_START_BGRB;
    	writel(ARCMSR_MESSAGE_START_BGRB, reg->drv2iop_doorbell);
	if (!arcmsr_hbb_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", arcmsr_adapterNo);
	}
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_start_hbc_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C *phbcmu = (struct MessageUnit_C *)pACB->pmuC;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	pACB->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, &phbcmu->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &phbcmu->inbound_doorbell);
	if (!arcmsr_hbc_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background \
				rebulid' timeout \n", pACB->host->host_no);
	}
	return;
}
/*
************************************************************************
**  start background rebulid
************************************************************************
*/
static void arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A:
		arcmsr_start_hba_bgrb(acb);
		break;
	case ACB_ADAPTER_TYPE_B:
		arcmsr_start_hbb_bgrb(acb);
		break;
	case ACB_ADAPTER_TYPE_C:
		arcmsr_start_hbc_bgrb(acb);
	}
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_polling_hba_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	struct CommandControlBlock *ccb;
	struct ARCMSR_CDB *arcmsr_cdb;
	uint32_t flag_ccb, outbound_intstatus, poll_ccb_done = 0, poll_count = 0;
	int id, lun, rtn;	
	bool error;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

 	polling_hba_ccb_retry:
	poll_count++;
	outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
	while (1) {
		/*if the outbound postqueue is empty, the value of -1 is returned.*/
		if ((flag_ccb = readl(&reg->outbound_queueport)) == 0xFFFFFFFF) {
			if (poll_ccb_done) {
				rtn = SUCCESS;
				break;
			} else {
				//arc_mdelay(25);
				if (poll_count > 100) {
					rtn = FAILED;
					break;
				}
				goto polling_hba_ccb_retry;
			}
		}
		arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));
		ccb = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
		poll_ccb_done = (ccb == poll_ccb)? 1:0;
		if ((ccb->acb != acb) || (ccb->startdone != ARCMSR_CCB_START)) {
			if ((ccb->startdone == ARCMSR_CCB_ABORTED) || (ccb == poll_ccb)) {
				id = ccb->pcmd->device->id;
				lun = ccb->pcmd->device->lun;
				printk(KERN_ERR "arcmsr%d: scsi id = %d lun = %d ccb = '0x%p'"
					" poll command abort successfully \n", arcmsr_adapterNo, ccb->pcmd->device->id, ccb->pcmd->device->lun, ccb);
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb);
				continue;
			}
			printk(KERN_ERR "arcmsr%d: polling get an illegal ccb"
				" command done ccb = '0x%p'"
				" ccboutstandingcount = %d \n"
				, arcmsr_adapterNo
				, ccb
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_report_ccb_state(acb, ccb, error);
	}
	return rtn;
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_polling_hbb_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	struct MessageUnit_B *reg = acb->pmuB;
	struct CommandControlBlock *ccb;
	struct ARCMSR_CDB *arcmsr_cdb;
	uint32_t flag_ccb, poll_ccb_done = 0, poll_count = 0;
	int32_t index;
	int id, lun, rtn;
	bool error;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	polling_hbb_ccb_retry:
	poll_count++;
	writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell); /* clear doorbell interrupt */
	while(1) {
		index = reg->doneq_index;
		if ((flag_ccb = reg->done_qbuffer[index]) == 0) {
			if (poll_ccb_done) {
				rtn = SUCCESS;
				break;
			} else {
				arc_mdelay(25);
				if (poll_count > 100) {
					rtn = FAILED;
					break;
				}
				goto polling_hbb_ccb_retry;
			}
		}
		writel(0, &reg->done_qbuffer[index]);
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;     /*if last index number set it to 0 */
		reg->doneq_index = index;
		/* check ifcommand done with no error*/
		arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));
		ccb = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
		poll_ccb_done = (ccb == poll_ccb) ? 1:0;
		if ((ccb->acb!=acb) || (ccb->startdone != ARCMSR_CCB_START)) {
			if ((ccb->startdone == ARCMSR_CCB_ABORTED) || (ccb == poll_ccb)) {
				id = ccb->pcmd->device->id;
				lun = ccb->pcmd->device->lun;
				printk(KERN_NOTICE "arcmsr%d: scsi id=%d lun=%d ccb='0x%p' poll command abort successfully \n"
					,arcmsr_adapterNo
					,ccb->pcmd->device->id
					,ccb->pcmd->device->lun
					,ccb);
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb);
				continue;
			}
			printk(KERN_NOTICE "arcmsr%d: polling get an illegal ccb"
				" command done ccb = '0x%p'"
				"ccboutstandingcount = %d \n"
				, arcmsr_adapterNo
				, ccb
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_report_ccb_state(acb, ccb, error);
	}	/*drain reply FIFO*/
	return rtn;
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_polling_hbc_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
	uint32_t flag_ccb, ccb_cdb_phy;
	struct ARCMSR_CDB *arcmsr_cdb;
	bool error;
	struct CommandControlBlock *pCCB;
	uint32_t poll_ccb_done = 0, poll_count = 0;
	int rtn;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	polling_hbc_ccb_retry:
	poll_count++;
	while (1) {
		if ((readl(&reg->host_int_status) & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) == 0) {
			if (poll_ccb_done) {
				rtn = SUCCESS;
				break;
			} else {
				msleep(25);
				if (poll_count > 100) {
					rtn = FAILED;
					break;
				}
				goto polling_hbc_ccb_retry;
			}
		}
		flag_ccb = readl(&reg->outbound_queueport_low);
		ccb_cdb_phy = (flag_ccb & 0xFFFFFFF0);
		arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);/*frame must be 32 bytes aligned*/
		pCCB = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
		poll_ccb_done = (pCCB == poll_ccb) ? 1 : 0;
		/* check if command done with no error*/
		if ((pCCB->acb != acb) || (pCCB->startdone != ARCMSR_CCB_START)) {
			if (pCCB->startdone == ARCMSR_CCB_ABORTED) {
				printk(KERN_NOTICE "arcmsr%d: scsi id = %d lun = %d ccb = '0x%p'"
					" poll command abort successfully \n"
					, acb->host->host_no
					, pCCB->pcmd->device->id
					, pCCB->pcmd->device->lun
					, pCCB);
					pCCB->pcmd->result = DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
				continue;
			}
			printk(KERN_NOTICE "arcmsr%d: polling get an illegal ccb"
				" command done ccb = '0x%p'"
				"ccboutstandingcount = %d \n"
				, acb->host->host_no
				, pCCB
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
		arcmsr_report_ccb_state(acb, pCCB, error);
	}
	return rtn;
}
/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	int rtn = 0;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A:{
			rtn = arcmsr_polling_hba_ccbdone(acb,poll_ccb);
		}
		break;
		case ACB_ADAPTER_TYPE_B:{
			rtn = arcmsr_polling_hbb_ccbdone(acb,poll_ccb);
		}
		break;
		case ACB_ADAPTER_TYPE_C: {
			rtn = arcmsr_polling_hbc_ccbdone(acb, poll_ccb);
		}
	}
	return rtn;
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb)
{
	int i = 0;
	uint32_t flag_ccb;
	struct ARCMSR_CDB *pARCMSR_CDB;
	bool error;
	struct CommandControlBlock *pCCB;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			uint32_t outbound_intstatus;
			outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
			/*clear and abort all outbound posted Q*/
			writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
			while (((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < ARCMSR_MAX_OUTSTANDING_CMD)) {
				pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
				pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
				error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
				arcmsr_drain_donequeue(acb, pCCB, error);
			}
		}
		break;

		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B *reg = acb->pmuB;
			/*clear all outbound posted Q*/
			writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);
			for (i = 0; i < ARCMSR_MAX_HBB_POSTQUEUE; i++) {
			if ((flag_ccb = readl(&reg->done_qbuffer[i])) != 0) {
				writel(0, &reg->done_qbuffer[i]);
				pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
				pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
				error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
				arcmsr_drain_donequeue(acb, pCCB, error);
			}
				reg->post_qbuffer[i] = 0;
			}
			reg->doneq_index = 0;
			reg->postq_index = 0;
		}
		break;

		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C *reg = acb->pmuC;
			struct  ARCMSR_CDB *pARCMSR_CDB;
			uint32_t flag_ccb, ccb_cdb_phy;
			bool error;
			struct CommandControlBlock *pCCB;
			while ((readl(&reg->host_int_status) & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) && (i++ < ARCMSR_MAX_OUTSTANDING_CMD)) {
				/*need to do*/
				flag_ccb = readl(&reg->outbound_queueport_low);
				ccb_cdb_phy = (flag_ccb & 0xFFFFFFF0);
				pARCMSR_CDB = (struct  ARCMSR_CDB *)(acb->vir2phy_offset+ccb_cdb_phy);/*frame must be 32 bytes aligned*/
				pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
				error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
				arcmsr_drain_donequeue(acb, pCCB, error);
			}
		}
	}
}
/*
**********************************************************************
**********************************************************************
*/	
static void arcmsr_iop_confirm(struct AdapterControlBlock *acb)
{
	uint32_t cdb_phyaddr, cdb_phyaddr_hi32;
	dma_addr_t dma_coherent_handle;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	/*
	********************************************************************
	** here we need to tell iop 331 our freeccb.HighPart 
	** if freeccb.HighPart is not zero
	********************************************************************
	*/
	dma_coherent_handle = acb->dma_coherent_handle;
	cdb_phyaddr = (uint32_t)(dma_coherent_handle);
	cdb_phyaddr_hi32 = (uint32_t)((cdb_phyaddr >> 16) >> 16);
	acb->cdb_phyaddr_hi32 = cdb_phyaddr_hi32;
	/*
	***********************************************************************
	**    if adapter type B, set window of "post command Q" 
	***********************************************************************
	*/	
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			if (cdb_phyaddr_hi32 != 0) {
				uint32_t intmask_org;
				struct MessageUnit_A __iomem *reg = acb->pmuA;
				intmask_org = arcmsr_disable_outbound_ints(acb);
				writel(ARCMSR_SIGNATURE_SET_CONFIG, &reg->msgcode_rwbuffer[0]);
				writel(cdb_phyaddr_hi32, &reg->msgcode_rwbuffer[1]);
				writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, &reg->inbound_msgaddr0);
				if (!arcmsr_hba_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "arcmsr%d: ""'set ccb high part physical address' timeout\n",
					arcmsr_adapterNo);
				}
				arcmsr_enable_outbound_ints(acb, intmask_org);
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			dma_addr_t post_queue_phyaddr;
			uint32_t __iomem *rwbuffer;
			uint32_t intmask_org;
			struct MessageUnit_B *reg = acb->pmuB;
			intmask_org = arcmsr_disable_outbound_ints(acb);
			reg->postq_index = 0;
			reg->doneq_index = 0;
			writel(ARCMSR_MESSAGE_SET_POST_WINDOW, reg->drv2iop_doorbell);
			if (!arcmsr_hbb_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d:can not set diver mode\n", arcmsr_adapterNo);
			}
			post_queue_phyaddr = acb->dma_coherent_handle_hbb_mu;
			rwbuffer = reg->msgcode_rwbuffer;
			writel(ARCMSR_SIGNATURE_SET_CONFIG, rwbuffer++);	/* driver "set config" signature		*/
			writel(cdb_phyaddr_hi32, rwbuffer++);			/* normal should be zero			*/
			writel(post_queue_phyaddr, rwbuffer++);		/* postQ size (256+8)*4			*/
			writel(post_queue_phyaddr + 1056, rwbuffer++);		/* doneQ size (256+8)*4			*/
			writel(1056, rwbuffer);				/* ccb maxQ size must be --> [(256+8)*4]	*/
			writel(ARCMSR_MESSAGE_SET_CONFIG, reg->drv2iop_doorbell);
			if (!arcmsr_hbb_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d: 'set command Q window' timeout \n", arcmsr_adapterNo);
			}
			arcmsr_hbb_enable_driver_mode(acb);
			arcmsr_enable_outbound_ints(acb, intmask_org);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			if (cdb_phyaddr_hi32 != 0) {
				struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
				writel(ARCMSR_SIGNATURE_SET_CONFIG, &reg->msgcode_rwbuffer[0]);
				writel(cdb_phyaddr_hi32, &reg->msgcode_rwbuffer[1]);
				writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, &reg->inbound_msgaddr0);
				writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
				if (!arcmsr_hbc_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "arcmsr%d: 'set command Q window'timeout \n", acb->host->host_no);
				}
			}
		}
	}
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_clear_doorbell_queue_buffer(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A *reg = acb->pmuA;
			uint32_t outbound_doorbell;
			/* empty doorbell Qbuffer if door bell ringed */
			outbound_doorbell = readl(&reg->outbound_doorbell);
			writel(outbound_doorbell, &reg->outbound_doorbell);/*clear doorbell interrupt */
			writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B *reg = acb->pmuB;
			writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
			writel(ARCMSR_DRV2IOP_DATA_READ_OK, reg->drv2iop_doorbell);
			/* let IOP know data has been read */
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C *reg = (struct MessageUnit_C *)acb->pmuC;
			uint32_t outbound_doorbell;
			/* empty doorbell Qbuffer if door bell ringed */
			outbound_doorbell = readl(&reg->outbound_doorbell);
			writel(outbound_doorbell, &reg->outbound_doorbell_clear);
			writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
		}
	}
}
/*
************************************************************************
**
************************************************************************
*/
static void arcmsr_enable_eoi_mode(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A:
			return;
		case ACB_ADAPTER_TYPE_B: {
		        	struct MessageUnit_B *reg = acb->pmuB;
				writel(ARCMSR_MESSAGE_ACTIVE_EOI_MODE, reg->drv2iop_doorbell);
				if (!arcmsr_hbb_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "ARCMSR IOP enables EOI_MODE TIMEOUT");
					return;
				}
			}
			break;
		case ACB_ADAPTER_TYPE_C:
			return;
	}
	return;
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_iop_init(struct AdapterControlBlock *acb)
{

    	uint32_t intmask_org;

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	intmask_org = arcmsr_disable_outbound_ints(acb);		/* disable all outbound interrupt */
	arcmsr_wait_firmware_ready(acb);
	arcmsr_iop_confirm(acb);
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	static void arcmsr_hardware_reset(struct AdapterControlBlock *acb)
	{
		uint8_t value[64];
		int i, count = 0;
		struct MessageUnit_A __iomem *reg = acb->pmuA;
		struct MessageUnit_C __iomem *pmuC = acb->pmuC;
		u32 temp = 0;

		printk(KERN_ERR "arcmsr%d: executing hw bus reset .....\n", arcmsr_adapterNo);
		/* backup pci config data */
		for (i = 0; i < 64; i++) {
			pci_read_config_byte(acb->pdev, i, &value[i]);
		}
		/* hardware reset signal */
		if ((acb->dev_id == 0x1680)) {
			writel(ARCMSR_ARC1680_BUS_RESET, &reg->reserved1[0]);
		} else if ((acb->dev_id == 0x1880)) {
			do {
				count++;
				writel(0xF, &pmuC->write_sequence);
				writel(0x4, &pmuC->write_sequence);
				writel(0xB, &pmuC->write_sequence);
				writel(0x2, &pmuC->write_sequence);
				writel(0x7, &pmuC->write_sequence);
				writel(0xD, &pmuC->write_sequence);
			} while ((((temp = readl(&pmuC->host_diagnostic)) | ARCMSR_ARC1880_DiagWrite_ENABLE) == 0) && (count < 5));
			writel(ARCMSR_ARC1880_RESET_ADAPTER, &pmuC->host_diagnostic);		
		} else {
			pci_write_config_byte(acb->pdev, 0x84, 0x20);
		}
		arc_mdelay(1000);
		/* write back pci config data */
		for (i = 0; i < 64; i++) {
			pci_write_config_byte(acb->pdev, i, value[i]);
		}
		arc_mdelay(1000);
		return;
	}
	/*
	****************************************************************************
	****************************************************************************
	*/
	static int arcmsr_abort_one_cmd(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb)
	{
		int rtn;
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		/*
		** Wait for 3 sec for all command done.
		*/
		SPIN_UNLOCK_IRQ_CHK(acb);
		spin_lock_irq(&acb->eh_lock);
		rtn = arcmsr_polling_ccbdone(acb, ccb);
		spin_unlock_irq(&acb->eh_lock);
		SPIN_LOCK_IRQ_CHK(acb);
		return rtn;
	}
	/*
	*****************************************************************************************
	*****************************************************************************************
	*/
	int arcmsr_abort(struct scsi_cmnd *cmd)
	{
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)cmd->device->host->hostdata;
		int i;
		int rtn = FAILED;

    		printk("scsi cmnd aborted, scsi_cmnd(0x%p), cmnd[0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x], scsi_id = 0x%2x, scsi_lun = 0x%2x.\n",
			cmd, 
			cmd->cmnd[0],
			cmd->cmnd[1],
			cmd->cmnd[2],
			cmd->cmnd[3],
			cmd->cmnd[4],
			cmd->cmnd[5],
			cmd->cmnd[6], 
			cmd->cmnd[7],
			cmd->cmnd[8],
			cmd->cmnd[9],
			cmd->cmnd[10], 
			cmd->cmnd[11],
			cmd->device->id,
			cmd->device->lun
			);
		acb->acb_flags |= ACB_F_ABORT;
		acb->num_aborts++;
		if (!atomic_read(&acb->ccboutstandingcount)) {
			acb->acb_flags &= ~ACB_F_ABORT;
			return SUCCESS;
		}
		for (i = 0; i <ARCMSR_MAX_FREECCB_NUM; i++) {
			struct CommandControlBlock *ccb = acb->pccb_pool[i];
			if (ccb->startdone == ARCMSR_CCB_START && ccb->pcmd == cmd) {
				ccb->startdone = ARCMSR_CCB_ABORTED;
				rtn = arcmsr_abort_one_cmd(acb, ccb);
				break;
			}
		}
		acb->acb_flags &= ~ACB_F_ABORT;
		return rtn;
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
	static uint8_t arcmsr_iop_reset(struct AdapterControlBlock *acb)
	{
		struct CommandControlBlock *ccb;
		uint32_t intmask_org;
		uint8_t rtnval = 0x00;
		int i = 0;
		unsigned long flags;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		SPIN_UNLOCK_IRQ_CHK(acb);
		if (atomic_read(&acb->ccboutstandingcount) != 0) {
			/* disable all outbound interrupt */
			intmask_org = arcmsr_disable_outbound_ints(acb);
			/* talk to iop 331 outstanding command aborted */
			rtnval = arcmsr_abort_allcmd(acb);
			/* clear all outbound posted Q */
			arcmsr_done4abort_postqueue(acb);
			for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
				ccb = acb->pccb_pool[i];
				if (ccb->startdone == ARCMSR_CCB_START) {
					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
						arcmsr_pci_unmap_dma(ccb);
					#endif
					ccb->startdone = ARCMSR_CCB_DONE;
					ccb->ccb_flags = 0;
					spin_lock_irqsave(&acb->ccblist_lock, flags);
					list_add_tail(&ccb->list, &acb->ccb_free_list);
					spin_unlock_irqrestore(&acb->ccblist_lock, flags);
				}
			}
			atomic_set(&acb->ccboutstandingcount, 0);
			/* enable all outbound interrupt */
			arcmsr_enable_outbound_ints(acb, intmask_org);
			SPIN_LOCK_IRQ_CHK(acb);
			return rtnval;
		}
		SPIN_LOCK_IRQ_CHK(acb);
		return rtnval;
	}
	/*
	****************************************************************************
	****************************************************************************
	*/
	int arcmsr_bus_reset(struct scsi_cmnd *cmd) {
		struct AdapterControlBlock *acb;
		uint32_t intmask_org, outbound_doorbell;
		int retry_count = 1;
		int rtn = FAILED;

		acb = (struct AdapterControlBlock *) cmd->device->host->hostdata;
		printk("arcmsr%d: executing eh bus reset .....num_resets = %d, num_aborts = %d \n", arcmsr_adapterNo, acb->num_resets, acb->num_aborts);
		arcmsr_touch_nmi_watchdog();
		acb->num_resets++;
		switch(acb->adapter_type){
			case ACB_ADAPTER_TYPE_A: {
				if (acb->acb_flags & ACB_F_BUS_RESET) {
					long timeout;
					printk(KERN_NOTICE "arcmsr%d: there is an eh bus reset proceeding.......\n", arcmsr_adapterNo);
					timeout = wait_event_timeout(wait_q, (acb->acb_flags & ACB_F_BUS_RESET) == 0, 220*HZ);
					if (timeout) {
						return SUCCESS;
					}
				}
				acb->acb_flags |= ACB_F_BUS_RESET;
				if (!arcmsr_iop_reset(acb)) {
					struct MessageUnit_A __iomem *reg;
					reg = acb->pmuA;
					SPIN_UNLOCK_IRQ_CHK(acb);
					arcmsr_hardware_reset(acb);
					acb->acb_flags &= ~ACB_F_IOP_INITED;
					sleep_again:
					#if ARCMSR_PP_RESET
						ssleep(ARCMSR_SLEEPTIME);
					#endif
					if ((readl(&reg->outbound_msgaddr1) & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0 ) {
							printk(KERN_ERR "arcmsr%d: waiting for hw bus reset return, retry=%d\n", acb->host->host_no, retry_count);
							if (retry_count > ARCMSR_RETRYCOUNT) {
							#if ARCMSR_FW_POLLING
								acb->fw_flag = FW_DEADLOCK;
							#endif
							printk(KERN_NOTICE "arcmsr%d: waiting for hw bus reset return, RETRY TERMINATED!!\n", arcmsr_adapterNo);
							SPIN_LOCK_IRQ_CHK(acb);
							return FAILED;
						}
						retry_count++;
						goto sleep_again;
					}
					acb->acb_flags |= ACB_F_IOP_INITED;
					/* disable all outbound interrupt */
					intmask_org = arcmsr_disable_outbound_ints(acb);
					arcmsr_get_firmware_spec(acb);
					arcmsr_start_adapter_bgrb(acb);
					/* clear Qbuffer if door bell ringed */
					outbound_doorbell = readl(&reg->outbound_doorbell);
					writel(outbound_doorbell, &reg->outbound_doorbell); /*clear interrupt */
	   				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
					/* enable outbound Post Queue,outbound doorbell Interrupt */
					arcmsr_enable_outbound_ints(acb, intmask_org);
					#if ARCMSR_FW_POLLING
						atomic_set(&acb->rq_map_token, 16);
						atomic_set(&acb->ante_token_value, 16);
						acb->fw_flag = FW_NORMAL;
						//init_timer(&acb->eternal_timer);
						//acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6*HZ);
						//acb->eternal_timer.data = (unsigned long) acb;
						//acb->eternal_timer.function = &arcmsr_request_device_map;
						mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6*HZ));
					#endif
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					rtn = SUCCESS;
					printk(KERN_NOTICE "arcmsr%d: scsi eh bus reset succeeds\n", arcmsr_adapterNo);
					SPIN_LOCK_IRQ_CHK(acb);
				} else {
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					#if ARCMSR_FW_POLLING
						//if(atomic_read(&acb->rq_map_token) == 0){
						//	atomic_set(&acb->rq_map_token, 16);
						//	atomic_set(&acb->ante_token_value, 16);
						//	acb->fw_flag = FW_NORMAL;
						//	init_timer(&acb->eternal_timer);
						//	acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6*HZ);
						//	acb->eternal_timer.data = (unsigned long) acb;
						//	acb->eternal_timer.function = &arcmsr_request_device_map;
						//	add_timer(&acb->eternal_timer);
						//}else{
							atomic_set(&acb->rq_map_token, 16);
							atomic_set(&acb->ante_token_value, 16);
							acb->fw_flag = FW_NORMAL;
							mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6*HZ));
						//}
					#endif
					rtn = SUCCESS;
				}
				break;
			}
			case ACB_ADAPTER_TYPE_B: {
				acb->acb_flags |= ACB_F_BUS_RESET;
				if (!arcmsr_iop_reset(acb)) {
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					rtn = FAILED;
				} else {
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					#if ARCMSR_FW_POLLING
						//if(atomic_read(&acb->rq_map_token) == 0){
						//	atomic_set(&acb->rq_map_token, 16);
						//	atomic_set(&acb->ante_token_value, 16);
						//	acb->fw_flag = FW_NORMAL;
						//	init_timer(&acb->eternal_timer);
						//	acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6*HZ);
						//	acb->eternal_timer.data = (unsigned long) acb;
						//	acb->eternal_timer.function = &arcmsr_request_device_map;
						//	add_timer(&acb->eternal_timer);
						//}else{
							atomic_set(&acb->rq_map_token, 16);
							atomic_set(&acb->ante_token_value, 16);
							acb->fw_flag = FW_NORMAL;
							mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6*HZ));
						//}
					#endif
					rtn = SUCCESS;
				}
				break;
			}
			case ACB_ADAPTER_TYPE_C: {
				if (acb->acb_flags & ACB_F_BUS_RESET) {
					long timeout;
					printk(KERN_NOTICE "arcmsr: there is an bus reset eh proceeding.......\n");
					timeout = wait_event_timeout(wait_q, (acb->acb_flags & ACB_F_BUS_RESET) == 0, 220*HZ);
					if (timeout) {
						return SUCCESS;
					}
				}
				acb->acb_flags |= ACB_F_BUS_RESET;
				if (!arcmsr_iop_reset(acb)) {
					struct MessageUnit_C __iomem *reg;
					reg = acb->pmuC;
					SPIN_UNLOCK_IRQ_CHK(acb);
					arcmsr_hardware_reset(acb);
					acb->acb_flags &= ~ACB_F_IOP_INITED;
					sleep:
					#if ARCMSR_PP_RESET
						ssleep(ARCMSR_SLEEPTIME);
					#endif
					if ((readl(&reg->host_diagnostic) & 0x04) != 0) {
							printk(KERN_ERR "arcmsr%d: waiting for hw bus reset return, retry=%d\n", acb->host->host_no, retry_count);
							if (retry_count > ARCMSR_RETRYCOUNT) {
							#if ARCMSR_FW_POLLING
							acb->fw_flag = FW_DEADLOCK;
							#endif
							printk(KERN_NOTICE "arcmsr%d: waiting for hw bus reset return, RETRY TERMINATED!!\n", acb->host->host_no);
							SPIN_LOCK_IRQ_CHK(acb);
							return FAILED;
						}
						retry_count++;
						goto sleep;
					}
					acb->acb_flags |= ACB_F_IOP_INITED;
					/* disable all outbound interrupt */
					intmask_org = arcmsr_disable_outbound_ints(acb);
					arcmsr_get_firmware_spec(acb);
					arcmsr_start_adapter_bgrb(acb);
					/* clear Qbuffer if door bell ringed */
					outbound_doorbell = readl(&reg->outbound_doorbell);
					writel(outbound_doorbell, &reg->outbound_doorbell_clear); /*clear interrupt */
					writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
					/* enable outbound Post Queue,outbound doorbell Interrupt */
					arcmsr_enable_outbound_ints(acb, intmask_org);
					#if ARCMSR_FW_POLLING
					atomic_set(&acb->rq_map_token, 16);
					atomic_set(&acb->ante_token_value, 16);					
					acb->fw_flag = FW_NORMAL;
					//init_timer(&acb->eternal_timer);
					//acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6 * HZ);
					//acb->eternal_timer.data = (unsigned long) acb;
					//acb->eternal_timer.function = &arcmsr_request_device_map;
					mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6*HZ));
					#endif
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					rtn = SUCCESS;					
					printk(KERN_NOTICE "arcmsr: scsi bus reset eh returns with success\n");
					SPIN_LOCK_IRQ_CHK(acb);
				} else {
					acb->acb_flags &= ~ACB_F_BUS_RESET;
					#if ARCMSR_FW_POLLING
						//if (atomic_read(&acb->rq_map_token) == 0) {
						//	atomic_set(&acb->rq_map_token, 16);
						//	atomic_set(&acb->ante_token_value, 16);
						//	acb->fw_flag = FW_NORMAL;
						//	init_timer(&acb->eternal_timer);
						//	acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6*HZ);
						//	acb->eternal_timer.data = (unsigned long) acb;
						//	acb->eternal_timer.function = &arcmsr_request_device_map;
						//	add_timer(&acb->eternal_timer);
						//} else {
							atomic_set(&acb->rq_map_token, 16);
							atomic_set(&acb->ante_token_value, 16);
							acb->fw_flag = FW_NORMAL;
							mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6*HZ));
						//}
					#endif
					rtn = SUCCESS;
				}
				break;
			}			
		}
		return rtn;
	}
#endif
/*
*****************************************************************************************
*****************************************************************************************
*/
const char *arcmsr_info(struct Scsi_Host *host)
{
	struct AdapterControlBlock *acb	= (struct AdapterControlBlock *) host->hostdata;
	static char buf[256];
	char *type;
	int raid6 = 1;
	
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
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
	case PCI_DEVICE_ID_ARECA_1680:
	case PCI_DEVICE_ID_ARECA_1681:
	case PCI_DEVICE_ID_ARECA_1880:
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
	/*
	*********************************************************************
	*********************************************************************
	*/
	static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb)
	{
		struct pci_dev *pdev = acb->pdev;
		
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		switch (acb->adapter_type) {

			case ACB_ADAPTER_TYPE_A: {

				void *dma_coherent;
				dma_addr_t dma_coherent_handle;
				struct CommandControlBlock *ccb_tmp;
				int i = 0, j = 0;
				dma_addr_t cdb_phyaddr;
				unsigned long roundup_ccbsize;
				unsigned long max_xfer_len;
				unsigned long max_sg_entrys;
				uint32_t  firm_config_version;

				for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
					for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
						acb->devstate[i][j] = ARECA_RAID_GONE;

				max_xfer_len = ARCMSR_MAX_XFER_LEN;
				max_sg_entrys = ARCMSR_DEFAULT_SG_ENTRIES;
				firm_config_version = acb->firm_cfg_version;
				#if ARCMSR_DEBUG_EXTDFW
					printk("firm_config_version = 0x%x....................................\n", firm_config_version);
				#endif
				if ((firm_config_version & 0xFF) >= 3) {
					max_xfer_len = (ARCMSR_CDB_SG_PAGE_LENGTH << ((firm_config_version >> 8) & 0xFF)) * 1024;/* max 16M byte */
					#if ARCMSR_DEBUG_EXTDFW
						printk("max_xfer_len = %ld....................................\n", max_xfer_len);
					#endif
					max_sg_entrys = (max_xfer_len/4096);			/* max 4096 sg entry*/
				}
				acb->host->max_sectors = max_xfer_len/512;
				acb->host->sg_tablesize = max_sg_entrys;
				roundup_ccbsize = roundup(sizeof(struct CommandControlBlock) + max_sg_entrys * sizeof(struct SG64ENTRY), 32);
				acb->uncache_size = roundup_ccbsize * ARCMSR_MAX_FREECCB_NUM;

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
					dma_coherent = dma_alloc_coherent(&pdev->dev, acb->uncache_size, &dma_coherent_handle, GFP_KERNEL);
				#else
					dma_coherent = pci_alloc_consistent(pdev, acb->uncache_size, &dma_coherent_handle);
				#endif

				if (!dma_coherent) {
					printk("arcmsr%d: dma_alloc_coherent got error\n", arcmsr_adapterNo);
					return -ENOMEM;
				}
				memset(dma_coherent, 0, acb->uncache_size);
				acb->dma_coherent = dma_coherent;
				acb->dma_coherent_handle = dma_coherent_handle;
				ccb_tmp = (struct CommandControlBlock *)dma_coherent;
				acb->vir2phy_offset = (unsigned long)dma_coherent - (unsigned long)dma_coherent_handle;
				for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
					#if ARCMSR_DEBUG_EXTDFW
						mdelay(15);
						printk("arcmsr%d: %d-th ccb address = 0x%p \n", arcmsr_adapterNo, i, ccb_tmp);
						mdelay(15);
					#endif
					cdb_phyaddr = dma_coherent_handle + offsetof(struct CommandControlBlock, arcmsr_cdb);
					ccb_tmp->shifted_cdb_phyaddr = cdb_phyaddr >> 5;
					#if ARCMSR_DEBUG_EXTDFW
						mdelay(15);
						printk("arcmsr%d: %d-th shifted_cdb_phyaddr = 0x%lx \n", arcmsr_adapterNo, i, ccb_tmp->shifted_cdb_phyaddr);
						mdelay(15);
					#endif
					acb->pccb_pool[i] = ccb_tmp;
					ccb_tmp->acb = acb;
					INIT_LIST_HEAD(&ccb_tmp->list);
					list_add_tail(&ccb_tmp->list, &acb->ccb_free_list);
					ccb_tmp = (struct CommandControlBlock *)((unsigned long)ccb_tmp + roundup_ccbsize);
					dma_coherent_handle = dma_coherent_handle + roundup_ccbsize;
				}
				break;
			}
			case ACB_ADAPTER_TYPE_B: {

				void *dma_coherent;
				dma_addr_t dma_coherent_handle;
				//dma_addr_t dma_addr;
				//uint32_t intmask_org;
				struct CommandControlBlock *ccb_tmp;
				uint32_t cdb_phyaddr;
				unsigned int roundup_ccbsize;
				unsigned long max_xfer_len;
				unsigned long max_sg_entrys;
				unsigned long firm_config_version;
				unsigned long max_freeccb_num=0;
				int i = 0, j = 0;

				max_freeccb_num = ARCMSR_MAX_FREECCB_NUM;
				max_xfer_len = ARCMSR_MAX_XFER_LEN;
				max_sg_entrys = ARCMSR_DEFAULT_SG_ENTRIES;
				firm_config_version = acb->firm_cfg_version;
				if ((firm_config_version & 0xFF) >= 3) {
					max_xfer_len = (ARCMSR_CDB_SG_PAGE_LENGTH << ((firm_config_version >> 8) & 0xFF)) * 1024;/* max 16M byte */
					max_sg_entrys = (max_xfer_len/4096);/* max 4097 sg entry*/
				}
				acb->host->max_sectors = max_xfer_len / 512;
				acb->host->sg_tablesize = max_sg_entrys;
				roundup_ccbsize = roundup(sizeof(struct CommandControlBlock)+ (max_sg_entrys-1)*sizeof(struct SG64ENTRY), 32);
				acb->uncache_size = roundup_ccbsize * ARCMSR_MAX_FREECCB_NUM;

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)		
					dma_coherent = dma_alloc_coherent(&pdev->dev,acb->uncache_size, &dma_coherent_handle, GFP_KERNEL);
				#else
					dma_coherent = pci_alloc_consistent(pdev, acb->uncache_size, &dma_coherent_handle);
				#endif

				if (!dma_coherent) {
					printk(KERN_NOTICE "DMA allocation failed...........................\n");
					return -ENOMEM;
				}
				memset(dma_coherent, 0, acb->uncache_size);
				acb->dma_coherent = dma_coherent;
				acb->dma_coherent_handle = dma_coherent_handle;
				ccb_tmp = (struct CommandControlBlock *)dma_coherent;
				acb->vir2phy_offset = (unsigned long)dma_coherent - (unsigned long)dma_coherent_handle;
				for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
					#if ARCMSR_DEBUG_EXTDFW
						printk("arcmsr%d: ccb = 0x%p \n", arcmsr_adapterNo, ccb_tmp);
						mdelay(15);
					#endif
					cdb_phyaddr = dma_coherent_handle + offsetof(struct CommandControlBlock, arcmsr_cdb);
					ccb_tmp->shifted_cdb_phyaddr = cdb_phyaddr >> 5;
					#if ARCMSR_DEBUG_EXTDFW
						printk("arcmsr%d: cdb_phyaddr = 0x%lx \n", arcmsr_adapterNo, cdb_phyaddr);
						printk("arcmsr%d: shifted_cdb_phyaddr = 0x%lx \n", arcmsr_adapterNo, ccb_tmp->shifted_cdb_phyaddr);
					#endif
					acb->pccb_pool[i] = ccb_tmp;
					ccb_tmp->acb = acb;
					INIT_LIST_HEAD(&ccb_tmp->list);
					list_add_tail(&ccb_tmp->list, &acb->ccb_free_list);
					ccb_tmp = (struct CommandControlBlock *)((unsigned long)ccb_tmp + roundup_ccbsize);
					dma_coherent_handle = dma_coherent_handle + roundup_ccbsize;
				}
				for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
					for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
						acb->devstate[i][j] = ARECA_RAID_GONE;
			}
			break;
			case ACB_ADAPTER_TYPE_C: {

				void *dma_coherent;
				dma_addr_t dma_coherent_handle;
				struct CommandControlBlock *ccb_tmp;
				int i = 0, j = 0;
				dma_addr_t cdb_phyaddr;
				unsigned long roundup_ccbsize;
				unsigned long max_xfer_len;
				unsigned long max_sg_entrys;
				uint32_t  firm_config_version;
				//unsigned long flags;

				for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
					for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
						acb->devstate[i][j] = ARECA_RAID_GONE;

				max_xfer_len = ARCMSR_MAX_XFER_LEN;
				max_sg_entrys = ARCMSR_DEFAULT_SG_ENTRIES;
				firm_config_version = acb->firm_cfg_version;
				#if ARCMSR_DEBUG_EXTDFW
					printk("firm_config_version = 0x%x....................................\n", firm_config_version);
				#endif
				if ((firm_config_version & 0xFF) >= 3) {
					max_xfer_len = (ARCMSR_CDB_SG_PAGE_LENGTH << ((firm_config_version >> 8) & 0xFF)) * 1024;/* max 16M byte */
					#if ARCMSR_DEBUG_EXTDFW
						printk("max_xfer_len = %ld....................................\n", max_xfer_len);
					#endif
					max_sg_entrys = (max_xfer_len/4096);			/* max 4096 sg entry*/
				}
				acb->host->max_sectors = max_xfer_len/512;
				acb->host->sg_tablesize = max_sg_entrys;
				roundup_ccbsize = roundup(sizeof(struct CommandControlBlock) + (max_sg_entrys - 1) * sizeof(struct SG64ENTRY), 32);
				acb->uncache_size = roundup_ccbsize * ARCMSR_MAX_FREECCB_NUM;

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
					dma_coherent = dma_alloc_coherent(&pdev->dev, acb->uncache_size, &dma_coherent_handle, GFP_KERNEL);
				#else
					dma_coherent = pci_alloc_consistent(pdev, acb->uncache_size, &dma_coherent_handle);
				#endif

				if (!dma_coherent) {
					printk("arcmsr%d: dma_alloc_coherent got error \n", arcmsr_adapterNo);
					return -ENOMEM;
				}
				memset(dma_coherent, 0, acb->uncache_size);
				acb->dma_coherent = dma_coherent;
				acb->dma_coherent_handle = dma_coherent_handle;
				acb->vir2phy_offset = (unsigned long)dma_coherent - (unsigned long)dma_coherent_handle;
				ccb_tmp = dma_coherent;
				for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
					#if ARCMSR_DEBUG_EXTDFW
						mdelay(15);
						printk("arcmsr%d: %d-th ccb address = 0x%p \n", arcmsr_adapterNo, i, ccb_tmp);
						mdelay(15);
					#endif
					cdb_phyaddr = dma_coherent_handle + offsetof(struct CommandControlBlock, arcmsr_cdb);
					ccb_tmp->shifted_cdb_phyaddr = cdb_phyaddr;
					#if ARCMSR_DEBUG_EXTDFW
						mdelay(15);
						printk("arcmsr%d: %d-th shifted_cdb_phyaddr = 0x%lx \n", arcmsr_adapterNo, i, ccb_tmp->shifted_cdb_phyaddr);
						mdelay(15);
					#endif
					acb->pccb_pool[i] = ccb_tmp;
					ccb_tmp->acb = acb;
					INIT_LIST_HEAD(&ccb_tmp->list);
					list_add_tail(&ccb_tmp->list, &acb->ccb_free_list);
					ccb_tmp = (struct CommandControlBlock *)((unsigned long)ccb_tmp + roundup_ccbsize);
					dma_coherent_handle = dma_coherent_handle + roundup_ccbsize;
				}
				break;
			}
		}
		return 0;
	}
#else
	static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb)
	{
		struct HCBARC *pHCBARC =& arcmsr_host_control_block;
		struct pci_dev *pdev = acb->pdev;
		
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
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

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)	
					dma_coherent = dma_alloc_coherent(&pdev->dev, ARCMSR_MAX_FREECCB_NUM *sizeof (struct CommandControlBlock) + 0x20, &dma_coherent_handle, GFP_KERNEL);
				#else
					dma_coherent = pci_alloc_consistent(pdev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + 0x20, &dma_coherent_handle);
				#endif
				if (!dma_coherent) {
					printk("arcmsr%d: dma_alloc_coherent got error \n", arcmsr_adapterNo);
					iounmap(acb->pmuA);
					return -ENOMEM;
				}

				acb->dma_coherent = dma_coherent;
				acb->dma_coherent_handle = dma_coherent_handle;
				memset(dma_coherent, 0, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock)+0x20);
				if (((unsigned long)dma_coherent & 0x1F) != 0) {
					dma_coherent = dma_coherent + (0x20 - ((unsigned long)dma_coherent & 0x1F));
					dma_coherent_handle = dma_coherent_handle + (0x20 - ((unsigned long)dma_coherent_handle & 0x1F));
				}

				dma_addr = dma_coherent_handle;
				ccb_tmp = (struct CommandControlBlock *)dma_coherent;
				for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
					ccb_tmp->shifted_cdb_phyaddr = dma_addr >> 5;
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

				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)		
					dma_coherent = dma_alloc_coherent(&pdev->dev,ARCMSR_MAX_FREECCB_NUM *sizeof (struct CommandControlBlock) + sizeof(struct MessageUnit_B) + 0x20, &dma_coherent_handle, GFP_KERNEL);
				#else
					dma_coherent = pci_alloc_consistent(pdev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + sizeof(struct MessageUnit_B) + 0x20, &dma_coherent_handle);
				#endif

				if (!dma_coherent) {
					printk(KERN_NOTICE "DMA allocation failed...........................\n");
					return -ENOMEM;
				}

				acb->dma_coherent = dma_coherent;
				acb->dma_coherent_handle = dma_coherent_handle;
				memset(dma_coherent, 0, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + sizeof(struct MessageUnit_B) + 0x20);

				if (((unsigned long)dma_coherent & 0x1F) != 0) {/*ccb address must 32 (0x20) boundary*/
					dma_coherent = dma_coherent + (0x20 - ((unsigned long)dma_coherent & 0x1F));
					dma_coherent_handle = dma_coherent_handle + (0x20 - ((unsigned long)dma_coherent_handle & 0x1F));
				}

				dma_addr = dma_coherent_handle;
				ccb_tmp = (struct CommandControlBlock *)dma_coherent;

				for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
					ccb_tmp->shifted_cdb_phyaddr = dma_addr >> 5;
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
				acb->mem_base0 = mem_base0;
				acb->mem_base1 = mem_base1;
				reg->drv2iop_doorbell = mem_base0 + ARCMSR_DRV2IOP_DOORBELL;
				reg->drv2iop_doorbell_mask = mem_base0 + ARCMSR_DRV2IOP_DOORBELL_MASK;
				reg->iop2drv_doorbell = mem_base0 + ARCMSR_IOP2DRV_DOORBELL;
				reg->iop2drv_doorbell_mask = mem_base0 + ARCMSR_IOP2DRV_DOORBELL_MASK;
				reg->message_wbuffer = mem_base1 + ARCMSR_MESSAGE_WBUFFER;
				reg->message_rbuffer =  mem_base1 + ARCMSR_MESSAGE_RBUFFER;
				reg->msgcode_rwbuffer = mem_base1 + ARCMSR_MESSAGE_RWBUFFER;
				for (i = 0; i < ARCMSR_MAX_TARGETID; i++)
					for (j = 0; j < ARCMSR_MAX_TARGETLUN; j++)
						acb->devstate[i][j] = ARECA_RAID_GONE;
				    		//intmask_org = arcmsr_disable_outbound_ints(acb);	
			}
			break;
		}
		acb->adapter_index = arcmsr_adapterNo;
		pHCBARC->acb[arcmsr_adapterNo] = acb;
		arcmsr_adapterNo++;
		return 0;
		
		out:
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0) 
			dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 3, 30)
			pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM) + 0x20 + sizeof(struct MessageUnit_B)), acb->dma_coherent, acb->dma_coherent_handle);
		#else
			kfree(acb->dma_coherent);
		#endif
		return -ENOMEM;
	}
#endif
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
#if (ARCMSR_PP_RESET && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static int arcmsr_set_info(char *buffer, int length, struct Scsi_Host *host) {
		struct scsi_device *pstSDev, *pstTmp;
		char *p;
		int iTmp;
		unsigned long flags;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif

		if (!strncmp("arcmsr ", buffer, 7)) {
			printk("arcmsr_set_info: arcmsr\n");
			if (!strncmp("devqueue", buffer + 7, 8)) {
				p = buffer + 16;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify dev queue from %d to %d\n",host->cmd_per_lun, iTmp);
				host->cmd_per_lun = iTmp;
				spin_lock_irqsave(host->host_lock, flags);
				list_for_each_entry_safe(pstSDev, pstTmp, &host->__devices, siblings) {
					pstSDev->queue_depth = iTmp;
				}
				spin_unlock_irqrestore(host->host_lock, flags);
			} else if (!strncmp("hostqueue", buffer + 7, 9)) {
				p = buffer + 17;
				iTmp = simple_strtoul(p, &p, 0);
				printk("modify host queue from %d to %d\n", host->can_queue, iTmp);
	            			host->can_queue = iTmp;
			}
		}
		return (length);
	}
#else
	static int arcmsr_set_info(char *buffer,int length) {
		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
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
	struct HCBARC *pHCBARC = &arcmsr_host_control_block;
	struct Scsi_Host *host;
	int poll_count = 0;
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	/* disable iop all outbound interrupt */
	#if (ARCMSR_FW_POLLING && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		//tasklet_kill(&acb->arcmsr_do_message_isr_bh);
		flush_scheduled_work();
		del_timer_sync(&acb->eternal_timer);
	#endif
    	arcmsr_disable_outbound_ints(acb);
	arcmsr_stop_adapter_bgrb(acb);
	arcmsr_flush_adapter_cache(acb);
	acb->acb_flags |= ACB_F_SCSISTOPADAPTER;
	acb->acb_flags &= ~ACB_F_IOP_INITED;
	for (poll_count = 0; poll_count < ARCMSR_MAX_OUTSTANDING_CMD; poll_count++){
		if (!atomic_read(&acb->ccboutstandingcount))
			break;
		arcmsr_interrupt(acb);/* FIXME: need spinlock */
		msleep(25);
	}

	if (atomic_read(&acb->ccboutstandingcount)) {
		int i;

		arcmsr_abort_allcmd(acb);
		arcmsr_done4abort_postqueue(acb);
		for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
			struct CommandControlBlock *ccb = acb->pccb_pool[i];
			if (ccb->startdone == ARCMSR_CCB_START) {
				ccb->startdone = ARCMSR_CCB_ABORTED;
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb);
			}
		}
	}
	host = acb->host;
    	pdev = acb->pdev;
	pHCBARC->acb[acb->adapter_index] = NULL; /* clear record */
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
		scsi_remove_host(host);
		scsi_host_put(host);
	#else
		scsi_unregister(host);
	#endif
	free_irq(pdev->irq, acb);
	arcmsr_free_ccb_pool(acb);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
		arcmsr_free_mu(acb);
	#endif
	arcmsr_unmap_pciregion(acb);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
}
/*
***************************************************************
***************************************************************
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 13)
	static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf)
	{
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC = &arcmsr_host_control_block;
		struct Scsi_Host *host;
		int i;

		#if ARCMSR_DEBUG_FUNCTION
			printk("%s:\n", __func__);
		#endif
		if ((event != SYS_RESTART) && (event != SYS_HALT) && (event != SYS_POWER_OFF)) {
			return NOTIFY_DONE;
		}
		for (i = 0;i<ARCMSR_MAX_ADAPTER;i++) {
			acb = pHCBARC->acb[i];
			if (acb == NULL) {
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
	int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
#else
	int arcmsr_proc_info(char *buffer, char ** start, off_t offset, int length, int hostno, int inout)
#endif
{
	uint8_t i;
	char * pos = buffer;
	struct AdapterControlBlock *acb;
	struct HCBARC *pHCBARC = &arcmsr_host_control_block;

	#if (ARCMSR_PP_RESET && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
		struct scsi_device *pstSDev, *pstTmp;
		unsigned long flags;
	#endif
	
	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	
   	if (inout) {
		#if (ARCMSR_PP_RESET && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
			return(arcmsr_set_info(buffer, length, host));
		#else
			return(arcmsr_set_info(buffer, length));
		#endif
	}
	
	for (i = 0;i < ARCMSR_MAX_ADAPTER; i++) {
		acb = pHCBARC->acb[i];
	    if (!acb) 
			continue;
		SPRINTF("ARECA SATA/SAS RAID Mass Storage Host Adadpter \n");
		SPRINTF("Driver Version %s ",ARCMSR_DRIVER_VERSION);
		SPRINTF("IRQ%d \n",acb->pdev->irq);
		SPRINTF("===========================\n"); 
	}

	#if (ARCMSR_PP_RESET && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
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
	if (pos - buffer < offset) {
		return 0;
	} else if(pos - buffer - offset < length) {
		return (pos - buffer - offset);
	} else {
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

	#if ARCMSR_DEBUG_FUNCTION
		printk("%s:\n", __func__);
	#endif
	if (!host) {
		return -ENXIO;
	}
	acb = (struct AdapterControlBlock *)host->hostdata;
	if (!acb) {
		return -ENXIO;
	}
	for (i = 0;i < ARCMSR_MAX_ADAPTER; i++) {
		if (pHCBARC->acb[i] == acb) {  
			match = i;
		}
	}
	if (match == 0xff) {
		return -ENODEV;
	}
	arcmsr_pcidev_disattach(acb);
	for (i = 0; i<ARCMSR_MAX_ADAPTER; i++) {
		if (pHCBARC->acb[i]) { 
			return 0;
		}
	}
	return 0;
}
