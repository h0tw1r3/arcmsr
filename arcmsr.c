/*
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
******************************************************************************************
*/
#define ARCMSR_DEBUG                      0
/************************************/
#if defined __KERNEL__
	#include <linux/config.h>
	#if defined( CONFIG_MODVERSIONS ) && ! defined( MODVERSIONS )
	    #define MODVERSIONS
	#endif
	/* modversions.h should be before should be before module.h */
	#if defined( MODVERSIONS )
	    #include <config/modversions.h>
	#endif
	#include <linux/module.h>
	#include <linux/version.h>
	/* Now your module include files & source code follows */
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
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,5,0)
		#include <linux/moduleparam.h>
		#include <linux/blkdev.h>
	#else
		#include <linux/blk.h>
	#endif
	#include <linux/timer.h>
	#include <linux/devfs_fs_kernel.h>
    #include <linux/reboot.h>
    #include <linux/notifier.h>
	#include <linux/sched.h>
	#include <linux/init.h>

	# if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
		# include <linux/spinlock.h>
	# else
		# include <asm/spinlock.h>
	# endif /* 2,3,30 */

	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,5,0)
        #include <scsi/scsi.h>
        #include <scsi/scsi_host.h>
		#include <scsi/scsi_cmnd.h>
		#include <scsi/scsi_tcq.h>
		#include <scsi/scsi_device.h>
	#else
        #include "/usr/src/linux/drivers/scsi/scsi.h"
		#include "/usr/src/linux/drivers/scsi/hosts.h"
		#include "/usr/src/linux/drivers/scsi/constants.h"
		#include "/usr/src/linux/drivers/scsi/sd.h"
	#endif
	#include "arcmsr.h"
#endif

MODULE_AUTHOR("Erich Chen <erich@areca.com.tw>");
MODULE_DESCRIPTION("ARECA (ARC11xx/12xx) SATA RAID HOST Adapter");

#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

/*
**********************************************************************************
**********************************************************************************
*/
static u_int8_t arcmsr_adapterCnt=0;
static struct _HCBARC arcmsr_host_control_block;
/*
**********************************************************************************
** notifier block to get a notify on system shutdown/halt/reboot
**********************************************************************************
*/
static int arcmsr_fops_ioctl(struct inode *inode, struct file *filep, unsigned int ioctl_cmd, unsigned long arg);
static int arcmsr_fops_close(struct inode *inode, struct file *filep);
static int arcmsr_fops_open(struct inode *inode, struct file *filep);
static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf);
static int arcmsr_initialize(struct _ACB *pACB,struct pci_dev *pPCI_DEV);
static int arcmsr_iop_ioctlcmd(struct _ACB *pACB,int ioctl_cmd,void *arg);
static void arcmsr_free_pci_pool(struct _ACB *pACB);
static void arcmsr_pcidev_disattach(struct _ACB *pACB);
static void arcmsr_iop_init(struct _ACB *pACB);
static u_int8_t arcmsr_wait_msgint_ready(struct _ACB *pACB);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	#define arcmsr_detect NULL
    static irqreturn_t arcmsr_interrupt(struct _ACB *pACB);
    static int __devinit arcmsr_device_probe(struct pci_dev *pPCI_DEV,const struct pci_device_id *id);
	static void arcmsr_device_remove(struct pci_dev *pPCI_DEV);
#else
    static void arcmsr_interrupt(struct _ACB *pACB);
    int arcmsr_schedule_command(struct scsi_cmnd *pcmd);
    int arcmsr_detect(Scsi_Host_Template *);
#endif
/*
**********************************************************************************
**********************************************************************************
*/
static struct notifier_block arcmsr_event_notifier={arcmsr_halt_notify,NULL,0};
static struct file_operations arcmsr_file_operations = 
{
	  ioctl:       arcmsr_fops_ioctl,
	   open:        arcmsr_fops_open,
	release:       arcmsr_fops_close
};
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
		S_IFDIR | S_IRUGO | S_IXUGO,
		2
	};
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	/* We do our own ID filtering.  So, grab all SCSI storage class devices. */
	static struct pci_device_id arcmsr_device_id_table[] = 
	{
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1110, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1120, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1130, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1160, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1170, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1210, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1220, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1230, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1260, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1270, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{0, 0},	/* Terminating entry */
	};
	MODULE_DEVICE_TABLE(pci, arcmsr_device_id_table);
	struct pci_driver arcmsr_pci_driver = 
	{
		.name		= "arcmsr",
		.id_table	= arcmsr_device_id_table,
		.probe		= arcmsr_device_probe,
		.remove		= arcmsr_device_remove,
	};
	/*
	*********************************************************************
	*********************************************************************
	*/
    static irqreturn_t arcmsr_do_interrupt(int irq,void *dev_id,struct pt_regs *regs)
	{
		irqreturn_t handle_state;
	    struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
	    struct _ACB *pACB;
	    struct _ACB *pACBtmp;
		int i=0;

		#if ARCMSR_DEBUG
		printk("arcmsr_do_interrupt.................. \n");
		#endif

        pACB=(struct _ACB *)dev_id;
		pACBtmp=pHCBARC->pACB[i];
 		while((pACB != pACBtmp) && pACBtmp && (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			pACBtmp=pHCBARC->pACB[i];
		}
		if(!pACBtmp)
		{
		    #if ARCMSR_DEBUG
			printk("arcmsr_do_interrupt: Invalid pACB=0x%p \n",pACB);
            #endif
			return IRQ_NONE;
		}
		spin_lock_irq(&pACB->isr_lockunlock);
        handle_state=arcmsr_interrupt(pACB);
		spin_unlock_irq(&pACB->isr_lockunlock);
		return(handle_state);
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
    int arcmsr_bios_param(struct scsi_device *sdev, struct block_device *bdev,sector_t capacity, int *geom)
	{
		int heads,sectors,cylinders,total_capacity;

		#if ARCMSR_DEBUG
		printk("arcmsr_bios_param.................. \n");
		#endif
		total_capacity=capacity;
		heads=64;
		sectors=32;
		cylinders=total_capacity / (heads * sectors);
		if(cylinders > 1024)
		{
			heads=255;
			sectors=63;
			cylinders=total_capacity / (heads * sectors);
		}
		geom[0]=heads;
		geom[1]=sectors;
		geom[2]=cylinders;
		return (0);
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int __devinit arcmsr_device_probe(struct pci_dev *pPCI_DEV,const struct pci_device_id *id)
	{
		struct Scsi_Host *host;
		struct _ACB *pACB;
		struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
        uint8_t bus,dev_fun;
		#if ARCMSR_DEBUG
		printk("arcmsr_device_probe............................\n");
		#endif
		if(pci_enable_device(pPCI_DEV))
		{
			printk("arcmsr%d adapter probe: pci_enable_device error \n",arcmsr_adapterCnt);
			return -ENODEV;
		}
		/* allocate scsi host information (includes out adapter) scsi_host_alloc==scsi_register */
		if((host=scsi_host_alloc(&arcmsr_scsi_host_template,sizeof(struct _ACB)))==0)
		{
			printk("arcmsr%d adapter probe: scsi_host_alloc error \n",arcmsr_adapterCnt);
            return -ENODEV;
		}
		if(!pci_set_dma_mask(pPCI_DEV, DMA_64BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else if(!pci_set_dma_mask(pPCI_DEV, DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 32BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else 
		{
			printk("ARECA RAID ADAPTER%d: No suitable DMA available.\n",arcmsr_adapterCnt);
			return -ENOMEM;
		}
		if (pci_set_consistent_dma_mask(pPCI_DEV, DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: No 32BIT coherent DMA adressing available.\n",arcmsr_adapterCnt);
			return -ENOMEM;
		}
		bus = pPCI_DEV->bus->number;
	    dev_fun = pPCI_DEV->devfn;
		pACB=(struct _ACB *) host->hostdata;
		memset(pACB,0,sizeof(struct _ACB));
		spin_lock_init(&pACB->isr_lockunlock);
		spin_lock_init(&pACB->wait2go_lockunlock);
		spin_lock_init(&pACB->qbuffer_lockunlock);
		spin_lock_init(&pACB->ccb_doneindex_lockunlock);
		spin_lock_init(&pACB->ccb_startindex_lockunlock);
		pACB->pPCI_DEV=pPCI_DEV;
		pACB->host=host;
		host->max_sectors=ARCMSR_MAX_XFER_SECTORS;
		host->max_lun=ARCMSR_MAX_TARGETLUN;
		host->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
		host->max_cmd_len=16;    /*this is issue of 64bit LBA ,over 2T byte*/
		host->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
		host->can_queue=ARCMSR_MAX_OUTSTANDING_CMD; /* max simultaneous cmds */             
		host->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;            
		host->this_id=ARCMSR_SCSI_INITIATOR_ID; 
		host->unique_id=(bus << 8) | dev_fun;
		host->io_port=0;
		host->n_io_port=0;
		host->irq=pPCI_DEV->irq;
		pci_set_master(pPCI_DEV);
 		if(arcmsr_initialize(pACB,pPCI_DEV))
		{
			printk("arcmsr%d initialize got error \n",arcmsr_adapterCnt);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->pACB[arcmsr_adapterCnt]=NULL;
			scsi_host_put(host);
			return -ENODEV;
		}
	    if (pci_request_regions(pPCI_DEV, "arcmsr"))
		{
      		printk("arcmsr%d adapter probe: pci_request_regions failed \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(host);
  			return -ENODEV;
		}
		if(request_irq(pPCI_DEV->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",pACB))
		{
  			printk("arcmsr%d request IRQ=%d failed !\n",arcmsr_adapterCnt--,pPCI_DEV->irq);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(host);
  			return -ENODEV;
		}
		arcmsr_iop_init(pACB);
		if(scsi_add_host(host, &pPCI_DEV->dev))
		{
            printk("arcmsr%d scsi_add_host got error \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(host);
			return -ENODEV;
		}
 		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		pci_set_drvdata(pPCI_DEV, host);
		scsi_scan_host(host);
 		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_device_remove(struct pci_dev *pPCI_DEV)
	{
	    struct Scsi_Host *host=pci_get_drvdata(pPCI_DEV);
		struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
	    struct _ACB *pACB=(struct _ACB *) host->hostdata;
		int i;

		#if ARCMSR_DEBUG
		printk("arcmsr_device_remove............................\n");
		#endif
  		/* Flush cache to disk */
		/* Free irq,otherwise extra interrupt is generated	 */
		/* Issue a blocking(interrupts disabled) command to the card */
        arcmsr_pcidev_disattach(pACB);
        scsi_remove_host(host);
	    scsi_host_put(host);
	    pci_set_drvdata(pPCI_DEV, NULL);
		/*if this is last pACB */
		for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			if(pHCBARC->pACB[i]!=NULL)
			{ 
				return;/* this is not last adapter's release */
			}
		}
		unregister_chrdev(pHCBARC->arcmsr_major_number, "arcmsr");
	    unregister_reboot_notifier(&arcmsr_event_notifier);
		return;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_scsi_host_template_init(struct scsi_host_template * host_template)
	{
		int	error;
		struct _HCBARC *pHCBARC= &arcmsr_host_control_block;

		#if ARCMSR_DEBUG
		printk("arcmsr_scsi_host_template_init..............\n");
		#endif
		/* 
		** register as a PCI hot-plug driver module 
		*/
		memset(pHCBARC,0,sizeof(struct _HCBARC));
		error=pci_module_init(&arcmsr_pci_driver);
 		if(pHCBARC->pACB[0]!=NULL)
		{
  			host_template->proc_name="arcmsr";
			register_reboot_notifier(&arcmsr_event_notifier);
 			pHCBARC->arcmsr_major_number=register_chrdev(0, "arcmsr", &arcmsr_file_operations);
			printk("arcmsr device major number %d \n",pHCBARC->arcmsr_major_number);
		}
		return(error);
  	}
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
		return;
	}
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
		return;
	}
	/*
	***************************************************************
	*	             arcmsr_schedule_command
	*	Description:	Process a command from the SCSI manager(A.P)
	*	Parameters:		cmd - Pointer to SCSI command structure.
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
		struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
		struct _ACB *pACB;
		struct _ACB *pACBtmp;
		int i=0;

		#if ARCMSR_DEBUG
		printk("arcmsr_do_interrupt.................. \n");
		#endif
		pACB=(struct _ACB *)dev_id;
		pACBtmp=pHCBARC->pACB[i];
 		while((pACB != pACBtmp) && pACBtmp && (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			pACBtmp=pHCBARC->pACB[i];
		}
		if(!pACBtmp)
		{
		    #if ARCMSR_DEBUG
			printk("arcmsr_do_interrupt: Invalid pACB=0x%p \n",pACB);
            #endif
			return;
		}
		spin_lock_irq(&pACB->isr_lockunlock);
        arcmsr_interrupt(pACB);
		spin_unlock_irq(&pACB->isr_lockunlock);
		return;
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
		if(cylinders > 1024)
		{
			heads=255;
			sectors=63;
			cylinders=total_capacity / (heads * sectors);
		}
		geom[0]=heads;
		geom[1]=sectors;
		geom[2]=cylinders;
		return (0);
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
		} const arcmsr_devices[]={
	  		{ PCIVendorIDARECA,PCIDeviceIDARC1110 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1120 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1130 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1160 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1170 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1210 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1220 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1230 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1260 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1270 }
		};
		struct pci_dev *pPCI_DEV=NULL;
		struct _ACB *pACB;
		struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
		struct Scsi_Host *host;
		static u_int8_t i;
		#if ARCMSR_DEBUG
		printk("arcmsr_detect............................\n");
		#endif
		memset(pHCBARC,0,sizeof(struct _HCBARC));
  		for(i=0; i < (sizeof(arcmsr_devices)/sizeof(arcmsr_devices[0])) ; ++i)   
		{
			pPCI_DEV=NULL;
			while((pPCI_DEV=pci_find_device(arcmsr_devices[i].vendor_id,arcmsr_devices[i].device_id,pPCI_DEV)))
			{
				if((host=scsi_register(host_template,sizeof(struct _ACB)))==0)
				{
					printk("arcmsr_detect: scsi_register error . . . . . . . . . . .\n");
					continue;
				}
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
				{
					if(pci_enable_device(pPCI_DEV))
					{
						printk("arcmsr_detect: pci_enable_device ERROR..................................\n");
						scsi_unregister(host);
      					continue;
					}
 					if(!pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0xffffffffffffffffULL))/*64bit*/
					{
						printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
					}
					else if(pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0x00000000ffffffffULL))/*32bit*/
					{
						printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED (ERROR)\n");
						scsi_unregister(host);
      					continue;
					}
				}
			#endif
				pACB=(struct _ACB *) host->hostdata;
				memset(pACB,0,sizeof(struct _ACB));
                spin_lock_init(&pACB->isr_lockunlock);
				spin_lock_init(&pACB->wait2go_lockunlock);
		    	spin_lock_init(&pACB->qbuffer_lockunlock);
				spin_lock_init(&pACB->ccb_doneindex_lockunlock);
				spin_lock_init(&pACB->ccb_startindex_lockunlock);
				pACB->pPCI_DEV=pPCI_DEV;
				pACB->host=host;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,7)
				host->max_sectors=ARCMSR_MAX_XFER_SECTORS;
			#endif
				host->max_lun=ARCMSR_MAX_TARGETLUN;
				host->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
                host->max_cmd_len=16;            /*this is issue of 64bit LBA ,over 2T byte*/
            #endif
				host->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
				host->can_queue=ARCMSR_MAX_OUTSTANDING_CMD; /* max simultaneous cmds */             
				host->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;            
				host->this_id=ARCMSR_SCSI_INITIATOR_ID; 
				host->io_port=0;
				host->n_io_port=0;
				host->irq=pPCI_DEV->irq;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,4)
				scsi_set_pci_device(host,pPCI_DEV);
			#endif
				if(!arcmsr_initialize(pACB,pPCI_DEV))
				{
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					pci_set_drvdata(pPCI_DEV,pACB); /*set driver_data*/
				#endif
					pci_set_master(pPCI_DEV);
					if(request_irq(pPCI_DEV->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",pACB))
					{
                        printk("arcmsr_detect:  request_irq got ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->pACB[pACB->adapter_index]=NULL;
	   					iounmap(pACB->pmu);
        				arcmsr_free_pci_pool(pACB);
   					    scsi_unregister(host);
						goto next_areca;
					}
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					if (pci_request_regions(pPCI_DEV, "arcmsr"))
					{
                        printk("arcmsr_detect:  pci_request_regions got ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->pACB[pACB->adapter_index]=NULL;
 						iounmap(pACB->pmu);
        				arcmsr_free_pci_pool(pACB);
   					    scsi_unregister(host);
						goto next_areca;
					}
                #endif
					arcmsr_iop_init(pACB);/* on kernel 2.4.21 driver's iop read/write must after request_irq */
				}
				else
				{
					printk("arcmsr: arcmsr_initialize got ERROR...................\n");
					scsi_unregister(host);
				}
        next_areca: ;
			}
		}
		if(arcmsr_adapterCnt)
		{
			#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
  			host_template->proc_name="arcmsr";
			#else		  
  			host_template->proc_dir= &arcmsr_proc_scsi;
			#endif
			register_reboot_notifier(&arcmsr_event_notifier);
		}
		else
		{
			printk("arcmsr_detect:...............NO ARECA RAID ADAPTER FOUND...........\n");
			return(arcmsr_adapterCnt);
		}
		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		pHCBARC->arcmsr_major_number=register_chrdev(0, "arcmsr", &arcmsr_file_operations);
		printk("arcmsr device major number %d \n",pHCBARC->arcmsr_major_number);
		return(arcmsr_adapterCnt);
	}
#endif 
/*
**********************************************************************
**********************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	void arcmsr_pci_unmap_dma(struct _CCB *pCCB)
	{
		struct _ACB *pACB=pCCB->pACB;
		struct scsi_cmnd *pcmd=pCCB->pcmd;

		if(pcmd->use_sg != 0) 
		{
			struct scatterlist *sl;

			sl = (struct scatterlist *)pcmd->request_buffer;
			pci_unmap_sg(pACB->pPCI_DEV, sl, pcmd->use_sg, pcmd->sc_data_direction);
		} 
		else if(pcmd->request_bufflen != 0)
		{
			pci_unmap_single(pACB->pPCI_DEV,(dma_addr_t)(unsigned long)pcmd->SCp.ptr,pcmd->request_bufflen, pcmd->sc_data_direction);
		}
		return;
	}
#endif
/*
**********************************************************************************
**********************************************************************************
*/
static int arcmsr_fops_open(struct inode *inode, struct file *filep)
{
	int i,minor;
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;

	minor = MINOR(inode->i_rdev);
	if(minor >= pHCBARC->adapterCnt) 
	{
		return -ENXIO;
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if((pACB=pHCBARC->pACB[i])!=NULL)
		{ 
			if(pACB->adapter_index==minor)
			{
				break;
			}
		}
	}
	if(i>=ARCMSR_MAX_ADAPTER)
	{
		return -ENXIO;
	}
	return 0;		/* success */
}
/*
**********************************************************************************
**********************************************************************************
*/
static int arcmsr_fops_close(struct inode *inode, struct file *filep)
{
	int i,minor;
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;

	minor = MINOR(inode->i_rdev);
	if(minor >= pHCBARC->adapterCnt) 
	{
		return -ENXIO;
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if((pACB=pHCBARC->pACB[i])!=NULL)
		{ 
			if(pACB->adapter_index==minor)
			{
				break;
			}
		}
	}
	if(i>=ARCMSR_MAX_ADAPTER)
	{
		return -ENXIO;
	}
	return 0;
}
/*
**********************************************************************************
**********************************************************************************
*/
static int arcmsr_fops_ioctl(struct inode *inode, struct file *filep, unsigned int ioctl_cmd, unsigned long arg)
{
	int i,minor;
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;

	minor = MINOR(inode->i_rdev);
	if(minor >= pHCBARC->adapterCnt) 
	{
		return -ENXIO;
	}
  	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if((pACB=pHCBARC->pACB[i])!=NULL)
		{ 
			if(pACB->adapter_index==minor)
			{
				break;
			}
		}
	}
	if(i>=ARCMSR_MAX_ADAPTER)
	{
		return -ENXIO;
	}
	/*
	************************************************************
	** We do not allow muti ioctls to the driver at the same duration.
	************************************************************
	*/
	return arcmsr_iop_ioctlcmd(pACB,ioctl_cmd,(void *)arg);
}
/*
************************************************************************
************************************************************************
*/
void arcmsr_flush_adapter_cache(struct _ACB *pACB)
{
    #if ARCMSR_DEBUG
    printk("arcmsr_flush_adapter_cache..............\n");
    #endif
	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE,&pACB->pmu->inbound_msgaddr0);
	return;
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_ccb_complete(struct _CCB *pCCB)
{
	unsigned long flag;
	struct _ACB *pACB=pCCB->pACB;
    struct scsi_cmnd *pcmd=pCCB->pcmd;

	#if ARCMSR_DEBUG
	printk("arcmsr_ccb_complete:pCCB=0x%p ccb_doneindex=0x%x ccb_startindex=0x%x\n",pCCB,pACB->ccb_doneindex,pACB->ccb_startindex);
	#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
    arcmsr_pci_unmap_dma(pCCB);
#endif
    spin_lock_irqsave(&pACB->ccb_doneindex_lockunlock,flag);
	atomic_dec(&pACB->ccboutstandingcount);
	pCCB->startdone=ARCMSR_CCB_DONE;
	pCCB->ccb_flags=0;
	pACB->pccbringQ[pACB->ccb_doneindex]=pCCB;
    pACB->ccb_doneindex++;
    pACB->ccb_doneindex %= ARCMSR_MAX_FREECCB_NUM;
    spin_unlock_irqrestore(&pACB->ccb_doneindex_lockunlock,flag);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	{
        pcmd->scsi_done(pcmd);
	}
    #else
	{
		unsigned long flags;
		spin_lock_irqsave(&io_request_lock, flags);
		pcmd->scsi_done(pcmd);
		spin_unlock_irqrestore(&io_request_lock, flags);
	}
	#endif
	return;
}
/*
**********************************************************************
**       if scsi error do auto request sense
**********************************************************************
*/
void arcmsr_report_sense_info(struct _CCB *pCCB)
{
	struct scsi_cmnd *pcmd=pCCB->pcmd;
	struct _SENSE_DATA *psenseBuffer=(struct _SENSE_DATA *)pcmd->sense_buffer;
	#if ARCMSR_DEBUG
    printk("arcmsr_report_sense_info...........\n");
	#endif
    pcmd->result=DID_OK << 16;
    if(psenseBuffer) 
	{
		int sense_data_length=sizeof(struct _SENSE_DATA) < sizeof(pcmd->sense_buffer) ? sizeof(struct _SENSE_DATA) : sizeof(pcmd->sense_buffer);
		memset(psenseBuffer, 0, sizeof(pcmd->sense_buffer));
		memcpy(psenseBuffer,pCCB->arcmsr_cdb.SenseData,sense_data_length);
	    psenseBuffer->ErrorCode=0x70;
        psenseBuffer->Valid=1;
    }
    return;
}
/*
*********************************************************************
** to insert pCCB into tail of pACB wait exec ccbQ 
*********************************************************************
*/
void arcmsr_queue_wait2go_ccb(struct _ACB *pACB,struct _CCB *pCCB)
{
    unsigned long flag;
	int i=0;
    #if ARCMSR_DEBUG
	printk("arcmsr_qtail_wait2go_ccb:......................................... \n");
    #endif

	spin_lock_irqsave(&pACB->wait2go_lockunlock,flag);
	while(1)
	{
		if(pACB->pccbwait2go[i]==NULL)
		{
			pACB->pccbwait2go[i]=pCCB;
        	atomic_inc(&pACB->ccbwait2gocount);
            spin_unlock_irqrestore(&pACB->wait2go_lockunlock,flag);
			return;
		}
		i++;
		i%=ARCMSR_MAX_OUTSTANDING_CMD;
	}
	return;
}
/*
*********************************************************************
*********************************************************************
*/
void arcmsr_abort_allcmd(struct _ACB *pACB)
{
	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD,&pACB->pmu->inbound_msgaddr0);
	return;
}
/*
**********************************************************************
**********************************************************************
*/
static u_int8_t arcmsr_wait_msgint_ready(struct _ACB *pACB)
{
	uint32_t Index;
	uint8_t Retries=0x00;

	#if ARCMSR_DEBUG
	printk("arcmsr_wait_msgint_ready: ...............................\n");
	#endif
	do
	{
		for(Index=0; Index < 100; Index++)
		{
			if(readl(&pACB->pmu->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT)
			{
				writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT,&pACB->pmu->outbound_intstatus);/*clear interrupt*/
				return 0x00;
			}
			arc_mdelay_int(10);
		}/*max 1 seconds*/
	}while(Retries++ < 20);/*max 20 sec*/
	return 0xff;
}
/*
****************************************************************************
** Routine Description: Reset 80331 iop.
**           Arguments: 
**        Return Value: Nothing.
****************************************************************************
*/
static void arcmsr_iop_reset(struct _ACB *pACB)
{
	struct _CCB *pCCB;
	uint32_t intmask_org,mask;
    int i=0;

	#if ARCMSR_DEBUG
	printk("arcmsr_iop_reset: reset iop controller......................................\n");
	#endif
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{
		#if ARCMSR_DEBUG
		printk("arcmsr_iop_reset: ccboutstandingcount=%d ...\n",atomic_read(&pACB->ccboutstandingcount));
		#endif
        /* disable all outbound interrupt */
		intmask_org=readl(&pACB->pmu->outbound_intmask);
        writel(intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE,&pACB->pmu->outbound_intmask);
        /* talk to iop 331 outstanding command aborted*/
		arcmsr_abort_allcmd(pACB);
		if(arcmsr_wait_msgint_ready(pACB))
		{
             printk("arcmsr%d: iop reset wait 'abort all outstanding command' timeout \n",pACB->adapter_index);
		}
		/*clear all outbound posted Q*/
		for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
		{
			readl(&pACB->pmu->outbound_queueport);
		}
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
            pCCB=pACB->pccb_pool[i];
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				pCCB->startdone=ARCMSR_CCB_ABORTED;
				pCCB->pcmd->result=DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
			}
		}
		/* enable all outbound interrupt */
		mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
        writel(intmask_org & mask,&pACB->pmu->outbound_intmask);
		/* post abort all outstanding command message to RAID controller */
	}
	i=0;
	while(atomic_read(&pACB->ccbwait2gocount)!=0)
	{
		pCCB=pACB->pccbwait2go[i];
		if(pCCB!=NULL)
		{
			printk("arcmsr%d:iop reset abort command ccbwait2gocount=%d \n",pACB->adapter_index,atomic_read(&pACB->ccbwait2gocount));
		    pACB->pccbwait2go[i]=NULL;
            pCCB->startdone=ARCMSR_CCB_ABORTED;
            pCCB->pcmd->result=DID_ABORT << 16;
			arcmsr_ccb_complete(pCCB);
  			atomic_dec(&pACB->ccbwait2gocount);
		}
		i++;
		i%=ARCMSR_MAX_OUTSTANDING_CMD;
	}
	atomic_set(&pACB->ccboutstandingcount,0);
	return;
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_build_ccb(struct _ACB *pACB,struct _CCB *pCCB,struct scsi_cmnd *pcmd)
{
    struct _ARCMSR_CDB *pARCMSR_CDB= &pCCB->arcmsr_cdb;
	uint8_t *psge=(uint8_t * )&pARCMSR_CDB->u;
	uint32_t address_lo,address_hi;
	int arccdbsize=0x30;
	
	#if ARCMSR_DEBUG
	printk("arcmsr_build_ccb........................... \n");
	#endif
    pCCB->pcmd=pcmd;
	memset(pARCMSR_CDB,0,sizeof(struct _ARCMSR_CDB));
    pARCMSR_CDB->Bus=0;
    pARCMSR_CDB->TargetID=pcmd->device->id;
    pARCMSR_CDB->LUN=pcmd->device->lun;
    pARCMSR_CDB->Function=1;
	pARCMSR_CDB->CdbLength=(uint8_t)pcmd->cmd_len;
    pARCMSR_CDB->Context=(unsigned long)pARCMSR_CDB;
    memcpy(pARCMSR_CDB->Cdb, pcmd->cmnd, pcmd->cmd_len);
	if(pcmd->use_sg) 
	{
		int length,sgcount,i,cdb_sgcount=0;
		struct scatterlist *sl;

		/* Get Scatter Gather List from scsiport. */
		sl=(struct scatterlist *) pcmd->request_buffer;
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
		sgcount=pci_map_sg(pACB->pPCI_DEV, sl, pcmd->use_sg, pcmd->sc_data_direction);
    #else
        sgcount=pcmd->use_sg;
    #endif
		/* map stor port SG list to our iop SG List.*/
		for(i=0;i<sgcount;i++) 
		{
			/* Get the physical address of the current data pointer */
        #if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
			length=cpu_to_le32(sg_dma_len(sl));
            address_lo=cpu_to_le32(dma_addr_lo32(sg_dma_address(sl)));
			address_hi=cpu_to_le32(dma_addr_hi32(sg_dma_address(sl)));
        #else
            length=cpu_to_le32(sl->length);
			address_lo=cpu_to_le32(virt_to_bus(sl->address));
            address_hi=0;
        #endif
 			if(address_hi==0)
			{
				struct _SG32ENTRY* pdma_sg=(struct _SG32ENTRY*)psge;

				pdma_sg->address=address_lo;
				pdma_sg->length=length;
				psge += sizeof(struct _SG32ENTRY);
				arccdbsize += sizeof(struct _SG32ENTRY);
			}
			else
			{
                struct _SG64ENTRY *pdma_sg=(struct _SG64ENTRY *)psge;
     			#if ARCMSR_DEBUG
				printk("arcmsr_build_ccb: ..........address_hi=0x%x.... \n",address_hi);
				#endif

				pdma_sg->addresshigh=address_hi;
				pdma_sg->address=address_lo;
				pdma_sg->length=length|IS_SG64_ADDR;
                psge +=sizeof(struct _SG64ENTRY);
				arccdbsize +=sizeof(struct _SG64ENTRY);
			}
			sl++;
			cdb_sgcount++;
		}
		pARCMSR_CDB->sgcount=(uint8_t)cdb_sgcount;
		pARCMSR_CDB->DataLength=pcmd->request_bufflen;
		if( arccdbsize > 256)
		{
			pARCMSR_CDB->Flags|=ARCMSR_CDB_FLAG_SGL_BSIZE;
		}
	}
	else if(pcmd->request_bufflen) 
	{
    #if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
        dma_addr_t dma_addr;
		dma_addr=pci_map_single(pACB->pPCI_DEV, pcmd->request_buffer, pcmd->request_bufflen, pcmd->sc_data_direction);
		pcmd->SCp.ptr = (char *)(unsigned long) dma_addr;
        address_lo=cpu_to_le32(dma_addr_lo32(dma_addr));
	    address_hi=cpu_to_le32(dma_addr_hi32(dma_addr));
    #else
        address_lo=cpu_to_le32(virt_to_bus(pcmd->request_buffer));/* Actual requested buffer */
	    address_hi=0;
    #endif
		if(address_hi==0)
		{
			struct _SG32ENTRY* pdma_sg=(struct _SG32ENTRY*)psge;
			pdma_sg->address=address_lo;
            pdma_sg->length=pcmd->request_bufflen;
		}
		else
		{
			struct _SG64ENTRY* pdma_sg=(struct _SG64ENTRY*)psge;
			pdma_sg->addresshigh=address_hi;
			pdma_sg->address=address_lo;
            pdma_sg->length=pcmd->request_bufflen|IS_SG64_ADDR;
		}
		pARCMSR_CDB->sgcount=1;
		pARCMSR_CDB->DataLength=pcmd->request_bufflen;
	}
    if(pcmd->cmnd[0]|WRITE_6 || pcmd->cmnd[0]|WRITE_10)
    {
        pARCMSR_CDB->Flags|=ARCMSR_CDB_FLAG_WRITE;
		pCCB->ccb_flags|=CCB_FLAG_WRITE;
	}
	#if ARCMSR_DEBUG
	printk("arcmsr_build_ccb: pCCB=0x%p cmd=0x%x xferlength=%d arccdbsize=%d sgcount=%d\n",pCCB,pcmd->cmnd[0],pARCMSR_CDB->DataLength,arccdbsize,pARCMSR_CDB->sgcount);
	#endif
    return;
}
/*
**************************************************************************
**	arcmsr_post_ccb - Send a protocol specific ARC send postcard to a AIOC .
**	handle: Handle of registered ARC protocol driver
**	adapter_id: AIOC unique identifier(integer)
**	pPOSTCARD_SEND: Pointer to ARC send postcard
**
**	This routine posts a ARC send postcard to the request post FIFO of a
**	specific ARC adapter.
**************************************************************************
*/ 
static void arcmsr_post_ccb(struct _ACB *pACB,struct _CCB *pCCB)
{
	uint32_t cdb_shifted_phyaddr=pCCB->cdb_shifted_phyaddr;
	struct _ARCMSR_CDB *pARCMSR_CDB=(struct _ARCMSR_CDB *)&pCCB->arcmsr_cdb;

	#if ARCMSR_DEBUG
	printk("arcmsr_post_ccb: pCCB=0x%p cdb_shifted_phyaddr=0x%x pCCB->pACB=0x%p \n",pCCB,cdb_shifted_phyaddr,pCCB->pACB);
	#endif
    atomic_inc(&pACB->ccboutstandingcount);
	pCCB->startdone=ARCMSR_CCB_START;
	if(pARCMSR_CDB->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE)
	{
	    writel(cdb_shifted_phyaddr|ARCMSR_CCBPOST_FLAG_SGL_BSIZE,&pACB->pmu->inbound_queueport);
	}
	else
	{
	    writel(cdb_shifted_phyaddr,&pACB->pmu->inbound_queueport);
	}
	return;
}
/*
**************************************************************************
**************************************************************************
*/
void arcmsr_post_wait2go_ccb(struct _ACB *pACB)
{
	unsigned long flag;
	struct _CCB *pCCB;
	int i=0;
	#if ARCMSR_DEBUG
	printk("arcmsr_post_wait2go_ccb:ccbwait2gocount=%d ccboutstandingcount=%d\n",atomic_read(&pACB->ccbwait2gocount),atomic_read(&pACB->ccboutstandingcount));
	#endif
    spin_lock_irqsave(&pACB->wait2go_lockunlock,flag);
	while((atomic_read(&pACB->ccbwait2gocount) > 0) && (atomic_read(&pACB->ccboutstandingcount) < ARCMSR_MAX_OUTSTANDING_CMD))
	{
		pCCB=pACB->pccbwait2go[i];
		if(pCCB!=NULL)
		{
			pACB->pccbwait2go[i]=NULL;
			arcmsr_post_ccb(pACB,pCCB);
			atomic_dec(&pACB->ccbwait2gocount);
		}
		i++;
		i%=ARCMSR_MAX_OUTSTANDING_CMD;
	}
	spin_unlock_irqrestore(&pACB->wait2go_lockunlock,flag);
	return;
}
/*
**********************************************************************
**   Function: arcmsr_post_Qbuffer
**     Output: 
**********************************************************************
*/
static void arcmsr_post_Qbuffer(struct _ACB *pACB)
{
	uint8_t *  pQbuffer;
	struct _QBUFFER* pwbuffer=(struct _QBUFFER*)&pACB->pmu->ioctl_wbuffer;
    uint8_t *  iop_data=(uint8_t * )pwbuffer->data;
	int32_t allxfer_len=0;

	while((pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex) && (allxfer_len<124))
	{
		pQbuffer= &pACB->wqbuffer[pACB->wqbuf_firstindex];
		memcpy(iop_data,pQbuffer,1);
		pACB->wqbuf_firstindex++;
		pACB->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set it to 0 */
		iop_data++;
		allxfer_len++;
	}
	pwbuffer->data_len=allxfer_len;
	/*
	** push inbound doorbell and wait reply at hwinterrupt routine for next Qbuffer post
	*/
 	writel(ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK,&pACB->pmu->inbound_doorbell);
	return;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_adapter_bgrb(struct _ACB *pACB)
{
    #if ARCMSR_DEBUG
    printk("arcmsr_stop_adapter_bgrb..............\n");
    #endif
	pACB->acb_flags |= ACB_F_MSG_STOP_BGRB;
	pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB,&pACB->pmu->inbound_msgaddr0);
	return;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_free_pci_pool(struct _ACB *pACB)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	{
        dma_free_coherent(&pACB->pPCI_DEV->dev,((sizeof(struct _CCB) * ARCMSR_MAX_FREECCB_NUM)+0x20),pACB->dma_coherent,pACB->dma_coherent_handle);
	}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	{
	    pci_free_consistent(pACB->pPCI_DEV, ((sizeof(struct _CCB) * ARCMSR_MAX_FREECCB_NUM)+0x20), pACB->dma_coherent, pACB->dma_coherent_handle);
	}
#else
	{
        kfree(pACB->dma_coherent);
	}
#endif
	return;
}
/*
**********************************************************************
**   Function:  arcmsr_interrupt
**     Output:  void
** DID_OK          0x00	// NO error                                
** DID_NO_CONNECT  0x01	// Couldn't connect before timeout period  
** DID_BUS_BUSY    0x02	// BUS stayed busy through time out period 
** DID_TIME_OUT    0x03	// TIMED OUT for other reason              
** DID_BAD_TARGET  0x04	// BAD target.                             
** DID_ABORT       0x05	// Told to abort for some other reason     
** DID_PARITY      0x06	// Parity error                            
** DID_ERROR       0x07	// Internal error                          
** DID_RESET       0x08	// Reset by somebody.                      
** DID_BAD_INTR    0x09	// Got an interrupt we weren't expecting.  
** DID_PASSTHROUGH 0x0a	// Force command past mid-layer            
** DID_SOFT_ERROR  0x0b	// The low level driver just wish a retry  
** DRIVER_OK       0x00	// Driver status        
**********************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    static irqreturn_t arcmsr_interrupt(struct _ACB *pACB)
#else
    static void arcmsr_interrupt(struct _ACB *pACB)
#endif
{
	struct _CCB *pCCB;
	uint32_t flag_ccb,outbound_intstatus,outbound_doorbell;
    #if ARCMSR_DEBUG
    printk("arcmsr_interrupt...................................\n");
    #endif

	/*
	*********************************************
	**   check outbound intstatus @K9n&35L6l.t+v*y9a
	*********************************************
	*/
	outbound_intstatus=readl(&pACB->pmu->outbound_intstatus) & pACB->outbound_int_enable;
	writel(outbound_intstatus,&pACB->pmu->outbound_intstatus);/*clear interrupt*/
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_DOORBELL_INT)
	{
		#if ARCMSR_DEBUG
		printk("arcmsr_interrupt:..........ARCMSR_MU_OUTBOUND_DOORBELL_INT \n");
		#endif
		/*
		*********************************************
		**  DOORBELL %m>4! ,O'_&36l%s-nC1&,
		*********************************************
		*/
		outbound_doorbell=readl(&pACB->pmu->outbound_doorbell);
		writel(outbound_doorbell,&pACB->pmu->outbound_doorbell);/*clear interrupt */
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK)
		{
			struct _QBUFFER* prbuffer=(struct _QBUFFER*)&pACB->pmu->ioctl_rbuffer;
			uint8_t *  iop_data=(uint8_t * )prbuffer->data;
			uint8_t *  pQbuffer;
			int32_t my_empty_len,iop_len,rqbuf_firstindex,rqbuf_lastindex;
	
	        /*check this iop data if overflow my rqbuffer*/
			rqbuf_lastindex=pACB->rqbuf_lastindex;
			rqbuf_firstindex=pACB->rqbuf_firstindex;
			iop_len=prbuffer->data_len;
            my_empty_len=(rqbuf_firstindex-rqbuf_lastindex-1)&(ARCMSR_MAX_QBUFFER-1);
			if(my_empty_len>=iop_len)
			{
				while(iop_len > 0)
				{
					pQbuffer= &pACB->rqbuffer[pACB->rqbuf_lastindex];
					memcpy(pQbuffer,iop_data,1);
					pACB->rqbuf_lastindex++;
					pACB->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;/*if last index number set it to 0 */
					iop_data++;
					iop_len--;
				}
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);/*signature, let IOP331 know data has been readed */
			}
			else
			{
				pACB->acb_flags|=ACB_F_IOPDATA_OVERFLOW;
			}
		}
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK)
		{
			/*
			*********************************************
			**           ,],],O'_AY&36l%s-n669D1H%X
			*********************************************
			*/
    		if(pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex)
			{
				uint8_t *  pQbuffer;
				struct _QBUFFER* pwbuffer=(struct _QBUFFER*)&pACB->pmu->ioctl_wbuffer;
				uint8_t *  iop_data=(uint8_t * )pwbuffer->data;
				int32_t allxfer_len=0;

				while((pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex) && (allxfer_len<124))
				{
					pQbuffer= &pACB->wqbuffer[pACB->wqbuf_firstindex];
   					memcpy(iop_data,pQbuffer,1);
					pACB->wqbuf_firstindex++;
					pACB->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set it to 0 */
					iop_data++;
					allxfer_len++;
				}
				pwbuffer->data_len=allxfer_len;
				/*
				** push inbound doorbell tell iop driver data write ok and wait reply on next hwinterrupt for next Qbuffer post
				*/
				writel(ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK,&pACB->pmu->inbound_doorbell);
 			}
			else
			{
				pACB->acb_flags |= ACB_F_IOCTL_WQBUFFER_CLEARED;
			}
		}
	}
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_POSTQUEUE_INT)
	{
		int id,lun;
 		/*
		*****************************************************************************
		**               areca cdb command done
		*****************************************************************************
		*/
		while(1)
		{
			if((flag_ccb=readl(&pACB->pmu->outbound_queueport)) == 0xFFFFFFFF)
			{
				break;/*chip FIFO no ccb for completion already*/
			}
			/* check if command done with no error*/
			pCCB=(struct _CCB *)(pACB->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
			if((pCCB->pACB!=pACB) || (pCCB->startdone!=ARCMSR_CCB_START))
			{
				if(pCCB->startdone==ARCMSR_CCB_ABORTED)
				{
					printk("arcmsr%d scsi id=%d lun=%d ccb='0x%p' isr command abort successfully \n",pACB->adapter_index,pCCB->pcmd->device->id,pCCB->pcmd->device->lun,pCCB);
					pCCB->pcmd->result=DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
					continue;
				}
				printk("arcmsr%d isr get an illegal ccb command done acb='0x%p' ccb='0x%p' ccbacb='0x%p' startdone=0x%x ccboutstandingcount=%d \n",pACB->adapter_index,pACB,pCCB,pCCB->pACB,pCCB->startdone,atomic_read(&pACB->ccboutstandingcount));
				continue;
			}
			id=pCCB->pcmd->device->id;
			lun=pCCB->pcmd->device->lun;
			if((flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)==0)
			{
 				#if ARCMSR_DEBUG
				printk("pCCB=0x%p   scsi cmd=0x%x................... GOOD ..............done\n",pCCB,pCCB->pcmd->cmnd[0]);
				#endif

				if(pACB->devstate[id][lun]==ARECA_RAID_GONE)
				{
                    pACB->devstate[id][lun]=ARECA_RAID_GOOD;
				}
				pCCB->pcmd->result=DID_OK << 16;
				arcmsr_ccb_complete(pCCB);
			} 
			else 
			{   
				switch(pCCB->arcmsr_cdb.DeviceStatus)
				{
				case ARCMSR_DEV_SELECT_TIMEOUT:
					{
				#if ARCMSR_DEBUG
				printk("pCCB=0x%p ......ARCMSR_DEV_SELECT_TIMEOUT\n",pCCB);
				#endif
                        pACB->devstate[id][lun]=ARECA_RAID_GONE;
						pCCB->pcmd->result=DID_TIME_OUT << 16;
						arcmsr_ccb_complete(pCCB);
					}
					break;
				case ARCMSR_DEV_ABORTED:
				case ARCMSR_DEV_INIT_FAIL:
					{
				#if ARCMSR_DEBUG
				printk("pCCB=0x%p .....ARCMSR_DEV_INIT_FAIL\n",pCCB);
				#endif
				        pACB->devstate[id][lun]=ARECA_RAID_GONE;
 						pCCB->pcmd->result=DID_BAD_TARGET << 16;
						arcmsr_ccb_complete(pCCB);
					}
					break;
				case SCSISTAT_CHECK_CONDITION:
					{
				#if ARCMSR_DEBUG
				printk("pCCB=0x%p .....SCSISTAT_CHECK_CONDITION\n",pCCB);
				#endif
				        pACB->devstate[id][lun]=ARECA_RAID_GOOD;
                        arcmsr_report_sense_info(pCCB);
						arcmsr_ccb_complete(pCCB);
					}
					break;
				default:
					/* error occur Q all error ccb to errorccbpending Q*/
 					printk("arcmsr%d scsi id=%d lun=%d isr get command error done, but got unknow DeviceStatus=0x%x \n",pACB->adapter_index,id,lun,pCCB->arcmsr_cdb.DeviceStatus);
					pACB->devstate[id][lun]=ARECA_RAID_GONE;
					pCCB->pcmd->result=DID_BAD_TARGET << 16;/*unknow error or crc error just for retry*/
					arcmsr_ccb_complete(pCCB);
					break;
				}
			}
		}	/*drain reply FIFO*/
	}
	if(!(outbound_intstatus & ARCMSR_MU_OUTBOUND_HANDLE_INT))
	{
		/*it must be share irq*/
		#if ARCMSR_DEBUG
		printk("arcmsr_interrupt..........FALSE....................share irq.....\n");
		#endif
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
            return IRQ_NONE;
		#else
            return;
		#endif
	}
	if(atomic_read(&pACB->ccbwait2gocount) != 0)
	{
    	arcmsr_post_wait2go_ccb(pACB);/*try to post all pending ccb*/
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
static void arcmsr_iop_parking(struct _ACB *pACB)
{
	if(pACB!=NULL)
	{
		/* stop adapter background rebuild */
		if(pACB->acb_flags & ACB_F_MSG_START_BGRB)
		{
			pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
			arcmsr_stop_adapter_bgrb(pACB);
			if(arcmsr_wait_msgint_ready(pACB))
			{
  				printk("arcmsr%d iop parking wait 'stop adapter rebulid' timeout \n",pACB->adapter_index);
			}
			arcmsr_flush_adapter_cache(pACB);
			if(arcmsr_wait_msgint_ready(pACB))
			{
  				printk("arcmsr%d iop parking wait 'flush adapter cache' timeout \n",pACB->adapter_index);
			}
		}
	}
}
/*
***********************************************************************
************************************************************************
*/
static int arcmsr_iop_ioctlcmd(struct _ACB *pACB,int ioctl_cmd,void *arg)
{
	PCMD_IOCTL_FIELD pcmdioctlfld;
	dma_addr_t cmd_handle;
    int retvalue=0;
	/* Only let one of these through at a time */

	#if ARCMSR_DEBUG
	printk("arcmsr_iop_ioctlcmd.......................................\n");
	#endif
	pcmdioctlfld=pci_alloc_consistent(pACB->pPCI_DEV, sizeof (struct _CMD_IOCTL_FIELD), &cmd_handle);
	if(pcmdioctlfld==NULL)
	{
        return -ENOMEM;
	}
	if(copy_from_user(pcmdioctlfld, arg, sizeof (struct _CMD_IOCTL_FIELD))!=0)
	{
        retvalue = -EFAULT;
		goto ioctl_out;
	}
	if(memcmp(pcmdioctlfld->cmdioctl.Signature,"ARCMSR",6)!=0)
    {
        retvalue = -EINVAL;
		goto ioctl_out;
	}
	switch(ioctl_cmd)
	{
	case ARCMSR_IOCTL_READ_RQBUFFER:
		{
			unsigned long flag;
			unsigned long *ver_addr;
			dma_addr_t buf_handle;
			uint8_t *pQbuffer,*ptmpQbuffer;
			int32_t allxfer_len=0;

            #if ARCMSR_DEBUG
            printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_READ_RQBUFFER..... \n");
            #endif
			ver_addr=pci_alloc_consistent(pACB->pPCI_DEV, 1032, &buf_handle);
			if(ver_addr==NULL)
			{
                retvalue = -ENOMEM;
				goto ioctl_out;
			}
            ptmpQbuffer=(uint8_t *)ver_addr;
			spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
   			while((pACB->rqbuf_firstindex!=pACB->rqbuf_lastindex) && (allxfer_len<1031))
			{
				/*copy READ QBUFFER to srb*/
                pQbuffer= &pACB->rqbuffer[pACB->rqbuf_firstindex];
				memcpy(ptmpQbuffer,pQbuffer,1);
				pACB->rqbuf_firstindex++;
				pACB->rqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set it to 0 */
				ptmpQbuffer++;
				allxfer_len++;
			}
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                struct _QBUFFER* prbuffer=(struct _QBUFFER*)&pACB->pmu->ioctl_rbuffer;
                uint8_t * pQbuffer;
				uint8_t * iop_data=(uint8_t *)prbuffer->data;
                int32_t iop_len;

                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
			    iop_len=(int32_t)prbuffer->data_len;
				/*this iop data does no chance to make me overflow again here, so just do it*/
				while(iop_len>0)
				{
                    pQbuffer= &pACB->rqbuffer[pACB->rqbuf_lastindex];
					memcpy(pQbuffer,iop_data,1);
					pACB->rqbuf_lastindex++;
					pACB->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;/*if last index number set it to 0 */
					iop_data++;
					iop_len--;
				}
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);/*signature, let IOP331 know data has been readed */
			}
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
            memcpy(pcmdioctlfld->ioctldatabuffer,(uint8_t *)ver_addr,allxfer_len);
			pcmdioctlfld->cmdioctl.Length=allxfer_len;
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
            pci_free_consistent(pACB->pPCI_DEV, 1032, ver_addr, buf_handle);
 		}
		break;
	case ARCMSR_IOCTL_WRITE_WQBUFFER:
		{
			unsigned long flag;
			unsigned long *ver_addr;
			dma_addr_t buf_handle;
			int32_t my_empty_len,user_len,wqbuf_firstindex,wqbuf_lastindex;
            uint8_t *pQbuffer,*ptmpuserbuffer;

            #if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_WRITE_WQBUFFER..... \n");
            #endif
		    ver_addr=pci_alloc_consistent(pACB->pPCI_DEV, 1032, &buf_handle);
			if(ver_addr==NULL)
			{
                retvalue= -ENOMEM;
                goto ioctl_out;
			}
            ptmpuserbuffer=(uint8_t *)ver_addr;
            user_len=pcmdioctlfld->cmdioctl.Length;
			memcpy(ptmpuserbuffer,pcmdioctlfld->ioctldatabuffer,user_len);
			/*check if data xfer length of this request will overflow my array qbuffer */
            spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			wqbuf_lastindex=pACB->wqbuf_lastindex;
			wqbuf_firstindex=pACB->wqbuf_firstindex;
			my_empty_len=(wqbuf_firstindex-wqbuf_lastindex-1)&(ARCMSR_MAX_QBUFFER-1);
			if(my_empty_len>=user_len)
			{
				while(user_len>0)
				{
					/*copy srb data to wqbuffer*/
					pQbuffer= &pACB->wqbuffer[pACB->wqbuf_lastindex];
					memcpy(pQbuffer,ptmpuserbuffer,1);
					pACB->wqbuf_lastindex++;
					pACB->wqbuf_lastindex %= ARCMSR_MAX_QBUFFER;/*if last index number set it to 0 */
 					ptmpuserbuffer++;
					user_len--;
				}
				/*post fist Qbuffer*/
				if(pACB->acb_flags & ACB_F_IOCTL_WQBUFFER_CLEARED)
				{
					pACB->acb_flags &=~ACB_F_IOCTL_WQBUFFER_CLEARED;
					arcmsr_post_Qbuffer(pACB);
				}
				pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			}
			else
			{
				#if ARCMSR_DEBUG
				printk("arcmsr_iop_ioctlcmd:invalid data xfer ............qbuffer full............ \n");
                #endif
				pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_ERROR;
			}
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
	    		retvalue= -EFAULT;
			}
            pci_free_consistent(pACB->pPCI_DEV, 1032, ver_addr, buf_handle);
		}
		break;
	case ARCMSR_IOCTL_CLEAR_RQBUFFER:
		{
		    unsigned long flag;
			uint8_t * pQbuffer=pACB->rqbuffer;
            #if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_RQBUFFER..... \n");
            #endif
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);/*signature, let IOP331 know data has been readed */
			}
            pACB->acb_flags |= ACB_F_IOCTL_RQBUFFER_CLEARED;
            spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			pACB->rqbuf_firstindex=0;
			pACB->rqbuf_lastindex=0;
            memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
			/*report success*/
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_CLEAR_WQBUFFER:
		{
		    unsigned long flag;
			uint8_t * pQbuffer=pACB->wqbuffer;
            #if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_WQBUFFER..... \n");
            #endif

			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);/*signature, let IOP331 know data has been readed */
			}
			pACB->acb_flags |= ACB_F_IOCTL_WQBUFFER_CLEARED;
            spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			pACB->wqbuf_firstindex=0;
			pACB->wqbuf_lastindex=0;
            memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
			/*report success*/
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
		    if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_CLEAR_ALLQBUFFER:
		{
		    unsigned long flag;
			uint8_t * pQbuffer;
            #if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_ALLQBUFFER..... \n");
            #endif
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);/*signature, let IOP331 know data has been readed */
			}
			pACB->acb_flags |= (ACB_F_IOCTL_WQBUFFER_CLEARED|ACB_F_IOCTL_RQBUFFER_CLEARED);
            spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			pACB->rqbuf_firstindex=0;
			pACB->rqbuf_lastindex=0;
			pACB->wqbuf_firstindex=0;
			pACB->wqbuf_lastindex=0;
			pQbuffer=pACB->rqbuffer;
            memset(pQbuffer, 0, sizeof(struct _QBUFFER));
			pQbuffer=pACB->wqbuffer;
            memset(pQbuffer, 0, sizeof(struct _QBUFFER));
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
			/*report success*/
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
		    if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_RETURN_CODE_3F:
		{
			#if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_RETURNCODE_3F..... \n");
            #endif
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_3F;
		    if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_SAY_HELLO:
		{
			int8_t * hello_string="Hello! I am ARCMSR";
            #if ARCMSR_DEBUG
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_SAY_HELLO..... \n");
            #endif
            memcpy(pcmdioctlfld->ioctldatabuffer,hello_string,(int16_t)strlen(hello_string));
            pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
	            retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_SAY_GOODBYE:
		{
            arcmsr_iop_parking(pACB);
		}
		break;
    case ARCMSR_IOCTL_FLUSH_ADAPTER_CACHE:
		{
			arcmsr_flush_adapter_cache(pACB);
			if(arcmsr_wait_msgint_ready(pACB))
			{
				printk("arcmsr%d ioctl flush cache wait 'flush adapter cache' timeout \n",pACB->adapter_index);
			}
		}
		break;
	default:
		retvalue= -EFAULT;
	}
ioctl_out:
    pci_free_consistent(pACB->pPCI_DEV, sizeof (struct _CMD_IOCTL_FIELD), pcmdioctlfld, cmd_handle);
    return retvalue;
}
/*
************************************************************************
**  arcmsr_ioctl
** Performs ioctl requests not satified by the upper levels.
** copy_from_user(to,from,n)
** copy_to_user(to,from,n)
**
**  The scsi_device struct contains what we know about each given scsi
**  device.
**
** FIXME(eric) - one of the great regrets that I have is that I failed to define
** these structure elements as something like sdev_foo instead of foo.  This would
** make it so much easier to grep through sources and so forth.  I propose that
** all new elements that get added to these structures follow this convention.
** As time goes on and as people have the stomach for it, it should be possible to 
** go back and retrofit at least some of the elements here with with the prefix.
**
**
**struct scsi_device {
**  %% private: %%
**	                                        %%
**	                                        %% This information is private to the scsi mid-layer.  Wrapping it in a
**	                                        %% struct private is a way of marking it in a sort of C++ type of way.
**	                                        %%
**
**	struct scsi_device      *next;	                    %% Used for linked list %%
**	struct scsi_device      *prev;	                    %% Used for linked list %%
**	wait_queue_head_t       scpnt_wait;	                %% Used to wait if device is busy %%
**	struct Scsi_Host        *host;
**	request_queue_t         request_queue;
**  atomic_t                device_active;              %% commands checked out for device %%
**	volatile unsigned short device_busy;	            %% commands actually active on low-level %%
**	int (*scsi_init_io_fn)  (struct scsi_cmnd *);	            %% Used to initialize  new request %%
**	Scsi_Cmnd               *device_queue;	            %% queue of SCSI Command structures %%
**
**  %% public: %%
**
**	unsigned int            id, lun, channel;
**	unsigned int            manufacturer;	            %% Manufacturer of device, for using vendor-specific cmd's %%
**	unsigned                sector_size;	            %% size in bytes %%
**	int                     attached;		            %% # of high level drivers attached to this %%
**	int                     access_count;	            %% Count of open channels/mounts %%
**	void                    *hostdata;		            %% available to low-level driver %%
**	devfs_handle_t          de;                         %% directory for the device %%
**	char                    type;
**	char                    scsi_level;
**	char                    vendor[8], model[16], rev[4];
**	unsigned char           current_tag;             	%% current tag %%
**	unsigned char           sync_min_period;	        %% Not less than this period %%
**	unsigned char           sync_max_offset;	        %% Not greater than this offset %%
**	unsigned char           queue_depth;	            %% How deep a queue to use %%
**	unsigned                online:1;
**	unsigned                writeable:1;
**	unsigned                removable:1;
**	unsigned                random:1;
**	unsigned                has_cmdblocks:1;
**	unsigned                changed:1;	                %% Data invalid due to media change %%
**	unsigned                busy:1;	                    %% Used to prevent races %%
**	unsigned                lockable:1;	                %% Able to prevent media removal %%
**	unsigned                borken:1;	                %% Tell the Seagate driver to be painfully slow on this device %%
**	unsigned                tagged_supported:1;	        %% Supports SCSI-II tagged queuing %%
**	unsigned                tagged_queue:1;	            %% SCSI-II tagged queuing enabled %%
**	unsigned                disconnect:1;	            %% can disconnect %%
**	unsigned                soft_reset:1;	            %% Uses soft reset option %%
**	unsigned                sync:1;	                    %% Negotiate for sync transfers %%
**	unsigned                wide:1;	                    %% Negotiate for WIDE transfers %%
**	unsigned                single_lun:1;	            %% Indicates we should only allow I/O to one of the luns for the device at a time. %%
**	unsigned                was_reset:1;	            %% There was a bus reset on the bus for this device %%
**	unsigned                expecting_cc_ua:1;	        %% Expecting a CHECK_CONDITION/UNIT_ATTN because we did a bus reset. %%
**	unsigned                device_blocked:1;	        %% Device returned QUEUE_FULL. %%
**	unsigned                ten:1;		                %% support ten byte read / write %%
**	unsigned                remap:1;	                %% support remapping  %%
**	unsigned                starved:1;	                %% unable to process commands because  host busy %%
**	int                     allow_revalidate;           %% Flag to allow revalidate to succeed in sd_open
**};
**
************************************************************************
*/
int arcmsr_ioctl(struct scsi_device *dev,int ioctl_cmd,void *arg) 
{
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
	int32_t match=0x55AA,i;
	
	#if ARCMSR_DEBUG
	printk("arcmsr_ioctl..................................................... \n");
	#endif

	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if((pACB=pHCBARC->pACB[i])!=NULL)
		{
			if(pACB->host==dev->host) 
			{  
    			match=i;
				break;
			}
		}
	}
	if(match==0x55AA)
	{
		return -ENXIO;
	}
	if(!arg)
	{
		return -EINVAL;
	}
	return(arcmsr_iop_ioctlcmd(pACB,ioctl_cmd,arg));
}
/*
**************************************************************************
**************************************************************************
*/
static struct _CCB * arcmsr_get_freeccb(struct _ACB *pACB)
{
    struct _CCB *pCCB;
    unsigned long flag;
	int ccb_startindex,ccb_doneindex;

    #if ARCMSR_DEBUG
	printk("arcmsr_get_freeccb: ccb_startindex=%d ccb_doneindex=%d\n",pACB->ccb_startindex,pACB->ccb_doneindex);
    #endif
	spin_lock_irqsave(&pACB->ccb_startindex_lockunlock,flag);
	ccb_doneindex=pACB->ccb_doneindex;
	ccb_startindex=pACB->ccb_startindex;
	pCCB=pACB->pccbringQ[ccb_startindex];
	ccb_startindex++;
	ccb_startindex %= ARCMSR_MAX_FREECCB_NUM;
	if(ccb_doneindex!=ccb_startindex)
	{
  		pACB->ccb_startindex=ccb_startindex;
	}
	else
	{
		pCCB=NULL;
	}
    spin_unlock_irqrestore(&pACB->ccb_startindex_lockunlock,flag);
	return(pCCB);
}
/*
***********************************************************************
**
** struct scsi_cmnd {
**	int          sc_magic;
**  // private: //
**	           //
**	           // This information is private to the scsi mid-layer. Wrapping it in a
**	           // struct private is a way of marking it in a sort of C++ type of way.
**	           //
**	struct Scsi_Host   *host;
**	unsigned short    state;
**	unsigned short    owner;
**	Scsi_Device      *device;
**	Scsi_Request     *sc_request;
**	struct scsi_cmnd   *next;
**	struct scsi_cmnd   *reset_chain;
**
**	int eh_state;		 // Used for state tracking in error handlr 
**	void (*done) (struct scsi_cmnd *);	
**            // Mid-level done function
**	           //
**	           // A SCSI Command is assigned a nonzero serial_number when internal_cmnd
**	           // passes it to the driver's queue command function. The serial_number
**	           // is cleared when scsi_done is entered indicating that the command has
**	           // been completed. If a timeout occurs,the serial number at the moment
**	           // of timeout is copied into serial_number_at_timeout. By subsequently
**	           // comparing the serial_number and serial_number_at_timeout fields
**	           // during abort or reset processing,we can detect whether the command
**	           // has already completed. This also detects cases where the command has
**	           // completed and the SCSI Command structure has already being reused
**	           // for another command,so that we can avoid incorrectly aborting or
**	           // resetting the new command.
**	           //
**
**	unsigned long     serial_number;
**	unsigned long     serial_number_at_timeout;
**
**	int          retries;
**	int          allowed;
**	int          timeout_per_command;
**	int          timeout_total;
**	int          timeout;
**
**	           //
**	           // We handle the timeout differently if it happens when a reset,
**	           // abort,etc are in process. 
**	           //
**	unsigned volatile char internal_timeout;
**	struct scsi_cmnd   *bh_next;	
**            // To enumerate the commands waiting to be processed.
**
**  // public: //
**
**	unsigned int     target;
**	unsigned int     lun;
**	unsigned int     channel;
**	unsigned char     cmd_len;
**	unsigned char     old_cmd_len;
**	unsigned char     sc_data_direction;
**	unsigned char     sc_old_data_direction;
**	           // These elements define the operation we are about to perform 
**	unsigned char     cmnd[MAX_COMMAND_SIZE];
**	unsigned       request_bufflen;	
**            // Actual request size 
**
**	struct timer_list eh_timeout;	
**            // Used to time out the command.
**	void         *request_buffer;	
**            // Actual requested buffer 
**	           // These elements define the operation we ultimately want to perform
**	unsigned char data_cmnd[MAX_COMMAND_SIZE];
**	unsigned short old_use_sg;	
**            // We save use_sg here when requesting sense info
**	unsigned short use_sg;	
**            // Number of pieces of scatter-gather 
**	unsigned short sglist_len;	
**            // size of malloc'd scatter-gather list
**	unsigned short abort_reason;	
**            // If the mid-level code requests an abort,this is the reason.
**	unsigned bufflen;	
**            // Size of data buffer 
**	void *buffer;		
**            // Data buffer 
**	unsigned underflow;	 
**            // Return error if less than this amount is transferred 
**	unsigned old_underflow;	
**            // save underflow here when reusing the command for error handling 
**
**	unsigned transfersize;
**           	 // How much we are guaranteed to transfer with each SCSI transfer 
**            // (ie,between disconnect/reconnects.  Probably==sector size 
**	int resid;		
**            // Number of bytes requested to be transferred 
**            // less actual number transferred (0 if not supported)
**	struct request request;	
**            // A copy of the command we are working on
**	unsigned char sense_buffer[SCSI_SENSE_BUFFERSIZE];		
**            // obtained by REQUEST SENSE when CHECK CONDITION is received on original command (auto-sense)
**	unsigned flags;
**	           // Used to indicate that a command which has timed out also
**	           // completed normally. Typically the completion function will
**	           // do nothing but set this flag in this instance because the
**	           // timeout handler is already running.
**	unsigned done_late:1;
**	           // Low-level done function - can be used by low-level driver to point
**	           // to completion function. Not used by mid/upper level code.
**	void (*scsi_done) (struct scsi_cmnd *);
**	           // The following fields can be written to by the host specific code. 
**	           // Everything else should be left alone. 
**	Scsi_Pointer SCp;	
**            // Scratchpad used by some host adapters 
**	unsigned char *host_scribble;	
**            // The host adapter is allowed to
**					   // call scsi_malloc and get some memory
**					   // and hang it here.   The host adapter
**					   // is also expected to call scsi_free
**					   // to release this memory. (The memory
**					   // obtained by scsi_malloc is guaranteed
**					   // to be at an address < 16Mb).
**	int result;		  
**            // Status code from lower level driver
**	unsigned char tag;	
**            // SCSI-II queued command tag 
**	unsigned long pid;	
**            // Process ID,starts at 0 
** };
**
** The Scsi_Cmnd structure is used by scsi.c internally,
** and for communication
** with low level drivers that support multiple outstanding commands.
**
**typedef struct scsi_pointer 
**{
**  char       * ptr;        // data pointer 
**  int        this_residual;   // left in this buffer 
**  struct scatterlist *buffer;      // which buffer 
**  int        buffers_residual; // how many buffers left 
**  
**  volatile int    Status;
**  volatile int    Message;
**  volatile int    have_data_in;
**  volatile int    sent_command;
**  volatile int    phase;
**} Scsi_Pointer;
***********************************************************************
*/
int arcmsr_queue_command(struct scsi_cmnd *cmd,void (* done)(struct scsi_cmnd *))
{
    struct Scsi_Host *host = cmd->device->host;
	struct _ACB *pACB=(struct _ACB *) host->hostdata;
    struct _CCB *pCCB;
	int target=cmd->device->id;
	int lun=cmd->device->lun;

    #if ARCMSR_DEBUG
    printk("arcmsr_queue_command:Cmd=%2x,TargetId=%d,Lun=%d \n",cmd->cmnd[0],target,lun);
    #endif

	cmd->scsi_done=done;
	cmd->host_scribble=NULL;
	cmd->result=0;
	if(cmd->cmnd[0]==SYNCHRONIZE_CACHE) /* 0x35 avoid synchronizing disk cache cmd during .remove : arcmsr_device_remove (linux bug) */
	{
		if(pACB->devstate[target][lun]==ARECA_RAID_GONE)
		{
            cmd->result=(DID_NO_CONNECT << 16);
		}
		cmd->scsi_done(cmd);
        return(0);
	}
	if(pACB->acb_flags & ACB_F_BUS_RESET)
	{
		printk("arcmsr%d bus reset and return busy \n",pACB->adapter_index);
	    cmd->result=(DID_BUS_BUSY << 16);
  	    cmd->scsi_done(cmd);
        return(0);
	}
	if(pACB->devstate[target][lun]==ARECA_RAID_GONE)
	{
		uint8_t block_cmd;

        block_cmd=cmd->cmnd[0] & 0x0f;
		if(block_cmd==0x08 || block_cmd==0x0a)
		{
			printk("arcmsr%d block 'read/write' command with gone raid volume Cmd=%2x,TargetId=%d,Lun=%d \n",pACB->adapter_index,cmd->cmnd[0],target,lun);
			cmd->result=(DID_NO_CONNECT << 16);
  			cmd->scsi_done(cmd);
			return(0);
		}
	}
	if((pCCB=arcmsr_get_freeccb(pACB)) != NULL)
	{
		arcmsr_build_ccb(pACB,pCCB,cmd);
		if(atomic_read(&pACB->ccboutstandingcount) < ARCMSR_MAX_OUTSTANDING_CMD)
		{   
			/*
			******************************************************************
			** and we can make sure there were no pending ccb in this duration
			******************************************************************
			*/
    		arcmsr_post_ccb(pACB,pCCB);
		}
		else
		{
			/*
			******************************************************************
			** Q of ccbwaitexec will be post out when any outstanding command complete
			******************************************************************
			*/
			arcmsr_queue_wait2go_ccb(pACB,pCCB);
		}
	}
	else
	{
		printk("arcmsr%d 'out of ccbs resource' ccb outstanding=%d pending=%d \n",pACB->adapter_index,atomic_read(&pACB->ccboutstandingcount),atomic_read(&pACB->ccbwait2gocount));
	    cmd->result=(DID_BUS_BUSY << 16);
  	    cmd->scsi_done(cmd);
	}
 	return(0);
}
/*
**********************************************************************
**  get firmware miscellaneous data
**********************************************************************
*/
static void arcmsr_get_firmware_spec(struct _ACB *pACB)
{
    char *acb_firm_model=pACB->firm_model;
    char *acb_firm_version=pACB->firm_version;
    char *iop_firm_model=(char *) (&pACB->pmu->message_rwbuffer[15]);   /*firm_model,15,60-67*/
    char *iop_firm_version=(char *) (&pACB->pmu->message_rwbuffer[17]); /*firm_version,17,68-83*/
	int count;

    writel(ARCMSR_INBOUND_MESG0_GET_CONFIG,&pACB->pmu->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(pACB))
	{
		printk("arcmsr%d wait 'get adapter firmware miscellaneous data' timeout \n",pACB->adapter_index);
	}
	count=8;
	while(count)
	{
        *acb_firm_model=readb(iop_firm_model);
        acb_firm_model++;
		iop_firm_model++;
		count--;
	}
	count=16;
	while(count)
	{
        *acb_firm_version=readb(iop_firm_version);
        acb_firm_version++;
		iop_firm_version++;
		count--;
	}
	printk("ARECA RAID ADAPTER%d: FIRMWARE VERSION %s \n",pACB->adapter_index,pACB->firm_version);
	if(strncmp(pACB->firm_version,"V1.37",5) < 0)
	{
        printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printk("!!!!!!   PLEASE UPDATE RAID FIRMWARE VERSION EQUAL OR MORE THAN 'V1.37'   !!!!!!\n");
		printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
    pACB->firm_request_len=readl(&pACB->pmu->message_rwbuffer[1]);   /*firm_request_len,1,04-07*/
	pACB->firm_numbers_queue=readl(&pACB->pmu->message_rwbuffer[2]); /*firm_numbers_queue,2,08-11*/
	pACB->firm_sdram_size=readl(&pACB->pmu->message_rwbuffer[3]);    /*firm_sdram_size,3,12-15*/
    pACB->firm_ide_channels=readl(&pACB->pmu->message_rwbuffer[4]);  /*firm_ide_channels,4,16-19*/
	return;
}
/*
**********************************************************************
**  start background rebulid
**********************************************************************
*/
static void arcmsr_start_adapter_bgrb(struct _ACB *pACB)
{
	#if ARCMSR_DEBUG
	printk("arcmsr_start_adapter_bgrb.................................. \n");
	#endif
	pACB->acb_flags |= ACB_F_MSG_START_BGRB;
	pACB->acb_flags &= ~ACB_F_MSG_STOP_BGRB;
    writel(ARCMSR_INBOUND_MESG0_START_BGRB,&pACB->pmu->inbound_msgaddr0);
	return;
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_polling_ccbdone(struct _ACB *pACB,struct _CCB *poll_ccb)
{
	struct _CCB *pCCB;
	uint32_t flag_ccb,outbound_intstatus,poll_ccb_done=0,poll_count=0;
	int id,lun;

	#if ARCMSR_DEBUG
	printk("arcmsr_polling_ccbdone.................................. \n");
	#endif
polling_ccb_retry:
	poll_count++;
	outbound_intstatus=readl(&pACB->pmu->outbound_intstatus) & pACB->outbound_int_enable;
	writel(outbound_intstatus,&pACB->pmu->outbound_intstatus);/*clear interrupt*/
	while(1)
	{
		if((flag_ccb=readl(&pACB->pmu->outbound_queueport))==0xFFFFFFFF)
		{
			if(poll_ccb_done)
			{
				break;/*chip FIFO no ccb for completion already*/
			}
			else
			{
                arc_mdelay(25);
                if(poll_count > 100)
				{
                    break;
				}
				goto polling_ccb_retry;
			}
		}
		/* check ifcommand done with no error*/
		pCCB=(struct _CCB *)(pACB->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
		if((pCCB->pACB!=pACB) || (pCCB->startdone!=ARCMSR_CCB_START))
		{
			if((pCCB->startdone==ARCMSR_CCB_ABORTED) && (pCCB==poll_ccb))
			{
		        printk("arcmsr%d scsi id=%d lun=%d ccb='0x%p' poll command abort successfully \n",pACB->adapter_index,pCCB->pcmd->device->id,pCCB->pcmd->device->lun,pCCB);
				pCCB->pcmd->result=DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
				poll_ccb_done=1;
				continue;
			}
		    printk("arcmsr%d polling get an illegal ccb command done ccb='0x%p' ccboutstandingcount=%d \n",pACB->adapter_index,pCCB,atomic_read(&pACB->ccboutstandingcount));
			continue;
		}
 		id=pCCB->pcmd->device->id;
		lun=pCCB->pcmd->device->lun;
		if((flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)==0)
		{
			if(pACB->devstate[id][lun]==ARECA_RAID_GONE)
			{
                pACB->devstate[id][lun]=ARECA_RAID_GOOD;
			}
 			pCCB->pcmd->result=DID_OK << 16;
			arcmsr_ccb_complete(pCCB);
		} 
		else 
		{   
			switch(pCCB->arcmsr_cdb.DeviceStatus)
			{
			case ARCMSR_DEV_SELECT_TIMEOUT:
				{
					pACB->devstate[id][lun]=ARECA_RAID_GONE;
					pCCB->pcmd->result=DID_TIME_OUT << 16;
					arcmsr_ccb_complete(pCCB);
				}
				break;
			case ARCMSR_DEV_ABORTED:
			case ARCMSR_DEV_INIT_FAIL:
				{
				    pACB->devstate[id][lun]=ARECA_RAID_GONE;
					pCCB->pcmd->result=DID_BAD_TARGET << 16;
					arcmsr_ccb_complete(pCCB);
				}
				break;
			case SCSISTAT_CHECK_CONDITION:
				{
				    pACB->devstate[id][lun]=ARECA_RAID_GOOD;
		    		arcmsr_report_sense_info(pCCB);
					arcmsr_ccb_complete(pCCB);
				}
				break;
			default:
				/* error occur Q all error ccb to errorccbpending Q*/
				printk("arcmsr%d scsi id=%d lun=%d polling and getting command error done, but got unknow DeviceStatus=0x%x \n",pACB->adapter_index,id,lun,pCCB->arcmsr_cdb.DeviceStatus);
				pACB->devstate[id][lun]=ARECA_RAID_GONE;
				pCCB->pcmd->result=DID_BAD_TARGET << 16;/*unknow error or crc error just for retry*/
				arcmsr_ccb_complete(pCCB);
				break;
			}
		}
	}	/*drain reply FIFO*/
	return;
}
/*
**********************************************************************
**  start background rebulid
**********************************************************************
*/
static void arcmsr_iop_init(struct _ACB *pACB)
{
    uint32_t intmask_org,mask,outbound_doorbell,firmware_state=0;

	#if ARCMSR_DEBUG
	printk("arcmsr_iop_init.................................. \n");
	#endif
	do
	{
        firmware_state=readl(&pACB->pmu->outbound_msgaddr1);
	}while((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK)==0);
    intmask_org=readl(&pACB->pmu->outbound_intmask);/*change "disable iop interrupt" to arcmsr_initialize*/ 
	arcmsr_get_firmware_spec(pACB);
	/*start background rebuild*/
	arcmsr_start_adapter_bgrb(pACB);
	if(arcmsr_wait_msgint_ready(pACB))
	{
		printk("arcmsr%d wait 'start adapter background rebulid' timeout \n",pACB->adapter_index);
	}
	/* clear Qbuffer if door bell ringed */
	outbound_doorbell=readl(&pACB->pmu->outbound_doorbell);
	if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK)
	{
		writel(outbound_doorbell,&pACB->pmu->outbound_doorbell);/*clear interrupt */
        writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&pACB->pmu->inbound_doorbell);
	}
	/* enable outbound Post Queue,outbound message0,outbell doorbell Interrupt */
	mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
    writel(intmask_org & mask,&pACB->pmu->outbound_intmask);
	pACB->outbound_int_enable = ~(intmask_org & mask) & 0x000000ff;
	pACB->acb_flags |=ACB_F_IOP_INITED;
	return;
}
/*
****************************************************************************
****************************************************************************
*/
int arcmsr_bus_reset(struct scsi_cmnd *cmd)
{
	struct _ACB *pACB;
    int retry=0;

	pACB=(struct _ACB *) cmd->device->host->hostdata;
	printk("arcmsr%d bus reset ..... \n",pACB->adapter_index);
	pACB->num_resets++;
	pACB->acb_flags |= ACB_F_BUS_RESET;
	while(atomic_read(&pACB->ccboutstandingcount)!=0 && retry < 400)
	{
        arcmsr_interrupt(pACB);
        arc_mdelay(25);
		retry++;
	}
	arcmsr_iop_reset(pACB);
    pACB->acb_flags &= ~ACB_F_BUS_RESET;
 	return SUCCESS;
} 
/*
*****************************************************************************************
*****************************************************************************************
*/
static int arcmsr_seek_cmd2abort(struct scsi_cmnd *pabortcmd)
{
    struct _ACB *pACB=(struct _ACB *) pabortcmd->device->host->hostdata;
 	struct _CCB *pCCB;
	uint32_t intmask_org,mask;
    int i=0;

    #if ARCMSR_DEBUG
    printk("arcmsr_seek_cmd2abort.................. \n");
    #endif
	pACB->num_aborts++;
	/* 
	*****************************************************************************
	** It is the upper layer do abort command this lock just prior to calling us.
	** First determine if we currently own this command.
	** Start by searching the device queue. If not found
	** at all,and the system wanted us to just abort the
	** command return success.
	*****************************************************************************
	*/
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
            pCCB=pACB->pccb_pool[i];
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				if(pCCB->pcmd==pabortcmd)
				{
                    pCCB->startdone=ARCMSR_CCB_ABORTED;
					printk("arcmsr%d scsi id=%d lun=%d abort ccb '0x%p' outstanding command \n",pACB->adapter_index,pabortcmd->device->id,pabortcmd->device->lun,pCCB);
                    goto abort_outstanding_cmd;
				}
			}
		}
	}
	/*
	*************************************************************
	** seek this command at our command list 
	** if command found then remove,abort it and free this CCB
	*************************************************************
	*/
	if(atomic_read(&pACB->ccbwait2gocount)!=0)
	{
		for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
		{
			pCCB=pACB->pccbwait2go[i];
			if(pCCB!=NULL)
			{
				if(pCCB->pcmd==pabortcmd)
				{
					printk("arcmsr%d scsi id=%d lun=%d abort ccb '0x%p' pending command \n",pACB->adapter_index,pabortcmd->device->id,pabortcmd->device->lun,pCCB);
					pACB->pccbwait2go[i]=NULL;
					pCCB->startdone=ARCMSR_CCB_ABORTED;
					pCCB->pcmd->result=DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
		   			atomic_dec(&pACB->ccbwait2gocount);
					return(SUCCESS);
				}
			}
		}
	}
	return (SUCCESS);
abort_outstanding_cmd:
    /* disable all outbound interrupt */
	intmask_org=readl(&pACB->pmu->outbound_intmask);
    writel(intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE,&pACB->pmu->outbound_intmask);
    /* do not talk to iop 331 abort command */
	arcmsr_polling_ccbdone(pACB,pCCB);
	/* enable all outbound interrupt */
	mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
    writel(intmask_org & mask,&pACB->pmu->outbound_intmask);
	atomic_set(&pACB->ccboutstandingcount,0);
    return (SUCCESS);
}
/*
*****************************************************************************************
*****************************************************************************************
*/
int arcmsr_cmd_abort(struct scsi_cmnd *cmd)
{
    struct _ACB *pACB=(struct _ACB *) cmd->device->host->hostdata;
	int error;

    printk("arcmsr%d abort device command of scsi id=%d lun=%d \n",pACB->adapter_index,cmd->device->id,cmd->device->lun);
    /*
	************************************************
	** the all interrupt service routine is locked
	** we need to handle it as soon as possible and exit
	************************************************
	*/
	error=arcmsr_seek_cmd2abort(cmd);
	if(error !=SUCCESS)
	{
		printk("arcmsr%d abort command failed scsi id=%d lun=%d \n",pACB->adapter_index,cmd->device->id,cmd->device->lun);
	}
	return (error);
}
/*
*********************************************************************
** arcmsr_info()
**struct pci_dev {
**	struct list_head      global_list;		## node in list of all PCI devices ##
**	struct list_head      bus_list;	    	## node in per-bus list ##
**	struct pci_bus	      *bus;		    	## bus this device is on ##
**	struct pci_bus	      *subordinate;		## bus this device bridges to ##
**	void		          *sysdata;	    	## hook for sys-specific extension ##
**	struct proc_dir_entry *procent;	        ## device entry in /proc/bus/pci ##
**	unsigned int	      devfn;		    ## encoded device & function index ##
**	unsigned short	      vendor;
**	unsigned short	      device;
**	unsigned short	      subsystem_vendor;
**	unsigned short	      subsystem_device;
**	unsigned int	      class;		    ## 3 bytes: (base,sub,prog-if) ##
**	u8		              hdr_type;	        ## PCI header type (`multi' flag masked out) ##
**	u8		              rom_base_reg;	    ## which config register controls the ROM ##
**
**	struct pci_driver    *driver;	        ## which driver has allocated this device ##
**	void		         *driver_data;	    ## data private to the driver ##
**	u64		              dma_mask;	        ## Mask of the bits of bus address this device implements.  Normally this is
**					                        ## 0xffffffff.  You only need to change this if your device has broken DMA
**					                        ## or supports 64-bit transfers.  
**	u32                   current_state;    ## Current operating state. In ACPI-speak, this is D0-D3, D0 being fully functional, and D3 being off. ##
**	unsigned short vendor_compatible[DEVICE_COUNT_COMPATIBLE]; ## device is compatible with these IDs ##
**	unsigned short device_compatible[DEVICE_COUNT_COMPATIBLE];
**	 										##
**	 										##Instead of touching interrupt line and base address registers
**	 										##directly, use the values stored here. They might be different!
**	 										##
**	unsigned int	      irq;
**	struct resource       resource[DEVICE_COUNT_RESOURCE]; ## I/O and memory regions + expansion ROMs ##
**	struct resource       dma_resource[DEVICE_COUNT_DMA];
**	struct resource       irq_resource[DEVICE_COUNT_IRQ];
**	char		          name[90];	                       ## device name ##
**	char		          slot_name[8];	                   ## slot name ##
**	u32		              saved_state[16];                 ## for saving the config space before suspend ##
**	int		              active;		                   ## ISAPnP: device is active ##
**	int		              ro;		                       ## ISAPnP: read only ##
**	unsigned short	      regs;		                       ## ISAPnP: supported registers ##
**	                                                       ## These fields are used by common fixups ##
**	unsigned short	      transparent:1;	               ## Transparent PCI bridge ##
**	int (*prepare)(struct pci_dev *dev);	               ## ISAPnP hooks ##
**	int (*activate)(struct pci_dev *dev);
**	int (*deactivate)(struct pci_dev *dev);
**};
**      
*********************************************************************
*/
const char *arcmsr_info(struct Scsi_Host *host)
{
	static char buf[256];
	struct _ACB *   pACB;
	uint16_t device_id;

    #if ARCMSR_DEBUG
    printk("arcmsr_info.............\n");
    #endif
	pACB=(struct _ACB *) host->hostdata;
	device_id=pACB->pPCI_DEV->device;
	switch(device_id)
	{
	case PCIDeviceIDARC1110:
		{
			sprintf(buf,"ARECA ARC1110 PCI-X 4 PORTS SATA RAID CONTROLLER\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1120:
		{
			sprintf(buf,"ARECA ARC1120 PCI-X 8 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1130:
		{
			sprintf(buf,"ARECA ARC1130 PCI-X 12 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1160:
		{
			sprintf(buf,"ARECA ARC1160 PCI-X 16 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1170:
		{
			sprintf(buf,"ARECA ARC1170 PCI-X 24 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1210:
		{
			sprintf(buf,"ARECA ARC1210 PCI-EXPRESS 4 PORTS SATA RAID CONTROLLER\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1220:
		{
			sprintf(buf,"ARECA ARC1220 PCI-EXPRESS 8 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1230:
		{
			sprintf(buf,"ARECA ARC1230 PCI-EXPRESS 12 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1260:
		{
			sprintf(buf,"ARECA ARC1260 PCI-EXPRESS 16 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	case PCIDeviceIDARC1270:
		{
			sprintf(buf,"ARECA ARC1270 PCI-EXPRESS 24 PORTS SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	default:
		{
			sprintf(buf,"ARECA X-TYPE SATA RAID CONTROLLER (RAID6-ENGINE Inside)\n        %s",ARCMSR_DRIVER_VERSION);
		    break;
		}
	}
	return buf;
}
/*
************************************************************************
************************************************************************
*/
static int arcmsr_initialize(struct _ACB *pACB,struct pci_dev *pPCI_DEV)
{
	uint32_t intmask_org,page_base,page_offset,mem_base_start,ccb_phyaddr_hi32;
	dma_addr_t dma_addr,dma_coherent_handle;
	void *page_remapped;
    void *dma_coherent;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
	uint8_t pcicmd;
	int i,j;
    struct _CCB *pccb_tmp;

	#if ARCMSR_DEBUG
	printk("arcmsr_initialize....................................\n");
	#endif
  	/* Enable Busmaster/Mem */
	pci_read_config_byte(pPCI_DEV,PCI_COMMAND,&pcicmd);
	pci_write_config_byte(pPCI_DEV,PCI_COMMAND,pcicmd|PCI_COMMAND_INVALIDATE|PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY); 
	mem_base_start=(uint32_t)arcget_pcicfg_base(pPCI_DEV,0);
	page_base=mem_base_start & PAGE_MASK;
	page_offset=mem_base_start - page_base;
	page_remapped=ioremap(page_base,page_offset + 0x1FFF);
	if( page_remapped==NULL )
	{
		printk("arcmsr%d memory mapping region fail \n",arcmsr_adapterCnt);
		return(ENXIO);
	}
	pACB->pmu=(PMU)(page_remapped+page_offset);
    pACB->acb_flags |= (ACB_F_IOCTL_WQBUFFER_CLEARED|ACB_F_IOCTL_RQBUFFER_CLEARED);
	pACB->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
 	pACB->irq=pPCI_DEV->irq;
    /* 
	*******************************************************************************
	**                 Allocate the pccb_pool memory 
	**     Attempt to claim larger area for request queue pCCB). 
	*******************************************************************************
	*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	dma_coherent = dma_alloc_coherent(&pPCI_DEV->dev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct _CCB) + 0x20, &dma_coherent_handle, GFP_KERNEL);
#else
	dma_coherent = pci_alloc_consistent(pPCI_DEV, ARCMSR_MAX_FREECCB_NUM * sizeof(struct _CCB) + 0x20, &dma_coherent_handle);
#endif
	if (dma_coherent == NULL)
	{
		printk("arcmsr%d dma_alloc_coherent got error \n",arcmsr_adapterCnt);
    	return -ENOMEM;
	}
	pACB->dma_coherent=dma_coherent;
	pACB->dma_coherent_handle=dma_coherent_handle;
	memset(dma_coherent, 0, ARCMSR_MAX_FREECCB_NUM * sizeof(struct _CCB)+0x20);
	if(((unsigned long)dma_coherent & 0x1F)!=0) /*ccb address must 32 (0x20) boundary*/
	{
		dma_coherent=dma_coherent+(0x20-((unsigned long)dma_coherent & 0x1F));
		dma_coherent_handle=dma_coherent_handle+(0x20-((unsigned long)dma_coherent_handle & 0x1F));
	}
	dma_addr=dma_coherent_handle;
	pccb_tmp=(struct _CCB *)dma_coherent;
	for(i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) 
	{
		pccb_tmp->cdb_shifted_phyaddr=dma_addr >> 5;
		pccb_tmp->pACB=pACB;
		pACB->pccbringQ[i]=pACB->pccb_pool[i]=pccb_tmp;
		dma_addr=dma_addr+sizeof(struct _CCB);
		pccb_tmp++;
  	}
	pACB->vir2phy_offset=(unsigned long)pccb_tmp-(unsigned long)dma_addr;
	/*
	********************************************************************
	** init raid volume state
	********************************************************************
	*/
	for(i=0;i<ARCMSR_MAX_TARGETID;i++)
	{
		for(j=0;j<ARCMSR_MAX_TARGETLUN;j++)
		{
			pACB->devstate[i][j]=ARECA_RAID_GOOD;
		}
	}
	/*
	********************************************************************
	** here we need to tell iop 331 our pccb_tmp.HighPart 
	** if pccb_tmp.HighPart is not zero
	********************************************************************
	*/
	ccb_phyaddr_hi32=(uint32_t) ((dma_coherent_handle>>16)>>16);
	if(ccb_phyaddr_hi32!=0)
	{
        writel(ARCMSR_SIGNATURE_SET_CONFIG,&pACB->pmu->message_rwbuffer[0]);
        writel(ccb_phyaddr_hi32,&pACB->pmu->message_rwbuffer[1]);
		writel(ARCMSR_INBOUND_MESG0_SET_CONFIG,&pACB->pmu->inbound_msgaddr0);
		if(arcmsr_wait_msgint_ready(pACB))
		{
			printk("arcmsr%d 'set ccb high part physical address' timeout \n",arcmsr_adapterCnt);
		}
	}
 	pACB->adapter_index=arcmsr_adapterCnt;
	pHCBARC->pACB[arcmsr_adapterCnt]=pACB;
    /* disable iop all outbound interrupt */
    intmask_org=readl(&pACB->pmu->outbound_intmask);
    writel(intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE,&pACB->pmu->outbound_intmask);
	arcmsr_adapterCnt++;
    return(0);
}
/*
*********************************************************************
*********************************************************************
*/
static int arcmsr_set_info(char *buffer,int length)
{
	#if ARCMSR_DEBUG
	printk("arcmsr_set_info.............\n");
	#endif
	return (0);
}
/*
*********************************************************************
*********************************************************************
*/
static void arcmsr_pcidev_disattach(struct _ACB *pACB)
{
	struct _CCB *pCCB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
    uint32_t intmask_org;
	int i=0,poll_count=0;
    #if ARCMSR_DEBUG
    printk("arcmsr_pcidev_disattach.................. \n");
    #endif
	/* disable all outbound interrupt */
    intmask_org=readl(&pACB->pmu->outbound_intmask);
    writel(intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE,&pACB->pmu->outbound_intmask);
	/* stop adapter background rebuild */
	arcmsr_stop_adapter_bgrb(pACB);
	if(arcmsr_wait_msgint_ready(pACB))
	{
		printk("arcmsr%d pcidev disattach wait 'stop adapter rebulid' timeout \n",pACB->adapter_index);
	}
	arcmsr_flush_adapter_cache(pACB);
	if(arcmsr_wait_msgint_ready(pACB))
	{
		printk("arcmsr%d pcidev disattach wait 'flush adapter cache' timeout \n",pACB->adapter_index);
	}
	/* abort all outstanding command */
	pACB->acb_flags |= ACB_F_SCSISTOPADAPTER;
	pACB->acb_flags &= ~ACB_F_IOP_INITED;
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{  
        while(atomic_read(&pACB->ccboutstandingcount)!=0 && (poll_count < 256))
		{
            arcmsr_interrupt(pACB);
            arc_mdelay(25);
            poll_count++;
		}
		if(atomic_read(&pACB->ccboutstandingcount)!=0)
	    {  
			/* talk to iop 331 outstanding command aborted*/
			arcmsr_abort_allcmd(pACB);
			if(arcmsr_wait_msgint_ready(pACB))
			{
				printk("arcmsr%d pcidev disattach wait 'abort all outstanding command' timeout \n",pACB->adapter_index);
			}
			/*clear all outbound posted Q*/
			for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
			{
				readl(&pACB->pmu->outbound_queueport);
			}
			for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
			{
				pCCB=pACB->pccb_pool[i];
				if(pCCB->startdone==ARCMSR_CCB_START)
				{
					pCCB->startdone=ARCMSR_CCB_ABORTED;
   					pCCB->pcmd->result=DID_ABORT << 16; 
					arcmsr_ccb_complete(pCCB);
				}
			}
		}
 	}
	if(atomic_read(&pACB->ccbwait2gocount)!=0)
	{	/*remove first wait2go ccb and abort it*/
		for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
		{
			pCCB=pACB->pccbwait2go[i];
			if(pCCB!=NULL)
			{
				pACB->pccbwait2go[i]=NULL;
                pCCB->startdone=ARCMSR_CCB_ABORTED;
   				pCCB->pcmd->result=DID_ABORT << 16; 
				arcmsr_ccb_complete(pCCB);
				atomic_dec(&pACB->ccbwait2gocount);
			}
		}
	}
	atomic_set(&pACB->ccboutstandingcount,0);
	free_irq(pACB->pPCI_DEV->irq,pACB);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	pci_release_regions(pACB->pPCI_DEV);
#endif
 	iounmap(pACB->pmu);
    arcmsr_free_pci_pool(pACB);
    pHCBARC->pACB[pACB->adapter_index]=0; /* clear record */
    arcmsr_adapterCnt--;
 	return;
}
/*
***************************************************************
***************************************************************
*/
static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf)
{
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
	struct Scsi_Host *host;
	int i;

	#if ARCMSR_DEBUG
	printk("arcmsr_halt_notify............................1 \n");
	#endif
	if((event !=SYS_RESTART) && (event !=SYS_HALT) && (event !=SYS_POWER_OFF))
	{
		return NOTIFY_DONE;
	}
 	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		pACB=pHCBARC->pACB[i];
		if(pACB==NULL) 
		{
			continue;
		}
		/* Flush cache to disk */
		/* Free irq,otherwise extra interrupt is generated	 */
		/* Issue a blocking(interrupts disabled) command to the card */
		host=pACB->host;
        arcmsr_pcidev_disattach(pACB);
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
        scsi_remove_host(host);
	    scsi_host_put(host);
    #else
 	    scsi_unregister(host);
    #endif
	}
 	unregister_chrdev(pHCBARC->arcmsr_major_number, "arcmsr");
	unregister_reboot_notifier(&arcmsr_event_notifier);
	return NOTIFY_OK;
}
/*
*********************************************************************
*********************************************************************
*/
#undef SPRINTF
#define SPRINTF(args...) pos +=sprintf(pos,## args)
#define YESNO(YN)\
if(YN) SPRINTF(" Yes ");\
else SPRINTF(" No ")
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
#else
    int arcmsr_proc_info(char * buffer,char ** start,off_t offset,int length,int hostno,int inout)
#endif
{
	uint8_t  i;
    char * pos=buffer;
    struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;

	#if ARCMSR_DEBUG
	printk("arcmsr_proc_info.............\n");
	#endif
    if(inout) 
	{
  	    return(arcmsr_set_info(buffer,length));
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		pACB=pHCBARC->pACB[i];
	    if(pACB==NULL) 
			continue;
		SPRINTF("ARECA SATA RAID Mass Storage Host Adadpter \n");
		SPRINTF("Driver Version %s ",ARCMSR_DRIVER_VERSION);
		SPRINTF("IRQ%d \n",pACB->pPCI_DEV->irq);
		SPRINTF("===========================\n"); 
	}
	*start=buffer + offset;
	if(pos - buffer < offset)
	{
  	    return 0;
	}
    else if(pos - buffer - offset < length)
	{
	    return (pos - buffer - offset);
	}
    else
	{
	    return length;
	}
}
/*
************************************************************************
************************************************************************
*/
int arcmsr_release(struct Scsi_Host *host)
{
	struct _ACB *pACB;
	struct _HCBARC *pHCBARC= &arcmsr_host_control_block;
    uint8_t match=0xff,i;

	#if ARCMSR_DEBUG
	printk("arcmsr_release...........................\n");
	#endif
	if(host==NULL)
	{
        return -ENXIO;
	}
    pACB=(struct _ACB *)host->hostdata;
	if(pACB==NULL)
	{
        return -ENXIO;
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if(pHCBARC->pACB[i]==pACB) 
		{  
			match=i;
		}
	}
	if(match==0xff)
	{
		return -ENXIO;
	}
	/* Flush cache to disk */
	/* Free irq,otherwise extra interrupt is generated	 */
	/* Issue a blocking(interrupts disabled) command to the card */
    arcmsr_pcidev_disattach(pACB);
	scsi_unregister(host);
	/*if this is last pACB */
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if(pHCBARC->pACB[i]!=NULL)
		{ 
			return(0);/* this is not last adapter's release */
		}
	}
	unregister_chrdev(pHCBARC->arcmsr_major_number, "arcmsr");
	unregister_reboot_notifier(&arcmsr_event_notifier);
	return(0);
}

