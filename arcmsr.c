/*
******************************************************************************************
**	  O.S	: Linux
**   FILE NAME	: arcmsr.c
**	  BY	: Erich	Chen   
**   Description: SCSI RAID Device Driver for 
**		  ARCMSR RAID Host adapter 
************************************************************************
** Copyright (C) 2002 -	2005, Areca Technology Corporation All rights reserved.
**
**     Web site: www.areca.com.tw
**	 E-mail: erich@areca.com.tw
**
** This	program	is free	software; you can redistribute it and/or modify
** it under the	terms of the GNU General Public	License	version	2 as
** published by	the Free Software Foundation.
** This	program	is distributed in the hope that	it will	be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
** GNU General Public License for more details.
************************************************************************
** Redistribution and use in source and	binary forms,with or without
** modification,are permitted provided that the	following conditions
** are met:
** 1. Redistributions of source	code must retain the above copyright
**    notice,this list of conditions and the following disclaimer.
** 2. Redistributions in binary	form must reproduce the	above copyright
**    notice,this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be	used to	endorse	or promote products
**    derived from this	software without specific prior	written	permission.
**
** THIS	SOFTWARE IS PROVIDED BY	THE AUTHOR ``AS	IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES,INCLUDING,BUT NOT	LIMITED	TO,THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A	PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR	BE LIABLE FOR ANY DIRECT,INDIRECT,
** INCIDENTAL,SPECIAL,EXEMPLARY,OR CONSEQUENTIAL DAMAGES(INCLUDING,BUT
** NOT LIMITED TO,PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA,OR PROFITS; OR BUSINESS	INTERRUPTION)HOWEVER CAUSED AND	ON ANY
** THEORY OF LIABILITY,WHETHER IN CONTRACT,STRICT LIABILITY,OR TORT
**(INCLUDING NEGLIGENCE	OR OTHERWISE)ARISING IN	ANY WAY	OUT OF THE USE OF
** THIS	SOFTWARE,EVEN IF ADVISED OF THE	POSSIBILITY OF SUCH DAMAGE.
**************************************************************************
** History
**
**	  REV#	    DATE(MM/DD/YYYY)		NAME		       DESCRIPTION
**     1.00.00.00       3/31/2004	        Erich Chen	              First release
**     1.10.00.04       7/28/2004	        Erich Chen	              modify	for ioctl
**     1.10.00.06       8/28/2004	        Erich Chen	              modify	for 2.6.x
**     1.10.00.08       9/28/2004	        Erich Chen	              modify	for x86_64 
**     1.10.00.10      10/10/2004	        Erich Chen	              bug fix	for SMP	& ioctl
**     1.20.00.00      11/29/2004	        Erich Chen	              bug fix	with arcmsr_bus_reset when PHY error
**     1.20.00.02      12/09/2004	        Erich Chen	              bug fix	with over 2T bytes RAID	Volume
**     1.20.00.04       1/09/2005		Erich Chen	              fits for Debian linux kernel version 2.2.xx 
**     1.20.0X.07       3/28/2005		Erich Chen	              sync for 1.20.00.07 (linux.org version)
**							                   			       remove some unused function
**							                   			       --.--.0X.-- is  for old style kernel compatibility
**     1.20.0X.08       6/23/2005	        Erich Chen	              bug fix with abort command,in case of heavy loading when sata cable
**							                   			       working on low quality connection
**     1.20.0X.09       9/12/2005	        Erich Chen	              bug fix with abort command handling,and firmware version check	
**							                   			        and firmware update notify for hardware bug fix
**     1.20.0X.10       9/23/2005	        Erich Chen	               enhance sysfs function for change driver's max tag Q number.
**							                   			        add DMA_64BIT_MASK for backward compatible with all 2.6.x
**							                   			        add some useful message for abort command
**							                   			        add ioctl code 'ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE'
**							                   			        customer can send this command for sync raid volume data
**     1.20.0X.11       9/29/2005	        Erich Chen	               by comment of Arjan van de Ven fix incorrect msleep redefine
**							                   			        cast off sizeof(dma_addr_t) condition for 64bit pci_set_dma_mask
**     1.20.0X.12       9/30/2005	        Erich Chen	               bug fix with 64bit platform's ccbs using if over 4G system memory
**							                   			        change 64bit pci_set_consistent_dma_mask into 32bit
**							                  			        increcct adapter count if adapter initialize fail.
**							                  		                miss edit at arcmsr_build_ccb....
**							                   			         psge += sizeof(struct SG64ENTRY *) => psge += sizeof(struct SG64ENTRY)
**							                   			         64 bits sg entry would be incorrectly calculated
**							                   			         thanks Kornel Wieliczek give me kindly notify and detail description
**     1.20.0X.13      11/15/2005	        Erich Chen	                 scheduling pending ccb with 'first in first out'
**							                                             new firmware update notify
**		                11/07/2006	        Erich Chen	                 1.remove #include config.h and devfs_fs_kernel.h
**                                                                                           2.enlarge the timeout duration of each scsi command it could aviod the vibration factor 
**     						                                             with sata disk on some bad machine
**							   
**     1.20.0X.14      05/02/2007	        Erich Chen/Nick Cheng	    1.add PCI-Express error recovery function and AER capability	
**                                                                         			    2.add the selection of ARCMSR_MAX_XFER_SECTORS_B=4096 if firmware version is newer than 1.41
**                                                                                           3.modify arcmsr_iop_reset to improve the stability
**											                  4.delect arcmsr_modify_timeout routine because it would malfunction as removal and recovery the lun
**											                  if somebody needs to adjust the scsi command timeout value, the script could be available on Areca FTP site or contact Areca support team 
**											                  5.modify the ISR, arcmsr_interrupt routine, to prevent the inconsistent with sg_mod driver if application directly calls the arcmsr driver 
**											                  w/o passing through  scsi mid layer		
**												       6.delect the callback function, arcmsr_ioctl
**   1.20.0X.14        08/27/2007	Erich Chen/Nick Cheng		    1. modify orig_mask readl(&reg->outbound_intmask)|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE in arcmsr_disable_allintr()
**											    2. delect the "results " from pci_enable_pcie_error_reporting(pdev) in arcmsr_probe() 
******************************************************************************************
*/
#define	ARCMSR_DEBUG	0			  
/************************************/
#if defined __KERNEL__
	#include <linux/version.h>
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif
	/* modversions.h should	be before should be before module.h */
    	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		#if defined( MODVERSIONS )
			#include <config/modversions.h>
		#endif
    	#endif
	#include <linux/module.h>
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

MODULE_AUTHOR("Erich Chen <erich@areca.com.tw>");
MODULE_VERSION(ARCMSR_DRIVER_VERSION);
MODULE_DESCRIPTION("ARECA (ARC11xx/12xx/13xx/16xx) SATA/SAS RAID HOST Adapter");

#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

/*
**********************************************************************************
**********************************************************************************
*/
static u_int8_t	arcmsr_adapterCnt=0;
static struct HCBARC arcmsr_host_control_block;
static int arcmsr_initialize(struct AdapterControlBlock	*acb,struct pci_dev *pdev);
static void arcmsr_free_ccb_pool(struct	AdapterControlBlock *acb);
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb);
static void arcmsr_iop_init(struct AdapterControlBlock *acb);
static void arcmsr_polling_ccbdone(struct AdapterControlBlock *acb);
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock	*acb);
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb);
static void arcmsr_get_firmware_spec(struct AdapterControlBlock	*acb);
static u_int8_t	arcmsr_wait_msgint_ready(struct	AdapterControlBlock *acb);
static void arcmsr_done4_abort_postqueue(struct AdapterControlBlock *acb);

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
	static pci_ers_result_t arcmsr_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state);
       static pci_ers_result_t arcmsr_pci_slot_reset(struct pci_dev *pdev);
#endif
/*
**********************************************************************************
**********************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
	static	struct pci_error_handlers arcmsr_pci_error_handlers=
	{ 
		.error_detected	        = arcmsr_pci_error_detected,
 		.slot_reset 		= arcmsr_pci_slot_reset,
	};
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
	/* We do our own ID filtering.	So, grab all SCSI storage class	devices. */
	static struct pci_device_id arcmsr_device_id_table[] = 
	{
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1110)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1120)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1130)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1160)},
		{PCI_DEVICE(PCI_VENDOR_ID_ARECA, PCI_DEVICE_ID_ARECA_1170)},
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
		.name		      		= "arcmsr",
		.id_table	      		= arcmsr_device_id_table,
		.probe		      		= arcmsr_probe,
		.remove	     	      	= arcmsr_remove,
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
	    .shutdown				= arcmsr_shutdown,
	    #endif               	
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
		.err_handler      		= &arcmsr_pci_error_handlers,
        #endif
	};
	/*
	*********************************************************************
	*********************************************************************
	*/
    static irqreturn_t arcmsr_do_interrupt(int irq,void *dev_id, struct pt_regs *regs)
	{
		irqreturn_t handle_state;
        struct HCBARC *pHCBARC= &arcmsr_host_control_block;
	    struct AdapterControlBlock *acb;
	    struct AdapterControlBlock *acbtmp;
		unsigned long flags;
		int i=0;

		#if ARCMSR_DEBUG
		printk("arcmsr_do_interrupt.................. \n");
		#endif

		acb=(struct AdapterControlBlock	*)dev_id;
		acbtmp=pHCBARC->acb[i];
		while((acb != acbtmp) && acbtmp	&& (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			acbtmp=pHCBARC->acb[i];
		}
		if(!acbtmp)
		{
		    #if	ARCMSR_DEBUG
			printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
	    	    #endif
			return IRQ_NONE;
		}
		spin_lock_irqsave(acb->host->host_lock,	flags);
		handle_state=arcmsr_interrupt(acb);
		spin_unlock_irqrestore(acb->host->host_lock, flags);
		return(handle_state);
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
    int	arcmsr_bios_param(struct scsi_device *sdev, struct block_device	*bdev,sector_t capacity, int *geom)
	{
		int ret,heads,sectors,cylinders;
		unsigned char *buffer;/* return	copy of	block device's partition table */

		#if ARCMSR_DEBUG
		printk("arcmsr_bios_param.................. \n");
		#endif

		buffer = scsi_bios_ptable(bdev);
		if(buffer) 
		{
			ret = scsi_partsize(buffer, capacity, &geom[2],	&geom[0], &geom[1]);
			kfree(buffer);
			if (ret	!= -1)
			{
				return(ret);
			}
		}
		heads=64;
		sectors=32;
		cylinders=sector_div(capacity, heads * sectors);
		if(cylinders >=	1024)
		{
			heads=255;
			sectors=63;
			cylinders=sector_div(capacity, heads * sectors);
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
	static int arcmsr_probe(struct pci_dev *pdev,const struct pci_device_id *id)
	{
		struct Scsi_Host *host;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC= &arcmsr_host_control_block;
		uint8_t bus,dev_fun;
		#if ARCMSR_DEBUG
		printk("arcmsr_probe............................\n");
		#endif
		if(pci_enable_device(pdev))
		{
			printk("arcmsr%d: adapter probe: pci_enable_device error \n",arcmsr_adapterCnt);
			return -ENODEV;
		}
		if((host=scsi_host_alloc(&arcmsr_scsi_host_template,sizeof(struct AdapterControlBlock)))==0)
		{
			printk("arcmsr%d: adapter probe: scsi_host_alloc error \n",arcmsr_adapterCnt);
	    	return -ENODEV;
		}
		if(!pci_set_dma_mask(pdev, DMA_64BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else if(!pci_set_dma_mask(pdev,	DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 32BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else 
		{
			printk("ARECA RAID ADAPTER%d: No suitable DMA available.\n",arcmsr_adapterCnt);
			return -ENOMEM;
		}
		if (pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: No 32BIT coherent	DMA adressing available.\n",arcmsr_adapterCnt);
			return -ENOMEM;
		}

		bus = pdev->bus->number;
	        dev_fun = pdev->devfn;
		acb=(struct AdapterControlBlock	*) host->hostdata;
		memset(acb,0,sizeof(struct AdapterControlBlock));
		acb->pdev=pdev;
		acb->host=host;
		host->max_sectors=ARCMSR_MAX_XFER_SECTORS;
		host->max_lun=ARCMSR_MAX_TARGETLUN;
		host->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
		host->max_cmd_len=16;	 /*this	is issue of 64bit LBA ,over 2T byte*/
		host->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
		host->can_queue=ARCMSR_MAX_FREECCB_NUM;	/* max simultaneous cmds */		
		host->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;	    
		host->this_id=ARCMSR_SCSI_INITIATOR_ID;	
		host->unique_id=(bus <<	8) | dev_fun;
		host->io_port=0;
		host->n_io_port=0;
		host->irq=pdev->irq;
		pci_set_master(pdev);
		if(arcmsr_initialize(acb,pdev))
		{
			printk("arcmsr%d: initialize got error \n",arcmsr_adapterCnt);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->acb[arcmsr_adapterCnt]=NULL;
	                scsi_remove_host(host);
			scsi_host_put(host);
			return -ENODEV;
		}
	        if (pci_request_regions(pdev, "arcmsr"))
		{
			printk("arcmsr%d: adapter probe: pci_request_regions failed \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}
		#ifdef CONFIG_SCSI_ARCMSR_MSI 
		if(pci_enable_msi(pdev)	== 0)
		{
			acb->acb_flags |= ACB_F_HAVE_MSI;
		}
		#endif
		if(request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb))
		{
			printk("arcmsr%d: request IRQ=%d failed	!\n",arcmsr_adapterCnt--,pdev->irq);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}
		arcmsr_iop_init(acb);
            	if(strncmp(acb->firm_version,"V1.42",5) >= 0)
            	host->max_sectors=ARCMSR_MAX_XFER_SECTORS_B;
		if(scsi_add_host(host, &pdev->dev))
		{
			printk("arcmsr%d: scsi_add_host got error \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return -ENODEV;
		}
	    	pHCBARC->adapterCnt=arcmsr_adapterCnt;
	   	pci_set_drvdata(pdev, host);
	    	scsi_scan_host(host);
		
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
			pci_enable_pcie_error_reporting(pdev);
	    	#endif
		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_remove(struct pci_dev *pdev)
	{
	    	struct Scsi_Host *host=pci_get_drvdata(pdev);
		struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
	    	struct AdapterControlBlock *acb=(struct AdapterControlBlock	*) host->hostdata;
		int i;

		#if ARCMSR_DEBUG
		printk("arcmsr_remove............................\n");
		#endif
		arcmsr_pcidev_disattach(acb);
		/*if this is last acb */
		for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			if(pHCBARC->acb[i])
			{ 
				return;/* this is not last adapter's release */
			}
		}
		unregister_chrdev(pHCBARC->arcmsr_major_number,	"arcmsr");
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
		unregister_reboot_notifier(&arcmsr_event_notifier);
	#endif
		return;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_scsi_host_template_init(struct scsi_host_template * host_template)
	{
		int	error;
		struct HCBARC *pHCBARC=	&arcmsr_host_control_block;

		#if ARCMSR_DEBUG
		printk("arcmsr_scsi_host_template_init..............\n");
		#endif
		/* 
		** register as a PCI hot-plug driver module 
		*/
		memset(pHCBARC,0,sizeof(struct HCBARC));
		error=pci_module_init(&arcmsr_pci_driver);
		if(pHCBARC->acb[0])
		{
			host_template->proc_name="arcmsr";
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
			register_reboot_notifier(&arcmsr_event_notifier);
		#endif
		}
		return(error);
	}
	/*
	************************************************************************
	************************************************************************
	*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
		static void arcmsr_shutdown(struct pci_dev *pdev)
		{
			struct Scsi_Host *host = pci_get_drvdata(pdev);
			struct AdapterControlBlock *acb	=
				(struct	AdapterControlBlock *)host->hostdata;
			
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
		return;
	}
    module_init(arcmsr_module_init);
    module_exit(arcmsr_module_exit);
#else
	/*
	*************************************************************************
	*************************************************************************
	*/
	static void arcmsr_internal_done(struct	scsi_cmnd *pcmd)
	{
		pcmd->SCp.Status++;
		return;
	}
	/*
	***************************************************************
	*		     arcmsr_schedule_command
	*	Description:	Process	a command from the SCSI	manager(A.P)
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
		timeout=jiffies	+ 60 * HZ; 
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
    void arcmsr_do_interrupt(int irq,void *dev_id)
	{
		struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
		struct AdapterControlBlock *acb;
		struct AdapterControlBlock *acbtmp;
		int i=0;

		#if ARCMSR_DEBUG
		printk("arcmsr_do_interrupt.................. \n");
		#endif
		acb=(struct AdapterControlBlock	*)dev_id;
		acbtmp=pHCBARC->acb[i];
		while((acb != acbtmp) && acbtmp	&& (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			acbtmp=pHCBARC->acb[i];
		}
		if(!acbtmp)
		{
		    #if	ARCMSR_DEBUG
			printk("arcmsr_do_interrupt: Invalid acb=0x%p \n",acb);
	    #endif
			return;
		}
		spin_lock_irq(&acb->isr_lock);
	arcmsr_interrupt(acb);
		spin_unlock_irq(&acb->isr_lock);
		return;
	}
	/*
	*********************************************************************
	*********************************************************************
	*/
    int	arcmsr_bios_param(Disk *disk,kdev_t dev,int geom[])
	{
		int heads,sectors,cylinders,total_capacity;

		#if ARCMSR_DEBUG
		printk("arcmsr_bios_param.................. \n");
		#endif
		total_capacity=disk->capacity;
		heads=64;
		sectors=32;
		cylinders=total_capacity / (heads * sectors);
		if(cylinders >=	1024)
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
		} const	arcmsr_devices[]={
			{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1110 }
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1120	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1130	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1160	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1170	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1210	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1220	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1230	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1260	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1270	}
			,{ PCI_VENDOR_ID_ARECA,PCI_DEVICE_ID_ARECA_1280	}
		};
		struct pci_dev *pdev=NULL;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
		struct Scsi_Host *host;
		static u_int8_t	i;
		#if ARCMSR_DEBUG
		printk("arcmsr_detect............................\n");
		#endif
		memset(pHCBARC,0,sizeof(struct HCBARC));
		for(i=0; i < (sizeof(arcmsr_devices)/sizeof(arcmsr_devices[0]))	; ++i)	 
		{
			pdev=NULL;
			while((pdev=pci_find_device(arcmsr_devices[i].vendor_id,arcmsr_devices[i].device_id,pdev)))
			{
				if((host=scsi_register(host_template,sizeof(struct AdapterControlBlock)))==0)
				{
					printk("arcmsr_detect: scsi_register error . . . . . . . . . . .\n");
					continue;
				}
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
				{
					if(pci_enable_device(pdev))
					{
						printk("arcmsr_detect: pci_enable_device ERROR..................................\n");
						scsi_unregister(host);
					continue;
					}
					if(!pci_set_dma_mask(pdev,(dma_addr_t)0xffffffffffffffffULL))/*64bit*/
					{
						printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
					}
					else if(pci_set_dma_mask(pdev,(dma_addr_t)0x00000000ffffffffULL))/*32bit*/
					{
						printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED	(ERROR)\n");
						scsi_unregister(host);
					continue;
					}
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
		host->max_cmd_len=16;		 /*this	is issue of 64bit LBA ,over 2T byte*/
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
				if(!arcmsr_initialize(acb,pdev))
				{
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					pci_set_drvdata(pdev,acb); /*set driver_data*/
				#endif
					pci_set_master(pdev);
					if(request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb))
					{
			printk("arcmsr_detect:	request_irq got	ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->acb[acb->adapter_index]=NULL;
						iounmap(acb->pmu);
					arcmsr_free_ccb_pool(acb);
					    scsi_unregister(host);
						goto next_areca;
					}
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
					if (pci_request_regions(pdev, "arcmsr"))
					{
			printk("arcmsr_detect:	pci_request_regions got	ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->acb[acb->adapter_index]=NULL;
						iounmap(acb->pmu);
					arcmsr_free_ccb_pool(acb);
					    scsi_unregister(host);
						goto next_areca;
					}
		            #endif
					arcmsr_iop_init(acb);/*	on kernel 2.4.21 driver's iop read/write must after request_irq	*/
				}
				else
				{
					printk("arcmsr:	arcmsr_initialize got ERROR...................\n");
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	void arcmsr_pci_unmap_dma(struct CommandControlBlock *ccb)
	{
		struct AdapterControlBlock *acb=ccb->acb;
		struct scsi_cmnd *pcmd=ccb->pcmd;

		if(pcmd->use_sg	!= 0) 
		{
			struct scatterlist *sl;

			sl = (struct scatterlist *)pcmd->request_buffer;
			pci_unmap_sg(acb->pdev,	sl, pcmd->use_sg, pcmd->sc_data_direction);
		} 
		else if(pcmd->request_bufflen != 0)
		{
			pci_unmap_single(acb->pdev,(dma_addr_t)(unsigned long)pcmd->SCp.ptr,pcmd->request_bufflen, pcmd->sc_data_direction);
		}
		return;
	}
#endif
/*
**********************************************************************************
**********************************************************************************
*/
static u32 arcmsr_disable_allintr(struct AdapterControlBlock *acb)
{
	struct MessageUnit __iomem *reg	= acb->pmu;
	u32 orig_mask =	readl(&reg->outbound_intmask)|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE;
	
	writel(orig_mask | ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE,
			&reg->outbound_intmask);
	return orig_mask;
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_enable_allintr(struct AdapterControlBlock *acb,
		u32 orig_mask)
{
	struct MessageUnit __iomem *reg	= acb->pmu;
	u32 mask;
	
	mask = orig_mask & ~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE	|
			     ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE);
	writel(mask, &reg->outbound_intmask);
	acb->outbound_int_enable = ~(orig_mask & mask) & 0x000000ff;
}
/*
**********************************************************************************
**********************************************************************************
*/
static void arcmsr_flush_adapter_cache(struct AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;

    #if	ARCMSR_DEBUG
    printk("arcmsr_flush_adapter_cache..............\n");
    #endif
	writel(ARCMSR_INBOUND_MESG0_FLUSH_CACHE,&reg->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(acb))
	{
		printk("arcmsr%d: wait 'flush adapter cache' timeout \n",acb->adapter_index);
	}
	return;
}
/*
**********************************************************************
**********************************************************************
*/
void arcmsr_ccb_complete(struct	CommandControlBlock *ccb,int stand_flag)
{
	struct AdapterControlBlock *acb=ccb->acb;
    struct scsi_cmnd *pcmd=ccb->pcmd;

	#if ARCMSR_DEBUG
	printk("arcmsr_ccb_complete:ccb=0x%p \n",ccb);
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
**	 if scsi error do auto request sense
**********************************************************************
*/
void arcmsr_report_sense_info(struct CommandControlBlock *ccb)
{
	struct scsi_cmnd *pcmd=ccb->pcmd;
	#if ARCMSR_DEBUG
    printk("arcmsr_report_sense_info...........\n");
	#endif
    pcmd->result=DID_OK	<< 16;
    if(pcmd->sense_buffer) 
	{
		memset(pcmd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE);
		memcpy(pcmd->sense_buffer,ccb->arcmsr_cdb.SenseData,SCSI_SENSE_BUFFERSIZE);
	    pcmd->sense_buffer[0] = (0x1 << 7 |	0x70); /* Valid,ErrorCode */
    }
    return;
}
/*
*********************************************************************
*********************************************************************
*/
void arcmsr_abort_allcmd(struct	AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;

	writel(ARCMSR_INBOUND_MESG0_ABORT_CMD,&reg->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(acb))
	{
	printk("arcmsr%d: wait 'abort all outstanding command' timeout \n",acb->adapter_index);
	}
	return;
}
/*
**********************************************************************
**********************************************************************
*/
static u_int8_t	arcmsr_wait_msgint_ready(struct	AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
	uint32_t Index;
	uint8_t	Retries=0x00;

	#if ARCMSR_DEBUG
	printk("arcmsr_wait_msgint_ready: ...............................\n");
	#endif
	if(acb->acb_flags &	ACB_F_FIRMWARE_TRAP)
	{
		writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT,&reg->outbound_intstatus);/*clear interrupt*/
		return 0x00;
	}
	do
	{
		for(Index=0; Index < 100; Index++)
		{
			if(readl(&reg->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT)
			{
				writel(ARCMSR_MU_OUTBOUND_MESSAGE0_INT,&reg->outbound_intstatus);/*clear interrupt*/
				return 0x00;
			}
			arc_mdelay_int(10);
		}/*max 1 seconds*/
	}while(Retries++ < 5);/*max 5 sec*/
	return 0xff;
}
/*
****************************************************************************
****************************************************************************
*/
static void arcmsr_iop_reset(struct AdapterControlBlock *acb)
{
	struct CommandControlBlock *ccb;
	uint32_t intmask_org;
	int i = 0;

	#if ARCMSR_DEBUG
	printk("arcmsr_iop_reset: reset iop controller......................................\n");
	#endif
	if (atomic_read(&acb->ccboutstandingcount) != 0) {
		#if ARCMSR_DEBUG
		printk("arcmsr_iop_reset: ccboutstandingcount=%d ...\n",atomic_read(&acb->ccboutstandingcount));
		#endif
		/* disable all outbound interrupt */
		intmask_org = arcmsr_disable_allintr(acb);
		/* talk to iop 331 outstanding command aborted */
		arcmsr_abort_allcmd(acb);
		/* clear all outbound posted Q */
		arcmsr_done4_abort_postqueue(acb);		
		for (i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) {			
			ccb = acb->pccb_pool[i];			
			if (ccb->startdone == ARCMSR_CCB_START) {				
				ccb->startdone = ARCMSR_CCB_ABORTED;			
			}		
		}		
		/* enable all outbound interrupt */		
		arcmsr_enable_allintr(acb, intmask_org);	
		}
	}

/*
**********************************************************************
**********************************************************************
*/
static int arcmsr_build_ccb(struct AdapterControlBlock *acb,struct CommandControlBlock *ccb,struct scsi_cmnd *pcmd)
{
    struct ARCMSR_CDB *arcmsr_cdb= &ccb->arcmsr_cdb;
	uint8_t	*psge=(uint8_t * )&arcmsr_cdb->u;
	uint32_t address_lo,address_hi;
	int arccdbsize=0x30,sgcount=0;
	
	#if ARCMSR_DEBUG
	printk("arcmsr_build_ccb........................... \n");
	#endif
    ccb->pcmd=pcmd;
	memset(arcmsr_cdb,0,sizeof(struct ARCMSR_CDB));
    arcmsr_cdb->Bus=0;
    arcmsr_cdb->TargetID=pcmd->device->id;
    arcmsr_cdb->LUN=pcmd->device->lun;
    arcmsr_cdb->Function=1;
	arcmsr_cdb->CdbLength=(uint8_t)pcmd->cmd_len;
    arcmsr_cdb->Context=(unsigned long)arcmsr_cdb;
    memcpy(arcmsr_cdb->Cdb, pcmd->cmnd,	pcmd->cmd_len);
	if(pcmd->use_sg) 
	{
		int length,i,cdb_sgcount=0;
		struct scatterlist *sl;

		/* Get Scatter Gather List from	scsiport. */
		sl=(struct scatterlist *) pcmd->request_buffer;
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
		sgcount=pci_map_sg(acb->pdev, sl, pcmd->use_sg,	pcmd->sc_data_direction);
    #else
	sgcount=pcmd->use_sg;
    #endif
		if(sgcount > ARCMSR_MAX_SG_ENTRIES)
		{
			return FAILED;
		}
		/* map stor port SG list to our	iop SG List.*/
		for(i=0;i<sgcount;i++) 
		{
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
			if(address_hi==0)
			{
				struct SG32ENTRY* pdma_sg=(struct SG32ENTRY*)psge;

				pdma_sg->address=address_lo;
				pdma_sg->length=length;
				psge +=	sizeof(struct SG32ENTRY);
				arccdbsize += sizeof(struct SG32ENTRY);
			}
			else
			{
		struct SG64ENTRY *pdma_sg=(struct SG64ENTRY *)psge;
			#if ARCMSR_DEBUG
				printk("arcmsr_build_ccb: ..........address_hi=0x%x....	\n",address_hi);
				#endif

				pdma_sg->addresshigh=address_hi;
				pdma_sg->address=address_lo;
				pdma_sg->length=length|IS_SG64_ADDR;
		psge +=sizeof(struct SG64ENTRY);
				arccdbsize +=sizeof(struct SG64ENTRY);
			}
			sl++;
			cdb_sgcount++;
		}
		arcmsr_cdb->sgcount=(uint8_t)cdb_sgcount;
		arcmsr_cdb->DataLength=pcmd->request_bufflen;
		if( arccdbsize > 256)
		{
			arcmsr_cdb->Flags|=ARCMSR_CDB_FLAG_SGL_BSIZE;
		}
	}
	else if(pcmd->request_bufflen) 
	{
    #if	LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
	dma_addr_t dma_addr;
		dma_addr=pci_map_single(acb->pdev, pcmd->request_buffer, pcmd->request_bufflen,	pcmd->sc_data_direction);
		pcmd->SCp.ptr =	(char *)(unsigned long)	dma_addr;
	address_lo=cpu_to_le32(dma_addr_lo32(dma_addr));
	    address_hi=cpu_to_le32(dma_addr_hi32(dma_addr));
    #else
	address_lo=cpu_to_le32(virt_to_bus(pcmd->request_buffer));/* Actual requested buffer */
	    address_hi=0;
    #endif
		if(address_hi==0)
		{
			struct SG32ENTRY* pdma_sg=(struct SG32ENTRY*)psge;
			pdma_sg->address=address_lo;
	    pdma_sg->length=pcmd->request_bufflen;
		}
		else
		{
			struct SG64ENTRY* pdma_sg=(struct SG64ENTRY*)psge;
			pdma_sg->addresshigh=address_hi;
			pdma_sg->address=address_lo;
	    pdma_sg->length=pcmd->request_bufflen|IS_SG64_ADDR;
		}
		arcmsr_cdb->sgcount=1;
		arcmsr_cdb->DataLength=pcmd->request_bufflen;
	}
	if (pcmd->cmnd[0]|WRITE_6 || pcmd->cmnd[0]|WRITE_10 || pcmd->cmnd[0]|WRITE_12 )	
	{
		arcmsr_cdb->Flags |= ARCMSR_CDB_FLAG_WRITE;
		ccb->ccb_flags |= CCB_FLAG_WRITE;
	}
	#if ARCMSR_DEBUG
	printk("arcmsr_build_ccb: ccb=0x%p cmd=0x%x xferlength=%d arccdbsize=%d	sgcount=%d\n",ccb,pcmd->cmnd[0],arcmsr_cdb->DataLength,arccdbsize,arcmsr_cdb->sgcount);
	#endif
    return SUCCESS;
}
/*
**************************************************************************
**	arcmsr_post_ccb	- Send a protocol specific ARC send postcard to	a AIOC .
**	handle:	Handle of registered ARC protocol driver
**	adapter_id: AIOC unique	identifier(integer)
**	pPOSTCARD_SEND:	Pointer	to ARC send postcard
**
**	This routine posts a ARC send postcard to the request post FIFO	of a
**	specific ARC adapter.
**************************************************************************
*/ 
static void arcmsr_post_ccb(struct AdapterControlBlock *acb,struct CommandControlBlock *ccb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
	uint32_t cdb_shifted_phyaddr=ccb->cdb_shifted_phyaddr;
	struct ARCMSR_CDB *arcmsr_cdb=(struct ARCMSR_CDB *)&ccb->arcmsr_cdb;

	#if ARCMSR_DEBUG
	printk("arcmsr_post_ccb: ccb=0x%p cdb_shifted_phyaddr=0x%x ccb->acb=0x%p \n",ccb,cdb_shifted_phyaddr,ccb->acb);
	#endif
    atomic_inc(&acb->ccboutstandingcount);
	ccb->startdone=ARCMSR_CCB_START;
	if(arcmsr_cdb->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE)
	{
	    writel(cdb_shifted_phyaddr|ARCMSR_CCBPOST_FLAG_SGL_BSIZE,&reg->inbound_queueport);
	}
	else
	{
	    writel(cdb_shifted_phyaddr,&reg->inbound_queueport);
	}
	return;
}
/*
**********************************************************************
**   Function: arcmsr_post_Qbuffer
**     Output: 
**********************************************************************
*/
static void arcmsr_post_Qbuffer(struct AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
	struct QBUFFER __iomem *pwbuffer=(struct QBUFFER __iomem *)&reg->message_wbuffer;
    uint8_t __iomem *iop_data=(uint8_t __iomem *)pwbuffer->data;
	int32_t	allxfer_len=0;

    if(acb->acb_flags &	ACB_F_MESSAGE_WQBUFFER_READED)
	{
		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
		while((acb->wqbuf_firstindex!=acb->wqbuf_lastindex) && (allxfer_len<124))
		{
			writeb(acb->wqbuffer[acb->wqbuf_firstindex], iop_data);
			acb->wqbuf_firstindex++;
			acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set	it to 0	*/
			iop_data++;
			allxfer_len++;
		}
	writel(allxfer_len, &pwbuffer->data_len);
		/*
		** push	inbound	doorbell and wait reply	at hwinterrupt routine for next	Qbuffer	post
		*/
		writel(ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK,&reg->inbound_doorbell);
	}
	return;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_stop_adapter_bgrb(struct AdapterControlBlock	*acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
    #if	ARCMSR_DEBUG
    printk("arcmsr_stop_adapter_bgrb..............\n");
    #endif

	acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
	writel(ARCMSR_INBOUND_MESG0_STOP_BGRB,&reg->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(acb))
	{
		printk("arcmsr%d: wait 'stop adapter background	rebulid' timeout \n",acb->adapter_index);
	}
	return;
}
/*
************************************************************************
************************************************************************
*/
static void arcmsr_free_ccb_pool(struct	AdapterControlBlock *acb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	{
	dma_free_coherent(&acb->pdev->dev,((sizeof(struct CommandControlBlock) * ARCMSR_MAX_FREECCB_NUM)+0x20),acb->dma_coherent,acb->dma_coherent_handle);
	}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	{
	    pci_free_consistent(acb->pdev, ((sizeof(struct CommandControlBlock)	* ARCMSR_MAX_FREECCB_NUM)+0x20), acb->dma_coherent, acb->dma_coherent_handle);
	}
#else
	{
	kfree(acb->dma_coherent);
	}
#endif
	return;
}
/*
**********************************************************************
**   Function:	arcmsr_interrupt
**     Output:	void
** DID_OK	   0x00	// NO error				   
** DID_NO_CONNECT  0x01	// Couldn't connect before timeout period  
** DID_BUS_BUSY	   0x02	// BUS stayed busy through time	out period 
** DID_TIME_OUT	   0x03	// TIMED OUT for other reason		   
** DID_BAD_TARGET  0x04	// BAD target.				   
** DID_ABORT	   0x05	// Told	to abort for some other	reason	   
** DID_PARITY	   0x06	// Parity error				   
** DID_ERROR	   0x07	// Internal error			   
** DID_RESET	   0x08	// Reset by somebody.			   
** DID_BAD_INTR	   0x09	// Got an interrupt we weren't expecting.  
** DID_PASSTHROUGH 0x0a	// Force command past mid-layer		   
** DID_SOFT_ERROR  0x0b	// The low level driver	just wish a retry  
** DRIVER_OK	   0x00	// Driver status	
**********************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    static irqreturn_t arcmsr_interrupt(struct AdapterControlBlock *acb)
#else
    static void	arcmsr_interrupt(struct	AdapterControlBlock *acb)
#endif
{
    struct MessageUnit __iomem *reg=acb->pmu;
	struct CommandControlBlock *ccb;
	uint32_t flag_ccb,outbound_intstatus,outbound_doorbell;
    #if	ARCMSR_DEBUG
    printk("arcmsr_interrupt...................................\n");
    #endif

	outbound_intstatus=readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	writel(outbound_intstatus,&reg->outbound_intstatus);/*clear interrupt*/
	if(outbound_intstatus &	ARCMSR_MU_OUTBOUND_DOORBELL_INT)
	{
		#if ARCMSR_DEBUG
		printk("arcmsr_interrupt:..........ARCMSR_MU_OUTBOUND_DOORBELL_INT \n");
		#endif

		outbound_doorbell=readl(&reg->outbound_doorbell);
		writel(outbound_doorbell,&reg->outbound_doorbell);/*clear interrupt */
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK)
		{
			struct QBUFFER __iomem *prbuffer=(struct QBUFFER __iomem *)&reg->message_rbuffer;
			uint8_t	__iomem	*iop_data=(uint8_t __iomem *)prbuffer->data;
			int32_t	my_empty_len,iop_len,rqbuf_firstindex,rqbuf_lastindex;
	
		/*check	this iop data if overflow my rqbuffer*/
			rqbuf_lastindex=acb->rqbuf_lastindex;
			rqbuf_firstindex=acb->rqbuf_firstindex;
			iop_len=prbuffer->data_len;
	    my_empty_len=(rqbuf_firstindex-rqbuf_lastindex-1)&(ARCMSR_MAX_QBUFFER-1);
			if(my_empty_len>=iop_len)
			{
				while(iop_len >	0)
				{
					acb->rqbuffer[acb->rqbuf_lastindex] = readb(iop_data);
					acb->rqbuf_lastindex++;
					acb->rqbuf_lastindex %=	ARCMSR_MAX_QBUFFER;/*if	last index number set it to 0 */
					iop_data++;
					iop_len--;
				}
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&reg->inbound_doorbell);/*signature, let IOP331 know data has	been readed */
			}
			else
			{
				acb->acb_flags|=ACB_F_IOPDATA_OVERFLOW;
			}
		}
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK)
		{
			acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_READED;

		if(acb->wqbuf_firstindex!=acb->wqbuf_lastindex)
			{
				struct QBUFFER __iomem *pwbuffer=(struct QBUFFER __iomem *)&reg->message_wbuffer;
				uint8_t	__iomem	*  iop_data=(uint8_t __iomem *)pwbuffer->data;
				int32_t	allxfer_len=0;

		acb->acb_flags &= (~ACB_F_MESSAGE_WQBUFFER_READED);
				while((acb->wqbuf_firstindex!=acb->wqbuf_lastindex) && (allxfer_len<124))
				{
		    writeb(acb->wqbuffer[acb->wqbuf_firstindex], iop_data);
					acb->wqbuf_firstindex++;
					acb->wqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set	it to 0	*/
					iop_data++;
					allxfer_len++;
				}
				writel(allxfer_len, &pwbuffer->data_len);
				/*
				** push	inbound	doorbell tell iop driver data write ok and wait	reply on next hwinterrupt for next Qbuffer post
				*/
				writel(ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK,&reg->inbound_doorbell);
			}
			if(acb->wqbuf_firstindex==acb->wqbuf_lastindex)
			{
				acb->acb_flags |= ACB_F_MESSAGE_WQBUFFER_CLEARED;
			}
		}
	}
	if(outbound_intstatus &	ARCMSR_MU_OUTBOUND_POSTQUEUE_INT)
	{
		int id,lun;
		/*
		*****************************************************************************
		**		 areca cdb command done
		*****************************************************************************
		*/
		while(1)
		{
			if((flag_ccb=readl(&reg->outbound_queueport)) == 0xFFFFFFFF)
			{
				break;/*chip FIFO no ccb for completion	already*/
			}
			/* check if command done with no error*/
			ccb=(struct CommandControlBlock	*)(acb->vir2phy_offset+(flag_ccb << 5));/*frame	must be	32 bytes aligned*/
			if((ccb->acb!=acb) || (ccb->startdone!=ARCMSR_CCB_START))
			{
				if(ccb->startdone==ARCMSR_CCB_ABORTED)
				{
					struct scsi_cmnd *abortcmd=ccb->pcmd;
					printk("arcmsr%d: ccb='0x%p' isr command abort successfully \n",acb->adapter_index,ccb);
					if(abortcmd)
					{
						id=abortcmd->device->id;
						lun=abortcmd->device->lun;
						if(acb->dev_aborts[id][lun] >= 4)
						{
							acb->devstate[id][lun]=ARECA_RAID_GONE;
							abortcmd->result = DID_NO_CONNECT << 16;
						}
						abortcmd->result |= DID_ABORT << 16;
						arcmsr_ccb_complete(ccb,1);
					}
					continue;
				}
				printk("arcmsr%d: isr get an illegal ccb command done acb='0x%p' ccb='0x%p' ccbacb='0x%p' startdone=0x%x ccboutstandingcount=%d	\n",acb->adapter_index,acb,ccb,ccb->acb,ccb->startdone,atomic_read(&acb->ccboutstandingcount));
				continue;
			}
			id=ccb->pcmd->device->id;
			lun=ccb->pcmd->device->lun;
			if((flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)==0)
			{
				#if ARCMSR_DEBUG
				printk("ccb=0x%p   scsi	cmd=0x%x................... GOOD ..............done\n",ccb,ccb->pcmd->cmnd[0]);
				#endif

				if(acb->devstate[id][lun]==ARECA_RAID_GONE)
				{
		    			acb->devstate[id][lun]=ARECA_RAID_GOOD;
					acb->dev_aborts[id][lun]=0;
				}
				ccb->pcmd->result=DID_OK << 16;
				arcmsr_ccb_complete(ccb,1);
			} 
			else 
			{   
				switch(ccb->arcmsr_cdb.DeviceStatus)
				{
				case ARCMSR_DEV_SELECT_TIMEOUT:
					{
				#if ARCMSR_DEBUG
				printk("ccb=0x%p ......ARCMSR_DEV_SELECT_TIMEOUT\n",ccb);
				#endif
						acb->devstate[id][lun]=ARECA_RAID_GONE;
						ccb->pcmd->result=DID_NO_CONNECT << 16;
						arcmsr_ccb_complete(ccb,1);
					}
					break;
				case ARCMSR_DEV_ABORTED:
				case ARCMSR_DEV_INIT_FAIL:
					{
				#if ARCMSR_DEBUG
				printk("ccb=0x%p .....ARCMSR_DEV_INIT_FAIL\n",ccb);
				#endif
					acb->devstate[id][lun]=ARECA_RAID_GONE;
						ccb->pcmd->result=DID_BAD_TARGET << 16;
						arcmsr_ccb_complete(ccb,1);
					}
					break;
				case SCSISTAT_CHECK_CONDITION:
					{
				#if ARCMSR_DEBUG
				printk("ccb=0x%p .....SCSISTAT_CHECK_CONDITION\n",ccb);
				#endif
				acb->devstate[id][lun]=ARECA_RAID_GOOD;
				arcmsr_report_sense_info(ccb);
				arcmsr_ccb_complete(ccb,1);
					}
					break;
				default:
					/* error occur Q all error ccb to errorccbpending Q*/
				printk("arcmsr%d: scsi id=%d lun=%d isr	get command error done,	but got	unknow DeviceStatus=0x%x \n",acb->adapter_index,id,lun,ccb->arcmsr_cdb.DeviceStatus);
				acb->devstate[id][lun]=ARECA_RAID_GONE;
				ccb->pcmd->result=DID_BAD_TARGET << 16;/*unknow	error or crc error just	for retry*/
				arcmsr_ccb_complete(ccb,1);
				break;
				}
			}
		}	/*drain	reply FIFO*/
	}
	if(!(outbound_intstatus	& ARCMSR_MU_OUTBOUND_HANDLE_INT))
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
	if(acb)
	{
		/* stop	adapter	background rebuild */
		if(acb->acb_flags & ACB_F_MSG_START_BGRB)
		{
			acb->acb_flags &= ~ACB_F_MSG_START_BGRB;
			arcmsr_stop_adapter_bgrb(acb);
			arcmsr_flush_adapter_cache(acb);
		}
	}
}
/*
************************************************************************
************************************************************************
*/
static int arcmsr_iop_message_xfer(struct AdapterControlBlock *acb, struct scsi_cmnd *cmd)
{
	struct MessageUnit __iomem *reg	= acb->pmu;
	struct CMD_MESSAGE_FIELD *pcmdmessagefld;
	int retvalue = 0, transfer_len = 0;
	char *buffer;
	uint32_t controlcode = (uint32_t ) cmd->cmnd[5]	<< 24 |	
						(uint32_t ) cmd->cmnd[6] << 16 | 
						(uint32_t ) cmd->cmnd[7] << 8  |
						(uint32_t ) cmd->cmnd[8]; 
					/* 4 bytes: Areca io control code */
	if (cmd->use_sg) {
		struct scatterlist *sg = (struct scatterlist *)cmd->request_buffer;

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		buffer = kmap_atomic(sg->page, KM_IRQ0)	+ sg->offset;
		#else
		buffer = sg->address;
		#endif
		if (cmd->use_sg	> 1) {
			retvalue = ARCMSR_MESSAGE_FAIL;
			goto message_out;
		}
		transfer_len +=	sg->length;
	} else {
		buffer = cmd->request_buffer;
		transfer_len = cmd->request_bufflen;
	}
	if (transfer_len > sizeof(struct CMD_MESSAGE_FIELD)) {
		retvalue = ARCMSR_MESSAGE_FAIL;
		goto message_out;
	}
	pcmdmessagefld = (struct CMD_MESSAGE_FIELD *) buffer;
	switch(controlcode) {
	case ARCMSR_MESSAGE_READ_RQBUFFER: {
			unsigned long *ver_addr;
			dma_addr_t buf_handle;
			uint8_t	*pQbuffer, *ptmpQbuffer;
			int32_t	allxfer_len = 0;
	
			ver_addr = pci_alloc_consistent(acb->pdev, 1032, &buf_handle);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				goto message_out;
			}
			ptmpQbuffer = (uint8_t *) ver_addr;
			while ((acb->rqbuf_firstindex != acb->rqbuf_lastindex)
				&& (allxfer_len	< 1031)) {
				pQbuffer = &acb->rqbuffer[acb->rqbuf_firstindex];
				memcpy(ptmpQbuffer, pQbuffer, 1);
				acb->rqbuf_firstindex++;
				acb->rqbuf_firstindex %= ARCMSR_MAX_QBUFFER;
				ptmpQbuffer++;
				allxfer_len++;
			}
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				struct QBUFFER __iomem * prbuffer = (struct QBUFFER __iomem *)
							&reg->message_rbuffer;
				uint8_t	__iomem	* iop_data = (uint8_t __iomem *)prbuffer->data;
				int32_t	iop_len;
	
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				iop_len	= readl(&prbuffer->data_len);
				while (iop_len > 0) {
					acb->rqbuffer[acb->rqbuf_lastindex] = readb(iop_data);
					acb->rqbuf_lastindex++;
					acb->rqbuf_lastindex %=	ARCMSR_MAX_QBUFFER;
					iop_data++;
					iop_len--;
				}
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,
						&reg->inbound_doorbell);
			}
			memcpy(pcmdmessagefld->messagedatabuffer,
				(uint8_t *)ver_addr, allxfer_len);
			pcmdmessagefld->cmdmessage.Length = allxfer_len;
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
			pci_free_consistent(acb->pdev, 1032, ver_addr, buf_handle);
		}
		break;
	case ARCMSR_MESSAGE_WRITE_WQBUFFER: {
			unsigned long *ver_addr;
			dma_addr_t buf_handle;
			int32_t	my_empty_len, user_len,	wqbuf_firstindex, wqbuf_lastindex;
			uint8_t	*pQbuffer, *ptmpuserbuffer;
	
			ver_addr = pci_alloc_consistent(acb->pdev, 1032, &buf_handle);
			if (!ver_addr) {
				retvalue = ARCMSR_MESSAGE_FAIL;
				goto message_out;
			}
			ptmpuserbuffer = (uint8_t *)ver_addr;
			user_len = pcmdmessagefld->cmdmessage.Length;
			memcpy(ptmpuserbuffer, pcmdmessagefld->messagedatabuffer, user_len);
			wqbuf_lastindex	= acb->wqbuf_lastindex;
			wqbuf_firstindex = acb->wqbuf_firstindex;
			if (wqbuf_lastindex != wqbuf_firstindex) {
				arcmsr_post_Qbuffer(acb);
				/* has error report sensedata */
				cmd->sense_buffer[0] = (0x1 << 7 | 0x70); /* Valid,ErrorCode */
				cmd->sense_buffer[2] = ILLEGAL_REQUEST;	/* FileMark,EndOfMedia,IncorrectLength,Reserved,SenseKey */
				cmd->sense_buffer[7] = 0x0A; /*	AdditionalSenseLength */
				cmd->sense_buffer[12] =	0x20; /* AdditionalSenseCode */
				retvalue = ARCMSR_MESSAGE_FAIL;
			} else {
				my_empty_len = (wqbuf_firstindex-wqbuf_lastindex - 1)
						&(ARCMSR_MAX_QBUFFER - 1);
				if (my_empty_len >= user_len) {
					while (user_len	> 0) {
						pQbuffer =
						&acb->wqbuffer[acb->wqbuf_lastindex];
						memcpy(pQbuffer, ptmpuserbuffer, 1);
						acb->wqbuf_lastindex++;
						acb->wqbuf_lastindex %=	ARCMSR_MAX_QBUFFER;
						ptmpuserbuffer++;
						user_len--;
					}
					if (acb->acb_flags & ACB_F_MESSAGE_WQBUFFER_CLEARED) {
						acb->acb_flags &=
							~ACB_F_MESSAGE_WQBUFFER_CLEARED;
						arcmsr_post_Qbuffer(acb);
					}
				} else {
					/* has error report sensedata */
					cmd->sense_buffer[0] = (0x1 << 7 | 0x70); /* Valid,ErrorCode */
					cmd->sense_buffer[2] = ILLEGAL_REQUEST;	/* FileMark,EndOfMedia,IncorrectLength,Reserved,SenseKey */
					cmd->sense_buffer[7] = 0x0A; /*	AdditionalSenseLength */
					cmd->sense_buffer[12] =	0x20; /* AdditionalSenseCode */
					retvalue = ARCMSR_MESSAGE_FAIL;
				}
			}
			pci_free_consistent(acb->pdev, 1032, ver_addr, buf_handle);
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_RQBUFFER: {
			uint8_t	*pQbuffer = acb->rqbuffer;
	
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,
					&reg->inbound_doorbell);
			}
			acb->acb_flags |= ACB_F_MESSAGE_RQBUFFER_CLEARED;
			acb->rqbuf_firstindex =	0;
			acb->rqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_WQBUFFER: {
			uint8_t	*pQbuffer = acb->wqbuffer;
 
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK
						, &reg->inbound_doorbell);
			}
			acb->acb_flags |=
				(ACB_F_MESSAGE_WQBUFFER_CLEARED	| ACB_F_MESSAGE_WQBUFFER_READED);
			acb->wqbuf_firstindex =	0;
			acb->wqbuf_lastindex = 0;
			memset(pQbuffer, 0, ARCMSR_MAX_QBUFFER);
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_CLEAR_ALLQBUFFER: {
			uint8_t	*pQbuffer;
 
			if (acb->acb_flags & ACB_F_IOPDATA_OVERFLOW) {
				acb->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
				writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK
						, &reg->inbound_doorbell);
			}
			acb->acb_flags |=
				(ACB_F_MESSAGE_WQBUFFER_CLEARED
				| ACB_F_MESSAGE_RQBUFFER_CLEARED
				| ACB_F_MESSAGE_WQBUFFER_READED);
			acb->rqbuf_firstindex =	0;
			acb->rqbuf_lastindex = 0;
			acb->wqbuf_firstindex =	0;
			acb->wqbuf_lastindex = 0;
			pQbuffer = acb->rqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			pQbuffer = acb->wqbuffer;
			memset(pQbuffer, 0, sizeof (struct QBUFFER));
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_RETURN_CODE_3F: {
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_3F;
		}
		break;
	case ARCMSR_MESSAGE_SAY_HELLO: {
			int8_t * hello_string =	"Hello!	I am ARCMSR";
	
			memcpy(pcmdmessagefld->messagedatabuffer, hello_string
				, (int16_t)strlen(hello_string));
			pcmdmessagefld->cmdmessage.ReturnCode =	ARCMSR_MESSAGE_RETURNCODE_OK;
		}
		break;
	case ARCMSR_MESSAGE_SAY_GOODBYE:
		arcmsr_iop_parking(acb);
		break;
	case ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE:
		arcmsr_flush_adapter_cache(acb);
		break;
	default:
		retvalue = ARCMSR_MESSAGE_FAIL;
	}
message_out:
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	if (cmd->use_sg) {
		struct scatterlist *sg;

		sg = (struct scatterlist *) cmd->request_buffer;
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

	if (!list_empty(head)) 
	{
		ccb = list_entry(head->next, struct CommandControlBlock, list);
		list_del(head->next);
	}
	return(ccb);
}
/*
***********************************************************************
***********************************************************************
*/
int arcmsr_queue_command(struct	scsi_cmnd *cmd,void (* done)(struct scsi_cmnd *))
{
    struct Scsi_Host *host = cmd->device->host;
	struct AdapterControlBlock *acb=(struct	AdapterControlBlock *) host->hostdata;
    struct CommandControlBlock *ccb;
	int target=cmd->device->id;
	int lun=cmd->device->lun;
	uint8_t	scsicmd	= cmd->cmnd[0];

    #if	ARCMSR_DEBUG
    printk("arcmsr_queue_command:Cmd=%2x,TargetId=%d,Lun=%d \n",scsicmd,target,lun);
    #endif
	cmd->scsi_done=done;
	cmd->host_scribble=NULL;
	cmd->result=0;
    /* 
	*************************************************
	** enlarge the timeout duration	of each	scsi command 
	** it could aviod the vibration	factor 
	** with	sata disk on some bad machine 
	*************************************************
	*/
	if(scsicmd==SYNCHRONIZE_CACHE) 
	{
		if(acb->devstate[target][lun]==ARECA_RAID_GONE)
		{
	    cmd->result=(DID_NO_CONNECT	<< 16);
		}
		cmd->scsi_done(cmd);
	return(0);
	}
	if (acb->acb_flags & ACB_F_BUS_RESET) {
		printk(KERN_NOTICE "arcmsr%d: bus reset"
			" and return busy \n"
			, acb->host->host_no);
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	if(target == 16) {
		/* virtual device for iop message transfer */
		switch(scsicmd)	{
		case INQUIRY: {
				unsigned char inqdata[36];
				char *buffer;

				if(lun != 0) {
					cmd->result = (DID_TIME_OUT << 16);
					cmd->scsi_done(cmd);
					return 0;
				}
				inqdata[0] = TYPE_PROCESSOR; 
				/* Periph Qualifier & Periph Dev Type */
				inqdata[1] = 0;	
				/* rem media bit & Dev Type Modifier */
				inqdata[2] = 0;	
				/* ISO,ECMA,& ANSI versions */
				inqdata[4] = 31; 
				/* length of additional	data */
				strncpy(&inqdata[8], "Areca   ", 8); 
				/* Vendor Identification */
				strncpy(&inqdata[16], "RAID controller ", 16);
				/* Product Identification */
				strncpy(&inqdata[32], "R001", 4); /* Product Revision */
				if (cmd->use_sg) {
					struct scatterlist *sg;

					sg = (struct scatterlist *) cmd->request_buffer;
					#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
					buffer = kmap_atomic(sg->page, KM_IRQ0)	+ sg->offset;
					#else
					buffer = sg->address;
					#endif
				} else {
					buffer = cmd->request_buffer;
				}
				memcpy(buffer, inqdata,	sizeof(inqdata));
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				if (cmd->use_sg) {
					struct scatterlist *sg;

					sg = (struct scatterlist *) cmd->request_buffer;
					kunmap_atomic(buffer - sg->offset, KM_IRQ0);
				}
				#endif
				cmd->scsi_done(cmd);
				return 0;
			}
		case WRITE_BUFFER:
		case READ_BUFFER: {
				if (arcmsr_iop_message_xfer(acb, cmd)) {
					cmd->result = (DID_ERROR << 16);
				}
				cmd->scsi_done(cmd);
				return 0;
			}
		default:
			cmd->scsi_done(cmd);
			return 0;
		}
	}
	if(acb->devstate[target][lun]==ARECA_RAID_GONE)
	{
		uint8_t	block_cmd;

	block_cmd=scsicmd & 0x0f;
		if(block_cmd==0x08 || block_cmd==0x0a)
		{
			printk("arcmsr%d: block	'read/write' command with gone raid volume Cmd=%2x,TargetId=%d,Lun=%d \n",acb->adapter_index,scsicmd,target,lun);
			cmd->result=(DID_NO_CONNECT << 16);
			cmd->scsi_done(cmd);
			return(0);
		}
	}
	if (atomic_read(&acb->ccboutstandingcount) >=
			ARCMSR_MAX_OUTSTANDING_CMD)
		return SCSI_MLQUEUE_HOST_BUSY;

    ccb=arcmsr_get_freeccb(acb);
	if (!ccb)
	{
		return SCSI_MLQUEUE_HOST_BUSY;
	}
	if(arcmsr_build_ccb(acb, ccb, cmd)==FAILED)
	{
		cmd->result=(DID_ERROR << 16) |	(RESERVATION_CONFLICT << 1);
		cmd->scsi_done(cmd);
		return(0);
	}
	arcmsr_post_ccb(acb, ccb);
	return 0;
}
/*
**********************************************************************
**  get	firmware miscellaneous data
**********************************************************************
*/
static void arcmsr_get_firmware_spec(struct AdapterControlBlock	*acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
    char *acb_firm_model=acb->firm_model;
    char *acb_firm_version=acb->firm_version;
    char *iop_firm_model=(char *) (&reg->message_rwbuffer[15]);	  /*firm_model,15,60-67*/
    char *iop_firm_version=(char *) (&reg->message_rwbuffer[17]); /*firm_version,17,68-83*/
	int count;

    writel(ARCMSR_INBOUND_MESG0_GET_CONFIG,&reg->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(acb))
	{
		printk("arcmsr%d: wait 'get adapter firmware miscellaneous data' timeout \n",acb->adapter_index);
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
	printk("ARECA RAID ADAPTER%d: FIRMWARE VERSION %s \n",acb->adapter_index,acb->firm_version);
    acb->firm_request_len=readl(&reg->message_rwbuffer[1]);   /*firm_request_len,1,04-07*/
	acb->firm_numbers_queue=readl(&reg->message_rwbuffer[2]); /*firm_numbers_queue,2,08-11*/
	acb->firm_sdram_size=readl(&reg->message_rwbuffer[3]);	  /*firm_sdram_size,3,12-15*/
    acb->firm_ide_channels=readl(&reg->message_rwbuffer[4]);  /*firm_ide_channels,4,16-19*/
	return;
}
/*
**********************************************************************
**  start background rebulid
**********************************************************************
*/
static void arcmsr_start_adapter_bgrb(struct AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
	#if ARCMSR_DEBUG
	printk("arcmsr_start_adapter_bgrb.................................. \n");
	#endif

	acb->acb_flags |= ACB_F_MSG_START_BGRB;
    writel(ARCMSR_INBOUND_MESG0_START_BGRB,&reg->inbound_msgaddr0);
	if(arcmsr_wait_msgint_ready(acb))
	{
		printk("arcmsr%d: wait 'start adapter background rebulid' timeout \n",acb->adapter_index);
	}
	return;
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_polling_ccbdone(struct AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
	struct CommandControlBlock *ccb;
	uint32_t flag_ccb,outbound_intstatus,poll_ccb_done=0,poll_count=0;
	int id,lun;

	#if ARCMSR_DEBUG
	printk("arcmsr_polling_ccbdone.................................. \n");
	#endif
       polling_ccb_retry:
	poll_count++;
	outbound_intstatus=readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
	writel(outbound_intstatus,&reg->outbound_intstatus);/*clear interrupt*/
	while(1)
	{
		if((flag_ccb=readl(&reg->outbound_queueport))==0xFFFFFFFF)
		{
			if(poll_ccb_done)
			{
				break;/*chip FIFO no ccb for completion already*/
			}
			else
			{
				arc_mdelay(25);
				if(poll_count >100)
				{
		    			 break;
				}
				goto polling_ccb_retry;
			}
		}
		/* check if command done with no error*/
		ccb=(struct CommandControlBlock *)(acb->vir2phy_offset+(flag_ccb << 5));/*frame must be	32 bytes aligned*/
		if((ccb->acb!=acb) || (ccb->startdone!=ARCMSR_CCB_START))
		{
			if(ccb->startdone==ARCMSR_CCB_ABORTED)
			{
				id=ccb->pcmd->device->id;
				lun=ccb->pcmd->device->lun;
			printk("arcmsr%d: scsi id=%d lun=%d ccb='0x%p' poll command abort successfully \n",acb->adapter_index,id,lun,ccb);
				if(acb->dev_aborts[id][lun] >= 4)
				{
					acb->devstate[id][lun]=ARECA_RAID_GONE;
					ccb->pcmd->result = DID_NO_CONNECT << 16;
				}
				ccb->pcmd->result |= DID_ABORT << 16;
				arcmsr_ccb_complete(ccb,1);
				poll_ccb_done=1;
				continue;
			}
		    printk("arcmsr%d: polling get an illegal ccb command done ccb='0x%p' ccboutstandingcount=%d	\n",acb->adapter_index,ccb,atomic_read(&acb->ccboutstandingcount));
			continue;
		}
		id=ccb->pcmd->device->id;
		lun=ccb->pcmd->device->lun;
		if((flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)==0)
		{
			if(acb->devstate[id][lun]==ARECA_RAID_GONE)
			{
		acb->devstate[id][lun]=ARECA_RAID_GOOD;
			}
			ccb->pcmd->result=DID_OK << 16;
			arcmsr_ccb_complete(ccb,1);
		} 
		else 
		{   
			switch(ccb->arcmsr_cdb.DeviceStatus)
			{
			case ARCMSR_DEV_SELECT_TIMEOUT:
				{
					acb->devstate[id][lun]=ARECA_RAID_GONE;
					ccb->pcmd->result=DID_NO_CONNECT<< 16;
					arcmsr_ccb_complete(ccb,1);
				}
				break;
			case ARCMSR_DEV_ABORTED:
			case ARCMSR_DEV_INIT_FAIL:
				{
				    acb->devstate[id][lun]=ARECA_RAID_GONE;
					ccb->pcmd->result=DID_BAD_TARGET << 16;
					arcmsr_ccb_complete(ccb,1);
				}
				break;
			case SCSISTAT_CHECK_CONDITION:
				{
				    acb->devstate[id][lun]=ARECA_RAID_GOOD;
				arcmsr_report_sense_info(ccb);
					arcmsr_ccb_complete(ccb,1);
				}
				break;
			default:
				/* error occur Q all error ccb to errorccbpending Q*/
				printk("arcmsr%d: scsi id=%d lun=%d polling and	getting	command	error done, but	got unknow DeviceStatus=0x%x \n",acb->adapter_index,id,lun,ccb->arcmsr_cdb.DeviceStatus);
				acb->devstate[id][lun]=ARECA_RAID_GONE;
				ccb->pcmd->result=DID_BAD_TARGET << 16;/*unknow	error or crc error just	for retry*/
				arcmsr_ccb_complete(ccb,1);
				break;
			}
		}
	}	/*drain	reply FIFO*/
	return;
}
/*
**********************************************************************
**********************************************************************
*/
static void arcmsr_done4_abort_postqueue(struct AdapterControlBlock *acb)
{
	int i=0,found=0;	
	int id, lun;	
	uint32_t flag_ccb,outbound_intstatus;	
	struct MessageUnit __iomem *reg = acb->pmu;	
	struct CommandControlBlock *ccb;	/*clear and abort all outbound posted Q*/
	
	while(((flag_ccb=readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < 256)){			
		ccb = (struct CommandControlBlock *)(acb->vir2phy_offset + (flag_ccb << 5));
		if(ccb){
			if ((ccb->acb != acb)||(ccb->startdone != ARCMSR_CCB_START)){
				printk(KERN_NOTICE 
					"arcmsr%d: polling get an illegal ccb"
					" command done ccb='0x%p'"		
					"ccboutstandingcount=%d \n", 			
					acb->host->host_no, 
					ccb, 
					atomic_read(&acb->ccboutstandingcount));	
				continue;
			}

			id = ccb->pcmd->device->id;
			lun = ccb->pcmd->device->lun;	
			if (!(flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR))	{
				if (acb->devstate[id][lun] == ARECA_RAID_GONE)
					acb->devstate[id][lun] = ARECA_RAID_GOOD;
				ccb->pcmd->result = DID_OK << 16;
				arcmsr_ccb_complete(ccb, 1);
			} 
			else {
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
						arcmsr_report_sense_info(ccb);
						arcmsr_ccb_complete(ccb, 1);
				}
				break;

				default:
						printk(KERN_NOTICE
							"arcmsr%d: scsi id=%d lun=%d"
							" polling and getting command error done"
							"but got unknown DeviceStatus = 0x%x \n"	
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
			found=1;
		}
	}
	if(found){
		outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
		writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
	}
	return;
}
/*
**********************************************************************
**  start background rebulid
**********************************************************************
*/
static void arcmsr_iop_init(struct AdapterControlBlock *acb)
{
    struct MessageUnit __iomem *reg=acb->pmu;
    uint32_t intmask_org,outbound_doorbell,firmware_state=0;

	#if ARCMSR_DEBUG
	printk("arcmsr_iop_init.................................. \n");
	#endif
	do
	{
	firmware_state=readl(&reg->outbound_msgaddr1);
	}while((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK)==0);
    /* disable all outbound interrupt */
    intmask_org=arcmsr_disable_allintr(acb);
 	arcmsr_get_firmware_spec(acb);
	/*start	background rebuild*/
	arcmsr_start_adapter_bgrb(acb);
	/* clear Qbuffer if door bell ringed */
	outbound_doorbell=readl(&reg->outbound_doorbell);
	writel(outbound_doorbell,&reg->outbound_doorbell);/*clear interrupt */
    	writel(ARCMSR_INBOUND_DRIVER_DATA_READ_OK,&reg->inbound_doorbell);
	/* enable outbound Post Queue,outbound doorbell Interrupt */
	arcmsr_enable_allintr(acb,intmask_org);
	acb->acb_flags |=ACB_F_IOP_INITED;
	return;
}
/*
****************************************************************************
****************************************************************************
*/
int arcmsr_bus_reset(struct scsi_cmnd *cmd)
{
	struct AdapterControlBlock *acb;
    int	retry=0;

	acb=(struct AdapterControlBlock	*) cmd->device->host->hostdata;
	printk("arcmsr%d: bus reset ..... \n",acb->adapter_index);
	acb->num_resets++;
	if((acb->num_resets > 1) && (acb->num_aborts > 10))
	{
		acb->acb_flags |= ACB_F_FIRMWARE_TRAP;
		return SUCCESS;
	}
	acb->acb_flags |= ACB_F_BUS_RESET;
	while(atomic_read(&acb->ccboutstandingcount)!=0	&& retry < 4)
	{
	arcmsr_interrupt(acb);
		retry++;
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
	
	ccb->startdone = ARCMSR_CCB_ABORTED;
	intmask	= arcmsr_disable_allintr(acb);
	arcmsr_polling_ccbdone(acb);
	arcmsr_enable_allintr(acb, intmask);
}
/*
*****************************************************************************************
*****************************************************************************************
*/
int arcmsr_abort(struct	scsi_cmnd *cmd)
{
	struct AdapterControlBlock *acb	=(struct	AdapterControlBlock *)cmd->device->host->hostdata;
	int i =	0,id,lun;
	
	id=cmd->device->id;
	lun=cmd->device->lun;
	printk(KERN_NOTICE
		"arcmsr%d: abort device command of scsi	id=%d lun=%d \n", 
		acb->host->host_no, id, lun);
	acb->num_aborts++;
	acb->dev_aborts[id][lun]++;
	if(acb->dev_aborts[id][lun]>1)
	{
		acb->devstate[id][lun] = ARECA_RAID_GONE;
		return SUCCESS;
	}
	/*
	************************************************
	** the all interrupt service routine is locked
	** we need to handle it	as soon as possible and	exit
	************************************************
	*/
	if (!atomic_read(&acb->ccboutstandingcount))
		return SUCCESS;
	
	for (i = 0; i <	ARCMSR_MAX_FREECCB_NUM;	i++) {
		struct CommandControlBlock *ccb	= acb->pccb_pool[i];
		if (ccb->startdone == ARCMSR_CCB_START && ccb->pcmd == cmd) {
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
	struct AdapterControlBlock *acb	=
		(struct	AdapterControlBlock *) host->hostdata;
	static char buf[256];
	char *type;
	int raid6 = 1;
	
	switch (acb->pdev->device) {
	case PCI_DEVICE_ID_ARECA_1110:
	case PCI_DEVICE_ID_ARECA_1210:
		raid6 =	0;
		/*FALLTHRU*/
	case PCI_DEVICE_ID_ARECA_1120:
	case PCI_DEVICE_ID_ARECA_1130:
	case PCI_DEVICE_ID_ARECA_1160:
	case PCI_DEVICE_ID_ARECA_1170:
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
************************************************************************
************************************************************************
*/
static int arcmsr_initialize(struct AdapterControlBlock *acb,struct pci_dev *pdev)
{
	struct MessageUnit __iomem *reg;
	uint32_t page_base,page_offset,mem_base_start,ccb_phyaddr_hi32;
	dma_addr_t dma_coherent_handle,dma_addr;
	struct HCBARC *pHCBARC=&arcmsr_host_control_block;
	uint8_t	pcicmd;
    	void *dma_coherent;
	void __iomem *page_remapped;
	int i,j;
    	struct CommandControlBlock *pccb_tmp;

	#if ARCMSR_DEBUG
	printk("arcmsr_initialize....................................\n");
	#endif
	/* Enable Busmaster/Mem	*/
	pci_read_config_byte(pdev,PCI_COMMAND,&pcicmd);
	pci_write_config_byte(pdev,PCI_COMMAND,pcicmd|PCI_COMMAND_INVALIDATE|PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY); 
	mem_base_start=(uint32_t)arcget_pcicfg_base(pdev,0);
	page_base=mem_base_start & PAGE_MASK;
	page_offset=mem_base_start - page_base;
	page_remapped=ioremap(page_base,page_offset + 0x1FFF);
	if(!page_remapped)
	{
		printk("arcmsr%d: memory mapping region	fail \n",arcmsr_adapterCnt);
		return -ENXIO;
	}
	acb->pmu=(struct MessageUnit __iomem *)(page_remapped+page_offset);
    	acb->acb_flags |= (ACB_F_MESSAGE_WQBUFFER_CLEARED|ACB_F_MESSAGE_RQBUFFER_CLEARED|ACB_F_MESSAGE_WQBUFFER_READED);
	acb->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
	acb->irq=pdev->irq;
    	INIT_LIST_HEAD(&acb->ccb_free_list);
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	dma_coherent = dma_alloc_coherent(&pdev->dev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) + 0x20, &dma_coherent_handle,	GFP_KERNEL);
	#else
	dma_coherent = pci_alloc_consistent(pdev, ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock) +	0x20, &dma_coherent_handle);
	#endif
	if (!dma_coherent)
	{
		printk("arcmsr%d: dma_alloc_coherent got error \n",arcmsr_adapterCnt);
	return -ENOMEM;
	}
	acb->dma_coherent=dma_coherent;
	acb->dma_coherent_handle=dma_coherent_handle;
	memset(dma_coherent, 0,	ARCMSR_MAX_FREECCB_NUM * sizeof(struct CommandControlBlock)+0x20);
	if(((unsigned long)dma_coherent	& 0x1F)!=0) /*ccb address must 32 (0x20) boundary*/
	{
		dma_coherent=dma_coherent+(0x20-((unsigned long)dma_coherent & 0x1F));
		dma_coherent_handle=dma_coherent_handle+(0x20-((unsigned long)dma_coherent_handle & 0x1F));
	}
	dma_addr=dma_coherent_handle;
	pccb_tmp=(struct CommandControlBlock *)dma_coherent;
	for(i =	0; i < ARCMSR_MAX_FREECCB_NUM; i++) 
	{
		pccb_tmp->cdb_shifted_phyaddr=dma_addr >> 5;
		pccb_tmp->acb=acb;
		acb->pccb_pool[i]=pccb_tmp;
		list_add_tail(&pccb_tmp->list, &acb->ccb_free_list);
		dma_addr=dma_addr+sizeof(struct	CommandControlBlock);
		pccb_tmp++;
	}
	acb->vir2phy_offset=(unsigned long)pccb_tmp-(unsigned long)dma_addr;
	for(i=0;i<ARCMSR_MAX_TARGETID;i++)
	{
		for(j=0;j<ARCMSR_MAX_TARGETLUN;j++)
		{
			acb->devstate[i][j]=ARECA_RAID_GONE;
		}
	}
	reg = acb->pmu;
    /* disable iop all outbound	interrupt */
    arcmsr_disable_allintr(acb);
	/*
	********************************************************************
	** here	we need	to tell	iop 331	our pccb_tmp.HighPart 
	** if pccb_tmp.HighPart	is not zero
	********************************************************************
	*/
	ccb_phyaddr_hi32=(uint32_t) ((dma_coherent_handle>>16)>>16);
	if(ccb_phyaddr_hi32!=0)
	{
	writel(ARCMSR_SIGNATURE_SET_CONFIG,&reg->message_rwbuffer[0]);
	writel(ccb_phyaddr_hi32,&reg->message_rwbuffer[1]);
		writel(ARCMSR_INBOUND_MESG0_SET_CONFIG,&reg->inbound_msgaddr0);
		if(arcmsr_wait_msgint_ready(acb))
		{
			printk("arcmsr%d: 'set ccb high	part physical address' timeout \n",arcmsr_adapterCnt);
		}
	}
	acb->adapter_index=arcmsr_adapterCnt;
	pHCBARC->acb[arcmsr_adapterCnt]=acb;
	arcmsr_adapterCnt++;
    return(0);
}
/*
*********************************************************************
*********************************************************************
*/
static int arcmsr_set_info(char	*buffer,int length)
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
static void arcmsr_pcidev_disattach(struct AdapterControlBlock *acb)
{
    struct pci_dev *pdev;
	struct CommandControlBlock *ccb;
	struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
	struct Scsi_Host *host;
	int i=0,poll_count=0,have_msi=0;
    #if	ARCMSR_DEBUG
    printk("arcmsr_pcidev_disattach.................. \n");
    #endif
	/* disable iop all outbound	interrupt */
    arcmsr_disable_allintr(acb);
	arcmsr_stop_adapter_bgrb(acb);
	arcmsr_flush_adapter_cache(acb);
	acb->acb_flags |= ACB_F_SCSISTOPADAPTER;
	acb->acb_flags &= ~ACB_F_IOP_INITED;
	if(atomic_read(&acb->ccboutstandingcount)!=0)
	{  
	while(atomic_read(&acb->ccboutstandingcount)!=0	&& (poll_count < 6))
		{
	    arcmsr_interrupt(acb);
	    poll_count++;
		}
		if(atomic_read(&acb->ccboutstandingcount)!=0)
	    {  
			arcmsr_abort_allcmd(acb);
			arcmsr_done4_abort_postqueue(acb);
			for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
			{
				ccb=acb->pccb_pool[i];
				if(ccb->startdone==ARCMSR_CCB_START)
				{
					ccb->startdone=ARCMSR_CCB_ABORTED;
				}
			}
		}
	}
	if((acb->acb_flags & ACB_F_HAVE_MSI) !=	0)
	{
		have_msi=1;
	}
	host=acb->host;
    pdev=acb->pdev;
	iounmap(acb->pmu);
    arcmsr_free_ccb_pool(acb);
    pHCBARC->acb[acb->adapter_index]=NULL; /* clear record */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    scsi_remove_host(host);
	scsi_host_put(host);
#else
	scsi_unregister(host);
#endif
	free_irq(pdev->irq,acb);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,30)
	#ifdef CONFIG_PCI_MSI
		if(have_msi==1)
		{
			pci_disable_msi(pdev);
		}
	#endif
	pci_release_regions(pdev);
    	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
#endif
	return;
}
/*
***************************************************************
***************************************************************
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
	static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf)
	{
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
		struct Scsi_Host *host;
		int i;

		#if ARCMSR_DEBUG
		printk("arcmsr_halt_notify............................1	\n");
		#endif
		if((event !=SYS_RESTART) && (event !=SYS_HALT) && (event !=SYS_POWER_OFF))
		{
			return NOTIFY_DONE;
		}
		for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			acb=pHCBARC->acb[i];
			if(acb==NULL) 
			{
				continue;
			}
			/* Flush cache to disk */
			/* Free	irq,otherwise extra interrupt is generated	 */
			/* Issue a blocking(interrupts disabled) command to the	card */
			host=acb->host;
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
#undef SPRINTF
#define	SPRINTF(args...) pos +=sprintf(pos,## args)
#define	YESNO(YN)\
if(YN) SPRINTF(" Yes ");\
else SPRINTF(" No ")
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    int	arcmsr_proc_info(struct	Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
#else
    int	arcmsr_proc_info(char *	buffer,char ** start,off_t offset,int length,int hostno,int inout)
#endif
{
	uint8_t	 i;
    char * pos=buffer;
    struct AdapterControlBlock *acb;
	struct HCBARC *pHCBARC=	&arcmsr_host_control_block;

	#if ARCMSR_DEBUG
	printk("arcmsr_proc_info.............\n");
	#endif
    if(inout) 
	{
	    return(arcmsr_set_info(buffer,length));
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		acb=pHCBARC->acb[i];
	    if(!acb) 
			continue;
		SPRINTF("ARECA SATA RAID Mass Storage Host Adadpter \n");
		SPRINTF("Driver	Version	%s ",ARCMSR_DRIVER_VERSION);
		SPRINTF("IRQ%d \n",acb->pdev->irq);
		SPRINTF("===========================\n"); 
	}
	*start=buffer +	offset;
	if(pos - buffer	< offset)
	{
	    return 0;
	}
    else if(pos	- buffer - offset < length)
	{
	    return (pos	- buffer - offset);
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
	struct AdapterControlBlock *acb;
	struct HCBARC *pHCBARC=	&arcmsr_host_control_block;
    	uint8_t match=0xff,i;

	#if ARCMSR_DEBUG
	printk("arcmsr_release...........................\n");
	#endif
	if(!host)
	{
	return -ENXIO;
	}
	acb=(struct AdapterControlBlock *)host->hostdata;
	if(!acb)
	{
	return -ENXIO;
	}
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if(pHCBARC->acb[i]==acb) 
		{  
			match=i;
		}
	}
	if(match==0xff)
	{
		return -ENODEV;
	}
    arcmsr_pcidev_disattach(acb);
	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if(pHCBARC->acb[i])
		{ 
			return(0);
		}
	}
	unregister_chrdev(pHCBARC->arcmsr_major_number,	"arcmsr");
	return(0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)

      	static pci_ers_result_t arcmsr_pci_slot_reset(struct pci_dev *pdev)
      	{
        	struct Scsi_Host *host;
		struct AdapterControlBlock *acb;
		struct HCBARC *pHCBARC= &arcmsr_host_control_block;
        	uint8_t bus,dev_fun;
		#if ARCMSR_DEBUG
		printk("arcmsr_probe............................\n");
		#endif
		if(pci_enable_device(pdev))
		{
			printk("arcmsr%d: adapter probe: pci_enable_device error \n",arcmsr_adapterCnt);
			return PCI_ERS_RESULT_DISCONNECT;
		}
		if((host=scsi_host_alloc(&arcmsr_scsi_host_template,sizeof(struct AdapterControlBlock)))==0)
		{
			printk("arcmsr%d: adapter probe: scsi_host_alloc error \n",arcmsr_adapterCnt);
            		return PCI_ERS_RESULT_DISCONNECT;
		}
		if(!pci_set_dma_mask(pdev, DMA_64BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else if(!pci_set_dma_mask(pdev, DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: 32BITS PCI BUS DMA ADDRESSING SUPPORTED\n",arcmsr_adapterCnt);
		} 
		else 
		{
			printk("ARECA RAID ADAPTER%d: No suitable DMA available.\n",arcmsr_adapterCnt);
			return PCI_ERS_RESULT_DISCONNECT;
		}
		if (pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK)) 
		{
			printk("ARECA RAID ADAPTER%d: No 32BIT coherent DMA adressing available.\n",arcmsr_adapterCnt);
			return PCI_ERS_RESULT_DISCONNECT;
		}
		bus = pdev->bus->number;
	    	dev_fun = pdev->devfn;
		acb=(struct AdapterControlBlock *) host->hostdata;
		memset(acb,0,sizeof(struct AdapterControlBlock));
		acb->pdev=pdev;
		acb->host=host;
		host->max_sectors=ARCMSR_MAX_XFER_SECTORS;
		host->max_lun=ARCMSR_MAX_TARGETLUN;
		host->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
		host->max_cmd_len=16;    /*this is issue of 64bit LBA ,over 2T byte*/
		host->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
		host->can_queue=ARCMSR_MAX_FREECCB_NUM; /* max simultaneous cmds */             
		host->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;            
		host->this_id=ARCMSR_SCSI_INITIATOR_ID; 
		host->unique_id=(bus << 8) | dev_fun;
		host->io_port=0;
		host->n_io_port=0;
		host->irq=pdev->irq;
		pci_set_master(pdev);
 		if(arcmsr_initialize(acb,pdev))
		{
			printk("arcmsr%d: initialize got error \n",arcmsr_adapterCnt);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->acb[arcmsr_adapterCnt]=NULL;
            		scsi_remove_host(host);
			scsi_host_put(host);
			return PCI_ERS_RESULT_DISCONNECT;
		}
	    if (pci_request_regions(pdev, "arcmsr"))
		{
      		printk("arcmsr%d: adapter probe: pci_request_regions failed \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
  			return PCI_ERS_RESULT_DISCONNECT;
		}
		#ifdef CONFIG_SCSI_ARCMSR_MSI 
			if(pci_enable_msi(pdev) == 0)
			{
				acb->acb_flags |= ACB_F_HAVE_MSI;
			}
		#endif
		if(request_irq(pdev->irq,arcmsr_do_interrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",acb))
		{
  			printk("arcmsr%d: request IRQ=%d failed !\n",arcmsr_adapterCnt--,pdev->irq);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
  			return PCI_ERS_RESULT_DISCONNECT;
		}
		arcmsr_iop_init(acb);
		if(strncmp(acb->firm_version,"V1.42",5) >= 0)
              	host->max_sectors= ARCMSR_MAX_XFER_SECTORS_B;
		if(scsi_add_host(host, &pdev->dev))
		{
            		printk("arcmsr%d: scsi_add_host got error \n",arcmsr_adapterCnt--);
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			arcmsr_pcidev_disattach(acb);
			return PCI_ERS_RESULT_DISCONNECT;
		}
 		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		pci_set_drvdata(pdev, host);
		scsi_scan_host(host);
 		return PCI_ERS_RESULT_RECOVERED;

	}
      
      	static void arcmsr_pci_ers_need_reset_forepart(struct pci_dev *pdev)
     	{
     		struct Scsi_Host *host=pci_get_drvdata(pdev);
	    	struct AdapterControlBlock *acb=(struct AdapterControlBlock *) host->hostdata;
		struct MessageUnit __iomem *reg = acb->pmu;	
		struct CommandControlBlock *ccb;	/*clear and abort all outbound posted Q*/
		int i=0,found=0;	
		int id, lun;	
		uint32_t flag_ccb,outbound_intstatus;		
		
		while(((flag_ccb=readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < 256)){			
			ccb = (struct CommandControlBlock *)(acb->vir2phy_offset + (flag_ccb << 5));
			if(ccb){
				if ((ccb->acb != acb)||(ccb->startdone != ARCMSR_CCB_START)){
					printk(KERN_NOTICE 
						"arcmsr%d: polling get an illegal ccb"
						" command done ccb='0x%p'"		
						"ccboutstandingcount=%d \n", 			
						acb->host->host_no, 
						ccb, 
						atomic_read(&acb->ccboutstandingcount));	
					continue;
				}

				id = ccb->pcmd->device->id;
				lun = ccb->pcmd->device->lun;	
				if (!(flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR))	{
					if (acb->devstate[id][lun] == ARECA_RAID_GONE)
						acb->devstate[id][lun] = ARECA_RAID_GOOD;
					ccb->pcmd->result = DID_OK << 16;
					arcmsr_ccb_complete(ccb, 1);
				} 
				else {
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
							arcmsr_report_sense_info(ccb);
							arcmsr_ccb_complete(ccb, 1);
					}
					break;
	
					default:
							printk(KERN_NOTICE
								"arcmsr%d: scsi id=%d lun=%d"
								" polling and getting command error done"
								"but got unknown DeviceStatus = 0x%x \n"	
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
				found=1;
			}
		}
		if(found){
			outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
			writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
		}
		return;
      	}   

      
	static void arcmsr_pci_ers_disconnect_forepart(struct pci_dev *pdev)
      	{
     		struct Scsi_Host *host=pci_get_drvdata(pdev);
	    	struct AdapterControlBlock *acb=(struct AdapterControlBlock *) host->hostdata;
		struct MessageUnit __iomem *reg = acb->pmu;	
		struct CommandControlBlock *ccb;	/*clear and abort all outbound posted Q*/
		int i=0,found=0;	
		int id, lun;	
		uint32_t flag_ccb,outbound_intstatus;		
		
		while(((flag_ccb=readl(&reg->outbound_queueport)) != 0xFFFFFFFF) && (i++ < 256)){			
			ccb = (struct CommandControlBlock *)(acb->vir2phy_offset + (flag_ccb << 5));
			if(ccb){
				if ((ccb->acb != acb)||(ccb->startdone != ARCMSR_CCB_START)){
					printk(KERN_NOTICE 
						"arcmsr%d: polling get an illegal ccb"
						" command done ccb='0x%p'"		
						"ccboutstandingcount=%d \n", 			
						acb->host->host_no, 
						ccb, 
						atomic_read(&acb->ccboutstandingcount));	
					continue;
				}

				id = ccb->pcmd->device->id;
				lun = ccb->pcmd->device->lun;	
				if (!(flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR))	{
					if (acb->devstate[id][lun] == ARECA_RAID_GONE)
						acb->devstate[id][lun] = ARECA_RAID_GOOD;
					ccb->pcmd->result = DID_OK << 16;
					arcmsr_ccb_complete(ccb, 1);
				} 
				else {
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
							arcmsr_report_sense_info(ccb);
							arcmsr_ccb_complete(ccb, 1);
					}
					break;
	
					default:
							printk(KERN_NOTICE
								"arcmsr%d: scsi id=%d lun=%d"
								" polling and getting command error done"
								"but got unknown DeviceStatus = 0x%x \n"	
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
				found=1;
			}
		}
		if(found){
			outbound_intstatus = readl(&reg->outbound_intstatus) & acb->outbound_int_enable;
			writel(outbound_intstatus, &reg->outbound_intstatus);/*clear interrupt*/
		}
		return;
}
      
      	static pci_ers_result_t arcmsr_pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
      	{
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
