/* 
 * $Id$
 */

#ifndef TELNET_SESSION_H
#define TELNET_SESSION_H

/* size of buffer */
#define BUF_LEN 2048

/* information on a telnet session */
typedef struct {
    int in_fd;
    int in_errno; 
    int in_eof; 
    int in_take;
    int in_add;
    char in_buf[BUF_LEN];
    int in_buflen;

    int in_status;

    int out_fd;
    int out_errno; 
    int out_eof; 
    int out_take;
    int out_add;
    char out_buf[BUF_LEN];
    int out_buflen;
} telnet_session_t;

/* functions */
void telnet_session_init(telnet_session_t *t, int in, int out);
int telnet_session_print(telnet_session_t *t, const char *s);
int telnet_session_println(telnet_session_t *t, const char *s);
int telnet_session_readln(telnet_session_t *t, char *buf, int buflen);
void telnet_session_destroy(telnet_session_t *t);

#endif /* TELNET_SESSION_H */

