#ifndef SYS_E
#define SYS_E

extern void sys_bp_handler(void);

extern void SYSMsgSend(tid_t sender, tid_t destination);

extern void SYSMsgRecv(tid_t receiver, tid_t source);

extern void SYSMsgReply(tid_t replier, tid_t source);

#endif
