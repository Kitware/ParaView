/*
 * Error conditions for the vector library
 */

#ifndef VECERR
#define VECERR

#define vFATAL    100        /* ------  FATAL errors after this ------- */
#define vNOMEM    101

void     vecerr       (char *sec, int msgid);

#endif
