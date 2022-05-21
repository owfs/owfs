/*    
	Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/* control over users of owlib -- the programs */
/* This file doesn't stand alone, it must be included in ow.h */

#ifndef OW_PROGRAMS_H			/* tedious wrapper */
#define OW_PROGRAMS_H

extern void set_signal_handlers(void (*exit_handler)
	(int signo, siginfo_t * info, void *context));
void set_exit_signal_handlers(void (*exit_handler)
	(int signo, siginfo_t * info, void *context));

#ifndef SI_FROMUSER
#define SI_FROMUSER(siptr)      ((siptr)->si_code <= 0)
#endif
#ifndef SI_FROMKERNEL
#define SI_FROMKERNEL(siptr)    ((siptr)->si_code > 0)
#endif

extern int main_threadid_init ;
extern pthread_t main_threadid;
#define IS_MAINTHREAD ( (main_threadid_init==1) && (main_threadid == pthread_self()) )

/* Exit handler */	
void exit_handler(int signo, siginfo_t * info, void *context) ;
void ow_exit(int exit_code) ;

#endif							/* OW_PROGRAMS_H */
