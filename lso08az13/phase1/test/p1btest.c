/*****************************************************************************
 * Copyright 2004, 2005, 2008 Michael Goldweber, Davide Brini, Renzo Davoli. *
 *                                                                           *
 * This file is part of mikonos.                                             *
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
tid_t tidp[MAXPROC];
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

	initTidTable();
	addokbuf("Initialized tid table   \n");

	/* Check allocTcb */
	for (i = 0; i < MAXPROC; i++) {
		if ((tidp[i] = newTcb()) == 255)
			adderrbuf("newTcp(): unexpected 255   ");
		if ((threadp[i] = resolveTid(tidp[i])) == NULL)
			adderrbuf("resolveTid(): unexpected NULL   ");
	}
	if (newTcb() != 255) {
		adderrbuf("newTcb(): allocated more than MAXPROC entries   ");
	}
	addokbuf("tid/tcb table ok   \n");

	/* return the last 10 entries back to free list */
	for (i = 10; i < MAXPROC; i++)
		killTcb(tidp[i]);
	addokbuf("freed 10 entries   \n");

	for (i = 0; i < 10; i++) {
		if (threadp[i] != resolveTid(tidp[i]))
			adderrbuf("resolveTid(): wrong value   ");
	}

	for (i = 10; i < MAXPROC; i++) {
		if (resolveTid(tidp[i]) != NULL)
			adderrbuf("resolveTid(): wrong value   ");
	}

	addokbuf("tid management queues module ok      \n");
	addokbuf("they lived for a long time afterwards, happy and contented.\n");
  
	return 0;

}

