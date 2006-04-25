/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    PID saving code -- isolated for convenience
*/

#ifndef OWPID_H  /* tedious wrapper */
#define OWPID_H


/* PID file nsme */
extern int pid_created ;
extern char * pid_file ;

void PIDstart( void ) ;
void PIDstop( void ) ;

#endif /* OWPID_H */
