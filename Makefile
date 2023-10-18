#
# Makefile for the linux NOVA filesystem routines.
#

obj-m += nova.o

nova-y := balloc.o bbuild.o dax.o dir.o file.o inode.o ioctl.o \
		  journal.o namei.o stats.o super.o symlink.o \
		  sysfs.o wprotect.o hash.o writecount.o mappingtable.o \
		  wearleveling.o detect.o record.o detect_time.o freelist.o \
		  average.o utility.o occupySpace.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=`pwd`

clean:
	make -C /lib/modules/$(shell uname -r)/build M=`pwd` clean
