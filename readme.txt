Copy this folder (arcmsr) to /usr/src/linux/drivers/scsi

Edit:


###############################################################################
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version lower than 2.6.0
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/usr/src/linux/drivers/scsi/Makefile
...
....

subdir-$(CONFIG_SCSI_ARCMSR)	+= arcmsr
obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/arcmsr.o

....
...

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version higher than 2.6.0
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/usr/src/linux/drivers/scsi/Makefile

...
....

obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/

....
...

###############################################################################


###############################################################################
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version lower than 2.6.0
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/usr/src/linux/drivers/scsi/Config.in

...
....

if [ "$CONFIG_PCI" = "y" ]; then
   dep_tristate 'ARECA (ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Controller SCSI support' CONFIG_SCSI_ARCMSR $CONFIG_SCSI
fi

....
...

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version higher than 2.6.0
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
/usr/src/linux/drivers/scsi/Kconfig

...
....

config SCSI_ARCMSR
	tristate "ARECA (ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Controller"
	depends on  PCI && SCSI
	help
	  This driver supports all of ARECA's SATA RAID controllers cards.  
	  This is an ARECA maintained
	  driver by Erich Chen.  <If you have any problems, please mail to: erich@areca.com.tw >.

	  To compile this driver as a module, choose M here: the
	  module will be called arcmsr (modprobe arcmsr) .	

....
...

###############################################################################

Make new Linux kernel......