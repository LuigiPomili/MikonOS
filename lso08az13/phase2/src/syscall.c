/*****************************************************************************
 * Gestione degli interrupt SW 
 *
 * - SYSCALL (Cause.ExcCode n° 8)
 * - BREAK (Cause.ExcCode n° 9)
 ****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/mikonos.h"
#include "../../h/our_const.h"

/* inclusione di interfacce esterne */
#include "../../e/tcb.e"
#include "../../e/tid.e"
#include "../../e/scheduler.e"
#include "../../e/interrupt.e"
#include "libumps.e"

/* dichiarazioni esterne al modulo */
extern unsigned int time_in_kernel;
extern tid_t current_thread;
extern unsigned char softb_count;
extern unsigned int last_time_slice;

/* Funzioni implementate in questo modulo */
void SYSMsgSend(tid_t sender, tid_t destination);
void SYSMsgRecv(tid_t receiver, tid_t source);
void SYSMsgReply(tid_t replier, tid_t source); 

/* Implementazione modulo */

/*
 * syscall #1, implementazione della MsgSend
 */
void SYSMsgSend(tid_t sender, tid_t destination) {
	tcb_t	*target_t, *sender_t;
	tcb_t	*outbox_c;
	tcb_t*	*ready_q;
	memaddr	*save;
	unsigned char	match;

	ready_q = getReadyQueue();
	sender_t = resolveTid(sender);
	target_t = resolveTid(destination);
	outbox_c = sender_t->outbox;
	if (!emptyThreadQ(outbox_c))
	/* ricerca destination nella propria outbox, destination puo' aver
	 * gia' eseguito una MsgRecv */
		do {
			if (outbox_c->tid == destination) match = TRUE;
			outbox_c = outbox_c->t_prev;
		} while ((outbox_c != sender_t->outbox) && !match);
	
	if (match)
		outThreadQ(&(sender_t->outbox), target_t);
	
	if (match || (target_t->status == W4_ANYTID)) { /* risveglia destination */
			sender_t->status = W4_REPLY;
			target_t->status = READY_THREAD;
			target_t->cpu_snapshot.reg_v0 = sender_t->tid;
			save = ((memaddr*) target_t->cpu_snapshot.reg_a2);
			*save = sender_t->last_payload;
			/* destination in testa alla ready queue in modo da essere
			 * il primo ad essere schedulato */
			insertFrontThreadQ(ready_q, target_t);
	}
	/* comunque sia, il sender viene accodato nella inbox di destination */
	insertBackThreadQ(&(target_t->inbox), sender_t);
}

/*
 * syscall #2, implementazione della MsgRecv
 */
void SYSMsgRecv(tid_t receiver, tid_t source) {
	tcb_t	*target_t, *receiver_t;
	tcb_t	*inbox_c;
	tcb_t*	*ready_q;
	memaddr	*save, just_pseudo;
	unsigned char	match, device;
	unsigned int	bit_i, interrupted_device, interrupted_terminal;

	ready_q = getReadyQueue();
	receiver_t = resolveTid(receiver);
	inbox_c = receiver_t->inbox;
	/* invocata in questo modo dalla SSI, ad esempio */
	if (source == ANYTID) {
		just_pseudo = getSigPseudo();
		interrupted_device = getSigDevice();
		interrupted_terminal = getSigTerminal();
		/* priorita' alle segnalazioni provenienti dal kernel, pseudo clock prima di tutto */
		if ((just_pseudo || interrupted_device || interrupted_terminal) 
			&& (receiver == SSI_TID)) {
			receiver_t->cpu_snapshot.reg_v0 = ANYTID;
			save = ((memaddr*) receiver_t->cpu_snapshot.reg_a2);
			receiver_t->status = RUNN_THREAD; /* cpu_remain != 0 per cui deve essere running */
			bit_i = 1;
			device = 0;
			if (just_pseudo) { /* gestione segnalazione pseudo clock */
				*save = just_pseudo;
				setSigPseudo(0);
			}
			else if (interrupted_device) { /* gestione segnalazione device */
				INTERRUPTED_I(interrupted_device, bit_i);
				setSigDevice(bit_i);
				BIT_NUM(bit_i, device);
				*save = BASE_OFFSET(device);
			}
			else { /* gestione segnalazione terminale */
				INTERRUPTED_I(interrupted_terminal, bit_i);
				setSigTerminal(bit_i);
				BIT_NUM(bit_i, device);
				if (device < 8) /* terminal recv */
					*save = BASE_OFFSET(device + 32);
				else
					/* base address del TRANSTATUS del terminal */
					*save = BASE_OFFSET(device + 24) + (2 * WORD_SIZE);
			}
			insertFrontThreadQ(ready_q, receiver_t);
		}
		else if (emptyThreadQ(inbox_c)) {
			receiver_t->status = W4_ANYTID;
			receiver_t->cpu_remain = 0; /* perde il suo time slice rimanente */
		}
		else {
			target_t = headThreadQ(inbox_c);
			target_t->status = W4_REPLY;
			receiver_t->cpu_snapshot.reg_v0 = target_t->tid;
			save = ((memaddr*) receiver_t->cpu_snapshot.reg_a2);
			*save = target_t->last_payload;

			receiver_t->status = RUNN_THREAD; /* cpu_remain != 0 per cui deve essere running */
			insertFrontThreadQ(ready_q, receiver_t);
		}
	}
	else { /* recv da un particolare thread */
		target_t = resolveTid(source);
		match = FALSE;
		if (!emptyThreadQ(inbox_c))
		/* ricerca source nella propria inbox, source puo' aver
		 * gia' eseguito una MsgSend */
			do {
				if (inbox_c->tid == source) match = TRUE;
				inbox_c = inbox_c->t_prev;
			} while ((inbox_c != receiver_t->inbox) && !match);
		
		if (match) { /* ricezione payload spedito da source */
			receiver_t->cpu_snapshot.reg_v0 = target_t->tid;
			save = ((memaddr*) receiver_t->cpu_snapshot.reg_a2);
			*save = target_t->last_payload;
			target_t->status = W4_REPLY;
			receiver_t->status = RUNN_THREAD; /* cpu_remain != 0 per cui deve essere running */
			insertFrontThreadQ(ready_q, receiver_t);
		}
		else { /* source non presente in inbox */
			receiver_t->cpu_remain = 0;	/* perde il suo time slice rimanente */
			/* accoda receiver nella outbox di source */
			insertBackThreadQ(&(target_t->outbox), receiver_t);
		}
	}
}

/*
 * syscall #3, implementazione della MsgReply
 */
void SYSMsgReply(tid_t replier, tid_t source) { 
	tcb_t	*target_t, *replier_t;
	tcb_t	*inbox_c;
	tcb_t*	*ready_q;
	unsigned char	match;

	ready_q = getReadyQueue();
	replier_t = resolveTid(replier);
	inbox_c = replier_t->inbox;

	if (source != ANYTID) { /* reply non al kernel */
		if (emptyThreadQ(inbox_c)) {
			PANIC(); /* errore */
		}
		else {
			target_t = resolveTid(source);
			/* ricerca nella inbox del source a cui fare reply, non solo
			 * deve essere in stato W4_REPLY o FROZEN in modo da impedire reply incorrette */
			do {
				if (inbox_c->tid == source && 
				((inbox_c->status == W4_REPLY) || (inbox_c->status == FROZEN))) match = TRUE;
				inbox_c = inbox_c->t_prev;
			} while ((inbox_c != replier_t->inbox) && !match);
				
			if (match) { /* risveglia il source */
				target_t->cpu_snapshot.reg_v0 = replier_t->last_payload;
				target_t->status = READY_THREAD;
				replier_t->cpu_snapshot.reg_v0 = 0; /* MsgReply torna 0 se va a buon fine */
				outThreadQ(&(replier_t->inbox), target_t); /* rimuove dalla inbox del replier */
				insertFrontThreadQ(ready_q, target_t);
				insertBackThreadQ(ready_q, replier_t);
			}
			else {
				PANIC(); /* errore: thread a cui far reply inesistente in inbox */
			}
		}
	}
}

/*
 * funzione attivata dall'esecuzione di una SYSCALL o BREAK.
 * settaggio avvennuto nella fase di boot del kernel
 */
void sys_bp_handler(void) {
	tcb_t	*caller_t, *tcbtokill;
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

	/* salvataggio del contesto, contenuto nella SYSBK_OLDAREA, nel cpu_snapshot del caller */
	initCPUSnap(&(caller_t->cpu_snapshot), SYSBK_OLDAREA, TRUE);

	if (caller_t->cpu_snapshot.status & TEST_USER_MODE) { /* thread chiamante in user mode */
		if (caller_t->sys_m == FALSE) {/* gestore non presente, kill caller */
			caller_t->to_kill = TRUE;
			insertFrontThreadQ(getReadyQueue(), caller_t);
			if (caller_t->cpu_snapshot.reg_a1 == SSI_TID)	softb_count--;
		}
		else {
			caller_t->status = FROZEN;
			caller_t->cpu_remain = 0;
			/* spedizione del valore del registro CAUSE */
			caller_t->last_payload = getCAUSE();
			SYSMsgSend(caller, caller_t->sys_m);
		}
	}
	else { /* thread chiamante in kernel mode */
		switch (CAUSE_EXCCODE_GET(getCAUSE())) {
		
			/* SYSCALL - exception code 8 */
			case EXC_SYSCALL : {
				/* discrimanazione tra le SYSCALL previste */
				switch (caller_t->cpu_snapshot.reg_a0) {
					case SYS_SEND : {	/* MsgSend */
						caller_t->status = EXEC_SEND;
						caller_t->cpu_remain = 0; /* perde il time slice rimanente */
						caller_t->last_payload = caller_t->cpu_snapshot.reg_a2;
						/* evita di fare send ad un thread inesistente */
						if (resolveTid(caller_t->cpu_snapshot.reg_a1) == NULL &&
							caller_t->cpu_snapshot.reg_a1 != ANYTID) {
							caller_t->to_kill = TRUE;
							insertFrontThreadQ(getReadyQueue(), caller_t);
						}
						else
							SYSMsgSend(caller, caller_t->cpu_snapshot.reg_a1);
						break;
					}
					case SYS_RECV : {	/* MsgRecv */
						caller_t->status = EXEC_RECV;
						caller_t->last_payload = caller_t->cpu_snapshot.reg_a2;
						/* evita di fare recv da un thread inesistente */
						if (resolveTid(caller_t->cpu_snapshot.reg_a1) == NULL &&
							caller_t->cpu_snapshot.reg_a1 != ANYTID) {
							caller_t->to_kill = TRUE;
							insertFrontThreadQ(getReadyQueue(), caller_t);
						}
						else
							SYSMsgRecv(caller, caller_t->cpu_snapshot.reg_a1);
						break;
					}
					case SYS_REPLY : {	/* MsgReply */
						caller_t->status = READY_THREAD;
						caller_t->cpu_remain = 0; /* perde il time slice rimanente */
						caller_t->last_payload = caller_t->cpu_snapshot.reg_a2;
						/* evita di fare reply ad un thread inesistente */
						if (resolveTid(caller_t->cpu_snapshot.reg_a1) == NULL &&
							caller_t->cpu_snapshot.reg_a1 != ANYTID) {
							caller_t->to_kill = TRUE;
							insertFrontThreadQ(getReadyQueue(), caller_t);
						}
						else if (caller_t->last_payload == TRAPTERMINATE) {
							/* messaggio di terminazione da parte del SYSTrap Thread Management */
							tcbtokill = resolveTid(caller_t->cpu_snapshot.reg_a1);
							tcbtokill->to_kill = TRUE;
							SYSMsgReply(caller, caller_t->cpu_snapshot.reg_a1);
						}
						else
							/* MsgReply normale */
							SYSMsgReply(caller, caller_t->cpu_snapshot.reg_a1);
						break;
					}
					default : { /* gestione comunicazione SYSTrap Management Thread, se presente */
						if (caller_t->sys_m == FALSE) { /* non presente, kill caller */
							caller_t->to_kill = TRUE;
							insertFrontThreadQ(getReadyQueue(), caller_t);
						}
						else {
							caller_t->status = FROZEN;
							caller_t->cpu_remain = 0;
							/* spedizione del valore del registro CAUSE */
							caller_t->last_payload = getCAUSE();
							SYSMsgSend(caller, caller_t->sys_m);
						}
					}
				}
				break;
			}
			/* BREAK - exception code 9 */
			case EXC_BREAKPOINT : {
				break;
			}
			default : PANIC();/* non previste */
		}
	}
	schedule();
}
