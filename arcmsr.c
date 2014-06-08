/*
******************************************************************************************
**        O.S   : Linux
**   FILE NAME  : arcmsr.c
**        BY    : Erich Chen   
**   Description: SCSI RAID Device Driver for 
**                ARECA (ARC1110/1120/1130/1160//1210/1220/1230/1260) SATA RAID HOST Adapter
**                ARCMSR RAID Host adapter[RAID controller:INTEL 331(PCI-X) 341(PCI-EXPRESS) chip set]
******************************************************************************************
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
******************************************************************************************
**********************************************************************
**   value=call_usermodehelper (argv [0],argv,envp);
**********************************************************************
*/
#define ARCMSR_DEBUG             1
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
	#include <linux/mc146818rtc.h>
	#include <linux/reboot.h>
	#include <linux/sched.h>
	#include <linux/init.h>

	# if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
		# include <linux/spinlock.h>
	# else
		# include <asm/spinlock.h>
	# endif /* 2,3,30 */

	#include "/usr/src/linux/drivers/scsi/scsi.h"
	#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,5,0)
		#include "/usr/src/linux/include/scsi/scsi_host.h"
	#else
		#include "/usr/src/linux/drivers/scsi/hosts.h"
	#endif
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
		#include "/usr/src/linux/drivers/scsi/constants.h"
		#include "/usr/src/linux/drivers/scsi/sd.h"
	#endif
	#include "arcmsr.h"
#endif
/*
**********************************************************************************
** 
**********************************************************************************
*/
static u_int8_t arcmsr_adapterCnt=0;
static struct  _HCBARC arcmsr_host_control_block;
static PHCBARC pHCBARC=&arcmsr_host_control_block;
/*
**********************************************************************************
** notifier block to get a notify on system shutdown/halt/reboot
**********************************************************************************
*/
static int arcmsr_fops_ioctl(struct inode *inode, struct file *filep, unsigned int ioctl_cmd, unsigned long arg);
static int arcmsr_fops_close(struct inode *inode, struct file *filep);
static int arcmsr_fops_open(struct inode *inode, struct file *filep);
static int arcmsr_halt_notify(struct notifier_block *nb,unsigned long event,void *buf);
static void arcmsr_free_pci_pool(PACB pACB);
void arcmsr_pcidev_disattach(PACB pACB);
void arcmsr_plunge_kernelthread(void);
BOOLEAN arcmsr_wait_msgint_ready(PACB pACB);
VOID arcmsr_iop_init(PACB pACB);
const char *arcmsr_info(struct Scsi_Host *);
int arcmsr_queue_command(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *));
int arcmsr_release(struct Scsi_Host *);
int arcmsr_iop_ioctlcmd(PACB pACB,int ioctl_cmd,void *arg);
int arcmsr_initialize(PACB pACB,struct pci_dev *pPCI_DEV);
int arcmsr_cmd_abort(Scsi_Cmnd *);
int arcmsr_bus_reset(Scsi_Cmnd *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	#define arcmsr_detect NULL
    int arcmsr_proc_info(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
    int arcmsr_bios_param(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int *info);
    static irqreturn_t arcmsr_HwInterrupt(PACB pACB);
    static int arcmsr_device_probe(struct pci_dev *pPCI_DEV,const struct pci_device_id *id);
	static void	arcmsr_device_remove(struct pci_dev *pPCI_DEV);
#else
    int arcmsr_schedule_command(PSCSICMD pcmd);
    int arcmsr_detect(Scsi_Host_Template *);
    int arcmsr_proc_info(char * buffer,char ** start,off_t offset,int length,int hostno,int inout);
	int arcmsr_bios_param(Disk *, kdev_t , int []);
	int arcmsr_ioctl(Scsi_Device *dev,int ioctl_cmd,void *arg);
    void arcmsr_HwInterrupt(PACB pACB);
#endif
#ifndef BCD_TO_BIN
    #define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)
#endif
/*
**********************************************************************************
**
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
************************************************************************
**            arcmsr
**           arcmsr_release
**
************************************************************************
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	static Scsi_Host_Template arcmsr_scsi_host_template = {
		    .module                 = THIS_MODULE,
			.proc_name	            = "arcmsr",
    		.proc_info	            = arcmsr_proc_info,
    		.name		            = "ARCMSR(ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Adapter" ARCMSR_DRIVER_VERSION,  /* *name */
    		.release	            = arcmsr_release,
			.info                   = arcmsr_info,
    		.queuecommand	        = arcmsr_queue_command,
			.eh_strategy_handler    = NULL,	
			.eh_abort_handler       = arcmsr_cmd_abort,
			.eh_device_reset_handler= NULL,	
			.eh_bus_reset_handler   = arcmsr_bus_reset,
			.eh_host_reset_handler  = NULL,	
     		.bios_param	            = arcmsr_bios_param,	
    		.can_queue	            = ARCMSR_MAX_OUTSTANDING_CMD,
    		.this_id	            = ARCMSR_SCSI_INITIATOR_ID,
    		.sg_tablesize	        = ARCMSR_MAX_SG_ENTRIES, 
			.max_sectors    	    = ARCMSR_MAX_XFER_SECTORS, 
    		.cmd_per_lun	        = ARCMSR_MAX_CMD_PERLUN,	
     		.unchecked_isa_dma      = 0,
    		.use_clustering	        = DISABLE_CLUSTERING,
	};
#else
	static Scsi_Host_Template driver_template = {
			.proc_name	            = "arcmsr",
    		.proc_info	            = arcmsr_proc_info,
    		.name		            = "ARCMSR(ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Adapter" ARCMSR_DRIVER_VERSION,  /* *name */
    		.detect		            = arcmsr_detect,
    		.release	            = arcmsr_release,
			.info                   = arcmsr_info,
			.ioctl                  = arcmsr_ioctl,
			.command                = arcmsr_schedule_command,
    		.queuecommand	        = arcmsr_queue_command,
			.eh_strategy_handler    = NULL,	
			.eh_abort_handler       = arcmsr_cmd_abort,
			.eh_device_reset_handler= NULL,	
			.eh_bus_reset_handler   = arcmsr_bus_reset,
			.eh_host_reset_handler  = NULL,	
     		.bios_param	            = arcmsr_bios_param,	
    		.can_queue	            = ARCMSR_MAX_OUTSTANDING_CMD,
    		.this_id	            = ARCMSR_SCSI_INITIATOR_ID,
    		.sg_tablesize	        = ARCMSR_MAX_SG_ENTRIES, 
			.max_sectors    	    = ARCMSR_MAX_XFER_SECTORS, 
    		.cmd_per_lun	        = ARCMSR_MAX_CMD_PERLUN,	
     		.unchecked_isa_dma      = 0,
    		.use_clustering	        = DISABLE_CLUSTERING,
     };
	 #include "/usr/src/linux/drivers/scsi/scsi_module.c"
#endif
/*
**********************************************************************************
**
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
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1210, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1220, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1230, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
		{.vendor=PCIVendorIDARECA, .device=PCIDeviceIDARC1260, .subvendor=PCI_ANY_ID, .subdevice=PCI_ANY_ID,},
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
    static irqreturn_t arcmsr_doInterrupt(int irq,void *dev_id,struct pt_regs *regs)
	{
		irqreturn_t handle_state;
		PACB pACB,pACBtmp;
		int i=0;

		#if ARCMSR_DEBUG0
		printk("arcmsr_doInterrupt.................. 1\n");
		#endif

        pACB=(PACB)dev_id;
		pACBtmp=pHCBARC->pACB[i];
 		while((pACB != pACBtmp) && pACBtmp && (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			pACBtmp=pHCBARC->pACB[i];
		}
		if(!pACBtmp)
		{
		    #if ARCMSR_DEBUG0
			printk("arcmsr_doInterrupt: Invalid pACB=0x%p \n",pACB);
            #endif
			return IRQ_NONE;
		}
		spin_lock_irq(&pACB->isr_lockunlock);
        handle_state=arcmsr_HwInterrupt(pACB);
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

		#if ARCMSR_DEBUG0
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
	static int arcmsr_device_probe(struct pci_dev *pPCI_DEV,const struct pci_device_id *id)
	{
		PSH psh;
		PACB pACB;

		#if ARCMSR_DEBUG0
		printk("arcmsr_device_probe............................\n");
		#endif
		if(pci_enable_device(pPCI_DEV))
		{
			printk("arcmsr_device_probe: pci_enable_device error .....................\n");
			return -ENODEV;
		}
		/* allocate scsi host information (includes out adapter) scsi_host_alloc==scsi_register */
		if((psh=scsi_host_alloc(&arcmsr_scsi_host_template,sizeof(struct _ACB)))==0)
		{
			printk("arcmsr_device_probe: scsi_host_alloc error .....................\n");
            return -ENODEV;
		}
		if(!pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0xffffffffffffffffULL))/*64bit*/
		{
			printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
		}
		else if(pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0x00000000ffffffffULL))/*32bit*/
		{
			printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED (ERROR)\n");
			scsi_host_put(psh);
            return -ENODEV;
		}
		pACB=(PACB) psh->hostdata;
		memset(pACB,0,sizeof(struct _ACB));
		spin_lock_init(&pACB->isr_lockunlock);
		spin_lock_init(&pACB->wait2go_lockunlock);
		spin_lock_init(&pACB->qbuffer_lockunlock);
		spin_lock_init(&pACB->ccb_doneindex_lockunlock);
		spin_lock_init(&pACB->ccb_startindex_lockunlock);
		pACB->pPCI_DEV=pPCI_DEV;
		pACB->pScsiHost=psh;
		psh->max_sectors=ARCMSR_MAX_XFER_SECTORS;
		psh->max_lun=ARCMSR_MAX_TARGETLUN;
		psh->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
		psh->max_cmd_len=16;    /*this is issue of 64bit LBA ,over 2T byte*/
		psh->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
		psh->can_queue=ARCMSR_MAX_OUTSTANDING_CMD; /* max simultaneous cmds */             
		psh->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;            
		psh->this_id=ARCMSR_SCSI_INITIATOR_ID; 
		psh->io_port=0;
		psh->n_io_port=0;
		psh->irq=pPCI_DEV->irq;
		pci_set_master(pPCI_DEV);
 		if(arcmsr_initialize(pACB,pPCI_DEV))
		{
			printk("arcmsr: arcmsr_initialize got error...................\n");
			scsi_host_put(psh);
			return -ENODEV;
		}
	    if (pci_request_regions(pPCI_DEV, "arcmsr"))
		{
    		printk("arcmsr_device_probe: pci_request_regions.............!\n");
			arcmsr_adapterCnt--;
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->pACB[pACB->adapter_index]=NULL;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(psh);
  			return -ENODEV;
		}
		if(request_irq(pPCI_DEV->irq,arcmsr_doInterrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",pACB))
		{
    		printk("arcmsr: pACB=0x%p register IRQ=%d error!\n",pACB,pPCI_DEV->irq);
			arcmsr_adapterCnt--;
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->pACB[pACB->adapter_index]=NULL;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(psh);
  			return -ENODEV;
		}
		arcmsr_iop_init(pACB);
		if(scsi_add_host(psh, &pPCI_DEV->dev))
		{
            printk("arcmsr: scsi_add_host got error...................\n");
			arcmsr_adapterCnt--;
			pHCBARC->adapterCnt=arcmsr_adapterCnt;
			pHCBARC->pACB[pACB->adapter_index]=NULL;
			arcmsr_pcidev_disattach(pACB);
			scsi_host_put(psh);
			return -ENODEV;
		}
 		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		pci_set_drvdata(pPCI_DEV, psh);
		scsi_scan_host(psh);
 		return 0;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static void arcmsr_device_remove(struct pci_dev *pPCI_DEV)
	{
	    PSH psh=pci_get_drvdata(pPCI_DEV);
	    PACB pACB=(PACB) psh->hostdata;
		int i;

		#if ARCMSR_DEBUG0
		printk("arcmsr_device_remove............................\n");
		#endif
  		/* Flush cache to disk */
		/* Free irq,otherwise extra interrupt is generated	 */
		/* Issue a blocking(interrupts disabled) command to the card */
        arcmsr_pcidev_disattach(pACB);
        scsi_remove_host(psh);
	    scsi_host_put(psh);
	    pci_set_drvdata(pPCI_DEV, NULL);
		/*if this is last pACB */
		for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
		{
			if(pHCBARC->pACB[i]!=NULL)
			{ 
				return;/* this is not last adapter's release */
			}
		}
		/* kill kernel thread */
		if(pHCBARC->do_dpcQ_task)
		{
			DECLARE_MUTEX_LOCKED(sem);
			/*
			** do_dpcQ_task is locking
			*/
			pHCBARC->mutex_semaphore= &sem;
			pHCBARC->hcb_flags |=HCB_F_STOP_THREAD;
			up(pHCBARC->do_dpcQ_semaphore);/* signal to kernel thread do_dpcQ: "stop thread" */
			down(pHCBARC->mutex_semaphore);
			pHCBARC->mutex_semaphore=NULL;
		}
		unregister_chrdev(pHCBARC->arcmsr_major_number, "arcmsr");
	    unregister_reboot_notifier(&arcmsr_event_notifier);
		return;
	}
	/*
	************************************************************************
	************************************************************************
	*/
	static int arcmsr_scsi_host_template_init(Scsi_Host_Template * psht)
	{
  		psht->proc_name="arcmsr";
        memset(pHCBARC,0,sizeof(struct _HCBARC));
		arcmsr_plunge_kernelthread();
		register_reboot_notifier(&arcmsr_event_notifier);
        spin_lock_init(&pHCBARC->dpc_lockunlock);
		pHCBARC->arcmsr_major_number=register_chrdev(0, "arcmsr", &arcmsr_file_operations);
		printk("arcmsr device major number %d \n",pHCBARC->arcmsr_major_number);
	    return(pci_module_init(&arcmsr_pci_driver));
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
	*********************************************************************
	*********************************************************************
	*/
    void arcmsr_doInterrupt(int irq,void *dev_id,struct pt_regs *regs)
	{
		PACB pACB,pACBtmp;
		int i=0;

		#if ARCMSR_DEBUG0
		printk("arcmsr_doInterrupt.................. 1\n");
		#endif
		pACB=(PACB)dev_id;
		pACBtmp=pHCBARC->pACB[i];
 		while((pACB != pACBtmp) && pACBtmp && (i <ARCMSR_MAX_ADAPTER) )
		{
			i++;
			pACBtmp=pHCBARC->pACB[i];
		}
		if(!pACBtmp)
		{
		    #if ARCMSR_DEBUG0
			printk("arcmsr_doInterrupt: Invalid pACB=0x%p \n",pACB);
            #endif
			return;
		}
		spin_lock_irq(&pACB->isr_lockunlock);
        arcmsr_HwInterrupt(pACB);
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

		#if ARCMSR_DEBUG0
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
	int arcmsr_detect(Scsi_Host_Template *psht)
	{
		struct
		{
			u_int16_t   vendor_id;
			u_int16_t   device_id;
		} const arcmsr_devices[]={
	  		{ PCIVendorIDARECA,PCIDeviceIDARC1110 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1120 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1130 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1160 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1210 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1220 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1230 }
			,{ PCIVendorIDARECA,PCIDeviceIDARC1260 }
		};
		struct pci_dev *pPCI_DEV=NULL;
		PACB pACB;
		PSH psh;
		static u_int8_t i;
		#if ARCMSR_DEBUG0
		printk("arcmsr_detect............................\n");
		#endif
		memset(pHCBARC,0,sizeof(struct _HCBARC));
		spin_lock_init(&pHCBARC->dpc_lockunlock);
  		for(i=0; i < (sizeof(arcmsr_devices)/sizeof(arcmsr_devices[0])) ; ++i)   
		{
			pPCI_DEV=NULL;
			while((pPCI_DEV=pci_find_device(arcmsr_devices[i].vendor_id,arcmsr_devices[i].device_id,pPCI_DEV)))
			{
				if((psh=scsi_register(psht,sizeof(struct _ACB)))==0)
				{
					printk("arcmsr_detect: scsi_register error . . . . . . . . . . .\n");
					continue;
				}
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
				{
					if(pci_enable_device(pPCI_DEV))
					{
						printk("arcmsr_detect: pci_enable_device ERROR..................................\n");
						scsi_unregister(psh);
      					continue;
					}
 					if(!pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0xffffffffffffffffULL))/*64bit*/
					{
						printk("ARECA RAID: 64BITS PCI BUS DMA ADDRESSING SUPPORTED\n");
					}
					else if(pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0x00000000ffffffffULL))/*32bit*/
					{
						printk("ARECA RAID: 32BITS PCI BUS DMA ADDRESSING NOT SUPPORTED (ERROR)\n");
						scsi_unregister(psh);
      					continue;
					}
				}
			#endif
				pACB=(PACB) psh->hostdata;
				memset(pACB,0,sizeof(struct _ACB));
                spin_lock_init(&pACB->isr_lockunlock);
				spin_lock_init(&pACB->wait2go_lockunlock);
		    	spin_lock_init(&pACB->qbuffer_lockunlock);
				spin_lock_init(&pACB->ccb_doneindex_lockunlock);
				spin_lock_init(&pACB->ccb_startindex_lockunlock);
				pACB->pPCI_DEV=pPCI_DEV;
				pACB->pScsiHost=psh;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,7)
				psh->max_sectors=ARCMSR_MAX_XFER_SECTORS;
			#endif
				psh->max_lun=ARCMSR_MAX_TARGETLUN;
				psh->max_id=ARCMSR_MAX_TARGETID;/*16:8*/
                psh->max_cmd_len=16;            /*this is issue of 64bit LBA ,over 2T byte*/
				psh->sg_tablesize=ARCMSR_MAX_SG_ENTRIES;
				psh->can_queue=ARCMSR_MAX_OUTSTANDING_CMD; /* max simultaneous cmds */             
				psh->cmd_per_lun=ARCMSR_MAX_CMD_PERLUN;            
				psh->this_id=ARCMSR_SCSI_INITIATOR_ID; 
				psh->io_port=0;
				psh->n_io_port=0;
				psh->irq=pPCI_DEV->irq;
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,4)
				scsi_set_pci_device(psh,pPCI_DEV);
			#endif
				if(!arcmsr_initialize(pACB,pPCI_DEV))
				{
				#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
					pci_set_drvdata(pPCI_DEV,pACB); /*set driver_data*/
				#endif
					pci_set_master(pPCI_DEV);
					if(request_irq(pPCI_DEV->irq,arcmsr_doInterrupt,SA_INTERRUPT | SA_SHIRQ,"arcmsr",pACB))
					{
                        printk("arcmsr_detect:  request_irq got ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->pACB[pACB->adapter_index]=NULL;
	   					iounmap(pACB->pmu);
        				arcmsr_free_pci_pool(pACB);
   					    scsi_unregister(psh);
						goto next_areca;
					}
					if (pci_request_regions(pPCI_DEV, "arcmsr"))
					{
                        printk("arcmsr_detect:  pci_request_regions got ERROR...................\n");
						arcmsr_adapterCnt--;
						pHCBARC->pACB[pACB->adapter_index]=NULL;
 						iounmap(pACB->pmu);
        				arcmsr_free_pci_pool(pACB);
   					    scsi_unregister(psh);
						goto next_areca;
					}
					arcmsr_iop_init(pACB);/* on kernel 2.4.21 driver's iop read/write must after request_irq */
				}
				else
				{
					printk("arcmsr: arcmsr_initialize got ERROR...................\n");
					scsi_unregister(psh);
				}
        next_areca:
			}
		}
		if(arcmsr_adapterCnt)
		{
			#if LINUX_VERSION_CODE >=KERNEL_VERSION(2,3,30)
  			psht->proc_name="arcmsr";
			#else		  
  			psht->proc_dir= &arcmsr_proc_scsi;
			#endif
			arcmsr_plunge_kernelthread();
			register_reboot_notifier(&arcmsr_event_notifier);
		}
		else
		{
			printk("arcmsr_detect:NO ARECA MASS STORAGE SATA RAID HOST ADAPTER FOUND...........\n");
		}
		pHCBARC->adapterCnt=arcmsr_adapterCnt;
		pHCBARC->arcmsr_major_number=register_chrdev(0, "arcmsr", &arcmsr_file_operations);
		printk("arcmsr device major number %d \n",pHCBARC->arcmsr_major_number);
		return(arcmsr_adapterCnt);
	}
#endif 
/*
**********************************************************************
**   
**    
**********************************************************************
*/
VOID arcmsr_pci_unmap_dma(PCCB pCCB)
{
	PACB pACB=pCCB->pACB;
    PSCSICMD pcmd=pCCB->pcmd;

	if(pcmd->use_sg != 0) 
	{
		struct scatterlist *sl;

		sl = (struct scatterlist *)pcmd->request_buffer;
		pci_unmap_sg(pACB->pPCI_DEV, sl, pcmd->use_sg, scsi_to_pci_dma_dir(pcmd->sc_data_direction));
	} 
	else if(pcmd->request_bufflen != 0)
	{
		pci_unmap_single(pACB->pPCI_DEV,(dma_addr_t)(ULONG)pcmd->SCp.ptr,pcmd->request_bufflen, scsi_to_pci_dma_dir(pcmd->sc_data_direction));
	}
	return;
}
/*
**********************************************************************************
**
**********************************************************************************
*/
static int arcmsr_fops_open(struct inode *inode, struct file *filep)
{
	int i,minor;
	PACB pACB;

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
	if(pACB->acb_flags & ACB_F_IOCTL_OPEN) 
	{
		return -EBUSY;
	}
	pACB->acb_flags |= ACB_F_IOCTL_OPEN;
	return 0;		/* success */
}
/*
**********************************************************************************
**
**********************************************************************************
*/
static int arcmsr_fops_close(struct inode *inode, struct file *filep)
{
	int i,minor;
	PACB pACB;

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
	pACB->acb_flags &= ~ACB_F_IOCTL_OPEN;
	return 0;
}
/*
**********************************************************************************
**
**********************************************************************************
*/
static int arcmsr_fops_ioctl(struct inode *inode, struct file *filep, unsigned int ioctl_cmd, unsigned long arg)
{
	int i,minor;
	PACB pACB;

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
**************************************************************************
**************************************************************************
*/
int arcmsr_queue_dpc(DPCFUN dpcfun,void *arg)
{
	unsigned long flag;
	UCHAR index_pointer;

	#if ARCMSR_DEBUG0
	printk("arcmsr_queue_dpc................. \n");
	#endif

    spin_lock_irqsave(&pHCBARC->dpc_lockunlock,flag);
	index_pointer=(pHCBARC->dpcQ_tail + 1) % ARCMSR_MAX_DPC;
	if(index_pointer==pHCBARC->dpcQ_head) 
	{
        spin_unlock_irqrestore(&pHCBARC->dpc_lockunlock,flag);

		printk("DPC Queue full!\n");
		return -1;
	}
	pHCBARC->dpcQ[pHCBARC->dpcQ_tail].dpcfun=dpcfun;
	pHCBARC->dpcQ[pHCBARC->dpcQ_tail].arg=arg;
	pHCBARC->dpcQ_tail=index_pointer;
	/* 
	*********************************************************
	*********************************************************
	*/
	up(pHCBARC->do_dpcQ_semaphore); 
    spin_unlock_irqrestore(&pHCBARC->dpc_lockunlock,flag);
	return 0;
}
/*
**************************************************************************
**         arcmsr_do_dpcQ
**    execute dpc routine by kernel thread 
***************************************************************************
*/
void arcmsr_do_dpcQ(void)
{
	#if ARCMSR_DEBUG0
	printk("arcmsr_do_dpcQ................. \n");
	#endif
	/*
	******************************************
	******************************************
	*/
	while(pHCBARC->dpcQ_head!=pHCBARC->dpcQ_tail) 
	{
		unsigned long flag;
		DPC dpc;

		/* got a "dpc routine" */
        spin_lock_irqsave(&pHCBARC->dpc_lockunlock,flag);

		dpc=pHCBARC->dpcQ[pHCBARC->dpcQ_head];
		pHCBARC->dpcQ_head++;
		pHCBARC->dpcQ_head %=ARCMSR_MAX_DPC;

        spin_unlock_irqrestore(&pHCBARC->dpc_lockunlock,flag);
		/* execute this "dpc routine" */
		dpc.dpcfun(dpc.arg);
		schedule();/* don't waste all the CPU time */
		/* if "abort rebulid" occured,we should finish all pending DPCs before terminate the thread. */
	}
	return;
}
/*
**************************************************************************
*         arcmsr_do_thread_works
*    execute programs schedule by kernel thread
*      :do background rebuilding 
***************************************************************************
*/
void arcmsr_do_thread_works(void *arg)
{
	PHCBARC phcb=(PHCBARC) arg;
 	//struct task_struct *tsk=current;
	DECLARE_MUTEX_LOCKED(sem);

	#if ARCMSR_DEBUG0
	printk("arcmsr_do_thread_works................. \n");
	#endif

	lock_kernel();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,60)
	daemonize();
#else
	daemonize("arcmsr kernel thread");
#endif
 	unlock_kernel();
	/* Set the name of this process. */
	strcpy(current->comm,"arcmsr_kernel_thread");
	phcb->do_dpcQ_semaphore= &sem;
	phcb->do_dpcQ_task=current; /*lock do_dpcQ*/
	if(phcb->mutex_semaphore !=NULL) 
	{
		/* 
		** mutex_semaphore is down signal it 
		*/
		up(phcb->mutex_semaphore); 
	}
	while(1) 
	{
		down(phcb->do_dpcQ_semaphore);
		/*
		** if do_dpcQ_semaphore is signal
		** do following works
		*/
        arcmsr_do_dpcQ(); /*see if there were some dpc routine need to execute*/
		if(phcb->hcb_flags & HCB_F_STOP_THREAD) 
		{
	#if ARCMSR_DEBUG0
	printk("arcmsr_do_thread_works.................HCB_F_STOP_THREAD \n");
	#endif
			break;
		}
	}
	phcb->do_dpcQ_task=NULL; /*unlock task*/
	if(phcb->mutex_semaphore !=NULL) 
	{
		/* 
		** mutex_semaphore is down,signal it 
		*/
		up(phcb->mutex_semaphore);
	}
}
/*
**************************************************************************
** 
** 
**************************************************************************
*/
void arcmsr_plunge_kernelthread(void)
{
	DECLARE_MUTEX_LOCKED(sem);

	#if ARCMSR_DEBUG0
	printk("arcmsr_plunge_kernelthread............ \n");
	#endif

	pHCBARC->mutex_semaphore= &sem;/*we use mutex_semaphore to signal when to do arcmsr_do dpcQ*/
	kernel_thread((int (*)(void *))arcmsr_do_thread_works,(void*)pHCBARC,0);
	down(pHCBARC->mutex_semaphore);
	pHCBARC->mutex_semaphore=NULL;
	return;
}
/*
**********************************************************************
** <second> bit 05,04,03,02,01,00: 0 - 59 
** <minute> bit 11,10,09,08,07,06: 0 - 59 
** <month>  bit       15,14,13,12: 1 - 12 
** <hour>   bit 21,20,19,18,17,16: 0 - 59 
** <day>    bit    26,25,24,23,22: 1 - 31 
** <year>   bit    31,30,29,28,27: 0=2000,31=2031 
**********************************************************************
*/
ULONG arcmsr_make_timespec(ULONG year,ULONG mon,ULONG day,ULONG hour,ULONG min,ULONG sec)
{
    return((year<<27)|(day<<22)|(hour<<16)|(mon<<12)|(min<<6)|(sec));
}
/*
********************************************************************
********************************************************************
*/
ULONG arcmsr_getcmos_time(void)
{
	ULONG year,mon,day,hour,min,sec;
	int i;

    #if ARCMSR_DEBUG0
    printk("arcmsr_getcmos_time \n");
    #endif

	/* read RTC exactly on falling edge of update flag */
	for(i=0 ; i<1000000 ; i++)	/* may take up to 1 second... */
	{
		if(CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP)
		{
			break;
		}
	}
	for(i=0 ; i<1000000 ; i++)	/* must try at least 2.228 ms */
	{
		if(!(CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP))
		{
			break;
		}
	}
	do { /* Isn't this overkill ? UIP above should guarantee consistency */
		sec=CMOS_READ(RTC_SECONDS);
		min=CMOS_READ(RTC_MINUTES);
		hour=CMOS_READ(RTC_HOURS);
		day=CMOS_READ(RTC_DAY_OF_MONTH);
		mon=CMOS_READ(RTC_MONTH);
		year=CMOS_READ(RTC_YEAR);
	} while(sec !=CMOS_READ(RTC_SECONDS));
	if(!(CMOS_READ(RTC_CONTROL) & RTC_DM_BINARY) || RTC_ALWAYS_BCD)
	{
	    BCD_TO_BIN(sec);
	    BCD_TO_BIN(min);
	    BCD_TO_BIN(hour);
	    BCD_TO_BIN(day);
	    BCD_TO_BIN(mon);
	    BCD_TO_BIN(year);
	}
	if((year +=1900) < 1970)
	{
		year +=100;
	}
	return arcmsr_make_timespec(year,mon,day,hour,min,sec);
}
/*
**********************************************************************
**
**  Linux scsi mid layer command complete
**
**********************************************************************
*/
VOID arcmsr_cmd_done(PSCSICMD pcmd)
{
	unsigned long flags;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		struct Scsi_Host *host = pcmd->device->host;
		spin_lock_irqsave(host->host_lock, flags);
		pcmd->scsi_done(pcmd);
		spin_unlock_irqrestore(host->host_lock, flags);
	#else
		spin_lock_irqsave(&io_request_lock, flags);
		pcmd->scsi_done(pcmd);
		spin_unlock_irqrestore(&io_request_lock, flags);
	#endif
	return;
}
/*
************************************************************************
**
**
************************************************************************
*/
VOID arcmsr_flush_adapter_cache(PACB pACB)
{
    #if ARCMSR_DEBUG0
    printk("arcmsr_flush_adapter_cache..............\n");
    #endif
	CHIP_REG_WRITE32(&pACB->pmu->inbound_msgaddr0,ARCMSR_INBOUND_MESG0_FLUSH_CACHE);
	return;
}
/*
**********************************************************************
**
**  Q back this CCB into ACB ArrayCCB
**
**********************************************************************
*/
VOID arcmsr_ccb_complete(PCCB pCCB)
{
	unsigned long flag;
	PACB pACB=pCCB->pACB;
    PSCSICMD pcmd=pCCB->pcmd;

	#if ARCMSR_DEBUG0
	printk("arcmsr_ccb_complete:..................... pCCB=0x%p ccb_doneindex=0x%x ccb_startindex=0x%x\n",pCCB,pACB->ccb_doneindex,pACB->ccb_startindex);
	#endif
    arcmsr_pci_unmap_dma(pCCB);
    spin_lock_irqsave(&pACB->ccb_doneindex_lockunlock,flag);
	atomic_dec(&pACB->ccboutstandingcount);
	pCCB->startdone=ARCMSR_CCB_DONE;
	pCCB->ccb_flags=0;
	pACB->pccbringQ[pACB->ccb_doneindex]=pCCB;
    pACB->ccb_doneindex++;
    pACB->ccb_doneindex %= ARCMSR_MAX_FREECCB_NUM;
    spin_unlock_irqrestore(&pACB->ccb_doneindex_lockunlock,flag);
   	arcmsr_cmd_done(pcmd);
	return;
}
/*
**********************************************************************
**       if scsi error do auto request sense
**********************************************************************
*/
VOID arcmsr_report_SenseInfoBuffer(PCCB pCCB)
{
	PSCSICMD pcmd=pCCB->pcmd;
	PSENSE_DATA  psenseBuffer=(PSENSE_DATA)pcmd->sense_buffer;
	#if ARCMSR_DEBUG0
    printk("arcmsr_report_SenseInfoBuffer...........\n");
	#endif

    pcmd->result=DID_OK << 16;
    if(psenseBuffer) 
	{
		memset(psenseBuffer, 0, sizeof(pcmd->sense_buffer));
		memcpy(psenseBuffer,pCCB->arcmsr_cdb.SenseData,get_min(sizeof(struct _SENSE_DATA),sizeof(pcmd->sense_buffer)));
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
VOID arcmsr_queue_wait2go_ccb(PACB pACB,PCCB pCCB)
{
    unsigned long flag;
	int i=0;
    #if ARCMSR_DEBUG0
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
** 
*********************************************************************
*/
VOID arcmsr_abort_allcmd(PACB pACB)
{
	CHIP_REG_WRITE32(&pACB->pmu->inbound_msgaddr0,ARCMSR_INBOUND_MESG0_ABORT_CMD);
	return;
}
/*
**********************************************************************
** 
**  
**
**********************************************************************
*/
BOOLEAN arcmsr_wait_msgint_ready(PACB pACB)
{
	ULONG Index;
	UCHAR Retries=0x00;
	do
	{
		for(Index=0; Index < 500000; Index++)
		{
			if(CHIP_REG_READ32(&pACB->pmu->outbound_intstatus) & ARCMSR_MU_OUTBOUND_MESSAGE0_INT)
			{
				CHIP_REG_WRITE32(&pACB->pmu->outbound_intstatus, ARCMSR_MU_OUTBOUND_MESSAGE0_INT);/*clear interrupt*/
				return TRUE;
			}
			/* one us delay	*/
			udelay(10);
		}/*max 5 seconds*/
	}while(Retries++ < 24);/*max 2 minutes*/
	return FALSE;
}
/*
****************************************************************************
** Routine Description: Reset 80331 iop.
**           Arguments: 
**        Return Value: Nothing.
****************************************************************************
*/
VOID arcmsr_iop_reset(PACB pACB)
{
	PCCB pCCB;
	ULONG intmask_org,mask;
    int i=0;

	#if ARCMSR_DEBUG0
	printk("arcmsr_iop_reset: reset iop controller......................................\n");
	#endif
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{
		#if ARCMSR_DEBUG0
		printk("arcmsr_iop_reset: ccboutstandingcount=%d ...\n",atomic_read(&pACB->ccboutstandingcount));
		#endif
        /* disable all outbound interrupt */
		intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);
        CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE);
        /* talk to iop 331 outstanding command aborted*/
		arcmsr_abort_allcmd(pACB);
		if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
		{
            printk("arcmsr_HwResetBus: wait 'abort all outstanding command' timeout................. \n");
		}
		/*clear all outbound posted Q*/
		for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
		{
			CHIP_REG_READ32(&pACB->pmu->outbound_queueport);
		}
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
			CPT2INT ccb_addr;

			ccb_addr=(CPT2INT)pACB->pccb_pool[i];
			pCCB=(ccb_addr & 0x1F) ? (PCCB)(ccb_addr+(0x20-(ccb_addr & 0x1F))) : (PCCB)ccb_addr;
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				pCCB->startdone=ARCMSR_CCB_ABORTED;
				pCCB->pcmd->result=DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
			}
		}
	#else
		{
			PCCB pfreeccb=pACB->pfreeccb;
			for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
			{
				pCCB=&pfreeccb[i];
				if(pCCB->startdone==ARCMSR_CCB_START)
				{
					pCCB->startdone=ARCMSR_CCB_ABORTED;
					pCCB->pcmd->result=DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
				}
			}
		}
	#endif
		/* enable all outbound interrupt */
		mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
        CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org & mask);
		atomic_set(&pACB->ccboutstandingcount,0);
		/* post abort all outstanding command message to RAID controller */
	}
	i=0;
	while(atomic_read(&pACB->ccbwait2gocount)!=0)
	{
		pCCB=pACB->pccbwait2go[i];
		if(pCCB!=NULL)
		{
			#if ARCMSR_DEBUG0
			printk("arcmsr_iop_reset:abort command... ccbwait2gocount=%d ...\n",atomic_read(&pACB->ccbwait2gocount));
			#endif
		    pACB->pccbwait2go[i]=NULL;
            pCCB->startdone=ARCMSR_CCB_ABORTED;
            pCCB->pcmd->result=DID_ABORT << 16;
			arcmsr_ccb_complete(pCCB);
  			atomic_dec(&pACB->ccbwait2gocount);
		}
		i++;
		i%=ARCMSR_MAX_OUTSTANDING_CMD;
	}
	return;
}
/*
**********************************************************************
** 
** PAGE_SIZE=4096 or 8192,PAGE_SHIFT=12
**********************************************************************
*/
VOID arcmsr_build_ccb(PACB pACB,PCCB pCCB,PSCSICMD pcmd)
{
    PARCMSR_CDB pARCMSR_CDB=&pCCB->arcmsr_cdb;
	PCHAR psge=(PCHAR)&pARCMSR_CDB->u;
	ULONG address_lo,address_hi;
	int arccdbsize=0x30;
	
	#if ARCMSR_DEBUG0
	printk("arcmsr_build_ccb........................... \n");
	#endif
    pCCB->pcmd=pcmd;
	memset(pARCMSR_CDB,0,sizeof(struct _ARCMSR_CDB));
    pARCMSR_CDB->Bus=0;
    pARCMSR_CDB->TargetID=pcmd->device->id;
    pARCMSR_CDB->LUN=pcmd->device->lun;
    pARCMSR_CDB->Function=1;
	pARCMSR_CDB->CdbLength=(UCHAR)pcmd->cmd_len;
    pARCMSR_CDB->Context=(CPT2INT)pARCMSR_CDB;
    memcpy(pARCMSR_CDB->Cdb, pcmd->cmnd, pcmd->cmd_len);
	if(pcmd->use_sg) 
	{
		int length,sgcount,i,cdb_sgcount=0;
		struct scatterlist *sl;

		/* Get Scatter Gather List from Storport. */
		sl=(struct scatterlist *) pcmd->request_buffer;
		sgcount=pci_map_sg(pACB->pPCI_DEV, sl, pcmd->use_sg, scsi_to_pci_dma_dir(pcmd->sc_data_direction));
		/* map stor port SG list to our iop SG List.*/
		for(i=0;i<sgcount;i++) 
		{
			/* Get the physical address of the current data pointer */
			length=cpu_to_le32(sg_dma_len(sl));
            address_lo=cpu_to_le32(dma_addr_lo32(sg_dma_address(sl)));
			address_hi=cpu_to_le32(dma_addr_hi32(sg_dma_address(sl)));
 			if(address_hi==0)
			{
				PSG32ENTRY pdma_sg=(PSG32ENTRY)psge;

				pdma_sg->address=address_lo;
				pdma_sg->length=length;
				psge += sizeof(SG32ENTRY);
				arccdbsize += sizeof(SG32ENTRY);
			}
			else
			{
				int sg64s_size=0,tmplength=length;

     			#if ARCMSR_DEBUG0
				printk("arcmsr_build_ccb: ..........address_hi=0x%x.... \n",address_hi);
				#endif

				while(1)
				{
					LONG64 span4G,length0;
					PSG64ENTRY pdma_sg=(PSG64ENTRY)psge;

					span4G=(LONG64)address_lo + tmplength;
					pdma_sg->addresshigh=address_hi;
					pdma_sg->address=address_lo;
					if(span4G > 0x100000000ULL)
					{   
						/*see if cross 4G boundary*/
						length0=0x100000000ULL-address_lo;
						pdma_sg->length=(ULONG)length0|IS_SG64_ADDR;
						address_hi=address_hi+1;
						address_lo=0;
						tmplength=tmplength-(LONG)length0;
						sg64s_size += sizeof(SG64ENTRY);
						psge += sizeof(SG64ENTRY);
						cdb_sgcount++;
					}
					else
					{
    					pdma_sg->length=tmplength|IS_SG64_ADDR;
						sg64s_size += sizeof(SG64ENTRY);
						psge += sizeof(SG64ENTRY);
						break;
					}
				}
				arccdbsize += sg64s_size;
			}
			sl++;
			cdb_sgcount++;
		}
		pARCMSR_CDB->sgcount=(UCHAR)cdb_sgcount;
		pARCMSR_CDB->DataLength=pcmd->request_bufflen;
		if( arccdbsize > 256)
		{
			pARCMSR_CDB->Flags|=ARCMSR_CDB_FLAG_SGL_BSIZE;
		}
	}
	else if(pcmd->request_bufflen) 
	{
        dma_addr_t dma_addr;

		dma_addr=pci_map_single(pACB->pPCI_DEV, pcmd->request_buffer, pcmd->request_bufflen, scsi_to_pci_dma_dir(pcmd->sc_data_direction));
		pcmd->SCp.ptr = (char *)(unsigned long) dma_addr;
        address_lo=cpu_to_le32(dma_addr_lo32(dma_addr));
	    address_hi=cpu_to_le32(dma_addr_hi32(dma_addr));
		if(address_hi==0)
		{
			PSG32ENTRY pdma_sg=(PSG32ENTRY)psge;
			pdma_sg->address=address_lo;
            pdma_sg->length=pcmd->request_bufflen;
		}
		else
		{
			PSG64ENTRY pdma_sg=(PSG64ENTRY)psge;
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
	#if ARCMSR_DEBUG0
	printk("arcmsr_build_ccb: pCCB=0x%p cmd=0x%x xferlength=%d arccdbsize=%d sgcount=%d\n",pCCB,pcmd->cmnd[0],pARCMSR_CDB->DataLength,arccdbsize,pARCMSR_CDB->sgcount);
	#endif
    return;
}
/*
**************************************************************************
**
**	arcmsr_post_ccb - Send a protocol specific ARC send postcard to a AIOC .
**	handle: Handle of registered ARC protocol driver
**	adapter_id: AIOC unique identifier(integer)
**	pPOSTCARD_SEND: Pointer to ARC send postcard
**
**	This routine posts a ARC send postcard to the request post FIFO of a
**	specific ARC adapter.
**                             
**************************************************************************
*/ 
static VOID arcmsr_post_ccb(PACB pACB,PCCB pCCB)
{
	ULONG cdb_shifted_phyaddr=pCCB->cdb_shifted_phyaddr;
	PARCMSR_CDB pARCMSR_CDB=(PARCMSR_CDB)&pCCB->arcmsr_cdb;

	#if ARCMSR_DEBUG0
	printk("arcmsr_post_ccb: pCCB=0x%p cdb_shifted_phyaddr=0x%x pCCB->pACB=0x%p \n",pCCB,cdb_shifted_phyaddr,pCCB->pACB);
	#endif
    atomic_inc(&pACB->ccboutstandingcount);
	pCCB->startdone=ARCMSR_CCB_START;
	if(pARCMSR_CDB->Flags & ARCMSR_CDB_FLAG_SGL_BSIZE)
	{
	    CHIP_REG_WRITE32(&pACB->pmu->inbound_queueport,cdb_shifted_phyaddr|ARCMSR_CCBPOST_FLAG_SGL_BSIZE);
	}
	else
	{
	    CHIP_REG_WRITE32(&pACB->pmu->inbound_queueport,cdb_shifted_phyaddr);
	}
	return;
}
/*
**************************************************************************
**
**
**************************************************************************
*/
VOID arcmsr_post_wait2go_ccb(PACB pACB)
{
	unsigned long flag;
	PCCB pCCB;
	int i=0;
	#if ARCMSR_DEBUG0
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
VOID arcmsr_post_Qbuffer(PACB pACB)
{
	PUCHAR pQbuffer;
	PQBUFFER pwbuffer=(PQBUFFER)&pACB->pmu->ioctl_wbuffer;
    PUCHAR iop_data=(PUCHAR)pwbuffer->data;
	LONG allxfer_len=0;

	while((pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex) && (allxfer_len<124))
	{
		pQbuffer=&pACB->wqbuffer[pACB->wqbuf_firstindex];
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
 	CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell,ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK);
	return;
}
/*
************************************************************************
**
**
************************************************************************
*/
VOID arcmsr_stop_adapter_bgrb(PACB pACB)
{
    #if ARCMSR_DEBUG0
    printk("arcmsr_stop_adapter_bgrb..............\n");
    #endif
	pACB->acb_flags |= ACB_F_MSG_STOP_BGRB;
	pACB->acb_flags &= ~ACB_F_MSG_START_BGRB;
	CHIP_REG_WRITE32(&pACB->pmu->inbound_msgaddr0,ARCMSR_INBOUND_MESG0_STOP_BGRB);
	return;
}
/*
************************************************************************
**
**
************************************************************************
*/
static void arcmsr_free_pci_pool(PACB pACB)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	{
		int i;

		for(i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) 
		{
			if(pACB->pccb_pool[i])
			{
				pci_pool_free(pACB->uncacheptr_pool, pACB->pccb_pool[i], pACB->pccb_pool_dma_addr[i]);
			}
			pACB->pccb_pool[i] = NULL;
		}
		if(pACB->uncacheptr_pool)
		{
			pci_pool_destroy(pACB->uncacheptr_pool);
		}
		pACB->uncacheptr_pool = NULL;
	}
#else
	{
	    pci_free_consistent(pACB->pPCI_DEV, ((sizeof(struct _CCB) * ARCMSR_MAX_FREECCB_NUM)+0x20), pACB->uncacheptr, pACB->dma_addr);
	}
#endif
	return;
}
/*
**********************************************************************
**   Function:  arcmsr_HwInterrupt
**     Output:  VOID
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
    static irqreturn_t arcmsr_HwInterrupt(PACB pACB)
#else
    void arcmsr_HwInterrupt(PACB pACB)
#endif
{
	PCCB pCCB;
	ULONG flag_ccb,outbound_intstatus,outbound_doorbell;
    #if ARCMSR_DEBUG0
    printk("arcmsr_HwInterrupt...................................\n");
    #endif

	/*
	*********************************************
	**   check outbound intstatus @K9n&35L6l.t+v*y9a
	*********************************************
	*/
	outbound_intstatus=CHIP_REG_READ32(&pACB->pmu->outbound_intstatus) & pACB->outbound_int_enable;
	CHIP_REG_WRITE32(&pACB->pmu->outbound_intstatus, outbound_intstatus);/*clear interrupt*/
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_DOORBELL_INT)
	{
		#if ARCMSR_DEBUG0
		printk("arcmsr_HwInterrupt:..........ARCMSR_MU_OUTBOUND_DOORBELL_INT\n");
		#endif
		/*
		*********************************************
		**  DOORBELL %m>4! ,O'_&36l%s-nC1&,
		*********************************************
		*/
		outbound_doorbell=CHIP_REG_READ32(&pACB->pmu->outbound_doorbell);
		CHIP_REG_WRITE32(&pACB->pmu->outbound_doorbell,outbound_doorbell);/*clear interrupt */
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK)
		{
			PQBUFFER prbuffer=(PQBUFFER)&pACB->pmu->ioctl_rbuffer;
			PUCHAR iop_data=(PUCHAR)prbuffer->data;
			PUCHAR pQbuffer;
			LONG my_empty_len,iop_len,rqbuf_firstindex,rqbuf_lastindex;
			//unsigned long flag;
	        /*check this iop data if overflow my rqbuffer*/
			//spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			rqbuf_lastindex=pACB->rqbuf_lastindex;
			rqbuf_firstindex=pACB->rqbuf_firstindex;
			iop_len=prbuffer->data_len;
            my_empty_len=(rqbuf_firstindex-rqbuf_lastindex-1)&(ARCMSR_MAX_QBUFFER-1);
			if(my_empty_len>=iop_len)
			{
				while(iop_len > 0)
				{
					pQbuffer=&pACB->rqbuffer[pACB->rqbuf_lastindex];
					memcpy(pQbuffer,iop_data,1);
					pACB->rqbuf_lastindex++;
					pACB->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;/*if last index number set it to 0 */
					iop_data++;
					iop_len--;
				}
				CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell, ARCMSR_INBOUND_DRIVER_DATA_READ_OK);/*signature, let IOP331 know data has been readed */
			}
			else
			{
				#if ARCMSR_DEBUG0
				printk("arcmsr_HwInterrupt: this iop data overflow my rqbuffer.....\n");
                #endif
				pACB->acb_flags|=ACB_F_IOPDATA_OVERFLOW;
			}
			//spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
		}
		if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_READ_OK)
		{
			//unsigned long flag;
			/*
			*********************************************
			**           ,],],O'_AY&36l%s-n669D1H%X
			*********************************************
			*/
            //spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
			if(pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex)
			{
				PUCHAR pQbuffer;
				PQBUFFER pwbuffer=(PQBUFFER)&pACB->pmu->ioctl_wbuffer;
				PUCHAR iop_data=(PUCHAR)pwbuffer->data;
				LONG allxfer_len=0;

				while((pACB->wqbuf_firstindex!=pACB->wqbuf_lastindex) && (allxfer_len<124))
				{
					pQbuffer=&pACB->wqbuffer[pACB->wqbuf_firstindex];
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
				CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell,ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK);
 			}
			else
			{
				pACB->acb_flags |= ACB_F_IOCTL_WQBUFFER_CLEARED;
			}
			//spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
		}
	}
	if(outbound_intstatus & ARCMSR_MU_OUTBOUND_POSTQUEUE_INT)
	{
 		/*
		*****************************************************************************
		**               areca cdb command done
		*****************************************************************************
		*/
		while(1)
		{
			if((flag_ccb=CHIP_REG_READ32(&pACB->pmu->outbound_queueport)) == 0xFFFFFFFF)
			{
				break;/*chip FIFO no ccb for completion already*/
			}
			/* check if command done with no error*/
			pCCB=(PCCB)(pACB->vir2phy_offset+(flag_ccb << 5));/*frame must be 32 bytes aligned*/
			if((pCCB->pACB!=pACB) || (pCCB->startdone!=ARCMSR_CCB_START))
			{
				if(pCCB->startdone==ARCMSR_CCB_ABORTED)
				{
					pCCB->pcmd->result=DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
					break;
				}
  				printk("arcmsr_HwInterrupt:got an illegal ccb command done ...pACB=0x%p pCCB=0x%p ccboutstandingcount=%d .....\n",pACB,pCCB,atomic_read(&pACB->ccboutstandingcount));
				break;
			}
			#if ARCMSR_DEBUG0
			printk("pCCB=0x%p .....................done\n",pCCB);
			#endif
			if((flag_ccb & ARCMSR_CCBREPLY_FLAG_ERROR)==0)
			{
 				#if ARCMSR_DEBUG0
				printk("pCCB=0x%p   scsi cmd=0x%x................... GOOD ..............done\n",pCCB,pCCB->pcmd->cmnd[0]);
				#endif
				pCCB->pcmd->result=DID_OK << 16;
				arcmsr_ccb_complete(pCCB);
			} 
			else 
			{   
				switch(pCCB->arcmsr_cdb.DeviceStatus)
				{
				case ARCMSR_DEV_SELECT_TIMEOUT:
					{
				#if ARCMSR_DEBUG0
				printk("pCCB=0x%p ......ARCMSR_DEV_SELECT_TIMEOUT\n",pCCB);
				#endif
 						pCCB->pcmd->result=DID_TIME_OUT << 16;
						arcmsr_ccb_complete(pCCB);
					}
					break;
				case ARCMSR_DEV_ABORTED:
					{
				#if ARCMSR_DEBUG0
				printk("pCCB=0x%p ......ARCMSR_DEV_ABORTED\n",pCCB);
				#endif
						pCCB->pcmd->result=DID_NO_CONNECT << 16;
						arcmsr_ccb_complete(pCCB);
					}
					break;
				case ARCMSR_DEV_INIT_FAIL:
					{
				#if ARCMSR_DEBUG0
				printk("pCCB=0x%p .....ARCMSR_DEV_INIT_FAIL\n",pCCB);
				#endif
 						pCCB->pcmd->result=DID_BAD_TARGET << 16;
						arcmsr_ccb_complete(pCCB);
					}
					break;
				case SCSISTAT_CHECK_CONDITION:
					{
				#if ARCMSR_DEBUG0
				printk("pCCB=0x%p .....SCSISTAT_CHECK_CONDITION\n",pCCB);
				#endif
                        arcmsr_report_SenseInfoBuffer(pCCB);
						arcmsr_ccb_complete(pCCB);
					}
					break;
				default:
					/* error occur Q all error ccb to errorccbpending Q*/
 					printk("arcmsr_HwInterrupt:command error done ......but got unknow DeviceStatus=0x%x....\n",pCCB->arcmsr_cdb.DeviceStatus);
					pCCB->pcmd->result=DID_PARITY << 16;/*unknow error or crc error just for retry*/
					arcmsr_ccb_complete(pCCB);
					break;
				}
			}
		}	/*drain reply FIFO*/
	}
	if(!(outbound_intstatus & ARCMSR_MU_OUTBOUND_HANDLE_INT))
	{
		/*it must be share irq*/
		#if ARCMSR_DEBUG0
		printk("arcmsr_HwInterrupt..........FALSE....................share irq.....\n");
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
***********************************************************************
**
************************************************************************
*/
int arcmsr_iop_ioctlcmd(PACB pACB,int ioctl_cmd,void *arg)
{
	CMD_IOCTL_FIELD cmdioctlfld;
	PCMD_IOCTL_FIELD pcmdioctlfld=&cmdioctlfld;
    int retvalue=0;
	/* Only let one of these through at a time */

	#if ARCMSR_DEBUG0
	printk("arcmsr_iop_ioctlcmd.......................................\n");
	#endif
	if(copy_from_user(pcmdioctlfld, arg, sizeof (struct _CMD_IOCTL_FIELD))!=0)
	{
		retvalue= -EFAULT;
		goto ioctl_out;
	}
	if(memcmp(pcmdioctlfld->cmdioctl.Signature,"ARCMSR",6)!=0)
    {
        retvalue= -EINVAL;
		goto ioctl_out;
	}
	switch(ioctl_cmd)
	{
	case ARCMSR_IOCTL_READ_RQBUFFER:
		{
			unsigned long flag;
			UCHAR tmpQbuffer[1032];
			PUCHAR pQbuffer,ptmpQbuffer=&tmpQbuffer[0];
			LONG allxfer_len=0;
            #if ARCMSR_DEBUG0
            printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_READ_RQBUFFER..... \n");
            #endif
			spin_lock_irqsave(&pACB->qbuffer_lockunlock,flag);
   			while((pACB->rqbuf_firstindex!=pACB->rqbuf_lastindex) && (allxfer_len<1031))
			{
				/*copy READ QBUFFER to srb*/
                pQbuffer=&pACB->rqbuffer[pACB->rqbuf_firstindex];
				memcpy(ptmpQbuffer,pQbuffer,1);
				pACB->rqbuf_firstindex++;
				pACB->rqbuf_firstindex %= ARCMSR_MAX_QBUFFER; /*if last index number set it to 0 */
				ptmpQbuffer++;
				allxfer_len++;
			}
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                PQBUFFER prbuffer=(PQBUFFER)&pACB->pmu->ioctl_rbuffer;
                PUCHAR pQbuffer;
				PUCHAR iop_data=(PUCHAR)prbuffer->data;
                LONG iop_len;

                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
			    iop_len=(LONG)prbuffer->data_len;
				/*this iop data does no chance to make me overflow again here, so just do it*/
				while(iop_len>0)
				{
                    pQbuffer=&pACB->rqbuffer[pACB->rqbuf_lastindex];
					memcpy(pQbuffer,iop_data,1);
					pACB->rqbuf_lastindex++;
					pACB->rqbuf_lastindex %= ARCMSR_MAX_QBUFFER;/*if last index number set it to 0 */
					iop_data++;
					iop_len--;
				}
				CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell, ARCMSR_INBOUND_DRIVER_DATA_READ_OK);/*signature, let IOP331 know data has been readed */
			}
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
            memcpy(pcmdioctlfld->ioctldatabuffer,&tmpQbuffer[0],allxfer_len);
			pcmdioctlfld->cmdioctl.Length=allxfer_len;
			pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
				retvalue= -EFAULT;
			}
 		}
		break;
	case ARCMSR_IOCTL_WRITE_WQBUFFER:
		{
			unsigned long flag;
			UCHAR tmpuserbuffer[1032];		
			LONG my_empty_len,user_len,wqbuf_firstindex,wqbuf_lastindex;
            PUCHAR pQbuffer,ptmpuserbuffer=&tmpuserbuffer[0];

            #if ARCMSR_DEBUG0
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_WRITE_WQBUFFER..... \n");
            #endif
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
					pQbuffer=&pACB->wqbuffer[pACB->wqbuf_lastindex];
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
				#if ARCMSR_DEBUG0
				printk("arcmsr_iop_ioctlcmd:invalid data xfer ............qbuffer full............ \n");
                #endif
				pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_ERROR;
			}
            spin_unlock_irqrestore(&pACB->qbuffer_lockunlock,flag);
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
	    		retvalue= -EFAULT;
			}
		}
		break;
	case ARCMSR_IOCTL_CLEAR_RQBUFFER:
		{
		    unsigned long flag;
			PUCHAR pQbuffer=pACB->rqbuffer;
            #if ARCMSR_DEBUG0
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_RQBUFFER..... \n");
            #endif
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell, ARCMSR_INBOUND_DRIVER_DATA_READ_OK);/*signature, let IOP331 know data has been readed */
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
			PUCHAR pQbuffer=pACB->wqbuffer;
            #if ARCMSR_DEBUG0
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_WQBUFFER..... \n");
            #endif

			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell, ARCMSR_INBOUND_DRIVER_DATA_READ_OK);/*signature, let IOP331 know data has been readed */
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
			PUCHAR pQbuffer;
            #if ARCMSR_DEBUG0
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_CLEAR_ALLQBUFFER..... \n");
            #endif
			if(pACB->acb_flags & ACB_F_IOPDATA_OVERFLOW)
			{
                pACB->acb_flags &= ~ACB_F_IOPDATA_OVERFLOW;
                CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell, ARCMSR_INBOUND_DRIVER_DATA_READ_OK);/*signature, let IOP331 know data has been readed */
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
			#if ARCMSR_DEBUG0
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
			PCHAR hello_string="Hello! I am ARCMSR";
            #if ARCMSR_DEBUG0
			printk("arcmsr_iop_ioctlcmd:  ARCMSR_IOCTL_SAY_HELLO..... \n");
            #endif
            memcpy(pcmdioctlfld->ioctldatabuffer,hello_string,(SHORT)strlen(hello_string));
            pcmdioctlfld->cmdioctl.ReturnCode=ARCMSR_IOCTL_RETURNCODE_OK;
			if(copy_to_user(arg,pcmdioctlfld,sizeof (struct _CMD_IOCTL_FIELD))!=0)
			{
	            retvalue= -EFAULT;
			}
		}
		break;
	default:
		retvalue= -EFAULT;
	}
ioctl_out:
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
**	int (*scsi_init_io_fn)  (Scsi_Cmnd *);	            %% Used to initialize  new request %%
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
int arcmsr_ioctl(Scsi_Device *dev,int ioctl_cmd,void *arg) 
{
	PACB pACB;
	LONG match=0x55AA,i;
 
    #if ARCMSR_DEBUG0
    printk("arcmsr_ioctl..................................................... \n");
    #endif

	for(i=0;i<ARCMSR_MAX_ADAPTER;i++)
	{
		if((pACB=pHCBARC->pACB[i])!=NULL)
		{
			if(pACB->pScsiHost==dev->host) 
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
**
**************************************************************************
*/
PCCB arcmsr_get_freeccb(PACB pACB)
{
    PCCB pCCB;
    unsigned long flag;
	int ccb_startindex,ccb_doneindex;

    #if ARCMSR_DEBUG0
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
int arcmsr_queue_command(Scsi_Cmnd *cmd,void (* done)(Scsi_Cmnd *))
{
    struct Scsi_Host *host = cmd->device->host;
	PACB pACB=(PACB) host->hostdata;
    PCCB pCCB;

    #if ARCMSR_DEBUG0
    printk("arcmsr_queue_command:Cmd=%2x,TargetId=%d,Lun=%d \n",cmd->cmnd[0],cmd->device->id,cmd->device->lun);
    #endif
	cmd->scsi_done=done;
	cmd->host_scribble=NULL;
	cmd->result=0;
	if(cmd->cmnd[0]==SYNCHRONIZE_CACHE) /* 0x35 avoid synchronizing disk cache cmd during .remove : arcmsr_device_remove (linux bug) */
	{
		cmd->scsi_done(cmd);
        return(0);
	}
#if 0
	/* In 2.5.X and some 2.4.X versions, the midlayer takes our lock just before calling us, so we avoid locking again.*/
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		spin_unlock_irq(host->host_lock);
	#else
        spin_unlock_irq(&io_request_lock);
	#endif
#endif
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
		printk("ARCMSR SCSI ID%d-LUN%d: invalid CCB in start\n",cmd->device->id,cmd->device->lun);
	    cmd->result=(DID_BUS_BUSY << 16);
  	    cmd->scsi_done(cmd);
	}
#if 0
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		spin_lock_irq(host->host_lock);
	#else
        spin_lock_irq(&io_request_lock);
	#endif
#endif
 	return(0);
}
/*
**********************************************************************
** 
**  start background rebulid
**
**********************************************************************
*/
VOID arcmsr_start_adapter_bgrb(PACB pACB)
{
	#if ARCMSR_DEBUG0
	printk("arcmsr_start_adapter_bgrb.................................. \n");
	#endif
	pACB->acb_flags |= ACB_F_MSG_START_BGRB;
	pACB->acb_flags &= ~ACB_F_MSG_STOP_BGRB;
    CHIP_REG_WRITE32(&pACB->pmu->inbound_msgaddr0,ARCMSR_INBOUND_MESG0_START_BGRB);
	return;
}
/*
**********************************************************************
** 
**  start background rebulid
**
**********************************************************************
*/
VOID arcmsr_iop_init(PACB pACB)
{
    ULONG intmask_org,mask,outbound_doorbell,firmware_state=0;

	#if ARCMSR_DEBUG0
	printk("arcmsr_iop_init.................................. \n");
	#endif
	do
	{
        firmware_state=CHIP_REG_READ32(&pACB->pmu->outbound_msgaddr1);
	}while((firmware_state & ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK)==0);
    intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);/*change "disable iop interrupt" to arcmsr_initialize*/ 
	/*start background rebuild*/
	arcmsr_start_adapter_bgrb(pACB);
	if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
	{
		printk("arcmsr_HwInitialize: wait 'start adapter background rebulid' timeout................ \n");
	}
	/* clear Qbuffer if door bell ringed */
	outbound_doorbell=CHIP_REG_READ32(&pACB->pmu->outbound_doorbell);
	if(outbound_doorbell & ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK)
	{
		CHIP_REG_WRITE32(&pACB->pmu->outbound_doorbell,outbound_doorbell);/*clear interrupt */
        CHIP_REG_WRITE32(&pACB->pmu->inbound_doorbell,ARCMSR_INBOUND_DRIVER_DATA_READ_OK);
	}
	/* enable outbound Post Queue,outbound message0,outbell doorbell Interrupt */
	mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
    CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org & mask);
	pACB->outbound_int_enable = ~(intmask_org & mask) & 0x000000ff;
	pACB->acb_flags |=ACB_F_IOP_INITED;
	return;
}
/*
****************************************************************************
** 
****************************************************************************
*/
int arcmsr_bus_reset(Scsi_Cmnd *cmd)
{
	PACB pACB;
 
	#if ARCMSR_DEBUG0
	printk("arcmsr_bus_reset.......................... \n");
	#endif
	pACB=(PACB) cmd->device->host->hostdata;
	arcmsr_iop_reset(pACB);
 	return SUCCESS;
} 
/*
*****************************************************************************************
**
*****************************************************************************************
*/
int arcmsr_seek_cmd2abort(PSCSICMD pabortcmd)
{
    PACB pACB=(PACB) pabortcmd->device->host->hostdata;
 	PCCB pCCB;
	ULONG intmask_org,mask;
    int i=0;

    #if ARCMSR_DEBUG0
    printk("arcmsr_seek_cmd2abort.................. \n");
    #endif
	/* 
	** It is the upper layer do abort command this lock just prior to calling us.
	** First determine if we currently own this command.
	** Start by searching the device queue. If not found
	** at all,and the system wanted us to just abort the
	** command return success.
	*/
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
		    CPT2INT ccb_addr;

			ccb_addr=(CPT2INT)pACB->pccb_pool[i];
			pCCB=(ccb_addr & 0x1F) ? (PCCB)(ccb_addr+(0x20-(ccb_addr & 0x1F))) : (PCCB)ccb_addr;
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				if(pCCB->pcmd==pabortcmd)
				{
                    goto abort_outstanding_cmd;
				}
			}
		}
	#else
		{
			PCCB pfreeccb;
			pfreeccb=pACB->pfreeccb;
			for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
			{
				pCCB=&pfreeccb[i];
				if(pCCB->startdone==ARCMSR_CCB_START)
				{
					if(pCCB->pcmd==pabortcmd)
					{
						goto abort_outstanding_cmd;
					}
				}
			}
		}
	#endif
	}
	/*
	** seek this command at our command list 
	** if command found then remove,abort it and free this CCB
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
	intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);
    CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE);
    /* talk to iop 331 outstanding command aborted*/
	arcmsr_abort_allcmd(pACB);
	if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
	{
        printk("arcmsr_HwResetBus: wait 'abort all outstanding command' timeout................. \n");
	}
	/*clear all outbound posted Q*/
	for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
	{
		CHIP_REG_READ32(&pACB->pmu->outbound_queueport);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
	{
		CPT2INT ccb_addr;

		ccb_addr=(CPT2INT)pACB->pccb_pool[i];
		pCCB=(ccb_addr & 0x1F) ? (PCCB)(ccb_addr+(0x20-(ccb_addr & 0x1F))) : (PCCB)ccb_addr;
		if(pCCB->startdone==ARCMSR_CCB_START)
		{
			pCCB->startdone=ARCMSR_CCB_ABORTED;
			pCCB->pcmd->result=DID_ABORT << 16;
			arcmsr_ccb_complete(pCCB);
		}
	}
#else
	{
		PCCB pCCB,pfreeccb;
		pfreeccb=pACB->pfreeccb;
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
			pCCB=&pfreeccb[i];
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				pCCB->startdone=ARCMSR_CCB_ABORTED;
				pCCB->pcmd->result=DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
			}
		}
	}
#endif
	/* enable all outbound interrupt */
	mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
    CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org & mask);
	atomic_set(&pACB->ccboutstandingcount,0);
    return (SUCCESS);
}
/*
*****************************************************************************************
**
*****************************************************************************************
*/
int arcmsr_cmd_abort(Scsi_Cmnd *cmd)
{
	int error;

    #if ARCMSR_DEBUG0
    printk("arcmsr_cmd_abort.................. \n");
    #endif
	error=arcmsr_seek_cmd2abort(cmd);
	if(error !=SUCCESS)
	{
		printk("arcmsr_cmd_abort: returns FAILED\n");
	}
	return (error);
}
/*
*************************************************************************
**
*************************************************************************
*/
void arcmsr_internal_done(Scsi_Cmnd *pcmd)
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
int arcmsr_schedule_command(PSCSICMD pcmd)
{
	unsigned long timeout;
	#if ARCMSR_DEBUG0
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
	PACB   pACB;
	USHORT device_id;

    #if ARCMSR_DEBUG0
    printk("arcmsr_info.............\n");
    #endif
	pACB=(PACB) host->hostdata;
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
	}
	return buf;
}
/*
************************************************************************
**
**static __inline__ dma_addr_t pci_dac_page_to_dma(struct pci_dev *pdev, struct page *page, unsigned long offset, int direction)
**{
**	return ((dma_addr_t) page_to_bus(page) +
**		(dma_addr_t) offset);
**}
**static __inline__ struct page * pci_dac_dma_to_page(struct pci_dev *pdev, dma_addr_t dma_addr)
**{
**	return pfn_to_page(phys_to_pfn(dma_addr)); 
**}
************************************************************************
*/
int arcmsr_initialize(PACB pACB,struct pci_dev *pPCI_DEV)
{
	ULONG intmask_org,page_base,page_offset,mem_base_start;
	dma_addr_t dma_addr;
	UCHAR pcicmd;
    PCCB pccb_tmp;
	int i,rc;
	void *page_remapped;

	#if ARCMSR_DEBUG0
	printk("arcmsr_initialize....................................\n");
	#endif
  	/* Enable Busmaster/Mem */
	pci_read_config_byte(pPCI_DEV,PCI_COMMAND,&pcicmd);
	pci_write_config_byte(pPCI_DEV,PCI_COMMAND,pcicmd|PCI_COMMAND_INVALIDATE|PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY); 
	mem_base_start=(ULONG)arcget_pcicfg_base(pPCI_DEV,0);
	page_base=mem_base_start & PAGE_MASK;
	page_offset=mem_base_start - page_base;
	page_remapped=ioremap(page_base,page_offset + 0x1FFF);
	if( page_remapped==NULL )
	{
		printk("arcmsr_initialize: memory mapping region fail............\n");
		free_irq(pPCI_DEV->irq,pACB);
		return(ENXIO);
	}
	pACB->pmu=(PMU)(page_remapped+page_offset);
    pACB->acb_flags |= (ACB_F_IOCTL_WQBUFFER_CLEARED|ACB_F_IOCTL_RQBUFFER_CLEARED);
	pACB->acb_flags &= ~ACB_F_SCSISTOPADAPTER;
 	pACB->irq=pPCI_DEV->irq;
    /* 
	*******************************************************************************
	**                 Allocate the pccb_pool memory 
	******************************************************
    **Create a pci_pool like this:
    **
	**struct pci_pool *pool;
    **
	**     pool = pci_pool_create(name, dev, size, align, alloc, flags);
    **
	**	   The "name" is for diagnostics (like a kmem_cache name); 
	**     dev and size are as above.  
	**     The device's hardware alignment requirement for this type of data is "align" 
	**     (which is expressed in bytes, and must be a	power of two).  
	**     The flags are SLAB_ flags as you'd pass to kmem_cache_create.  
	**     Not all flags are understood, but SLAB_POISON may help you find driver bugs.  
	**     If you call this in a non- sleeping	context (f.e. in_interrupt is true or while holding SMP locks), pass SLAB_ATOMIC.  
	**     If your device has no boundary crossing restrictions, pass 0 for alloc; 
	**     passing 4096 says memory allocated from this pool must not cross 4KByte boundaries 
	**     (but at that time it may be better to go for pci_alloc_consistent directly instead).
    **
    **Allocate memory from a pci pool like this:
    **
	**     cpu_addr = pci_pool_alloc(pool, flags, &dma_addr);
    **
	**     flags are SLAB_KERNEL if blocking is permitted (not in_interrupt nor holding SMP locks), SLAB_ATOMIC otherwise.
	**     Like pci_alloc_consistent,this returns two values, cpu_addr and dma_addr.
	**
    **
    **Free memory that was allocated from a pci_pool like this:
    **
	**     pci_pool_free(pool, cpu_addr, dma_addr);
    **
	**     where pool is what you passed to pci_pool_alloc, and cpu_addr and dma_addr are the values pci_pool_alloc returned.
	**     This function may be called in interrupt context.
    ** 
    **Destroy a pci_pool by calling:
    **
	**     pci_pool_destroy(pool);
	******************************************************************************
	*/
	rc=pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0x00000000ffffffffULL);/*32bit*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	{
		pACB->uncacheptr_pool = pci_pool_create("arcmsr", pPCI_DEV, sizeof(struct _CCB)+0x20, 8, 0);
		if(!pACB->uncacheptr_pool)
		{
			printk("arcmsr_initialize:pci_pool_create got error.......... \n");
    		return -ENOMEM;
		}
		for(i = 0; i < ARCMSR_MAX_FREECCB_NUM; i++) 
		{
			pccb_tmp=(PCCB)pci_pool_alloc(pACB->uncacheptr_pool, SLAB_KERNEL, &dma_addr);
			if(!pccb_tmp)
			{
				printk("arcmsr_initialize:pci_pool_alloc got error.......... \n");
				arcmsr_free_pci_pool(pACB);
    			return -ENOMEM;
			}
			memset(pccb_tmp, 0, sizeof(struct _CCB)+0x20);
			pACB->pccb_pool[i]=pccb_tmp;
			pACB->pccb_pool_dma_addr[i]=dma_addr;
			if(((CPT2INT)pccb_tmp & 0x1F)!=0) /*ccb address must 32 (0x20) boundary*/
			{
				pccb_tmp=pccb_tmp+(0x20-((CPT2INT)pccb_tmp & 0x1F));
				dma_addr= dma_addr+(0x20-((CPT2INT)dma_addr & 0x1F));
			}
			pccb_tmp->cdb_shifted_phyaddr=dma_addr >> 5;
			pccb_tmp->pACB=pACB;
			pACB->pccbringQ[i]=pccb_tmp;
  		}
        pACB->vir2phy_offset=(CPT2INT)pccb_tmp-(CPT2INT)dma_addr;
	}
#else
	{
		PCCB pfreeccb;

	    pACB->uncacheptr=(PUCHAR)pci_alloc_consistent(pPCI_DEV,((sizeof(struct _CCB) * ARCMSR_MAX_FREECCB_NUM)+0x20),&pACB->dma_addr);
		if(pACB->uncacheptr==NULL)
		{
			printk("arcmsr_initialize:......pci_alloc_consistent fail......\n");
			iounmap(pACB->pmu);
			return(ENXIO);
		}
		memset(pACB->uncacheptr, 0, (sizeof(struct _CCB) * ARCMSR_MAX_FREECCB_NUM)+0x20);
        pfreeccb=(PCCB)pACB->uncacheptr;
		dma_addr=pACB->dma_addr;
		if(((CPT2INT)pACB->uncacheptr & 0x1F)!=0)
		{
			pfreeccb=pfreeccb+(0x20-((CPT2INT)pfreeccb & 0x1F));
			dma_addr=dma_addr+(0x20-((CPT2INT)dma_addr & 0x1F));
		}
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
			pccb_tmp=&pfreeccb[i];
			if(((ULONG)pccb_tmp & 0x0000001F)==0) /*ccb address must 32 (0x20) boundary*/
			{
				pccb_tmp->cdb_shifted_phyaddr=dma_addr >> 5;
			    pccb_tmp->pACB=pACB;
				pACB->pccbringQ[i]=pccb_tmp;
				dma_addr=dma_addr+sizeof(struct _CCB);
 			}
			else
			{
				printk(" arcmsr_initialize: this ccb cross 32 bytes boundary ignored ..............\n");
				iounmap(pACB->pmu);
				arcmsr_free_pci_pool(pACB);
				return(ENXIO);
			}
		}
 		pACB->pfreeccb=pfreeccb;
		pACB->vir2phy_offset=(CPT2INT)pccb_tmp-((CPT2INT)dma_addr-sizeof(struct _CCB));
 	}
#endif
	/*
	********************************************************************
	** here we need to tell iop 331 our pccb_tmp.HighPart 
	** if pccb_tmp.HighPart is not zero
	********************************************************************
	*/
    rc=pci_set_dma_mask(pPCI_DEV,(dma_addr_t)0xffffffffffffffffULL);/*set dma 64bit again if could*/
 	pACB->adapter_index=arcmsr_adapterCnt;
	pHCBARC->pACB[arcmsr_adapterCnt]=pACB;
    /* disable iop all outbound interrupt */
    intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);
    CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE);
	arcmsr_adapterCnt++;
    return(0);
}
/*
*********************************************************************
*********************************************************************
*/
int arcmsr_set_info(char *buffer,int length)
{
	#if ARCMSR_DEBUG0
	printk("arcmsr_set_info.............\n");
	#endif
	return (0);
}
/*
*********************************************************************
**      
*********************************************************************
*/
void arcmsr_pcidev_disattach(PACB pACB)
{
	PCCB pCCB;
    ULONG intmask_org,mask;
	int i=0;
    #if ARCMSR_DEBUG0
    printk("arcmsr_pcidev_disattach.................. \n");
    #endif
	/* disable all outbound interrupt */
    intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);
    CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE);
	/* stop adapter background rebuild */
	arcmsr_stop_adapter_bgrb(pACB);
	if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
	{
		printk("arcmsr_pcidev_disattach: wait 'stop adapter rebulid' timeout.... \n");
	}
	arcmsr_flush_adapter_cache(pACB);
	if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
	{
		printk("arcmsr_pcidev_disattach: wait 'flush adapter cache' timeout.... \n");
	}
	/* abort all outstanding command */
	pACB->acb_flags |= ACB_F_SCSISTOPADAPTER;
	pACB->acb_flags &= ~ACB_F_IOP_INITED;
	if(atomic_read(&pACB->ccboutstandingcount)!=0)
	{  
		#if ARCMSR_DEBUG0
		printk("arcmsr_pcidev_disattach: .....pACB->ccboutstandingcount!=0 \n");
		#endif
        /* disable all outbound interrupt */
		intmask_org=CHIP_REG_READ32(&pACB->pmu->outbound_intmask);
        CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org|ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE);
        /* talk to iop 331 outstanding command aborted*/
		arcmsr_abort_allcmd(pACB);
		if(arcmsr_wait_msgint_ready(pACB)!=TRUE)
		{
            printk("arcmsr_HwResetBus: wait 'abort all outstanding command' timeout................. \n");
		}
		/*clear all outbound posted Q*/
		for(i=0;i<ARCMSR_MAX_OUTSTANDING_CMD;i++)
		{
			CHIP_REG_READ32(&pACB->pmu->outbound_queueport);
		}
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
		{
			CPT2INT ccb_addr;

			ccb_addr=(CPT2INT)pACB->pccb_pool[i];
			pCCB=(ccb_addr & 0x1F) ? (PCCB)(ccb_addr+(0x20-(ccb_addr & 0x1F))) : (PCCB)ccb_addr;
			if(pCCB->startdone==ARCMSR_CCB_START)
			{
				pCCB->startdone=ARCMSR_CCB_ABORTED;
				pCCB->pcmd->result=DID_ABORT << 16;
				arcmsr_ccb_complete(pCCB);
			}
		}
	#else
		{
			PCCB pfreeccb=pACB->pfreeccb;
			for(i=0;i<ARCMSR_MAX_FREECCB_NUM;i++)
			{
				pCCB=&pfreeccb[i];
				if(pCCB->startdone==ARCMSR_CCB_START)
				{
					pCCB->startdone=ARCMSR_CCB_ABORTED;
					pCCB->pcmd->result=DID_ABORT << 16;
					arcmsr_ccb_complete(pCCB);
				}
			}
		}
	#endif
		/* enable all outbound interrupt */
		mask=~(ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE|ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE|ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE);
        CHIP_REG_WRITE32(&pACB->pmu->outbound_intmask,intmask_org & mask);
		atomic_set(&pACB->ccboutstandingcount,0);
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
	free_irq(pACB->pPCI_DEV->irq,pACB);
	pci_release_regions(pACB->pPCI_DEV);
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
	PACB pACB;
	PSH psh;
	int i;

	#if ARCMSR_DEBUG0
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
		psh=pACB->pScsiHost;
        arcmsr_pcidev_disattach(pACB);
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
        scsi_remove_host(psh);
	    scsi_host_put(psh);
    #else
 	    scsi_unregister(psh);
    #endif
	}
	/* kill kernel thread */
	if(pHCBARC->do_dpcQ_task)
	{
		DECLARE_MUTEX_LOCKED(sem);
		/*
		** do_dpcQ_task is locking
		*/
		pHCBARC->mutex_semaphore= &sem;
        pHCBARC->hcb_flags |=HCB_F_STOP_THREAD;
		up(pHCBARC->do_dpcQ_semaphore);/* signal to kernel thread do_dpcQ: "stop thread" */
		down(pHCBARC->mutex_semaphore);
		pHCBARC->mutex_semaphore=NULL;
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
	UCHAR  i;
    char * pos=buffer;
    PACB pACB;

	#if ARCMSR_DEBUG0
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
**            arcmsr
**           arcmsr_release
**
************************************************************************
*/
int arcmsr_release(struct Scsi_Host *host)
{
	PACB pACB;
    UCHAR match=0xff,i;

	#if ARCMSR_DEBUG0
	printk("arcmsr_release...........................\n");
	#endif
	if(host==NULL)
	{
        return -ENXIO;
	}
    pACB=(PACB)host->hostdata;
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
	/* kill kernel thread */
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
	if(pHCBARC->do_dpcQ_task)
	{
		DECLARE_MUTEX_LOCKED(sem);
		/*
		** do_dpcQ_task is locking
		*/
		pHCBARC->mutex_semaphore= &sem;
		pHCBARC->hcb_flags |=HCB_F_STOP_THREAD;
		up(pHCBARC->do_dpcQ_semaphore);/* signal to kernel thread do_dpcQ: "stop thread" */
		down(pHCBARC->mutex_semaphore);
		pHCBARC->mutex_semaphore=NULL;
	}
	unregister_chrdev(pHCBARC->arcmsr_major_number, "arcmsr");
	unregister_reboot_notifier(&arcmsr_event_notifier);
	return(0);
}

MODULE_AUTHOR("Erich Chen");
MODULE_DESCRIPTION("ARCMSR(ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Adapter");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif
