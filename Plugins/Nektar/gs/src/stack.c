/******************************************************************************
File: stack.c
-------------
  This file implements the stack abstraction via a linked list ...
*****************************************************************************/
#include <stdio.h>
#include "stack.h"
#include "error.h"
#include "bss_malloc.h"


/******************************************************************************
Type: stack_CDT
---------------
  Basic linked list implememtation w/header node and chain
******************************************************************************/
struct node{
  void  *obj;
  struct node *next;
};


struct stack_CDT{
  int len;
  struct node *top;
};



/******************************************************************************
Function: new_stack()

Input : na
Output: na
Return: pointer to ADT.
Description: This function allocates and returns an empty stack.
******************************************************************************/
stack_ADT new_stack(void)
{
  stack_ADT s;


  s = (stack_ADT) bss_malloc(sizeof(struct stack_CDT));
  s->len = 0;
  s->top = NULL;
  return(s);
}



/******************************************************************************
Function: free_stack()

Input : pointer to ADT.
Output: na
Return: na
Description: This function frees the storage associated with stack but not any
pointer contained w/in.
******************************************************************************/
void free_stack(stack_ADT s)
{
  struct node *hold, *remove;

#ifdef DEBUG
  if (!s->len)
    {
      if (s->top)
  {error_msg_fatal("free_stack() :: len=0 but top NULL?");}
    }

  if (s->len)
    {
      if (!s->top)
  {error_msg_fatal("free_stack() :: len!=0 but top is NULL?");}
    }
#endif

#ifdef INFO
  /* could be destroying the only pointer to prev malloced space!!! */
  if (s->len)
    {error_msg_warning("free_stack() :: prev malloced space?");}
#endif

  /* should use while (s->len--) {pop();} but why waste the fct calls */
  hold = s->top;
  while (remove = hold)
    {
      hold = hold->next;
      bss_free(remove);
    }

  bss_free(s);
}



/******************************************************************************
Function: push()

Input : pointer to ADT and pointer to object
Output: na
Return: na
Description: This function adds obj to the top of the stack.
******************************************************************************/
void push(stack_ADT s, void *obj)
{
  struct node *new_node;


  new_node = (struct node *) bss_malloc(sizeof(struct node));
  new_node->next = s->top;
  new_node->obj  = obj;

  s->top = new_node;
  s->len++;
}



/******************************************************************************
Function:pop()

Input : pointer to ADT
Output: na
Return: void * to element
Description: This function removes the data value at the top of the stack
and returns it to the client.  popping an empty stack is an error.
******************************************************************************/
void *pop(stack_ADT s)
{
  struct node *hold;
  void *obj;


  if (!s->len--)
    {error_msg_fatal("pop :: trying to remove from an empty stack!");}

  hold = s->top;
  s->top = s->top->next;

#ifdef DEBUG
  if (!hold)
    {error_msg_fatal("pop :: len > 0 but head NULL?");}
#endif

  obj = hold->obj;
  bss_free(hold);
  return(obj);
}



/******************************************************************************
Function: len_stack()

Input : pointer to ADT
Output: na
Return: integer number of elements
Description: This function returns the number of elements in the stack.
******************************************************************************/
int len_stack(stack_ADT s)
{
  return(s->len);
}
