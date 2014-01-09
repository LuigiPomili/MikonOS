/*****************************************************************************
 * Copyright 2004, 2005 Michael Goldweber, Davide Brini,  Renzo Davoli       *
 *                                                                           *
 * This file is part of kaya.                                                *
 *                                                                           *
 * kaya is free software; you can redistribute it and/or modify it under the *
 * terms of the GNU General Public License as published by the Free Software *
 * Foundation; either version 2 of the License, or (at your option) any      *
 * later version.                                                            *
 * This program is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of                *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General *
 * Public License for more details.                                          *
 * You should have received a copy of the GNU General Public License along   *
 * with this program; if not, write to the Free Software Foundation, Inc.,   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.                  *
 *****************************************************************************/

/*********************************P1TEST.C*******************************
 *
 *	Test program for the modules ASL and tcbQueues (phase 1).
 *
 *	Produces progress messages on terminal 0 in addition 
 *		to the array ``okbuf[]''
 *		Error messages will also appear on terminal 0 in 
 *		addition to the array ``errbuf[]''.
 *
 *		Aborts as soon as an error is detected.
 *
 *    Modified by Davide Brini on Nov 02, 2004
 */

#include "../../h/const.h"
#include "../../h/types.h"

#include "../../h/mikonos.h"
#include "../../e/tcb.e"
#include "../../e/tid.e"

#define	MAXSEM	MAXPROC

char okbuf[2048];			/* sequence of progress messages */
char errbuf[128];			/* contains reason for failing */
char msgbuf[128];			/* nonrecoverable error message before shut down */
int onesem;
tcb_t	*threadp[MAXPROC], *p, *qa, *q, *firstthread, *lastthread, *midthread;
char *mp = okbuf;

#define TRANSMITTED	5
#define TRANSTATUS    2
#define ACK	1
#define PRINTCHR	2
#define CHAROFFSET	8
#define STATUSMASK	0xFF
#define	TERM0ADDR	0x10000250
#define DEVREGSIZE 16       /* device register size in bytes */
#define READY     1
#define DEVREGLEN   4
#define TRANCOMMAND   3
#define BUSY      3

typedef unsigned int devreg;

/* This function returns the terminal transmitter status value given its address */ 
devreg termstat(memaddr *stataddr) {
	return((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if 
 * print was successful, FALSE if not   */
unsigned int termprint(char * str, unsigned int term) {

	memaddr *statusp;
	memaddr *commandp;
	
	devreg stat;
	devreg cmd;
	
	unsigned int error = FALSE;
	
	if (term < DEV_PER_INT) {
		/* terminal is correct */
		/* compute device register field addresses */
		statusp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
		commandp = (devreg *) (TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));
		
		/* test device status */
		stat = termstat(statusp);
		if ((stat == READY) || (stat == TRANSMITTED)) {
			/* device is available */
			
			/* print cycle */
			while ((*str != '\0') && (!error)) {
				cmd = (*str << CHAROFFSET) | PRINTCHR;
				*commandp = cmd;

				/* busy waiting */
				while ((stat = termstat(statusp)) == BUSY);
				
				/* end of wait */
				if (stat != TRANSMITTED) {
					error = TRUE;
				} else {
					/* move to next char */
					str++;
				}
			} 
		}	else {
			/* device is not available */
			error = TRUE;
		}
	}	else {
		/* wrong terminal device number */
		error = TRUE;
	}

	return (!error);		
}


/* This function places the specified character string in okbuf and
 *	causes the string to be written out to terminal0 */
void addokbuf(char *strp) {
	char *tstrp = strp;
	
	while ((*mp++ = *strp++) != '\0');
	mp--; 
	
	termprint(tstrp, 0); 
}


/* This function places the specified character string in errbuf and
 *	causes the string to be written out to terminal0.  After this is done
 *	the system shuts down with a panic message */
void adderrbuf(char *strp) {

	char *ep = errbuf; 
	char *tstrp = strp;
	
	while ((*ep++ = *strp++) != '\0'); 
	
	termprint(tstrp, 0); 
		
	PANIC();
}



int main() {
	int i;

	initTcbs();
	addokbuf("Initialized thread control blocks   \n");

	/* Check allocTcb */
	for (i = 0; i < MAXPROC; i++) {
		if ((threadp[i] = allocTcb()) == NULL)
			adderrbuf("allocTcb(): unexpected NULL   ");
	}
	if (allocTcb() != NULL) {
		adderrbuf("allocTcb(): allocated more than MAXPROC entries   ");
	}
	addokbuf("allocTcb ok   \n");

	/* return the last 10 entries back to free list */
	for (i = 10; i < MAXPROC; i++)
		freeTcb(threadp[i]);
	addokbuf("freed 10 entries   \n");

	/* create a 10-element thread queue */
	qa = mkEmptyThreadQ();
	if (!emptyThreadQ(qa)) adderrbuf("emptyThreadQ(qa): unexpected FALSE   ");
	addokbuf("Inserting...   \n");
	for (i = 0; i < 10; i++) {
		if ((q = allocTcb()) == NULL)
			adderrbuf("allocTcb(): unexpected NULL while insert   ");
		switch (i) {
		case 0:
			firstthread = q;
			break;
		case 5:
			midthread = q;
			break;
		case 9:
			lastthread = q;
			break;
		default:
			break;
		}
		insertBackThreadQ(&qa, q);
	}
	addokbuf("inserted 10 elements   \n");

	if (emptyThreadQ(qa)) adderrbuf("emptyThreadQ(qa): unexpected TRUE"   );

	/* Check outThreadQ and headThreadQ */
	if (headThreadQ(qa) != firstthread)
		adderrbuf("headThreadQ(qa) failed   ");
		
	q = outThreadQ(&qa, firstthread);
	if ((q == NULL) || (q != firstthread))
		adderrbuf("outThreadQ(&qa, firstthread) failed on first entry   ");		
	freeTcb(q);
	
	q = outThreadQ(&qa, midthread);
	if (q == NULL || q != midthread)
		adderrbuf("outThreadQ(&qa, midthread) failed on middle entry   ");
	freeTcb(q);

	if (outThreadQ(&qa, threadp[0]) != NULL)
		adderrbuf("outThreadQ(&qa, threadp[0]) failed on nonexistent entry   ");
	addokbuf("outThreadQ() ok   \n");

	/* Check if removeThread and insertThread remove in the correct order */
	addokbuf("Removing...   \n");
	for (i = 0; i < 8; i++) {
		if ((q = removeThreadQ(&qa)) == NULL)
			adderrbuf("removeThreadQ(&qa): unexpected NULL   ");
		freeTcb(q);
	}
	
	if (q != lastthread)
		adderrbuf("removeThreadQ(): failed on last entry   ");
		
	if (removeThreadQ(&qa) != NULL)
		adderrbuf("removeThreadQ(&qa): removes too many entries   ");

  if (!emptyThreadQ(qa))
    adderrbuf("emptyThreadQ(qa): unexpected FALSE   ");

	addokbuf("insertThreadQ(), removeThreadQ() and emptyThreadQ() ok   \n");
	addokbuf("thread queues module ok      \n");

	addokbuf("After all, tomorrow is another day!\n");
  
	return 0;

}

