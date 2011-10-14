/**********************************queue.h*************************************
SPARSE GATHER-SCATTER PACKAGE: bss_malloc bss_malloc ivec error comm gs queue

Author: Henry M. Tufo III

e-mail: hmt@cs.brown.edu

snail-mail:
Division of Applied Mathematics
Brown University
Providence, RI 02912

Last Modification:
6.21.97
**********************************queue.h*************************************/

/**********************************queue.h*************************************
File Description:
-----------------
  This file provides an interface to a simple queue abstraction.
**********************************queue.h*************************************/
#ifndef _queue_h
#define _queue_h


/**********************************queue.h*************************************
Type: queue_ADT
---------------
  This line defines the abstract queue type as a pointer to
  its concrete counterpart.  Clients have no access to the
  underlying representation.
**********************************queue.h*************************************/
typedef struct queue_CDT *queue_ADT;



/**********************************queue.h*************************************
Function: new_queue()

Input : na
Output: na
Return: pointer to ADT.
Description: This function allocates and returns an empty queue.
Usage: queue = new_queue();
**********************************queue.h*************************************/
extern queue_ADT new_queue(void);



/**********************************queue.h*************************************
Function: free_queue()

Input : pointer to ADT.
Output: na
Return: na
Description: This function frees the storage associated with queue but not any
pointer contained w/in.
Usage: free_queue(queue);
**********************************queue.h*************************************/
extern void free_queue(queue_ADT queue);



/**********************************queue.h*************************************
Function: enqueue()

Input : pointer to ADT and pointer to object
Output: na
Return: na
Description: This function adds obj to the end of the queue.
Usage: enqueue(queue, obj);
**********************************queue.h*************************************/
extern void enqueue(queue_ADT queue, void *obj);



/**********************************queue.h*************************************
Function: dequeue()

Input : pointer to ADT
Output: na
Return: void * to element
Description: This function removes the data value at the head of the queue
and returns it to the client.  dequeueing an empty queue is an error
Usage: obj = dequeue(queue);
**********************************queue.h*************************************/
extern void *dequeue(queue_ADT queue);



/**********************************queue.h*************************************
Function: len_queue()

Input : pointer to ADT
Output: na
Return: integer number of elements
Description: This function returns the number of elements in the queue.
Usage: n = len_queue(queue);
**********************************queue.h*************************************/
extern int len_queue(queue_ADT queue);



#endif
