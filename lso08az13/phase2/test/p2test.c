/*
 * Copyright (C) Michael Goldweber, Enrico Cataldi, Mauro Morsiani
 * Renzo Davoli, Ludovico Gardenghi, Claudio Sacerdoti Coen, Matteo Bigoi,
 * Matteo Buferli, Luca Toscano.
 *
 * (2008 v.0.4)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*********************************P2TEST.C*******************************
 *
 *      Test program for the AMIKaya Kernel: phase 2.
 *
 *      Produces progress messages on Terminal0.
 *
 *      This is pretty convoluted code, so good luck!
 *
 *              Aborts as soon as an error is detected.
 *
 */

/* vedi our_const.h 
#define STCK(T) ((T) = ((* ((cpu_t *) BUS_TODLOW)) / (* ((cpu_t *) BUS_TIMESCALE))))
*/
#include "../../h/const.h"
#include "../../h/our_const.h"
#include "../../h/types.h"
#include "../../h/mikonos.h"
#include "libumps.e"

extern void SSIRequest(unsigned int service, unsigned int payload, unsigned int * reply); 

/*	 p2 tests constants	*/
#define MINLOOPTIME	30000
#define LOOPNUM 	10000

/*	 p3 tests constants	*/
#define CLOCKLOOP	10
#define MINCLOCKLOOP	3000
#define CLOCKINTERVAL 100000

/*	 p5 and p9 tests constants	*/
#define SYSCAUSE	(0x8)
#define ADDRCAUSE	(0x6)
#define UPLEVELSYS 	4
#define LOCATIONADDR	0x40000000
#define BADADDR	0xFFFFFFFF
/*	 p8 tests constants	*/
#define P8LOOP		4
#define NOLEAVES	4

/* sync code used by p1 and p2 to acknowledge each other */
#define SYNCCODE	0x10101010

/* Status register constants */
#define ALLOFF                  0x00000000
#define VMPON                   0x02000000
#define USERPON                 0x00000008
#define IEPON                   0x00000004

#define IMALLON                 0x0000FF00 /* This is a mask to set that all device interrupts are active for the thread */

#define QPAGE                   1024

state_t		p2state, p3state,p4state, p5state,p6state, p7state,p8rootstate, child1state,
		child2state, gchild1state,gchild2state, gchild3state,gchild4state,
		p9state, printstate, p5trap_s, p9trap_s;

HIDDEN int		p1p2synch,		/* to check on p1/p2 synchronization */
			p8inc,		/* p8's incarnation number */
			p4inc=1;		/* p4 incarnation number */

/* to get some fresh stack space for the second p5 thread */
memaddr p5Stack;

/* Threads main funcions */
void test(void);
HIDDEN void p2(void);
HIDDEN void p3(void);
HIDDEN void p4(void);
HIDDEN void p5(void);
HIDDEN void p5a(void);
HIDDEN void p6(void);
HIDDEN void p7(void);
HIDDEN void p5trapm(void);
HIDDEN void p8root(void);
HIDDEN void child1(void);
HIDDEN void child2(void);
HIDDEN void p8leaf(void);
HIDDEN void p9(void);
HIDDEN void p9trapm(void);
HIDDEN void printthread(void);

/* Local functions */
HIDDEN void print(char * message);
HIDDEN unsigned int termprint(char * str, unsigned int term);
HIDDEN void putChar(memaddr *command, char ch);

/* Local vars */
HIDDEN tid_t printid, p2id, p3id, p4id, p5id, p6id, p7id, p8id, p9id;


/*                 p1 -- the root process                            */
void test(void){
	unsigned int reply;
	char *tmp;


	p1p2synch = 0;
	p8inc = 0;
	p4inc = 1;

	/* Storing actual state */
	STST(&printstate);

	/* stack of printthread should sit below*/
	printstate.reg_sp = printstate.reg_sp - QPAGE;
	/* printthread starts executing his code */
	printstate.pc_epc = printstate.reg_t9 = (memaddr)printthread;
	/* Here we set that IEp is on */
	printstate.status = printstate.status | IEPON | IMALLON;

	SSIRequest(CREATE, (unsigned int) &printstate, &reply);

	if (reply == CREATENOGOOD)
		/* could not start print terminal thread */
		PANIC();

	printid=(tid_t)reply;
	tmp="print terminal thread has started\n";

	print(tmp);

	/* set up states of the other processes */

	/* set up p2's state */
	STST(&p2state);
	p2state.reg_sp = printstate.reg_sp - QPAGE;
	p2state.pc_epc = p2state.reg_t9 = (memaddr)p2;
	p2state.status = p2state.status | IEPON | IMALLON;

	STST(&p3state);
	p3state.reg_sp = p2state.reg_sp - QPAGE;
	p3state.pc_epc = p3state.reg_t9 = (memaddr)p3;
	p3state.status = p3state.status | IEPON | IMALLON;

	STST(&p4state);
	p4state.reg_sp = p3state.reg_sp - QPAGE;
	p4state.pc_epc = p4state.reg_t9 = (memaddr)p4;
	p4state.status = p4state.status | IEPON | IMALLON;

	STST(&p5state);
	p5Stack = p5state.reg_sp = p4state.reg_sp - (2 * QPAGE);
	p5state.pc_epc = p5state.reg_t9 = (memaddr)p5;
	p5state.status = p5state.status | IEPON | IMALLON;

	STST(&p5trap_s);
	p5trap_s.reg_sp = p5state.reg_sp - (3 * QPAGE);
	p5trap_s.pc_epc = p5trap_s.reg_t9 = (memaddr) p5trapm;
	p5trap_s.status = p5trap_s.status | IEPON | IMALLON;

	STST(&p6state);
	p6state.reg_sp = p5trap_s.reg_sp - QPAGE;
	p6state.pc_epc = p6state.reg_t9 = (memaddr)p6;
	p6state.status = p6state.status | IEPON | IMALLON;

	STST(&p7state);
	p7state.reg_sp = p6state.reg_sp - QPAGE;
	p7state.pc_epc = p7state.reg_t9 = (memaddr)p7;
	p7state.status = p7state.status | IEPON | IMALLON;

	STST(&p8rootstate);
	p8rootstate.reg_sp = p7state.reg_sp - QPAGE;
	p8rootstate.pc_epc = p8rootstate.reg_t9 = (memaddr)p8root;
	p8rootstate.status = p8rootstate.status | IEPON | IMALLON;

	STST(&child1state);
	child1state.reg_sp = p8rootstate.reg_sp - QPAGE;
	child1state.pc_epc = child1state.reg_t9 = (memaddr)child1;
	child1state.status = child1state.status | IEPON | IMALLON;

	STST(&child2state);
	child2state.reg_sp = child1state.reg_sp - QPAGE;
	child2state.pc_epc = child2state.reg_t9 = (memaddr)child2;
	child2state.status = child2state.status | IEPON | IMALLON;

	STST(&gchild1state);
	gchild1state.reg_sp = child2state.reg_sp - QPAGE;
	gchild1state.pc_epc = gchild1state.reg_t9 = (memaddr)p8leaf;
	gchild1state.status = gchild1state.status | IEPON | IMALLON;

	STST(&gchild2state);
	gchild2state.reg_sp = gchild1state.reg_sp - QPAGE;
	gchild2state.pc_epc = gchild2state.reg_t9 = (memaddr)p8leaf;
	gchild2state.status = gchild2state.status | IEPON | IMALLON;

	STST(&gchild3state);
	gchild3state.reg_sp = gchild2state.reg_sp - QPAGE;
	gchild3state.pc_epc = gchild3state.reg_t9 = (memaddr)p8leaf;
	gchild3state.status = gchild3state.status | IEPON | IMALLON;

	STST(&gchild4state);
	gchild4state.reg_sp = gchild3state.reg_sp - QPAGE;
	gchild4state.pc_epc = gchild4state.reg_t9 = (memaddr)p8leaf;
	gchild4state.status = gchild4state.status | IEPON | IMALLON;

	STST(&p9state);
	p9state.reg_sp = gchild4state.reg_sp - QPAGE;
	p9state.pc_epc = p9state.reg_t9 = (memaddr)p9;
	p9state.status = p9state.status | IEPON | IMALLON;

	STST(&p9trap_s);
	p9trap_s.reg_sp = p9state.reg_sp - QPAGE;
	p9trap_s.pc_epc = p9trap_s.reg_t9 = (memaddr) p9trapm;
	p9trap_s.status = p9trap_s.status | IEPON | IMALLON;

	print ("p1 setup ok: now starting the tests\n");

	/*		 start p2 thread	*/
	SSIRequest(CREATE, (unsigned int) &p2state, (unsigned int *)&reply);

	if ((int)reply == CREATENOGOOD)
		print("error: cannot create brother p2\n");
	else
		print("p2 started\n");

	p2id=reply;

	/*	 check p1/p2 message syncronization	*/
	reply=MsgSend(p2id, SYNCCODE);

	/*		 make sure they really synced	*/
	if (p1p2synch == 0)
		print("error: p1/p2 synchronization bad\n");
	else
		print("p2 provide a good synch\n");

	/*		 start p3 thread	*/
	SSIRequest(CREATE, (unsigned int) &p3state, (unsigned int *)&reply);
	p3id=reply;

	print("p3 started\n");

	/* start p3 tests and wait for p3 to end its tests	*/
	reply=MsgSend(p3id, 0); 

	/*		 start first incarnation of p4	*/
	SSIRequest(CREATE, (unsigned int) &p4state, (unsigned int *)&reply);
	p4id=reply;

	/*	 	 start p4 tests	*/
	/*		 wait for p4 first incarnation	*/
	reply=MsgSend(p4id, 0);

	/*		 ask p4 to spawn another thread	*/
	MsgSend(p4id, 0);

	/*		 start p5 thread	*/
	SSIRequest(CREATE, (unsigned int) &p5state, (unsigned int *)&reply);
	p5id=reply;

	/*		 start p5 tests	*/
	MsgSend(p5id, 0);

	/*		 start p6 thread	*/
	SSIRequest(CREATE, (unsigned int) &p6state, (unsigned int *)&reply);
	p6id=reply;

	/*		 start p6 tests	*/
	MsgSend(p6id, 0);

	/*		 start p7 thread	*/
	SSIRequest(CREATE, (unsigned int) &p7state, (unsigned int *)&reply);
	p7id=reply;

	/*		 start p7 tests	*/
	MsgSend(p7id, 0);

	/*		 this should signal p5a to continue	*/
	/*		 wait for p5a to complete handshake	*/
	MsgSend(p5id, 0);

	/*		 start p9 thread	*/
	SSIRequest(CREATE, (unsigned int) &p9state, (unsigned int *)&reply);
	p9id=reply;

	/*		 start p9 tests	*/
	MsgSend(p9id, 0);

	/*		 now a more rigorous check of process creation/termination 	*/
	for (p8inc = 0; p8inc < P8LOOP; p8inc++){
		SSIRequest(CREATE, (unsigned int) &p8rootstate, (unsigned int *)&reply);

		if ((int) (reply) == CREATENOGOOD){
			print("error in thread creation\n");
			PANIC();
		}
		p8id=reply;

		/*			 else start p8 tests 	*/
		MsgSend(p8id, 0);

		MsgRecv(p8id, &reply);
		MsgReply(p8id, 0);
	}

	print("p1 finishes OK\n");
	* ((memaddr *) BADADDR) = 0;				/* terminate p1 */

	/* should not reach this point, since p1 just got a program trap */
	print("error: p1 still alive after progtrap & no trap vector\n");
	PANIC();
}


/* p2 -- sync and cputime test thread */
void p2(void)
{
	int	i;			/* just to waste time  */
	unsigned int 	reply;
	tid_t parent;
	cpu_t	now1,now2;		/* times of day        */
	cpu_t	cpu_t1,cpu_t2;		/* cpu time used       */


	parent = MsgRecv(ANYTID, &reply);

	if (reply != SYNCCODE){
		print("error: p2 received a bad message\n");
		PANIC();
	}

	print("p2 received right message from p1\n");

	STCK(now1);				/* time of day   */
	SSIRequest(GETCPUTIME, 0, &reply);


	/* CPU time used until now*/
	cpu_t1 = (cpu_t) reply;

	/* delay for several milliseconds */
	for (i=1; i < LOOPNUM; i++);

	SSIRequest(GETCPUTIME, 0, &reply);

	cpu_t2 = (cpu_t) reply;
	STCK(now2);/* time of day  */

	if (((now2 - now1) >= (cpu_t2 - cpu_t1)) && ((cpu_t2 - cpu_t1) >= (MINLOOPTIME / (* ((cpu_t *)BUS_TIMESCALE)))))
		print("p2 is OK\n");
	else  {
		if ((now2 - now1) < (cpu_t2 - cpu_t1))
			print("error: more cpu time than real time\n");
		if ((cpu_t2 - cpu_t1) < (MINLOOPTIME / (* ((cpu_t *)BUS_TIMESCALE))))
			print("error: not enough cpu time went by\n");
		print("error: p2 blew it!\n");
	}

	p1p2synch = 1; /* p1 will check this */

	/*		 restart p1	*/
	MsgReply(parent, 0);

	SSIRequest(TERMINATE, ANYTID, &reply);	/* terminate p2 */

	/* just did a TERMINATE, so should not get to this point */
	print("error: p2 didn't terminate\n");
	PANIC();
}


/* p3 -- waitclock test thread */
void p3(void)
{
	int	i;
	unsigned int reply;
	tid_t parent;
	cpu_t	time1,time2;
	cpu_t	cpu_t1,cpu_t2;		/* cpu time used       */


	time1=0;
	time2=0;

	parent = MsgRecv(ANYTID, &reply);

	/* loop until we are delayed at least half of clock V interval */
	while (time2 - time1 < (CLOCKINTERVAL >> 1) ) {
		STCK(time1);			/* time of day     */
		SSIRequest(WAITFORCLOCK, 0, &reply);
		STCK(time2);			/* new time of day */
	}

	print("p3: WAITFORCLOCK looks OK: now a longer test\n");

	/* now let's check to see if we're really charge for CPU
	   time correctly */
	SSIRequest(GETCPUTIME, 0, &reply);
	cpu_t1 = (cpu_t) reply;

	for (i = 0; i < CLOCKLOOP; i++)
		SSIRequest(WAITFORCLOCK, 0, &reply);

	SSIRequest(GETCPUTIME, 0, &reply);
	cpu_t2 = (cpu_t) reply;

	if (cpu_t2 - cpu_t1 < (MINCLOCKLOOP / (* ((cpu_t *) BUS_TIMESCALE))))
		print("error: p3: CPU time incorrectly maintained\n");
	else
		print("p3: CPU time correctly maintained\n");

	/* restart p1 	*/
	MsgReply(parent, 0);

	SSIRequest(TERMINATE, ANYTID, &reply);			/* terminate p3    */

	/* just did a TERMINATE, so should not get to this point */
	print("error: p3 didn't terminate\n");
	PANIC();
}


/* p4 -- termination test process */
void p4(void)
{
	unsigned int reply;
	tid_t parent, son;

	parent = MsgRecv(ANYTID, &reply);

	switch (p4inc) {
		case 1:
			print("p4: first incarnation starts\n");
			p4inc++;
			break;
		case 2:
			print("p4: second incarnation starts\n");
			break;
	}
	/*		 notify p1 (or p4 first incarnation)	*/

	MsgReply(parent, 0);

	/*		 wait parent authorization to start a new thread;	*/
	/*		 p4 second incarnation will wait here forever	if no answer */
	MsgRecv(parent, &reply);

	p4state.reg_sp =   p4state.reg_sp - QPAGE;		/* give some stack space  */

	SSIRequest(CREATE, (unsigned int) &p4state, &reply); /* start a new p4    */

	if ((int)reply == CREATENOGOOD){
		print("error: p4 cannot create the second p4\n");
		PANIC();
	}
	son = (tid_t ) reply;

	/* start p4 second incarnation  */
	/* and wait for it       */
	reply=MsgSend(son, 0);			

	print("p4 is OK\n");

	/* restart p1 (p4 second incarnation will never reach this point)	*/
	MsgReply(parent, 0);

	/* terminate both p4 incarnations	*/
	SSIRequest(TERMINATE, son, &reply);
	SSIRequest(TERMINATE, ANYTID, &reply);

	/* just did a TERMINATE, so should not get to this point */
	print("error: p4 didn't terminate\n");
	PANIC(); /* PANIC */
}


/* p5 trap manager; will be terminated when p5 dies */
void p5trapm(void)
{
	unsigned int reply;
	tid_t trapped_tid;
	state_t *tstate;

	print("p5 trap mgr started\n");

	for (;;){
		/* wait for a trap to happen	*/
		trapped_tid = MsgRecv(ANYTID, &reply);
		SSIRequest(GETTSTATE, trapped_tid, &tstate);

		switch(CAUSE_EXCCODE_GET(reply)){

			case SYSCAUSE:
				if (tstate->reg_a0 == UPLEVELSYS){
					print("p5 trap mgr: got up-level SYSCALL passup\n");
					tstate->pc_epc += 4; /* to avoid SYS looping */
				}else{
					/* should be the MsgRecv done in p5a, user mode */
					print("p5 trap mgr: Kernel mode required for MsgRecv(): retrying\n");
					/* restart p5 from p5a(), kernel mode	*/
					tstate->status = tstate->status & (~((unsigned int) USERPON));
					tstate->pc_epc = tstate->reg_t9 = (memaddr)p5a;
					/* some fresh stack for it */
					tstate->reg_sp = p5Stack - (2 * QPAGE);
				}
				break;
			case ADDRCAUSE:
				print("p5 trap mgr: TLB trap; restarting as p5a, User mode\n");
				/* restart p5 from p5a(), user mode	*/
				tstate->pc_epc = tstate->reg_t9 = (memaddr)p5a;
				tstate->status = tstate->status | USERPON | IEPON;
				/* some fresh stack for it */
				tstate->reg_sp = p5Stack - QPAGE;
				break;

			default:
				/* an unexpected error has happened	*/
				print("error: unexpected trap in p5\n");
				PANIC();
				break;
		}
		MsgReply(trapped_tid, TRAPCONTINUE);
	}
}

/* p5 -- trap management test process */
void p5(void)
{
	memaddr location = 0;

	unsigned int reply;
	tid_t p5trap_m;
	tid_t p5parent;

	/* syncs with p1 */
	p5parent=MsgRecv(ANYTID, &reply);
	print("p5 starts\n");
	MsgReply(p5parent,0);

	/* setup & start p5 trap management thread */
	SSIRequest(CREATE, (unsigned int) &p5trap_s, &reply);
	p5trap_m = (tid_t) reply;

	/* specify trap manager thread for p5 */
	SSIRequest(SPECPRGMGR, (unsigned int) p5trap_m, &reply);
	SSIRequest(SPECTLBMGR, (unsigned int) p5trap_m, &reply);
	SSIRequest(SPECSYSMGR, (unsigned int) p5trap_m, &reply);

	/* this SYSCALL should be handled by p5trap_m */
	SYSCALL(UPLEVELSYS,0,0,0);

	print("p5 returns after up-level syscall\n");

	/* to cause a TLB trap access a valid but not-used kuseg location is enough */

	location = * ((memaddr *) LOCATIONADDR);

	/* after this, the trap manager will load p5a in user mode, to test other handlers */
}

/* second part of p5 - should be entered in user mode */
void p5a(void)
{
	unsigned int reply;
	tid_t thread;
	tid_t p5parent;

	/* the first time through, we are in user mode */
	/* and the MsgRecv() should generate a program trap */
	p5parent = MsgRecv(ANYTID, &reply);

	/* authorizes p1 to continue */
	MsgReply(p5parent, 0);

	print("p5a handshake with p1 OK\n");

	/* should cause a termination for p5 and his trap mgr
	   since this SSIRequest has already been done for p5 */
	SSIRequest(SPECPRGMGR, (unsigned int) NULL, &reply);

	/* should have terminated, so should not get to this point */
	print("error: p5 didn't terminate\n");
	PANIC();
}


/*p6 -- high level syscall without initializing trap vector*/
void p6(void)
{
	unsigned int reply;
	tid_t parent;

	/* waiting for a message from p1 */
	parent=MsgRecv(ANYTID, &reply);
	MsgReply(parent,0);

	print("p6 starts\n");

	/* should cause termination because p6 has no trap mgr*/
	SYSCALL(UPLEVELSYS,0,0,0);

	print("error: p6 alive after a passup with no trap vector\n");
	PANIC();
}

/*p7 -- program trap without initializing passup vector*/
void p7(void)
{
	unsigned int reply;
	tid_t parent;

	/*		 waiting for a message from p1	*/
	parent = MsgRecv(ANYTID, &reply);
	MsgReply(parent,0);

	print("p7 starts\n");

	* ((memaddr *) BADADDR) = 0;

	print("error: p7 alive after program trap with no trap vector\n");
	PANIC();
}


/* p8root -- test of termination of a subtree of threads              */
/* create a subtree of threads, wait for the leaves to block, signal  */
/* the parent process (p1), and then terminate                        */
void p8root(void)
{
	unsigned int reply;
	tid_t parent;
	int	grandchild;
	tid_t child1,child2,gctid[NOLEAVES];

	print("p8root starts\n");

	/* sync with p1	*/
	parent = MsgRecv(ANYTID, &reply);
	MsgReply(parent, 0);

	/*		 start sons		*/
	SSIRequest(CREATE, (unsigned int) &child1state, (unsigned int *)&reply);
	child1=reply;
	MsgSend(child1, 0);

	SSIRequest(CREATE, (unsigned int) &child2state, (unsigned int *)&reply);
	child2=reply;
	MsgSend(child2, 0);

	/* wait for leaves 		*/
	/* it does not reply, thus grandchildren will be killed by terminate
	 * without unblocking */
	for (grandchild = 0; grandchild < NOLEAVES; grandchild++) 
		gctid[grandchild]=MsgRecv(ANYTID, &reply);


	SSIRequest(TERMINATE, child1, &reply);
	SSIRequest(TERMINATE, child2, &reply);

	for (grandchild = 0; grandchild < NOLEAVES; grandchild++) 
		SSIRequest(TERMINATE, gctid[grandchild], &reply);

	/* signal p1 to continue	*/
	MsgSend(parent, 0);

	SSIRequest(TERMINATE, ANYTID, &reply);
}

/*child1 & child2 -- create two sub-processes each*/

void child1(void)
{
	unsigned int reply;
	tid_t parent;

	print("child1 starts\n");

	/* syncs with p8root	*/
	parent = MsgRecv(ANYTID, &reply);
	MsgReply(parent, 0);

	/* creates sons and passes them p8root address	*/
	SSIRequest(CREATE, (unsigned int) &gchild1state, &reply);
	MsgSend((tid_t ) reply, (unsigned int) parent);

	SSIRequest(CREATE, (unsigned int) &gchild2state, &reply);
	MsgSend((tid_t ) reply, (unsigned int) parent);

	/* wait forever	*/
	MsgRecv(ANYTID, &reply);

	/*		 should never get there: nobody should send message to childs	*/
	print("error: child1 received message from someone\n");
	PANIC();
}

void child2(void)
{
	unsigned int reply;
	tid_t parent;

	print("child2 starts\n");

	/*	 syncs with p8root	*/
	parent = MsgRecv(ANYTID, &reply);
	MsgReply(parent, 0);

	/*	 creates sons and passes them p8root address	*/
	SSIRequest(CREATE, (unsigned int) &gchild3state, &reply);
	MsgSend((tid_t ) reply, (unsigned int) parent);

	SSIRequest(CREATE, (unsigned int) &gchild4state, &reply);
	MsgSend((tid_t ) reply, (unsigned int) parent);

	/*		 wait forever	*/
	MsgRecv(ANYTID, &reply);

	/*		 should never get there: nobody should send message to childs	*/
	print("error: child2 received message from someone\n");
	PANIC();
}

/*p8leaf -- code for leaf processes*/
void p8leaf()
{
	unsigned int  parent,grandparent;
	unsigned int reply;

	print("leaf process starts\n");

	/* syncs with child */
	parent=MsgRecv(ANYTID,&grandparent);
	MsgReply(parent, 0);
	/* send a msg to unblock the grandparent! */
	/* and wait forever (granparent will not reply!) */
	MsgSend(grandparent, 0);

	/*		 should never get there: nobody should send message to leafs	*/
	print("error: leaf unblocked without any reply\n");
	PANIC();
}


/* p9 trap manager; should be terminated when p9 dies */
void p9trapm(void)
{
	unsigned int reply;
	tid_t trapped_tid;
	state_t *tstate;

	print("p9 trap mgr started\n");

	for(;;) {
		/*	 wait for a trap to happen	*/
		trapped_tid = MsgRecv(ANYTID, &reply);
		SSIRequest(GETTSTATE, trapped_tid, &tstate);

		if (CAUSE_EXCCODE_GET(reply) == SYSCAUSE && tstate->reg_a0 == UPLEVELSYS){
			print("SYSCALL in p9: terminate it\n");
			MsgReply(trapped_tid, TRAPTERMINATE);
		} else {
			print("error: unexpected trap in p9\n");
			PANIC();
		}
	}
}


/* p9 -- a simpler trap management test process */
void p9(void)
{
	unsigned int reply;
	tid_t parent,p9trap_m;

	/*	 syncs with p1	*/
	parent=MsgRecv(ANYTID, &reply);
	MsgReply(parent, 0);

	print("p9 starts\n");

	/*		 setup & start p9 trap management thread	*/
	SSIRequest(CREATE, (unsigned int) &p9trap_s, &reply);
	p9trap_m = (tid_t ) reply;

	/* specify trap manager thread for p9 */
	SSIRequest(SPECPRGMGR, (unsigned int) p9trap_m, &reply);
	SSIRequest(SPECTLBMGR, (unsigned int) p9trap_m, &reply);
	SSIRequest(SPECSYSMGR, (unsigned int) p9trap_m, &reply);

	/*		 this SYSCALL should be handled by p9trap_m	*/
	/*		 which should terminate 	*/
	SYSCALL(UPLEVELSYS,0,0,0);

	/* should have terminated, so should not get to this point */
	print("error: p9 didn't terminate\n");
	PANIC();
}



/* This procedure will send a message to the terminal printer thread */
void print(char * message){
	unsigned int reply;
	reply=MsgSend(printid, (unsigned int) message);
}


/* a RPC server thread which prints messages on terminal 0 */
void printthread(void)
{
	unsigned int payload;
	tid_t sender;


	for(;;){
		/* waits for a job */
		sender = MsgRecv(ANYTID, &payload);

		termprint((char *) payload,0);

		if ((MsgReply(sender, 0)) != 0)
		{
			termprint("print thread cannot give back an answer\n",0);
			PANIC();
		}
	}
}


/* This function prints a string on specified terminal and returns TRUE if print was successful, FALSE if not  */
unsigned int termprint(char *inpstr, unsigned int term){
	memaddr *statusp;
	memaddr *commandp;
	char *str;
	unsigned int stat;
	unsigned int error;

	error = FALSE;
	str=inpstr;
	/* IF the term number is correct */
	if (term < DEV_PER_INT) {
		/* Compute device register field addresses */
		statusp =  (unsigned int *) (TERMINAL0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
		commandp = (unsigned int *) (TERMINAL0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));

		/* Retrieve device status */
		stat = ((*statusp) & DEV_STATUSMASK);

		/* If device is available */
		if (stat == DEV_S_READY) {

			/* During this cycle we will print one char a time*/
			while (*str != EOS && !error) {
				/* Now we shift the char to be printed by CHAROFFSET to
				 * put its bits to the 8 higher bits and set the command code at 2*/
				putChar(commandp,*str);

				/* Waiting for IO Completion */
				SSIRequest(WAITFORIO,(unsigned int)statusp,(unsigned int *)&stat);

				str++;
			}
		}
		else
			/* Device is not available */
			error = TRUE;
	}
	else
		/* Wrong terminal device number */
		error = TRUE;

	*commandp = DEV_C_ACK;

	return (!error);
}


HIDDEN void putChar(memaddr *command, char ch){
	*command = (ch << CHAROFFSET) | DEV_PRNT_C_PRINTCHR;
}
