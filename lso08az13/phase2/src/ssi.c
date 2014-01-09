/*****************************************************************************
 * System Service Interface
 *
 * - Kernel Thread (server) a cui vengono spedite tutte le segnalazioni
 *   rilevanti del sistema e tutte le richieste provenienti da altri thread
 * - Gestione (subscribe) dei Trap Management Threads
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

/* dichiarazioni esterne al modulo */
extern tid_t current_thread;
extern unsigned int thread_count;
extern unsigned int softb_count;

/* Funzioni implementate in questo modulo */
void SSIRequest(unsigned int service, unsigned int payload, unsigned int* reply);

/* variabili locali al modulo */
HIDDEN tcb_t	*wfc;			/* coda di TCB che aspettano il tick dello pseudo clock */
HIDDEN ssi_dev	wfio[6];		/* gestione richiesta/completamento operazione di I/O */

/* Implementazione modulo */

/* 
 * SSI - funzione principale
 */
void SSI_start(void) {
	unsigned int	new_service, dev_base, store_reg_v0;
	unsigned char	dev_base_offset;
	request_t	*last_request;
	tid_t	last_tid;
	tcb_t	*SSI_t, *last_tcb, *requestor, **ready_q;
	state_t	*getstate;

	ready_q = getReadyQueue();

	for (;;) {
		DISABLE_INTERRUPT;
		MsgRecv(ANYTID, &new_service);
		SSI_t = resolveTid(SSI_TID);
		store_reg_v0 = SSI_t->cpu_snapshot.reg_v0;
		ENABLE_INTERRUPT;
		/* verificatosi una interrupt HW o pseudo clock da gestire */
		if (store_reg_v0 == ANYTID) { /* segnalazione dal kernel */
			if (new_service == BUS_INTERVALTIMER) { /* risveglia tutti quelli in WAITFORCLOCK */
				while (!emptyThreadQ(wfc)) {
					last_tcb = removeThreadQ(&wfc);
					/* il TCB deve essere in inbox per una giusta reply */
					insertFrontThreadQ(&(SSI_t->inbox), last_tcb);
					MsgReply(last_tcb->tid, BUS_INTERVALTIMER);
				}
			}
			else {
				dev_base = OFFSET_BASE(new_service);
				dev_base_offset = 1;
				dev_base = dev_base % DEV_PER_INT; /* indice del device 0-7 */
				NUM_BIT(dev_base_offset, dev_base);
				dev_base = OFFSET_BASE(new_service);
				if (dev_base < 32 || ((new_service % 16) == 0)) {	/* non terminale oppure terminal recv */
					wfio[dev_base / N_INT_LINE].happened_int |= dev_base_offset;
					if ((wfio[dev_base / N_INT_LINE].happened_req & dev_base_offset) == TRUE) { /* richiesta per tale device attiva */
						wfio[dev_base / N_INT_LINE].happened_int &= ~dev_base_offset;
						wfio[dev_base / N_INT_LINE].happened_req &= ~dev_base_offset;
						last_tid = wfio[dev_base / N_INT_LINE].requestor[dev_base % DEV_PER_INT];
						wfio[dev_base / N_INT_LINE].requestor[dev_base % DEV_PER_INT] = FALSE;
						last_tcb = resolveTid(last_tid);
						insertFrontThreadQ(&(SSI_t->inbox), last_tcb);
						MsgReply(last_tid, *((memaddr*) new_service));
					}
				}
				else { /* terminal transm di indice 5 in wfio */
					wfio[5].happened_int |= dev_base_offset;
					if ((wfio[5].happened_req & dev_base_offset) == TRUE) { /* richiesta per tale device attiva */
						wfio[5].happened_int &= ~dev_base_offset;
						wfio[5].happened_req &= ~dev_base_offset;
						last_tid = wfio[5].requestor[dev_base % DEV_PER_INT];
						wfio[5].requestor[dev_base % DEV_PER_INT] = FALSE;
						last_tcb = resolveTid(last_tid);
						insertFrontThreadQ(&(SSI_t->inbox), last_tcb);
						MsgReply(last_tid, *((memaddr*) new_service));
					}
				}
			}
		}
		else {
			last_request = (request_t*) new_service;

			switch (last_request->SSIservice) {
				case CREATE: {
					last_tid = newTcb();
					if (last_tid == NEW_TCB_ERROR) /* raggiunto #TCB massimo */
						MsgReply(store_reg_v0, CREATENOGOOD);
					else {
						last_tcb = resolveTid(last_tid);
						/* inizializza lo snapshot di cpu del nuovo TCB */
						initCPUSnap(&(last_tcb->cpu_snapshot), last_request->payload, FALSE);
						insertBackThreadQ(ready_q, last_tcb);
						thread_count++; /* aumento contatore thread */
						MsgReply(store_reg_v0, last_tid);
					}
					break;
				}
				case TERMINATE: {
					if (last_request->payload == ANYTID) { /* auto-kill */
						last_tcb = resolveTid(store_reg_v0);
						last_tcb->to_kill = TRUE;
						softb_count--;
						MsgReply(store_reg_v0, FALSE);
						/* richiedente non continuera' con SSIRequest quindi
						 * diminuzione contatore soft block */
					}
					else {
						last_tcb = resolveTid(last_request->payload);
						if (last_tcb == NULL) /* evita il kill di un TCB inesistente */
							MsgReply(store_reg_v0, TRUE);
						else {
							last_tcb->to_kill = TRUE;
							MsgReply(store_reg_v0, FALSE);
						}
					}
					break;
				}
				case SPECPRGMGR: {
					last_tcb = resolveTid(last_request->payload);
					requestor = resolveTid(store_reg_v0);
					if (last_tcb == NULL) { /* thread management inesistente, kill requestor */
						requestor->to_kill = TRUE;
						softb_count--; 
						MsgReply(store_reg_v0, FALSE);
						/* requestor non continuera' con SSIRequest quindi
						 * diminuzione contatore soft block */
					}
					else {
						if (requestor->prgtrap_m != FALSE) { /* thread management gia' specificato */
							(resolveTid(requestor->prgtrap_m))->to_kill = TRUE;
							requestor->to_kill = TRUE;
							softb_count--;
							MsgReply(store_reg_v0, FALSE);
						}
						else {
							requestor->prgtrap_m = last_request->payload;
							MsgReply(store_reg_v0, FALSE);
						}
					}
					break;
				}
				case SPECTLBMGR: {
					last_tcb = resolveTid(last_request->payload);
					requestor = resolveTid(store_reg_v0);
					if (last_tcb == NULL) { /* thread management inesistente, kill requestor */
						requestor->to_kill = TRUE;
						softb_count--;
						MsgReply(store_reg_v0, FALSE);
						/* requestor non continuera' con SSIRequest quindi
						 * diminuzione contatore soft block */
					}
					else {
						if (requestor->tlb_m != FALSE) { /* thread management gia' specificato */
							(resolveTid(requestor->tlb_m))->to_kill = TRUE;
							requestor->to_kill = TRUE;
							softb_count--;
							MsgReply(store_reg_v0, FALSE);
						}
						else {
							requestor->tlb_m = last_request->payload;
							MsgReply(store_reg_v0, FALSE);
						}
					}
					break;
				}
				case SPECSYSMGR: {
					last_tcb = resolveTid(last_request->payload);
					requestor = resolveTid(store_reg_v0);
					if (last_tcb == NULL) { /* thread management inesistente, kill requestor */
						requestor->to_kill = TRUE;
						softb_count--;
						MsgReply(store_reg_v0, FALSE);
						/* requestor non continuera' con SSIRequest quindi
						 * diminuzione contatore soft block */
					}
					else {
						if (requestor->sys_m != FALSE) { /* thread management gia' specificato */
							(resolveTid(requestor->sys_m))->to_kill = TRUE;
							requestor->to_kill = TRUE;
							softb_count--;
							MsgReply(store_reg_v0, FALSE);
						}
						else {
							requestor->sys_m = last_request->payload;
							MsgReply(store_reg_v0, FALSE);
						}
					}
					break;
				}
				case GETCPUTIME: {
					last_tcb = resolveTid(store_reg_v0);
					MsgReply(store_reg_v0, last_tcb->cpu_time);
					break;
				}
				case WAITFORCLOCK: {
					last_tcb = removeThreadQ(&(SSI_t->inbox));
					insertFrontThreadQ(&wfc, last_tcb);
					break;
				}
				case WAITFORIO: {
					if (last_request->payload >= DEV_REGS_START && last_request->payload <= (DEV_REGS_END + (2 * WORD_SIZE))) {
					/* device esistente */
						dev_base = OFFSET_BASE(last_request->payload);
						dev_base_offset = 1;
						dev_base = dev_base % DEV_PER_INT; /* indice del device 0-7 */
						NUM_BIT(dev_base_offset, dev_base);
						dev_base = OFFSET_BASE(last_request->payload);
						last_tcb = resolveTid(store_reg_v0);

						if (dev_base < 32 || ((last_request->payload % 16) == 0)) { /* non terminale oppure terminal recv */
							wfio[dev_base / N_INT_LINE].happened_req |= dev_base_offset;
							wfio[dev_base / N_INT_LINE].requestor[dev_base % DEV_PER_INT] = store_reg_v0;
							last_tcb = outThreadQ(&(SSI_t->inbox), last_tcb);

							if ((wfio[dev_base / N_INT_LINE].happened_int & dev_base_offset) == TRUE) { /* interrupt per tale device avvenuta */
								wfio[dev_base / N_INT_LINE].happened_int &= ~dev_base_offset;
								wfio[dev_base / N_INT_LINE].happened_req &= ~dev_base_offset;
								last_tid = wfio[dev_base / N_INT_LINE].requestor[dev_base % DEV_PER_INT];
								wfio[dev_base / N_INT_LINE].requestor[dev_base % DEV_PER_INT] = FALSE;
								insertFrontThreadQ(&(SSI_t->inbox), last_tcb);
								MsgReply(last_tid, *((memaddr*) new_service));
							}
						}
						else { /* terminal transm */
							wfio[5].happened_req |= dev_base_offset;
							wfio[5].requestor[dev_base % DEV_PER_INT] = store_reg_v0;
							last_tcb = outThreadQ(&(SSI_t->inbox), last_tcb);

							if ((wfio[5].happened_int & dev_base_offset) == TRUE) { /* interrupt per tale device avvenuta */
								wfio[5].happened_int &= ~dev_base_offset;
								wfio[5].happened_req &= ~dev_base_offset;
								last_tid = wfio[5].requestor[dev_base % 8];
								wfio[5].requestor[dev_base % DEV_PER_INT] = FALSE;
								insertFrontThreadQ(&(SSI_t->inbox), last_tcb);
								MsgReply(last_tid, *((memaddr*) new_service));
							}
						}

					}
					else { /* device inesistente */
						requestor = resolveTid(store_reg_v0);
						requestor->to_kill = TRUE;
						softb_count--;
						MsgReply(store_reg_v0, FALSE);
					}
					break;
				}
				case GETTSTATE: {
					requestor = resolveTid(store_reg_v0);
					last_tcb = resolveTid(last_request->payload);
					if (last_tcb == NULL && last_request->payload != ANYTID) { /* thread inesistente, kill requestor */
						requestor->to_kill = TRUE;
						softb_count--;
						MsgReply(store_reg_v0, FALSE);
						/* requestor non continuera' con SSIRequest quindi
						 * diminuzione contatore soft block */
					}
					else if (last_request->payload == ANYTID) { /* il proprio stato del processore */
						getstate = &(requestor->cpu_snapshot);
						MsgReply(store_reg_v0, (memaddr*) getstate);
					} else {
						getstate = &(last_tcb->cpu_snapshot);
						MsgReply(store_reg_v0, (memaddr*) getstate);
					}
					break;
				}
				default: { /* servizio non previsto */
					requestor = resolveTid(store_reg_v0);
					requestor->to_kill = TRUE;
					softb_count--;
					MsgReply(store_reg_v0, FALSE);
					/* requestor non continuera' con SSIRequest quindi
					 * diminuzione contatore soft block */
				}
			}
		}
	}
}

/* 
 * richiesta di un particolare servizio da inoltrare alla SSI
 */
void SSIRequest(unsigned int service, unsigned int payload, unsigned int* reply) {
	request_t	new_request;
	tcb_t*		current;

	new_request.SSIservice = service;
	new_request.payload = payload;
	softb_count++; /* aggiunge un soft block */
	MsgSend(SSI_TID, &new_request); /* inoltra la richiesta */
	softb_count--; /* rimuove un soft block */
	current = resolveTid(current_thread);
	*reply = current->cpu_snapshot.reg_v0; /* ritorna il risultato della richiesta */
}
