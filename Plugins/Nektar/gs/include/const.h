/**********************************const.h*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
***********************************const.h************************************/

/**********************************const.h*************************************
File Description:
-----------------

***********************************const.h************************************/
#include <limits.h>


#define X          0
#define Y          1
#define Z          2
#define XY         3
#define XZ         4
#define YZ         5


#define THRESH          0.2
#define N_HALF          4096
#define PRIV_BUF_SZ     45

/*4096 8192 32768 65536 */
#define MAX_MSG_BUF     65536

/* fortran gs limit */
#define MAX_GS_IDS      100

#define FULL          2
#define PARTIAL       1
#define NONE          0

#define BYTE    8
#define BIT_0    0x1
#define BIT_1    0x2
#define BIT_2    0x4
#define BIT_3    0x8
#define BIT_4    0x10
#define BIT_5    0x20
#define BIT_6    0x40
#define BIT_7    0x80
#define TOP_BIT         INT_MIN
#define ALL_ONES        -1

#define FALSE    0
#define TRUE    1

#define C    0
#define FORTRAN   1


#define MAX_VEC    1674
#define FORMAT    30
#define MAX_COL_LEN      100
#define MAX_LINE  FORMAT*MAX_COL_LEN
#define   DELIM         " \n \t"
#define LINE    12
#define C_LINE    80





#define PTR_LEN    (int)sizeof(void *)
#define INT             int
#define INT_PTR_LEN  (int)sizeof(int *)
#define INT_LEN    (int)sizeof(int)
#define CHAR_LEN        (int)sizeof(char)

/* assuming LP64 for 64bit systems and standard sizeof(long) = 32 for
   32bit systems as well as sizeof(int) = sizeof(long) = 64 on _CRAYMPP
   we can assume sizeof(long) = sizeof(void *) and therefore pointer
   arithmetic can always be done with longs with no loss of accuracy */
#define PTRINT          long

#ifdef MPISRC
#define INT_TYPE  MPI_INT
#else
#define INT_TYPE  1
#endif


#if MPISRC&&r8
#define REAL_TYPE  MPI_DOUBLE
#elif MPISRC
#define REAL_TYPE  MPI_FLOAT
#else
#define REAL_TYPE  2
#endif

#ifdef MPISRC
#define DATA_TYPE  MPI_Datatype
#else
#define DATA_TYPE  INT
#endif


#define REAL float
#define REAL_LEN  (int)sizeof(float)
#define REAL_MAX  FLT_MAX
#define REAL_MIN  FLT_MIN




/* -Dr8 as compile line arg for c compiler */
#ifdef  r8
#undef  REAL
#undef  REAL_LEN
#undef  REAL_MAX
#undef  REAL_MIN
#define REAL            double
#define REAL_LEN  (int)sizeof(double)
#define REAL_MAX  DBL_MAX
#define REAL_MIN  DBL_MIN
#endif

#define   ERROR        -1
#define   PASS          1
#define   FAIL          0


#define   UT            5               /* dump upper 1/2 */
#define   LT            6               /* dump lower 1/2 */
#define   SYMM          8               /* we assume symm and dump upper 1/2 */
#define   NON_SYMM      9

#define   ROW          10
#define   COL          11

#ifdef r8
#define EPS   1.0e-14
#else
#define EPS   1.0e-06
#endif

#ifdef r8
#define EPS2  1.0e-07
#else
#define EPS2  1.0e-03
#endif

#define MPI   1
#define NX    2


#define LOG2(x)    (REAL)log((double)x)/log(2)
#define SWAP(a,b)       temp=(a); (a)=(b); (b)=temp;
#define P_SWAP(a,b)     ptr=(a); (a)=(b); (b)=ptr;
#define MAX(x,y)        ((x)>(y)) ? (x) : (y)
#define MIN(x,y)        ((x)<(y)) ? (x) : (y)

#define MAX_FABS(x,y)   ((double)fabs(x)>(double)fabs(y)) ? ((REAL)x) : ((REAL)y)
#define MIN_FABS(x,y)   ((double)fabs(x)<(double)fabs(y)) ? ((REAL)x) : ((REAL)y)

/* specer's existence ... can be done w/MAX_ABS */
#define EXISTS(x,y)     ((x)==0.0) ? (y) : (x)

#define MULT_NEG_ONE(a) (a) *= -1;
#define NEG(a)          (a) |= BIT_31;
#define POS(a)          (a) &= INT_MAX;
