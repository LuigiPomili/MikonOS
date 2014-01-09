/*
 *  This file is part of MIKONOS.
 *  Copyright (C) 2004, 2005, 2008 Michael Goldweber, Davide Brini,
 *  Renzo Davoli, Ludovico Gardenghi.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Phase 1B "the real one" test interface.
 *
 * Please read p1b2test.c for more information.
 */

#ifndef P1B2TEST_E
#define P1B2TEST_E

/* Test exception handlers */
void sys_bp_handler();
void trap_handler();
void tlb_handler();
void ints_handler();

/* Test scheduling function */
void schedule();

#endif

