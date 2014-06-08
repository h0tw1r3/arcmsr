/*
*******************************************************************************
**	O.S		: Linux
**	FILE NAME	: arcmsr.h
**	Author		: Nick Cheng, C.L. Huang
**	E-mail		: support@areca.com.tw
**	Description	: SCSI RAID Device Driver for Areca RAID Controller
*******************************************************************************
**
** Copyright (C) 2007 - 2012, Areca Technology Corporation
** 
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
*/
#include <linux/version.h>
#ifndef KERNEL_VERSION
	#define KERNEL_VERSION(V, P, S)	(((V) << 16) + ((P) << 8) + (S))
#endif
#if defined(__SMP__) && !defined(CONFIG_SMP)
	#define CONFIG_SMP
#endif
/*
*******************************************************************************
**
*******************************************************************************
*/
#define ARCMSR_MAX_OUTSTANDING_CMD	256
#ifdef CONFIG_XEN
	#define ARCMSR_MAX_FREECCB_NUM	160
#else
	#define ARCMSR_MAX_FREECCB_NUM	256
#endif

#ifndef __user
	#define __user
#endif

#ifndef __iomem
	#define __iomem          
#endif

#ifndef __le32
	#define __le32 uint32_t
#endif

#ifndef bool
	#define bool int
#endif

#ifndef true
	#define true 1 
#endif
#ifndef false
	#define false 0 
#endif
#define warn(format, args...)   printk(format, ## args)
/*
*******************************************************************************
**
*******************************************************************************
*/
#if defined(__KCONF_64BIT__)||defined(_64_SYS_)
	#define _SUPPORT_64_BIT
#else
	#ifdef _SUPPORT_64_BIT
		#error Error 64_BIT CPU Macro
	#endif
#endif /* defined(__KCONF_64BIT__) || _64_SYS_*/
/*
*******************************************************************************
*******************************************************************************
*/
#if defined(_SUPPORT_64_BIT)
	#define PtrToNum(p)	((u64)(void *)(p))
	#define NumToPtr(ul)	((void *)((u64)ul))
#else
	#define PtrToNum(p)	((unsigned int)(unsigned char *)(p))
	#define NumToPtr(ul)	((void *)((u8 *)ul))
#endif
/*
*******************************************************************************
*******************************************************************************
*/
#ifndef list_for_each_entry_safe
	#define list_for_each_entry_safe(pos, n, head, member)		\
	for (pos = list_entry((head)->next, typeof(*pos), member),		\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))
#endif
#if !defined(scsi_sglist) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	#define scsi_sglist(cmd) ((struct scatterlist *)(cmd)->request_buffer)
#endif
#ifndef scsi_for_each_sg
	#define scsi_for_each_sg(cmd, sg, nseg, i)				\
	 for (i = 0, sg = scsi_sglist(cmd); i < (nseg); i++, (sg)++)
#endif
#define ARCMSR_SCSI_INITIATOR_ID	255
#define ARCMSR_MAX_XFER_SECTORS		512 /* (512*512)/1024 = 0x40000(256K) */
#define ARCMSR_MAX_XFER_SECTORS_B	4096 /* (4096*512)/1024 = 0x200000(2M) */
#define ARCMSR_MAX_XFER_SECTORS_C	304 /* (304*512)= 155648(=0x26000)*/
#define ARCMSR_MAX_TARGETID		17 /*17 max target id + 1*/
#define ARCMSR_MAX_TARGETLUN		8 /*8*/
#define ARCMSR_MAX_CMD_PERLUN		ARCMSR_MAX_OUTSTANDING_CMD
#define ARCMSR_MAX_QBUFFER		4096 /* ioctl QBUFFER */
#define ARCMSR_DEFAULT_SG_ENTRIES	38 /* max 38*/
#define ARCMSR_MAX_HBB_POSTQUEUE	264
#define ARCMSR_MAX_ARC1214_POSTQUEUE	256
#define ARCMSR_MAX_ARC1214_DONEQUEUE	257
#define ARCMSR_MAX_ADAPTER		4
#define ARCMSR_SD_TIMEOUT		90
#define ARCMSR_MAX_XFER_LEN		0x26000 /* 152K */
#define ARCMSR_CDB_SG_PAGE_LENGTH	256
#define ARCMST_NUM_MSIX_VECTORS		4
#define ARCMSR_SCSI_CMD_PER_DEV		ARCMSR_MAX_CMD_PERLUN
/*
*******************************************************************************
*******************************************************************************
*/
#ifndef PCI_VENDOR_ID_ARECA
	#define PCI_VENDOR_ID_ARECA		0x17d3 /* Vendor ID */
	#define PCI_DEVICE_ID_ARECA_1110	0x1110 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1120	0x1120 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1130	0x1130 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1160	0x1160 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1170	0x1170 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1200	0x1200 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1201	0x1201 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1202	0x1202 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1210	0x1210 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1214	0x1214 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1220	0x1220 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1230	0x1230 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1260	0x1260 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1270	0x1270 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1280	0x1280 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1680	0x1680 /* Device ID */
	#define PCI_DEVICE_ID_ARECA_1681	0x1681 /* Device ID */
#elif !defined(PCI_DEVICE_ID_ARECA_1200)
	#define PCI_DEVICE_ID_ARECA_1200 	0x1200
	#define PCI_DEVICE_ID_ARECA_1201 	0x1201
	#define PCI_DEVICE_ID_ARECA_1202 	0x1202
#endif

#ifndef PCI_DEVICE_ID_ARECA_1880
	#define PCI_DEVICE_ID_ARECA_1880	0x1880 /* Device ID */
#endif
#ifndef PCI_DEVICE_ID_ARECA_1214
	#define PCI_DEVICE_ID_ARECA_1214	0x1214
#endif

#ifndef PCI_ANY_ID
    #define PCI_ANY_ID (~0)
#endif
#define ACB_F_SCSISTOPADAPTER		0x0001
#define ACB_F_MSG_STOP_BGRB		0x0002	/* stop RAID background rebuild */
#define ACB_F_MSG_START_BGRB		0x0004	/* stop RAID background rebuild */
#define ACB_F_IOPDATA_OVERFLOW		0x0008	/* iop ioctl data rqbuffer overflow */
#define ACB_F_MESSAGE_WQBUFFER_CLEARED	0x0010	/* ioctl:clear wqbuffer on the driver*/
#define ACB_F_MESSAGE_RQBUFFER_CLEARED	0x0020	/* ioctl:clear rqbuffer on the driver*/
#define ACB_F_MESSAGE_WQBUFFER_READED	0x0040	/* ioctl:data in wqbuffer on the IOP have been read*/
#define ACB_F_BUS_RESET			0x0080
#define ACB_F_IOP_INITED		0x0100	/* iop init */
#define ACB_F_ABORT			0x0200
#define ACB_F_FIRMWARE_TRAP		0x0400
#define ACB_F_MSI_ENABLED		0x1000
#define ACB_F_MSIX_ENABLED		0x2000
/*
*******************************************************************************
*******************************************************************************
*/
#define dma_addr_hi32(addr)		(uint32_t) ((addr >> 16) >> 16)
#define dma_addr_lo32(addr)		(uint32_t) (addr & 0xffffffff)

#ifndef DMA_BIT_MASK
	#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL << (n)) - 1))
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 9)
	#define arc_mdelay(msec)	msleep(msec)
	#define arc_mdelay_int(msec)	msleep_interruptible(msec)
#else
	#define arc_mdelay(msec)	mdelay(msec)
	#define arc_mdelay_int(msec)	mdelay(msec)
#endif

#ifndef SA_INTERRUPT
	#define SA_INTERRUPT		IRQF_DISABLED
#endif

#ifndef SA_SHIRQ
	#define SA_SHIRQ		IRQF_SHARED
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
	#define SPIN_LOCK_IRQ_CHK(acb)
	#define SPIN_UNLOCK_IRQ_CHK(acb)
#else
	#define SPIN_LOCK_IRQ_CHK(acb)		spin_lock_irq(acb->host->host_lock)
	#define SPIN_UNLOCK_IRQ_CHK(acb)	spin_unlock_irq(acb->host->host_lock)
#endif 

#ifndef roundup
	#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#endif

#if !defined(RHEL_RELEASE_CODE) && LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32) 
	enum {
		SCSI_QDEPTH_DEFAULT,	/* default requested change, e.g. from sysfs */
		SCSI_QDEPTH_QFULL,	/* scsi-ml requested due to queue full */
		SCSI_QDEPTH_RAMP_UP,	/* scsi-ml requested due to threshhold event */
	};
#endif
/*
*******************************************************************************
*******************************************************************************
*/
typedef struct sPORT_CONFIG_W {
	u32 cfgSignature;
	u32 cfgReqAddrHigh;
	u32 cfgRequestQueueBase; //Must be in the same ReqAddrHigh
	u32 cfgPostQueueBase;
	u32 cfgMaxQueueSize;	//For Driver Mode:set cfgMaxQueueSize to 256
} sPORT_CONFIG_W, *pPORT_CONFIG_W;
/*
*******************************************************************************
**        MESSAGE CONTROL CODE
*******************************************************************************
*/
struct CMD_MESSAGE
{
      uint32_t HeaderLength;
      uint8_t  Signature[8];
      uint32_t Timeout;
      uint32_t ControlCode;
      uint32_t ReturnCode;
      uint32_t Length;
};
/*
*******************************************************************************
**         IOP Message Transfer Data for user space
*******************************************************************************
*/
struct CMD_MESSAGE_FIELD 
{
	struct CMD_MESSAGE		cmdmessage; /*ioctl header*/
	uint8_t				messagedatabuffer[1032];/*areca gui program does not accept more than 1031 byte*/
};
/* IOP message transfer */
#define ARCMSR_MESSAGE_FAIL			0x0001
/*error code for StorPortLogError,ScsiPortLogError*/
#define ARCMSR_IOP_ERROR_ILLEGALPCI		0x0001
#define ARCMSR_IOP_ERROR_VENDORID		0x0002
#define ARCMSR_IOP_ERROR_DEVICEID		0x0002
#define ARCMSR_IOP_ERROR_ILLEGALCDB		0x0003
#define ARCMSR_IOP_ERROR_UNKNOW_CDBERR		0x0004
#define ARCMSR_SYS_ERROR_MEMORY_ALLOCATE	0x0005
#define ARCMSR_SYS_ERROR_MEMORY_CROSS4G		0x0006
#define ARCMSR_SYS_ERROR_MEMORY_LACK		0x0007
#define ARCMSR_SYS_ERROR_MEMORY_RANGE		0x0008
#define ARCMSR_SYS_ERROR_DEVICE_BASE		0x0009
#define ARCMSR_SYS_ERROR_PORT_VALIDATE		0x000A
/*DeviceType*/
#define ARECA_SATA_RAID				0x90000000
/*FunctionCode*/
#define FUNCTION_READ_RQBUFFER		0x0801
#define FUNCTION_WRITE_WQBUFFER		0x0802
#define FUNCTION_CLEAR_RQBUFFER		0x0803
#define FUNCTION_CLEAR_WQBUFFER		0x0804
#define FUNCTION_CLEAR_ALLQBUFFER	0x0805
#define FUNCTION_RETURN_CODE_3F		0x0806
#define FUNCTION_SAY_HELLO		0x0807
#define FUNCTION_SAY_GOODBYE		0x0808
#define FUNCTION_FLUSH_ADAPTER_CACHE	0x0809
#define FUNCTION_GET_FIRMWARE_STATUS	0x080A
#define FUNCTION_HARDWARE_RESET		0x080B
/* ARECA IO CONTROL CODE*/
#define ARCMSR_MESSAGE_READ_RQBUFFER		ARECA_SATA_RAID | FUNCTION_READ_RQBUFFER	/*GUI tells driver that it will read the data in the Qbuffer*/
#define ARCMSR_MESSAGE_WRITE_WQBUFFER		ARECA_SATA_RAID | FUNCTION_WRITE_WQBUFFER	/*GUI tells driver that it will write the data to the Qbuffer*/
#define ARCMSR_MESSAGE_CLEAR_RQBUFFER		ARECA_SATA_RAID | FUNCTION_CLEAR_RQBUFFER
#define ARCMSR_MESSAGE_CLEAR_WQBUFFER		ARECA_SATA_RAID | FUNCTION_CLEAR_WQBUFFER
#define ARCMSR_MESSAGE_CLEAR_ALLQBUFFER		ARECA_SATA_RAID | FUNCTION_CLEAR_ALLQBUFFER
#define ARCMSR_MESSAGE_RETURN_CODE_3F		ARECA_SATA_RAID | FUNCTION_RETURN_CODE_3F
#define ARCMSR_MESSAGE_SAY_HELLO		ARECA_SATA_RAID | FUNCTION_SAY_HELLO
#define ARCMSR_MESSAGE_SAY_GOODBYE		ARECA_SATA_RAID | FUNCTION_SAY_GOODBYE
#define ARCMSR_MESSAGE_FLUSH_ADAPTER_CACHE	ARECA_SATA_RAID | FUNCTION_FLUSH_ADAPTER_CACHE
#define ARCMSR_MESSAGE_GET_FIRMWARE_STATUS	ARECA_SATA_RAID | FUNCTION_GET_FIRMWARE_STATUS
#define ARCMSR_MESSAGE_HARDWARE_RESET		ARECA_SATA_RAID | FUNCTION_HARDWARE_RESET
/* ARECA IOCTL ReturnCode */
#define ARCMSR_MESSAGE_RETURNCODE_OK		0x00000001
#define ARCMSR_MESSAGE_RETURNCODE_ERROR		0x00000006
#define ARCMSR_MESSAGE_RETURNCODE_3F		0x0000003F
#define ARCMSR_MESSAGE_RETURNCODE_BUS_HANG_ON	0x00000088
/* 
*******************************************************************************
*******************************************************************************
*/
#define IS_DMA64		(sizeof(dma_addr_t) == 8)
#define IS_SG64_ADDR	0x01000000			/* bit24 */
struct  SG32ENTRY			/* size 8 bytes */
{					/* length bit 24 == 0 */
	__le32		length;		/* high 8 bit == flag,low 24 bit == length */
	__le32		address;
}__attribute__ ((packed));
struct  SG64ENTRY			/* size 12 bytes */
{					/* length bit 24 == 1 */
	__le32		length;		/* high 8 bit == flag,low 24 bit == length */
   	__le32		address;
   	__le32		addresshigh;
}__attribute__ ((packed));
/* 
*******************************************************************************
*******************************************************************************
*/
struct ARCMSR_PCIINFO
{
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	irq;
	uint16_t	reserved;
}; 
struct QBUFFER
{
	uint32_t __iomem data_len;
	uint8_t __iomem data[124];
};
/*
*******************************************************************************
**      FIRMWARE INFO
*******************************************************************************
*/
struct FIRMWARE_INFO
{
	uint32_t signature;	/*0,00-03*/
	uint32_t request_len;	/*1,04-07*/
	uint32_t numbers_queue;	/*2,08-11*/
	uint32_t sdram_size;	/*3,12-15*/
	uint32_t ide_channels;	/*4,16-19*/
	uint8_t vendor[40];	/*5,20-59*/
	uint8_t model[8];	/*15,60-67*/
	uint8_t firmware_ver[16];	/*17,68-83*/
	uint8_t device_map[16];	/*21,84-99*/
	uint32_t cfgVersion;	/*25,100-103 Added for checking of new firmware capability*/
	uint8_t cfgSerial[16];	/*26,104-119*/
	uint32_t cfgPicStatus;	/*30,120-123*/
};
/* 
*******************************************************************************
**                SPEC. for Areca Type A adapter
*******************************************************************************
*/
/* signature of set and get firmware config */
#define ARCMSR_SIGNATURE_GET_CONFIG		0x87974060
#define ARCMSR_SIGNATURE_SET_CONFIG		0x87974063
/* message code of inbound message register */
#define ARCMSR_INBOUND_MESG0_NOP		0x00000000
#define ARCMSR_INBOUND_MESG0_GET_CONFIG		0x00000001
#define ARCMSR_INBOUND_MESG0_SET_CONFIG		0x00000002
#define ARCMSR_INBOUND_MESG0_ABORT_CMD		0x00000003
#define ARCMSR_INBOUND_MESG0_STOP_BGRB		0x00000004
#define ARCMSR_INBOUND_MESG0_FLUSH_CACHE	0x00000005
#define ARCMSR_INBOUND_MESG0_START_BGRB		0x00000006
#define ARCMSR_INBOUND_MESG0_CHK331PENDING	0x00000007
#define ARCMSR_INBOUND_MESG0_SYNC_TIMER		0x00000008
/* doorbell interrupt generator *//* they are corresponding pairs */
#define ARCMSR_INBOUND_DRIVER_DATA_WRITE_OK	0x00000001	/*Driver notices IOP data bound to IOP has been written down ok			*/
#define ARCMSR_INBOUND_DRIVER_DATA_READ_OK	0x00000002	/*Driver notices IOP data bound to driver has been read ok			*/
#define ARCMSR_OUTBOUND_IOP331_DATA_WRITE_OK	0x00000001	/*IOP notices driver there is data ready to be written to the driver in adapter type A	*/
#define ARCMSR_OUTBOUND_IOP331_DATA_READ_OK	0x00000002	/*IOP notices driver that it is ready to receive the data in adapter type A		*/
/* ccb areca cdb flag */
#define ARCMSR_CCBPOST_FLAG_SGL_BSIZE		0x80000000
#define ARCMSR_CCBPOST_FLAG_IAM_BIOS		0x40000000
#define ARCMSR_CCBREPLY_FLAG_IAM_BIOS		0x40000000
#define ARCMSR_CCBREPLY_FLAG_ERROR_MODE0	0x10000000
#define ARCMSR_CCBREPLY_FLAG_ERROR_MODE1	0x00000001
/* outbound firmware ok */
#define ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK	0x80000000
/* ARC-1680 Bus Reset*/
#define ARCMSR_ARC1680_BUS_RESET		0x00000003
/* 
*******************************************************************************
**                SPEC. for Areca Type B adapter
*******************************************************************************
*/
/* ARECA HBB COMMAND for its FIRMWARE */
#define ARCMSR_DRV2IOP_DOORBELL		0x00020400	/* window of "instruction flags" from driver to iop */
#define ARCMSR_DRV2IOP_DOORBELL_MASK	0x00020404
#define ARCMSR_IOP2DRV_DOORBELL		0x00020408	/* window of "instruction flags" from iop to driver */
#define ARCMSR_IOP2DRV_DOORBELL_MASK	0x0002040C
/* ARECA FLAG LANGUAGE */

#define ARCMSR_IOP2DRV_DATA_WRITE_OK	0x00000001	/* IOP notices driver there is data ready to be written to in adapter type B*/
#define ARCMSR_IOP2DRV_DATA_READ_OK	0x00000002	/* IOP notices driver that it is ready to receive the data in adapter type B*/
#define ARCMSR_IOP2DRV_CDB_DONE		0x00000004
#define ARCMSR_IOP2DRV_MESSAGE_CMD_DONE	0x00000008

#define ARCMSR_DOORBELL_HANDLE_INT		0x0000000F
#define ARCMSR_DOORBELL_INT_CLEAR_PATTERN	0xFF00FFF0
#define ARCMSR_MESSAGE_INT_CLEAR_PATTERN	0xFF00FFF7

#define ARCMSR_MESSAGE_GET_CONFIG		0x00010008	/* (ARCMSR_INBOUND_MESG0_GET_CONFIG << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_SET_CONFIG		0x00020008	/* (ARCMSR_INBOUND_MESG0_SET_CONFIG << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_ABORT_CMD		0x00030008	/* (ARCMSR_INBOUND_MESG0_ABORT_CMD << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_STOP_BGRB		0x00040008	/* (ARCMSR_INBOUND_MESG0_STOP_BGRB << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_FLUSH_CACHE		0x00050008	/* (ARCMSR_INBOUND_MESG0_FLUSH_CACHE << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_START_BGRB		0x00060008	/* (ARCMSR_INBOUND_MESG0_START_BGRB << 16) | ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED) */
#define ARCMSR_MESSAGE_START_DRIVER_MODE	0x000E0008	
#define ARCMSR_MESSAGE_SET_POST_WINDOW		0x000F0008
#define ARCMSR_MESSAGE_ACTIVE_EOI_MODE		0x00100008
#define ARCMSR_MESSAGE_FIRMWARE_OK		0x80000000	/* ARCMSR_OUTBOUND_MESG1_FIRMWARE_OK */

#define ARCMSR_DRV2IOP_DATA_WRITE_OK		0x00000001	/* Driver notices IOP the data have been written to the IOP's register */
#define ARCMSR_DRV2IOP_DATA_READ_OK		0x00000002	/* Driver notices the data in IOP's register have been read */
#define ARCMSR_DRV2IOP_CDB_POSTED		0x00000004
#define ARCMSR_DRV2IOP_MESSAGE_CMD_POSTED	0x00000008
#define ARCMSR_DRV2IOP_END_OF_INTERRUPT		0x00000010	/*  */

/* data tunnel buffer between user space program and its firmware */
#define ARCMSR_MESSAGE_WBUFFER		0x0000fe00	/* user space data to iop 128bytes */
#define ARCMSR_MESSAGE_RBUFFER		0x0000ff00	/* iop data to user space 128bytes */
#define ARCMSR_MESSAGE_RWBUFFER		0x0000fa00	/* iop message_rwbuffer for message command */
/* 
*******************************************************************************
**                SPEC. for Areca Type C adapter
*******************************************************************************
*/
#define ARCMSR_HBC_ISR_THROTTLING_LEVEL		12
#define ARCMSR_HBC_ISR_MAX_DONE_QUEUE		20
/* Host Interrupt Mask */
#define ARCMSR_HBCMU_UTILITY_A_ISR_MASK		0x00000001 /* When clear, the Utility_A interrupt routes to the host.*/
#define ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR_MASK	0x00000004 /* When clear, the General Outbound Doorbell interrupt routes to the host.*/
#define ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR_MASK	0x00000008 /* When clear, the Outbound Post List FIFO Not Empty interrupt routes to the host.*/
#define ARCMSR_HBCMU_ALL_INTMASKENABLE		ARCMSR_HBCMU_UTILITY_A_ISR_MASK | ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR_MASK | ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR_MASK /* disable all ISR */
/* Host Interrupt Status */
#define ARCMSR_HBCMU_UTILITY_A_ISR		0x00000001
#define ARCMSR_HBCMU_OUTBOUND_DOORBELL_ISR	0x00000004
#define ARCMSR_HBCMU_OUTBOUND_POSTQUEUE_ISR	0x00000008
#define ARCMSR_HBCMU_SAS_ALL_INT		0x00000010

/* DoorBell*/
#define ARCMSR_HBCMU_DRV2IOP_DATA_WRITE_OK	0x00000002
#define ARCMSR_HBCMU_DRV2IOP_DATA_READ_OK	0x00000004
/*inbound message 0 ready*/
#define ARCMSR_HBCMU_DRV2IOP_MESSAGE_CMD_DONE	0x00000008
/*more than 15 request completed in a time*/
#define ARCMSR_HBCMU_DRV2IOP_POSTQUEUE_THROTTLING	0x00000010
#define ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_OK		0x00000002
/*outbound DATA WRITE isr door bell clear*/
#define ARCMSR_HBCMU_IOP2DRV_DATA_WRITE_DOORBELL_CLEAR	0x00000002
#define ARCMSR_HBCMU_IOP2DRV_DATA_READ_OK		0x00000004
/*outbound DATA READ isr door bell clear*/
#define ARCMSR_HBCMU_IOP2DRV_DATA_READ_DOORBELL_CLEAR	0x00000004
/*outbound message 0 ready*/
#define ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE	0x00000008
/*outbound message cmd isr door bell clear*/
#define ARCMSR_HBCMU_IOP2DRV_MESSAGE_CMD_DONE_DOORBELL_CLEAR	0x00000008
/*ARCMSR_HBAMU_MESSAGE_FIRMWARE_OK*/
#define ARCMSR_HBCMU_MESSAGE_FIRMWARE_OK	0x80000000
/* ARC-1880 Bus Reset*/
#define ARCMSR_ARC1880_RESET_ADAPTER		0x00000024
#define ARCMSR_ARC1880_DiagWrite_ENABLE		0x00000080
/* 
*******************************************************************************
**                SPEC. for Areca Type D adapter
*******************************************************************************
*/
#define ARCMSR_ARC1214_CHIP_ID	0x00004
#define ARCMSR_ARC1214_CPU_MEMORY_CONFIGURATION		0x00008
#define ARCMSR_ARC1214_I2_HOST_INTERRUPT_MASK		0x00034
#define ARCMSR_ARC1214_SAMPLE_RESET			0x00100
#define ARCMSR_ARC1214_RESET_REQUEST			0x00108
#define ARCMSR_ARC1214_MAIN_INTERRUPT_STATUS		0x00200
#define ARCMSR_ARC1214_PCIE_F0_INTERRUPT_ENABLE		0x0020C
#define ARCMSR_ARC1214_INBOUND_MESSAGE0			0x00400
#define ARCMSR_ARC1214_INBOUND_MESSAGE1			0x00404
#define ARCMSR_ARC1214_OUTBOUND_MESSAGE0		0x00420
#define ARCMSR_ARC1214_OUTBOUND_MESSAGE1		0x00424
#define ARCMSR_ARC1214_INBOUND_DOORBELL			0x00460
#define ARCMSR_ARC1214_OUTBOUND_DOORBELL		0x00480
#define ARCMSR_ARC1214_OUTBOUND_DOORBELL_ENABLE		0x00484
#define ARCMSR_ARC1214_INBOUND_LIST_BASE_LOW		0x01000
#define ARCMSR_ARC1214_INBOUND_LIST_BASE_HIGH		0x01004
#define ARCMSR_ARC1214_INBOUND_LIST_WRITE_POINTER	0x01018
#define ARCMSR_ARC1214_OUTBOUND_LIST_BASE_LOW		0x01060
#define ARCMSR_ARC1214_OUTBOUND_LIST_BASE_HIGH		0x01064
#define ARCMSR_ARC1214_OUTBOUND_LIST_COPY_POINTER	0x0106C
#define ARCMSR_ARC1214_OUTBOUND_LIST_READ_POINTER	0x01070
#define ARCMSR_ARC1214_OUTBOUND_INTERRUPT_CAUSE		0x01088
#define ARCMSR_ARC1214_OUTBOUND_INTERRUPT_ENABLE	0x0108C
#define ARCMSR_ARC1214_MESSAGE_WBUFFER			0x02000
#define ARCMSR_ARC1214_MESSAGE_RBUFFER			0x02100
#define ARCMSR_ARC1214_MESSAGE_RWBUFFER			0x02200
/* Host Interrupt Mask */
#define ARCMSR_ARC1214_ALL_INT_ENABLE		0x00001010
#define ARCMSR_ARC1214_ALL_INT_DISABLE		0x00000000
/* Host Interrupt Status */
#define ARCMSR_ARC1214_OUTBOUND_DOORBELL_ISR	0x00001000
#define ARCMSR_ARC1214_OUTBOUND_POSTQUEUE_ISR	0x00000010
/* DoorBell*/
#define ARCMSR_ARC1214_DRV2IOP_DATA_IN_READY	0x00000001
#define ARCMSR_ARC1214_DRV2IOP_DATA_OUT_READ	0x00000002
/*inbound message 0 ready*/
#define ARCMSR_ARC1214_IOP2DRV_DATA_WRITE_OK	0x00000001
/*outbound DATA WRITE isr door bell clear*/
#define ARCMSR_ARC1214_IOP2DRV_DATA_READ_OK	0x00000002
/*outbound message 0 ready*/
#define ARCMSR_ARC1214_IOP2DRV_MESSAGE_CMD_DONE	0x02000000
/*ARCMSR_HBAMU_MESSAGE_FIRMWARE_OK*/
#define ARCMSR_ARC1214_MESSAGE_FIRMWARE_OK	0x80000000
#define ARCMSR_ARC1214_OUTBOUND_LIST_INTERRUPT_CLEAR	0x00000001
/*
*******************************************************************************
**     Messaging Unit (MU) of Type A, Type B, Type C,  and Type D processor
*******************************************************************************
*/
struct MessageUnit_A
{
	uint32_t	resrved0[4];		/* 0000 000F */
	uint32_t	inbound_msgaddr0;	/* 0010 0013 */
	uint32_t	inbound_msgaddr1;	/* 0014 0017 */
	uint32_t	outbound_msgaddr0;	/* 0018 001B */
	uint32_t	outbound_msgaddr1;	/* 001C 001F */
	uint32_t	inbound_doorbell;	/* 0020 0023 */
	uint32_t	inbound_intstatus;	/* 0024 0027 */
	uint32_t	inbound_intmask;	/* 0028 002B */
	uint32_t	outbound_doorbell;	/* 002C 002F */
	uint32_t	outbound_intstatus;	/* 0030 0033 */
	uint32_t	outbound_intmask;	/* 0034 0037 */
	uint32_t	reserved1[2];		/* 0038 003F */
	uint32_t	inbound_queueport;	/* 0040 0043 */
	uint32_t	outbound_queueport;     /* 0044 0047 */
	uint32_t	reserved2[2];		/* 0048 004F */
	uint32_t	reserved3[492];		/* 0050 07FF 492*/
	uint32_t	reserved4[128];		/* 0800 09FF 128*/
	uint32_t	msgcode_rwbuffer[256];	/* 0a00 0DFF 256*/
	uint32_t	message_wbuffer[32];	/* 0E00 0E7F 32*/
	uint32_t	reserved5[32];		/* 0E80 0EFF 32*/
	uint32_t	message_rbuffer[32];	/* 0F00 0F7F 32*/
	uint32_t	reserved6[32];		/* 0F80 0FFF 32*/
};

struct MessageUnit_B
{
  	uint32_t	post_qbuffer[ARCMSR_MAX_HBB_POSTQUEUE];
  	uint32_t	done_qbuffer[ARCMSR_MAX_HBB_POSTQUEUE];
	uint32_t	postq_index;
	uint32_t	doneq_index;
	uint32_t	__iomem	*drv2iop_doorbell;	/*offset 0x00020400:00,01,02,03: window of "instruction flags" from driver to iop */
	uint32_t	__iomem	*drv2iop_doorbell_mask;	/*04,05,06,07: doorbell mask */
	uint32_t	__iomem	*iop2drv_doorbell;	/*08,09,10,11: window of "instruction flags" from iop to driver */
	uint32_t	__iomem	*iop2drv_doorbell_mask;	/*12,13,14,15: doorbell mask */
	uint32_t	__iomem	*msgcode_rwbuffer;	/*offset 0x0000fa00:   0,   1,   2,   3,...,1023: message code read write 1024bytes */
	uint32_t	__iomem	*message_wbuffer;	/*offset 0x0000fe00:1024,1025,1026,1027,...,1151: user space data to iop 128bytes */
	uint32_t	__iomem	*message_rbuffer;	/*offset 0x0000ff00:1280,1281,1282,1283,...,1407: iop data to user space 128bytes */ 
};

struct MessageUnit_C{
	uint32_t	message_unit_status;			/*0000 0003*/
	uint32_t	slave_error_attribute;			/*0004 0007*/
	uint32_t	slave_error_address;			/*0008 000B*/
	uint32_t	posted_outbound_doorbell;		/*000C 000F*/
	uint32_t	master_error_attribute;			/*0010 0013*/
	uint32_t	master_error_address_low;		/*0014 0017*/
	uint32_t	master_error_address_high;		/*0018 001B*/
	uint32_t	hcb_size;				/*001C 001F*/
	uint32_t	inbound_doorbell;			/*0020 0023*/
	uint32_t	diagnostic_rw_data;			/*0024 0027*/
	uint32_t	diagnostic_rw_address_low;		/*0028 002B*/
	uint32_t	diagnostic_rw_address_high;		/*002C 002F*/
	uint32_t	host_int_status;			/*0030 0033*/
	uint32_t	host_int_mask;				/*0034 0037*/
	uint32_t	dcr_data;				/*0038 003B*/
	uint32_t	dcr_address;				/*003C 003F*/
	uint32_t	inbound_queueport;			/*0040 0043*/
	uint32_t	outbound_queueport;			/*0044 0047*/
	uint32_t	hcb_pci_address_low;			/*0048 004B*/
	uint32_t	hcb_pci_address_high;			/*004C 004F*/
	uint32_t	iop_int_status;				/*0050 0053*/
	uint32_t	iop_int_mask;				/*0054 0057*/
	uint32_t	iop_inbound_queue_port;			/*0058 005B*/
	uint32_t	iop_outbound_queue_port;		/*005C 005F*/
	uint32_t	inbound_free_list_index;		/*0060 0063*/
	uint32_t	inbound_post_list_index;		/*0064 0067*/
	uint32_t	outbound_free_list_index;		/*0068 006B*/
	uint32_t	outbound_post_list_index;		/*006C 006F*/
	uint32_t	inbound_doorbell_clear;			/*0070 0073*/
	uint32_t	i2o_message_unit_control;		/*0074 0077*/
	uint32_t	last_used_message_source_address_low;	/*0078 007B*/
	uint32_t	last_used_message_source_address_high;	/*007C 007F*/
	uint32_t	pull_mode_data_byte_count[4];		/*0080 008F*/
	uint32_t	message_dest_address_index;		/*0090 0093*/
	uint32_t	done_queue_not_empty_int_counter_timer;	/*0094 0097*/
	uint32_t	utility_A_int_counter_timer;		/*0098 009B*/
	uint32_t	outbound_doorbell;			/*009C 009F*/
	uint32_t	outbound_doorbell_clear;		/*00A0 00A3*/
	uint32_t	message_source_address_index;		/*00A4 00A7*/
	uint32_t	message_done_queue_index;		/*00A8 00AB*/
	uint32_t	reserved0;				/*00AC 00AF*/
	uint32_t	inbound_msgaddr0;			/*00B0 00B3*/
	uint32_t	inbound_msgaddr1;			/*00B4 00B7*/
	uint32_t	outbound_msgaddr0;			/*00B8 00BB*/
	uint32_t	outbound_msgaddr1;			/*00BC 00BF*/
	uint32_t	inbound_queueport_low;			/*00C0 00C3*/
	uint32_t	inbound_queueport_high;			/*00C4 00C7*/
	uint32_t	outbound_queueport_low;			/*00C8 00CB*/
	uint32_t	outbound_queueport_high;		/*00CC 00CF*/
	uint32_t	iop_inbound_queue_port_low;		/*00D0 00D3*/
	uint32_t	iop_inbound_queue_port_high;		/*00D4 00D7*/
	uint32_t	iop_outbound_queue_port_low;		/*00D8 00DB*/
	uint32_t	iop_outbound_queue_port_high;		/*00DC 00DF*/
	uint32_t	message_dest_queue_port_low;		/*00E0 00E3*/
	uint32_t	message_dest_queue_port_high;		/*00E4 00E7*/
	uint32_t	last_used_message_dest_address_low;	/*00E8 00EB*/
	uint32_t	last_used_message_dest_address_high;	/*00EC 00EF*/
	uint32_t	message_done_queue_base_address_low;	/*00F0 00F3*/
	uint32_t	message_done_queue_base_address_high;	/*00F4 00F7*/
	uint32_t	host_diagnostic;			/*00F8 00FB*/
	uint32_t	write_sequence;				/*00FC 00FF*/
	uint32_t	reserved1[34];				/*0100 0187*/
	uint32_t	reserved2[1950];			/*0188 1FFF*/
	uint32_t	message_wbuffer[32];			/*2000 207F*/
	uint32_t	reserved3[32];				/*2080 20FF*/
	uint32_t	message_rbuffer[32];			/*2100 217F*/
	uint32_t	reserved4[32];				/*2180 21FF*/
	uint32_t	msgcode_rwbuffer[256];			/*2200 23FF*/
};

struct InBound_SRB {
	uint32_t addressLow;//pointer to SRB block
	uint32_t addressHigh;
	uint32_t length;// in DWORDs
	uint32_t reserved0;
};

struct OutBound_SRB {
	uint32_t addressLow;//pointer to SRB block
	uint32_t addressHigh;
};

struct MessageUnit_D {
 	struct InBound_SRB post_qbuffer[ARCMSR_MAX_ARC1214_POSTQUEUE];
   	struct OutBound_SRB done_qbuffer[ARCMSR_MAX_ARC1214_DONEQUEUE];
	u16 postq_index;
	u16 doneq_index;
	u32 __iomem *chip_id;			//0x00004
	u32 __iomem *cpu_mem_config;		//0x00008
	u32 __iomem *i2o_host_interrupt_mask;	//0x00034
	u32 __iomem *sample_at_reset;		//0x00100
	u32 __iomem *reset_request;		//0x00108
	u32 __iomem *host_int_status;		//0x00200
	u32 __iomem *pcief0_int_enable;		//0x0020C
	u32 __iomem *inbound_msgaddr0;		//0x00400
	u32 __iomem *inbound_msgaddr1;		//0x00404
	u32 __iomem *outbound_msgaddr0;		//0x00420
	u32 __iomem *outbound_msgaddr1;		//0x00424
	u32 __iomem *inbound_doorbell;		//0x00460
	u32 __iomem *outbound_doorbell;		//0x00480
	u32 __iomem *outbound_doorbell_enable;	//0x00484
	u32 __iomem *inboundlist_base_low;	//0x01000
	u32 __iomem *inboundlist_base_high;	//0x01004
	u32 __iomem *inboundlist_write_pointer;	//0x01018
	u32 __iomem *outboundlist_base_low;	//0x01060
	u32 __iomem *outboundlist_base_high;	//0x01064
	u32 __iomem *outboundlist_copy_pointer;	//0x0106C
	u32 __iomem *outboundlist_read_pointer;	//0x01070 0x01072
	u32 __iomem *outboundlist_interrupt_cause;	//0x1088
	u32 __iomem *outboundlist_interrupt_enable;	//0x108C
	u32 __iomem *message_wbuffer;		//0x2000
	u32 __iomem *message_rbuffer;		//0x2100
	u32 __iomem *msgcode_rwbuffer;		//0x2200
};
/*
*******************************************************************************
*******************************************************************************
*/
struct ARCMSR_CDB                          
{
	uint8_t		Bus;            /* 00h   should be 0 */
	uint8_t		TargetID;       /* 01h   should be 0--15 */
	uint8_t 	LUN;            /* 02h   should be 0--7	*/
	uint8_t		Function;       /* 03h   should be 1 */

	uint8_t		CdbLength;      /* 04h   not used now */
	uint8_t		sgcount;        /* 05h   no used now */
	uint8_t		Flags;          /* 06h			*/
	#define ARCMSR_CDB_FLAG_SGL_BSIZE	0x01   	/* bit 0: 0(256) / 1(512) bytes	*/
	#define ARCMSR_CDB_FLAG_BIOS		0x02   	/* bit 1: 0(from driver) / 1(from BIOS)	*/
	#define ARCMSR_CDB_FLAG_WRITE		0x04	/* bit 2: 0(Data in) / 1(Data out) */
	#define ARCMSR_CDB_FLAG_SIMPLEQ		0x00	/* bit 4/3 ,00 : simple Q,01 : head of Q,10 : ordered Q	*/
	#define ARCMSR_CDB_FLAG_HEADQ		0x08
	#define ARCMSR_CDB_FLAG_ORDEREDQ	0x10
	uint8_t		msgPages;	/* 07h	page length is 256K	*/
	uint32_t	msgContext;	/* 08h	Address of this request	*/
	uint32_t	DataLength;	/* 0ch	*/
	uint8_t		Cdb[16];	/* 10h	SCSI CDB */
	/*
	*******************************************************************************
	**Device Status : the same as SCSI bus if error occur SCSI bus status codes. 
	**Although this is the internal record for the target device status but pratically reflect the condition.
	*******************************************************************************
	*/
	uint8_t     DeviceStatus;	/* 20h   if error	*/
	#define ARCMSR_DEV_SELECT_TIMEOUT	0xF0
	#define ARCMSR_DEV_ABORTED		0xF1
	#define ARCMSR_DEV_INIT_FAIL		0xF2
	#define ARCMSR_DEV_CHECK_CONDITION	0x02	
	uint8_t     SenseData[15];	/* 21h   output	*/        

	union {
  		struct SG32ENTRY	sg32entry[1];	/* 30h...37h 8bytes sg*/
  		struct SG64ENTRY	sg64entry[1];	/* 30h...3Ch 12bytes sg*/
	} u;
};
/*
*******************************************************************************
** Command Control Block (SrbExtension)
** CCB must be not cross page boundary,and the order from offset 0
** structure describing an ATA disk request
** this CCB length must be 32 bytes boundary
*******************************************************************************
*/
struct CommandControlBlock 
{	/*x32:sizeof struct_CCB=(32+60)byte, x64:sizeof struct_CCB=(64+60)byte*/
	struct list_head	list;		/*x32: 8byte, x64: 16byte*/
	struct scsi_cmnd	*pcmd;		/*4 bytes/8 bytes  pointer of linux scsi command */
	struct AdapterControlBlock    *acb;	/*x32: 4byte, x64: 8byte*/
	uint32_t	cdb_phyaddr;		/*x32: 4byte, x64: 4byte*/
	uint32_t	arc_cdb_size;		/*x32:4byte,x64:4byte*/
	uint16_t	ccb_flags;			/*x32: 2byte, x64: 2byte*/
	#define			CCB_FLAG_READ		0x0000
	#define			CCB_FLAG_WRITE		0x0001
	#define			CCB_FLAG_ERROR		0x0002
	#define			CCB_FLAG_FLUSHCACHE	0x0004
	#define			CCB_FLAG_MASTER_ABORTED	0x0008	
	uint16_t	startdone;		/*x32:2byte,x32:2byte*/
	#define			ARCMSR_CCB_DONE		0x0000
	#define			ARCMSR_CCB_START	0x55AA
	#define			ARCMSR_CCB_ABORTED	0xAA55
	#define			ARCMSR_CCB_ILLEGAL	0xFFFF
#if BITS_PER_LONG == 64
	/*  ======================512+64 bytes========================  */
	uint32_t	reserved[5];		/*24 byte*/
#else
	/*  ======================512+32 bytes========================  */
	uint32_t	reserved[1];		/*8  byte*/
#endif
	/*  ==========================================================  */
	struct ARCMSR_CDB	arcmsr_cdb;	/*this ARCMSR_CDB address must be 32 bytes boundary */ 
};
/*
*******************************************************************************
*******************************************************************************
*/
struct AdapterControlBlock
{
	uint8_t		rqbuffer[ARCMSR_MAX_QBUFFER];	/* data collection buffer for read from 80331 */
	uint8_t		wqbuffer[ARCMSR_MAX_QBUFFER];	/* data collection buffer for write to 80331  */
	uint32_t	adapter_type;		/* adapter A,B..... */
	#define ACB_ADAPTER_TYPE_A	0x00000001	/* hba I IOP */
	#define ACB_ADAPTER_TYPE_B	0x00000002	/* hba M IOP */
	#define ACB_ADAPTER_TYPE_C	0x00000004	/* hba P IOP */
	#define ACB_ADAPTER_TYPE_D	0x00000008	/* hba A IOP */
	uint32_t	intmask_org;
	u32		roundup_ccbsize;
	struct pci_dev		*pdev;
	struct Scsi_Host	*host;
	unsigned long		vir2phy_offset;		/* Offset is used in making arc cdb physical to virtual calculations */
	struct msix_entry	entries[ARCMST_NUM_MSIX_VECTORS];
	uint32_t		outbound_int_enable;
	uint32_t		cdb_phyaddr_hi32;
	spinlock_t		eh_lock;
	spinlock_t		ccblist_lock;
	spinlock_t		postq_lock;
	spinlock_t		doneq_lock;
	spinlock_t		rqbuffer_lock;
	spinlock_t		wqbuffer_lock;
	spinlock_t		doorell_lock;
	struct tasklet_struct	isr_tasklet;
	union {
		struct MessageUnit_A __iomem	*pmuA;
		struct MessageUnit_B __iomem	*pmuB;
		struct MessageUnit_C __iomem	*pmuC;
		struct MessageUnit_D __iomem	*pmuD;
	};
	void __iomem *mem_base0;
	void __iomem *mem_base1;
	uint8_t			adapter_index;
	uint8_t			msix_enable;
	uint8_t			irq;            
	uint16_t		acb_flags;
	uint16_t		dev_id;
	uint16_t		subsystem_id;
	struct CommandControlBlock	*pccb_pool[ARCMSR_MAX_FREECCB_NUM];	/* used for memory free */
	struct list_head	ccb_free_list;		/* head of free ccb list */
	struct list_head	list;
	atomic_t		ccboutstandingcount;
	void *			dma_coherent;		/* dma_coherent used for memory free */
	dma_addr_t		dma_coherent_handle;	/* dma_coherent_handle used for memory free */
	dma_addr_t		dma_coherent_handle2;
	void *			dma_coherent2;
	unsigned int	uncache_size;
	uint32_t		rqbuf_firstindex;	/* first of read buffer  */
	uint32_t		rqbuf_lastindex;	/* last of read buffer   */

	uint32_t		wqbuf_firstindex;	/* first of write buffer */
	uint32_t		wqbuf_lastindex;	/* last of write buffer  */
	uint8_t			devstate[ARCMSR_MAX_TARGETID][ARCMSR_MAX_TARGETLUN];	/* id0 ..... id15,lun0...lun7 */
	#define ARECA_RAID_GONE		0x55
	#define ARECA_RAID_GOOD		0xaa
	uint32_t		num_aborts;
	uint32_t		num_resets;
	uint32_t		signature;		/*0,00-03*/
	uint32_t		firm_request_len;	/*1,04-07*/
	uint32_t		firm_numbers_queue;	/*2,08-11*/
	uint32_t		firm_sdram_size;	/*3,12-15*/
	uint32_t		firm_hd_channels;	/*4,16-19*/
	uint32_t		firm_cfg_version;
	char			firm_model[12];		/*15,60-67*/
	char 			firm_version[20];	/*17,68-83*/
	char			device_map[20];		/*21,84-99*/
	struct work_struct	arcmsr_do_message_isr_bh;
	struct timer_list	eternal_timer;
	unsigned short		fw_flag;
	#define	FW_NORMAL	0x0000
	#define	FW_BOG		0x0001
	#define	FW_DEADLOCK	0x0010
	atomic_t		rq_map_token;
	atomic_t		ante_token_value;
	uint32_t		maxOutstanding;
};
/*
*******************************************************************************
*******************************************************************************
*/	
#define 	SCSI_SENSE_CURRENT_ERRORS	0x70
#define 	SCSI_SENSE_DEFERRED_ERRORS	0x71
struct SENSE_DATA 
{
	uint8_t	ErrorCode:7;
	uint8_t	Valid:1;			/* 0 */
	uint8_t	SegmentNumber;			/* 1 */
	uint8_t	SenseKey:4;
	uint8_t	Reserved:1;
	uint8_t	IncorrectLength:1;
	uint8_t	EndOfMedia:1;
	uint8_t	FileMark:1;			/* 2 */
	uint8_t	Information[4];			/* 3 4 5 6 */
	uint8_t	AdditionalSenseLength;		/* 7 */
	uint8_t	CommandSpecificInformation[4];	/* 8 9 10 11 */
	uint8_t	AdditionalSenseCode;		/* 12 */
	uint8_t	AdditionalSenseCodeQualifier;	/* 13 */
	uint8_t	FieldReplaceableUnitCode;	/* 14 */
	uint8_t	SenseKeySpecific[3];		/* 15 16 17 */
};
/* 
*******************************************************************************
**  Peripheral Device Type definitions 
*******************************************************************************
*/
#define SCSI_DASD	0x00	   /* Direct-access Device */
#define SCSI_SEQACESS	0x01	   /* Sequential-access device */
#define SCSI_PRINTER	0x02	   /* Printer device */
#define SCSI_PROCESSOR	0x03	   /* Processor device */
#define SCSI_WRITEONCE	0x04	   /* Write-once device */
#define SCSI_CDROM	0x05	   /* CD-ROM device */
#define SCSI_SCANNER	0x06	   /* Scanner device */
#define SCSI_OPTICAL	0x07	   /* Optical memory device */
#define SCSI_MEDCHGR	0x08	   /* Medium changer device */
#define SCSI_COMM	0x09	   /* Communications device */
#define SCSI_NODEV	0x1F	   /* Unknown or no device type */

#define ARCMSR_PCI2PCI_VENDORID_REG		0x00    /*word*/
#define ARCMSR_PCI2PCI_DEVICEID_REG		0x02    /*word*/

#define ARCMSR_PCI2PCI_PRIMARY_COMMAND_REG	0x04    /*word*/
#define PCI_DISABLE_INTERRUPT			0x0400

#define ARCMSR_PCI2PCI_PRIMARY_STATUS_REG	0x06    /*word: 06,07 */
#define ARCMSR_ADAP_66MHZ			0x20

#define ARCMSR_PCI2PCI_REVISIONID_REG			0x08    /*byte*/
#define ARCMSR_PCI2PCI_CLASSCODE_REG			0x09    /*3bytes*/
#define ARCMSR_PCI2PCI_PRIMARY_CACHELINESIZE_REG	0x0C    /*byte*/
#define ARCMSR_PCI2PCI_PRIMARY_LATENCYTIMER_REG		0x0D    /*byte*/
#define ARCMSR_PCI2PCI_HEADERTYPE_REG			0x0E    /*byte*/
#define ARCMSR_PCI2PCI_PRIMARY_BUSNUMBER_REG		0x18    /*3byte 0x1A,0x19,0x18*/
#define ARCMSR_PCI2PCI_SECONDARY_BUSNUMBER_REG		0x19    /*byte*/
#define ARCMSR_PCI2PCI_SUBORDINATE_BUSNUMBER_REG	0x1A    /*byte*/
#define ARCMSR_PCI2PCI_SECONDARY_LATENCYTIMER_REG	0x1B    /*byte*/
#define ARCMSR_PCI2PCI_IO_BASE_REG			0x1C    /*byte*/
#define ARCMSR_PCI2PCI_IO_LIMIT_REG			0x1D    /*byte*/
#define ARCMSR_PCI2PCI_SECONDARY_STATUS_REG			0x1E    /*word: 0x1F,0x1E */
#define ARCMSR_PCI2PCI_NONPREFETCHABLE_MEMORY_BASE_REG		0x20    /*word: 0x21,0x20 */
#define ARCMSR_PCI2PCI_NONPREFETCHABLE_MEMORY_LIMIT_REG		0x22    /*word: 0x23,0x22 */
#define ARCMSR_PCI2PCI_PREFETCHABLE_MEMORY_BASE_REG		0x24    /*word: 0x25,0x24 */
#define ARCMSR_PCI2PCI_PREFETCHABLE_MEMORY_LIMIT_REG		0x26    /*word: 0x27,0x26 */
#define ARCMSR_PCI2PCI_PREFETCHABLE_MEMORY_BASE_UPPER32_REG	0x28    /*dword: 0x2b,0x2a,0x29,0x28 */
#define ARCMSR_PCI2PCI_PREFETCHABLE_MEMORY_LIMIT_UPPER32_REG	0x2C    /*dword: 0x2f,0x2e,0x2d,0x2c */
#define ARCMSR_PCI2PCI_CAPABILITIES_POINTER_REG			0x34    /*byte*/ 
#define ARCMSR_PCI2PCI_PRIMARY_INTERRUPT_LINE_REG		0x3C    /*byte*/ 
#define ARCMSR_PCI2PCI_PRIMARY_INTERRUPT_PIN_REG		0x3D    /*byte*/ 
#define ARCMSR_PCI2PCI_BRIDGE_CONTROL_REG			0x3E    /*word*/ 

#define ARCMSR_ATU_VENDOR_ID_REG	0x00    /*word*/
#define ARCMSR_ATU_DEVICE_ID_REG	0x02    /*word*/
#define ARCMSR_ATU_COMMAND_REG		0x04    /*word*/
#define ARCMSR_ATU_STATUS_REG		0x06    /*word*/
#define ARCMSR_ATU_REVISION_REG		0x08    /*byte*/
#define ARCMSR_ATU_CLASS_CODE_REG	0x09    /*3bytes 0x0B,0x0A,0x09*/
#define ARCMSR_ATU_CACHELINE_SIZE_REG	0x0C    /*byte*/
#define ARCMSR_ATU_LATENCY_TIMER_REG	0x0D    /*byte*/
#define ARCMSR_ATU_HEADER_TYPE_REG	0x0E    /*byte*/
#define ARCMSR_ATU_BIST_REG		0x0F    /*byte*/


#define ARCMSR_INBOUND_ATU_BASE_ADDRESS0_REG			0x10    /*dword 0x13,0x12,0x11,0x10*/
#define ARCMSR_INBOUND_ATU_MEMORY_PREFETCHABLE			0x08
#define ARCMSR_INBOUND_ATU_MEMORY_WINDOW64			0x04
#define ARCMSR_INBOUND_ATU_UPPER_BASE_ADDRESS0_REG		0x14    /*dword 0x17,0x16,0x15,0x14*/
#define ARCMSR_INBOUND_ATU_BASE_ADDRESS1_REG			0x18    /*dword 0x1B,0x1A,0x19,0x18*/
#define ARCMSR_INBOUND_ATU_UPPER_BASE_ADDRESS1_REG		0x1C    /*dword 0x1F,0x1E,0x1D,0x1C*/
#define ARCMSR_INBOUND_ATU_BASE_ADDRESS2_REG			0x20    /*dword 0x23,0x22,0x21,0x20*/
#define ARCMSR_INBOUND_ATU_UPPER_BASE_ADDRESS2_REG		0x24    /*dword 0x27,0x26,0x25,0x24*/
#define ARCMSR_ATU_SUBSYSTEM_VENDOR_ID_REG			0x2C    /*word 0x2D,0x2C*/
#define ARCMSR_ATU_SUBSYSTEM_ID_REG				0x2E    /*word 0x2F,0x2E*/
#define ARCMSR_EXPANSION_ROM_BASE_ADDRESS_REG			0x30    /*dword 0x33,0x32,0v31,0x30*/
#define ARCMSR_EXPANSION_ROM_ADDRESS_DECODE_ENABLE		0x01    
#define ARCMSR_ATU_CAPABILITY_PTR_REG				0x34    /*byte*/
#define ARCMSR_ATU_INTERRUPT_LINE_REG				0x3C    /*byte*/
#define ARCMSR_ATU_INTERRUPT_PIN_REG				0x3D    /*byte*/
#define ARCMSR_ATU_MINIMUM_GRANT_REG				0x3E    /*byte*/
#define ARCMSR_ATU_MAXIMUM_LATENCY_REG				0x3F    /*byte*/
#define ARCMSR_INBOUND_ATU_LIMIT0_REG				0x40    /*dword 0x43,0x42,0x41,0x40*/
#define ARCMSR_INBOUND_ATU_TRANSLATE_VALUE0_REG			0x44    /*dword 0x47,0x46,0x45,0x44*/
#define ARCMSR_EXPANSION_ROM_LIMIT_REG				0x48    /*dword 0x4B,0x4A,0x49,0x48*/
#define ARCMSR_EXPANSION_ROM_TRANSLATE_VALUE_REG		0x4C    /*dword 0x4F,0x4E,0x4D,0x4C*/
#define ARCMSR_INBOUND_ATU_LIMIT1_REG				0x50    /*dword 0x53,0x52,0x51,0x50*/
#define ARCMSR_INBOUND_ATU_LIMIT2_REG				0x54    /*dword 0x57,0x56,0x55,0x54*/
#define ARCMSR_INBOUND_ATU_TRANSLATE_VALUE2_REG			0x58    /*dword 0x5B,0x5A,0x59,0x58*/
#define ARCMSR_OUTBOUND_IO_WINDOW_TRANSLATE_VALUE_REG		0x5C    /*dword 0x5F,0x5E,0x5D,0x5C*/
#define ARCMSR_OUTBOUND_MEMORY_WINDOW_TRANSLATE_VALUE0_REG	0x60    /*dword 0x63,0x62,0x61,0x60*/
#define ARCMSR_OUTBOUND_UPPER32_MEMORY_WINDOW_TRANSLATE_VALUE0_REG	0x64    /*dword 0x67,0x66,0x65,0x64*/
#define ARCMSR_OUTBOUND_MEMORY_WINDOW_TRANSLATE_VALUE1_REG	0x68    /*dword 0x6B,0x6A,0x69,0x68*/
#define ARCMSR_OUTBOUND_UPPER32_MEMORY_WINDOW_TRANSLATE_VALUE1_REG	0x6C    /*dword 0x6F,0x6E,0x6D,0x6C*/
#define ARCMSR_OUTBOUND_UPPER32_DIRECT_WINDOW_TRANSLATE_VALUE_REG	0x78    /*dword 0x7B,0x7A,0x79,0x78*/
#define ARCMSR_ATU_CONFIGURATION_REG				0x80    /*dword 0x83,0x82,0x81,0x80*/
#define ARCMSR_PCI_CONFIGURATION_STATUS_REG		        0x84    /*dword 0x87,0x86,0x85,0x84*/
#define ARCMSR_ATU_INTERRUPT_STATUS_REG		          	0x88    /*dword 0x8B,0x8A,0x89,0x88*/
#define ARCMSR_ATU_INTERRUPT_MASK_REG				0x8C    /*dword 0x8F,0x8E,0x8D,0x8C*/
#define ARCMSR_INBOUND_ATU_BASE_ADDRESS3_REG			0x90    /*dword 0x93,0x92,0x91,0x90*/
#define ARCMSR_INBOUND_ATU_UPPER_BASE_ADDRESS3_REG		0x94    /*dword 0x97,0x96,0x95,0x94*/
#define ARCMSR_INBOUND_ATU_LIMIT3_REG				0x98    /*dword 0x9B,0x9A,0x99,0x98*/
#define ARCMSR_INBOUND_ATU_TRANSLATE_VALUE3_REG			0x9C    /*dword 0x9F,0x9E,0x9D,0x9C*/
#define ARCMSR_OUTBOUND_CONFIGURATION_CYCLE_ADDRESS_REG		0xA4    /*dword 0xA7,0xA6,0xA5,0xA4*/
#define ARCMSR_OUTBOUND_CONFIGURATION_CYCLE_DATA_REG		0xAC    /*dword 0xAF,0xAE,0xAD,0xAC*/
#define ARCMSR_VPD_CAPABILITY_IDENTIFIER_REG			0xB8    /*byte*/
#define ARCMSR_VPD_NEXT_ITEM_PTR_REG		          	0xB9    /*byte*/
#define ARCMSR_VPD_ADDRESS_REG		          		0xBA    /*word 0xBB,0xBA*/
#define ARCMSR_VPD_DATA_REG					0xBC    /*dword 0xBF,0xBE,0xBD,0xBC*/
#define ARCMSR_POWER_MANAGEMENT_CAPABILITY_IDENTIFIER_REG	0xC0    /*byte*/
#define ARCMSR_POWER_NEXT_ITEM_PTR_REG		          	0xC1    /*byte*/
#define ARCMSR_POWER_MANAGEMENT_CAPABILITY_REG			0xC2    /*word 0xC3,0xC2*/
#define ARCMSR_POWER_MANAGEMENT_CONTROL_STATUS_REG		0xC4    /*word 0xC5,0xC4*/
#define ARCMSR_PCIX_CAPABILITY_IDENTIFIER_REG			0xE0    /*byte*/
#define ARCMSR_PCIX_NEXT_ITEM_PTR_REG		          	0xE1    /*byte*/
#define ARCMSR_PCIX_COMMAND_REG					0xE2    /*word 0xE3,0xE2*/
#define ARCMSR_PCIX_STATUS_REG					0xE4    /*dword 0xE7,0xE6,0xE5,0xE4*/

#define ARCMSR_MU_INBOUND_MESSAGE_REG0			0x10    /*dword 0x13,0x12,0x11,0x10*/
#define ARCMSR_MU_INBOUND_MESSAGE_REG1			0x14    /*dword 0x17,0x16,0x15,0x14*/
#define ARCMSR_MU_OUTBOUND_MESSAGE_REG0			0x18    /*dword 0x1B,0x1A,0x19,0x18*/
#define ARCMSR_MU_OUTBOUND_MESSAGE_REG1			0x1C    /*dword 0x1F,0x1E,0x1D,0x1C*/
#define ARCMSR_MU_INBOUND_DOORBELL_REG			0x20    /*dword 0x23,0x22,0x21,0x20*/
#define ARCMSR_MU_INBOUND_INTERRUPT_STATUS_REG		0x24    /*dword 0x27,0x26,0x25,0x24*/
#define ARCMSR_MU_INBOUND_INDEX_INT			0x40
#define ARCMSR_MU_INBOUND_QUEUEFULL_INT			0x20
#define ARCMSR_MU_INBOUND_POSTQUEUE_INT			0x10         
#define ARCMSR_MU_INBOUND_ERROR_DOORBELL_INT		0x08
#define ARCMSR_MU_INBOUND_DOORBELL_INT			0x04
#define ARCMSR_MU_INBOUND_MESSAGE1_INT			0x02
#define ARCMSR_MU_INBOUND_MESSAGE0_INT			0x01
#define ARCMSR_MU_INBOUND_INTERRUPT_MASK_REG		0x28    /*dword 0x2B,0x2A,0x29,0x28*/
#define ARCMSR_MU_INBOUND_INDEX_INTMASKENABLE		0x40
#define ARCMSR_MU_INBOUND_QUEUEFULL_INTMASKENABLE	0x20
#define ARCMSR_MU_INBOUND_POSTQUEUE_INTMASKENABLE	0x10         
#define ARCMSR_MU_INBOUND_DOORBELL_ERROR_INTMASKENABLE	0x08
#define ARCMSR_MU_INBOUND_DOORBELL_INTMASKENABLE	0x04
#define ARCMSR_MU_INBOUND_MESSAGE1_INTMASKENABLE	0x02
#define ARCMSR_MU_INBOUND_MESSAGE0_INTMASKENABLE	0x01

#define ARCMSR_MU_OUTBOUND_DOORBELL_REG			0x2C    //dword 0x2F,0x2E,0x2D,0x2C//
#define ARCMSR_MU_OUTBOUND_INTERRUPT_STATUS_REG		0x30    //dword 0x33,0x32,0x31,0x30//
#define ARCMSR_MU_OUTBOUND_PCI_INT			0x10
#define ARCMSR_MU_OUTBOUND_POSTQUEUE_INT		0x08 
#define ARCMSR_MU_OUTBOUND_DOORBELL_INT			0x04 
#define ARCMSR_MU_OUTBOUND_MESSAGE1_INT			0x02 
#define ARCMSR_MU_OUTBOUND_MESSAGE0_INT			0x01 
/*Collect all kind of outbound interrupts of adapter type A*/
#define ARCMSR_MU_OUTBOUND_HANDLE_INT	(ARCMSR_MU_OUTBOUND_MESSAGE0_INT | ARCMSR_MU_OUTBOUND_MESSAGE1_INT | ARCMSR_MU_OUTBOUND_DOORBELL_INT | ARCMSR_MU_OUTBOUND_POSTQUEUE_INT | ARCMSR_MU_OUTBOUND_PCI_INT)
#define ARCMSR_MU_OUTBOUND_INTERRUPT_MASK_REG		0x34    //dword 0x37,0x36,0x35,0x34//
#define ARCMSR_MU_OUTBOUND_PCI_INTMASKENABLE		0x10
#define ARCMSR_MU_OUTBOUND_POSTQUEUE_INTMASKENABLE	0x08
#define ARCMSR_MU_OUTBOUND_DOORBELL_INTMASKENABLE	0x04
#define ARCMSR_MU_OUTBOUND_MESSAGE1_INTMASKENABLE	0x02
#define ARCMSR_MU_OUTBOUND_MESSAGE0_INTMASKENABLE	0x01 
#define ARCMSR_MU_OUTBOUND_ALL_INTMASKENABLE		0x1F 
/*
**************************************************************************
**************************************************************************
*/
#define ARCMSR_MU_INBOUND_QUEUE_PORT_REG	0x40    //dword 0x43,0x42,0x41,0x40//
#define ARCMSR_MU_OUTBOUND_QUEUE_PORT_REG	0x44    //dword 0x47,0x46,0x45,0x44//

#define ARCMSR_MU_CONFIGURATION_REG		0xFFFFE350        
#define ARCMSR_MU_CIRCULAR_QUEUE_SIZE64K	0x0020    
#define ARCMSR_MU_CIRCULAR_QUEUE_SIZE32K	0x0010
#define ARCMSR_MU_CIRCULAR_QUEUE_SIZE16K	0x0008   
#define ARCMSR_MU_CIRCULAR_QUEUE_SIZE8K		0x0004   
#define ARCMSR_MU_CIRCULAR_QUEUE_SIZE4K		0x0002    
#define ARCMSR_MU_CIRCULAR_QUEUE_ENABLE		0x0001        /*0:disable 1:enable*/
#define ARCMSR_MU_QUEUE_BASE_ADDRESS_REG	0xFFFFE354   
#define ARCMSR_MU_INBOUND_FREE_HEAD_PTR_REG	0xFFFFE360   
#define ARCMSR_MU_INBOUND_FREE_TAIL_PTR_REG	0xFFFFE364  
#define ARCMSR_MU_INBOUND_POST_HEAD_PTR_REG	0xFFFFE368
#define ARCMSR_MU_INBOUND_POST_TAIL_PTR_REG	0xFFFFE36C
#define ARCMSR_MU_LOCAL_MEMORY_INDEX_REG	0xFFFFE380    /*1004 dwords 0x0050....0x0FFC, 4016 bytes 0x0050...0x0FFF*/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 26)
	static ssize_t arcmsr_show_firmware_info(struct class_device *dev, char *buf)
	{
		struct Scsi_Host *host = class_to_shost(dev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
		unsigned long flags = 0;
		ssize_t len;

		spin_lock_irqsave(acb->host->host_lock, flags);
		len = snprintf(buf, PAGE_SIZE,
					"=================================\n"
					"Firmware Version:  		%s\n"
					"Adapter Model:			%s\n"
					"Reguest Lenth:			%4d\n"
					"Numbers of Queue:		%4d\n"
					"SDRAM Size:			%4d\n"
					"IDE Channels:			%4d\n"
					"=================================\n",
					acb->firm_version,
					acb->firm_model,
					acb->firm_request_len,
					acb->firm_numbers_queue,
					acb->firm_sdram_size,
					acb->firm_hd_channels);
		spin_unlock_irqrestore(acb->host->host_lock, flags);
		return len;
	}

	static ssize_t arcmsr_show_driver_state(struct class_device *dev, char *buf)
	{
		struct Scsi_Host *host = class_to_shost(dev);
		struct AdapterControlBlock *acb = (struct AdapterControlBlock *)host->hostdata;
		unsigned long flags = 0;
		ssize_t len;

		spin_lock_irqsave(acb->host->host_lock, flags);
		len = snprintf(buf, PAGE_SIZE, 
				"=================================\n"
				"ARCMSR: %s\n"
				"Current commands posted:     	%4d\n"
				"Max commands posted:         	%4d\n"
				"Max sgl length:              		%4d\n"
				"Max sector count:            		%4d\n"
				"SCSI Host Resets:            		%4d\n"
				"SCSI Aborts/Timeouts:        	%4d\n"
				"=================================\n",
				ARCMSR_DRIVER_VERSION,
				atomic_read(&acb->ccboutstandingcount),
				ARCMSR_MAX_OUTSTANDING_CMD,
				acb->host->sg_tablesize,
				acb->host->max_sectors,
				acb->num_resets,
				acb->num_aborts);
		spin_unlock_irqrestore(acb->host->host_lock, flags);
		return len;
	}

	static struct class_device_attribute arcmsr_firmware_info_attr =
	{
		.attr = {
			.name = "firmware_info",
			.mode = S_IRUGO, 
		},
		.show	= arcmsr_show_firmware_info,
	};

	static struct class_device_attribute arcmsr_driver_state_attr =
	{
		.attr = {
			.name = "driver_state",
			.mode = S_IRUGO,
		},
		.show = arcmsr_show_driver_state
	};

	static struct class_device_attribute *arcmsr_scsi_host_attr[] =
	{
		&arcmsr_firmware_info_attr,
		&arcmsr_driver_state_attr,
		NULL
	};
#endif
