/****************************************************************************
 * Manutenzione dei Thread ID
 *
 * Ogni Thread Control Block possiedo un proprio Thread ID che lo identifica.
 * **************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/our_const.h"
#include "../../h/mikonos.h"

/* inclusione di interfacce estrerne */
#include "../../e/tcb.e"

/* dichiarazioni locali al modulo */
HIDDEN tcb_t *assegna_tid[MAXTID];		/* puntatori ai TCBs */
HIDDEN tid_t tid_assegnato;			/* ultimo tid utilizzato */

/* Funzioni implementate in questo modulo */
void initTidTable(void);
tid_t newTcb(void);
tcb_t* resolveTid(tid_t tid);
void killTcb(tid_t tid);

/* Implementazione funzioni */

/* 
 * inizializza la bitmap (chiamando initTcbs) e tutti i puntatori
 * in assegna_tid
 */
void initTidTable(void) {

	unsigned char i;
	
	initTcbs();

	for (i = 0; i < MAXTID; i++){
		assegna_tid[i] = NULL;
	}
	tid_assegnato = 0;
}

/* 
 * alloca un nuovo TCB chiamando allocTcb. Se quest'ultima fallisce, ritorna 255. 
 * Altrimenti ne restituisce il TID
 */
tid_t newTcb(void) {

	tcb_t *tcb_allocato;

	/* determina il tid da utilizzare in caso di allocazione del TCB */
	while (assegna_tid[tid_assegnato] != NULL) {
		tid_assegnato = (tid_assegnato + 1) % MAXTID;
	}
	tcb_allocato = allocTcb();
	
	if (tcb_allocato == NULL) /* allocazione fallita */
		return NEW_TCB_ERROR;

	assegna_tid[tid_assegnato] = tcb_allocato;
	assegna_tid[tid_assegnato]->tid = tid_assegnato; /* aggiornamento del TID nel TCB allocato */
	
	return tid_assegnato;
}

/* 
 * restituisce il puntatore al TCB identificato da tid
 */
tcb_t* resolveTid(tid_t tid) {
	return assegna_tid[tid];
}

/* 
 * dealloca (chiamando freeTcb) il TCB identificato da tid; rende di nuovo disponibile 
 * il TID per uso futuro
 */
void killTcb(tid_t tid) {

	tcb_t *tcb_da_terminare;

	tcb_da_terminare = resolveTid(tid);
	freeTcb (tcb_da_terminare);
	
	assegna_tid[tid] = NULL;
	/* evita di ri-assegnare immediatamente lo stesso tid */
	if (tid == tid_assegnato)
		tid_assegnato = (tid_assegnato + 1) % MAXTID;
}
