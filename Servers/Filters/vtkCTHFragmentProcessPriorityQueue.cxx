/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentProcessPriorityQueue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFragmentProcessPriorityQueue.h"
#include <cassert>
#include "vtksys/ios/iostream"
using vtksys_ios::ostream;
using vtksys_ios::cerr;
using vtksys_ios::endl;
//
vtkCTHFragmentProcessPriorityQueue::vtkCTHFragmentProcessPriorityQueue()
{
  this->NProcs=0;
  this->Heap=0;
  this->HeapSize=0;
  this->EOH=0;
}
//
vtkCTHFragmentProcessPriorityQueue::~vtkCTHFragmentProcessPriorityQueue()
{
  this->Clear();
}
//
vtkCTHFragmentProcessPriorityQueue::vtkCTHFragmentProcessPriorityQueue(
                                                             int nProcs)
{
  this->Initialize(nProcs); 
}
//
void vtkCTHFragmentProcessPriorityQueue::Initialize(int nProcs)
{
  assert("Queue is sized statically and must have at least one proccess."
          && nProcs>0);

  this->Clear();
  this->EOH=nProcs+1;
  this->HeapSize=this->ComputeHeapSize(nProcs+1);
  this->Heap
    = new vtkCTHFragmentProcessLoading[this->HeapSize];
  this->NProcs=nProcs;
  this->ProcLocation.resize(this->NProcs,0);
  this->Heap[0].Initialize(-1,0);
  for (int i=1;i<this->EOH; ++i)
    {
    this->Heap[i].Initialize(i-1,0);
    this->ProcLocation[i-1]=i;
    }
}
//
void vtkCTHFragmentProcessPriorityQueue::Clear()
{
  if (this->HeapSize>0)
    {
    delete [] this->Heap;
    this->Heap=0;
    }
  this->HeapSize=0;
  this->EOH=0;
  this->NProcs=0;
  this->ProcLocation.clear();
}
//
int vtkCTHFragmentProcessPriorityQueue::AssignFragmentToProcess(
                                                  vtkIdType loadFactor)
{
  int asignedToId=this->Heap[1].GetId();
  this->Heap[1].UpdateLoadFactor(loadFactor);
  this->HeapifyTopDown(1);
  return asignedToId;
}
//
void vtkCTHFragmentProcessPriorityQueue::UpdateProcessLoadFactor(
                                                  int procId,
                                                  vtkIdType loadFactor)
{
  int idx=this->ProcLocation[procId];
  this->Heap[idx].UpdateLoadFactor(loadFactor);
  this->HeapifyBottomUp(idx);
}
//
vtkCTHFragmentProcessLoading &vtkCTHFragmentProcessPriorityQueue::Pop()
{
  assert("Queue is empty." && this->EOH>1);

  this->Exchange(this->Heap[1], this->Heap[this->EOH-1]);
  this->Exchange(this->Heap[1].GetId(), this->Heap[this->EOH-1].GetId());
  --this->EOH;
  this->HeapifyTopDown(1);

  return this->Heap[this->EOH];
}
//
void vtkCTHFragmentProcessPriorityQueue::Print(/*ostream &sout*/)
{
  ostream &sout=cerr;

  if (this->EOH<1)
    {
    sout << "The heap is empty." << endl;
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
      if ( i==this->EOH )
        {
        sout << "  EOH";
        return;
        }
      sout << this->Heap[i] << ", ";
      }
     sout << (char)0x08 << (char)0x08 << endl;
     ++p;
    }
}
//
void 
vtkCTHFragmentProcessPriorityQueue::PrintProcessLoading(
  /*ostream &sout*/)
{
  ostream &sout=cerr;

  if (this->EOH<1)
    {
    sout << "The heap is empty." << endl;
    return;
    }
  for (int heapItemIdx=1; heapItemIdx<=this->NProcs; ++heapItemIdx)
    {
    sout << this->Heap[heapItemIdx] << endl;
    }
}
//
vtkIdType
vtkCTHFragmentProcessPriorityQueue::GetTotalLoading()
{
  vtkIdType totalLoading=0;

  if (this->EOH<1)
    {
    return 0;
    }
  for (int heapItemIdx=1; heapItemIdx<=this->NProcs; ++heapItemIdx)
    {
    totalLoading+=this->Heap[heapItemIdx].GetLoadFactor();
    }
  return totalLoading;
}
//
void vtkCTHFragmentProcessPriorityQueue::InitialHeapify()
{
  for (int i=this->EOH/2; i>=1; --i)
    {
    this->HeapifyTopDown(i);
    }
}
//
void vtkCTHFragmentProcessPriorityQueue::HeapifyBottomUp(int nodeId)
{
  int parentNodeId=nodeId/2;
  while( nodeId>1
         && this->Heap[parentNodeId]>this->Heap[nodeId] )
    {
    this->Exchange(this->Heap[parentNodeId],this->Heap[nodeId]);
    this->Exchange(this->Heap[parentNodeId].GetId(), this->Heap[nodeId].GetId());
    nodeId=parentNodeId;
    parentNodeId/=2;
    }
}
//
void vtkCTHFragmentProcessPriorityQueue::HeapifyTopDown(int nodeId)
{
  while( 2*nodeId<this->EOH )
    {
    int childNodeId=2*nodeId;
    // get the smaller of the two children, 
    // without going off the end of the heap
    if (childNodeId+1<this->EOH 
        && this->Heap[childNodeId+1]<this->Heap[childNodeId])
      {
      ++childNodeId;
      }
    // if heap condition satsified then stop
    if (this->Heap[nodeId]<this->Heap[childNodeId])
      {
      break;
      }
    // restore heap condition at this depth
    this->Exchange(this->Heap[nodeId], this->Heap[childNodeId]);
    this->Exchange(this->Heap[nodeId].GetId(), this->Heap[childNodeId].GetId());
    // go down a level
    nodeId=childNodeId;
    }
}
// Returns the nearest power of 2 larger.
unsigned int vtkCTHFragmentProcessPriorityQueue::ComputeHeapSize(unsigned int nItems)
{
  unsigned int bitsPerInt=sizeof(unsigned int)*8;
  unsigned int size=1;
  unsigned int i=0;
  for (; i<bitsPerInt-1; ++i)
    {
    size<<=1;
    if (size>=nItems)
      {
      break;
      }
    }
  assert("Too many items requested."
         && i!=bitsPerInt-1);

  return size;
}
