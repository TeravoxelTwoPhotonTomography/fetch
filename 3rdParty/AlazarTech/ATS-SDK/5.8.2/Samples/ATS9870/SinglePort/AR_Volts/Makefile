
CC := gcc
PLX_OPT := $(PLX_OPT) -DPCI_CODE -DPLX_LITTLE_ENDIAN -DPLX_LINUX
# DEBUG := 1
ifdef DEBUG
   CC_OPT := $(CC_OPT) -g3
   PLX_OPT := $(PLX_OPT) -DPLX_DEBUG
else
   CC_OPT := $(CC_OPT) -O2
endif
INC_DIR := -I../../../include 
CFLAGS := $(CFLAGS) -c -Wall $(CC_OPT) $(PLX_OPT) $(INC_DIR)
LIBS := $(LIBS) -lPlxApi -L/usr/local/AlazarTech/lib -lcurses

PROG = AcqToDisk
OBJS = AcqToDisk.o Utils.o

$(PROG) : $(OBJS)
	$(CC) -o $(PROG) $(LIBS) $(OBJS)

Acq2Disk.o : AcqToDisk.c
	$(CC) -c $(CFLAGS) AcqToDisk.c

Utils.o : Utils.c
	$(CC) -c $(CFLAGS) Utils.c

clean : 
	rm -f $(PROG) *.o *~ 


