#ifndef TID_E
#define TID_E

extern void initTidTable(void);

extern tid_t newTcb(void);

extern void killTcb(tid_t tid);

extern tcb_t* resolveTid(tid_t tid);

#endif
