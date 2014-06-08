Driver User Guide for ARC-11XX/ARC12XX/ARC16XX/18XX RAID Controller
=================================================================================
=================================================================================
Contact 
mail address: support@areca.com.tw
Tel: 886-2-8797-4060 Ext.223
Fax: 886-2-8797-5970
Web site: www.areca.com.tw
=================================================================================
=================================================================================
 
**********************************************************************************
** 1. Contents									**
**********************************************************************************
	1. readme.txt	- driver user guide for ARC-11XX/12XX/16XX/18XX driver
	2. arcmsr.c	- driver source file for kernel 2.6.X
	3. arcmsr.h	- driver source file for kernel 2.6.X
	4. Makefile	- Makefile for kernel 2.6.X
	5. release note	- change log
**********************************************************************************
** 2. Compile and install arcmsr RAID driver on the running system		**
**********************************************************************************
	Notice!! Before all of steps below, please make sure there are complete development tools in your system.
	If not, the driver module can not be built successfully.
	2.1 make binary driver file,
		#make
	2.2 insert the driver on your system,
		#insmod arcmsr.ko,
	2.3 if you need the driver being inserted automatically after every reboot,
		make a new initrd image with the driver as following steps.
        Example:
		#cp arcmsr.ko /lib/modules/`uname -r`/kernel/drivers/scsi/arcmsr/
		#vi /etc/modprobe.conf, and add "alias scsi_hostadapter arcmsr"
		#mkinitrd -f -v /boot/initrd-2.6.18-8.el5.custom.img 2.6.18-8.el5
	2.4 Add new items for new kernel into /boot/grub/grub.conf, and designate new kernel
		as the bootup default item,
		#vi /etc/grub.conf
		Example:
		@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		@ default=0
		@ timeout=5
		@ ......
		@ title Red Hat Linux (2.6.18-8.el5)custom
		@	root (hd0,0)
		@	kernel /vmlinuz-2.6.18-8.el5-custom ro root=/dev/sda1
		@   	initrd /initrd-2.6.18-8.el5-custom.img
		@ title Red Hat Linux (2.6.18-8.el5)
		@	root (hd0,0)
		@	kernel /vmlinuz-2.6.18-8.el5 ro root=/dev/sda1
		@	initrd /initrd-2.6.18-8.el5.img
		@ ......
		@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	2.11 Reboot,
		#shutdown -r now
Note:
	If you need to increase the timeout value to certain number such as 90 seconds or 120 seconds after consulting the FAE,
	please insert driver module with "insmod arcmsr.ko timeout=90" or do "echo 90 > /sys/module/arcmsr/parameters/timeout"
	after inserting driver modules.