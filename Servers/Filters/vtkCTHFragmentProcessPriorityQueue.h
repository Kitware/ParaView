/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentProcessPriorityQueue.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentProcessPriorityQueue - Minimum ordered heap based priority queue.
// .SECTION Description
//
// Queue size is static and given by the number of processes.
// Ordering is based on the loading factors associated with each 
// process.
//
// Usage:
//   vtkCTHFragmentProcessPriorityQueue Q;
//   Q.Initialize(3);
//   vtkCTHFragmentProcessLoading *H = Q.GetHeap();
//   H[1].Initialize(0,'A');
//   H[2].Initialize(1,'S');
//   H[3].Initialize(2,'O');
//   ...
//   Q.InitialHeapify();
//   ...
//   recip=Q.AssignFragmentToProcess('C');
//   Q.UpdateProcessLoadFactor(donor,-'A');
//   ...
// Caveates:
// Heap indexing is 1 based.

#ifndef __vtkCTHFragmentProcessPriorityQueue_h
#define __vtkCTHFragmentProcessPriorityQueue_h

#include "vtkCTHFragmentProcessLoading.h"
#include "vtkstd/vector"
#include "vtkType.h"

class vtkCTHFragmentProcessPriorityQueue
{
  public:
    // Description:
    // Allocate resources and set to an empty un-intialized state.
    vtkCTHFragmentProcessPriorityQueue();
    // Description:
    // Free resources and set to an un-intialized state.
    ~vtkCTHFragmentProcessPriorityQueue();
    // Description:
    // Allocate resources and set to an empty but intialized state.
    vtkCTHFragmentProcessPriorityQueue(int nProcs);
    // Description:
    // Allocate resources and set to an empty but intialized state.
    void Initialize(int nProcs);
    // Description:
    // Free all resources and return to un-initialized state.
    void Clear();
    // Description:
    // Determine if there are any elements on the heap.
    bool Empty()
    {
      return this->EOH<=1;
    }
    // Description:
    // Get the heap array for direct manipulations.
    vtkCTHFragmentProcessLoading *GetHeap(){ return this->Heap; }
    // Description:
    // Get process location within the heap.
    vtkstd::vector<int> &GetProcLocations(){ return this->ProcLocation; }
    // Description:
    // Assigns a fragment to the next available
    // node, returns the node id. Updates the heap
    // oredering according to the adjusted loadFactor.
    int AssignFragmentToProcess(vtkIdType loadFactor);
    //Description:
    // Update the given processes load factor. Updates
    // the heap order accordingly.
    void UpdateProcessLoadFactor(int procId, vtkIdType loadFactor);
    // Description:
    // Remove the item at the top of the heap.
    vtkCTHFragmentProcessLoading &Pop();
    // Description:
    // Print the heap
    void Print(/*ostream &sout*/);
    void PrintProcessLoading();
    // Description:
    // Sum loading across all processes.
    vtkIdType GetTotalLoading();
    // Description:
    // Enforce heap ordering on the entire heap,
    // If the heap is set rather than built by repetitive
    // insertion this will be needed.
    void InitialHeapify();
    // Description:
    // Strating at the given node, restore the heap
    // ordering from top to bottom.
    void HeapifyTopDown(int nodeId);
    // Description:
    // Strating at the given node, restore the heap
    // ordering from bottom to top.
    void HeapifyBottomUp(int nodeId);
  private:
    // Under the hood the heap is a
    // binary tree and thus we want
    // its size to be a power of 2.
    unsigned int ComputeHeapSize(unsigned int nItems);
    // Exchange two heap items via copy.
    void Exchange(vtkCTHFragmentProcessLoading &a,
                  vtkCTHFragmentProcessLoading &b)
    {
      vtkCTHFragmentProcessLoading t(a);
      a=b;
      b=t;
    }
    // Exchange record of two item's position's
    void Exchange(int aProcId, int bProcId)
    {
      int tLocation=this->ProcLocation[aProcId];
      this->ProcLocation[aProcId]=this->ProcLocation[bProcId];
      this->ProcLocation[bProcId]=tLocation;
    }
    //
    int NProcs;                // Size of data in the heap.
    vtkCTHFragmentProcessLoading *Heap; // minimum ordered heap
    int HeapSize;              // Memory used by heap(power of 2)
    int EOH;                   // end of heap
    vtkstd::vector<int> ProcLocation;  // index of a proc's location in the heap.
};
#endif
