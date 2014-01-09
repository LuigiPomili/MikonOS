/*****************************************************************************
 * Thread Control Block allocazione e deallocazione.
 * Gestione delle Thread Queue.
 *
 * Non avendo allocazione/deallocazione dinamica della memoria, allochiamo
 * staticamente MAXTHREAD elementi e assumiamo questi come gli unici thread
 * a runtime.
 ****************************************************************************/

/* inclusione di header files */
#include "../../h/const.h"
#include "../../h/our_const.h"
#include "../../h/mikonos.h"

/* dichiarazioni locali al modulo */
HIDDEN tcb_t tcb_array[MAXTHREAD];	/* array dei thread possibili, allocato staticamente */
HIDDEN unsigned int bitmap;		/* determina quanti/quali TCBs sono liberi */

/* Funzioni implementate in questo modulo */
void initTcbs(void);
void freeTcb(tcb_t *t);
tcb_t* allocTcb(void);
tcb_t* mkEmptyThreadQ(void);
int emptyThreadQ(tcb_t *tp);
void insertFrontThreadQ(tcb_t **tp, tcb_t *t_ptr);
void insertBackThreadQ(tcb_t **tp, tcb_t *t_ptr);
tcb_t* removeThreadQ(tcb_t **tp);
tcb_t* outThreadQ(tcb_t **tp, tcb_t *t_ptr);
tcb_t* headThreadQ(tcb_t *tp);
void initCPUSnap(state_t *p, memaddr base, unsigned char update_pc);

/* Implementazione funzioni */

/* 
 * inizializza la bitmap 
 */
void initTcbs(void) {
	bitmap = 0;
}

/* 
 * azzera l'elemento nella bitmap corrispondente al TCB puntato da t
 */
void freeTcb(tcb_t *t) {

	unsigned int numerobitmap;

	/* creazione della maschera adatta all'azzeramento */
	numerobitmap = t->numero_bit;
	numerobitmap = ~numerobitmap;

	/* infine azzera il bit */
	bitmap = bitmap & numerobitmap;
}

/*
 * inizializza ai valori di default, oppure dall'indirizzo base passato, lo snapshot 
 * del processore all'interno del TCB
 */
void initCPUSnap(state_t *p, memaddr base, unsigned char update_pc) {
	
	unsigned int i;

	if (base == FALSE) {
		p->entry_hi = 0;
		p->cause = 0;
		p->status = STATUS_DEFAULT;
		p->pc_epc = 0;

		for (i = 0; i < CPU_N_REG_GP; i++)
			p->gpr[i] = 0;

		p->hi = 0;
		p->lo = 0;
	}
	else {
	/* all'indirizzo base viene aggiunto un'offset per puntare al giusto 
	 * indirizzo di memoria, si va di word (U32) in word */

		p->entry_hi = *((memaddr *)(base + 0));
		p->cause = *((memaddr *)(base + 4));
		p->status = *((memaddr *)(base + 8));
		p->pc_epc = *((memaddr *)(base + 12));

		if (update_pc) p->pc_epc += 4;	/* punta alla prossima istruzione, solo in
						caso di SYSCALL e BREAK */

		for (i = 0; i < CPU_N_REG_GP; i++)
			p->gpr[i] = *((memaddr *)(base + 16 + 4*i));

		p->hi = *((memaddr *)(base + EXCP_AREASIZE - 8));
		p->lo = *((memaddr *)(base + EXCP_AREASIZE - 4));
	}
}

/*
 * ritorna NULL se tutti i blocchi sono marcati usati nella bitmap; 
 * altrimenti sceglie un blocco libero, lo inizializza ritornando un puntatore ad esso
 */
tcb_t* allocTcb(void) {

	unsigned int maschera, bit;
	unsigned char numerobitmap, i;
	tcb_t *tcb_da_restituire;

	maschera = 0;
	bit = 0;
	i = 0;
	numerobitmap = 0;

	/* cerca un bit nullo nella bitmap ritornando in caso positivo
	 * la sua posizione */
	while ((i < MAXTHREAD) && (numerobitmap == 0)) {
			
		/* crea una maschera che selezioni solo il bit i-esimo */
		i++;
		if (maschera == 0)
			maschera = 1;
		else
			maschera = maschera << 1;

		bit = maschera & bitmap;

		if (bit == 0){
			numerobitmap = i;
		}
	}
		
	if (numerobitmap == 0)
		return NULL; 
		
	bitmap = bitmap | maschera; /* aggiornamento bitmap */
	/* determinazione puntatore al TCB appena allocato */
	tcb_da_restituire = &tcb_array[numerobitmap - 1];
	/* azzeramento dei campi del TCB */
	tcb_da_restituire->tid = 0;
	tcb_da_restituire->status = 0;
	tcb_da_restituire->t_next = NULL;
	tcb_da_restituire->t_prev = NULL;
	tcb_da_restituire->inbox = NULL;
	tcb_da_restituire->outbox = NULL;
	tcb_da_restituire->cpu_time = 0;
	tcb_da_restituire->cpu_remain = SCHED_TIME_SLICE;
	tcb_da_restituire->to_kill = FALSE;
	tcb_da_restituire->tlb_m = FALSE;
	tcb_da_restituire->prgtrap_m = FALSE;
	tcb_da_restituire->sys_m = FALSE;
	tcb_da_restituire->last_payload = 0;
	tcb_da_restituire->numero_bit = maschera; /* posizione nella bitmap */
	initCPUSnap(&(tcb_da_restituire->cpu_snapshot), FALSE, FALSE); /* inizializzazione stato processore */

	return tcb_da_restituire;
}

/*
 * inizializza una variabile ad essere il tail pointer ad una coda di thread
 */
tcb_t* mkEmptyThreadQ(void) {
	return NULL;
}

/*
 * ritorna TRUE se la coda di thread puntata da tp e' vuota, FALSE altrimenti
 */
int emptyThreadQ(tcb_t *tp) {
	return (tp == NULL ? TRUE : FALSE);
}

/*
 * inserisce il TCB puntato da t_ptr in testa alla coda di thread il cui
 * tail pointer e' puntato da tp
 */
void insertFrontThreadQ(tcb_t **tp, tcb_t *t_ptr) {

	if (!((*tp) == NULL)) {
		t_ptr->t_next = (*tp)->t_next;
		(*tp)->t_next = t_ptr;
		t_ptr->t_prev = (*tp);
		t_ptr->t_next->t_prev = t_ptr;
	}
	else {
		t_ptr->t_prev = t_ptr;
		t_ptr->t_next = t_ptr;
		(*tp) = t_ptr;
	}
}

/*
 * inserisce il TCB puntato da t_ptr in coda alla coda di thread il cui
 * tail pointer e' puntato da tp
 */
void insertBackThreadQ(tcb_t **tp, tcb_t *t_ptr) {
	
	if (!(*tp == NULL)) {
		t_ptr->t_next = (*tp)->t_next;
	    	(*tp)->t_next = t_ptr;
	    	t_ptr->t_prev = (*tp);
		t_ptr->t_next->t_prev = t_ptr;
	}
	else {
	    	t_ptr->t_prev = t_ptr;
	    	t_ptr->t_next = t_ptr;
	}
	(*tp) = t_ptr;
}

/*
 * rimuove l'elemento in testa alla coda di thread il cui tail pointer e'
 * puntato da tp. Ritorna NULL se la coda di thread e' vuota; altrimenti ritorna il puntatore
 * all'elemento rimosso
 */
tcb_t* removeThreadQ(tcb_t **tp) {

	tcb_t *tmp;

	if ((*tp) == NULL)
	    	return NULL;

	tmp = (*tp)->t_next;
	
	if ((*tp) == tmp) {
		(*tp) = NULL;
		return tmp;
	}

	tmp->t_next->t_prev = (*tp);
	(*tp)->t_next = tmp->t_next;
	return tmp;
}

/*
 * rimuove il TCB puntato da t_ptr dalla coda di thread il cui tail pointer e'
 * puntato da tp. Se l'elemento cercato non e' presente nella coda di thread, ritorna NULL;
 * altrimenti ritorna t_ptr
 */
tcb_t* outThreadQ(tcb_t **tp, tcb_t *t_ptr) {
	
	tcb_t *tmp;
	unsigned int match;

	if ((*tp) == NULL) /* coda vuota */
		return NULL;
	
	tmp = (*tp);
	match = 0;
	
	do{
		if (t_ptr == tmp)
			match = 1;
		tmp = tmp->t_next;
	}while(!match && (tmp != (*tp)));

	if (match == 0) /* l'elemento non è stato trovato */
		return NULL;
	else {		/* l'elemento è presente nella coda */
		if ((*tp) == (*tp)->t_next){ /* un solo elemento nella coda, tp va messo a null e viene restituito t_ptr */
			(*tp) = NULL;
			return t_ptr;
		}
		if((*tp) == t_ptr)	/* elemento da cancellare è il tp, aggiornare */
			(*tp) = (*tp)->t_prev;
    		(t_ptr->t_prev)->t_next = t_ptr->t_next; /* aggiornamento dei puntatori */
    		(t_ptr->t_next)->t_prev = t_ptr->t_prev;
		return t_ptr;
	}
}

/*
 * ritorna un puntatore al primo TCB della coda di thread il cui tail
 * pointer e' puntato da tp
 */
tcb_t* headThreadQ(tcb_t *tp) {
	
	if (tp == NULL) return NULL;

	if ((tp->t_next) == tp) return tp;

	return tp->t_next;
}
