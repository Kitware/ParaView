/**********************************types.h*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
***********************************types.h************************************/

/**********************************types.h*************************************
File Description:
-----------------

***********************************types.h************************************/

typedef void (*vfp)();
typedef void (*rbfp)(REAL *, REAL *, int len);
#ifdef MPISRC
#define vbfp MPI_User_function *
#else
typedef void (*vbfp)(void *, void *, int len, DATA_TYPE dt);
#endif
typedef int (*bfp)(void *, void *, int *len, DATA_TYPE *dt);
/* typedef REAL (*bfp)(REAL, REAL); */
