/**********************************ivec.h**************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
***********************************ivec.h*************************************/

/**********************************ivec.h**************************************
File Description:
-----------------

***********************************ivec.h*************************************/
#ifndef _ivec_h
#define _ivec_h


#define SORT_REAL    1
#define SORT_INTEGER          0
#define SORT_INT_PTR          2


#define NON_UNIFORM     0
#define GL_MAX          1
#define GL_MIN          2
#define GL_MULT         3
#define GL_ADD          4
#define GL_B_XOR        5
#define GL_B_OR         6
#define GL_B_AND        7
#define GL_L_XOR        8
#define GL_L_OR         9
#define GL_L_AND        10
#define GL_MAX_ABS      11
#define GL_MIN_ABS      12
#define GL_EXISTS       13



/**********************************ivec.h**************************************
Function:

Input :
Output:
Return:
Description:
Usage:
***********************************ivec.h*************************************/
extern int *ivec_copy(int *arg1, int *arg2, int n);
/*void ivec_copy(int *arg1, int *arg2, int n); */

extern void ivec_comp(int *arg1, int n);

extern int ivec_reduce_and(int *arg1, int n);
extern int ivec_reduce_or(int *arg1, int n);

extern void ivec_zero(int *arg1, int n);
extern void ivec_pos_one(int *arg1, int n);
extern void ivec_neg_one(int *arg1, int n);
extern void ivec_set(int *arg1, int arg2, int n);
extern int ivec_cmp(int *arg1, int *arg2, int n);

extern int ivec_lb(int *work, int n);
extern int ivec_ub(int *work, int n);
extern int ivec_sum(int *arg1, int n);
extern int ivec_u_sum(unsigned *arg1, int n);
extern int ivec_prod(int *arg1, int n);

extern vfp ivec_fct_addr(int type);

extern void ivec_non_uniform(int *arg1, int *arg2, int n, int *arg3);
extern void ivec_max(int *arg1, int *arg2, int n);
extern void ivec_min(int *arg1, int *arg2, int n);
extern void ivec_mult(int *arg1, int *arg2, int n);
extern void ivec_add(int *arg1, int *arg2, int n);
extern void ivec_xor(int *arg1, int *arg2, int n);
extern void ivec_or(int *arg1, int *arg2, int len);
extern void ivec_and(int *arg1, int *arg2, int len);
extern void ivec_lxor(int *arg1, int *arg2, int n);
extern void ivec_lor(int *arg1, int *arg2, int len);
extern void ivec_land(int *arg1, int *arg2, int len);

extern void ivec_or3 (int *arg1, int *arg2, int *arg3, int len);
extern void ivec_and3(int *arg1, int *arg2, int *arg3, int n);

extern int ivec_split_buf(int *buf1, int **buf2, int size);


extern void ivec_sort_companion(int *ar, int *ar2, int size);
extern void ivec_sort(int *ar, int size);
extern void SMI_sort(void *ar1, void *ar2, int size, int type);
extern int ivec_binary_search(int item, int *list, int n);
extern int ivec_linear_search(int item, int *list, int n);

extern void ivec_c_index(int *arg1, int n);
extern void ivec_fortran_index(int *arg1, int n);
extern void ivec_sort_companion_hack(int *ar, int **ar2, int size);



extern void rvec_zero(REAL *arg1, int n);
extern void rvec_one(REAL *arg1, int n);
extern void rvec_neg_one(REAL *arg1, int n);
extern void rvec_set(REAL *arg1, REAL arg2, int n);
extern void rvec_copy(REAL *arg1, REAL *arg2, int n);

extern void rvec_scale(REAL *arg1, REAL arg2, int n);

extern vfp rvec_fct_addr(int type);
extern void rvec_add(REAL *arg1, REAL *arg2, int n);
extern void rvec_mult(REAL *arg1, REAL *arg2, int n);
extern void rvec_max(REAL *arg1, REAL *arg2, int n);
extern void rvec_max_abs(REAL *arg1, REAL *arg2, int n);
extern void rvec_min(REAL *arg1, REAL *arg2, int n);
extern void rvec_min_abs(REAL *arg1, REAL *arg2, int n);
extern void vec_exists(REAL *arg1, REAL *arg2, int n);


extern void rvec_sort(REAL *ar, int size);
extern void rvec_sort_companion(REAL *ar, int *ar2, int size);

extern REAL rvec_dot(REAL *arg1, REAL *arg2, int n);

extern void rvec_axpy(REAL *arg1, REAL *arg2, REAL scale, int n);

extern int  rvec_binary_search(REAL item, REAL *list, int rh);

#endif
