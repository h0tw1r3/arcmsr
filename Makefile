# File: drivers/arcmsr/Makefile
# Makefile for the ARECA PCI-X PCI-EXPRESS SATA RAID controllers SCSI driver.

obj-$(CONFIG_SCSI_ARCMSR) := arcmsr.o

EXTRA_CFLAGS += -I.


