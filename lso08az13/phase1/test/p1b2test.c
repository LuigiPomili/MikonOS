/*
 *  This file is part of MIKONOS.
 *  Copyright (C) 2004, 2005, 2008 Michael Goldweber, Davide Brini,
 *  Renzo Davoli, Ludovico Gardenghi.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Phase 1B "the real one" test.
 * Thu Feb 14 2008
 *
 * How to use:
 *
 * - include "p1b2test.e" where you have your boot function
 * - 4 exception handlers are exported, you should use them:
 *   * sys_bp_handler (for SYSCALL/BREAKPOINT)
 *   * trap_handler (for Program Traps)
 *   * tlb_handler (for TLB exceptions)
 *   * ints_handler (for Interrupt exceptions)
 * - at the end of your boot phase just call schedule().
 * - compile p1b2test.c and link p1b2test.o with the rest of your kernel.
 *   Remember to add the path of libumps.e to the -I options of gcc.
 * - output is sent to Terminal 0.
 *
 * There are 9 tests. If everything goes fine, you should read "All tests
 * completed. System halted" at the end. Else, a kernel panic or infinte loop
 * will probably occur. Tests should appear in the correct order (1 to 9).
 *
 * At the beginning, each exception handler prints the difference between the
 * correct initial stack pointer for the handler and the current one. This
 * value should be "small" (i.e. less than 0xff), else there could be errors
 * in your stack pointer settings.
 *
 * Bug reports to garden@cs.unibo.it. :-)
 *
 */

#include "../../h/const.h"
#include "../../h/types.h"

/* Needs -I/usr/include/uMPS */
#include "libumps.e"

#include "../../h/mikonos.h"

HIDDEN char okbuf[2048];			/* sequence of progress messages */
HIDDEN char *mp = okbuf;

#define TRANSMITTED	5
#define TRANSTATUS	2
#define ACK			1
#define PRINTCHR	2
#define CHAROFFSET	8
#define STATUSMASK	0xFF
#define TERM0ADDR	0x10000250
#define DEVREGSIZE	16       /* device register size in bytes */
#define READY		1
#define DEVREGLEN	4
#define TRANCOMMAND	3
#define BUSY		3

typedef unsigned int devreg;

/* This function returns the terminal transmitter status value given its address */ 
HIDDEN devreg termstat(memaddr *stataddr)
{
	return((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if 
 * print was successful, FALSE if not   */
unsigned int termprint(char * str, unsigned int term)
{
	memaddr *statusp;
	memaddr *commandp;

	devreg stat;
	devreg cmd;

	unsigned int error = FALSE;

	if (term < DEV_PER_INT) {
		/* terminal is correct */
		/* compute device register field addresses */
		statusp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
		commandp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));

		/* test device status */
		stat = termstat(statusp);
		if ((stat == READY) || (stat == TRANSMITTED)) {
			/* device is available */

			/* print cycle */
			while ((*str != '\0') && (!error)) {
				cmd = (*str << CHAROFFSET) | PRINTCHR;
				*commandp = cmd;

				/* busy waiting */
				while ((stat = termstat(statusp)) == BUSY);

				/* end of wait */
				if (stat != TRANSMITTED) {
					error = TRUE;
				} else {
					/* move to next char */
					str++;
				}
			} 
		}	else {
			/* device is not available */
			error = TRUE;
		}
	}	else {
		/* wrong terminal device number */
		error = TRUE;
	}

	return (!error);
}


/* This function places the specified character string in okbuf and
 *	causes the string to be written out to terminal0 */
void addokbuf(char *strp) {
	char *tstrp = strp;

	while ((*mp++ = *strp++) != '\0');
	mp--; 

	termprint(tstrp, 0); 
}

/***************************************************************************/

HIDDEN int test_step = 1;
HIDDEN int prev_step = 0;
HIDDEN int old_step;
HIDDEN state_t test_state;

HIDDEN char *exccodes[] = {
	"External Device Interrupt",
	"TLB-Modification Exception",
	"TLB Invalid Exception: on Load/Fetch",
	"TLB Invalid Exception: on Store",
	"Address Error Exception: on Load/Fetch",
	"Address Error Excetpion: on Store",
	"Bus Error Exception: on Fetch",
	"Bus Error Exception: on Load/Store",
	"Syscall Exception",
	"Breakpoint Exception",
	"Reserved Instruction Exception",
	"Coprocessor Unusable Exception",
	"Arithmetic Overflow Exception",
	"Bad Page Table",
	"Page Table Miss"};

HIDDEN char *interrupts[] = {
	"Software Interrupt 0",
	"Software Interrupt 1",
	"Bus (Interval Timer",
	"Disk Devices",
	"Tape Devices",
	"Network Devices",
	"Printer Devices",
	"Terminal Devices"};

HIDDEN char hex[] = "0123456789abcdef";

HIDDEN void save_test_state(state_t *old)
{
	int i;

	char bit[2];
	bit[1] = '\0';

	test_state.entry_hi = old->entry_hi;
	test_state.cause = old->cause;
	test_state.status = old->status;
	test_state.pc_epc = old->pc_epc;

	for (i = 0; i < 29; i++)
		test_state.gpr[i] = old->gpr[i];

	test_state.hi = old->hi;
	test_state.lo = old->lo;
}

HIDDEN void disableTermInt()
{
	setSTATUS(getSTATUS() & ~0x00008000);
}

HIDDEN void enableTermInt()
{
	setSTATUS(getSTATUS() | 0x00008000);
}


HIDDEN void setUserMode()
{
	setSTATUS(getSTATUS() | 0x00000002);
}

HIDDEN void unaligned_test()
{
	return;
}

void printword(int n)
{
	int i;
	char buf[2] = { '\0', '\0' };
	addokbuf("0x");
	for (i = 7; i >= 0; i--)
	{
		buf[0] = hex[(n >> (i * 4)) & 0x0f];
		addokbuf(buf);
	}
}

HIDDEN void test()
{
	unsigned int n = 0;
	char *p;

	for(;;)
	{
		old_step = prev_step;
		prev_step = test_step;
		test_step = 0;
		switch(prev_step)
		{
			case 0:
				disableTermInt();
				addokbuf("LAST TEST FAILED!\n");
				PANIC();
				break;

			case 1:
				disableTermInt();
				addokbuf("Test begins!\n");
				test_step = prev_step + 1;
				break;

			case 2:
				disableTermInt();
				addokbuf("\n1. Trying terminal interrupt (once and for all).\n");
				enableTermInt();
				disableTermInt();
				break;

			case 3:
				addokbuf("\n2. Trying to fetch at unaligned address.\n");
				STST(&test_state);
				test_state.pc_epc = (memaddr)((char*)unaligned_test + 1);
				test_state.reg_t9 = test_state.pc_epc;
				LDST(&test_state);
				break;

			case 4:
				addokbuf("\n3. Trying division by 0.\n");
				n = 0;
				n = 42 / n;
				break;

			case 5:
				addokbuf("\n4. Trying access at 0xffffffff.\n");
				p = (char*)0xffffffff;
				*p = 'x';
				break;

			case 6:
				addokbuf("\n5. (VM on) Trying TLB usage with wrong page table.\n");
				for (n = SEGTABLE_START; n < (SEGTABLE_START + 0x300); n += 4)
					*((int*)n) = 0xffffffff;
				setSTATUS(getSTATUS() | 0x01000000);
				p = (char*) 0x20001000;
				*p = *p;
				break;

			case 7:
				addokbuf("\n6. Calling syscall 0x42.\n");
				SYSCALL(0x42, 1, 2, 3);
				break;

			case 8:
				addokbuf("\n7. (user mode) Trying privileged instruction (breakpoint expected).\n");
				setUserMode();
				PANIC();
				break;

			case 9:
				addokbuf("\n8. (user mode) Trying coprocessor instruction while not allowed.\n");
				setSTATUS(getSTATUS() & ~0x10000000);
				setUserMode();
				setSTATUS(0);
				break;

			case 10:
				addokbuf("\n9. (VM on) Trying TLB usage with wrong page table.\n");
				for (n = SEGTABLE_START; n < (SEGTABLE_START + 0x300); n += 4)
					*((int*) n) = 0xffffffff;
				setSTATUS(getSTATUS() | 0x01000000);
				p = (char*) 0x20001000;
				*p = *p;
				break;

			case 11:
				addokbuf("\nAll tests completed.\n");
				HALT();
				break; /* never reached */
		}
	}
}

#define ACT_RESET 0
#define ACT_SKIP 1

HIDDEN void update_pc(int action)
{
	test_state.pc_epc = (memaddr) test;
	test_state.reg_t9 = (memaddr) test;
	test_state.reg_sp = RAMTOP - 2048;
}

void schedule()
{
	if (test_step == 1)
	{
		STST(&test_state);
		update_pc(ACT_RESET);
		test_state.status |= STATUS_IEc | STATUS_IEp | STATUS_INT_UNMASKED;
		test_state.status &= ~0x00008000;
	}

	test_state.status &= ~STATUS_KUp;
	test_state.status &= ~STATUS_VMp;

	LDST(&test_state);

	PANIC();
}


HIDDEN void print_exccode(state_t *state)
{
	addokbuf(exccodes[(state->cause & 0x7C) >> 2]);
	addokbuf("@");
	printword(state->pc_epc);
	addokbuf("\n");
}

HIDDEN void print_stack()
{
	state_t tmp;
	STST(&tmp);
	addokbuf("[");
	printword(RAMTOP - tmp.reg_sp);
	addokbuf("] ");
}

void sys_bp_handler()
{
	save_test_state((state_t*) SYSBK_OLDAREA);

	print_stack();
	addokbuf("sys_bp: ");
	print_exccode(&test_state);

	if (((test_state.cause & 0x7C) >> 2) == 8)
	{
		addokbuf("syscall number: ");
		printword(test_state.reg_a0);
		addokbuf("\n");
		update_pc(ACT_SKIP);
	}
	else
		update_pc(ACT_RESET);

	if (old_step == prev_step - 1)
		switch(prev_step)
		{
			case 4:
			case 7:
			case 8:
				test_step = prev_step+1;
				break;
		}

	schedule();
}

void trap_handler()
{
	save_test_state((state_t*) PGMTRAP_OLDAREA);

	print_stack();

	addokbuf("trap: ");
	print_exccode(&test_state);

	update_pc(ACT_RESET);

	if (old_step == prev_step - 1)
		switch (prev_step)
		{
			case 3:
			case 5:
			case 9:
				test_step = prev_step+1;
				break;
		}

	schedule();
}

void tlb_handler()
{
	save_test_state((state_t*) TLB_OLDAREA); 

	print_stack();

	addokbuf("tlb: ");
	print_exccode(&test_state);

	update_pc(ACT_RESET);

	if (old_step == prev_step - 1)
		switch (prev_step)
		{
			case 6:
			case 10:
				test_step = prev_step+1;
				break;
		}

	schedule();
}

void ints_handler()
{
	int intnr;
	save_test_state((state_t*) INT_OLDAREA);

	print_stack();

	for (intnr = 8; (intnr <= 15) && !((test_state.cause >> intnr) & 1); intnr++);

	intnr -= 8;

	addokbuf("ints: ");

	if (intnr > 7)
		PANIC();

	addokbuf(interrupts[intnr]);
	addokbuf("\n");

	print_exccode(&test_state);

	if (intnr == 7)
		(*(memaddr*) 0x1000025C) = ACK;

	if (old_step == prev_step - 1)
		switch(prev_step)
		{
			case 2:
				test_step = prev_step+1;
				break;
		}

	schedule();
}



