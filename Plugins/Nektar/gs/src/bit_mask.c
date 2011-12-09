/********************************bit_mask.c************************************
SPARSE GATHER-SCATTER PACKAGE: bit_mask bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
11.21.97
*********************************bit_mask.c***********************************/

/********************************bit_mask.c************************************
File Description:
-----------------

*********************************bit_mask.c***********************************/
#include "const.h"
#include "bit_mask.h"
#include "error.h"



/********************************bit_mask.c************************************
Function: bm_to_proc

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
void
bm_to_proc(register char *ptr, int p_mask, register int *msg_list)
{
  register int i, tmp;

  if (msg_list)
    {
      /* low to high */
      ptr+=(p_mask-1);
      for (i=p_mask-1;i>=0;i--)
  {
    tmp = BYTE*(p_mask-i-1);
    if (*ptr&BIT_0)
      {*msg_list = tmp; msg_list++;}
    if (*ptr&BIT_1)
      {*msg_list = tmp+1; msg_list++;}
    if (*ptr&BIT_2)
      {*msg_list = tmp+2; msg_list++;}
    if (*ptr&BIT_3)
      {*msg_list = tmp+3; msg_list++;}
    if (*ptr&BIT_4)
      {*msg_list = tmp+4; msg_list++;}
    if (*ptr&BIT_5)
      {*msg_list = tmp+5; msg_list++;}
    if (*ptr&BIT_6)
      {*msg_list = tmp+6; msg_list++;}
    if (*ptr&BIT_7)
      {*msg_list = tmp+7; msg_list++;}
    ptr --;
  }

      /* high to low */
      /*
      for (i=0;i<p_mask;i++)
  {
    tmp = BYTE*(p_mask-i-1);
    if (*ptr&128)
      {*msg_list = tmp+7; msg_list++;}
    if (*ptr&64)
      {*msg_list = tmp+6; msg_list++;}
    if (*ptr&32)
      {*msg_list = tmp+5; msg_list++;}
    if (*ptr&16)
      {*msg_list = tmp+4; msg_list++;}
    if (*ptr&8)
      {*msg_list = tmp+3; msg_list++;}
    if (*ptr&4)
      {*msg_list = tmp+2; msg_list++;}
    if (*ptr&2)
      {*msg_list = tmp+1; msg_list++;}
    if (*ptr&1)
      {*msg_list = tmp; msg_list++;}
    ptr ++;
  }
      */

    }
}



/********************************bit_mask.c************************************
Function: ct_bits()

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
int
ct_bits(register char *ptr, int n)
{
  register int i, tmp=0;


  for(i=0;i<n;i++)
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



/********************************bit_mask.c************************************
Function: len_buf()

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
int
div_ceil(register int numer, register int denom)
{
  register int rt_val;

  if ((numer<0)||(denom<=0))
    {error_msg_fatal("div_ceil() :: numer=%d ! >=0, denom=%d ! >0",numer,denom);}

  /* if integer division remainder then increment */
  rt_val = numer/denom;
  if (numer%denom)
    {rt_val++;}

  return(rt_val);
}



/********************************bit_mask.c************************************
Function: len_bit_mask()

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
int
len_bit_mask(register int num_items)
{
  register int rt_val, tmp;

  if (num_items<0)
    {error_msg_fatal("Value Sent To len_bit_mask() Must be >= 0!");}

  /* mod BYTE ceiling function */
  rt_val = num_items/BYTE;
  if (num_items%BYTE)
    {rt_val++;}

  /* make mults of sizeof int */
  if (tmp=rt_val%INT_LEN)
    {rt_val+=(INT_LEN-tmp);}

  return(rt_val);
}



/********************************bit_mask.c************************************
Function: set_bit_mask()

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
void
set_bit_mask(register int *bm, int len, int val)
{
  register int i, offset;
  register char mask = 1;
  char *cptr;


  if (len_bit_mask(val)>len)
    {error_msg_fatal("The Bit Mask Isn't That Large!");}

  cptr = (char *) bm;

  offset = len/INT_LEN;
  for (i=0;i<offset;i++)
    {*bm=0; bm++;}

  offset = val%BYTE;
  for (i=0;i<offset;i++)
    {mask <<= 1;}

  offset = len - val/BYTE - 1;
  cptr[offset] = mask;
}



/********************************bit_mask.c************************************
Function: len_buf()

Input :
Output:
Return:
Description:
*********************************bit_mask.c***********************************/
int
len_buf(int item_size, int num_items)
{
  register int rt_val, tmp;

  rt_val = item_size * num_items;

  /*  double precision align for now ... consider page later */
  if (tmp = (rt_val%(int)sizeof(double)))
    {rt_val += (sizeof(double) - tmp);}

  return(rt_val);
}
