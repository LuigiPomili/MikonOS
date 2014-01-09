/*****************************************************************************
 * Gestione degli interrupt HW
 *
 * - Interval Timer
 * - Pseudo Clock
 * - Device
 ****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce esterne */
#include "../../e/tcb.e"
#include "../../e/tid.e"
#include "../../e/scheduler.e"
#include "libumps.e"

#define INT_SW_0	0	/* linee di interrupt sw */
#define INT_SW_1	1

/* dichiarazioni esterne al modulo */
extern unsigned int time_in_kernel;
extern tid_t current_thread;
extern unsigned char all_blocked;
extern unsigned int pseudo_count;
extern unsigned int last_time_slice;

/* dichiarazioni locali al modulo, segnalazioni del kernel alla SSI */
HIDDEN unsigned int interrupted_device = FALSE;		/* avvenuto interrupt */
HIDDEN unsigned int interrupted_terminal = FALSE;	/* avvenuto interrupt da terminale */
HIDDEN memaddr just_pseudo = FALSE;			/* avvenuto tick */

/* Funzioni implementate in questo modulo */
memaddr getSigPseudo();
void setSigPseudo(memaddr value);
unsigned int getSigDevice();
void setSigDevice(unsigned int value);
unsigned int getSigTerminal();
void setSigTerminal(unsigned int value);

/* Funzioni locali al modulo */
HIDDEN void KtoSSI(memaddr payload, unsigned char terminal);
HIDDEN void elapsedTimeSlice(void);
HIDDEN void elapsedPseudoClock(void);

/* Implementazione modulo */

/*
 * gestione dell'interrupt del device - "interval timer"
 */
HIDDEN void elapsedTimeSlice(void) {
	tcb_t	*thread_c;
	tcb_t*	*readyq_c;

	thread_c = resolveTid(current_thread);
	readyq_c = getReadyQueue();
	/* il thread corrente viene riposto in fondo alla ready queue, di nuovo
	 * in stato READY*/
	removeThreadQ(readyq_c);
	insertBackThreadQ(readyq_c, thread_c);
	thread_c->status = READY_THREAD;
}

/*
 * gestione dell'interrupt del device virtuale - "pseudo clock"
 */
HIDDEN void elapsedPseudoClock(void) {
	last_time_slice = 0;
	pseudo_count = SCHED_PSEUDO_CLOCK; /* re-inizializzazione dello pseudo clock */
}

/*
 * funzione attivata dall'HW quando avviene un'interrupt da device.
 * settaggio avvennuto nella fase di boot del kernel
 */
void ints_handler(void) {
	tcb_t *current;
	unsigned int int_line, int_pending, ndev_pending;
	signed int time_snap;
	memaddr dev_base_addr;

	int_line = INT_TIMER; /* default */

	/* inizio conteggio del tempo passato in codice kernel */
	time_in_kernel = TOD_SNAPSHOT;
	/* calcolo della lunghezza dell'ultimo intervallo di time slice */
	last_time_slice = TOD_SNAPSHOT - last_time_slice;

	if (all_blocked == FALSE) {
		/* aggiornamento "accounted" CPU del thread */
		current = resolveTid(current_thread);
		current->cpu_time += last_time_slice;
		
		insertFrontThreadQ(getReadyQueue(), current);

		/* salvataggio del contesto, contenuto nell'INT_OLDAREA, nel cpu_snapshot del thread */
		initCPUSnap(&(current->cpu_snapshot), INT_OLDAREA, FALSE);
	
		/* calcolo del tempo di CPU rimanente per il thread corrente */
		if (current->cpu_remain < last_time_slice) {
			current->cpu_remain = 0;
			elapsedTimeSlice(); /* subira' il dispatch perche' finito il
					    proprio rimanente tempo di CPU */
		}
		else 
			current->cpu_remain -= last_time_slice;
	}
	else {
		all_blocked = FALSE;
		/* aggiorna il valore dello pseudo count evitando un possibile underflow */
		if (pseudo_count < (last_time_slice + STCK(time_snap) - time_in_kernel))
			pseudo_count = 0; /* tutti bloccati e tick dello pseudo clock */
		else
			/* tutti bloccati e interrupt dovuta ad un device diverso dal timer */
			pseudo_count -= last_time_slice + time_snap - time_in_kernel;
	}

	/* controllo interrupt pendenti */
	while ((int_pending = (CAUSE_IP_GET(getCAUSE(), int_line))) == 0)
		int_line++; /* linea interrupt pendente piu' importante */

	int_pending = 1; /* default */
	ndev_pending = 0;
	
	switch (int_line) {
		/* linea 0,1 - INTERRUPT SW */
		case INT_SW_0: 
		case INT_SW_1:
			PANIC(); /* Mikonos non gestisce interrupt software */
			break;
		/* linea 2 - INTERVAL TIMER */
		case INT_TIMER : {
			if (pseudo_count <= SCHED_TIME_SLICE) {
				/* pseudo clock interrupt */
				elapsedPseudoClock();
				KtoSSI(BUS_INTERVALTIMER, FALSE);
			}
			break;
		}
		/* linea 7 - TERMINAL */
		case INT_TERMINAL : {
			F_DEV_PEND(INT_TERMINAL, int_pending, ndev_pending);
			ndev_pending--; /* F_DEV_PEND fa un ciclo in piu' */
			dev_base_addr = DEV_BASE_ADDR_I(INT_TERMINAL, ndev_pending);
			/* char ricevuto */
			if ((*((memaddr*) dev_base_addr) & DEV_STATUSMASK) == DEV_TRCV_S_CHARRECV) {
				*((memaddr*) (dev_base_addr + WORD_SIZE)) = DEV_C_ACK;
				KtoSSI(dev_base_addr, 1); /* segnalazione */
			}
			/* char trasmesso */
			if ((*((memaddr*) (dev_base_addr + (2 * WORD_SIZE))) & DEV_STATUSMASK) == DEV_TTRS_S_CHARTRSM) {
				*((memaddr*) (dev_base_addr + (3 * WORD_SIZE))) = DEV_C_ACK;
				KtoSSI(dev_base_addr, 2); /* segnalazione */
			}
			break;
		}
		/* tutte le altre linee */
		default : {
			F_DEV_PEND(int_line, int_pending, ndev_pending);
			ndev_pending--; /* F_DEV_PEND fa un ciclo in piu' */
			dev_base_addr = DEV_BASE_ADDR_I(int_line, int_pending);
			*((memaddr*) (dev_base_addr + WORD_SIZE)) = DEV_C_ACK;
			KtoSSI(dev_base_addr, FALSE); /* segnalazione */
			break;
		}
	}
	schedule();
}

/*
 * "segnalazione" del kernel alla SSI, utilizzato solo dal kernel
 * per segnalare avvenuti interrupt HW
 */
HIDDEN void KtoSSI(memaddr payload, unsigned char terminal) {
	tcb_t*	SSI_t = resolveTid(SSI_TID);
	tcb_t*	*ready_q;
	memaddr	*save;
	unsigned char device;
	unsigned int bit_i = 1;
	
	/* segnalazione di interrupt proveniente da device, non da terminale */
	if (payload != BUS_INTERVALTIMER && terminal == FALSE) {
		device = OFFSET_BASE(payload);
		NUM_BIT(bit_i, device);
		interrupted_device |= bit_i;
	}
	else if (payload == BUS_INTERVALTIMER) /* tick dello pseudo clock */
		just_pseudo = payload;
	else { /* interrupt da terminale */
		if (terminal == 1) /* terminal recv */
			device = (OFFSET_BASE(payload) - 32);	/* primi 8 bit */
		else
			device = (OFFSET_BASE(payload) - 24);	/* secondi 8 bit */
		NUM_BIT(bit_i, device);
		interrupted_terminal |= bit_i;
	}
	
	if (SSI_t->status == W4_ANYTID) { /* SSI sospesa in attesa di ANYTID */
		SSI_t->status = READY_THREAD;
		SSI_t->cpu_snapshot.reg_v0 = ANYTID;
		save = ((memaddr*) SSI_t->cpu_snapshot.reg_a2);
		*save = payload;
		if (terminal == 2)
			*save = payload + (2 * WORD_SIZE);
		ready_q = getReadyQueue();
		if ((*ready_q != SSI_t) && (headThreadQ(*ready_q) != SSI_t))
			insertFrontThreadQ(ready_q, SSI_t);
	}
}

/*
 * ritorna BUS_INTERVALTIMER se avvenuto tick dello
 * pseudo clock
 */
memaddr getSigPseudo() {
	return just_pseudo;
}

/*
 * setta il valore della segnalazione
 */
void setSigPseudo(memaddr value) {
	just_pseudo = value;
}

/*
 * ritorna la bitmap dei device che hanno spedito
 * una interrupt
 */
unsigned int getSigDevice() {
	return interrupted_device;
}

/*
 * setta la bitmap che gestisce gli interrupt dei device
 */
void setSigDevice(unsigned int value) {
	interrupted_device &= ~value;
}

/*
 * ritorna la bitmap dei terminal che hanno spedita
 * una interrupt
 */
unsigned int getSigTerminal() {
	return interrupted_terminal;
}

/*
 * setta la bitmap che gestisce gli interrupt dei terminal
 */
void setSigTerminal(unsigned int value) {
	interrupted_terminal &= ~value;
}
