#ifndef OUR_MIKONOS
#define OUR_MIKONOS
#include "types.h"

/* thread control block */
typedef struct tcb_t {

	tid_t	tid; 	/* ID del thread */

	status_t	status;		/* status del thread */

	struct tcb_t 	*t_next,	/* puntatore al prossimo tcb della coda */	
			*t_prev,	/* puntatore al precedente tcb della coda */
			*inbox,		/* thread che aspettano per mandare un messaggio a questo thread */
			*outbox;	/* thread che aspettano di ricevere un messaggio da questo thread */
	unsigned int	numero_bit;	/* bit corrispondente nella bitmap */
	unsigned int	last_payload;	/* ultimo messaggio spedito da questo thread */
	unsigned char	to_kill;	/* flag per il kill del thread */

	state_t		cpu_snapshot;	/* immagine del processore */
	unsigned int	cpu_time;	/* tempo di processore totale */
	unsigned int	cpu_remain;	/* tempo di processore che rimane da utilizzare */

	tid_t	prgtrap_m;		/* program trap thread manager */
	tid_t	tlb_m;			/* tlb exceptions thread manager */
	tid_t	sys_m;			/* syscall thread manager */

} tcb_t;

/* struttura che determina una richiesta di servizio alla SSI */
typedef struct {
	unsigned int SSIservice;
	unsigned int payload;
} request_t;

/* struttura utile per il match tra le SSIRequest e gli interrupt da device */
typedef struct {
	unsigned char	happened_req;
	unsigned char	happened_int;
	tid_t	requestor[8];
} ssi_dev;

#endif
