Copy this folder (arcmsr) to /usr/src/linux/drivers/scsi

Edit files:



     /usr/src/linux/drivers/scsi/Makefile
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 @ ...
	 @ ....
         @
	 @ obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/
         @
	 @ ....
	 @ ...
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         
     /usr/src/linux/drivers/scsi/Kconfig
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 @ ...
	 @ ....
         @
	 @ config SCSI_ARCMSR	 
         @     tristate "ARECA (ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Controller"	 
         @     depends on  PCI && SCSI
	 @     help
	 @       This driver supports all of ARECA's SATA RAID controllers cards. 
	 @       This is an ARECA maintained	
	 @       driver by Erich Chen.  <If you have any problems, please mail to: erich@areca.com.tw >.
         @
	 @       To compile this driver as a module, choose M here: the
	 @       module will be called arcmsr (modprobe arcmsr) .	
         @
	 @ ....
	 @ ...
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

###############################################################################

Make new Linux kernel......