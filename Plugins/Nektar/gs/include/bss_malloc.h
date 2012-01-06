/********************************bss_malloc.h**********************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
11.21.97
*********************************bss_malloc.h*********************************/

/********************************bss_malloc.h**********************************
File Description:
-----------------

*********************************bss_malloc.h*********************************/
#ifndef _bss_malloc_h
#define _bss_malloc_h



/********************************bss_malloc.h**********************************
Function:

Input :
Output:
Return:
Description:
Usage:
*********************************bss_malloc.h*********************************/
extern void  bss_init(void);
extern void *bss_malloc(size_t size);
extern void  bss_free(void *ptr);
extern void  bss_stats(void);
extern int   bss_frees(void);
extern int   bss_calls(void);

extern void  perm_init(void);
extern void *perm_malloc(size_t size);
extern void  perm_free(void *ptr);
extern void  perm_stats(void);
extern int   perm_frees(void);
extern int   perm_calls(void);

#endif
