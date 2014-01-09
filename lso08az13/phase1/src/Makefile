-include ../../Makefile.common
GCC_OPTIONS_PLUS =	-ansi -pedantic -Wall -O2
GCC_OPTIONS =	-ansi -pedantic -Wall -O0
ARCH =	$(shell uname -m)

HDIR =	../../h
EDIR =	../../e
TESTDIR = ../test

HDEF =	$(HDIR)/const.h $(HDIR)/types.h $(HDIR)/mikonos.h \
	$(HDIR)/our_const.h

EDEF =	$(EDIR)/tid.e $(TESTDIR)/p1b2test.e $(EDIR)/tcb.e $(I)/libumps.e 

ifeq ($(ARCH),ppc)
	MIPS_GCC = $(B)/mips-linux-gcc
else
	MIPS_GCC = $(B)/mipsel-linux-gcc
endif

##### Phase 1A e 1B #####

all: phase1A phase1B

##### Phase 1A - ma non solo ##### 

phase1A: tcb.o tid.o

tcb.o:	tcb.c $(EDEF) $(HDEF)

	$(MIPS_GCC) $(GCC_OPTIONS) -c tcb.c

tid.o:	tid.c $(EDEF) $(HDEF)

	$(MIPS_GCC) $(GCC_OPTIONS) -c tid.c

##### Phase 1B #####

phase1B: boot.o

boot.o:	boot.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) -c boot.c

.PHONY:	clean

clean:
	rm -f *.o
