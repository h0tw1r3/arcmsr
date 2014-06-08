/*
*******************************************************************************
*	O.S		: Linux
*	FILE NAME	: arcmsr.c
*	Author		: Nick Cheng, C.L. Huang
*	E-mail		: support@areca.com.tw
*	Description	: SCSI RAID Device Driver for Areca RAID Controller
*******************************************************************************
*
* Copyright (C) 2007 - 2012, Areca Technology Corporation
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*******************************************************************************
* History
*
* REV#		DATE		NAME		DESCRIPTION
* 1.20.0X.15	23/08/2007	Nick Cheng
*						1.support ARC1200/1201/1202
* 1.20.0X.15	01/10/2007	Nick Cheng
*						1.add arcmsr_enable_eoi_mode()
* 1.20.0X.15	04/12/2007	Nick Cheng		
*						1.delete the limit of if dev_aborts[id][lun]>1, then
*							acb->devstate[id][lun] = ARECA_RAID_GONE in arcmsr_abort()
*							to wait for OS recovery on delicate HW
*						2.modify arcmsr_drain_donequeue() to ignore unknown command
*							and let kernel process command timeout.
*							This could handle IO request violating max. segments 
*							while Linux XFS over DM-CRYPT. 
*							Thanks to Milan Broz's comments <mbroz@redhat.com>
*
* 1.20.0X.15	24/12/2007	Nick Cheng
*						1.fix the portability problems
*						2.fix type B where we should _not_ iounmap() acb->pmu;
*							it's not ioremapped.
*						3.add return -ENOMEM if ioremap() fails
*						4.transfer IS_SG64_ADDR w/ cpu_to_le32() in arcmsr_build_ccb
*						5.modify acb->devstate[i][j] as ARECA_RAID_GONE 
*							instead of ARECA_RAID_GOOD in arcmsr_alloc_ccb_pool()
*						6.fix arcmsr_cdb->Context as (unsigned long)arcmsr_cdb
*						7.add the checking state of (outbound_intstatus &
*							ARCMSR_MU_OUTBOUND_HANDLE_INT) == 0 in arcmsr_hbaA_handle_isr
*						8.replace pci_alloc_consistent()/pci_free_consistent() 
*							with kmalloc()/kfree() in arcmsr_iop_message_xfer()
*						9. fix the release of dma memory for type B in arcmsr_free_ccb_pool()
*						10.fix the arcmsr_hbaB_polling_ccbdone()
* 1.20.0X.15	27/02/2008	Nick Cheng		
*						1.arcmsr_iop_message_xfer() is called from atomic context under the 
*							queuecommand scsi_host_template handler.
*							James Bottomley pointed out that the current
*							GFP_KERNEL|GFP_DMA flags are wrong: firstly we are in
*							atomic context, secondly this memory is not used for DMA.
*							Also removed some unneeded casts.
*							Thanks to Daniel Drake <dsd@gentoo.org>
* 1.20.0X.15	07/04/2008	Nick Cheng
*						1. add the function of HW reset for Type_A 
*							for kernel version greater than 2.6.0
*						2. add the function to automatic scan as the volume 
*							added or delected for kernel version greater than 2.6.0
*						3. support the notification of the FW status
*							to the AP layer for kernel version greater than 2.6.0
* 1.20.0X.15	03/06/2008	Nick Cheng				
* 						1. support SG-related functions after kernel-2.6.2x
* 1.20.0X.15	03/11/2008	Nick Cheng
*						1. fix the syntax error
* 						2. compatible to kernel-2.6.26
* 1.20.0X.15	06/05/2009	Nick Cheng
*						1. improve SCSI EH mechanism for ARC-1680 series
* 1.20.0X.15	02/06/2009	Nick Cheng
*						1. fix cli access unavailably issue on ARC-1200/1201
*							while a certain HD is unstable
* 1.20.0X.15	05/06/2009	Nick Cheng
*						1. support the maximum transfer size to 4M 
* 1.20.0X.15	09/12/2009	Nick Cheng
*						1. change the "for loop" for manipulating sg list to scsi_for_each_sg.
*							There is 127 entries per sg list. 
*							If the sg list entry is larget than 127, 
*							it will allocate the rest of entries in the other sg list on other page.
*							The old for-loop type could hit the bugs
*							if the second sg list is not in the subsequent page.
* 1.20.0X.15	05/10/2010	Nick Cheng
*						1. support ARC-1880 series on kernel 2.6.X
* 1.20.0X.15	07/27/2010	Nick Cheng
*						1. fix the system lock-up on Intel Xeon 55XX series, x86_64
* 1.20.0X.15	07/29/2010	Nick Cheng
*						1. revise the codes for scatter-gather applicable to RHEL5.0 to RHEL5.3
* 1.20.0X.15	10/08/2010	Nick Cheng
*						1. fix DMA memory allocation failure in Xen
*						2. use command->sc_data_direction instead of trying (incorrectly) to
*							figure it out from the command itself in arcsas_build_ccb()
* 1.20.0X.15	03/30/2011	Nick Cheng
*						1. increase the timeout value for every kind of scsi commands
*							during driver modules installation if needed it
* 1.20.0X.15	06/22/2011	Nick Cheng
*						1. change debug print
* 1.20.0X.15	08/30/2011	Nick Cheng
*						1. fix the bug of recovery from hibernation
* 1.20.0X.15	10/12/2011	Nick Cheng
*						1. fix some syntax error
* 1.20.0X.15	05/08/2012	Nick Cheng
*						1. support MSI/MSI-X
* 1.20.0X.15	09/13/2012	Nick Cheng
*						1. support ARC-1214,1224,1264,1284
* 1.20.0X.15	04/30/2013	C.L. Huang
*						1. Fixed auto request sense
*						2. Fixed bug of no use_sg in arcmsr_build_ccb
* 1.20.0X.15	05/07/2013	C.L. Huang
*						1. Fixed bug of out standing cmd full on ARC-12x4
* 1.20.0X.15	05/15/2013	C.L. Huang
*						1. Fixed bug of cmd throttling on ARC-188x
* 1.20.0X.15	05/31/2013	C.L. Huang
*						1. Fixed bug of updating F/W through Archttp
******************************************************************************************
*/
#define ARCMSR_DBG_FUNC		0
#define ARCMSR_DRIVER_VERSION	"v1.30.0X.16-20131015"
/*****************************************************************************************/
#if defined __KERNEL__
	#include <linux/version.h>
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif
	#define ARCMSR_SLEEPTIME	10
	#define ARCMSR_RETRYCOUNT	12
	#include <linux/module.h>
	#include <asm/dma.h>
	#include <asm/io.h>
	#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
		#include <asm/system.h>
	#endif
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
	#include <linux/pci_ids.h>
	#include <linux/moduleparam.h>
	#include <linux/blkdev.h>
	#include <linux/timer.h>
	#include <linux/reboot.h>
	#include <linux/sched.h>
	#include <linux/init.h>
	#include <linux/highmem.h>
	#include <linux/spinlock.h>
	#include <scsi/scsi.h>
	#include <scsi/scsi_host.h>
	#include <scsi/scsi_cmnd.h>
	#include <scsi/scsi_tcq.h>
	#include <scsi/scsi_device.h>
	#include <scsi/scsicam.h>
	#include "arcmsr.h"
#endif

MODULE_AUTHOR("Nick Cheng, Ching Huang <support@areca.com.tw>");
MODULE_DESCRIPTION("Areca SAS/SATA RAID Controller Driver");
MODULE_VERSION(ARCMSR_DRIVER_VERSION);
#ifdef MODULE_LICENSE
	MODULE_LICENSE("Dual BSD/GPL");
#endif
static wait_queue_head_t wait_q;
struct list_head rc_list;
LIST_HEAD(rc_list);
static int timeout = 0;
module_param(timeout, int, 0644);
MODULE_PARM_DESC(timeout, " scsi cmd timeout value (0 - 120 sec.)");
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 31)
	static int sdev_queue_depth = ARCMSR_SCSI_CMD_PER_DEV;
	static int arcmsr_set_sdev_queue_depth(const char *val, struct kernel_param *kp);
	module_param_call(sdev_queue_depth, arcmsr_set_sdev_queue_depth, param_get_int, &sdev_queue_depth, 0644);
	MODULE_PARM_DESC(sdev_queue_depth, " Max Device Queue Depth(default=256)");
#endif
static int msix_enable = 1;
module_param(msix_enable, int, 0644);
MODULE_PARM_DESC(msix_enable, " disable msix (default=1)");
static int arcmsr_module_init(void);
static void arcmsr_module_exit(void);
module_init(arcmsr_module_init);
module_exit(arcmsr_module_exit);

static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb);
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb);
static bool arcmsr_iop_init(struct AdapterControlBlock *acb);
static int arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb);
static bool arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb);
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock *acb);
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb);
static bool arcmsr_get_firmware_spec(struct AdapterControlBlock *acb);
static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb);
static bool arcmsr_define_adapter_type(struct AdapterControlBlock *acb);
static void arcmsr_request_device_map(unsigned long pacb);
static void arcmsr_hbaA_request_device_map(struct AdapterControlBlock *acb);
static void arcmsr_hbaB_request_device_map(struct AdapterControlBlock *acb);
static void arcmsr_hbaC_request_device_map(struct AdapterControlBlock *acb);
static void arcmsr_hbaD_request_device_map(struct AdapterControlBlock *acb);
static void arcmsr_hbaC_message_isr(struct AdapterControlBlock *acb);
static void arcmsr_hbaD_message_isr(struct AdapterControlBlock *acb);
static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb);
static void arcmsr_enable_outbound_ints(struct AdapterControlBlock *acb, u32 orig_mask);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20))
	static void arcmsr_message_isr_bh_fn(struct work_struct *work);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_message_isr_bh_fn(void *pacb);
#endif

static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb);
static int arcmsr_probe(struct pci_dev *pdev,const struct pci_device_id *id);
#ifdef CONFIG_PM
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
		static int arcmsr_suspend(struct pci_dev *pdev, pm_message_t state);
		static int arcmsr_resume(struct pci_dev *pdev);
	#endif
#endif
static void arcmsr_remove(struct pci_dev *pdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	static void arcmsr_shutdown(struct pci_dev *pdev);
#endif
static void arcmsr_hbaC_postqueue_isr(struct AdapterControlBlock *acb);
static void arcmsr_hbaC_doorbell_isr(struct AdapterControlBlock *pACB);
static void arcmsr_iop2drv_data_wrote_handle(struct AdapterControlBlock *acb);
static void arcmsr_iop2drv_data_read_handle(struct AdapterControlBlock *acb);
static bool arcmsr_iop_confirm(struct AdapterControlBlock *acb);
static bool arcmsr_enable_eoi_mode(struct AdapterControlBlock *acb);
static uint8_t arcmsr_iop_reset(struct AdapterControlBlock *acb);
int arcmsr_release(struct Scsi_Host *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
	int arcmsr_queue_command(struct Scsi_Host *h, struct scsi_cmnd *cmd);
#else
	int arcmsr_queue_command(struct scsi_cmnd *cmd, void (* done)(struct scsi_cmnd *));
#endif
int arcmsr_abort(struct scsi_cmnd *cmd);
int arcmsr_bus_reset(struct scsi_cmnd *cmd);
const char *arcmsr_info(struct Scsi_Host *);
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 8)
int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
#endif
int arcmsr_bios_param(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int *info);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
	int arcmsr_adjust_disk_queue_depth(struct scsi_device *sdev, int queue_depth, int reason)
	{
		if (reason != SCSI_QDEPTH_DEFAULT)
			return -EOPNOTSUPP;
		if (queue_depth > ARCMSR_MAX_CMD_PERLUN)
			queue_depth = ARCMSR_MAX_CMD_PERLUN;
		scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), queue_depth);
		return queue_depth;
	}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
	static int arcmsr_adjust_disk_queue_depth(struct scsi_device *sdev, int queue_depth)
	{
		if (queue_depth > ARCMSR_MAX_CMD_PERLUN)
			queue_depth = ARCMSR_MAX_CMD_PERLUN;
		scsi_adjust_queue_depth(sdev, MSG_ORDERED_TAG, queue_depth);
		return queue_depth;
	}
#else
	static ssize_t arcmsr_adjust_disk_queue_depth(struct device *dev, const char *buf, size_t count)
	{
		int queue_depth;
		struct scsi_device *sdev = to_scsi_device(dev);

		queue_depth = simple_strtoul(buf, NULL, 0);
		if (queue_depth > ARCMSR_MAX_CMD_PERLUN)
			return -EINVAL;
		scsi_adjust_queue_depth(sdev, MSG_ORDERED_TAG, queue_depth);
		return count;
	}
	static struct device_attribute arcmsr_queue_depth_attr = 
	{
		.attr = {
			.name =	"queue_depth",
			.mode =	S_IRUSR | S_IWUSR,
		},
		.store = arcmsr_adjust_disk_queue_depth
	};
	static struct device_attribute *arcmsr_scsi_device_attr[] = 
	{
		&arcmsr_queue_depth_attr,
		NULL,
	};
#endif

static struct scsi_host_template arcmsr_scsi_host_template = {
	.module		= THIS_MODULE,
	.proc_name	= "arcmsr",
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 8)
	.proc_info	= arcmsr_proc_info,
#endif
	.name		= "Areca SAS/SATA RAID Controller" ARCMSR_DRIVER_VERSION,  /* *name */
	.release	= arcmsr_release,
	.info		= arcmsr_info,
	.queuecommand	= arcmsr_queue_command,
	.eh_abort_handler		= arcmsr_abort,
	.eh_device_reset_handler	= NULL,	
	.eh_bus_reset_handler		= arcmsr_bus_reset,
	.eh_host_reset_handler		= NULL,	
	.bios_param	= arcmsr_bios_param,	
	.can_queue	= ARCMSR_MAX_FREECCB_NUM,
	.this_id	= ARCMSR_SCSI_INITIATOR_ID,
	//.sg_tablesize	= ARCMSR_DEFAULT_SG_ENTRIES, 
	//.max_sectors	= ARCMSR_MAX_XFER_SECTORS_C, 
	.cmd_per_lun	= 128,
	.unchecked_isa_dma	= 0,
	.use_clustering		= ENABLE_CLUSTERING,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	.shost_attrs	= arcmsr_scsi_host_attr,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 11)
	.change_queue_depth = arcmsr_adjust_disk_queue_depth,
#else
	.sdev_attrs	= arcmsr_scsi_device_attr,
#endif
};

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
	{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1214)},
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
	.name		= "arcmsr",
	.id_table	= arcmsr_device_id_table,
	.probe		= arcmsr_probe,
	.remove		= arcmsr_remove,
#ifdef CONFIG_PM
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
	.suspend	= arcmsr_suspend,
	.resume		= arcmsr_resume,
	#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
    .shutdown	= arcmsr_shutdown,
#endif
};

static int arcmsr_module_init(void)
{
	int error = 0;
	error = pci_register_driver(&arcmsr_pci_driver);
	return error;
}

static void arcmsr_module_exit(void)
{
	pci_unregister_driver(&arcmsr_pci_driver);
}

static void arcmsr_free_mu(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_C:
			break;
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			dma_free_coherent(&acb->pdev->dev, sizeof(struct MessageUnit_B), reg, acb->dma_coherent_handle2);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			dma_free_coherent(&acb->pdev->dev, acb->roundup_ccbsize, acb->dma_coherent2, acb->dma_coherent_handle2);
			break;
		}
	}
}

static bool arcmsr_remap_pciregion(struct AdapterControlBlock *acb)
{
	struct pci_dev *pdev = acb->pdev;

	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A: {
			acb->pmuA = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
			if (!acb->pmuA) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
				return false;
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			void __iomem *mem_base0, *mem_base1;
			mem_base0 = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
			if (!mem_base0) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
				return false;
			}
			mem_base1 = ioremap(pci_resource_start(pdev, 2), pci_resource_len(pdev, 2));
			if (!mem_base1) {
				iounmap(mem_base0);
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
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
		case ACB_ADAPTER_TYPE_D: {
			void __iomem *mem_base0;
			unsigned long addr, range, flags;
			
			addr = (unsigned long)pci_resource_start(pdev, 0);
			range = pci_resource_len(pdev, 0);
			flags = pci_resource_flags(pdev, 0);
			if (flags & IORESOURCE_CACHEABLE) {
				mem_base0 = ioremap(addr, range);
			} else {
				mem_base0 = ioremap_nocache(addr, range);
			}
			if (!mem_base0) {
				printk(KERN_NOTICE "arcmsr%d: memory mapping region fail \n", acb->host->host_no);
				return false;
			}
			acb->mem_base0 = mem_base0;
			break;
		}
	}
	return true;
}

static void arcmsr_unmap_pciregion(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			iounmap(acb->pmuA);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			iounmap(acb->mem_base0);
			iounmap(acb->mem_base1);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			iounmap(acb->pmuC);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			iounmap(acb->mem_base0);
			break;
		}
	}
}

static void arcmsr_wait_firmware_ready(struct AdapterControlBlock *acb)
{
	uint32_t firmware_state = 0;

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			do {
				firmware_state = readl(&reg->outbound_msgaddr1);
			} while ((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			do {
				firmware_state = readl(reg->iop2drv_doorbell);
			} while ((firmware_state & ARCMSR_MESSAGE_FIRMWARE_OK) == 0);
			writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			do {
				firmware_state = readl(&reg->outbound_msgaddr1);
			} while ((firmware_state & ARCMSR_HBCMU_MESSAGE_FIRMWARE_OK) == 0);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			do {
				firmware_state = readl(reg->outbound_msgaddr1);
			} while ((firmware_state & ARCMSR_ARC1214_MESSAGE_FIRMWARE_OK) == 0);
			break;
		}
	}
}

static bool arcmsr_hbaA_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	int i;
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	for (i = 0; i < 2000; i++) {
		if (readl(&reg->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) {
			writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}

static bool arcmsr_hbaB_wait_msgint_ready(struct AdapterControlBlock *acb)
{
	int i;
	struct MessageUnit_B __iomem *reg = acb->pmuB;

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

static bool arcmsr_hbaC_wait_msgint_ready(struct AdapterControlBlock *pACB)
{
	int i;
	struct MessageUnit_C __iomem *phbcmu = pACB->pmuC;

	for (i = 0; i < 2000; i++) {
		if (readl(&phbcmu->outbound_doorbell) & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
			writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &phbcmu->outbound_doorbell_clear); /*clear interrupt*/
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}

static bool arcmsr_hbaD_wait_msgint_ready(struct AdapterControlBlock *pACB)
{
	int i;
	struct MessageUnit_D __iomem *reg = pACB->pmuD;

	for (i = 0; i < 2000; i++) {
		if (readl(reg->outbound_doorbell) & ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE) {
			writel(ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE, reg->outbound_doorbell); /*clear interrupt*/
			return true;
		}
		msleep(10);
	} /* max 20 seconds */
	return false;
}

static void arcmsr_clear_doorbell_queue_buffer(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			uint32_t outbound_doorbell;
			/* empty doorbell Qbuffer if door bell ringed */
			outbound_doorbell = readl(&reg->outbound_doorbell);
			writel(outbound_doorbell, &reg->outbound_doorbell);/*clear doorbell interrupt */
			writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
			writel(ARCMSR_DRV2IOP_DATA_READ_OK, reg->drv2iop_doorbell);
			/* let IOP know data has been read */
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			uint32_t outbound_doorbell, i;
			/* empty doorbell Qbuffer if door bell ringed */
			outbound_doorbell = readl(&reg->outbound_doorbell);
			writel(outbound_doorbell, &reg->outbound_doorbell_clear);
			writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
			readl(&reg->outbound_doorbell_clear);/* Dummy readl to force pci flush */
			readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
			for(i=0; i < 200; i++) {
				msleep(10);
				outbound_doorbell = readl(&reg->outbound_doorbell);
				if( outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_OK) {
					writel(outbound_doorbell, &reg->outbound_doorbell_clear);
					writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
				} else
					break;
			}
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			uint32_t outbound_doorbell, i;
			/* empty doorbell Qbuffer if door bell ringed */
			outbound_doorbell = readl(reg->outbound_doorbell);
			writel(outbound_doorbell, reg->outbound_doorbell);
			writel(ARCMSR_ARC1214_DRV2IOP_DATA_OUT_READ, reg->inbound_doorbell);
			for(i=0; i < 200; i++) {
				msleep(10);
				outbound_doorbell = readl(reg->outbound_doorbell);
				if( outbound_doorbell & ARCMSR_ARC1214_IOP2DRV_DATA_WRITE_OK) {
					writel(outbound_doorbell, reg->outbound_doorbell);
					writel(ARCMSR_ARC1214_DRV2IOP_DATA_OUT_READ, reg->inbound_doorbell);
				} else
					break;
			}
			break;
		}
	}
}

static void arcmsr_touch_nmi_watchdog(void)
{
	#ifdef CONFIG_X86_64
		touch_nmi_watchdog();
	#endif

	#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 13)
		touch_softlockup_watchdog();
	#endif
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
	static irqreturn_t arcmsr_do_interrupt(int irq, void *dev_id)
	{
		return arcmsr_interrupt((struct AdapterControlBlock *)dev_id);
	}
#else
	static irqreturn_t arcmsr_do_interrupt(int irq, void *dev_id, struct pt_regs *regs)
	{
		return arcmsr_interrupt((struct AdapterControlBlock *)dev_id);
	}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20))
	static void arcmsr_message_isr_bh_fn(struct work_struct *work) 
	{
		struct AdapterControlBlock *acb = container_of(work, struct AdapterControlBlock, arcmsr_do_message_isr_bh);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	static void arcmsr_message_isr_bh_fn(void *pacb) 
	{
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;
#endif
		char *acb_dev_map = (char *)acb->device_map;
		uint32_t __iomem *signature = NULL;
		char __iomem *devicemap = NULL;
		int target, lun;
		struct scsi_device *psdev;
		char diff, temp;

		switch (acb->adapter_type) {
			case ACB_ADAPTER_TYPE_A: {
				struct MessageUnit_A __iomem *reg  = acb->pmuA;
				signature = (uint32_t __iomem*) (&reg->msgcode_rwbuffer[0]);
				devicemap = (char __iomem*) (&reg->msgcode_rwbuffer[21]);
				break;
			}
			case ACB_ADAPTER_TYPE_B: {
				struct MessageUnit_B __iomem *reg  = acb->pmuB;
				signature = (uint32_t __iomem*)(&reg->msgcode_rwbuffer[0]);
				devicemap = (char __iomem*)(&reg->msgcode_rwbuffer[21]);
				break;
			}
			case ACB_ADAPTER_TYPE_C: {
				struct MessageUnit_C __iomem *reg  = acb->pmuC;
				signature = (uint32_t __iomem *)(&reg->msgcode_rwbuffer[0]);
				devicemap = (char __iomem *)(&reg->msgcode_rwbuffer[21]);
				break;
			}
			case ACB_ADAPTER_TYPE_D: {
				struct MessageUnit_D __iomem *reg  = acb->pmuD;
				signature = (uint32_t __iomem *)(&reg->msgcode_rwbuffer[0]);
				devicemap = (char __iomem *)(&reg->msgcode_rwbuffer[21]);
				break;
			}
		}
		atomic_inc(&acb->rq_map_token);
		if (readl(signature) == ARCMSR_SIGNATURE_GET_CONFIG) {
			for (target = 0; target < ARCMSR_MAX_TARGETID -1; target++) {
				temp = readb(devicemap);
				diff = (*acb_dev_map) ^ temp;
				if (diff != 0) {
					*acb_dev_map = temp;
					for (lun = 0; lun < ARCMSR_MAX_TARGETLUN; lun++) {
						if ((diff & 0x01) == 1 && (temp & 0x01) == 1) {	
							scsi_add_device(acb->host, 0, target, lun);
						} else if ((diff & 0x01) == 1 && (temp & 0x01) == 0) {
							psdev = scsi_device_lookup(acb->host, 0, target, lun);
							if (psdev != NULL ) {
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

#ifdef CONFIG_PM
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 18)
	static int arcmsr_suspend(struct pci_dev *pdev, pm_message_t state)
	{
		int i;
		uint32_t intmask_org;
		struct Scsi_Host *host = pci_get_drvdata(pdev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;

		intmask_org = arcmsr_disable_outbound_ints(acb);
		if (acb->acb_flags & ACB_F_MSI_ENABLED) {
			free_irq(pdev->irq, acb);
			pci_disable_msi(pdev);
		} else if (acb->acb_flags & ACB_F_MSIX_ENABLED) {
			for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
				free_irq(acb->entries[i].vector, acb);
			}
			pci_disable_msix(pdev);
		} else {
			free_irq(pdev->irq, acb);
		}
		del_timer_sync(&acb->eternal_timer);
		flush_scheduled_work();
		arcmsr_stop_adapter_bgrb(acb);
		arcmsr_flush_adapter_cache(acb);
		arcmsr_enable_outbound_ints(acb, intmask_org);
		pci_set_drvdata(pdev, host);
		pci_save_state(pdev);
		pci_disable_device(pdev);
		pci_set_power_state(pdev, pci_choose_state(pdev, state));
		return 0;
	}
	
	static int arcmsr_resume(struct pci_dev *pdev)
	{
		int error, i, j;
		struct Scsi_Host *host = pci_get_drvdata(pdev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
		struct msix_entry entries[ARCMST_NUM_MSIX_VECTORS];
		pci_set_power_state(pdev, PCI_D0);
		pci_enable_wake(pdev, PCI_D0, 0);
		pci_restore_state(pdev);
		if (pci_enable_device(pdev)) {
			printk("%s: pci_enable_device error \n", __func__);
			return -ENODEV;
		}
		error = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
		if (error) {
			error = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
			if (error) {
				printk(KERN_WARNING
				       "scsi%d: No suitable DMA mask available\n",
				       host->host_no);
				goto controller_unregister;
			}
		}
		pci_set_master(pdev);
		if (pci_find_capability(pdev, PCI_CAP_ID_MSIX) && msix_enable) {
			int r;
			for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
				entries[i].entry = i;
			}
			r = pci_enable_msix(pdev, entries, ARCMST_NUM_MSIX_VECTORS);
			if (r >= 0) {
				if (r == 0) {
					for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
						if (request_irq(entries[i].vector, arcmsr_do_interrupt, 0, "arcmsr", acb)) {
							printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
							for (j = 0 ; j < i ; j++)
								free_irq(entries[i].vector, acb);
							goto controller_stop;
						}
						acb->entries[i] = entries[i];
					}
				} else {
					if (!pci_enable_msix(pdev, entries, r)) {
						for (i = 0; i < r; i++) {
							if (request_irq(entries[i].vector, arcmsr_do_interrupt, 0, "arcmsr", acb)) {
								printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
								for (j = 0 ; j < i ; j++)
									free_irq(entries[i].vector, acb);
								goto controller_stop;
							}
							acb->entries[i] = entries[i];
						}
						acb->acb_flags |= ACB_F_MSIX_ENABLED;
						printk("arcmsr%d: msi-x enabled\n", acb->host->host_no);
					} else {
						if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", acb)) {
							printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
							goto controller_stop;
						}
						printk("arcmsr%d: enabled legacy interrupt(%d) \n", acb->host->host_no, __LINE__);
					}
				}
			} else {
				if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", acb)) {
					printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
					goto controller_stop;
				}
				printk("arcmsr%d: msi-x failed(%d) \n", acb->host->host_no, r);
				printk("arcmsr%d: enabled legacy interrupt(%d) \n", acb->host->host_no, __LINE__);
			}
		} else if (pci_find_capability(pdev, PCI_CAP_ID_MSI) && msix_enable) {
			if (!pci_enable_msi(pdev)) {
				acb->acb_flags |= ACB_F_MSI_ENABLED;
			}
			if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", acb)) {
				printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
				goto controller_stop;
			}
			printk("arcmsr%d: msi enabled\n", acb->host->host_no);
		} else {
			if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", acb)) {
				printk("arcmsr%d: request_irq =%d failed!\n", acb->host->host_no, pdev->irq);
				goto controller_stop;
			}
			printk("arcmsr%d: enabled legacy interrupt(%d) \n", acb->host->host_no, __LINE__);
		}
		arcmsr_iop_init(acb);
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
		acb->eternal_timer.data = (unsigned long)acb;
		acb->eternal_timer.function = &arcmsr_request_device_map;
		add_timer(&acb->eternal_timer);
		return 0;
		controller_stop:
			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);
		controller_unregister:
			scsi_remove_host(host);
			arcmsr_free_ccb_pool(acb);
			arcmsr_unmap_pciregion(acb);
			pci_release_regions(pdev);
			scsi_host_put(host);	
			pci_disable_device(pdev);
		return -ENODEV;
	}
	#endif
#endif

static int arcmsr_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	uint8_t bus, dev_fun;
	int error, i, j;
	struct Scsi_Host *host;
	struct AdapterControlBlock *pacb;
	struct msix_entry entries[ARCMST_NUM_MSIX_VECTORS];
	if (pci_enable_device(pdev)) {
		printk("adapter probe: pci_enable_device error \n");
		return -ENODEV;
	}
	if ((host = scsi_host_alloc(&arcmsr_scsi_host_template, sizeof(struct AdapterControlBlock))) == 0) {
		printk("adapter probe: scsi_host_alloc error \n");
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
	pacb = (struct AdapterControlBlock *)host->hostdata;
	INIT_LIST_HEAD(&pacb->list);
	memset(pacb, 0, sizeof(struct AdapterControlBlock));
	pacb->pdev = pdev;
	pacb->host = host;
	host->max_lun = ARCMSR_MAX_TARGETLUN;
	host->max_id = ARCMSR_MAX_TARGETID;
	host->max_cmd_len = 16;	 			/*this is issue of 64bit LBA ,over 2T byte*/
	host->can_queue = ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
	host->cmd_per_lun = ARCMSR_MAX_CMD_PERLUN;	    
	host->this_id = ARCMSR_SCSI_INITIATOR_ID;
	host->unique_id = (bus << 8) | dev_fun;
	pci_set_drvdata(pdev, host);
	pci_set_master(pdev);
	if (pci_request_regions(pdev, "arcmsr")) {
		printk("arcmsr%d: pci_request_regions failed \n", pacb->host->host_no);
		goto pci_disable_dev;
	}
	spin_lock_init(&pacb->eh_lock);
	spin_lock_init(&pacb->ccblist_lock);
	spin_lock_init(&pacb->postq_lock);
	spin_lock_init(&pacb->doneq_lock);
	spin_lock_init(&pacb->rqbuffer_lock);
	spin_lock_init(&pacb->wqbuffer_lock);
	pacb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_RQBUFFER_CLEARED |ACB_F_MESSAGE_WQBUFFER_READED);
	INIT_LIST_HEAD(&pacb->ccb_free_list);
	if (!arcmsr_define_adapter_type(pacb)) {
		printk("arcmsr%d: arcmsr_define_adapter_type got error \n", pacb->host->host_no);
		goto pci_release_regs;
	}
	if (!arcmsr_remap_pciregion(pacb)) {
		printk("arcmsr%d: arcmsr_remap_pciregion got error \n", pacb->host->host_no);
		goto pci_release_regs;
	}
	if (!arcmsr_get_firmware_spec(pacb)) {
		printk("arcmsr%d: arcmsr_get_firmware_spec got error \n", pacb->host->host_no);
		goto unmap_pci_region;
	}
	if (arcmsr_alloc_ccb_pool(pacb)) {
		printk("arcmsr%d: arcmsr_alloc_ccb_pool got error \n", pacb->host->host_no);
		goto free_mu;
	}
	if (scsi_add_host(host, &pdev->dev)) {
		printk("arcmsr%d: scsi_add_host got error \n", pacb->host->host_no);
		goto RAID_controller_stop;
	}
	if (pci_find_capability(pdev, PCI_CAP_ID_MSIX) && msix_enable) {
		int r;
		for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
			entries[i].entry = i;
		}
		r = pci_enable_msix(pdev, entries, ARCMST_NUM_MSIX_VECTORS);
		if (r >= 0) {
			if (r == 0) {
				for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
					if (request_irq(entries[i].vector, arcmsr_do_interrupt, 0, "arcmsr", pacb)) {
						printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
						for (j = 0 ; j < i ; j++)
							free_irq(entries[i].vector, pacb);
						goto scsi_host_remove;
					}
					pacb->entries[i] = entries[i];
				}
				pacb->acb_flags |= ACB_F_MSIX_ENABLED;
				printk("arcmsr%d: msi-x enabled\n", pacb->host->host_no);
			} else {
				if (!pci_enable_msix(pdev, entries, r)) {
					for (i = 0; i < r; i++) {
						if (request_irq(entries[i].vector, arcmsr_do_interrupt, 0, "arcmsr", pacb)) {
							printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
							for (j = 0 ; j < i ; j++)
								free_irq(entries[i].vector, pacb);
							goto scsi_host_remove;
						}
						pacb->entries[i] = entries[i];
					}
					pacb->acb_flags |= ACB_F_MSIX_ENABLED;
					printk("arcmsr%d: msi-x enabled\n", pacb->host->host_no);
				} else {
					if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", pacb)) {
						printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
						goto scsi_host_remove;
					}
					printk("arcmsr%d: fail to enable msi-x (%d)\n", pacb->host->host_no, r);
					printk("arcmsr%d: enabled legacy interrupt(%d)\n", pacb->host->host_no, __LINE__);
				}
			}
		} else {
			if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", pacb)) {
				printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
				goto scsi_host_remove;
			}
			printk("arcmsr%d: fail to enable msi-x (%d)\n", pacb->host->host_no, r);
			printk("arcmsr%d: enabled legacy interrupt(%d) \n", pacb->host->host_no, __LINE__);
		}
	} else if (pci_find_capability(pdev, PCI_CAP_ID_MSI) && msix_enable) {
		if (!pci_enable_msi(pdev)) {
			pacb->acb_flags |= ACB_F_MSI_ENABLED;
		}
		if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", pacb)) {
			printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
			goto scsi_host_remove;
		}
		printk("arcmsr%d: msi enabled\n", pacb->host->host_no);
	} else {
		if (request_irq(pdev->irq, arcmsr_do_interrupt, SA_INTERRUPT | SA_SHIRQ, "arcmsr", pacb)) {
			printk("arcmsr%d: request_irq =%d failed!\n", pacb->host->host_no, pdev->irq);
			goto scsi_host_remove;
		}
		printk("arcmsr%d: enabled legacy interrupt(%d) \n", pacb->host->host_no, __LINE__);
	}
	if (!arcmsr_iop_init(pacb)) {
		printk("arcmsr%d: controller init failed!\n", pacb->host->host_no);
		goto scsi_host_remove;
	}
	list_add_tail(&pacb->list, &rc_list);
	scsi_scan_host(host);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
		INIT_WORK(&pacb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn);
	#else
		INIT_WORK(&pacb->arcmsr_do_message_isr_bh, arcmsr_message_isr_bh_fn, pacb);
	#endif
	atomic_set(&pacb->rq_map_token, 16);
	atomic_set(&pacb->ante_token_value, 16);
	pacb->fw_flag = FW_NORMAL;
	init_timer(&pacb->eternal_timer);
	pacb->eternal_timer.expires = jiffies + msecs_to_jiffies(6 * HZ);
	pacb->eternal_timer.data = (unsigned long)pacb;
	pacb->eternal_timer.function = &arcmsr_request_device_map;
	add_timer(&pacb->eternal_timer);
	return 0;
	scsi_host_remove:
		if (pacb->acb_flags & ACB_F_MSI_ENABLED) {
			pci_disable_msi(pdev);
		} else if (pacb->acb_flags & ACB_F_MSIX_ENABLED) {
			pci_disable_msix(pdev);
		}
		scsi_remove_host(host);
	RAID_controller_stop:
		arcmsr_stop_adapter_bgrb(pacb);
		arcmsr_flush_adapter_cache(pacb);
		arcmsr_free_ccb_pool(pacb);
	free_mu:
		arcmsr_free_mu(pacb);
	unmap_pci_region:
		arcmsr_unmap_pciregion(pacb);
	pci_release_regs:
		pci_release_regions(pdev);
	scsi_host_release:
		scsi_host_put(host);	
	pci_disable_dev:
		pci_disable_device(pdev);
	return -ENODEV;
}

static void arcmsr_remove(struct pci_dev *pdev)
{
	struct Scsi_Host *host = pci_get_drvdata(pdev);
    	struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
	arcmsr_pcidev_disattach(acb);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 13)
	static void arcmsr_shutdown(struct pci_dev *pdev)
	{
		int i;
		struct Scsi_Host *host = pci_get_drvdata(pdev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;

		arcmsr_disable_outbound_ints(acb);
		if (acb->acb_flags & ACB_F_MSI_ENABLED) {
			free_irq(pdev->irq, acb);
			pci_disable_msi(pdev);
		} else if (acb->acb_flags & ACB_F_MSIX_ENABLED) {
			for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
				free_irq(acb->entries[i].vector, acb);
			}
			pci_disable_msix(pdev);
		} else {
			free_irq(pdev->irq, acb);
		}
		del_timer_sync(&acb->eternal_timer);
		flush_scheduled_work();
		arcmsr_stop_adapter_bgrb(acb);
		arcmsr_flush_adapter_cache(acb);
	}
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb)
	{
		struct scsi_cmnd *pcmd = ccb->pcmd;
		scsi_dma_unmap(pcmd);
	}
#else
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb)
	{
		struct AdapterControlBlock *acb = ccb->acb;
		struct scsi_cmnd *pcmd = ccb->pcmd;
		struct scatterlist *sl;
		#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 23)
			sl = scsi_sglist(pcmd);
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

static bool arcmsr_define_adapter_type(struct AdapterControlBlock *acb)
{
	u16 dev_id;
	struct pci_dev *pdev = acb->pdev;

	pci_read_config_word(pdev, PCI_DEVICE_ID, &dev_id);
	acb->dev_id = dev_id;
	switch(dev_id) {
		case 0x1880: {
			acb->adapter_type = ACB_ADAPTER_TYPE_C;
			break;
		}
		case 0x1200:
		case 0x1201:
		case 0x1202: {
			acb->adapter_type = ACB_ADAPTER_TYPE_B;
			break;
		}
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
			acb->adapter_type = ACB_ADAPTER_TYPE_A;
			break;
		}
		case 0x1214: {
			acb->adapter_type = ACB_ADAPTER_TYPE_D;
			break;
		}
		default: {
			printk("Unknown device ID = 0x%x\n", dev_id);
			return false;
		}
	}
	return true;
}

static u32 arcmsr_disable_outbound_ints(struct AdapterControlBlock *acb)
{
	u32 orig_mask = 0;
	switch(acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A : {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			orig_mask = readl(&reg->outbound_intmask);
			writel(orig_mask |ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE, &reg->outbound_intmask);
			break;		
		}
		case ACB_ADAPTER_TYPE_B : {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			orig_mask = readl(reg->iop2drv_doorbell_mask);
			writel(0, reg->iop2drv_doorbell_mask);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			/* disable all outbound interrupt */
			orig_mask = readl(&reg->host_int_mask); /* disable outbound message0 int */
			writel(orig_mask | ARCMSR_HBCMU_ALL_INTMASKENABLE, &reg->host_int_mask);
			readl(&reg->host_int_mask);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			/* disable all outbound interrupt */
			writel(ARCMSR_ARC1214_ALL_INT_DISABLE, reg->pcief0_int_enable);
			break;
		}
	}
	return orig_mask;	
}

static void arcmsr_enable_outbound_ints(struct AdapterControlBlock *acb, u32 orig_mask)
{
	uint32_t mask;

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A : {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			mask = orig_mask & ~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE | ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE | ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
			writel(mask, &reg->outbound_intmask);
			acb->outbound_int_enable = ~(orig_mask & mask) & 0x000000ff;
			break;
		}
		case ACB_ADAPTER_TYPE_B : {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			mask = orig_mask | (ARCMSR_IOP2DRV_DATA_WRITE_OK | ARCMSR_IOP2DRV_DATA_READ_OK | ARCMSR_IOP2DRV_CDB_DONE | ARCMSR_IOP2DRV_MESSAGE_CMD_DONE);
			writel(mask, reg->iop2drv_doorbell_mask);
			acb->outbound_int_enable = (orig_mask | mask) & 0x0000000f;
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			mask = ~(ARCMSR_HBCMU_ALL_INTMASKENABLE);
			writel(orig_mask & mask, &reg->host_int_mask);
			readl(&reg->host_int_mask);/* Dummy readl to force pci flush */
			acb->outbound_int_enable = ~(orig_mask & mask) & 0x0000000f;
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			mask = ARCMSR_ARC1214_ALL_INT_ENABLE;
			writel(orig_mask | mask, reg->pcief0_int_enable);
			readl(reg->pcief0_int_enable);/* Dummy readl to force pci flush */
			break;
		}
	}
}

static void arcmsr_hbaA_flush_cache(struct AdapterControlBlock *acb)
{
	int retry_count = 6;
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, &reg->inbound_msgaddr0);
	do {
		if (arcmsr_hbaA_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down=%d \n", acb->host->host_no, retry_count);
		}
	} while (retry_count != 0);
}

static void arcmsr_hbaB_flush_cache(struct AdapterControlBlock *acb)
{
	int retry_count = 6;
	struct MessageUnit_B __iomem *reg = acb->pmuB;

	writel(ARCMSR_MESSAGE_FLUSH_CACHE, reg->drv2iop_doorbell);
	do {
		if (arcmsr_hbaB_wait_msgint_ready(acb))
			break;
		else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down= %d \n", acb->host->host_no, retry_count);
		}
	} while (retry_count != 0);
}

static void arcmsr_hbaC_flush_cache(struct AdapterControlBlock *pACB)
{
	int retry_count = 6;
	struct MessageUnit_C __iomem *reg = pACB->pmuC;

	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
	readl(&reg->inbound_msgaddr0);/* Dummy readl to force pci flush */
	do {
		if (arcmsr_hbaC_wait_msgint_ready(pACB)) {
			break;
		} else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down = %d \n", pACB->host->host_no, retry_count);
		}
	} while (retry_count != 0);
	return;
}

static void arcmsr_hbaD_flush_cache(struct AdapterControlBlock *pACB)
{
	int retry_count = 6;
	struct MessageUnit_D __iomem *reg = pACB->pmuD;

	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE, reg->inbound_msgaddr0);
	do {
		if (arcmsr_hbaD_wait_msgint_ready(pACB)) {
			break;
		} else {
			retry_count--;
			printk(KERN_NOTICE "arcmsr%d: wait 'flush adapter cache' timeout, retry count down = %d \n", pACB->host->host_no, retry_count);
		}
	} while (retry_count != 0);
	return;
}

static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			arcmsr_hbaA_flush_cache(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			arcmsr_hbaB_flush_cache(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			arcmsr_hbaC_flush_cache(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			arcmsr_hbaD_flush_cache(acb);
			break;
		}
	}
}

void arcmsr_ccb_complete(struct CommandControlBlock *ccb)
{
	unsigned long flags;
	struct AdapterControlBlock *acb = ccb->acb;
	struct scsi_cmnd *pcmd = ccb->pcmd;

	atomic_dec(&acb->ccboutstandingcount);
    arcmsr_pci_unmap_dma(ccb);
	ccb->startdone = ARCMSR_CCB_DONE;
	ccb->ccb_flags = 0;
	spin_lock_irqsave(&acb->ccblist_lock, flags);
	list_add_tail(&ccb->list, &acb->ccb_free_list);
	spin_unlock_irqrestore(&acb->ccblist_lock, flags);
	pcmd->scsi_done(pcmd);
}

static void arcmsr_report_sense_info(struct CommandControlBlock *ccb)
{
	struct scsi_cmnd *pcmd = ccb->pcmd;
	struct SENSE_DATA *sensebuffer = (struct SENSE_DATA *)pcmd->sense_buffer;

	pcmd->result |= DRIVER_SENSE << 24;
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

static uint8_t arcmsr_hbaA_abort_allcmd(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, &reg->inbound_msgaddr0);
	if (arcmsr_hbaA_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n", acb->host->host_no);
		return 0xff;
	}
	return 0x00;
}

static uint8_t arcmsr_hbaB_abort_allcmd(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B __iomem *reg = acb->pmuB;
	writel(ARCMSR_MESSAGE_ABORT_CMD, reg->drv2iop_doorbell);
	if (arcmsr_hbaB_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'abort all outstanding command' timeout \n", acb->host->host_no);
		return 0xff;
	}
	return 0x00;
}

static uint8_t arcmsr_hbaC_abort_allcmd(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C __iomem *reg = pACB->pmuC;
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	if (!arcmsr_hbaC_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, pACB->host->host_no);
		return false;
	}
	return true;
}

static uint8_t arcmsr_hbaD_abort_allcmd(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_D __iomem *reg = pACB->pmuD;
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD, reg->inbound_msgaddr0);
	if (!arcmsr_hbaD_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE
			"arcmsr%d: wait 'abort all outstanding command' timeout \n"
			, pACB->host->host_no);
		return false;
	}
	return true;
}

static uint8_t arcmsr_abort_allcmd(struct AdapterControlBlock *acb)
{
	uint8_t rtnval = 0;
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			rtnval = arcmsr_hbaA_abort_allcmd(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			rtnval = arcmsr_hbaB_abort_allcmd(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			rtnval = arcmsr_hbaC_abort_allcmd(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			rtnval = arcmsr_hbaD_abort_allcmd(acb);
			break;
		}
	}
	return rtnval;
}

static int arcmsr_build_ccb(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb, struct scsi_cmnd *pcmd)
{
	__le32 address_lo, address_hi;
	int arccdbsize = 0x30, sgcount = 0;
	unsigned request_bufflen;
	unsigned short use_sg;
	__le32 length = 0, length_sum = 0;
	int i;
	struct ARCMSR_CDB *arcmsr_cdb = &ccb->arcmsr_cdb;
	uint8_t *psge = (uint8_t *)&arcmsr_cdb->u;
	struct scatterlist *sl, *sg;
	u64 temp;

	ccb->pcmd = pcmd;
	memset(arcmsr_cdb, 0, sizeof(struct ARCMSR_CDB));
    arcmsr_cdb->Bus = 0;
   	arcmsr_cdb->TargetID = pcmd->device->id;
    arcmsr_cdb->LUN = pcmd->device->lun;
    arcmsr_cdb->Function = 1;
	arcmsr_cdb->CdbLength = (uint8_t)pcmd->cmd_len;
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
		#else
			sl = (struct scatterlist *) pcmd->request_buffer;
			sgcount = pci_map_sg(acb->pdev, sl, pcmd->use_sg, pcmd->sc_data_direction);
		#endif
		if (sgcount > acb->host->sg_tablesize)
			return FAILED;
		scsi_for_each_sg(pcmd, sg, sgcount, i) {
			length = cpu_to_le32(sg_dma_len(sg));
			address_lo = cpu_to_le32(dma_addr_lo32(sg_dma_address(sg)));
			address_hi = cpu_to_le32(dma_addr_hi32(sg_dma_address(sg)));

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
				pdma_sg->length = length | cpu_to_le32(IS_SG64_ADDR);
				psge += sizeof(struct SG64ENTRY);
				arccdbsize += sizeof(struct SG64ENTRY);
			}
		}
		arcmsr_cdb->sgcount = sgcount;
		arcmsr_cdb->DataLength = request_bufflen;
		arcmsr_cdb->msgPages = arccdbsize / 0x100 + (arccdbsize % 0x100 ? 1 : 0);
		if (arccdbsize > 256)
			arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_SGL_BSIZE;
	} else if (request_bufflen) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
			dma_addr_t dma_addr;
			dma_addr = pci_map_single(acb->pdev, scsi_sglist(pcmd), scsi_bufflen(pcmd), pcmd->sc_data_direction);			
			pcmd->SCp.ptr =	(char *)(unsigned long)dma_addr;/* We hide it here for later unmap. */
			address_lo = cpu_to_le32(dma_addr_lo32(dma_addr));
	    	address_hi = cpu_to_le32(dma_addr_hi32(dma_addr));
		#else
			dma_addr_t dma_addr;
			dma_addr = pci_map_single(acb->pdev, pcmd->request_buffer, pcmd->request_bufflen, pcmd->sc_data_direction);
			/* We hide it here for later unmap. */
			pcmd->SCp.ptr =	(char *)(unsigned long)dma_addr;
			address_lo = cpu_to_le32(dma_addr_lo32(dma_addr));
	    	address_hi = cpu_to_le32(dma_addr_hi32(dma_addr));
    	#endif
		if (address_hi == 0) {
			struct SG32ENTRY* pdma_sg = (struct SG32ENTRY*)psge;
			pdma_sg->address = address_lo;
	    	pdma_sg->length = request_bufflen;
			arccdbsize += sizeof(struct SG32ENTRY);
		} else {
			struct SG64ENTRY* pdma_sg = (struct SG64ENTRY*)psge;
			pdma_sg->addresshigh = address_hi;
			pdma_sg->address = address_lo;
	    	pdma_sg->length = request_bufflen | cpu_to_le32(IS_SG64_ADDR);
			arccdbsize += sizeof(struct SG64ENTRY);
		}
		arcmsr_cdb->sgcount = 1;
		arcmsr_cdb->DataLength = request_bufflen;
		arcmsr_cdb->msgPages = arccdbsize / 0x100 + (arccdbsize % 0x100 ? 1 : 0);
	}	
	if (pcmd->sc_data_direction == DMA_TO_DEVICE) {
		arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_WRITE;
		ccb->ccb_flags |= CCB_FLAG_WRITE;
	}
	ccb->arc_cdb_size = arccdbsize;
	if (timeout) {
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
			temp = pcmd->request->timeout / HZ;//timeout value in struct request is in jiffy
			pcmd->request->deadline = jiffies + timeout * HZ;
			if (pcmd->device->request_queue->timeout.function) {
				mod_timer(&pcmd->device->request_queue->timeout, pcmd->request->deadline);			
			}
		#else
			temp = pcmd->timeout_per_command / HZ;//timeout value in struct request is in jiffy
			if (pcmd->eh_timeout.function) {
				mod_timer(&pcmd->eh_timeout, jiffies + timeout  * HZ );
			}
		#endif
	}
    return SUCCESS;
}

static void arcmsr_post_ccb(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb)
{
	u32 cdb_phyaddr = ccb->cdb_phyaddr;
	struct ARCMSR_CDB *arcmsr_cdb = (struct ARCMSR_CDB *)&ccb->arcmsr_cdb;
	u32 arccdbsize = ccb->arc_cdb_size;

	atomic_inc(&acb->ccboutstandingcount);
	ccb->startdone = ARCMSR_CCB_START;
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;

			if (arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE) {
				writel(cdb_phyaddr | ARCMSR_CCBPOST_FLAG_SGL_BSIZE, &reg->inbound_queueport);
			} else {
				writel(cdb_phyaddr, &reg->inbound_queueport);
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			u32 ending_index, index = reg->postq_index;

			ending_index = ((index + 1) % ARCMSR_MAX_HBB_POSTQUEUE);
			reg->post_qbuffer[ending_index] = 0;
			if (arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE) {
				reg->post_qbuffer[index] = (cdb_phyaddr | ARCMSR_CCBPOST_FLAG_SGL_BSIZE);
			} else {
				reg->post_qbuffer[index] = cdb_phyaddr;
			}
			index++;
			index %= ARCMSR_MAX_HBB_POSTQUEUE;     /*if last index number set it to 0 */
			reg->postq_index = index;
			writel(ARCMSR_DRV2IOP_CDB_POSTED, reg->drv2iop_doorbell);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *phbcmu = acb->pmuC;
			u32 ccb_post_stamp, arc_cdb_size;

			arc_cdb_size = (ccb->arc_cdb_size > 0x300) ? 0x300 : ccb->arc_cdb_size;
			ccb_post_stamp = (cdb_phyaddr | ((arc_cdb_size - 1) >> 6) | 1);
			if (acb->cdb_phyaddr_hi32) {
				writel(acb->cdb_phyaddr_hi32, &phbcmu->inbound_queueport_high);
				writel(ccb_post_stamp, &phbcmu->inbound_queueport_low);
			} else {
				writel(ccb_post_stamp, &phbcmu->inbound_queueport_low);
			}
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *pmu = acb->pmuD;
			u16 index_stripped;
			u16 postq_index;
			unsigned long flags;
			struct InBound_SRB *pinbound_srb;

			spin_lock_irqsave(&acb->postq_lock, flags);
			postq_index = pmu->postq_index;
			pinbound_srb = (struct InBound_SRB *)&pmu->post_qbuffer[postq_index & 0xFF];
			pinbound_srb->addressHigh = dma_addr_hi32(cdb_phyaddr);
			pinbound_srb->addressLow= dma_addr_lo32(cdb_phyaddr);
			pinbound_srb->length= arccdbsize >> 2;
			arcmsr_cdb->msgContext = dma_addr_lo32(cdb_phyaddr);
			if (postq_index & 0x4000) {
				index_stripped = postq_index & 0xFF;
				index_stripped += 1;
				index_stripped %= ARCMSR_MAX_ARC1214_POSTQUEUE;
				pmu->postq_index = index_stripped ? (index_stripped | 0x4000) : index_stripped;
			} else {
				index_stripped = postq_index;
				index_stripped += 1;
				index_stripped %= ARCMSR_MAX_ARC1214_POSTQUEUE;
				pmu->postq_index = index_stripped ? index_stripped : (index_stripped | 0x4000);
			}
			writel(postq_index, pmu->inboundlist_write_pointer);
			spin_unlock_irqrestore(&acb->postq_lock, flags);
			break;
		}
	}
}
/*
*******************************************************************************
** To notice IOP the message has been read
*******************************************************************************
*/
static void arcmsr_iop_message_read(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DBG_FUNC
		printk("%s:\n", __func__);
	#endif
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK, &reg->inbound_doorbell);
			readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			writel(ARCMSR_DRV2IOP_DATA_READ_OK, reg->drv2iop_doorbell);
			readl(reg->drv2iop_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			writel(ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK, &reg->inbound_doorbell);
			readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			writel(ARCMSR_ARC1214_DRV2IOP_DATA_OUT_READ, reg->inbound_doorbell);
			readl(reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
	}
}
/*
*******************************************************************************
** To notice IOP the message has been written down
*******************************************************************************
*/
static void arcmsr_iop_message_wrote(struct AdapterControlBlock *acb)
{
	#if ARCMSR_DBG_FUNC
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
			readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			/*
			** push inbound doorbell tell iop, driver data write ok 
			** and wait reply on next hwinterrupt for next Qbuffer post
			*/
			writel(ARCMSR_DRV2IOP_DATA_WRITE_OK, reg->drv2iop_doorbell);
			readl(reg->drv2iop_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			/*
			** push inbound doorbell tell iop, driver data write ok
			** and wait reply on next hwinterrupt for next Qbuffer post
			*/
			writel(ARCMSR_HBCMU_DRV2IOP_DATA_WRITE_OK, &reg->inbound_doorbell);
			readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *reg = acb->pmuD;
			/*
			** push inbound doorbell tell iop, driver data write ok
			** and wait reply on next hwinterrupt for next Qbuffer post
			*/
			writel(ARCMSR_ARC1214_DRV2IOP_DATA_IN_READY, reg->inbound_doorbell);
			readl(reg->inbound_doorbell);/* Dummy readl to force pci flush */
			break;
		}
	}
}

static struct QBUFFER __iomem  *arcmsr_get_iop_rqbuffer(struct AdapterControlBlock *acb)
{
	struct QBUFFER __iomem *qbuffer = NULL;
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *pmu = acb->pmuA;
			qbuffer = (struct QBUFFER __iomem *)&pmu->message_rbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *pmu = acb->pmuB;
			qbuffer = (struct QBUFFER __iomem *)pmu->message_rbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *pmu = acb->pmuC;
			qbuffer = (struct QBUFFER __iomem *)pmu->message_rbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *pmu = acb->pmuD;
			qbuffer = (struct QBUFFER __iomem *)pmu->message_rbuffer;
			break;
		}
	}
	return qbuffer;
}

static struct QBUFFER __iomem  *arcmsr_get_iop_wqbuffer(struct AdapterControlBlock *acb)
{
	struct QBUFFER __iomem *pqbuffer = NULL;
	switch (acb->adapter_type) {	
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *pmu = acb->pmuA;
			pqbuffer = (struct QBUFFER __iomem *)&pmu->message_wbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B  __iomem *pmu = acb->pmuB;
			pqbuffer = (struct QBUFFER __iomem *)pmu->message_wbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *pmu = acb->pmuC;
			pqbuffer = (struct QBUFFER __iomem *)pmu->message_wbuffer;
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *pmu = acb->pmuD;
			pqbuffer = (struct QBUFFER __iomem *)pmu->message_wbuffer;
			break;
		}
	}
	return pqbuffer;
}

static void arcmsr_hbaA_stop_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, &reg->inbound_msgaddr0);
	if (!arcmsr_hbaA_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter"
		"background rebulid' timeout \n", acb->host->host_no);
	}
}

static void arcmsr_hbaB_stop_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B __iomem *reg = acb->pmuB;

	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_MESSAGE_STOP_BGRB, reg->drv2iop_doorbell);
	if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter"
		"background rebulid' timeout \n", acb->host->host_no);
	}
}

static void arcmsr_hbaC_stop_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C __iomem *reg = pACB->pmuC;

	pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, &reg->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
	readl(&reg->inbound_doorbell);/* Dummy readl to force pci flush */
	readl(&reg->inbound_msgaddr0);/* Dummy readl to force pci flush */
	if (!arcmsr_hbaC_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter"
		"background rebulid' timeout \n", pACB->host->host_no);
	}
}

static void arcmsr_hbaD_stop_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_D __iomem *reg = pACB->pmuD;

	pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB, reg->inbound_msgaddr0);
	if (!arcmsr_hbaD_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'stop adapter"
		"background rebulid' timeout \n", pACB->host->host_no);
	}
}

static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			arcmsr_hbaA_stop_bgrb(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			arcmsr_hbaB_stop_bgrb(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			arcmsr_hbaC_stop_bgrb(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			arcmsr_hbaD_stop_bgrb(acb);
			break;
		}
	}	
}

static uint32_t arcmsr_Read_iop_rqbuffer_in_DWORD(struct AdapterControlBlock *acb,
    struct QBUFFER __iomem *prbuffer) {

	uint8_t *pQbuffer;
	uint8_t *buf1 = 0;
	uint32_t __iomem *iop_data;
	uint32_t iop_len, data_len, *buf2 = 0;

	iop_data = (uint32_t *)prbuffer->data;
	iop_len = readl(&prbuffer->data_len);
	if (iop_len > 0) {
		buf1 = kmalloc(128, GFP_ATOMIC);
		buf2 = (uint32_t *)buf1;
		if (buf1 == NULL)
			return 0;
		data_len = iop_len;
		while(data_len >= 4)
		{
			*buf2++ = *iop_data++;
			data_len -= 4;
		}
		if (data_len)
			*buf2 = *iop_data;
		buf2 = (uint32_t *)buf1;
	}
	while (iop_len > 0) {
		pQbuffer = &acb->rqbuffer[acb->rqbuf_lastindex];
		*pQbuffer = *buf1;
		acb->rqbuf_lastindex++;
		/* if last, index number set it to 0 */
		acb->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
		buf1++;
		iop_len--;
	}
	if (buf2)
		kfree( (uint8_t *)buf2);
	/* let IOP know data has been read */
	arcmsr_iop_message_read(acb);
	return 1;
}

static uint32_t arcmsr_Read_iop_rqbuffer_data(struct AdapterControlBlock *acb,
    struct QBUFFER __iomem *prbuffer) {

	uint8_t *pQbuffer;
	uint8_t __iomem *iop_data;
	uint32_t iop_len;

	if (acb->adapter_type & (ACB_ADAPTER_TYPE_C | ACB_ADAPTER_TYPE_D)) {
		return(arcmsr_Read_iop_rqbuffer_in_DWORD(acb, prbuffer));
	}
	iop_data = (uint8_t *)prbuffer->data;
	iop_len = readl(&prbuffer->data_len);
	while (iop_len > 0) {
		pQbuffer = &acb->rqbuffer[acb->rqbuf_lastindex];
		*pQbuffer = *iop_data;
		acb->rqbuf_lastindex++;
		acb->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
		iop_data++;
		iop_len--;
	}
	arcmsr_iop_message_read(acb);
	return 1;
}

/*
*******************************************************************************
** Copy data "to driver" after IOP notices driver that IOP has the data prepared
*******************************************************************************
*/
static void arcmsr_iop2drv_data_wrote_handle(struct AdapterControlBlock *acb)
{
	unsigned long flags;
	struct QBUFFER __iomem  *prbuffer;
	int32_t buf_empty_len;

	spin_lock_irqsave(&acb->rqbuffer_lock, flags);
	prbuffer = arcmsr_get_iop_rqbuffer(acb);
	buf_empty_len = (acb->rqbuf_lastindex - acb->rqbuf_firstindex -1) & (ARCMSR_MAX_QBUFFER -1);
	if (buf_empty_len >= prbuffer->data_len) {
		if(arcmsr_Read_iop_rqbuffer_data(acb, prbuffer) == 0)
			acb->acb_flags |= ACB_F_IOPDATA_OVERFLOW;
	} else {
		acb->acb_flags |= ACB_F_IOPDATA_OVERFLOW;
	}
	spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
}

static void arcmsr_write_ioctldata2iop_in_DWORD(struct AdapterControlBlock *acb)
{
	uint8_t *pQbuffer;
	struct QBUFFER __iomem *pwbuffer;
	uint8_t *buf1 = 0;
	uint32_t __iomem *iop_data;
	uint32_t allxfer_len = 0, data_len, *buf2 = 0;
	
	if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_READED) {
		buf1 = kmalloc(128, GFP_ATOMIC);
		buf2 = (uint32_t *)buf1;
		if (buf1 == NULL)
			return;

		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		pwbuffer = arcmsr_get_iop_wqbuffer(acb);
		iop_data = (uint32_t *)pwbuffer->data;
		while((acb->wqbuf_firstindex != acb->wqbuf_lastindex) 
			&& (allxfer_len < 124)) {
			pQbuffer = &acb->wqbuffer[acb->wqbuf_firstindex];
			*buf1 = *pQbuffer;
			acb->wqbuf_firstindex++;
			acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
			buf1++;
			allxfer_len++;
		}
		data_len = allxfer_len;
		buf1 = (uint8_t *)buf2;
		while(data_len >= 4)
		{
			*iop_data++ = *buf2++;
			data_len -= 4;
		}
		if (data_len)
			*iop_data = *buf2;
		writel(allxfer_len, &pwbuffer->data_len);
		kfree(buf1);
		arcmsr_iop_message_wrote(acb);
	}
}

static void arcmsr_write_ioctldata2iop(struct AdapterControlBlock *acb)
{
	uint8_t *pQbuffer;
	struct QBUFFER __iomem *pwbuffer;
	uint8_t __iomem *iop_data;
	int32_t allxfer_len = 0;
	
	if (acb->adapter_type & (ACB_ADAPTER_TYPE_C | ACB_ADAPTER_TYPE_D)) {
		arcmsr_write_ioctldata2iop_in_DWORD(acb);
		return;
	}
	if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_READED) {
		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		pwbuffer = arcmsr_get_iop_wqbuffer(acb);
		iop_data = (uint8_t *)pwbuffer->data;
		while((acb->wqbuf_firstindex != acb->wqbuf_lastindex) 
			&& (allxfer_len < 124)) {
			pQbuffer = &acb->wqbuffer[acb->wqbuf_firstindex];
			*iop_data = *pQbuffer;
			acb->wqbuf_firstindex++;
			acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
			iop_data++;
			allxfer_len++;
		}
		writel(allxfer_len, &pwbuffer->data_len);
		arcmsr_iop_message_wrote(acb);
	}
}

/*
*******************************************************************************
** Copy data "to IOP" after IOP notices driver that IOP is ready to receive the data
*******************************************************************************
*/
static void arcmsr_iop2drv_data_read_handle(struct AdapterControlBlock *acb)
{
	unsigned long flags;

	#if ARCMSR_DBG_FUNC
		printk("%s:\n", __func__);
	#endif
	spin_lock_irqsave(&acb->wqbuffer_lock, flags);
	acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_READED;
	if (acb->wqbuf_firstindex != acb->wqbuf_lastindex)
		arcmsr_write_ioctldata2iop(acb);
	if (acb->wqbuf_firstindex == acb->wqbuf_lastindex)
		acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_CLEARED;
	spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
}

static void arcmsr_report_ccb_state(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb, bool error)
{
	uint8_t id, lun;	

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
				break;
			}
			case ARCMSR_DEV_ABORTED: 
			case ARCMSR_DEV_INIT_FAIL: {
				acb->devstate[id][lun] = ARECA_RAID_GONE;
				ccb->pcmd->result = DID_BAD_TARGET << 16;
				arcmsr_ccb_complete(ccb);
				break;
			}
			case ARCMSR_DEV_CHECK_CONDITION: {
				acb->devstate[id][lun] = ARECA_RAID_GOOD;
				ccb->pcmd->result = (DID_OK << 16) | (CHECK_CONDITION << 1);
				arcmsr_report_sense_info(ccb);
				arcmsr_ccb_complete(ccb);
				break;
			}
			default:
				printk(KERN_EMERG "arcmsr%d: scsi id = %d lun = %d get command error done, but got unknown DeviceStatus = 0x%x \n"
						, acb->host->host_no, id, lun, ccb->arcmsr_cdb.DeviceStatus);
				acb->devstate[id][lun] = ARECA_RAID_GONE;
				ccb->pcmd->result = DID_BAD_TARGET << 16;
				arcmsr_ccb_complete(ccb);
				break;
		}
	}	
}

static void arcmsr_drain_donequeue(struct AdapterControlBlock *acb, struct CommandControlBlock *pCCB, bool error)
{
	if (unlikely((pCCB->acb != acb) || (pCCB->startdone != ARCMSR_CCB_START))) {
		printk(KERN_NOTICE "arcmsr%d: isr get an illegal ccb command "
			"done acb = 0x%p, "
			"ccb = 0x%p, "
			"ccbacb = 0x%p, "
			"startdone = 0x%x, "
			"pscsi_cmd = 0x%p, "
			"ccboutstandingcount = %d\n"
			, acb->host->host_no
			, acb
			, pCCB
			, pCCB->acb
			, pCCB->startdone
			, pCCB->pcmd
			, atomic_read(&acb->ccboutstandingcount));
		return;
	}
	arcmsr_report_ccb_state(acb, pCCB, error);
}

static void arcmsr_hbaA_doorbell_isr(struct AdapterControlBlock *acb) 
{
    uint32_t outbound_doorbell;
	struct MessageUnit_A __iomem *reg  = acb->pmuA;

	outbound_doorbell = readl(&reg->outbound_doorbell);
	do {
		writel(outbound_doorbell, &reg->outbound_doorbell);
		if (outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK) {
			arcmsr_iop2drv_data_wrote_handle(acb);
		}
		if (outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK) {
			arcmsr_iop2drv_data_read_handle(acb);
		}
		outbound_doorbell = readl(&reg->outbound_doorbell);
	} while (outbound_doorbell & (ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK | ARCMSR_OUTBOUND_IOP331_DATA_READ_OK));
}

static void arcmsr_hbaC_doorbell_isr(struct AdapterControlBlock *pACB)
{
	uint32_t outbound_doorbell;
	struct MessageUnit_C __iomem *reg = pACB->pmuC;

	outbound_doorbell = readl(&reg->outbound_doorbell);
	do {
		writel(outbound_doorbell, &reg->outbound_doorbell_clear);/*clear interrupt*/
		readl(&reg->outbound_doorbell_clear);/* Dummy readl to force pci flush */
		if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_DATA_READ_OK) {
			arcmsr_iop2drv_data_read_handle(pACB);
		}
		if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_OK) {
			arcmsr_iop2drv_data_wrote_handle(pACB);
		}
		if (outbound_doorbell & ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbaC_message_isr(pACB);    /* messenger of "driver to iop commands" */
		}
		outbound_doorbell = readl(&reg->outbound_doorbell);
	} while (outbound_doorbell & (ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_OK | ARCMSR_HBCMU_IOP2DRV_DATA_READ_OK | ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE));
}

static void arcmsr_hbaD_doorbell_isr(struct AdapterControlBlock *pACB)
{
	uint32_t outbound_doorbell;
	struct MessageUnit_D __iomem *pmu = pACB->pmuD;

	outbound_doorbell = readl(pmu->outbound_doorbell);
	do {
		writel(outbound_doorbell, pmu->outbound_doorbell);/*clear interrupt*/
		//readl(reg->outbound_doorbell);/* Dummy readl to force pci flush */
		if (outbound_doorbell & ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbaD_message_isr(pACB);/* messenger of "driver to iop commands" */
		}
		if (outbound_doorbell & ARCMSR_ARC1214_IOP2DRV_DATA_WRITE_OK) {
			arcmsr_iop2drv_data_wrote_handle(pACB);
		}
		if (outbound_doorbell & ARCMSR_ARC1214_IOP2DRV_DATA_READ_OK) {
			arcmsr_iop2drv_data_read_handle(pACB);
		}
		outbound_doorbell = readl(pmu->outbound_doorbell);
	} while (outbound_doorbell & (ARCMSR_ARC1214_IOP2DRV_DATA_WRITE_OK | ARCMSR_ARC1214_IOP2DRV_DATA_READ_OK | ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE));
}

static void arcmsr_hbaA_postqueue_isr(struct AdapterControlBlock *acb)
{
	bool error;
	uint32_t flag_ccb;
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	struct ARCMSR_CDB *pARCMSR_CDB;
	struct CommandControlBlock *pCCB;

	while ((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) {
		pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
		pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_drain_donequeue(acb, pCCB, error);
	}
}

static void arcmsr_hbaB_postqueue_isr(struct AdapterControlBlock *acb)
{
	bool error;
	uint32_t index, flag_ccb;
	struct MessageUnit_B __iomem *reg = acb->pmuB;
	struct ARCMSR_CDB *pARCMSR_CDB;
	struct CommandControlBlock *pCCB;

	index = reg->doneq_index;
	while ((flag_ccb = readl(&reg->done_qbuffer[index])) != 0) {
		writel(0, &reg->done_qbuffer[index]);
		pARCMSR_CDB = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
		pCCB = container_of(pARCMSR_CDB, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_drain_donequeue(acb, pCCB, error);
		index++;
		index %= ARCMSR_MAX_HBB_POSTQUEUE;
		reg->doneq_index = index;
	}
}

static void arcmsr_hbaC_postqueue_isr(struct AdapterControlBlock *acb)
{
	uint32_t flag_ccb, ccb_cdb_phy, throttling = 0;
	int error;
	struct MessageUnit_C __iomem *phbcmu;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *ccb;

	phbcmu = acb->pmuC;
	do {
		/* check if command done with no error*/
		flag_ccb = readl(&phbcmu->outbound_queueport_low);
		ccb_cdb_phy = (flag_ccb & 0xFFFFFFF0);/*frame must be 32 bytes aligned*/
		arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);
		ccb = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
		/* check if command done with no error */
		arcmsr_drain_donequeue(acb, ccb, error);
		throttling++;
		if (throttling == ARCMSR_HBC_ISR_THROTTLING_LEVEL) {
			writel(ARCMSR_HBCMU_DRV2IOP_POSTQUEUE_THROTTLING, &phbcmu->inbound_doorbell);
			throttling = 0;
		}
	} while (readl(&phbcmu->host_int_status) & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR);
}

static void arcmsr_hbaD_postqueue_isr(struct AdapterControlBlock *acb)
{
	u32 outbound_write_pointer, doneq_index, index_stripped;
	uint32_t addressLow, ccb_cdb_phy;
	int error;
	struct MessageUnit_D __iomem *pmu;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *ccb;
	unsigned long flags;

	spin_lock_irqsave(&acb->doneq_lock, flags);
	pmu = acb->pmuD;
	outbound_write_pointer = pmu->done_qbuffer[0].addressLow + 1;
	doneq_index = pmu->doneq_index;
	if ((doneq_index & 0xFFF) != (outbound_write_pointer & 0xFFF)) {
		do {
			if (doneq_index & 0x4000) {
				index_stripped = doneq_index & 0xFFF;
				index_stripped += 1;
				index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
				pmu->doneq_index = index_stripped ? (index_stripped | 0x4000) : (index_stripped + 1);
			} else {
				index_stripped = doneq_index;
				index_stripped += 1;
				index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
				pmu->doneq_index = index_stripped ? index_stripped : ((index_stripped | 0x4000) + 1);
			}
			doneq_index = pmu->doneq_index;
			addressLow = pmu->done_qbuffer[doneq_index & 0xFFF].addressLow;
			ccb_cdb_phy = (addressLow & 0xFFFFFFF0);/*frame must be 32 bytes aligned*/
			arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);
			ccb = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
			error = (addressLow & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
			arcmsr_drain_donequeue(acb, ccb, error);/*Check if command done with no error */
			writel(doneq_index, pmu->outboundlist_read_pointer);
		} while ((doneq_index & 0xFFF) != (outbound_write_pointer & 0xFFF));
	}
	writel(ARCMSR_ARC1214_OUTBOUND_LIST_INTERRUPT_CLEAR, pmu->outboundlist_interrupt_cause);
	spin_unlock_irqrestore(&acb->doneq_lock, flags);
}

static void arcmsr_hbaA_message_isr(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg  = acb->pmuA;

	writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT, &reg->outbound_intstatus);/*clear interrupt and message state*/
	schedule_work(&acb->arcmsr_do_message_isr_bh);
}

static void arcmsr_hbaB_message_isr(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B __iomem *reg  = acb->pmuB;
	writel(ARCMSR_MESSAGE_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);/*clear interrupt and message state*/
	schedule_work(&acb->arcmsr_do_message_isr_bh);
}

static void arcmsr_hbaC_message_isr(struct AdapterControlBlock *acb)
{
	struct MessageUnit_C __iomem *reg  = acb->pmuC;
	writel(ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR, &reg->outbound_doorbell_clear);
	schedule_work(&acb->arcmsr_do_message_isr_bh);
	/*clear interrupt and message state*/
}

static void arcmsr_hbaD_message_isr(struct AdapterControlBlock *acb)
{
	struct MessageUnit_D __iomem *reg  = acb->pmuD;
	writel(ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE, reg->outbound_doorbell);
	schedule_work(&acb->arcmsr_do_message_isr_bh);
	/*clear interrupt and message state*/
}

static irqreturn_t arcmsr_hbaA_handle_isr(struct AdapterControlBlock *acb)
{
	uint32_t outbound_intstatus;
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	if (!(outbound_intstatus & ARCMSR_MU_OUTBOUND_HANDLE_INT)) {
        		return IRQ_NONE;
	}
	do {
		writel(outbound_intstatus, &reg->outbound_intstatus);/* clear doorbell interrupt */
		if (outbound_intstatus & ARCMSR_MU_OUTBOUND_POSTQUEUE_INT) {
			arcmsr_hbaA_postqueue_isr(acb);
		}
		if (outbound_intstatus & ARCMSR_MU_OUTBOUND_DOORBELL_INT) {
			arcmsr_hbaA_doorbell_isr(acb);
		}
		if (outbound_intstatus & ARCMSR_MU_OUTBOUND_MESSAGE0_INT) {
			arcmsr_hbaA_message_isr(acb);		/* messenger of "driver to iop commands" */
		}
		outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	} while (outbound_intstatus & (ARCMSR_MU_OUTBOUND_DOORBELL_INT | ARCMSR_MU_OUTBOUND_POSTQUEUE_INT | ARCMSR_MU_OUTBOUND_MESSAGE0_INT));
	return IRQ_HANDLED;	
}

static irqreturn_t arcmsr_hbaB_handle_isr(struct AdapterControlBlock *acb)
{
	uint32_t outbound_doorbell;
	struct MessageUnit_B __iomem *reg = acb->pmuB;

	outbound_doorbell = readl(reg->iop2drv_doorbell) & acb->outbound_int_enable;
	if (!outbound_doorbell)
		return IRQ_NONE;
	do {
		writel(~outbound_doorbell, reg->iop2drv_doorbell);/* clear doorbell interrupt */
		writel(ARCMSR_DRV2IOP_END_OF_INTERRUPT, reg->drv2iop_doorbell);
		if (outbound_doorbell & ARCMSR_IOP2DRV_DATA_WRITE_OK) {
			arcmsr_iop2drv_data_wrote_handle(acb);
		}
		if (outbound_doorbell & ARCMSR_IOP2DRV_DATA_READ_OK) {
			arcmsr_iop2drv_data_read_handle(acb);
		}
		if (outbound_doorbell & ARCMSR_IOP2DRV_CDB_DONE) {
			arcmsr_hbaB_postqueue_isr(acb);
		}
		if (outbound_doorbell & ARCMSR_IOP2DRV_MESSAGE_CMD_DONE) {
			arcmsr_hbaB_message_isr(acb);/* messenger of "driver to iop commands" */
		}
		outbound_doorbell = readl(reg->iop2drv_doorbell) & acb->outbound_int_enable;
	} while (outbound_doorbell & (ARCMSR_IOP2DRV_DATA_WRITE_OK | ARCMSR_IOP2DRV_DATA_READ_OK | ARCMSR_IOP2DRV_CDB_DONE | ARCMSR_IOP2DRV_MESSAGE_CMD_DONE));
	return IRQ_HANDLED;
}

static irqreturn_t arcmsr_hbaC_handle_isr(struct AdapterControlBlock *pACB)
{
	uint32_t host_interrupt_status;
	struct MessageUnit_C __iomem *phbcmu = pACB->pmuC;

	host_interrupt_status = readl(&phbcmu->host_int_status);
	if (!host_interrupt_status)
		return IRQ_NONE;
	do {
		/* MU ioctl transfer doorbell interrupts*/
		if (host_interrupt_status & ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR) {
			arcmsr_hbaC_doorbell_isr(pACB);/*messenger of "ioctl message read write"*/
		}
		/* MU post queue interrupts*/
		if (host_interrupt_status & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) {
			arcmsr_hbaC_postqueue_isr(pACB);/*messenger of "scsi commands"*/
		}
		host_interrupt_status = readl(&phbcmu->host_int_status);
	} while (host_interrupt_status & (ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR | ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR));
	return IRQ_HANDLED;
}

static irqreturn_t arcmsr_hbaD_handle_isr(struct AdapterControlBlock *pACB)
{
	u32 host_interrupt_status;
	struct MessageUnit_D __iomem *pmu = pACB->pmuD;

	host_interrupt_status = readl(pmu->host_int_status);
	do {
		/* MU post queue interrupts*/
		if (host_interrupt_status & ARCMSR_ARC1214_OUTBOUND_POSTQUEUE_ISR) {
			arcmsr_hbaD_postqueue_isr(pACB);
		}
		if (host_interrupt_status & ARCMSR_ARC1214_OUTBOUND_DOORBELL_ISR) {
			arcmsr_hbaD_doorbell_isr(pACB);
		}
		host_interrupt_status = readl(pmu->host_int_status);
	} while (host_interrupt_status & (ARCMSR_ARC1214_OUTBOUND_POSTQUEUE_ISR | ARCMSR_ARC1214_OUTBOUND_DOORBELL_ISR));
	return IRQ_HANDLED;
}

static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			return arcmsr_hbaA_handle_isr(acb);
			break;
		} 
		case ACB_ADAPTER_TYPE_B: {
			return arcmsr_hbaB_handle_isr(acb);
			break;
		} 
		case ACB_ADAPTER_TYPE_C: {
			return arcmsr_hbaC_handle_isr(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			return arcmsr_hbaD_handle_isr(acb);
			break;
		}
		default:
			return IRQ_NONE;
	}
}

static void arcmsr_iop_parking(struct AdapterControlBlock *acb)
{
	/* stop adapter background rebuild */
	if (acb && acb->acb_flags & ACB_F_MSG_START_BGRB) {
		uint32_t intmask_org;
		acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
		intmask_org = arcmsr_disable_outbound_ints(acb);
		arcmsr_stop_adapter_bgrb(acb);
		arcmsr_flush_adapter_cache(acb);
		arcmsr_enable_outbound_ints(acb, intmask_org);
	}
}

static void arcmsr_clear_iop2drv_rqueue_buffer(struct AdapterControlBlock *acb)
{
	uint32_t	i;

	if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
		for(i=0; i < 15; i++) {
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				acb->rqbuf_firstindex = 0;
				acb->rqbuf_lastindex = 0;
				arcmsr_iop_message_read(acb);
				mdelay(30);
			} else if (acb->rqbuf_firstindex != acb->rqbuf_lastindex) {
				acb->rqbuf_firstindex = 0;
				acb->rqbuf_lastindex = 0;
				mdelay(30);
			} else 
				break;
		}
	}
}

static int arcmsr_iop_message_xfer(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	char *buffer;
	unsigned short use_sg;
	int retvalue = 0, transfer_len = 0;
	unsigned long flags;
	struct CMD_MESSAGE_FIELD *pcmdmessagefld;
	uint32_t	controlcode = (uint32_t )cmd->cmnd[5] << 24 | (uint32_t )cmd->cmnd[6] << 16 | (uint32_t )cmd->cmnd[7] << 8 | (uint32_t )cmd->cmnd[8]; /* 4 bytes: Areca io control code */
	struct scatterlist *sg;

	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
		use_sg = scsi_sg_count(cmd);
		sg = scsi_sglist(cmd);
	#else
		use_sg = cmd->use_sg;
		sg = (struct scatterlist *)cmd->request_buffer;
	#endif
	if (use_sg > 1) {
		printk("%s: ARCMSR_MESSAGE_FAIL(use_sg > 1)\n", __func__);
		retvalue = ARCMSR_MESSAGE_FAIL;
		return retvalue;
	}
	if (use_sg) {
	#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
		buffer = kmap_atomic(sg_page(sg)) + sg->offset;
	#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
		buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
	#else
		buffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
	#endif
		transfer_len = sg->length;
	} else {
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 23)
		buffer = (char *)scsi_sglist(cmd);
		transfer_len = scsi_bufflen(cmd);
	#else
		buffer = cmd->request_buffer;
		transfer_len = cmd->request_bufflen;
	#endif
	}
	if (transfer_len > sizeof(struct CMD_MESSAGE_FIELD)) {
		retvalue = ARCMSR_MESSAGE_FAIL;
		printk("%s: ARCMSR_MESSAGE_FAIL(transfer_len > sizeof(struct CMD_MESSAGE_FIELD))\n", __func__);
		goto message_out;
	}
	pcmdmessagefld = (struct CMD_MESSAGE_FIELD *)buffer;
	switch(controlcode) {
		case ARCMSR_MESSAGE_READ_RQBUFFER: {
			unsigned char *ver_addr;
			uint8_t *pQbuffer, *ptmpQbuffer;
			uint32_t allxfer_len = 0;
			ptmpQbuffer = ver_addr = kmalloc(1032, GFP_ATOMIC);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				printk("%s: memory not enough!\n", __func__);
				goto message_out;
			}
			spin_lock_irqsave(&acb->rqbuffer_lock, flags);
			if (acb->rqbuf_firstindex != acb->rqbuf_lastindex) {
				pQbuffer = &acb->rqbuffer[acb->rqbuf_firstindex];
				if (acb->rqbuf_firstindex > acb->rqbuf_lastindex) {
					if ((ARCMSR_MAX_QBUFFER -acb->rqbuf_firstindex) >= 1032) {
						memcpy(ptmpQbuffer, pQbuffer, 1032);
						acb->rqbuf_firstindex += 1032;
						acb->rqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
						allxfer_len = 1032;
					} else {
						if (((ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex) + acb->rqbuf_lastindex) > 1032) {
							memcpy(ptmpQbuffer, pQbuffer, ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex);
							ptmpQbuffer += ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex;
							memcpy(ptmpQbuffer, acb->rqbuffer, 1032 -(ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex));
							acb->rqbuf_firstindex = 1032 - (ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex);
							allxfer_len = 1032;
						} else {
							memcpy(ptmpQbuffer, pQbuffer, ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex);
							ptmpQbuffer += ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex;
							memcpy(ptmpQbuffer, acb->rqbuffer, acb->rqbuf_lastindex);
							allxfer_len = (ARCMSR_MAX_QBUFFER - acb->rqbuf_firstindex) + acb->rqbuf_lastindex;
							acb->rqbuf_firstindex = acb->rqbuf_lastindex;
						}
					}
				} else {
					if ((acb->rqbuf_lastindex - acb->rqbuf_firstindex) > 1032) {
						memcpy(ptmpQbuffer, pQbuffer, 1032);
						acb->rqbuf_firstindex += 1032;
						allxfer_len = 1032;
					} else {
						memcpy(ptmpQbuffer, pQbuffer, acb->rqbuf_lastindex - acb->rqbuf_firstindex);
						allxfer_len = acb->rqbuf_lastindex - acb->rqbuf_firstindex;
						acb->rqbuf_firstindex = acb->rqbuf_lastindex;
					}
				}
			}
			memcpy(pcmdmessagefld->messagedatabuffer, ver_addr, allxfer_len);
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				struct QBUFFER __iomem *prbuffer;
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				prbuffer = arcmsr_get_iop_rqbuffer(acb);
				if(arcmsr_Read_iop_rqbuffer_data(acb, prbuffer) == 0)
					acb->acb_flags |= ACB_F_IOPDATA_OVERFLOW;
			}
			spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
			kfree(ver_addr);
			pcmdmessagefld->cmdmessage.Length = allxfer_len;
			if (acb->fw_flag == FW_DEADLOCK)
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			else
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			break;
		}
		case ARCMSR_MESSAGE_WRITE_WQBUFFER: {
			unsigned char *ver_addr;
			int32_t my_empty_len, user_len, wqbuf_firstindex, wqbuf_lastindex;
			uint8_t *pQbuffer, *ptmpuserbuffer;
			ver_addr = kmalloc(1032, GFP_ATOMIC);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				printk("%s: memory not enough!\n", __func__);
				goto message_out;
			}
			spin_lock_irqsave(&acb->wqbuffer_lock, flags);
			ptmpuserbuffer = ver_addr;
			user_len = pcmdmessagefld->cmdmessage.Length;
			memcpy(ptmpuserbuffer, pcmdmessagefld->messagedatabuffer, user_len);
			wqbuf_lastindex = acb->wqbuf_lastindex;
			wqbuf_firstindex = acb->wqbuf_firstindex;
			if (wqbuf_lastindex != wqbuf_firstindex) {
				struct SENSE_DATA *sensebuffer = (struct SENSE_DATA *)cmd->sense_buffer;
				arcmsr_write_ioctldata2iop(acb);
				/* has error report sensedata */
				sensebuffer->ErrorCode = SCSI_SENSE_CURRENT_ERRORS;
				sensebuffer->SenseKey = ILLEGAL_REQUEST;
				sensebuffer->AdditionalSenseLength = 0x0A;
				sensebuffer->AdditionalSenseCode = 0x20;
				sensebuffer->Valid = 1;
				retvalue = ARCMSR_MESSAGE_FAIL;
			} else {
				my_empty_len = (wqbuf_firstindex - wqbuf_lastindex - 1) & (ARCMSR_MAX_QBUFFER - 1);
				if (my_empty_len >= user_len) {
					while (user_len > 0) {
						pQbuffer = &acb->wqbuffer[acb->wqbuf_lastindex];
						if ((acb->wqbuf_lastindex + user_len) > ARCMSR_MAX_QBUFFER) {
							memcpy(pQbuffer, ptmpuserbuffer, ARCMSR_MAX_QBUFFER - acb->wqbuf_lastindex);
							ptmpuserbuffer += (ARCMSR_MAX_QBUFFER - acb->wqbuf_lastindex);
							user_len -= (ARCMSR_MAX_QBUFFER - acb->wqbuf_lastindex);
							acb->wqbuf_lastindex = 0;
						} else {
							memcpy(pQbuffer, ptmpuserbuffer, user_len);
							acb->wqbuf_lastindex += user_len;
							acb->wqbuf_lastindex %= ARCMSR_MAX_QBUFFER;
							user_len = 0;
						}
					}
					if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_CLEARED) {
						acb->acb_flags &= ~ACB_F_MESSAGE_WQBUFFER_CLEARED;
						arcmsr_write_ioctldata2iop(acb);
					}
				} else {
					struct SENSE_DATA *sensebuffer = (struct SENSE_DATA *)cmd->sense_buffer;
					/* has error report sensedata */
					sensebuffer->ErrorCode = SCSI_SENSE_CURRENT_ERRORS;
					sensebuffer->SenseKey = ILLEGAL_REQUEST;
					sensebuffer->AdditionalSenseLength = 0x0A;
					sensebuffer->AdditionalSenseCode = 0x20;
					sensebuffer->Valid = 1;
					retvalue = ARCMSR_MESSAGE_FAIL;
				}
			}
			spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
			kfree(ver_addr);
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			break;
		}
		case ARCMSR_MESSAGE_CLEAR_RQBUFFER: {
			uint8_t *pQbuffer = acb->rqbuffer;

			#if ARCMSR_DBG_FUNC
			printk("%s:ARCMSR_MESSAGE_CLEAR_RQBUFFER\n", __func__);
			#endif
			arcmsr_clear_iop2drv_rqueue_buffer(acb);
			spin_lock_irqsave(&acb->rqbuffer_lock, flags);
			acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
			acb->rqbuf_firstindex = 0;
			acb->rqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			break;
		}
		case ARCMSR_MESSAGE_CLEAR_WQBUFFER: {
			uint8_t *pQbuffer = acb->wqbuffer;
			#if ARCMSR_DBG_FUNC
				printk("%s: ARCMSR_MESSAGE_CLEAR_WQBUFFER\n", __func__);
			#endif
			spin_lock_irqsave(&acb->wqbuffer_lock, flags);
			acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_WQBUFFER_READED);
			acb->wqbuf_firstindex = 0;
			acb->wqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			break;
		}
		case ARCMSR_MESSAGE_CLEAR_ALLQBUFFER: {
			uint8_t *pQbuffer;
			#if ARCMSR_DBG_FUNC
				printk("%s: ARCMSR_MESSAGE_CLEAR_ALLQBUFFER\n", __func__);
			#endif
			arcmsr_clear_iop2drv_rqueue_buffer(acb);
			spin_lock_irqsave(&acb->rqbuffer_lock, flags);
			acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
			acb->rqbuf_firstindex = 0;
			acb->rqbuf_lastindex = 0;
			pQbuffer = acb->rqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			spin_unlock_irqrestore(&acb->rqbuffer_lock, flags);
			spin_lock_irqsave(&acb->wqbuffer_lock, flags);
			acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED | ACB_F_MESSAGE_WQBUFFER_READED);
			acb->wqbuf_firstindex = 0;
			acb->wqbuf_lastindex = 0;
			pQbuffer = acb->wqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			spin_unlock_irqrestore(&acb->wqbuffer_lock, flags);
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			break;
		}
		case ARCMSR_MESSAGE_RETURN_CODE_3F: {
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_3F;
			}
			break;
		}
		case ARCMSR_MESSAGE_SAY_HELLO: {
			int8_t *hello_string = "Hello! I am ARCMSR";
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			memcpy(pcmdmessagefld->messagedatabuffer, hello_string, (int16_t)strlen(hello_string));
			break;
		}
		case ARCMSR_MESSAGE_SAY_GOODBYE: {
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			arcmsr_iop_parking(acb);
			break;
		}
		case ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE: {
			if (acb->fw_flag == FW_DEADLOCK) {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON;
			} else {
				pcmdmessagefld->cmdmessage.ReturnCode = ARCMSR_MESSAGE_RETURNCODE_OK;
			}
			arcmsr_flush_adapter_cache(acb);
			break;
		}
		default:
			retvalue = ARCMSR_MESSAGE_FAIL;
			printk("unknown controlcode(%d)\n", __LINE__);
	}
	message_out:
	if (use_sg) {
		#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
			kunmap_atomic(buffer - sg->offset);
		#else
			kunmap_atomic(buffer - sg->offset, KM_IRQ0);
		#endif
	}
	return retvalue;
}

static struct CommandControlBlock * arcmsr_get_freeccb(struct AdapterControlBlock *acb)
{
	unsigned long flags;
	struct list_head *head = &acb->ccb_free_list;
	struct CommandControlBlock *ccb;

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

static void arcmsr_handle_virtual_command(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	switch (cmd->cmnd[0]) {
		case INQUIRY: {
			unsigned char inqdata[36];
			char *buffer;
			unsigned short use_sg;
			struct scatterlist *sg;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
				sg = scsi_sglist(cmd);
			#else
				sg = (struct scatterlist *) cmd->request_buffer;
			#endif
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
				#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
					buffer = kmap_atomic(sg_page(sg)) + sg->offset;
				#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
					buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
				#else
					buffer = kmap_atomic(sg->page, KM_IRQ0) + sg->offset;
				#endif
			} else {
				#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
					buffer = kmap_atomic(sg_page(sg)) + sg->offset;
				#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 24)
					buffer = kmap_atomic(sg_page(sg), KM_IRQ0) + sg->offset;
				#else
					buffer = cmd->request_buffer;
				#endif
			}
			memcpy(buffer, inqdata, sizeof(inqdata));
			#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 2, 0)
				if (scsi_sg_count(cmd)) {
					kunmap_atomic(buffer - sg->offset);
				}
			#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
				if (scsi_sg_count(cmd)) {
					kunmap_atomic(buffer - sg->offset, KM_IRQ0);
				}
			#else
				if (cmd->use_sg) {
					kunmap_atomic(buffer - sg->offset, KM_IRQ0);
				}
			#endif
			cmd->scsi_done(cmd);
			break;
		}
		case WRITE_BUFFER:
		case READ_BUFFER: {
			if (arcmsr_iop_message_xfer(acb, cmd))
				cmd->result = (DID_ERROR << 16);
			cmd->scsi_done(cmd);
			break;
		}
		default:
			cmd->scsi_done(cmd);
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
	int arcmsr_queue_command_lck(struct scsi_cmnd *cmd, void (* done)(struct scsi_cmnd *))
	{
		uint8_t scsicmd = cmd->cmnd[0];
		int target = cmd->device->id;
		int lun = cmd->device->lun;
		struct Scsi_Host *host = cmd->device->host;
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *) host->hostdata;
		struct CommandControlBlock *ccb;

		cmd->scsi_done = done;
		cmd->host_scribble = NULL;
		cmd->result = 0;
	 	if ((scsicmd == SYNCHRONIZE_CACHE) || (scsicmd == SEND_DIAGNOSTIC)) {
			if (acb->devstate[target][lun] == ARECA_RAID_GONE)
		    	cmd->result = (DID_NO_CONNECT << 16);
			cmd->scsi_done(cmd);
			return 0;
		}
		if (target == 16) {
			/* virtual device for iop message transfer */
			arcmsr_handle_virtual_command(acb, cmd);
			return 0;
		}
		if (atomic_read(&acb->ccboutstandingcount) >= acb->maxOutstanding)
			return SCSI_MLQUEUE_HOST_BUSY;
	    ccb = arcmsr_get_freeccb(acb);
		if (!ccb)
			return SCSI_MLQUEUE_HOST_BUSY;
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
		int target = cmd->device->id;
		int lun = cmd->device->lun;
		uint8_t scsicmd = cmd->cmnd[0];
		struct Scsi_Host *host = cmd->device->host;
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *) host->hostdata;
		struct CommandControlBlock *ccb;

		cmd->scsi_done = done;
		cmd->host_scribble = NULL;
		cmd->result = 0;
	 	if ((scsicmd == SYNCHRONIZE_CACHE) || (scsicmd == SEND_DIAGNOSTIC)) {
			if (acb->devstate[target][lun] == ARECA_RAID_GONE)
		    		cmd->result = (DID_NO_CONNECT << 16);
			cmd->scsi_done(cmd);
			return 0;
		}
		if (target == 16) {
			/* virtual device for iop message transfer */
			arcmsr_handle_virtual_command(acb, cmd);
			return 0;
		}
		if (atomic_read(&acb->ccboutstandingcount) >= acb->maxOutstanding)
			return SCSI_MLQUEUE_HOST_BUSY;
	    ccb = arcmsr_get_freeccb(acb);
		if (!ccb)
			return SCSI_MLQUEUE_HOST_BUSY;
		if (arcmsr_build_ccb(acb, ccb, cmd) == FAILED) {
			cmd->result = (DID_ERROR << 16) |(RESERVATION_CONFLICT << 1);
			cmd->scsi_done(cmd);
			return 0;
		}
		arcmsr_post_ccb(acb, ccb);
		return 0;
	}
#endif

bool arcmsr_hbaA_get_config(struct AdapterControlBlock *acb)
{
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	int count;
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	char __iomem *iop_firm_model = (char __iomem *) (&reg->msgcode_rwbuffer[15]);
	char __iomem *iop_firm_version = (char __iomem *) (&reg->msgcode_rwbuffer[17]);
	char __iomem *iop_device_map = (char __iomem *) (&reg->msgcode_rwbuffer[21]);

	writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, &reg->inbound_msgaddr0);
	if (!arcmsr_hbaA_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'get adapter firmware miscellaneous data' timeout \n", acb->host->host_no);
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
		*acb_device_map = readb(iop_device_map);
		acb_device_map++;
		iop_device_map++;
		count--;
 	}
	acb->signature = readl(&reg->msgcode_rwbuffer[0]);
	acb->firm_request_len = readl(&reg->msgcode_rwbuffer[1]);
	acb->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[2]);
	acb->firm_sdram_size = readl(&reg->msgcode_rwbuffer[3]);
	acb->firm_hd_channels = readl(&reg->msgcode_rwbuffer[4]);
	acb->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	printk("Areca RAID Controller%d: Model %s, F/W %s\n", acb->host->host_no, acb->firm_model, acb->firm_version);
	return true;
}

bool arcmsr_hbaB_get_config(struct AdapterControlBlock *acb)
{
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model;	/*firm_model,15,60-67*/
	char __iomem *iop_firm_version;	/*firm_version,17,68-83*/
	char __iomem *iop_device_map;	/*firm_version,21,84-99*/
 	int count;
 	uint32_t __iomem *lrwbuffer;
	struct MessageUnit_B __iomem *reg = acb->pmuB;
	struct pci_dev *pdev = acb->pdev;
	void *dma_coherent;
	dma_addr_t dma_coherent_handle;

	dma_coherent = dma_alloc_coherent(&pdev->dev, sizeof(struct MessageUnit_B), &dma_coherent_handle, GFP_KERNEL);
	if (!dma_coherent) {
		printk("arcmsr%d: dma_alloc_coherent got error for hba mu\n", acb->host->host_no);
		return false;
	}
	acb->dma_coherent_handle2 = dma_coherent_handle;
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
	if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait get adapter firmware miscellaneous data timeout\n", acb->host->host_no);
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
	count = 16;
	while (count) {
		*acb_device_map = readb(iop_device_map);
		acb_device_map++;
		iop_device_map++;
		count--;
	}
	acb->signature = readl(&reg->msgcode_rwbuffer[1]);
	/*firm_signature,1,00-03*/
	acb->firm_request_len = readl(&reg->msgcode_rwbuffer[2]); 
	/*firm_request_len,1,04-07*/
	acb->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[3]); 
	/*firm_numbers_queue,2,08-11*/
	acb->firm_sdram_size = readl(&reg->msgcode_rwbuffer[4]); 
	/*firm_sdram_size,3,12-15*/
	acb->firm_hd_channels = readl(&reg->msgcode_rwbuffer[5]); 
	/*firm_hd_channels,4,16-19*/
	acb->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	printk("Areca RAID Controller%d: Model %s, F/W %s\n", acb->host->host_no, acb->firm_model, acb->firm_version);
	return true;
}

static bool arcmsr_hbaC_get_config(struct AdapterControlBlock *pACB)
{
	char *acb_firm_model = pACB->firm_model;
	char *acb_firm_version = pACB->firm_version;
	struct MessageUnit_C __iomem *reg = pACB->pmuC;
	char *iop_firm_model = (char *)(&reg->msgcode_rwbuffer[15]);    /*firm_model,15,60-67*/
	char *iop_firm_version = (char *)(&reg->msgcode_rwbuffer[17]);  /*firm_version,17,68-83*/
	uint32_t intmask_org, Index, firmware_state = 0;
	int count;

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
		printk(KERN_NOTICE "arcmsr%d: wait 'get adapter firmware miscellaneous data' timeout \n", pACB->host->host_no);
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
	pACB->firm_request_len = readl(&reg->msgcode_rwbuffer[1]);   /*firm_request_len,1,04-07*/
	pACB->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[2]); /*firm_numbers_queue,2,08-11*/
	pACB->firm_sdram_size = readl(&reg->msgcode_rwbuffer[3]);    /*firm_sdram_size,3,12-15*/
	pACB->firm_hd_channels = readl(&reg->msgcode_rwbuffer[4]);  /*firm_ide_channels,4,16-19*/
	pACB->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	printk("Areca RAID Controller%d: Model %s, F/W %s\n", pACB->host->host_no, pACB->firm_model, pACB->firm_version);
	/*all interrupt service will be enable at arcmsr_iop_init*/
	return true;
}

 bool arcmsr_hbaD_get_config(struct AdapterControlBlock *acb)
{
	char *acb_firm_model = acb->firm_model;
	char *acb_firm_version = acb->firm_version;
	char *acb_device_map = acb->device_map;
	char __iomem *iop_firm_model;	/*firm_model,15,60-67*/
	char __iomem *iop_firm_version;	/*firm_version,17,68-83*/
	char __iomem *iop_device_map;	/*firm_version,21,84-99*/
 	u32 count;
	struct MessageUnit_D __iomem *reg ;
	void *dma_coherent;
	dma_addr_t dma_coherent_handle;
	struct pci_dev *pdev = acb->pdev;

	acb->roundup_ccbsize = roundup(sizeof(struct MessageUnit_D), 32);
	dma_coherent = dma_alloc_coherent(&pdev->dev, acb->roundup_ccbsize, &dma_coherent_handle, GFP_KERNEL);
	if (!dma_coherent) {
		printk(KERN_NOTICE "DMA allocation failed...........................\n");
		return false;
	}
	memset(dma_coherent, 0, acb->roundup_ccbsize);
	acb->dma_coherent2 = dma_coherent;
	acb->dma_coherent_handle2 = dma_coherent_handle;
	reg = (struct MessageUnit_D __iomem *)dma_coherent;
	acb->pmuD = reg;
	reg->chip_id = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_CHIP_ID);
	reg->cpu_mem_config = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_CPU_MEMORY_CONFIGURATION);
	reg->i2o_host_interrupt_mask = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_I2_HOST_INTERRUPT_MASK);
	reg->sample_at_reset = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_SAMPLE_RESET);
	reg->reset_request = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_RESET_REQUEST);
	reg->host_int_status = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_MAIN_INTERRUPT_STATUS);
	reg->pcief0_int_enable = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_PCIE_F0_INTERRUPT_ENABLE);
	reg->inbound_msgaddr0 = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_MESSAGE0);
	reg->inbound_msgaddr1 = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_MESSAGE1);
	reg->outbound_msgaddr0 = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_MESSAGE0);
	reg->outbound_msgaddr1 = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_MESSAGE1);
	reg->inbound_doorbell = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_DOORBELL);
	reg->outbound_doorbell = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_DOORBELL);
	reg->outbound_doorbell_enable = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_DOORBELL_ENABLE);
	reg->inboundlist_base_low = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_LIST_BASE_LOW);
	reg->inboundlist_base_high = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_LIST_BASE_HIGH);
	reg->inboundlist_write_pointer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_INBOUND_LIST_WRITE_POINTER);
	reg->outboundlist_base_low = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_LIST_BASE_LOW);
	reg->outboundlist_base_high = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_LIST_BASE_HIGH);
	reg->outboundlist_copy_pointer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_LIST_COPY_POINTER);
	reg->outboundlist_read_pointer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_LIST_READ_POINTER);
	reg->outboundlist_interrupt_cause = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_INTERRUPT_CAUSE);
	reg->outboundlist_interrupt_enable = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_OUTBOUND_INTERRUPT_ENABLE);
	reg->message_wbuffer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_MESSAGE_WBUFFER);
	reg->message_rbuffer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_MESSAGE_RBUFFER);
	reg->msgcode_rwbuffer = (u32 __iomem *)((unsigned long)acb->mem_base0 + ARCMSR_ARC1214_MESSAGE_RWBUFFER);
	iop_firm_model = (char __iomem *)(&reg->msgcode_rwbuffer[15]);/*firm_model,15,60-67*/
	iop_firm_version = (char __iomem *)(&reg->msgcode_rwbuffer[17]);/*firm_version,17,68-83*/
	iop_device_map = (char __iomem *)(&reg->msgcode_rwbuffer[21]);/*firm_version,21,84-99*/
	/* disable all outbound interrupt */
	if (readl(acb->pmuD->outbound_doorbell) & ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE) {
		writel(ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE, acb->pmuD->outbound_doorbell);/*clear interrupt*/
	}
	/* post "get config" instruction */
	writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, reg->inbound_msgaddr0);
	/* wait message ready */
	if (!arcmsr_hbaD_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait get adapter firmware miscellaneous data timeout\n", acb->host->host_no);
		dma_free_coherent(&acb->pdev->dev, acb->roundup_ccbsize, acb->dma_coherent2, acb->dma_coherent_handle2);
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
	count = 16;
	while (count) {
		*acb_device_map = readb(iop_device_map);
		acb_device_map++;
		iop_device_map++;
		count--;
	}
	acb->signature = readl(&reg->msgcode_rwbuffer[1]);
	/*firm_signature,1,00-03*/
	acb->firm_request_len = readl(&reg->msgcode_rwbuffer[2]);
	/*firm_request_len,1,04-07*/
	acb->firm_numbers_queue = readl(&reg->msgcode_rwbuffer[3]);
	/*firm_numbers_queue,2,08-11*/
	acb->firm_sdram_size = readl(&reg->msgcode_rwbuffer[4]);
	/*firm_sdram_size,3,12-15*/
	acb->firm_hd_channels = readl(&reg->msgcode_rwbuffer[5]);
	/*firm_hd_channels,4,16-19*/
	acb->firm_cfg_version = readl(&reg->msgcode_rwbuffer[25]);  /*firm_cfg_version,25,100-103*/
	printk("Areca RAID Controller%d: Model %s, F/W %s\n", acb->host->host_no, acb->firm_model, acb->firm_version);
	return true;
}

static bool arcmsr_get_firmware_spec(struct AdapterControlBlock *acb)
{
	bool rtn = false;
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
			rtn = arcmsr_hbaA_get_config(acb);
			break;
		case ACB_ADAPTER_TYPE_B:
			rtn = arcmsr_hbaB_get_config(acb);
			break;
		case ACB_ADAPTER_TYPE_C:
			rtn = arcmsr_hbaC_get_config(acb);
			break;
		case ACB_ADAPTER_TYPE_D:
			rtn = arcmsr_hbaD_get_config(acb);
			break;
		default:
			break;
	}
	if(acb->firm_numbers_queue > ARCMSR_MAX_FREECCB_NUM)
		acb->maxOutstanding = ARCMSR_MAX_FREECCB_NUM-1;
	else
		acb->maxOutstanding = acb->firm_numbers_queue - 1;
	return rtn;
}

static void arcmsr_hbaA_request_device_map(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	if (unlikely(atomic_read(&acb->rq_map_token) == 0) || ((acb->acb_flags & ACB_F_BUS_RESET) != 0 ) || ((acb->acb_flags & ACB_F_ABORT) != 0 )) {
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
}

static void arcmsr_hbaB_request_device_map(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B __iomem *reg = acb->pmuB;

	if (unlikely(atomic_read(&acb->rq_map_token) == 0) || ((acb->acb_flags & ACB_F_BUS_RESET) != 0 ) || ((acb->acb_flags & ACB_F_ABORT) != 0 )) {
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
}

static void arcmsr_hbaC_request_device_map(struct AdapterControlBlock *acb)
{
	struct MessageUnit_C __iomem *reg = acb->pmuC;

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
}

static void arcmsr_hbaD_request_device_map(struct AdapterControlBlock *acb)
{
	struct MessageUnit_D __iomem *reg = acb->pmuD;

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
		writel(ARCMSR_INBOUND_MESG0_GET_CONFIG, reg->inbound_msgaddr0);
		mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
	}
}

static void arcmsr_request_device_map(unsigned long pacb)
{
	struct AdapterControlBlock *acb = (struct AdapterControlBlock *)pacb;

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			arcmsr_hbaA_request_device_map(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			arcmsr_hbaB_request_device_map(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			arcmsr_hbaC_request_device_map(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			arcmsr_hbaD_request_device_map(acb);
			break;
		}
	}
}

static bool arcmsr_hbaA_start_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_A __iomem *reg = acb->pmuA;

	acb->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, &reg->inbound_msgaddr0);
	if (!arcmsr_hbaA_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", acb->host->host_no);
		return false;
	}
	return true;
}

static bool arcmsr_hbaB_start_bgrb(struct AdapterControlBlock *acb)
{
	struct MessageUnit_B __iomem *reg = acb->pmuB;

	acb->acb_flags |= ACB_F_MSG_START_BGRB;
    	writel(ARCMSR_MESSAGE_START_BGRB, reg->drv2iop_doorbell);
	if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", acb->host->host_no);
		return false;
	}
	return true;
}

static bool arcmsr_hbaC_start_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_C __iomem *pmu = pACB->pmuC;

	pACB->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, &pmu->inbound_msgaddr0);
	writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &pmu->inbound_doorbell);
	if (!arcmsr_hbaC_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", pACB->host->host_no);
		return false;
	}
	return true;
}

static bool arcmsr_hbaD_start_bgrb(struct AdapterControlBlock *pACB)
{
	struct MessageUnit_D __iomem *pmu = pACB->pmuD;

	pACB->acb_flags |= ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_START_BGRB, pmu->inbound_msgaddr0);
	if (!arcmsr_hbaD_wait_msgint_ready(pACB)) {
		printk(KERN_NOTICE "arcmsr%d: wait 'start adapter background rebulid' timeout \n", pACB->host->host_no);
		return false;
	}
	return true;
}

static bool arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb)
{
	bool rtn = true;
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
			rtn = arcmsr_hbaA_start_bgrb(acb);
			break;
		case ACB_ADAPTER_TYPE_B:
			rtn = arcmsr_hbaB_start_bgrb(acb);
			break;
		case ACB_ADAPTER_TYPE_C:
			rtn = arcmsr_hbaC_start_bgrb(acb);
			break;
		case ACB_ADAPTER_TYPE_D:
			rtn = arcmsr_hbaD_start_bgrb(acb);
			break;
	}
	return rtn;
}

static int arcmsr_hbaA_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	bool error;
	uint32_t flag_ccb, outbound_intstatus, poll_ccb_done = 0, poll_count = 0;
	int id, lun, rtn;	
	struct MessageUnit_A __iomem *reg = acb->pmuA;
	struct CommandControlBlock *ccb;
	struct ARCMSR_CDB *arcmsr_cdb;

 	polling_hbaA_ccb_retry:
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
				if (poll_count > 100) {
					rtn = FAILED;
					break;
				}
				goto polling_hbaA_ccb_retry;
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
					" poll command abort successfully \n", acb->host->host_no, ccb->pcmd->device->id, ccb->pcmd->device->lun, ccb);
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb);
				continue;
			}
			printk(KERN_ERR "arcmsr%d: polling an illegal ccb"
				" command done ccb = '0x%p'"
				" ccboutstandingcount = %d \n"
				, acb->host->host_no
				, ccb
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_report_ccb_state(acb, ccb, error);
	}
	return rtn;
}

static int arcmsr_hbaB_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	bool error;
	uint32_t flag_ccb, poll_ccb_done = 0, poll_count = 0;
	int32_t index;
	int id, lun, rtn;
	struct MessageUnit_B __iomem *reg = acb->pmuB;
	struct CommandControlBlock *ccb;
	struct ARCMSR_CDB *arcmsr_cdb;

	polling_hbaB_ccb_retry:
	poll_count++;
	writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell); /* clear doorbell interrupt */
	while (1) {
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
				goto polling_hbaB_ccb_retry;
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
					,acb->host->host_no
					,ccb->pcmd->device->id
					,ccb->pcmd->device->lun
					,ccb);
				ccb->pcmd->result = DID_ABORT << 16;
				arcmsr_ccb_complete(ccb);
				continue;
			}
			printk(KERN_NOTICE "arcmsr%d: polling an illegal ccb"
				" command done ccb = '0x%p'"
				"ccboutstandingcount = %d \n"
				, acb->host->host_no
				, ccb
				, atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
		arcmsr_report_ccb_state(acb, ccb, error);
	}	/*drain reply FIFO*/
	return rtn;
}

static int arcmsr_hbaC_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	bool error;
	uint32_t poll_ccb_done = 0, poll_count = 0, flag_ccb, ccb_cdb_phy;
	int rtn;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *pCCB;
	struct MessageUnit_C __iomem *reg = acb->pmuC;

	polling_hbaC_ccb_retry:
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
				goto polling_hbaC_ccb_retry;
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
			printk(KERN_NOTICE "arcmsr%d: polling an illegal ccb"
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

static int arcmsr_hbaD_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	bool error;
	uint32_t poll_ccb_done = 0, poll_count = 0, flag_ccb, ccb_cdb_phy;
	int rtn, doneq_index, index_stripped, outbound_write_pointer;
	unsigned long flags;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *pCCB;
	struct MessageUnit_D __iomem *pmu = acb->pmuD;

	spin_lock_irqsave(&acb->doneq_lock, flags);
	polling_hbaD_ccb_retry:
	poll_count++;
	while (1) {
		outbound_write_pointer = pmu->done_qbuffer[0].addressLow + 1;
		doneq_index = pmu->doneq_index;
		if ((outbound_write_pointer & 0xFFF) == (doneq_index & 0xFFF)) {
			if (poll_ccb_done) {
				rtn = SUCCESS;
				break;
			} else {
				arc_mdelay(25);
				if (poll_count > 100) {
					rtn = FAILED;
					break;
				}
				goto polling_hbaD_ccb_retry;
			}
		}
		if (doneq_index & 0x4000) {
			index_stripped = doneq_index & 0xFFF;
			index_stripped += 1;
			index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
			pmu->doneq_index = index_stripped ? (index_stripped | 0x4000) : (index_stripped + 1);
		} else {
			index_stripped = doneq_index;
			index_stripped += 1;
			index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
			pmu->doneq_index = index_stripped ? index_stripped : ((index_stripped | 0x4000) + 1);
		}
		doneq_index = pmu->doneq_index;
		flag_ccb = pmu->done_qbuffer[doneq_index & 0xFFF].addressLow;
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
			printk(KERN_NOTICE "arcmsr%d: polling an illegal ccb"
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
	spin_unlock_irqrestore(&acb->doneq_lock, flags);
	return rtn;
}

static int arcmsr_polling_ccbdone(struct AdapterControlBlock *acb, struct CommandControlBlock *poll_ccb)
{
	int rtn = 0;

	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A:{
			rtn = arcmsr_hbaA_polling_ccbdone(acb, poll_ccb);
			break;
		}
		case ACB_ADAPTER_TYPE_B:{
			rtn = arcmsr_hbaB_polling_ccbdone(acb, poll_ccb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			rtn = arcmsr_hbaC_polling_ccbdone(acb, poll_ccb);
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			rtn = arcmsr_hbaD_polling_ccbdone(acb, poll_ccb);
			break;
		}
	}
	return rtn;
}

static void arcmsr_done4abort_postqueue(struct AdapterControlBlock *acb)
{
	bool error;
	int i = 0;
	uint32_t flag_ccb;
	struct ARCMSR_CDB *arcmsr_cdb;
	struct CommandControlBlock *pCCB;

	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			struct MessageUnit_A __iomem *reg = acb->pmuA;
			uint32_t outbound_intstatus;
			outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
			/*clear and abort all outbound posted Q*/
			writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
			while (((flag_ccb = readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < ARCMSR_MAX_OUTSTANDING_CMD)) {
				arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
				pCCB = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
				error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
				arcmsr_drain_donequeue(acb, pCCB, error);
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			struct MessageUnit_B __iomem *reg = acb->pmuB;
			/*clear all outbound posted Q*/
			writel(ARCMSR_DOORBELL_INT_CLEAR_PATTERN, reg->iop2drv_doorbell);
			for (i = 0; i < ARCMSR_MAX_HBB_POSTQUEUE; i++) {
				if ((flag_ccb = readl(&reg->done_qbuffer[i])) != 0) {
					writel(0, &reg->done_qbuffer[i]);
					arcmsr_cdb = (struct ARCMSR_CDB *)(acb->vir2phy_offset + (flag_ccb << 5));/*frame must be 32 bytes aligned*/
					pCCB = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
					error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE0) ? true : false;
					arcmsr_drain_donequeue(acb, pCCB, error);
				}
				reg->post_qbuffer[i] = 0;
			}
			reg->doneq_index = 0;
			reg->postq_index = 0;
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			struct MessageUnit_C __iomem *reg = acb->pmuC;
			uint32_t ccb_cdb_phy;
			bool error;
			struct CommandControlBlock *pCCB;
			while ((readl(&reg->host_int_status) & ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR) && (i++ < ARCMSR_MAX_OUTSTANDING_CMD)) {
				/*need to do*/
				flag_ccb = readl(&reg->outbound_queueport_low);
				ccb_cdb_phy = (flag_ccb & 0xFFFFFFF0);
				arcmsr_cdb = (struct  ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);/*frame must be 32 bytes aligned*/
				pCCB = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
				error = (flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
				arcmsr_drain_donequeue(acb, pCCB, error);
			}
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			struct MessageUnit_D __iomem *pmu = acb->pmuD;
			uint32_t ccb_cdb_phy, outbound_write_pointer, doneq_index, index_stripped, addressLow, residual;
			bool error;
			struct CommandControlBlock *pCCB;
			outbound_write_pointer = pmu->done_qbuffer[0].addressLow + 1;
			doneq_index = pmu->doneq_index;
			residual = atomic_read(&acb->ccboutstandingcount);
			for (i = 0; i < residual; i++) {
				while ((doneq_index & 0xFFF) != (outbound_write_pointer & 0xFFF)) {
					if (doneq_index & 0x4000) {
						index_stripped = doneq_index & 0xFFF;
						index_stripped += 1;
						index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
						pmu->doneq_index = index_stripped ? (index_stripped | 0x4000) : (index_stripped + 1);
					} else {
						index_stripped = doneq_index;
						index_stripped += 1;
						index_stripped %= ARCMSR_MAX_ARC1214_DONEQUEUE;
						pmu->doneq_index = index_stripped ? index_stripped : ((index_stripped | 0x4000) + 1);
					}
					doneq_index = pmu->doneq_index;
					addressLow = pmu->done_qbuffer[doneq_index & 0xFFF].addressLow;
					ccb_cdb_phy = (addressLow & 0xFFFFFFF0);
					arcmsr_cdb = (struct  ARCMSR_CDB *)(acb->vir2phy_offset + ccb_cdb_phy);
					pCCB = container_of(arcmsr_cdb, struct CommandControlBlock, arcmsr_cdb);
					error = (addressLow & ARCMSR_CCBREPLY_FLAG_ERROR_MODE1) ? true : false;
					arcmsr_drain_donequeue(acb, pCCB, error);
					writel(doneq_index, pmu->outboundlist_read_pointer);
				}
				mdelay(10);
				outbound_write_pointer = pmu->done_qbuffer[0].addressLow + 1;
				doneq_index = pmu->doneq_index;
			}
			pmu->postq_index = 0;
			pmu->doneq_index = 0;
			break;
		}
	}
}

static bool arcmsr_hbaB_enable_driver_mode(struct AdapterControlBlock *pacb)
{
	struct MessageUnit_B __iomem *reg = pacb->pmuB;

	writel(ARCMSR_MESSAGE_START_DRIVER_MODE, reg->drv2iop_doorbell);
	if (!arcmsr_hbaB_wait_msgint_ready(pacb)) {
		printk(KERN_ERR "arcmsr%d: can't set driver mode. \n", pacb->host->host_no);
		return false;
	}
	return true;
}

static bool arcmsr_iop_confirm(struct AdapterControlBlock *acb)
{
	uint32_t cdb_phyaddr_hi32, cdb_phyaddr_lo32;
	dma_addr_t dma_coherent_handle;

	/*
	********************************************************************
	** Here we need to tell iop our freeccb.HighPart
	** if freeccb.HighPart is not zero
	********************************************************************
	*/
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_B:
		case ACB_ADAPTER_TYPE_C:
			dma_coherent_handle = acb->dma_coherent_handle;
			break;
		case ACB_ADAPTER_TYPE_D:
			dma_coherent_handle = acb->dma_coherent_handle2;
			break;
		default:
			dma_coherent_handle = acb->dma_coherent_handle;
			break;
	}
	cdb_phyaddr_hi32 = (uint32_t)((dma_coherent_handle >> 16) >> 16);
	cdb_phyaddr_lo32 = (uint32_t)(dma_coherent_handle & 0xffffffff);
	acb->cdb_phyaddr_hi32 = cdb_phyaddr_hi32;
	/*
	***********************************************************************
	**    if adapter type B, set window of "post command Q" 
	***********************************************************************
	*/	
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A: {
			if (cdb_phyaddr_hi32 != 0) {
				struct MessageUnit_A __iomem *reg = acb->pmuA;
				writel(ARCMSR_SIGNATURE_SET_CONFIG, &reg->msgcode_rwbuffer[0]);
				writel(cdb_phyaddr_hi32, &reg->msgcode_rwbuffer[1]);
				writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, &reg->inbound_msgaddr0);
				if (!arcmsr_hbaA_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "arcmsr%d: ""'set ccb high part physical address' timeout\n", acb->host->host_no);
					return false;
				}
			}
			break;
		}
		case ACB_ADAPTER_TYPE_B: {
			dma_addr_t post_queue_phyaddr;
			uint32_t __iomem *rwbuffer;
			struct MessageUnit_B __iomem *reg = acb->pmuB;

			reg->postq_index = 0;
			reg->doneq_index = 0;
			writel(ARCMSR_MESSAGE_SET_POST_WINDOW, reg->drv2iop_doorbell);
			if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d: can not set diver mode\n", acb->host->host_no);
				return false;
			}
			post_queue_phyaddr = acb->dma_coherent_handle2;
			rwbuffer = reg->msgcode_rwbuffer;
			writel(ARCMSR_SIGNATURE_SET_CONFIG, rwbuffer++);/* driver "set config" signature */
			writel(cdb_phyaddr_hi32, rwbuffer++);		/* normal should be zero */
			writel(post_queue_phyaddr, rwbuffer++);		/* postQ size (256 + 8) * 4 */
			writel(post_queue_phyaddr + 1056, rwbuffer++);	/* doneQ size (256 + 8) * 4 */
			writel(1056, rwbuffer);				/* ccb maxQ size must be --> [(256 + 8) * 4] */
			writel(ARCMSR_MESSAGE_SET_CONFIG, reg->drv2iop_doorbell);
			if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d: 'set command Q window' timeout \n", acb->host->host_no);
				return false;
			}
			arcmsr_hbaB_enable_driver_mode(acb);
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			if (cdb_phyaddr_hi32 != 0) {
				struct MessageUnit_C __iomem *reg = acb->pmuC;
				writel(ARCMSR_SIGNATURE_SET_CONFIG, &reg->msgcode_rwbuffer[0]);
				writel(cdb_phyaddr_hi32, &reg->msgcode_rwbuffer[1]);
				writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, &reg->inbound_msgaddr0);
				writel(ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE, &reg->inbound_doorbell);
				if (!arcmsr_hbaC_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "arcmsr%d: 'set command Q window' timeout \n", acb->host->host_no);
					return false;
				}
			}
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			uint32_t __iomem *rwbuffer;
			struct MessageUnit_D __iomem *reg = acb->pmuD;

			reg->postq_index = 0;
			reg->doneq_index = 0;
			rwbuffer = reg->msgcode_rwbuffer;
			writel(ARCMSR_SIGNATURE_SET_CONFIG, rwbuffer++);
			writel(cdb_phyaddr_hi32, rwbuffer++);
			writel(cdb_phyaddr_lo32, rwbuffer++);
			writel(cdb_phyaddr_lo32 + (ARCMSR_MAX_ARC1214_POSTQUEUE * sizeof(struct InBound_SRB)), rwbuffer++);
			writel(0x100, rwbuffer);
			writel(ARCMSR_INBOUND_MESG0_SET_CONFIG, reg->inbound_msgaddr0);
			if (!arcmsr_hbaD_wait_msgint_ready(acb)) {
				printk(KERN_NOTICE "arcmsr%d: 'set command Q window' timeout \n", acb->host->host_no);
				return false;
			}
			break;
		}
	}
	return true;
}

static bool arcmsr_enable_eoi_mode(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type){
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_C:	
		case ACB_ADAPTER_TYPE_D:
			return true;
		case ACB_ADAPTER_TYPE_B: {
		        	struct MessageUnit_B __iomem *reg = acb->pmuB;
				writel(ARCMSR_MESSAGE_ACTIVE_EOI_MODE, reg->drv2iop_doorbell);
				if (!arcmsr_hbaB_wait_msgint_ready(acb)) {
					printk(KERN_NOTICE "ARCMSR IOP enables EOI_MODE TIMEOUT");
					return false;
				}
		}
		break;
	}
	return true;
}

static bool arcmsr_iop_init(struct AdapterControlBlock *acb)
{
	bool rtn;
	uint32_t intmask_org;

	intmask_org = arcmsr_disable_outbound_ints(acb);
	arcmsr_wait_firmware_ready(acb);
	if ((rtn = arcmsr_iop_confirm(acb))) {
		if ((rtn = arcmsr_start_adapter_bgrb(acb))) {
			arcmsr_clear_doorbell_queue_buffer(acb);
			if ((rtn = arcmsr_enable_eoi_mode(acb))) {
				acb->acb_flags |= ACB_F_IOP_INITED;
			}
		}
	}
	arcmsr_enable_outbound_ints(acb, intmask_org);
	return rtn;
}

static void arcmsr_hardware_reset(struct AdapterControlBlock *acb)
{
	uint8_t value[64];
	int i, count = 0;
	struct MessageUnit_A __iomem *pmuA = acb->pmuA;
	struct MessageUnit_C __iomem *pmuC = acb->pmuC;
	struct MessageUnit_D __iomem *pmuD = acb->pmuD;

	printk(KERN_ERR "arcmsr%d: executing hw bus reset .....\n", acb->host->host_no);
	/* backup pci config data */
	for (i = 0; i < 64; i++) {
		pci_read_config_byte(acb->pdev, i, &value[i]);
	}
	/* hardware reset signal */
	if ((acb->dev_id == 0x1680)) {
		writel(ARCMSR_ARC1680_BUS_RESET, &pmuA->reserved1[0]);
	} else if ((acb->dev_id == 0x1880)) {
		do {
			count++;
			writel(0xF, &pmuC->write_sequence);
			writel(0x4, &pmuC->write_sequence);
			writel(0xB, &pmuC->write_sequence);
			writel(0x2, &pmuC->write_sequence);
			writel(0x7, &pmuC->write_sequence);
			writel(0xD, &pmuC->write_sequence);
		} while ((((readl(&pmuC->host_diagnostic)) & ARCMSR_ARC1880_DiagWrite_ENABLE) == 0) && (count < 5));
		writel(ARCMSR_ARC1880_RESET_ADAPTER, &pmuC->host_diagnostic);
	} else if ((acb->dev_id == 0x1214)) {
		writel(0x20, pmuD->reset_request);
	} else {
		pci_write_config_byte(acb->pdev, 0x84, 0x20);
	}
	arc_mdelay(1000);
	/* write back pci config data */
	for (i = 0; i < 64; i++) {
		pci_write_config_byte(acb->pdev, i, value[i]);
	}
}

static int arcmsr_abort_one_cmd(struct AdapterControlBlock *acb, struct CommandControlBlock *ccb)
{
	int rtn;
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

int arcmsr_abort(struct scsi_cmnd *cmd)
{
	int i;
	int rtn = FAILED;
	uint32_t intmask_org;
	struct AdapterControlBlock *acb = (struct AdapterControlBlock *)cmd->device->host->hostdata;

	printk("arcmsr: abort scsi_cmnd(0x%p), cmnd[0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x], scsi_id = 0x%2x, scsi_lun = 0x%2x.\n",
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
	intmask_org = arcmsr_disable_outbound_ints(acb);
	for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
		struct CommandControlBlock *ccb = acb->pccb_pool[i];
		if (ccb->startdone == ARCMSR_CCB_START && ccb->pcmd == cmd) {
			ccb->startdone = ARCMSR_CCB_ABORTED;
			rtn = arcmsr_abort_one_cmd(acb, ccb);
			break;
		}
	}
	arcmsr_enable_outbound_ints(acb, intmask_org);
	acb->acb_flags &= ~ACB_F_ABORT;
	return rtn;
}

static uint8_t arcmsr_iop_reset(struct AdapterControlBlock *acb)
{
	uint8_t rtnval = 0x00;
	uint32_t intmask_org;
	int i = 0;
	unsigned long flags;
	struct CommandControlBlock *ccb;

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
				arcmsr_pci_unmap_dma(ccb);
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

int arcmsr_bus_reset(struct scsi_cmnd *cmd)
{
	uint32_t intmask_org;
	int retry_count = 1;
	int rtn = FAILED;
	struct AdapterControlBlock *acb;

	acb = (struct AdapterControlBlock *) cmd->device->host->hostdata;
	printk("arcmsr%d: executing bus reset eh.....num_resets = %d, num_aborts = %d \n", acb->host->host_no, acb->num_resets, acb->num_aborts);
	arcmsr_touch_nmi_watchdog();
	acb->num_resets++;
	switch(acb->adapter_type){
		case ACB_ADAPTER_TYPE_A: {
			if (acb->acb_flags & ACB_F_BUS_RESET) {
				long timeout;
				printk(KERN_NOTICE "arcmsr%d: there is an eh bus reset proceeding.......\n", acb->host->host_no);
				timeout = wait_event_timeout(wait_q, (acb->acb_flags & ACB_F_BUS_RESET) == 0, 220 * HZ);
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
				ssleep(ARCMSR_SLEEPTIME);
				if ((readl(&reg->outbound_msgaddr1) & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK) == 0 ) {
					printk(KERN_ERR "arcmsr%d: waiting for hw bus reset return, retry=%d\n", acb->host->host_no, retry_count);
					if (retry_count > ARCMSR_RETRYCOUNT) {
						acb->fw_flag = FW_DEADLOCK;
						printk(KERN_NOTICE "arcmsr%d: waiting for hw bus reset return, RETRY TERMINATED!!\n", acb->host->host_no);
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
				arcmsr_clear_doorbell_queue_buffer(acb)/* clear Qbuffer if door bell ringed */;
				/* enable outbound Post Queue,outbound doorbell Interrupt */
				arcmsr_enable_outbound_ints(acb, intmask_org);
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);
				acb->fw_flag = FW_NORMAL;
				//init_timer(&acb->eternal_timer);
				//acb->eternal_timer.expires = jiffies + msecs_to_jiffies(6*HZ);
				//acb->eternal_timer.data = (unsigned long) acb;
				//acb->eternal_timer.function = &arcmsr_request_device_map;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				rtn = SUCCESS;
				printk(KERN_NOTICE "arcmsr%d: scsi eh bus reset succeeds\n", acb->host->host_no);
				SPIN_LOCK_IRQ_CHK(acb);
			} else {
				acb->acb_flags &= ~ACB_F_BUS_RESET;
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
					mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				//}
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
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);
				acb->fw_flag = FW_NORMAL;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				rtn = SUCCESS;
			}
			break;
		}
		case ACB_ADAPTER_TYPE_C: {
			if (acb->acb_flags & ACB_F_BUS_RESET) {
				long timeout;
				printk(KERN_NOTICE "arcmsr: there is an bus reset eh proceeding.......\n");
				timeout = wait_event_timeout(wait_q, (acb->acb_flags & ACB_F_BUS_RESET) == 0, 220 * HZ);
				if (timeout)
					return SUCCESS;
			}
			acb->acb_flags |= ACB_F_BUS_RESET;
			if (!arcmsr_iop_reset(acb)) {
				struct MessageUnit_C __iomem *reg;
				reg = acb->pmuC;
				SPIN_UNLOCK_IRQ_CHK(acb);
				arcmsr_hardware_reset(acb);
				acb->acb_flags &= ~ACB_F_IOP_INITED;
			sleep:
				ssleep(ARCMSR_SLEEPTIME);
				if ((readl(&reg->host_diagnostic) & 0x04) != 0) {
					printk(KERN_ERR "arcmsr%d: waiting for hw bus reset return, retry=%d\n", acb->host->host_no, retry_count);
					if (retry_count > ARCMSR_RETRYCOUNT) {
						acb->fw_flag = FW_DEADLOCK;
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
				arcmsr_clear_doorbell_queue_buffer(acb)/* clear Qbuffer if door bell ringed */;
				/* enable outbound Post Queue,outbound doorbell Interrupt */
				arcmsr_enable_outbound_ints(acb, intmask_org);
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);					
				acb->fw_flag = FW_NORMAL;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				rtn = SUCCESS;					
				printk(KERN_NOTICE "arcmsr: scsi bus reset eh returns with success\n");
				SPIN_LOCK_IRQ_CHK(acb);
			} else {
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);
				acb->fw_flag = FW_NORMAL;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				rtn = SUCCESS;
			}
			break;
		}
		case ACB_ADAPTER_TYPE_D: {
			if (acb->acb_flags & ACB_F_BUS_RESET) {
				long timeout;
				printk(KERN_NOTICE "arcmsr: there is an bus reset eh proceeding.......\n");
				timeout = wait_event_timeout(wait_q, (acb->acb_flags & ACB_F_BUS_RESET) == 0, 220 * HZ);
				if (timeout)
					return SUCCESS;
			}
			acb->acb_flags |= ACB_F_BUS_RESET;
			if (!arcmsr_iop_reset(acb)) {
				struct MessageUnit_D __iomem *reg;
				reg = acb->pmuD;
				SPIN_UNLOCK_IRQ_CHK(acb);
				arcmsr_hardware_reset(acb);
				acb->acb_flags &= ~ACB_F_IOP_INITED;
			nap:
				ssleep(ARCMSR_SLEEPTIME);
				if ((readl(reg->sample_at_reset) & 0x80) != 0) {
					printk(KERN_ERR "arcmsr%d: waiting for hw bus reset return, retry=%d\n", acb->host->host_no, retry_count);
					if (retry_count > ARCMSR_RETRYCOUNT) {
						acb->fw_flag = FW_DEADLOCK;
						printk(KERN_NOTICE "arcmsr%d: waiting for hw bus reset return, RETRY TERMINATED!!\n", acb->host->host_no);
						SPIN_LOCK_IRQ_CHK(acb);
						return FAILED;
					}
					retry_count++;
					goto nap;
				}
				acb->acb_flags |= ACB_F_IOP_INITED;
				/* disable all outbound interrupt */
				intmask_org = arcmsr_disable_outbound_ints(acb);
				arcmsr_get_firmware_spec(acb);
				arcmsr_start_adapter_bgrb(acb);
				arcmsr_clear_doorbell_queue_buffer(acb)/* clear Qbuffer if door bell ringed */;
				/* enable outbound Post Queue,outbound doorbell Interrupt */
				arcmsr_enable_outbound_ints(acb, intmask_org);
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);					
				acb->fw_flag = FW_NORMAL;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				rtn = SUCCESS;					
				printk(KERN_NOTICE "arcmsr: scsi bus reset eh returns with success\n");
				SPIN_LOCK_IRQ_CHK(acb);
			} else {
				acb->acb_flags &= ~ACB_F_BUS_RESET;
				atomic_set(&acb->rq_map_token, 16);
				atomic_set(&acb->ante_token_value, 16);
				acb->fw_flag = FW_NORMAL;
				mod_timer(&acb->eternal_timer, jiffies + msecs_to_jiffies(6 * HZ));
				rtn = SUCCESS;
			}
			break;
		}
		break;
	}
	return rtn;
}

const char *arcmsr_info(struct Scsi_Host *host)
{
	char *type;
	int raid6 = 1;
	struct AdapterControlBlock *acb = (struct AdapterControlBlock *) host->hostdata;
	static char buf[256];

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
		case PCI_DEVICE_ID_ARECA_1214:
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
			type = "unknown";
			raid6 =	0;
			break;
	}
	sprintf(buf, "Areca %s RAID Controller %s\narcmsr "
		"version %s\n", type, raid6 ? "(RAID6 capable)" : "",
		ARCMSR_DRIVER_VERSION);
	return buf;
}

static int arcmsr_alloc_ccb_pool(struct AdapterControlBlock *acb)
{
	struct pci_dev *pdev = acb->pdev;
	void *dma_coherent;
	dma_addr_t dma_coherent_handle;
	struct CommandControlBlock *ccb_tmp = NULL;
	int i = 0, j = 0;
	dma_addr_t cdb_phyaddr;
	unsigned long roundup_ccbsize = 0;
	unsigned long max_xfer_len;
	unsigned long max_sg_entrys;
	uint32_t  firm_config_version;

	max_xfer_len = ARCMSR_MAX_XFER_LEN;
	max_sg_entrys = ARCMSR_DEFAULT_SG_ENTRIES;
	firm_config_version = acb->firm_cfg_version;
	if ((firm_config_version & 0xFF) >= 3) {
		max_xfer_len = (ARCMSR_CDB_SG_PAGE_LENGTH << ((firm_config_version >> 8) & 0xFF)) * 1024;
		max_sg_entrys = (max_xfer_len / 4096);
	}
	acb->host->max_sectors = max_xfer_len / 512;
	acb->host->sg_tablesize = max_sg_entrys;
	switch (acb->adapter_type) {
	case ACB_ADAPTER_TYPE_A:
		roundup_ccbsize = roundup(sizeof(struct CommandControlBlock) +
			max_sg_entrys * sizeof(struct SG64ENTRY), 32);
		break;
	case ACB_ADAPTER_TYPE_B:
	case ACB_ADAPTER_TYPE_C:
	case ACB_ADAPTER_TYPE_D:
		roundup_ccbsize = roundup(sizeof(struct CommandControlBlock) +
			(max_sg_entrys - 1) * sizeof(struct SG64ENTRY), 32);
		break;
	}
	acb->uncache_size = roundup_ccbsize * ARCMSR_MAX_FREECCB_NUM;
	dma_coherent = dma_alloc_coherent(&pdev->dev, acb->uncache_size,
		&dma_coherent_handle, GFP_KERNEL);
	if (!dma_coherent) {
		printk("arcmsr%d: dma_alloc_coherent got error\n", acb->host->host_no);
		return -ENOMEM;
	}
	memset(dma_coherent, 0, acb->uncache_size);
	acb->dma_coherent = dma_coherent;
	acb->dma_coherent_handle = dma_coherent_handle;
	ccb_tmp = (struct CommandControlBlock *)dma_coherent;
	acb->vir2phy_offset = (unsigned long)dma_coherent - (unsigned long)dma_coherent_handle;
	for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {
		cdb_phyaddr = dma_coherent_handle + offsetof(struct CommandControlBlock, arcmsr_cdb);
		switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_B:
			ccb_tmp->cdb_phyaddr = cdb_phyaddr >> 5;
			break;
		case ACB_ADAPTER_TYPE_C:
		case ACB_ADAPTER_TYPE_D:
			ccb_tmp->cdb_phyaddr = cdb_phyaddr;
			break;
		}
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
	return 0;
}

static void arcmsr_free_ccb_pool(struct AdapterControlBlock *acb)
{
	switch (acb->adapter_type) {
		case ACB_ADAPTER_TYPE_A:
		case ACB_ADAPTER_TYPE_B:
		case ACB_ADAPTER_TYPE_C:
		case ACB_ADAPTER_TYPE_D:
			dma_free_coherent(&acb->pdev->dev, acb->uncache_size,
				acb->dma_coherent, acb->dma_coherent_handle);
			break;
	}
}

static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb)
{
	int poll_count = 0, i;
	struct pci_dev *pdev;
	struct Scsi_Host *host;

	/* disable iop all outbound interrupt */
	flush_scheduled_work();
	del_timer_sync(&acb->eternal_timer);
	arcmsr_disable_outbound_ints(acb);
	arcmsr_stop_adapter_bgrb(acb);
	arcmsr_flush_adapter_cache(acb);
	acb->acb_flags &= ~ACB_F_IOP_INITED;
	for (poll_count = 0; poll_count < ARCMSR_MAX_OUTSTANDING_CMD; poll_count++) {
		if (!atomic_read(&acb->ccboutstandingcount))
			break;
		arcmsr_interrupt(acb);
		msleep(25);
	}
	if (atomic_read(&acb->ccboutstandingcount)) {
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
	arcmsr_free_ccb_pool(acb);
	arcmsr_free_mu(acb);
	scsi_remove_host(host);
	scsi_host_put(host);
	if (acb->acb_flags & ACB_F_MSI_ENABLED) {
		free_irq(pdev->irq, acb);
		pci_disable_msi(pdev);
	} else if (acb->acb_flags & ACB_F_MSIX_ENABLED) {
		for (i = 0; i < ARCMST_NUM_MSIX_VECTORS; i++) {
			free_irq(acb->entries[i].vector, acb);
		}
		pci_disable_msix(pdev);
	} else {
		free_irq(pdev->irq, acb);
	}
	arcmsr_unmap_pciregion(acb);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 8)
/*
*******************************************************************************
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
*******************************************************************************
*/
static int arcmsr_set_info(char *buffer, int length, struct Scsi_Host *host)
{
	char *p;
	int iTmp;
	unsigned long flags;
	struct scsi_device *pstSDev, *pstTmp;

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
	return length;
}

int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
{
	char *pos = buffer;
	
   	if (inout) {
		return(arcmsr_set_info(buffer, length, host));
	}
	*start = buffer + offset;
	if (pos - buffer < offset) {
		return 0;
	} else if (pos - buffer - offset < length) {
		return (pos - buffer - offset);
	} else {
		return length;
	}
}
#endif

int arcmsr_release(struct Scsi_Host *host)
{
	struct AdapterControlBlock *acb;
	if (!host)
		return -ENXIO;
	acb = (struct AdapterControlBlock *)host->hostdata;
	if (!acb)
		return -ENXIO;
	arcmsr_pcidev_disattach(acb);
	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 31)
static int
arcmsr_set_sdev_queue_depth(const char *val, struct kernel_param *kp)
{
	struct AdapterControlBlock *pacb;
	struct scsi_device 	*sdev;
	int ret = param_set_int(val, kp);

	if (ret)
		return ret;
	list_for_each_entry(pacb, &rc_list, list) {
		shost_for_each_device(sdev, pacb->host)
		arcmsr_adjust_disk_queue_depth(sdev, sdev_queue_depth, SCSI_QDEPTH_DEFAULT);
	}
	return 0;
}
#endif

