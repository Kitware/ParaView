/********************************bit_mask.h************************************
SPARSE GATHER-SCATTER PACKAGE: bit_mask bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
11.21.97
*********************************bit_mask.h***********************************/

/********************************bit_mask.h************************************
File Description:
-----------------

*********************************bit_mask.h***********************************/
#ifndef _bit_mask_h
#define _bit_mask_h


/********************************bit_mask.h************************************
Function:

Input :
Output:
Return:
Description:
Usage:
*********************************bit_mask.h***********************************/
extern int div_ceil(int numin, int denom);
extern void set_bit_mask(int *bm, int len, int val);
extern int len_bit_mask(int num_items);
extern int ct_bits(char *ptr, int n);
extern void bm_to_proc(char *ptr, int p_mask, int *msg_list);
extern int len_buf(int item_size, int num_items);

#endif
