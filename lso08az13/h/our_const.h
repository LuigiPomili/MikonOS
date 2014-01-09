#ifndef OUR_CONSTS
#define OUR_CONSTS

/*** thread and tid ***/
#define MAXTID		255
#define MAXTHREAD	MAXPROC
#define READY_THREAD	1
#define RUNN_THREAD	2
#define EXEC_SEND	8
#define EXEC_RECV	9
#define W4_ANYTID	10
#define W4_REPLY	11
#define FROZEN		12
#define SSI_TID		0
#define TEST_TID	1

/*** utilita' ***/
#define NEW_TCB_ERROR	255
#define STATUS_CP0	0x10000000
#define STATUS_DEFAULT	0x10400000
#define TEST_USER_MODE	0x0000000A
#define CPU_N_REG_GP	29
#define EXCP_AREASIZE	140	/* dimensione di una old_area */
#define DEV_REGS_END	0x100002C0
#define N_INT_LINE	DEV_PER_INT

/*** valori per syscall ***/
#define SYS_SEND	1
#define SYS_RECV	2
#define SYS_REPLY	3

/*** valori specifici di servizi SSI ***/
#define CREATE              1
#define TERMINATE           2
#define SPECPRGMGR          3
#define SPECTLBMGR          4
#define SPECSYSMGR          5
#define GETCPUTIME          6
#define WAITFORCLOCK        7
#define WAITFORIO           8
#define GETTSTATE           9

/*** valori specifici di messaggi ***/
#define ANYTID		255
#define TRAPCONTINUE	301
#define TRAPTERMINATE	300
#define CREATENOGOOD	-1 /* creazione thread fallita */
#define MSGNOGOOD	-1 /* send fallita */

/*** macro ***/

#define ENABLE_INTERRUPT    ( setSTATUS(STATUS_CP0 | STATUS_INT_UNMASKED | STATUS_IEc) )
#define DISABLE_INTERRUPT    ( setSTATUS(STATUS_CP0) )

/* snapshot del time of day */
#define STCK(T)	((T) = ((* ((cpu_t*) BUS_TODLOW)) / (* ((cpu_t*) BUS_TIMESCALE))))
#define TOD_SNAPSHOT	((unsigned int) (*((memaddr *) BUS_TODLOW) / (*((memaddr *) BUS_TIMESCALE))))

/* valore del registro di indirizzo (base + offset) nell'old_area. offset in parole (U32) */
#define OLD_REG_VALUE(base, offset)	(unsigned int) *((memaddr *) ((base) + (offset)))

/* indirizzo del registro base dato interrupt line e device */
#define DEV_BASE_ADDR_I(int_no, dev_no)	DEV_REGS_START + (((int_no) - 3) * 0x80) + ((dev_no) * 0x10)

/* numero del piu' importante device, allacciato alla interrupt line, pendente */
#define F_DEV_PEND(int_no, n, i) \
while ((i++ < 8) && (!(*((memaddr*)(PENDING_BITMAP_START + (WORD_SIZE * ((int_line) - 3)))) & (n)))) ((n) = ((n) << 1))

/* primo bit a 1 nella bitmap int_i passata */
#define INTERRUPTED_I(int_i, n)	while(!((int_i) & (n))) ((n) = (n) << 1)

/* indice device dato il base address */
#define OFFSET_BASE(ind)	((ind) - DEV_REGS_START) / DEV_REG_SIZE

/* base address dato il indice di device */
#define BASE_OFFSET(num)	((num) * DEV_REG_SIZE) + DEV_REGS_START

/* trasforma in bit, dato il numero */
#define NUM_BIT(bit, n)		while (((n)--) > 0) ((bit) = ((bit) << 1))

/* trasforma in numero, dato il bit */
#define BIT_NUM(bit, n)		while (((bit) = ((bit) >> 1))) ((n)++)

/*** macro - SYSCALL ***/
#define MsgSend(dest, payload)		((U32) SYSCALL(SYS_SEND, (U32) (dest), (U32) (payload), 0))
#define MsgRecv(source, payload)	(((tid_t) SYSCALL(SYS_RECV, (U32) (source), (U32) (payload), 0)))
#define MsgReply(dest,payload)		(((tid_t) SYSCALL(SYS_REPLY, (U32) (dest), (U32) (payload), 0)))

#endif
