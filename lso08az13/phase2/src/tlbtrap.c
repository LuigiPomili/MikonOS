/****************************************************************************
 * TLB exception management
 ***************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce estrerne */
#include "../../e/tcb.e"
#include "../../e/tid.e"
#include "../../e/scheduler.e"
#include "../../e/syscall.e"
#include "libumps.e"

/* dichiarazioni esterne al modulo */
extern unsigned int time_in_kernel;
extern tid_t current_thread;
extern unsigned int last_time_slice;

/* Implementazione modulo */

/*
 * funzione attivata dall'HW quando avviene una TLB miss.
 * settaggio avvennuto nella fase di boot del kernel 
 */
void tlb_handler(void) {
	tcb_t	*caller_t;
	tid_t	caller;

	/* inizio conteggio del tempo passato in codice kernel */
	time_in_kernel = TOD_SNAPSHOT;
	/* calcolo della lunghezza dell'ultimo intervallo di time slice */
	last_time_slice = TOD_SNAPSHOT - last_time_slice;

	/* aggiornamento "accounted" CPU del thread */
	caller_t = resolveTid(caller = current_thread);
	caller_t->cpu_time += last_time_slice;

	/* calcolo del tempo di CPU rimanente per il thread corrente */
	if (caller_t->cpu_remain < last_time_slice)
		caller_t->cpu_remain = 0;
	else
		caller_t->cpu_remain -= last_time_slice;

	/* salvataggio del contesto, contenuto nella TLB_OLDAREA, nel cpu_snapshot del caller */
	initCPUSnap(&(caller_t->cpu_snapshot), TLB_OLDAREA, FALSE);

	if (caller_t->tlb_m == FALSE) { /* gestore non presente, kill caller */
		caller_t->to_kill = TRUE;
		insertFrontThreadQ(getReadyQueue(), caller_t);
	}
	else {
		caller_t->status = FROZEN;
		caller_t->cpu_remain = 0;
		/* spedizione del valore del registro CAUSE */
		caller_t->last_payload = getCAUSE();
		SYSMsgSend(caller, caller_t->tlb_m);
	}
	schedule();
}
