/**********************************ivec.c**************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
***********************************ivec.c*************************************/

/**********************************ivec.c**************************************
File Description:
-----------------

***********************************ivec.c*************************************/
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>

#ifdef MPISRC
#include <mpi.h>
#endif


#include "const.h"
#include "types.h"
#include "ivec.h"
#include "error.h"
#include "comm.h"


/* sorting args ivec.c ivec.c ... */
#define   SORT_OPT  6
#define   SORT_STACK  500


/* allocate an address and size stack for sorter(s) */
static void *offset_stack[2*SORT_STACK];
static int   size_stack[SORT_STACK];
static PTRINT psize_stack[SORT_STACK];



/**********************************ivec.c**************************************
Function ivec_copy()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int *
ivec_copy(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ = *arg2++;}
  return(arg1);
}



/**********************************ivec.c**************************************
Function ivec_zero()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_zero(register int *arg1, register int n)
{
  while (n--)  {*arg1++ = 0;}
}



/**********************************ivec.c**************************************
Function ivec_comp()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_comp(register int *arg1, register int n)
{
  while (n--)  {*arg1 = ~*arg1; arg1++;}
}



/**********************************ivec.c**************************************
Function ivec_neg_one()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_neg_one(register int *arg1, register int n)
{
  while (n--)  {*arg1++ = -1;}
}



/**********************************ivec.c**************************************
Function ivec_pos_one()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_pos_one(register int *arg1, register int n)
{
  while (n--)  {*arg1++ = 1;}
}



/**********************************ivec.c**************************************
Function ivec_c_index()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_c_index(register int *arg1, register int n)
{
  register int i=0;


  while (n--)  {*arg1++ = i++;}
}



/**********************************ivec.c**************************************
Function ivec_fortran_index()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_fortran_index(register int *arg1, register int n)
{
  register int i=0;


  while (n--)  {*arg1++ = ++i;}
}



/**********************************ivec.c**************************************
Function ivec_set()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_set(register int *arg1, register int arg2, register int n)
{
  while (n--)  {*arg1++ = arg2;}
}



/**********************************ivec.c**************************************
Function ivec_cmp()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_cmp(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {if (*arg1++ != *arg2++)  {return(FALSE);}}
  return(TRUE);
}



/**********************************ivec.c**************************************
Function ivec_max()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_max(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1 = MAX(*arg1,*arg2); arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_min()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_min(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*(arg1) = MIN(*arg1,*arg2); arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_mult()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_mult(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ *= *arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_add()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_add(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ += *arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_lxor()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_lxor(register int *arg1, register int *arg2, register int n)
{
  while (n--) {*arg1=((*arg1 || *arg2) && !(*arg1 && *arg2)) ; arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_xor()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_xor(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ ^= *arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_or()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_or(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ |= *arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_lor()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_lor(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1 = (*arg1 || *arg2); arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_or3()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_or3(register int *arg1, register int *arg2, register int *arg3,
   register int n)
{
  while (n--)  {*arg1++ = (*arg2++ | *arg3++);}
}



/**********************************ivec.c**************************************
Function ivec_and()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_and(register int *arg1, register int *arg2, register int n)
{
  while (n--)  {*arg1++ &= *arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_land()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_land(register int *arg1, register int *arg2, register int n)
{
  while (n--) {*arg1++ = (*arg1 && *arg2); arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function ivec_and3()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_and3(register int *arg1, register int *arg2, register int *arg3,
    register int n)
{
  while (n--)  {*arg1++ = (*arg2++ & *arg3++);}
}



/**********************************ivec.c**************************************
Function ivec_sum

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_sum(register int *arg1, register int n)
{
  register int tmp = 0;


  while (n--) {tmp += *arg1++;}
  return(tmp);
}



/**********************************ivec.c**************************************
Function ivec_reduce_and

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_reduce_and(register int *arg1, register int n)
{
  register int tmp = ALL_ONES;


  while (n--) {tmp &= *arg1++;}
  return(tmp);
}



/**********************************ivec.c**************************************
Function ivec_reduce_or

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_reduce_or(register int *arg1, register int n)
{
  register int tmp = 0;


  while (n--) {tmp |= *arg1++;}
  return(tmp);
}



/**********************************ivec.c**************************************
Function ivec_prod

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_prod(register int *arg1, register int n)
{
  register int tmp = 1;


  while (n--)  {tmp *= *arg1++;}
  return(tmp);
}



/**********************************ivec.c**************************************
Function ivec_u_sum

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_u_sum(register unsigned *arg1, register int n)
{
  register unsigned tmp = 0;


  while (n--)  {tmp += *arg1++;}
  return(tmp);
}



/**********************************ivec.c**************************************
Function ivec_lb()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_lb(register int *arg1, register int n)
{
  register int min = INT_MAX;


  while (n--)  {min = MIN(min,*arg1); arg1++;}
  return(min);
}



/**********************************ivec.c**************************************
Function ivec_ub()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_ub(register int *arg1, register int n)
{
  register int max = INT_MIN;


  while (n--)  {max = MAX(max,*arg1); arg1++;}
  return(max);
}



/**********************************ivec.c**************************************
Function split_buf()

Input :
Output:
Return:
Description:

assumes that sizeof(int) == 4bytes!!!
***********************************ivec.c*************************************/
int
ivec_split_buf(int *buf1, int **buf2, register int size)
{
  *buf2 = (buf1 + (size>>3));
  return(size);
}



/**********************************ivec.c**************************************
Function ivec_non_uniform()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
ivec_non_uniform(int *arg1, int *arg2, register int n, register int *arg3)
{
  register int i, j, type;


  /* LATER: if we're really motivated we can sort and then unsort */
  for (i=0;i<n;)
    {
      /* clump 'em for now */
      j=i+1;
      type = arg3[i];
      while ((j<n)&&(arg3[j]==type))
  {j++;}

      /* how many together */
      j -= i;

      /* call appropriate ivec function */
      if (type == GL_MAX)
  {ivec_max(arg1,arg2,j);}
      else if (type == GL_MIN)
  {ivec_min(arg1,arg2,j);}
      else if (type == GL_MULT)
  {ivec_mult(arg1,arg2,j);}
      else if (type == GL_ADD)
  {ivec_add(arg1,arg2,j);}
      else if (type == GL_B_XOR)
  {ivec_xor(arg1,arg2,j);}
      else if (type == GL_B_OR)
  {ivec_or(arg1,arg2,j);}
      else if (type == GL_B_AND)
  {ivec_and(arg1,arg2,j);}
      else if (type == GL_L_XOR)
  {ivec_lxor(arg1,arg2,j);}
      else if (type == GL_L_OR)
  {ivec_lor(arg1,arg2,j);}
      else if (type == GL_L_AND)
  {ivec_land(arg1,arg2,j);}
      else
  {error_msg_fatal("unrecognized type passed to ivec_non_uniform()!");}

      arg1+=j; arg2+=j; i+=j;
    }
}



/**********************************ivec.c**************************************
Function ivec_addr()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
vfp ivec_fct_addr(register int type)
{
  if (type == NON_UNIFORM)
    {return(&ivec_non_uniform);}
  else if (type == GL_MAX)
    {return(&ivec_max);}
  else if (type == GL_MIN)
    {return(&ivec_min);}
  else if (type == GL_MULT)
    {return(&ivec_mult);}
  else if (type == GL_ADD)
    {return(&ivec_add);}
  else if (type == GL_B_XOR)
    {return(&ivec_xor);}
  else if (type == GL_B_OR)
    {return(&ivec_or);}
  else if (type == GL_B_AND)
    {return(&ivec_and);}
  else if (type == GL_L_XOR)
    {return(&ivec_lxor);}
  else if (type == GL_L_OR)
    {return(&ivec_lor);}
  else if (type == GL_L_AND)
    {return(&ivec_land);}

  /* catch all ... not good if we get here */
  return(NULL);
}


/**********************************ivec.c**************************************
Function ct_bits()

Input :
Output:
Return:
Description: MUST FIX THIS!!!
***********************************ivec.c*************************************/
static
int
ivec_ct_bits(register int *ptr, register int n)
{
  register int tmp=0;


  /* should expand to full 32 bit */
  while (n--)
    {
      if (*ptr&128) {tmp++;}
      if (*ptr&64)  {tmp++;}
      if (*ptr&32)  {tmp++;}
      if (*ptr&16)  {tmp++;}
      if (*ptr&8)   {tmp++;}
      if (*ptr&4)   {tmp++;}
      if (*ptr&2)   {tmp++;}
      if (*ptr&1)   {tmp++;}
      ptr++;
    }

  return(tmp);
}



/******************************************************************************
Function: ivec_sort().

Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
ivec_sort(register int *ar, register int size)
{
  register int *pi, *pj, temp;
  register int **top_a = (int **) offset_stack;
  register int *top_s = size_stack, *bottom_s = size_stack;


  /* we're really interested in the offset of the last element */
  /* ==> length of the list is now size + 1                    */
  size--;

  /* do until we're done ... return when stack is exhausted */
  for (;;)
    {
      /* if list is large enough use quicksort partition exchange code */
      if (size > SORT_OPT)
  {
    /* start up pointer at element 1 and down at size     */
    pi = ar+1;
    pj = ar+size;

    /* find middle element in list and swap w/ element 1 */
    SWAP(*(ar+(size>>1)),*pi)

    /* order element 0,1,size-1 st {M,L,...,U} w/L<=M<=U */
    /* note ==> pivot_value in index 0                   */
    if (*pi > *pj)
      {SWAP(*pi,*pj)}
    if (*ar > *pj)
      {SWAP(*ar,*pj)}
    else if (*pi > *ar)
      {SWAP(*(ar),*(ar+1))}

    /* partition about pivot_value ...                        */
    /* note lists of length 2 are not guaranteed to be sorted */
    for(;;)
      {
        /* walk up ... and down ... swap if equal to pivot! */
        do pi++; while (*pi<*ar);
        do pj--; while (*pj>*ar);

        /* if we've crossed we're done */
        if (pj<pi) break;

        /* else swap */
        SWAP(*pi,*pj)
      }

    /* place pivot_value in it's correct location */
    SWAP(*ar,*pj)

    /* test stack_size to see if we've exhausted our stack */
    if (top_s-bottom_s >= SORT_STACK)
      {error_msg_fatal("ivec_sort() :: STACK EXHAUSTED!!!");}

    /* push right hand child iff length > 1 */
    if (*top_s = size-((int) (pi-ar)))
      {
        *(top_a++) = pi;
        size -= *top_s+2;
        top_s++;
      }
    /* set up for next loop iff there is something to do */
    else if (size -= *top_s+2)
      {;}
    /* might as well pop - note NR_OPT >=2 ==> we're ok! */
    else
      {
        ar = *(--top_a);
        size = *(--top_s);
      }
  }

      /* else sort small list directly then pop another off stack */
      else
  {
    /* insertion sort for bottom */
          for (pj=ar+1;pj<=ar+size;pj++)
            {
              temp = *pj;
              for (pi=pj-1;pi>=ar;pi--)
                {
                  if (*pi <= temp) break;
                  *(pi+1)=*pi;
                }
              *(pi+1)=temp;
      }

    /* check to see if stack is exhausted ==> DONE */
    if (top_s==bottom_s) return;

    /* else pop another list from the stack */
    ar = *(--top_a);
    size = *(--top_s);
  }
    }
}



/******************************************************************************
Function: ivec_sort_companion().

Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
ivec_sort_companion(register int *ar, register int *ar2, register int size)
{
  register int *pi, *pj, temp, temp2;
  register int **top_a = (int **)offset_stack;
  register int *top_s = size_stack, *bottom_s = size_stack;
  register int *pi2, *pj2;
  register int mid;


  /* we're really interested in the offset of the last element */
  /* ==> length of the list is now size + 1                    */
  size--;

  /* do until we're done ... return when stack is exhausted */
  for (;;)
    {
      /* if list is large enough use quicksort partition exchange code */
      if (size > SORT_OPT)
  {
    /* start up pointer at element 1 and down at size     */
    mid = size>>1;
    pi = ar+1;
    pj = ar+mid;
    pi2 = ar2+1;
    pj2 = ar2+mid;

    /* find middle element in list and swap w/ element 1 */
    SWAP(*pi,*pj)
    SWAP(*pi2,*pj2)

    /* order element 0,1,size-1 st {M,L,...,U} w/L<=M<=U */
    /* note ==> pivot_value in index 0                   */
    pj = ar+size;
    pj2 = ar2+size;
    if (*pi > *pj)
      {SWAP(*pi,*pj) SWAP(*pi2,*pj2)}
    if (*ar > *pj)
      {SWAP(*ar,*pj) SWAP(*ar2,*pj2)}
    else if (*pi > *ar)
      {SWAP(*(ar),*(ar+1)) SWAP(*(ar2),*(ar2+1))}

    /* partition about pivot_value ...                        */
    /* note lists of length 2 are not guaranteed to be sorted */
    for(;;)
      {
        /* walk up ... and down ... swap if equal to pivot! */
        do {pi++; pi2++;} while (*pi<*ar);
        do {pj--; pj2--;} while (*pj>*ar);

        /* if we've crossed we're done */
        if (pj<pi) break;

        /* else swap */
        SWAP(*pi,*pj)
        SWAP(*pi2,*pj2)
      }

    /* place pivot_value in it's correct location */
    SWAP(*ar,*pj)
    SWAP(*ar2,*pj2)

    /* test stack_size to see if we've exhausted our stack */
    if (top_s-bottom_s >= SORT_STACK)
      {error_msg_fatal("ivec_sort_companion() :: STACK EXHAUSTED!!!");}

    /* push right hand child iff length > 1 */
    if (*top_s = size-((int) (pi-ar)))
      {
        *(top_a++) = pi;
        *(top_a++) = pi2;
        size -= *top_s+2;
        top_s++;
      }
    /* set up for next loop iff there is something to do */
    else if (size -= *top_s+2)
      {;}
    /* might as well pop - note NR_OPT >=2 ==> we're ok! */
    else
      {
        ar2 = *(--top_a);
        ar  = *(--top_a);
        size = *(--top_s);
      }
  }

      /* else sort small list directly then pop another off stack */
      else
  {
    /* insertion sort for bottom */
          for (pj=ar+1, pj2=ar2+1;pj<=ar+size;pj++,pj2++)
            {
              temp = *pj;
              temp2 = *pj2;
              for (pi=pj-1,pi2=pj2-1;pi>=ar;pi--,pi2--)
                {
                  if (*pi <= temp) break;
                  *(pi+1)=*pi;
                  *(pi2+1)=*pi2;
                }
              *(pi+1)=temp;
              *(pi2+1)=temp2;
      }

    /* check to see if stack is exhausted ==> DONE */
    if (top_s==bottom_s) return;

    /* else pop another list from the stack */
    ar2 = *(--top_a);
    ar  = *(--top_a);
    size = *(--top_s);
  }
    }
}



/******************************************************************************
Function: ivec_sort_companion_hack().

Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
ivec_sort_companion_hack(register int *ar, register int **ar2,
       register int size)
{
  register int *pi, *pj, temp, *ptr;
  register int **top_a = (int **)offset_stack;
  register int *top_s = size_stack, *bottom_s = size_stack;
  register int **pi2, **pj2;
  register int mid;


  /* we're really interested in the offset of the last element */
  /* ==> length of the list is now size + 1                    */
  size--;

  /* do until we're done ... return when stack is exhausted */
  for (;;)
    {
      /* if list is large enough use quicksort partition exchange code */
      if (size > SORT_OPT)
  {
    /* start up pointer at element 1 and down at size     */
    mid = size>>1;
    pi = ar+1;
    pj = ar+mid;
    pi2 = ar2+1;
    pj2 = ar2+mid;

    /* find middle element in list and swap w/ element 1 */
    SWAP(*pi,*pj)
    P_SWAP(*pi2,*pj2)

    /* order element 0,1,size-1 st {M,L,...,U} w/L<=M<=U */
    /* note ==> pivot_value in index 0                   */
    pj = ar+size;
    pj2 = ar2+size;
    if (*pi > *pj)
      {SWAP(*pi,*pj) P_SWAP(*pi2,*pj2)}
    if (*ar > *pj)
      {SWAP(*ar,*pj) P_SWAP(*ar2,*pj2)}
    else if (*pi > *ar)
      {SWAP(*(ar),*(ar+1)) P_SWAP(*(ar2),*(ar2+1))}

    /* partition about pivot_value ...                        */
    /* note lists of length 2 are not guaranteed to be sorted */
    for(;;)
      {
        /* walk up ... and down ... swap if equal to pivot! */
        do {pi++; pi2++;} while (*pi<*ar);
        do {pj--; pj2--;} while (*pj>*ar);

        /* if we've crossed we're done */
        if (pj<pi) break;

        /* else swap */
        SWAP(*pi,*pj)
        P_SWAP(*pi2,*pj2)
      }

    /* place pivot_value in it's correct location */
    SWAP(*ar,*pj)
    P_SWAP(*ar2,*pj2)

    /* test stack_size to see if we've exhausted our stack */
    if (top_s-bottom_s >= SORT_STACK)
         {error_msg_fatal("ivec_sort_companion_hack() :: STACK EXHAUSTED!!!");}

    /* push right hand child iff length > 1 */
    if (*top_s = size-((int) (pi-ar)))
      {
        *(top_a++) = pi;
        *(top_a++) = (int *) pi2;
        size -= *top_s+2;
        top_s++;
      }
    /* set up for next loop iff there is something to do */
    else if (size -= *top_s+2)
      {;}
    /* might as well pop - note NR_OPT >=2 ==> we're ok! */
    else
      {
        ar2 = (int **) *(--top_a);
        ar  = *(--top_a);
        size = *(--top_s);
      }
  }

      /* else sort small list directly then pop another off stack */
      else
  {
    /* insertion sort for bottom */
          for (pj=ar+1, pj2=ar2+1;pj<=ar+size;pj++,pj2++)
            {
              temp = *pj;
              ptr = *pj2;
              for (pi=pj-1,pi2=pj2-1;pi>=ar;pi--,pi2--)
                {
                  if (*pi <= temp) break;
                  *(pi+1)=*pi;
                  *(pi2+1)=*pi2;
                }
              *(pi+1)=temp;
              *(pi2+1)=ptr;
      }

    /* check to see if stack is exhausted ==> DONE */
    if (top_s==bottom_s) return;

    /* else pop another list from the stack */
    ar2 = (int **)*(--top_a);
    ar  = *(--top_a);
    size = *(--top_s);
  }
    }
}



/******************************************************************************
Function: SMI_sort().
Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
SMI_sort(void *ar1, void *ar2, int size, int type)
{
  if (type == SORT_INTEGER)
    {
      if (ar2)
  {ivec_sort_companion((int *)ar1,(int *)ar2,size);}
      else
  {ivec_sort(ar1,size);}
    }
  else if (type == SORT_INT_PTR)
    {
      if (ar2)
  {ivec_sort_companion_hack((int *)ar1,(int **)ar2,size);}
      else
  {ivec_sort(ar1,size);}
    }

  else
    {
      error_msg_fatal("SMI_sort only does SORT_INTEGER!");
    }
/*
  if (type == SORT_REAL)
    {
      if (ar2)
  {rvec_sort_companion(ar2,ar1,size);}
      else
  {rvec_sort(ar1,size);}
    }
*/
}



/**********************************ivec.c**************************************
Function ivec_linear_search()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_linear_search(register int item, register int *list, register int n)
{
  register int tmp = n-1;

  while (n--)  {if (*list++ == item) {return(tmp-n);}}
  return(-1);
}



/**********************************ivec.c**************************************
Function ivec_binary_search()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
ivec_binary_search(register int item, register int *list, register int rh)
{
  register int mid, lh=0;

  rh--;
  while (lh<=rh)
    {
      mid = (lh+rh)>>1;
      if (*(list+mid) == item)
  {return(mid);}
      if (*(list+mid) > item)
  {rh = mid-1;}
      else
  {lh = mid+1;}
    }
  return(-1);
}



/********************************ivec.c**************************************
Function rvec_copy()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_copy(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1++ = *arg2++;}
}



/********************************ivec.c**************************************
Function rvec_zero()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_zero(register REAL *arg1, register int n)
{
  while (n--)  {*arg1++ = 0.0;}
}



/**********************************ivec.c**************************************
Function rvec_one()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
rvec_one(register REAL *arg1, register int n)
{
  while (n--)  {*arg1++ = 1.0;}
}



/**********************************ivec.c**************************************
Function rvec_neg_one()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
rvec_neg_one(register REAL *arg1, register int n)
{
  while (n--)  {*arg1++ = -1.0;}
}



/**********************************ivec.c**************************************
Function rvec_set()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
rvec_set(register REAL *arg1, register REAL arg2, register int n)
{
  while (n--)  {*arg1++ = arg2;}
}



/**********************************ivec.c**************************************
Function rvec_scale()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
void
rvec_scale(register REAL *arg1, register REAL arg2, register int n)
{
  while (n--)  {*arg1++ *= arg2;}
}



/********************************ivec.c**************************************
Function rvec_add()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_add(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1++ += *arg2++;}
}



/********************************ivec.c**************************************
Function rvec_dot()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
REAL
rvec_dot(register REAL *arg1, register REAL *arg2, register int n)
{
  REAL dot=0.0;

  while (n--)  {dot+= *arg1++ * *arg2++;}

  return(dot);
}



/********************************ivec.c**************************************
Function rvec_axpy()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_axpy(register REAL *arg1, register REAL *arg2, register REAL scale,
    register int n)
{
  while (n--)  {*arg1++ += scale * *arg2++;}
}


/********************************ivec.c**************************************
Function rvec_mult()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_mult(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1++ *= *arg2++;}
}



/********************************ivec.c**************************************
Function rvec_max()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_max(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1 = MAX(*arg1,*arg2); arg1++; arg2++;}
}



/********************************ivec.c**************************************
Function rvec_max_abs()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_max_abs(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1 = MAX_FABS(*arg1,*arg2); arg1++; arg2++;}
}



/********************************ivec.c**************************************
Function rvec_min()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_min(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1 = MIN(*arg1,*arg2); arg1++; arg2++;}
}



/********************************ivec.c**************************************
Function rvec_min_abs()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_min_abs(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1 = MIN_FABS(*arg1,*arg2); arg1++; arg2++;}
}



/********************************ivec.c**************************************
Function rvec_exists()

Input :
Output:
Return:
Description:
*********************************ivec.c*************************************/
void
rvec_exists(register REAL *arg1, register REAL *arg2, register int n)
{
  while (n--)  {*arg1 = EXISTS(*arg1,*arg2); arg1++; arg2++;}
}



/**********************************ivec.c**************************************
Function rvec_fct_addr()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
vfp rvec_fct_addr(register int type)
{
  if (type == GL_MAX)
    {return(&rvec_max);}
  else if (type == GL_MAX_ABS)
    {return(&rvec_max_abs);}
  else if (type == GL_MIN)
    {return(&rvec_min);}
  else if (type == GL_MIN_ABS)
    {return(&rvec_min_abs);}
  else if (type == GL_MULT)
    {return(&rvec_mult);}
  else if (type == GL_ADD)
    {return(&rvec_add);}
  else if (type == GL_EXISTS)
    {return(&rvec_exists);}

  /* catch all ... not good if we get here */
  return(NULL);
}


/******************************************************************************
Function: my_sort().
Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
rvec_sort(register REAL *ar, register int Size)
{
  register REAL *pi, *pj, temp;
  register REAL **top_a = (REAL **)offset_stack;
  register PTRINT *top_s = psize_stack, *bottom_s = psize_stack;
  register PTRINT size = (PTRINT) Size;

  /* we're really interested in the offset of the last element */
  /* ==> length of the list is now size + 1                    */
  size--;

  /* do until we're done ... return when stack is exhausted */
  for (;;)
    {
      /* if list is large enough use quicksort partition exchange code */
      if (size > SORT_OPT)
  {
    /* start up pointer at element 1 and down at size     */
    pi = ar+1;
    pj = ar+size;

    /* find middle element in list and swap w/ element 1 */
    SWAP(*(ar+(size>>1)),*pi)

    pj = ar+size;

    /* order element 0,1,size-1 st {M,L,...,U} w/L<=M<=U */
    /* note ==> pivot_value in index 0                   */
    if (*pi > *pj)
      {SWAP(*pi,*pj)}
    if (*ar > *pj)
      {SWAP(*ar,*pj)}
    else if (*pi > *ar)
      {SWAP(*(ar),*(ar+1))}

    /* partition about pivot_value ...                        */
    /* note lists of length 2 are not guaranteed to be sorted */
    for(;;)
      {
        /* walk up ... and down ... swap if equal to pivot! */
        do pi++; while (*pi<*ar);
        do pj--; while (*pj>*ar);

        /* if we've crossed we're done */
        if (pj<pi) break;

        /* else swap */
        SWAP(*pi,*pj)
      }

    /* place pivot_value in it's correct location */
    SWAP(*ar,*pj)

    /* test stack_size to see if we've exhausted our stack */
    if (top_s-bottom_s >= SORT_STACK)
      {error_msg_fatal("\nSTACK EXHAUSTED!!!\n");}

    /* push right hand child iff length > 1 */
    if (*top_s = size-(pi-ar))
      {
        *(top_a++) = pi;
        size -= *top_s+2;
        top_s++;
      }
    /* set up for next loop iff there is something to do */
    else if (size -= *top_s+2)
      {;}
    /* might as well pop - note NR_OPT >=2 ==> we're ok! */
    else
      {
        ar = *(--top_a);
        size = *(--top_s);
      }
  }

      /* else sort small list directly then pop another off stack */
      else
  {
    /* insertion sort for bottom */
          for (pj=ar+1;pj<=ar+size;pj++)
            {
              temp = *pj;
              for (pi=pj-1;pi>=ar;pi--)
                {
                  if (*pi <= temp) break;
                  *(pi+1)=*pi;
                }
              *(pi+1)=temp;
      }

    /* check to see if stack is exhausted ==> DONE */
    if (top_s==bottom_s) return;

    /* else pop another list from the stack */
    ar = *(--top_a);
    size = *(--top_s);
  }
    }
}



/******************************************************************************
Function: my_sort().
Input : offset of list to be sorted, number of elements to be sorted.
Output: sorted list (in ascending order).
Return: none.
Description: stack based (nonrecursive) quicksort w/brute-shell bottom.
******************************************************************************/
void
rvec_sort_companion(register REAL *ar, register int *ar2, register int Size)
{
  register REAL *pi, *pj, temp;
  register REAL **top_a = (REAL **)offset_stack;
  register PTRINT *top_s = psize_stack, *bottom_s = psize_stack;
  register PTRINT size = (PTRINT) Size;

  register int *pi2, *pj2;
  register int ptr;
  register PTRINT mid;


  /* we're really interested in the offset of the last element */
  /* ==> length of the list is now size + 1                    */
  size--;

  /* do until we're done ... return when stack is exhausted */
  for (;;)
    {
      /* if list is large enough use quicksort partition exchange code */
      if (size > SORT_OPT)
  {
    /* start up pointer at element 1 and down at size     */
    mid = size>>1;
    pi = ar+1;
    pj = ar+mid;
    pi2 = ar2+1;
    pj2 = ar2+mid;

    /* find middle element in list and swap w/ element 1 */
    SWAP(*pi,*pj)
    P_SWAP(*pi2,*pj2)

    /* order element 0,1,size-1 st {M,L,...,U} w/L<=M<=U */
    /* note ==> pivot_value in index 0                   */
    pj = ar+size;
    pj2 = ar2+size;
    if (*pi > *pj)
      {SWAP(*pi,*pj) P_SWAP(*pi2,*pj2)}
    if (*ar > *pj)
      {SWAP(*ar,*pj) P_SWAP(*ar2,*pj2)}
    else if (*pi > *ar)
      {SWAP(*(ar),*(ar+1)) P_SWAP(*(ar2),*(ar2+1))}

    /* partition about pivot_value ...                        */
    /* note lists of length 2 are not guaranteed to be sorted */
    for(;;)
      {
        /* walk up ... and down ... swap if equal to pivot! */
        do {pi++; pi2++;} while (*pi<*ar);
        do {pj--; pj2--;} while (*pj>*ar);

        /* if we've crossed we're done */
        if (pj<pi) break;

        /* else swap */
        SWAP(*pi,*pj)
        P_SWAP(*pi2,*pj2)
      }

    /* place pivot_value in it's correct location */
    SWAP(*ar,*pj)
    P_SWAP(*ar2,*pj2)

    /* test stack_size to see if we've exhausted our stack */
    if (top_s-bottom_s >= SORT_STACK)
      {error_msg_fatal("\nSTACK EXHAUSTED!!!\n");}

    /* push right hand child iff length > 1 */
    if (*top_s = size-(pi-ar))
      {
        *(top_a++) = pi;
        *(top_a++) = (REAL *) pi2;
        size -= *top_s+2;
        top_s++;
      }
    /* set up for next loop iff there is something to do */
    else if (size -= *top_s+2)
      {;}
    /* might as well pop - note NR_OPT >=2 ==> we're ok! */
    else
      {
        ar2 = (int *) *(--top_a);
        ar  = *(--top_a);
        size = *(--top_s);
      }
  }

      /* else sort small list directly then pop another off stack */
      else
  {
    /* insertion sort for bottom */
          for (pj=ar+1, pj2=ar2+1;pj<=ar+size;pj++,pj2++)
            {
              temp = *pj;
              ptr = *pj2;
              for (pi=pj-1,pi2=pj2-1;pi>=ar;pi--,pi2--)
                {
                  if (*pi <= temp) break;
                  *(pi+1)=*pi;
                  *(pi2+1)=*pi2;
                }
              *(pi+1)=temp;
              *(pi2+1)=ptr;
      }

    /* check to see if stack is exhausted ==> DONE */
    if (top_s==bottom_s) return;

    /* else pop another list from the stack */
    ar2 = (int *) *(--top_a);
    ar  = *(--top_a);
    size = *(--top_s);
  }
    }
}





/**********************************ivec.c**************************************
Function ivec_binary_search()

Input :
Output:
Return:
Description:
***********************************ivec.c*************************************/
int
rvec_binary_search(register REAL item, register REAL *list, register int rh)
{
  register int mid, lh=0;

  rh--;
  while (lh<=rh)
    {
      mid = (lh+rh)>>1;
      if (*(list+mid) == item)
  {return(mid);}
      if (*(list+mid) > item)
  {rh = mid-1;}
      else
  {lh = mid+1;}
    }
  return(-1);
}
