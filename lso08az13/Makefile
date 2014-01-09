-include Makefile.common
ELF32 = elf32ltsmip.h.umpscore.x 
LINKER = mipsel-linux-ld
MKDEV = umps-mkdev
ELF2UMPS = umps-elf2umps

TESTDIR_1 = phase1/test
SRCDIR_1 = phase1/src
TESTDIR_2 = phase2/test
SRCDIR_2 = phase2/src

############
#  Phase 1 #
############

phase1: clean src_b_dir
	#########################################
	# Compilazione moduli phase1 completata #
	#########################################

####################
#  Phase 1A - Test #
####################

# test 1a #

p1atest: clean test_a_dir src_a_dir linkaggio_test_a disk0.umps
	##########################################
	# Compilazione test 1 phase1A completata #
	##########################################

pointer:
	# creazione link simbolici
	ln -sf $(SRCDIR_1)/tcb.o tcb
	ln -sf $(SRCDIR_1)/tid.o tid

pointer_1ab:
	ln -sf $(TESTDIR_1)/p1atest.o p1a
	ln -sf $(TESTDIR_1)/p1btest.o p1b

linkaggio_test_a: pointer pointer_1ab
	# linkaggio moduli
	$(B)/$(LINKER) -T $(X)/$(ELF32) $(L)/crtso.o $(L)/libumps.o tcb tid p1a -o kernel

disk0.umps: tape0.umps
	# creazione disk0
	$(B)/$(MKDEV) -d $@

tape0.umps: kernel.core.umps
	# creazione tape0
	$(B)/$(MKDEV) -t $@ $<

kernel.core.umps:
	# conversione da ELF a umps
	$(B)/$(ELF2UMPS) -k kernel

test_a_dir:
	# compilazione test phase1a
	cd $(TESTDIR_1) && make phase1A

src_a_dir:
	# compilazione moduli
	cd $(SRCDIR_1) && make phase1A

# test 1b #

p1btest: clean test_a_dir src_a_dir linkaggio_test_b disk0.umps
	##########################################
	# Compilazione test 2 phase1A completata #
	##########################################

linkaggio_test_b: pointer pointer_1ab
	# linkaggio moduli
	$(B)/$(LINKER) -T $(X)/$(ELF32) $(L)/crtso.o $(L)/libumps.o tcb tid p1b -o kernel

###################
# Phase 1B - Test #
###################

p1b2test: clean test_b_dir src_b_dir linkaggio_test_1b disk0.umps
	########################################
	# Compilazione test phase1B completata #
	########################################

pointer_1b2:
	# creazione link simbolici
	ln -sf $(SRCDIR_1)/boot.o boot
	ln -sf $(TESTDIR_1)/p1b2test.o p1b2

linkaggio_test_1b: pointer_1b2 pointer
	# linkaggio moduli
	$(B)/$(LINKER) -T $(X)/$(ELF32) $(L)/crtso.o $(L)/libumps.o tcb tid boot p1b2 -o kernel
	
test_b_dir:
	# compilazione test phase1b
	cd $(TESTDIR_1) && make phase1B

src_b_dir:
	# compilazione moduli
	cd $(SRCDIR_1) && make all 

###########
# Phase 2 #
###########

phase2: clean src_2
	#########################################
	# Compilazione moduli phase2 completata #
	#########################################

p2test: clean src_a_dir src_2 test_2 linkaggio_test_2 disk0.umps
	#######################################
	# Compilazione test phase2 completata #
	#######################################

src_2:
	# compilazione moduli
	cd $(SRCDIR_2) && make all

test_2:
	# compilazione moduli
	cd $(TESTDIR_2) && make all

pointer_2:
	# creazione link simbolici
	ln -sf $(SRCDIR_2)/boot.o boot
	ln -sf $(SRCDIR_2)/scheduler.o scheduler
	ln -sf $(SRCDIR_2)/interrupt.o inter
	ln -sf $(SRCDIR_2)/syscall.o sys
	ln -sf $(SRCDIR_2)/ssi.o ssi
	ln -sf $(SRCDIR_2)/prgtrap.o prg
	ln -sf $(SRCDIR_2)/tlbtrap.o tlb
	ln -sf $(TESTDIR_2)/p2test.o p2test

linkaggio_test_2: pointer pointer_2
	# linkaggio moduli
	$(B)/$(LINKER) -T $(X)/$(ELF32) $(L)/crtso.o $(L)/libumps.o tcb tid boot p2test scheduler inter sys ssi prg tlb -o kernel
	
####################
# Pulizia Progetto #
####################

clean:
# pulizia progetto
	rm -rf *.umps
	rm -rf kernel
	rm -rf p1* tcb tid boot scheduler inter sys ssi prg tlb p2test
	cd $(SRCDIR_1) && make clean
	cd $(TESTDIR_1) && make clean
	cd $(SRCDIR_2) && make clean
	cd $(TESTDIR_2) && make clean
	rm -rf *.o

#########################
# Generazione pacchetto #
#########################

compressed: clean
	# generazione pacchetto
	cd ../ && tar cvf lso08az13.phase2.tar lso08az13 && gzip lso08az13.phase2.tar
	###############################
	# Creazione tar.gz completata #
	###############################

.PHONY:	clean compressed p1atest p1btest p1b2test p2test
