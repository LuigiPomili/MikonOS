-include ../../Makefile.common
GCC_OPTIONS =	-ansi -pedantic -Wall -c
ARCH =	$(shell uname -m)

HDIR =	../../h
EDIR =	../../e

HDEF =	$(HDIR)/const.h $(HDIR)/types.h $(HDIR)/mikonos.h \
	$(HDIR)/our_const.h $(HDIR)/base.h

EDEF =	$(EDIR)/tid.e $(EDIR)/tcb.e $(I)/libumps.e 

ifeq ($(ARCH),ppc)
	MIPS_GCC = $(B)/mips-linux-gcc
else
	MIPS_GCC = $(B)/mipsel-linux-gcc
endif

all: p2test

##### Phase 2 #####

p2test: p2test.o

p2test.o: p2test.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) p2test.c

clean:
	rm -f *.o

.PHONY:	clean p2test
