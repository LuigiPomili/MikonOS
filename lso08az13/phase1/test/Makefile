-include ../../Makefile.common
GCC_OPTIONS =	-ansi -pedantic -Wall -c
ARCH =	$(shell uname -m)

HDIR =	../../h
EDIR =	../../e

HDEF =	$(HDIR)/const.h $(HDIR)/types.h $(HDIR)/mikonos.h \
	$(HDIR)/our_const.h $(HDIR)/base.h

EDEF =	$(EDIR)/tid.e ./p1b2test.e $(EDIR)/tcb.e \
	$(I)/libumps.e 

ifeq ($(ARCH),ppc)
	MIPS_GCC = $(B)/mips-linux-gcc
else
	MIPS_GCC = $(B)/mipsel-linux-gcc
endif

##### Phase 1A  ##### 

phase1A: p1atest.o p1btest.o

p1atest.o: p1atest.c $(EDEF) $(HDEF)

	$(MIPS_GCC) $(GCC_OPTIONS) p1atest.c

p1btest.o: p1btest.c $(EDEF) $(HDEF)

	$(MIPS_GCC) $(GCC_OPTIONS) p1btest.c

##### Phase 1B #####

phase1B: p1b2test.o

p1b2test.o: p1b2test.c $(EDEF) $(HDEF)

	$(MIPS_GCC) -I$(I) $(GCC_OPTIONS) p1b2test.c

clean:
	rm -f *.o

.PHONY:	clean phase1A phase1B
