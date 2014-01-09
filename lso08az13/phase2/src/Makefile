-include ../../Makefile.common
GCC_OPTIONS_PLUS =	-ansi -pedantic -Wall -O2
GCC_OPTIONS =	-ansi -pedantic -Wall -O0
ARCH =	$(shell uname -m)

HDIR =	../../h
EDIR =	../../e
TESTDIR = ../test

HDEF =	$(HDIR)/const.h $(HDIR)/types.h $(HDIR)/mikonos.h \
	$(HDIR)/our_const.h

EDEF =	$(EDIR)/tid.e $(EDIR)/tcb.e \
	$(I)/libumps.e $(EDIR)/scheduler.e $(EDIR)/interrupt.e \
	$(EDIR)/syscall.e $(EDIR)/tlbtrap.e $(EDIR)/prgtrap.e

ifeq ($(ARCH),ppc)
	MIPS_GCC = $(B)/mips-linux-gcc
else
	MIPS_GCC = $(B)/mipsel-linux-gcc
endif

all: phase2

##### Phase 2 #####

phase2: boot.o scheduler.o ssi.o interrupt.o syscall.o tlbtrap.o prgtrap.o

boot.o:	boot.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c boot.c

scheduler.o: scheduler.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c scheduler.c

ssi.o: ssi.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c ssi.c

tlbtrap.o: tlbtrap.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c tlbtrap.c

prgtrap.o: prgtrap.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c prgtrap.c

interrupt.o: interrupt.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c interrupt.c

syscall.o: syscall.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c syscall.c

clean:
	rm -f *.o

.PHONY:	clean
