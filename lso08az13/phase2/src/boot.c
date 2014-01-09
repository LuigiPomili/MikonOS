/*****************************************************************************
 * Main MIKONOS file 
 *
 * Boot del SO ed exception handler.
 *****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce esterne */
#include "../../e/tid.e"
#include "../../e/scheduler.e"
#include "libumps.e"
#include "../../e/interrupt.e"
#include "../../e/prgtrap.e"
#include "../../e/tlbtrap.e"
#include "../../e/syscall.e"

/* implementazioni esterne (extern) */
extern void SSI_start(void);
extern void test(void);

/* Implementazione modulo */

/*
 * alloca il TCB per la SSI e inizializza il suo cpu_snapshot
 */
HIDDEN tid_t initSSI(void) {
	tid_t SSI_tid;
	tcb_t *SSI_p;

	/* allocazione TCB per la SSI */
	SSI_tid = newTcb();
	SSI_p = resolveTid(SSI_tid);

	SSI_p->cpu_snapshot.status = STATUS_CP0 | STATUS_INT_UNMASKED | STATUS_IEp;
	SSI_p->cpu_snapshot.pc_epc = SSI_p->cpu_snapshot.reg_t9 = (memaddr) SSI_start;
	SSI_p->cpu_snapshot.reg_sp = RAMTOP - FRAME_SIZE;

	return SSI_tid;
}

/*
 * alloca il TCB per il test e inizializza il suo cpu_snapshot
 */
HIDDEN tid_t initTest(void) {
	tid_t test_tid;
	tcb_t *test_p;

	/* allocazione TCB per la SSI */
	test_tid = newTcb();
	test_p = resolveTid(test_tid);

	/* interrupt on, kernel mode, VM off */
	test_p->cpu_snapshot.status = STATUS_CP0 | STATUS_INT_UNMASKED | STATUS_IEp;
	test_p->cpu_snapshot.pc_epc = test_p->cpu_snapshot.reg_t9 = (memaddr) test;
	test_p->cpu_snapshot.reg_sp = RAMTOP - (2 * FRAME_SIZE);

	return test_tid;
}

/*
 * inizializza la New Area passata e setta
 * l'handler per l'eccezione corrispondente
 */
HIDDEN void impNewArea(memaddr baseArea, memaddr baseHandler) {
	state_t *attuale = (state_t *) baseArea;
	STST(attuale); /* store del processore */
	attuale->reg_t9 = attuale->pc_epc = baseHandler; /* settaggio dell'handler */
	attuale->reg_sp = RAMTOP;
	attuale->status = STATUS_CP0; /* disabilitazione interrupt, kernel mode, VM off */
}

int main(void) {

	/* Riempimento delle New Areas per la gestione
	 * delle eccezioni */

	/* SYSCALL */
	impNewArea(SYSBK_NEWAREA, (memaddr) sys_bp_handler);

	/* Program TRAP */
	impNewArea(PGMTRAP_NEWAREA, (memaddr) trap_handler);

	/* TLB Management */
	impNewArea(TLB_NEWAREA, (memaddr) tlb_handler);

	/* Interrupt */
	impNewArea(INT_NEWAREA, (memaddr) ints_handler);
	
	/* inizializzazione strutture */
	initTidTable();

	initScheduler();
	addThread(initSSI());
	addThread(initTest());

	/* chiamata dello scheduler */
	schedule();

	return 0;
}
