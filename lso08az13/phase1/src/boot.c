/*****************************************************************************
 * Main MIKONOS file 
 *
 * Boot del SO ed exception handler.
 *****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/types.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce esterne */
#include "../../e/tid.e"
#include "libumps.e"
#include "../test/p1b2test.e"

/* Implementazione modulo */

/*
 * inizializza la New Area passata e setta
 * l'handler per l'eccezione corrispondente
 */
void impNewArea(memaddr baseArea, memaddr baseHandler) {
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

	/* chiamata dello scheduler */
	schedule();

	return 0;
}
