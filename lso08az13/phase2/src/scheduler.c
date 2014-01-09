/*****************************************************************************
 * MIKONOS Scheduler
 *****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce esterne */
#include "../../e/tid.e"
#include "../../e/tcb.e"
#include "libumps.e"

/* dichiarazioni di variabili globali */
tid_t current_thread;		/* tid del thread in esecuzione */
unsigned int thread_count;	/* numero dei thread nel sistema */
unsigned int softb_count;	/* soft block count */
unsigned char first_up;		/* per discriminare la prima esecuzione dell' upThread */
unsigned char all_blocked;	/* per identificare il fatto che tutti i thread sono bloccati in attesa di interrupt */
unsigned int pseudo_count;	/* esprime il tempo che deve passare prima del prossimo tick dello pseudo clock */
unsigned int time_in_kernel;	/* esprime il tempo passato ad eseguire codice kernel */
unsigned int last_time_slice;	/* esprime l'intervallo di tempo dell'ultimo time slice */

/* dichiarazioni locali al modulo */
HIDDEN tcb_t *ready_queue;
HIDDEN tcb_t *killable_queue;

/* Funzioni implementate in questo modulo */
void initScheduler(void);
void addThread(tid_t tid);
tcb_t** getReadyQueue(void);
void schedule(void);

/* Funzioni locali al modulo */
HIDDEN void upThread(void);
HIDDEN void force_terminate(tid_t bill);

/* Implementazione modulo */

/*
 * crea la ready queue e inizializza tutte le strutture
 * necessarie allo scheduler
 */
void initScheduler(void) {
	ready_queue = mkEmptyThreadQ();
	killable_queue = mkEmptyThreadQ();
	current_thread = 0; /* SSI tid */
	thread_count = 0;
	softb_count = 0;
	pseudo_count = SCHED_PSEUDO_CLOCK;
	last_time_slice = 0;
	all_blocked = FALSE;
	first_up = TRUE; /* necessario per discriminare la prima esecuzione */
}

/*
 * inserisce nella ready queue il thread specificato da tid
 */
void addThread(tid_t tid) {
	tcb_t *added;

	added = resolveTid(tid);
	insertFrontThreadQ(&ready_queue, added);
	added->status = READY_THREAD;
	thread_count++;
}

/*
 * ritorna un puntatore alla ready queue, in modo da poterla modificare
 */
tcb_t** getReadyQueue(void) {
	return &ready_queue;
}

/*
 * carica il processore con lo snapshot salvato nel TCB in testa
 * alla ready queue
 */
HIDDEN void upThread(void) {
	tcb_t *current;
	signed int time_snap;
	
	/* inizio del conteggio del tempo passato in codice kernel, solo per la prima
	 * esecuzione, altrimenti inizializzazione prevista negli vari handler */
	if (first_up) time_in_kernel = TOD_SNAPSHOT;

	current = removeThreadQ(&ready_queue);
	current_thread = current->tid;
	current->status = RUNN_THREAD;
	current = resolveTid(current_thread);
	if (current->cpu_remain == 0)
		current->cpu_remain = SCHED_TIME_SLICE;

	/* aggiorna il valore dello pseudo count evitando un possibile underflow */
	if (pseudo_count < (last_time_slice + STCK(time_snap) - time_in_kernel)) /* STCK piu' preciso */
		pseudo_count = 0;
	else
		pseudo_count -= last_time_slice + time_snap - time_in_kernel;

	/* settaggio interval timer considerando il tempo rimasto allo scadere dello pseudo clock*/
	if (pseudo_count > SCHED_TIME_SLICE) {
		if (current->cpu_remain < SCHED_TIME_SLICE) {
			last_time_slice = TOD_SNAPSHOT; /* inizio intervallo di tempo del prossimo time slice */
			SET_IT(current->cpu_remain); /* settaggio dell'interval timer */
		}
		else {
			last_time_slice = TOD_SNAPSHOT; /* inizio intervallo di tempo del prossimo time slice */
			SET_IT(SCHED_TIME_SLICE); /* settaggio dell'interval timer */
		}
	}
	else {
		last_time_slice = TOD_SNAPSHOT; /* inizio intervallo di tempo del prossimo time slice */
		SET_IT(pseudo_count); /* settaggio dell'interval timer */
	}
	/* carica il processore con lo stato del thread */
	LDST(&(current->cpu_snapshot));
	/* esegue codice del thread appena caricato */
}

/*
 * funzione principale
 */
void schedule(void) {
	tcb_t	*SSI_t, *tmp_q;
	unsigned int execKill;

	/* esami tutta la ready queue, spostando tutti i thread contrassegnati to_kill
	 * nella killable_queue. Inizia la scansione da capo ad ogni thread spostato */
	if (!emptyThreadQ(ready_queue)){
		tmp_q = ready_queue;
		do{
			execKill = FALSE;
			if(tmp_q->to_kill == TRUE){
				tmp_q = outThreadQ(&ready_queue, tmp_q);
				insertBackThreadQ(&killable_queue, tmp_q);
				execKill = TRUE;
			}
			tmp_q = tmp_q->t_prev;
			if (execKill == TRUE)
				tmp_q = ready_queue;
			if (emptyThreadQ(ready_queue))
				execKill = FALSE;
		} while(((tmp_q != ready_queue) && (!emptyThreadQ(ready_queue))) || execKill == TRUE);
	}

	/* termina tutti i thread, svuotando contestualmente killable_queue */
	if (!emptyThreadQ(killable_queue)){
		tmp_q = killable_queue;
		do{
			outThreadQ(&killable_queue, tmp_q);
			force_terminate(tmp_q->tid);
			tmp_q = tmp_q->t_prev;
		} while(!emptyThreadQ(killable_queue));
	}

	if (emptyThreadQ(ready_queue)) {
		if (thread_count == 1) HALT(); /* dovrebbe bastare questo tipo di controllo */
		SSI_t = resolveTid(SSI_TID);
		if (thread_count > 0 && softb_count == 0) PANIC();
		/* anche la SSI non puo' eseguire perche' e' in recv */
		if (thread_count > 0 && softb_count > 0 && SSI_t->status == W4_ANYTID) {
			all_blocked = TRUE; /* tutti i thread sospesi */
			last_time_slice = TOD_SNAPSHOT;
			SET_IT(pseudo_count); /* settaggio interval timer con il valore di pseudo clock */
			ENABLE_INTERRUPT;
			for (;;); /* aspetta una interrupt */
		}
	}
	upThread(); /* dispatch di un nuovo thread */
}


/*
 * termina/elimina il TCB passato ponendo in stato READY_THREAD tutti i
 * thread che fossero in inbox e in oubox, questi ultimi vengono poi 
 * re-inseriti nella ready_queue
 */
HIDDEN void force_terminate(tid_t bill) {
	tcb_t	*bill_t, *tmp_bill;

	bill_t = resolveTid(bill);

	/* controllo presenza TCB in attesa in inbox */
	while (!emptyThreadQ(bill_t->inbox)) {
		tmp_bill = removeThreadQ(&(bill_t->inbox));
		tmp_bill->status = READY_THREAD;
		insertBackThreadQ(&ready_queue, tmp_bill);
	}

	/* controllo presenza TCB in attesa in outbox */
	while (!emptyThreadQ(bill_t->outbox)) {
		tmp_bill = removeThreadQ(&(bill_t->outbox));
		tmp_bill->status = READY_THREAD;
		insertBackThreadQ(&ready_queue, tmp_bill);
	}

	killTcb(bill);
	thread_count--;
}
