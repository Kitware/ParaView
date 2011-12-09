/******************************************************************************
File: stack.h
-------------
  This file provides an interface to a simple stack
  abstraction.
******************************************************************************/
#ifndef _stack_h
#define _stack_h


/******************************************************************************
Type: stack_ADT
---------------
  This line defines the abstract stack type as a pointer to
  its concrete counterpart.  Clients have no access to the
  underlying representation.
******************************************************************************/
typedef struct stack_CDT *stack_ADT;



/******************************************************************************
Function: new_stack()

Input : na
Output: na
Return: pointer to ADT.
Description: This function allocates and returns an empty stack.
Usage: stack = new_stack();
******************************************************************************/
stack_ADT new_stack(void);



/******************************************************************************
Function: free_stack()

Input : pointer to ADT.
Output: na
Return: na
Description: This function frees the storage associated with stack but not any
pointer contained w/in.
Usage: free_stack(stack);
******************************************************************************/
void free_stack(stack_ADT stack);



/******************************************************************************
Function: push()

Input : pointer to ADT and pointer to object
Output: na
Return: na
Description: This function adds obj to the top of the stack.
Usage: push(stack, obj);
******************************************************************************/
void push(stack_ADT stack, void *obj);



/******************************************************************************
Function: pop()

Input : pointer to ADT
Output: na
Return: void * to element
Description: This function removes the data value at the top of the stack
and returns it to the client.  popping an empty stack is an error.
Usage: obj = pop(queue, obj);
******************************************************************************/
void *pop(stack_ADT stack);



/******************************************************************************
Function: len_stack()

Input : pointer to ADT
Output: na
Return: integer number of elements
Description: This function returns the number of elements in the stack.
n = len_stack(queue);
******************************************************************************/
int len_stack(stack_ADT stack);



#endif
