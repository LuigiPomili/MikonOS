#ifndef SCHEDULER_E
#define SCHEDULER_E

extern void initScheduler(void);

extern void addThread(tid_t tid);

extern void schedule(void);

extern tcb_t** getReadyQueue(void);

extern void force_terminate(tid_t bill);

#endif
