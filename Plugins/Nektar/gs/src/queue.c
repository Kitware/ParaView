/**********************************queue.c*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
**********************************queue.c*************************************/


/**********************************queue.c*************************************
File Description:
-----------------
  This file implements the queue abstraction via a linked list ...
***********************************queue.c*************************************/
#include <stdio.h>
#include "queue.h"
#include "error.h"
#include "bss_malloc.h"


/**********************************queue.c*************************************
Type: queue_CDT
---------------
  Basic linked list implememtation w/header node and chain.
**********************************queue.c*************************************/
struct node{
  void  *obj;
  struct node *next;
};


struct queue_CDT{
  int len;
  struct node *head, *tail;
};



/**********************************queue.c*************************************
Function: new_queue()

Input : na
Output: na
Return: pointer to ADT.
Description: This function allocates and returns an empty queue.
Usage: queue = new_queue();
**********************************queue.c*************************************/
queue_ADT new_queue(void)
{
  queue_ADT q;


  q = (queue_ADT) bss_malloc(sizeof(struct queue_CDT));
  q->len = 0;
  q->head = q->tail = NULL;
  return(q);
}



/**********************************queue.c*************************************
Function: free_queue()

Input : pointer to ADT.
Output: na
Return: na
Description: This function frees the storage associated with queue but not any
pointer contained w/in.
Usage: free_queue(queue);
**********************************queue.c*************************************/
void free_queue(queue_ADT q)
{
  struct node *hold, *remove;

#ifdef DEBUG
  if (!q->len)
    {
      if (q->head || q->tail)
  {error_msg_fatal("free_queue :: len=0 but head, tail not NULL?");}
    }

  if (q->len)
    {
      if (!q->head || !q->tail)
  {error_msg_fatal("free_queue :: len!=0 but head, tail are NULL?");}
    }
#endif

#ifdef INFO
  /* could be destroying the only pointer to prev malloced space!!! */
  if (q->len)
    {error_msg_warning("free_queue :: prev malloced space?");}
#endif

  /* should use other queue fcts but what's the point */
  hold = q->head;
  while (remove = hold)
    {
      hold = hold->next;
      bss_free(remove);
    }

  bss_free(q);
}



/**********************************queue.c*************************************
Function: enqueue()

Input : pointer to ADT and pointer to object
Output: na
Return: na
Description: This function adds obj to the end of the queue.
Usage: enqueue(queue, obj);
**********************************queue.c*************************************/
void enqueue(queue_ADT q, void *obj)
{
  if (q->len++)
    {q->tail= q->tail->next = (struct node *) bss_malloc(sizeof(struct node));}
  else
    {q->tail= q->head       = (struct node *) bss_malloc(sizeof(struct node));}

  q->tail->next = NULL;
  q->tail->obj  = obj;
}



/**********************************queue.c*************************************
Function: dequeue()

Input : pointer to ADT
Output: na
Return: void * to element
Description: This function removes the data value at the head of the queue
and returns it to the client.  dequeueing an empty queue is an error
Usage: obj = dequeue(queue);
**********************************queue.c*************************************/
void *dequeue(queue_ADT q)
{
  struct node *hold;
  void *obj;


  if (!q->len--)
    {error_msg_fatal("dequeue :: trying to remove from an empty queue!");}

  if ((hold=q->head) == q->tail)
    {q->head = q->tail = NULL;}
  else
    {q->head = q->head->next;}

#ifdef DEBUG
  if (!hold)
    {error_msg_fatal("dequeue :: len > 0 but head NULL?");}
#endif

  obj = hold->obj;
  bss_free(hold);
  return(obj);
}



/**********************************queue.c*************************************
Function: len_queue()

Input : pointer to ADT
Output: na
Return: integer number of elements
Description: This function returns the number of elements in the queue.
n = len_queue(queue);
**********************************queue.c*************************************/
int len_queue(queue_ADT q)
{
  return(q->len);
}
