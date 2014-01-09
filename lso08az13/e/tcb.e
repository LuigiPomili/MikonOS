#ifndef TCB_E
#define TCB_E

extern void initTcbs(void);

extern void freeTcb(tcb_t *t);

extern tcb_t* allocTcb(void);

extern tcb_t* mkEmptyThreadQ(void);

extern int emptyThreadQ(tcb_t *tp);

extern void insertBackThreadQ(tcb_t **tp, tcb_t *t_ptr);

extern void insertFrontThreadQ(tcb_t **tp, tcb_t *t_ptr);

extern tcb_t* removeThreadQ(tcb_t **tp);

extern tcb_t* outThreadQ(tcb_t **tp, tcb_t *t_ptr);

extern tcb_t* headThreadQ(tcb_t *tp);

extern void initCPUSnap(state_t *p, memaddr base, unsigned char update_pc);

#endif
