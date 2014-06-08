Copy this folder (arcmsr) to /usr/src/linux/drivers/scsi

Edit files:


###############################################################################
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version lower than 2.3.x
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/drivers/scsi/Makefile
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         @
         @  ifeq ($(CONFIG_SCSI_AIC7XXX),y)
         @  L_OBJS += aic7xxx.o
         @  else
         @    ifeq ($(CONFIG_SCSI_AIC7XXX),m)
         @    M_OBJS += aic7xxx.o
         @    endif
         @  endif
         @
         @  ifeq ($(CONFIG_SCSI_ARCMSR),y)              <----new
         @  L_OBJS += arcmsr.o                          <----new
         @  else                                        <----new
         @    ifeq ($(CONFIG_SCSI_ARCMSR),m)            <----new
         @    M_OBJS += arcmsr.o                        <----new
         @    endif                                     <----new
         @  endif                                       <----new
         @
         @  ifeq ($(CONFIG_SCSI_DC390T),y)
         @  L_OBJS += tmscsim.o
         @  else
         @    ifeq ($(CONFIG_SCSI_DC390T),m)
         @    M_OBJS += tmscsim.o
         @    endif
         @  endif
         @
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/drivers/scsi/Config.in
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         @
         @  if [ "$CONFIG_PCI" = "y" ]; then
         @    dep_tristate 'Qlogic ISP SCSI support' CONFIG_SCSI_QLOGIC_ISP $CONFIG_SCSI
         @    dep_tristate 'Qlogic ISP FC SCSI support' CONFIG_SCSI_QLOGIC_FC $CONFIG_SCSI
         @  fi
         @  dep_tristate 'Seagate ST-02 and Future Domain TMC-8xx SCSI support' CONFIG_SCSI_SEAGATE $CONFIG_SCSI
         @  dep_tristate 'ARECA (ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Controller' CONFIG_SCSI_ARCMSR $CONFIG_SCSI       <----new
         @  ....
         @  ..
         @  .
         @  dep_tristate 'Trantor T128/T128F/T228 SCSI support' CONFIG_SCSI_T128 $CONFIG_SCSI
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/drivers/scsi/hosts.c
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         @  #ifdef CONFIG_SCSI_SYM53C416 
         @  #include "sym53c416.h" 
         @  #endif
         @
         @  #ifdef CONFIG_SCSI_ARCMSR             <----new
         @  #include "arcmsr.h"                   <----new
         @  #endif                                <----new
         @
         @  #ifdef CONFIG_SCSI_DC390T           
         @  #include "dc390.h"                  
         @  #endif                              
         @ 
         @  #ifdef CONFIG_SCSI_AM53C974
         @  #include "AM53C974.h"
         @  #endif
         @  -----------------------
         @       ----
         @       ---
         @       --
         @  -----------------------
         @
         @  #ifdef CONFIG_SCSI_IBMMCA
         @      IBMMCA,
         @  #endif
         @  #ifdef CONFIG_SCSI_EATA
         @      EATA,
         @  #endif
         @  #ifdef CONFIG_SCSI_ARCMSR             <----new
         @      ARCMSR,                           <----new
         @  #endif                                <----new
         @  #ifdef CONFIG_SCSI_DC390T
         @      DC390_T,
         @  #endif
         @  #ifdef CONFIG_SCSI_AM53C974
         @      AM53C974,
         @  #endif
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/include/linux/proc_fs.h
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         @
         @	PROC_SCSI_FD_MCS,
         @	PROC_SCSI_EATA2X,
         @	PROC_SCSI_ARCMSR,                     <----new
         @	PROC_SCSI_DC390T,
         @	PROC_SCSI_AM53C974,
         @
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version between 2.5.x and 2.3.x
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/drivers/scsi/Makefile
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 @ ...
	 @ ....
         @ 
	 @ subdir-$(CONFIG_SCSI_ARCMSR)	+= arcmsr
	 @ obj-$(CONFIG_SCSI_ARCMSR)	+= arcmsr/arcmsr.o
         @
	 @ ....
	 @ ...
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

     /usr/src/linux/drivers/scsi/Config.in
         @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	 @ ...
	 @ ....
         @ 
	 @ if [ "$CONFIG_PCI" = "y" ]; then
	 @ dep_tristate 'ARECA (ARC1110/1120/1130/1160/1210/1220/1230/1260) SATA RAID HOST Controller SCSI support' CONFIG_SCSI_ARCMSR $CONFIG_SCSI
	 @ fi
         @ 
	 @ ....
	 @ ...
	 @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		 
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
If your linux kernel version higher than 2.6.0
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

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