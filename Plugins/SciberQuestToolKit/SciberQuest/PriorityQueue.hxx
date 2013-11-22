/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __PriorityQueue_h
#define __PriorityQueue_h

#include "SQMacros.h"

/// Minimum ordered heap based priority queue.
/**
This is an index heap, indexes are ordered from comparisions
of keys stored internally. Keys are initialized during Push()
and can be later modified with Update. Key's must support
operator< and operator=.
*/
template <typename T>
class PriorityQueue
{
public:
  PriorityQueue();
  ~PriorityQueue();

  // Description:
  // Allocate resources for a queue ordering at most n of m
  // externally managed items. The order is determined keys
  // stored interally which are modfied via the Update call.
  // Keys must support operator<
  void Initialize(unsigned int n, unsigned int m);

  // Description:
  // Free all resources and return to un-initialized state.
  void Clear();

  // Description:
  // Return the number of items referenced by the heap.
  unsigned int Size(){ return this->NIds; }


  // Description:
  // Return non-zero if the the queue is empty/full.
  int Empty(){ return this->NIds<1; }
  int Full(){ return this->NIds>=this->NIdsMax; }

  // Description:
  // Insert the index into the heap.
  void Push(int idx, T &key);

  // Description:
  // Remove and return the index of the item  at the top
  // of the heap.
  int Pop();

  // Description:
  // Enforce heap ordering from the given index.
  void Update(int idx, T &key);

  // Description:
  // Print to a stream.
  void Print(ostream &os);

private:
  // Description:
  // Strating at the given node, restore the heap
  // ordering from top to bottom. Operates on node ids.
  void HeapifyTopDown(unsigned int i);

  // Description:
  // Strating at the given node, restore the heap
  // ordering from bottom to top. Operates on node ids.
  void HeapifyBottomUp(unsigned int i);

  // Exchange two nodes.
  void Exchange(unsigned int i, unsigned int j)
    {
    // update the reverse lookup
    int rid=this->RIds[this->Ids[i]];
    this->RIds[this->Ids[i]]=this->RIds[this->Ids[j]];
    this->RIds[this->Ids[j]]=rid;

    // update ids
    int id=this->Ids[i];
    this->Ids[i]=this->Ids[j];
    this->Ids[j]=id;
    }

private:
  PriorityQueue(PriorityQueue &);  // not implemented
  void operator=(PriorityQueue &); // not implemented

private:
  unsigned int NIds;      // current number of items in the heap.
  unsigned int NIdsMax;   // maximum number of items allowed in the heap at any time.
  int *Ids;               // heap of indexes into user supplied array
  int *RIds;              // reverse indexes hold index to heap locations
  T *Keys;                // user supplied array of objects with < defined.
};


//-----------------------------------------------------------------------------
template <typename T>
PriorityQueue<T>::PriorityQueue()
      :
    NIds(0),
    NIdsMax(0),
    Ids(0),
    RIds(0),
    Keys(0)
{}

//-----------------------------------------------------------------------------
template <typename T>
PriorityQueue<T>::~PriorityQueue()
{
  this->Clear();
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::Initialize(unsigned int n, unsigned int m)
{
  this->NIds=0;
  this->NIdsMax=n;

  this->Ids = new int [n+1];
  this->RIds = new int [m];
  this->Keys = new T [m];
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::Clear()
{
  this->NIds=0;
  this->NIdsMax=0;

  delete [] this->Ids;
  delete [] this->RIds;
  delete [] this->Keys;

  this->Ids=0;
  this->RIds=0;
  this->Keys=0;
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::Push(int id, T &key)
{
  if (this->Full())
    {
    sqErrorMacro(std::cerr,"Queue is full.");
    return;
    }

  ++this->NIds;

  this->Ids[this->NIds]=id;
  this->RIds[id]=this->NIds;
  this->Keys[id]=key;

  this->HeapifyBottomUp(this->NIds);
}

//-----------------------------------------------------------------------------
template <typename T>
int PriorityQueue<T>::Pop()
{
  if (this->Empty())
    {
    sqErrorMacro(std::cerr,"Queue is empty.");
    return 0;
    }

  int id=this->Ids[1];

  this->Exchange(1,this->NIds);
  --this->NIds;

  this->HeapifyTopDown(1);

  return id;
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::Update(int id, T &key)
{
  this->Keys[id]=key;

  unsigned int node=this->RIds[id];

  // note: one of these calls is superfluous,
  // but determining which one is no less costly,
  // and quite a bit more verbose than making both.
  this->HeapifyTopDown(node);
  this->HeapifyBottomUp(node);
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::HeapifyBottomUp(unsigned int node)
{
  unsigned int parent=node/2;
  while( node>1
    && (this->Keys[this->Ids[node]]<this->Keys[this->Ids[parent]]) )
    {
    this->Exchange(parent,node);
    node=parent;
    parent/=2;
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::HeapifyTopDown(unsigned int node)
{
  while( 2*node<this->NIds )
    {
    unsigned int child=2*node;

    // get the smaller of the two children,
    // without going off the end of the heap
    if ( ((child+1)<this->NIds)
      && (this->Keys[this->Ids[child+1]]<this->Keys[this->Ids[child]]) )
      {
      ++child;
      }

    // if heap condition satsified then stop
    if (this->Keys[this->Ids[node]]<this->Keys[this->Ids[child]])
      {
      break;
      }

    // restore heap condition at this depth
    this->Exchange(node, child);

    // check heap condition at child
    node=child;
    }
}

//-----------------------------------------------------------------------------
template <typename T>
void PriorityQueue<T>::Print(ostream &os)
{
  if (this->NIds<1)
    {
    os << "The heap is empty." << std::endl;
    return;
    }
  //
  int p=1;
  while(1)
    {
    int beg=1<<(p-1);
    int end=1<<p;
    for (int i=beg; i<end; ++i)
      {
      if ( i==this->NIds )
        {
        os << "  EOH";
        return;
        }
      os << this->Keys[this->Ids[i]] << ", ";
      }
     os << (char)0x08 << (char)0x08 << std::endl;
     ++p;
    }
}

#endif
