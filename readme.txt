 Areca (ARC-11XX/12XX/16XX) SATA/SAS RAID Controller Driver Source Code User Guide
===================================================================================================
===================================================================================================
Contact 
mail address: support@areca.com.tw
Tel: 886-2-8797-4060 Ext.223
Fax: 886-2-8797-5970
Web site: www.areca.com.tw
===================================================================================================
===================================================================================================
 
***************************************************************************************************   
** 1. Contents                                                      			 				 **
***************************************************************************************************   
	1. readme.txt							- the guidance for ARC-11XX/12XX/16XX driver
	2. arcmsr.c								- driver source
	3. arcmsr.h								- driver source
	4. kernel-version-2.5.x-2.3.x/arcmsr	- driver folder applicable to kernel-2.4.*
	5. kernel-version-2.6.x/arcmsr			- driver folder applicable to kernel-2.6.*
	6. release note							- change log
***************************************************************************************************   
** 2. Compile and install arcmsr RAID driver on the running system  			 				 **
***************************************************************************************************   
	2.1 select a folder which is applicable to your kernel version 
	2.2 copy that folder to /root, i.e. cp -r kernel-version-* /root
	2.3 cd /root/kernel-version-*/arcmsr, in the folder there is a Makefile file
	2.4 copy arcmsr.c and arcmsr.h to your $PWD 
	2.5 make -C /lib/modules/`uname -r`/build CONFIG_SCSI_ARCMSR=m SUBDIRS=$PWD modules
	2.6 insmod arcmsr.ko to your system, i.e. insmod arcmsr.ko,
      if something wrong, please check the kernel version is appropriate to the folder you select
	  or check the output when you compile the driver. If everything is ok, please go to next step.
	2.7 copy arcmsr.ko to /lib/modules/`uname -r`/kernel/drivers/scsi/
	2.8 make a new initrd image
		An initrd image is needed for loading your SCSI module at boot time.
		So you need update your initrd image.....
		First, insert and add this context description into /etc/modprobe.conf
			Example:
			@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			@       ......
			@   alias scsi_hostadapter arcmsr		<----add this entry
			@       ......
			@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		Second, build up a new initrd image. Assume your kernel version is 2.6.18-53.el5.
			# mkinitrd -f -v /boot/initrd-2.6.18-53.el5.custom.img 2.6.18-53.el5
	2.9 insert and add these entries into /boot/grub/grub.conf
		Example:
		@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		@  ......
		@ title Red Hat Linux (2.6.18-53.el5)
		@	root (hd0,0)										<----This depends on your root partition
		@	kernel /vmlinuz-2.6.18-53.el5 ro root=/dev/hda2		<----This depends on your root partition
		@	initrd /initrd-2.6.18-53.el5.img
		@
		@ title Red Hat Linux (2.6.18-53.el5)custom				<----add this entry
		@	root (hd0,0)										<----add this entry
		@	kernel /vmlinuz-2.6.18-53.el5 ro root=/dev/hda2		<----add this entry which goes with your root partition
		@   initrd /initrd-2.6.18-53.el5.custom.img				<----add this entry
		@  ......
		@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
	2.10 designate the bootup item in /etc/grub.conf  
	2.11 reboot