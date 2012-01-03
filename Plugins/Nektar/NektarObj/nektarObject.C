#include "nektarObject.h"


nektarObject::nektarObject()
{
    this->ugrid = NULL;
    this->pressure = false;
    this->velocity = false;
    this->vorticity = false;
    this->lambda_2 = false;
    this->index = 0;
    this->resolution = 2;
    this->prev = NULL;
    this->next = NULL;
}

nektarObject::~nektarObject()
{
    //fprintf(stdout, "nektarObject::~nektarObject() Called\n");
    if(this->ugrid)
  this->ugrid->Delete();
}
void nektarObject::reset()
{
    this->pressure = false;
    this->velocity = false;
    this->vorticity = false;
    this->lambda_2 = false;
    this->index = 0;
    this->resolution = 2;
    if(this->ugrid)
    {
  this->ugrid->Delete();
  this->ugrid = NULL;
    }
}

nektarList::nektarList()
{
    this->head = NULL;
    this->tail = NULL;
    this->max_count = 10;
    this->cur_count = 0;
}

nektarList::~nektarList()
{
    nektarObject* curObj = this->head;
    while (curObj)
    {
  this->head = this->head->next;
  //curObj->Delete();
  curObj->~nektarObject();
  curObj = this->head;

    }
}
nektarObject*
nektarList::getObject(int id)
{
    nektarObject* curObj = this->head;
    while (curObj)
    {
  if (curObj->index == id)  // if we found it
  {
      // move found obj to tail of the list
      // if already tail, do nothing
      if(curObj == this->tail)
    break;

      // if it's the head, update head to next
      if(curObj == this->head)
      {
    this->head = this->head->next;
      }
      // now move curObj to tail
      curObj->next->prev = curObj->prev;
      if(curObj->prev) // i.e. if current was not the head
      {
    curObj->prev->next = curObj->next;
      }
      this->tail->next = curObj;
      curObj->prev = this->tail;
      curObj->next = NULL;
      this->tail = curObj;
      break;

  }
  else // otherwise, lok at the next one
  {
      curObj = curObj->next;
  }
    }

    // if we didn't find it
    if(curObj == NULL)
    {
  // if we are not over allocated,
  // create a new object, and put it at the tail
  if(this->cur_count < this->max_count)
  {
      this->cur_count++;
      //curObj = nektarObject::New();
      curObj = new nektarObject();
      if (this->head == NULL)  // if list is empty
      {
    this->head = curObj;
    this->tail = curObj;
      }
      else
      {
    this->tail->next = curObj;
    curObj->prev = this->tail;
    curObj->next = NULL;
    this->tail = curObj;
      }
      // set the index to the one requested
      curObj->index = id;
  }
  else  // otherwise reuse oldest obj (head), reset and move to tail
  {
      curObj = this->head;
      this->head = this->head->next;
      this->head->prev = NULL;

      this->tail->next = curObj;
      curObj->prev = this->tail;
      curObj->next = NULL;
      this->tail = curObj;
      curObj->reset();
      curObj->index = id;
  }
    }

    return(curObj);
}
