/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentConnect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFragmentConnect.h"

#include "vtkMultiProcessController.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
// Pipeline
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
// PV interface
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
// Data sets
#include "vtkDataSet.h"
#include "vtkPolyData.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkAMRBox.h"
// Arrays & Containers
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkCharArray.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCollection.h"
#include "vtkPointAccumulator.hxx"
// IO
#include "vtkDataSetWriter.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkCTHFragmentCommBuffer.h"
// Filters
#include "vtkOBBTree.h"
#include "vtkAppendPolyData.h"
#include "vtkMarchingCubesCases.h"
// STL
#include "vtksys/ios/fstream"
using vtksys_ios::ofstream;
#include "vtksys/ios/sstream"
using vtksys_ios::ostringstream;
#include "vtkstd/vector"
using vtkstd::vector;
#include "vtkstd/string"
using vtkstd::string;
#include "vtkstd/algorithm"
// ansi c
#include <math.h>
#include <ctime>
// other 
#include "vtkCTHFragmentUtils.hxx"

vtkCxxRevisionMacro(vtkCTHFragmentConnect, "1.72");
vtkStandardNewMacro(vtkCTHFragmentConnect);

// NOTE:
//
// 1: We have three tasks:  Connect and index face connected cells.
//    Create faces of connected cells.
//    Share points but not all points.  Split points to make topology manifold.
// 2: Connectivity is straight forward depth or breadth first search.
// 3: We can have a special id for not visited and a special id for empty.
// 4: Depth first is easier but we must protect for stack overflow.
// 5: For the surface, we are simply going to take the surface of the voxels
//    and place the verticies based on volume fraction values.
// 6: We could walk the surface in a second pass, but that would be complicated.
// 7: It would be easier to simply create faces as the connectivity search
//    terminates.
// 8: Merging points is tricky.  We want to share points on the surface, 
//    but split points to keep a manifold surface topology.
// 9: We can easily determine which points are new or have been created already
//    when we add a new face.  We simply have to design a way to lookup previously 
//    created points.
// 10: How about preferentially searching voxels with surfaces to minimize 
//    the number of surface points that we need to hash.  This would be complicated
//    and could not eliminate the need to hash points.
// 11: The simple solution is to just use a point locator.
// 12: We could pass in points we know (suspect) will be used by the next cell.
// 13: Why not just have an array of ids as a locator.  We can split as we need to.
//    We could not, however, hash multiple points for a single corner.
// 14: All point separated for first implementation.
// 15: Things I have to ask Jerry:
//      Where should the mass properties be placed?  
//          Separate API in class
//          Field data.
//          Cell or point data.
//      Should I handle multiple volume fractions or just one?  Equivalent (layers)....
// 16: Move corner point along gradient of trilinear interpolation of volume fraction.
// 17: Trying to solve for exact kx, ky, and kz.
// 18: Gradient (-V000-V001-V010-V011+V100+V101+V110+V111,
//               -V000-V001+V010+V011-V100-V101+V110+V111,
//               -V000+V001-V010+V011-V100+V101-V110+V111)
//     Normalize the gradiant vector into (nx,ny,nz)
// 19: Can we solve for k: V((0.5,0.5,0.5)+k(n)) = 0.5; (Cubic equation).
//     It would be a pain.  Lets just assume linear and scale based on magnitude of gradient
//     at the center.  We can at least threshold to keep point inside the cube.
// 20: I can imagine the gradient being 0 at a saddle point. This would cause a sigularity.
//     We could pick a direction, compute the center and a point in the surface (or interior)
//     and
// 21: We also have to deal with points on the boundary.  Lets just duplicate the values
//     to degenerate into bilinear or simply linear   
// 22: First implementation for image data works well!         
// 23: Move corner points in the direction of the gradient (after threshold!) gives
//     the best mesh.
// 24: Todo in the next phase:
//       Share points
//       compute normals
//       compute mass properties
//       save fragment id as cell data
//       convert to AMR input
//       parallelize algorithm
// 25: AMR: How are the scalars stored?  I assume one vtkScalars per block.
//          How do I find neighboring blocks? Loop though and look at extents.
//            Build up a neighbor object that store all extents ...
//          I need a test data set.
//          How do I strip off the ghost cells that I will not need? 

// We have connectivity working with iterators across blocks of different levels.
// We handle level transitions for connectivity and creating faces.
// We place corners using neighbors from different blocks and levels.
//   We look at point neighbors rather than face neighbors.

// 26:  Lets create ghost blocks that have only one layer of cells that adjoin blocks
//      in the process.  There may be a case where two ghost blocks are used to 
//      represent a single block.  They will share/duplicate a row of voxels.
//      We can do connectivity through the ghost cells (might as well) but
//      the do not contribute to the integrated values.  We will not create faces
//      from a ghost layer, but we will use them to place points.
//      We will use the fragment ids in the ghost layers to create an equivalency realtion
//      and to combine fragments in a post processing step. 




// We need to strip off ghost cells that we do not need.
// We need to handle rectilinear grids.
// We need to protect against bad extrapolation spikes.
// There are still cracks because faces are created with at most 4 points.
// We need to add extra points to faces that adjoin higher level faces.
// We still need to protect against running out of stack space.
// We still need to handle multiple processes.



// Distributed: Easy.
// Create a ghost layer around each process partition.
// Each process executes connectivity algorithm.
// Connectivity through ghost cells will reduce the number of fragments.
// Create an equivalence set of fragments.
// Compute new fragments and properties.

// Coupled simulations
// Runtime: Tienchu: NDGM HDF5 XDMF
// Compressing data - for faster visualization.
//

//============================================================================

/**
Description:
Data structure that describes a fragments loading.
Holds its id and its loading factor.
*/
class vtkCTHFragmentPieceLoading
{
  public:
    enum {ID=0,LOADING=1,SIZE=2};
    vtkCTHFragmentPieceLoading(){ this->Initialize(-1,0); }
    ~vtkCTHFragmentPieceLoading(){ this->Initialize(-1,0); }
    void Initialize(int id, vtkIdType loading)
    {
      this->Data[ID]=id;
      this->Data[LOADING]=loading;
    }
    // Description:
    // Place into a buffer (id, loading)
    void Pack(vtkIdType *buf)
    {
      buf[ID]=this->Data[ID];
      buf[LOADING]=this->Data[LOADING];
    }
    // Description:
    // Initialize from a buffer (id, loading)
    void UnPack(vtkIdType *buf)
    {
      this->Data[ID]=buf[ID];
      this->Data[LOADING]=buf[LOADING];
    }
    // Description:
    // Set/Get
    int GetId() const{ return this->Data[ID]; }
    vtkIdType GetLoading() const{ return this->Data[LOADING]; }
    void SetLoading(vtkIdType loading){ this->Data[LOADING]=loading; }
    // Description:
    // Adds to laoding and returns the updated loading.
    vtkIdType UpdateLoading(vtkIdType update)
    {
      assert("Update would make loading negative."
             && (this->Data[LOADING]+update)>=0);
      return this->Data[LOADING]+=update;
    }
    // Description:
    // Comparision are made by id.
    bool operator<(const vtkCTHFragmentPieceLoading &other) const
    {
      return this->Data[ID]<other.Data[ID];
    }
    bool operator==(const vtkCTHFragmentPieceLoading &other) const
    {
      return this->Data[ID]==other.Data[ID];
    }
  private:
    vtkIdType Data[SIZE];
};
//
ostream &operator<<(ostream &sout, vtkCTHFragmentPieceLoading &fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoading() << ")";

  return sout;
}
//
ostream &operator<<(ostream &sout, 
  vector<vector<vtkCTHFragmentPieceLoading> > &pla)
{
  int nProcs=pla.size();
  for (int procId=0; procId<nProcs; ++procId)
    {
    cerr << "Fragment loading on process " << procId << ":" << endl;
    int nLocalFragments=pla[procId].size();
    for (int fragmentIdx=0; fragmentIdx<nLocalFragments; ++fragmentIdx)
      {
      sout << pla[procId][fragmentIdx] << ", ";
      }
    sout << endl;
    }
  return sout;
}
//
void PrintPieceLoadingHistogram(
   vector<vector<vtkIdType> > &pla)
{
//   cerr << "loading array:" <<endl;
//   cerr << pla << endl;

  int nProcs=pla.size();
  // get min and max loading
  vtkIdType minLoading=(vtkIdType)1<<((sizeof(vtkIdType)*8)-2);
  vtkIdType maxLoading=0;
  for (int procId=0; procId<nProcs; ++procId)
    {
    int nPieces=pla[procId].size();
    for (int pieceId=0; pieceId<nPieces; ++pieceId)
      {
      vtkIdType loading=pla[procId][pieceId];
      if (loading>0 
          && minLoading>loading)
        {
        minLoading=loading;
        }
      if (maxLoading<loading)
        {
        maxLoading=loading;
        }
      }
    }
  // generate histogram
  const vtkIdType nBins=40;
  const vtkIdType binWidth=(maxLoading-minLoading)/nBins;
  const vtkIdType r=(maxLoading-minLoading)%nBins;
  vector<int> hist(nBins,0);
  for (int procId=0; procId<nProcs; ++procId)
    {
    int nPieces=pla[procId].size();
    for (int pieceId=0; pieceId<nPieces; ++pieceId)
      {
      vtkIdType loading=pla[procId][pieceId];
      if (loading==0) 
        {
        continue;
        }
      for (int binId=0; binId<nBins; ++binId)
        {
        vtkIdType binTop=minLoading+(binId+1)*binWidth+binId*r;
        if (loading<=binTop)
          {
          ++hist[binId];
          break;
          }
        }
      }
    }
  // generate bin ids
  vector<vtkIdType> binIds(nBins);
  for (int binId=0; binId<nBins; ++binId)
    {
    binIds[binId]=static_cast<int>(minLoading+(binId+1)*binWidth);
    }
  // print
  cerr << "minLoading: " << minLoading << endl;
  cerr << "maxLoading: " << maxLoading << endl;
  cerr << "binWidth:   " << binWidth << endl;
  cerr << "nBins:      " << nBins << endl;
  PrintHistogram(hist,binIds);
}

/**
Type to represent a node in a multiprocess job
and its current loading state.
*/
class vtkCTHFragmentProcessLoading
{
  public:
    enum {ID=0, LOADING=1, SIZE=2};
    vtkCTHFragmentProcessLoading(){ this->Initialize(-1,0); }
    //
    ~vtkCTHFragmentProcessLoading(){ this->Initialize(-1,0); }
    //
    void Initialize(int id, vtkIdType loadFactor)
    {
      this->Data[ID]=id;
      this->Data[LOADING]=loadFactor;
    }
    //
    bool operator<(const vtkCTHFragmentProcessLoading &rhs) const
    {
      return this->Data[LOADING]<rhs.Data[LOADING];
    }
    //
    bool operator>(const vtkCTHFragmentProcessLoading &rhs) const
    {
      return this->Data[LOADING]>rhs.Data[LOADING];
    }
    //
    int GetId() const{ return this->Data[ID]; }
    //
    vtkIdType GetLoadFactor() const{ return this->Data[LOADING]; }
    //
    vtkIdType UpdateLoadFactor(vtkIdType loadFactor)
    {
      assert("Update would make loading negative."
             && (this->Data[LOADING]+loadFactor)>=0);
      return this->Data[LOADING]+=loadFactor;
    }
  private:
    vtkIdType Data[SIZE];
};
//
ostream &operator<<(ostream &sout, vtkCTHFragmentProcessLoading &fp)
{
  sout << "(" << fp.GetId() << "," << fp.GetLoadFactor() << ")";

  return sout;
}

/**
Minimum ordered heap based priority queue.

Particular to this application:
1) Queue size is static and given by the number of processes.

Usage:
  vtkCTHFragmentProcessPriorityQueue Q;
  Q.Initialize(3);
  vtkCTHFragmentProcessLoading *H = Q.GetHeap();
  H[1].Initialize(0,'A');
  H[2].Initialize(1,'S');
  H[3].Initialize(2,'O');
  ...
  Q.InitialHeapify();
  ...
  recip=Q.AssignFragmentToProcess('C');
  Q.UpdateProcessLoadFactor(donor,-'A');
  ...
Caveates:
Heap indexing is 1 based.
*/
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
    vector<int> &GetProcLocations(){ return this->ProcLocation; }
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
    vector<int> ProcLocation;  // index of a proc's location in the heap.
};

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

/**
Data structure used to distribute work amongst available
processes. The buffer is to intialized from a process
priority queue such that only those processes with loading
less than a specified tolerance are included.
*/
class vtkCTHFragmentProcessRingBuffer
{
  public:
    // Description:
    vtkCTHFragmentProcessRingBuffer()
    {
      this->Clear();
    }
    // Description:
    ~vtkCTHFragmentProcessRingBuffer()
    {
      this->Clear();
    }
    // Description:
    void Clear()
    {
      this->NextElement=0;
      this->BufferSize=0;
      this->Buffer.clear();
    }
    // Description:
    // Size buffer and point to first element.
    void Initialize(int nProcs)
    {
      this->NextElement=0;
      this->BufferSize=nProcs;
      this->Buffer.resize(nProcs);
    }
    // Description:
    // Build from a process loading priority queue.
    void Initialize(
        vtkCTHFragmentProcessPriorityQueue &Q,
        vtkIdType upperLoadingBound)
    {
      this->NextElement=0;
      this->BufferSize=0;

      // check that upper bound does not exclude
      // all processes.
      vtkCTHFragmentProcessLoading &pl=Q.Pop();
      vtkIdType minimumLoading=pl.GetLoadFactor();

      // Make sure at least one process can be used.
      if (upperLoadingBound!=-1
          && minimumLoading>upperLoadingBound)
        {
        upperLoadingBound=minimumLoading;
        cerr << "vtkCTHFragmentProcessRingBuffer "
             << "[" << __LINE__ << "] "
             << "Error: Upper loading bound excludes all processes."
             << endl;
        }

      // Build ring of process ids.
      this->Buffer.push_back(pl.GetId());
      ++this->BufferSize;
      while (!Q.Empty())
        {
        pl=Q.Pop();
        if (upperLoadingBound!=-1
            && pl.GetLoadFactor()>upperLoadingBound)
          {
          break;
          }
        this->Buffer.push_back(pl.GetId());
        ++this->BufferSize;
        }
    }
    // Description:
    // Get the next process id from the ring.
    int GetNextId()
    {
      int id=this->Buffer[this->NextElement];
      ++this->NextElement;
      if (this->NextElement==this->BufferSize)
        {
        this->NextElement=0;
        }
      return id;
    }
    // Description:
    void Print()
    {
      int n=this->Buffer.size();
      if (n==0)
        {
        cerr << "{}";
        return;
        }
      cerr << "{" << this->Buffer[0];
      for (int i=1; i<n; ++i)
        {
        cerr << ", " << this->Buffer[i];
        }
      cerr << "}";
    }

  private:
    int NextElement;
    int BufferSize;
    vector<int> Buffer;
};

/**
Data structure which allows constant time determination 
of weather a given proc has a piece of some fragment,
and the number of procs which a specified fragment is 
spread across.
*/
class vtkCTHFragmentToProcMap
{
  public:
    vtkCTHFragmentToProcMap(){ this->Clear(); }
    vtkCTHFragmentToProcMap(int nFragments);
    vtkCTHFragmentToProcMap(int nProcs, int nFragments);
    vtkCTHFragmentToProcMap(const vtkCTHFragmentToProcMap &other);
    vtkCTHFragmentToProcMap &operator=(const vtkCTHFragmentToProcMap &rhs);
    // logistics
    void Clear();
    void Initialize(int nFragments);
    void Initialize(int nProcs, int nFragments);
    void DeepCopy(const vtkCTHFragmentToProcMap &from);
    // interface
    int GetProcOwnsPiece(int fragmentId) const;
    int GetProcOwnsPiece(int procId, int fragmentId) const;
    void SetProcOwnsPiece(int procId, int fragmentId);
    void SetProcOwnsPiece(int fragmentId);
    void UnSetProcOwnsPiece(int procId, int fragmentId);
    void UnSetProcOwnsPiece(int fragmentId);
    vector<int> WhoHasAPiece(int fragmentId, int excludeProc) const;
    vector<int> WhoHasAPiece(int fragmentId) const;
    int GetProcCount(int fragmentId){ return this->ProcCount[fragmentId]; }
  private:
    // proc -> fragment -> bit mask, bit is 1 if frag is on proc
    vector<vector<int> > PieceToProcMap;
    // fragment id -> count num procs
    vector<int> ProcCount;

    int NProcs;     // number of procs
    int NFragments; // number of fragments to map
    int PieceToProcMapSize; // length of map array
    int BitsPerInt; // number of bits in an integer
};
//
vtkCTHFragmentToProcMap::vtkCTHFragmentToProcMap(
              int nFragments)
{
  this->Initialize(nFragments);
}
//
vtkCTHFragmentToProcMap::vtkCTHFragmentToProcMap(
              int nProcs,
              int nFragments)
{
  this->Initialize(nProcs, nFragments);
}
//
vtkCTHFragmentToProcMap::vtkCTHFragmentToProcMap(
              const vtkCTHFragmentToProcMap &other)
{
  this->DeepCopy(other);
}
//
void vtkCTHFragmentToProcMap::Clear()
{
  this->PieceToProcMap.clear();
  this->ProcCount.clear();
  this->NProcs=0;
  this->NFragments=0;
  this->PieceToProcMapSize=0;
  this->BitsPerInt=0;
}
//
void vtkCTHFragmentToProcMap::Initialize(
              int nFragments)
{
  this->Initialize(1,nFragments);
}
//
void vtkCTHFragmentToProcMap::Initialize(
              int nProcs,
              int nFragments)
{
  this->Clear();

  this->NProcs=nProcs;
  this->NFragments=nFragments;
  this->BitsPerInt=8*sizeof(int);
  this->PieceToProcMapSize=nFragments/this->BitsPerInt+1;

  this->ProcCount.resize(nFragments,0);

  this->PieceToProcMap.resize(nProcs);
  for (int i=0; i<nProcs; ++i)
    {
    this->PieceToProcMap[i].resize(this->PieceToProcMapSize,0);
    }
}
//
vtkCTHFragmentToProcMap &vtkCTHFragmentToProcMap::operator=(
              const vtkCTHFragmentToProcMap &other)
{
  this->DeepCopy(other);

  return *this;
}
//
void vtkCTHFragmentToProcMap::DeepCopy(
              const vtkCTHFragmentToProcMap &from)
{
  this->NProcs=from.NProcs;
  this->NFragments=from.NFragments;
  this->PieceToProcMapSize=from.PieceToProcMapSize;
  this->BitsPerInt=from.BitsPerInt;
  this->PieceToProcMap=from.PieceToProcMap;
}
//
int vtkCTHFragmentToProcMap::GetProcOwnsPiece(
                int fragmentId) const
{
  return this->GetProcOwnsPiece(0,fragmentId);
}
//
int vtkCTHFragmentToProcMap::GetProcOwnsPiece(
                int procId,
                int fragmentId) const
{
  assert( "Invalid fragment id"
          && fragmentId >= 0
          && fragmentId < this->NFragments );
  assert( "Invalid proc id"
          && procId >= 0
          && procId < this->NProcs );

  int maskIdx=fragmentId/this->BitsPerInt;
  int maskBit=1<<fragmentId%this->BitsPerInt;

  return maskBit & this->PieceToProcMap[procId][maskIdx];
}
//
void vtkCTHFragmentToProcMap::SetProcOwnsPiece(int fragmentId)
{
  this->SetProcOwnsPiece(0,fragmentId);
}
//
void vtkCTHFragmentToProcMap::SetProcOwnsPiece(int procId, int fragmentId)
{
  assert( "Invalid fragment id"
          && fragmentId >= 0
          && fragmentId < this->NFragments );
  assert( "Invalid proc id"
          && procId >= 0
          && procId < this->NProcs );

  // set bit in this proc's mask array
  int maskIdx=fragmentId/this->BitsPerInt;
  int maskBit=1<<fragmentId%this->BitsPerInt;
  this->PieceToProcMap[procId][maskIdx] |= maskBit;

  // inc fragments ownership count
  ++this->ProcCount[fragmentId];
}
//
vector<int> vtkCTHFragmentToProcMap::WhoHasAPiece(
                int fragmentId,
                int excludeProc) const
{
  assert( "Invalid proc id"
          && excludeProc >= 0
          && excludeProc < this->NProcs );

  vector<int> whoHasList;

  for (int procId=0; procId<this->NProcs; ++procId)
    {
    if ( procId==excludeProc )
      {
      continue;
      }
    int maskIdx=fragmentId/this->BitsPerInt;
    int maskBit=1<<fragmentId%this->BitsPerInt;

    // this guy has a piece
    if (maskBit & this->PieceToProcMap[procId][maskIdx])
      {
      whoHasList.push_back(procId);
      }
    }
  return whoHasList;
}
//
vector<int> vtkCTHFragmentToProcMap::WhoHasAPiece(
                int fragmentId) const
{
  vector<int> whoHasList;

  for (int procId=0; procId<this->NProcs; ++procId)
    {
     int maskIdx=fragmentId/this->BitsPerInt;
     int maskBit=1<<fragmentId%this->BitsPerInt;

    // this guy has a piece
    if (maskBit & this->PieceToProcMap[procId][maskIdx])
      {
      whoHasList.push_back(procId);
      }
    }
  return whoHasList;
}

/**
Description:
Helper to the transaction matrix.

Data structure that describes a single transaction
that needs to be executed in the process of 
moving a fragment piece around.

The fragment to be transacted and the executing process are 
determined implicitly by where the transaction is stored.
*/
class vtkCTHFragmentPieceTransaction
{
  public:
    enum {TYPE=0,REMOTE_PROC=1,SIZE=2};
    //
    vtkCTHFragmentPieceTransaction(){ Clear(); }
    ~vtkCTHFragmentPieceTransaction(){ Clear(); }
    //
    vtkCTHFragmentPieceTransaction(
                    char type,
                    int remoteProc)
    {
      this->Initialize(type,remoteProc);
    }
    // 
    void Initialize(char type,
                    int remoteProc)
    {
      this->Data[TYPE]=(int)type;
      this->Data[REMOTE_PROC]=remoteProc;
    }
    //
    bool Empty() const{ return this->Data[TYPE]==0; }
    //
    void Clear()
    {
      this->Data[TYPE]=0;
      this->Data[REMOTE_PROC]=-1;
    }
    //
    void Pack(int *buf)
    {
      buf[0]=this->Data[TYPE];
      buf[1]=this->Data[REMOTE_PROC];
    }
    //
    void UnPack(int *buf)
    {
      this->Data[TYPE]=buf[0];
      this->Data[REMOTE_PROC]=buf[1];
    }
    //
    char GetType() const{ return (char)this->Data[TYPE]; }
    //
    int GetRemoteProc() const{ return this->Data[REMOTE_PROC]; }
    //
    int GetFlatSize() const{ return SIZE; }
  private:
    int Data[SIZE];
};

ostream &operator<<(ostream &sout, const vtkCTHFragmentPieceTransaction &ta)
{
  sout << "(" << ta.GetType() << "," << ta.GetRemoteProc() << ")";

  return sout;
}

/**
Description:
Container to hold  a sets of transactions (sends/recvs)
indexed by fragment and proc, inteneded to facilitiate
moving fragment peices around.

Internaly we have a 2D matrix. On one axis is fragment id
on the other is proc id.

Transaction are intended to execute in fragment order
so that no deadlocks occur.
*/
class vtkCTHFragmentPieceTransactionMatrix
{
  public:
    // Description:
    // Set the object to an un-initialized state.
    vtkCTHFragmentPieceTransactionMatrix();
    // Description:
    // Allocate internal resources and set the object
    // to an initialized state.
    vtkCTHFragmentPieceTransactionMatrix(int nFragments, int nProcs);
    // Description:
    // Free allocated resources and leave the object in an
    // un-initialized state.
    ~vtkCTHFragmentPieceTransactionMatrix();
    void Initialize(int nProcs, int nFragments);
    // Description:
    // Free allocated resources and leave the object in an
    // un-initialized state.
    void Clear();
    // Description:
    // Get the number of transaction a given process will
    // execute.
    vtkIdType GetNumberOfTransactions(int procId);
    // Description:
    // Given a proc and a fragment, return a ref to
    // the associated list of tranactions.
    vector<vtkCTHFragmentPieceTransaction> &GetTransactions(
                    int fragmentId,
                    int procId);
    // Description:
    // Add a transaction to the end of the given a proc,fragment pair's
    // transaction list.
    void PushTransaction(
                    int fragmentId,
                    int procId,
                    vtkCTHFragmentPieceTransaction &transaction);
    // Description:
    // Send the transaction matrix on srcProc to all
    // other procs.
    void Broadcast(vtkCommunicator *comm, int srcProc);
    //
    void Print();
    // Description:
    // Tells how much memory the matrix has allocated.
    vtkIdType Capacity()
    {
      return this->FlatMatrixSize
        + this->NumberOfTransactions*sizeof(vtkCTHFragmentPieceTransaction);
    }
  private:
    // Put the matrix into a buffer for communication.
    // returns size of the buffer in ints. The buffer
    // will be allocated, and is expected to be null
    // on entry.
    vtkIdType Pack(int *&buffer);
    vtkIdType PackRow(int *&buffer);
    // Load state from a buffer containing a Pack'ed
    // transaction matrix. 0 is returned on error.
    int UnPack(int *buffer);
    int UnPackRow(int *buffer);
    int NProcs;
    int NFragments;
    vector<vtkCTHFragmentPieceTransaction> *Matrix;
    vtkIdType FlatMatrixSize;
    vtkIdType NumberOfTransactions;
};
//
vtkCTHFragmentPieceTransactionMatrix::vtkCTHFragmentPieceTransactionMatrix()
{
  this->NFragments=0;
  this->NProcs=0;
  this->FlatMatrixSize=0;
  this->Matrix=0;
  this->NumberOfTransactions=0;
}
//
vtkCTHFragmentPieceTransactionMatrix::vtkCTHFragmentPieceTransactionMatrix(
                int nFragments,
                int nProcs)
{
  this->Initialize(nFragments, nProcs);
}
//
vtkCTHFragmentPieceTransactionMatrix::~vtkCTHFragmentPieceTransactionMatrix()
{
  this->Clear();
}
//
void vtkCTHFragmentPieceTransactionMatrix::Clear()
{
  this->NFragments=0;
  this->NProcs=0;
  if ( this->Matrix )
    {
    delete [] this->Matrix;
    this->Matrix=0;
    }
  this->NumberOfTransactions=0;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Initialize(
                int nFragments,
                int nProcs)
{
  this->Clear();

  this->NFragments=nFragments;
  this->NProcs=nProcs;
  this->FlatMatrixSize=nFragments*nProcs;
  this->Matrix 
    = new vector<vtkCTHFragmentPieceTransaction>[this->FlatMatrixSize];
}
//
void vtkCTHFragmentPieceTransactionMatrix::PushTransaction(
                int fragmentId,
                int procId,
                vtkCTHFragmentPieceTransaction &transaction)
{
  int idx=fragmentId+procId*this->NFragments;
  this->Matrix[idx].push_back(transaction);
  ++this->NumberOfTransactions;
}
//
inline
vector<vtkCTHFragmentPieceTransaction> &
vtkCTHFragmentPieceTransactionMatrix::GetTransactions(
                int fragmentId,
                int procId)
{
  int idx=fragmentId+procId*this->NFragments;
  return this->Matrix[idx];
}
//
inline 
vtkIdType 
vtkCTHFragmentPieceTransactionMatrix::GetNumberOfTransactions(
                int procId)
{
  int nTransactions=0;

  for (int fragmentId=0; fragmentId<this->NFragments; ++fragmentId)
    {
    nTransactions+=this->GetTransactions(fragmentId,procId).size();
    }

  return nTransactions;
}
// 
vtkIdType vtkCTHFragmentPieceTransactionMatrix::PackRow(int *&buf)
{
/*
the packed row has this structure:

[nT,T0,...TN][nT,T0,...,TN]...[nT,T0,...,TN]
    /\           /\               /\
    |            |                |
  f0,p0        f1,p0            fN,p0 
*/

  // caller should not allocate memory.
  assert( "Buffer appears to be pre-allocated."
          && buf==0 );

  //int matIdx=i+j*this->NFragments;
  //int nTransactions=this->Matrix[matIdx].size();

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  const vtkIdType bufSize = this->NFragments         // transaction count for each fragment
        + transactionSize*this->NumberOfTransactions // enough to store all transactions
        + 2;                                         // nFragments, nProcs

  buf = new int[bufSize];
  // header
  buf[0]=this->NFragments;
  buf[1]=this->NProcs;
  vtkIdType bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      // put the count for this i,j
      buf[bufIdx]=nTransactions;
      ++bufIdx;

      // put this i,j's transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].Pack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }
  // now bufIdx is number of ints we have stored
  return bufIdx;
}
// 
vtkIdType vtkCTHFragmentPieceTransactionMatrix::Pack(int *&buf)
{
/*
the buffer has this structure:

[nFragments,nProcs][nT,T0,...TN][nT,T0,...,TN]...[nT,T0,...,TN]...[nT,T0,...,TN]
                       /\           /\               /\               /\
                       |            |                |                |
                      f0,p0        f1,p0            fN,p0            fN,PN
*/

  // caller should not allocate memory.
  assert( "Buffer appears to be pre-allocated."
          && buf==0 );

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  const vtkIdType bufSize = this->FlatMatrixSize     // transaction count for each i,j
        + transactionSize*this->NumberOfTransactions // enough to store all transactions
        + 2;                                         // nFragments, nProcs

  buf = new int[bufSize];
  // header
  buf[0]=this->NFragments;
  buf[1]=this->NProcs;
  vtkIdType bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      // put the count for this i,j
      buf[bufIdx]=nTransactions;
      ++bufIdx;

      // put this i,j's transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].Pack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }
  // now bufIdx is number of ints we have stored
  return bufIdx;
}
//
int vtkCTHFragmentPieceTransactionMatrix::UnPack(int *buf)
{
  assert("Buffer has not been allocated."
         && buf!=0 );

  const int transactionSize
    = vtkCTHFragmentPieceTransaction::SIZE;

  this->Initialize(buf[0],buf[1]);
  int bufIdx=2;

  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      // get the number of transactions for this i,j
      int nTransactions=buf[bufIdx];
      ++bufIdx;

      // size the i,j th transaction list
      int matIdx=i+j*this->NFragments;
      this->Matrix[matIdx].resize(nTransactions);

      // load the i,j th transaction list
      for (int q=0; q<nTransactions; ++q)
        {
        this->Matrix[matIdx][q].UnPack(&buf[bufIdx]);
        bufIdx+=transactionSize;
        }
      }
    }

  return 1;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Broadcast(
                vtkCommunicator *comm,
                int srcProc)
{
  int myProc=comm->GetLocalProcessId();

  // pack
  int *buf=0;
  int bufSize=0;
  if ( myProc==srcProc )
    {
    bufSize=this->Pack(buf);
    }

  // move
  comm->Broadcast(&bufSize,1,srcProc);
  if ( myProc!=srcProc )
    {
    buf = new int[bufSize];
    }
  comm->Broadcast(buf,bufSize,srcProc);

  // unpack
  if ( myProc!=srcProc )
    {
    this->UnPack(buf);
    }

  // clean up
  delete [] buf;
}
//
void vtkCTHFragmentPieceTransactionMatrix::Print()
{
  for (int j=0; j<this->NProcs; ++j)
    {
    for (int i=0; i<this->NFragments; ++i)
      {
      int matIdx=i+j*this->NFragments;
      int nTransactions=this->Matrix[matIdx].size();

      if (nTransactions>0)
        {
        cerr << "TM[f=" << i << ",p=" << j << "]=";

        // put this i,j's transaction list
        for (int q=0; q<nTransactions; ++q)
          {
          cerr << this->Matrix[matIdx][q] << ",";
          }
        cerr << endl;
        }
      }
    }
}
//============================================================================
// A class that implements an equivalent set.  It is used to combine fragments
// from different processes.
//
// I believe that this class is a strictly ordered tree of equivalences.
// Every member points to its own id or an id smaller than itself.
class vtkCTHFragmentEquivalenceSet
{
public:
  vtkCTHFragmentEquivalenceSet();
  ~vtkCTHFragmentEquivalenceSet();

  void Print();

  void Initialize();
  void AddEquivalence(int id1, int id2);

  // The length of the equivalent array...
  int GetNumberOfMembers() { return this->EquivalenceArray->GetNumberOfTuples();}

  // Return the id of the equivalent set.
  int GetEquivalentSetId(int memberId);

  // Equivalent set ids are reassinged to be sequential.
  // You cannot add anymore equivalences after this is called.
  int ResolveEquivalences();

  void DeepCopy(vtkCTHFragmentEquivalenceSet* in);

  // Needed for sending the set over MPI.
  // Be very careful with the pointer.
  int* GetPointer() { return this->EquivalenceArray->GetPointer(0);}

  // Free unused memory
  void Squeeze(){ this->EquivalenceArray->Squeeze(); }

  // Report used memory
  vtkIdType Capacity(){ return this->EquivalenceArray->Capacity(); }

  // We should fix the pointer API and hide this ivar.
  int Resolved;

private:

  // To merge connected framgments that have different ids because they were
  // traversed by different processes or passes.
  vtkIntArray *EquivalenceArray;

  // Return the id of the equivalent set.
  int GetReference(int memberId);
  void EquateInternal(int id1, int id2);
};

//----------------------------------------------------------------------------
vtkCTHFragmentEquivalenceSet::vtkCTHFragmentEquivalenceSet()
{
  this->Resolved = 0;
  this->EquivalenceArray = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkCTHFragmentEquivalenceSet::~vtkCTHFragmentEquivalenceSet()
{
  this->Resolved = 0;
  if (this->EquivalenceArray)
    {
    this->EquivalenceArray->Delete();
    this->EquivalenceArray = 0;
    }
}

//----------------------------------------------------------------------------
void vtkCTHFragmentEquivalenceSet::Initialize()
{
  this->Resolved = 0;
  this->EquivalenceArray->Initialize();
}

//----------------------------------------------------------------------------
void vtkCTHFragmentEquivalenceSet::DeepCopy(vtkCTHFragmentEquivalenceSet* in)
{
  this->Resolved = in->Resolved;
  this->EquivalenceArray->DeepCopy(in->EquivalenceArray);
}

//----------------------------------------------------------------------------
void vtkCTHFragmentEquivalenceSet::Print()
{
  vtkIdType num = this->GetNumberOfMembers();
  cerr << num << endl;
  for (vtkIdType ii = 0; ii < num; ++ii)
    {
    cerr << "  " << ii << " : " << this->GetEquivalentSetId(ii) << endl;
    }
  cerr << endl;
}


//----------------------------------------------------------------------------
// Return the id of the equivalent set.
int vtkCTHFragmentEquivalenceSet::GetEquivalentSetId(int memberId)
{
  int ref;

  ref = this->GetReference(memberId);
  while (!this->Resolved && ref != memberId)
    {
    memberId = ref;
    ref = this->GetReference(memberId);
    }

  return ref;
}

//----------------------------------------------------------------------------
// Return the id of the equivalent set.
int vtkCTHFragmentEquivalenceSet::GetReference(int memberId)
{
  if (memberId >= this->EquivalenceArray->GetNumberOfTuples())
    { // We might consider this an error ...
    return memberId;
    }
  return this->EquivalenceArray->GetValue(memberId);
}

//----------------------------------------------------------------------------
// Makes two new or existing ids equivalent.
// If the array is too small, the range of ids is increased until it contains
// both the ids.  Negative ids are not allowed.
void vtkCTHFragmentEquivalenceSet::AddEquivalence(int id1, int id2)
{
  if (this->Resolved)
    {
    vtkGenericWarningMacro("Set already resolved, you cannot add more equivalences.");
    return;
    }

  int num = this->EquivalenceArray->GetNumberOfTuples();

  // Expand the range to include both ids.
  while (num <= id1 || num <= id2)
    {
    // All values inserted are equivalent to only themselves.
    this->EquivalenceArray->InsertNextTuple1(num);
    ++num;
    }

 // Our rule for references in the equivalent set is that
 // all elements must point to a member equal to or smaller
 // than itself.

 // The only problem we could encounter is changing a reference.
 // we do not want to orphan anything previously referenced.

  // Replace the larger references.
  if (id1 < id2)
    {
    // We could follow the references to the smallest.  It might
    // make this processing more efficient.  This is a compromise.
    // The referenced id will always be smaller than the id so
    // order does not change.
    this->EquateInternal(this->GetReference(id1), id2);
    }
  else
    {
    this->EquateInternal(this->GetReference(id2), id1);
    }
}

//----------------------------------------------------------------------------
// id1 must be less than or equal to id2.
void vtkCTHFragmentEquivalenceSet::EquateInternal(int id1, int id2)
{
  // This is the reference that might be orphaned in this process.
  int oldRef = this->GetEquivalentSetId(id2);
  
 // The only problem we could encounter is changing a reference.
 // we do not want to orphan anything previously referenced.
  if (oldRef == id2 || oldRef == id1)
    {
    this->EquivalenceArray->SetValue(id2, id1);
    }
  else if (oldRef > id1)
    {
    this->EquivalenceArray->SetValue(id2, id1);
    this->EquateInternal(id1, oldRef);
    }
  else
    { // oldRef < id1
    this->EquateInternal(oldRef, id1);
    }
}


//----------------------------------------------------------------------------
// Returns the number of merged sets.
int vtkCTHFragmentEquivalenceSet::ResolveEquivalences()
{
  // Go through the equivalence array collapsing chains 
  // and assigning consecutive ids.
  int count = 0;
  int id;
  int newId;

  int numIds = this->EquivalenceArray->GetNumberOfTuples();
  for (int ii = 0; ii < numIds; ++ii)
    {
    id = this->EquivalenceArray->GetValue(ii);
    if (id == ii)
      { // This is a new equivalence set.
      this->EquivalenceArray->SetValue(ii, count);
      ++count;
      }
    else
      {
      // All earlier ids will be resolved already.
      // This array only point to less than or equal ids. (id <= ii).
      newId = this->EquivalenceArray->GetValue(id);
      this->EquivalenceArray->SetValue(ii,newId);
      }
    }
  this->Resolved = 1;
  //cerr << "Final number of equivalent sets: " << count << endl;

  return count;
}



//============================================================================
// A class to hold block information. Input is not HierarichalBox so I need
// to extract and store this extra information for each block.
class vtkCTHFragmentConnectBlock
{
public:
  vtkCTHFragmentConnectBlock();
  ~vtkCTHFragmentConnectBlock();

  // For normal (local) blocks.
  void Initialize(int blockId, vtkImageData* imageBlock, int level, 
                  double globalOrigin[3], double rootSapcing[3],
                  string &volumeFractionArrayName,
                  string &massArrayName,
                  vector<string> &volumeWtdAvgArrayNames,
                  vector<string> &massWtdAvgArrayNames,
                  vector<string> &summedArrayNames,
                  vector<string> &integratedArrayNames);

  // Determine the extent without overlap using the standard block
  // dimensions. (ie indexes region excluding ghosts)
  void ComputeBaseExtent(int blockDims[3]);

  // For ghost layers received by other processes.
  // The argument list here is getting a bit long ....
  void InitializeGhostLayer(unsigned char* volFraction, 
                            int cellExtent[6],
                            int level,
                            double globalOrigin[3],
                            double rootSpacing[3],
                            int ownerProcessId,
                            int blockId);
  // This adds a block as our neighbor and makes the reciprical connection too.
  // It is used to connect ghost layer blocks.  Ghost blocks need the reciprical 
  // connection to continue the connectivity search.
  // I am hoping this will make the number of fragments to merge smaller.
  void AddNeighbor(vtkCTHFragmentConnectBlock *block, int axis, int maxFlag);

  void GetPointExtent(int ext[6]);
  void GetCellExtent(int ext[6]);
  void GetCellIncrements(int incs[3]);
  const int* GetCellIncrements() {return this->CellIncrements;}
  // Get the cell extents that exclude the ghosts.
  void GetBaseCellExtent(int ext[6]);
  // This was a major time consumer so use the pointer directly.
  const int* GetBaseCellExtent() { return this->BaseCellExtent;}

  unsigned char GetGhostFlag() { return this->GhostFlag;}
  // Information saved for ghost cells that makes it easier to
  // resolve equivalent fragment ids.
  int GetOwnerProcessId() { return this->ProcessId;}
  int GetBlockId() { return this->BlockId;}

  //Returns a pointer to the minimum AMR Extent with is the first
  // cell values that is not a ghost cell.
  unsigned char* GetBaseVolumeFractionPointer();
  int*           GetBaseFragmentIdPointer();
  int            GetBaseFlatIndex();
  int*           GetFragmentIdPointer() {return this->FragmentIds;}
  int            GetLevel() {return this->Level;}
  double*        GetSpacing() {return this->Spacing;}
  double*        GetOrigin() {return this->Origin;}
  // Faces are indexed: 0=xMin, 1=xMax, 2=yMin, 3=yMax, 4=zMin, 5=zMax
  int GetNumberOfFaceNeighbors(int face)
  {
    return this->Neighbors[face].size();
  }
  //
  vtkCTHFragmentConnectBlock* GetFaceNeighbor(int face, int neighborId)
  {
    return (vtkCTHFragmentConnectBlock*)(this->Neighbors[face][neighborId]);
  }
  //
  vtkDataArray *GetVolumeWtdAvgArray(int id)
  {
    assert(id>=0 && id<this->NVolumeWtdAvgs);
    return this->VolumeWtdAvgArrays[id];
  }
  //
  vtkDataArray *GetMassWtdAvgArray(int id)
  {
    assert(id>=0 && id<this->NMassWtdAvgs);
    return this->MassWtdAvgArrays[id];
  }
  //
  vtkDataArray *GetIntegratedArray(int id)
  {
    assert(id>=0 && id<this->NToIntegrate);
    return this->IntegratedArrays[id];
  }
  //
  vtkDataArray *GetArrayToSum(int id)
  {
    assert(id>=0 && id<this->NToSum);
    return this->ArraysToSum[id];
  }
  //
  vtkDataArray *GetMassArray(){ return this->MassArray; }

  // For basis of face coordinate systems.
  // -x,x,-y,y,-z,z
  double HalfEdges[6][3];
  // Just for debugging.
  int LevelBlockId;
  void ExtractExtent(unsigned char* buf, int ext[6]);

private:
  unsigned char GhostFlag;
  // Information saved for ghost cells that makes it easier to
  // resolve equivalent fragment ids.
  int BlockId;
  int ProcessId;
  // This id is for connectivity computation.
  // It is essentially a cell array of the image.
  int *FragmentIds;
  // The original image block (worry about rectilinear grids later).
  // We have two ways of managing the memory.  We only keep the 
  // image around to keep a reference.
  vtkImageData* Image;
  unsigned char* VolumeFractionArray;
  // arrays to integrate
  // we need to know the name, and number of components
  // during integration/resolution process
  vector<vtkDataArray *>IntegratedArrays;
  int NToIntegrate;
  vector<vtkDataArray *>VolumeWtdAvgArrays;
  int NVolumeWtdAvgs;
  vector<vtkDataArray *>MassWtdAvgArrays;
  int NMassWtdAvgs;
  vector<vtkDataArray *>ArraysToSum;
  int NToSum;
  vtkDataArray *MassArray; // do not delete
  // We might as well store the increments.
  int CellIncrements[3];
  // Extent of the cell arrays as in memory.
  // All cells.
  int CellExtent[6];
  // Useful for neighbor computations and to avoid processing overlapping cells.
  // Cell extents that exclude the ghost cells.
  int BaseCellExtent[6];
  // The blocks do not follow the convention of sharing an origin.
  // Just save these for placing the faces.  We will have to find
  // another way to compute neighbors.
  double Spacing[3]; // cell dx this block
  double Origin[3]; // lower left xyz of this block
  int Level;
  // Store index to neiboring blocks.
  // There may be more than one neighbor because higher levels.
  vtkstd::vector<vtkCTHFragmentConnectBlock*> Neighbors[6];
};

//----------------------------------------------------------------------------
vtkCTHFragmentConnectBlock::vtkCTHFragmentConnectBlock ()
{
  this->GhostFlag = 0;
  this->Image = 0;
  this->VolumeFractionArray = 0;
  this->Level = 0;
  this->CellIncrements[0] = this->CellIncrements[1] = this->CellIncrements[2] = 0;
  for (int ii = 0; ii < 6; ++ii)
    {
    this->CellExtent[ii] = 0;
    this->BaseCellExtent[ii] = 0;
    }
  this->FragmentIds = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->NToIntegrate=0;
  this->NVolumeWtdAvgs=0;
  this->NMassWtdAvgs=0;
  this->NToSum=0;
}
//----------------------------------------------------------------------------
vtkCTHFragmentConnectBlock::~vtkCTHFragmentConnectBlock()
{
  if (this->Image)
    {
    this->VolumeFractionArray = 0;
    this->Image->UnRegister(0);
    this->Image = 0;
    }
  if (this->VolumeFractionArray)
    { // Memory was allocated without an image.
    delete [] this->VolumeFractionArray;
    this->VolumeFractionArray = 0;
    }

  this->Level = 0;

  for (int ii = 0; ii < 6; ++ii)
    {
    this->CellExtent[ii] = 0;
    this->BaseCellExtent[ii] = 0;
    }
  if (this->FragmentIds != 0)
    {
    delete [] this->FragmentIds;
    this->FragmentIds = 0;
    }
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->NToIntegrate=0;
  this->NVolumeWtdAvgs=0;
  this->NMassWtdAvgs=0;
  this->NToSum=0;
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::Initialize(
        int blockId,
        vtkImageData* image,
        int level,
        double globalOrigin[3],
        double rootSpacing[3],
        string &volumeFractionArrayName,
        string &massArrayName,
        vector<string> &volumeWtdAvgArrayNames,
        vector<string> &massWtdAvgArrayNames,
        vector<string> &summedArrayNames,
        vector<string> &integratedArrayNames)
{
  if (this->VolumeFractionArray)
    {
    vtkGenericWarningMacro("Block alread initialized !!!");
    return;
    }
  if (image == 0)
    {
    vtkGenericWarningMacro("No image to initialize with.");
    return;
    }

  this->BlockId = blockId;
  this->Image = image;
  this->Image->Register(0);
  this->Level = level;
  image->GetSpacing(this->Spacing);
  image->GetOrigin(this->Origin);

  int numCells = image->GetNumberOfCells();
  this->FragmentIds = new int[numCells];
  // Initialize fragment ids to -1 / uninitialized
  for (int ii = 0; ii < numCells; ++ii)
    {
    this->FragmentIds[ii] = -1;
    }
  int imageExt[6];
  image->GetExtent(imageExt);

  // get a pointer to the volume fraction data
  vtkDataArray* volumeFractionArray 
    = this->Image->GetCellData()->GetArray(volumeFractionArrayName.c_str());
  assert( "Could not find volume fraction array." 
          && volumeFractionArray );
  this->VolumeFractionArray 
    = (unsigned char*)(volumeFractionArray->GetVoidPointer(0));

  // get pointers to arrays to volume weighted average
  this->NVolumeWtdAvgs=volumeWtdAvgArrayNames.size();
  this->VolumeWtdAvgArrays.clear();
  this->VolumeWtdAvgArrays.resize(this->NVolumeWtdAvgs,0);
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    this->VolumeWtdAvgArrays[i]
      = this->Image->GetCellData()->GetArray(volumeWtdAvgArrayNames[i].c_str());
    //
    assert("\nCould not find array to weighted average.\n"
           && this->VolumeWtdAvgArrays[i]);
    }
  // get pointers to arrays to mass weighted average
  this->NMassWtdAvgs=massWtdAvgArrayNames.size();
  this->MassWtdAvgArrays.clear();
  this->MassWtdAvgArrays.resize(this->NMassWtdAvgs,0);
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    this->MassWtdAvgArrays[i]
      = this->Image->GetCellData()->GetArray(massWtdAvgArrayNames[i].c_str());
    //
    assert("\nCould not find array to weighted average.\n"
           && this->MassWtdAvgArrays[i]);
    }
  // get pointers to arrays to directly copy to the 
  // output.ie Integrated arrays
  this->NToIntegrate=integratedArrayNames.size();
  this->IntegratedArrays.clear();
  this->IntegratedArrays.resize(this->NToIntegrate,0);
  for (int i=0; i<this->NToIntegrate; ++i)
    {
    this->IntegratedArrays[i]
      = this->Image->GetCellData()->GetArray(integratedArrayNames[i].c_str());

    assert("\nCould not find array to integrate.\n"
            && this->IntegratedArrays[i]);
    }
  // get pointers to arrays to sum
  this->NToSum=summedArrayNames.size();
  this->ArraysToSum.clear();
  this->ArraysToSum.resize(this->NToSum,0);
  for (int i=0; i<this->NToSum; ++i)
    {
    this->ArraysToSum[i]
      = this->Image->GetCellData()->GetArray(summedArrayNames[i].c_str());

    assert( "\nCould not find array to sum.\n"
            && this->ArraysToSum[i] );
    }
  // Get the mass array, if it is provided then certain 
  // calculations which depend on it will be unavailable.
  this->MassArray=0;
  if (!massArrayName.empty())
    {
    this->MassArray
      = this->Image->GetCellData()->GetArray(massArrayName.c_str());
    }

  // Since CTH does not use convention that all blocks share an origin,
  // We must compute a new extent to help locate neighboring blocks.
  // It is important to compute the global origin so that the base min
  // extent is a multiple of the block dimensions (0, 8, 16...).
  int shift;
  shift = (int)(((this->Origin[0] - globalOrigin[0]) / this->Spacing[0]) + 0.5);
  this->CellExtent[0] = imageExt[0] + shift;
  this->CellExtent[1] = imageExt[1] + shift - 1;
  shift = (int)(((this->Origin[1] - globalOrigin[1]) / this->Spacing[1]) + 0.5); 
  this->CellExtent[2] = imageExt[2] + shift;
  this->CellExtent[3] = imageExt[3] + shift - 1;
  shift = (int)(((this->Origin[2] - globalOrigin[2]) / this->Spacing[2]) + 0.5); 
  this->CellExtent[4] = imageExt[4] + shift;
  this->CellExtent[5] = imageExt[5] + shift - 1;
  for (int ii = 0; ii < 3; ++ii)
    {
    this->Origin[ii] = globalOrigin[ii];
    }

  // On this pass, assume that there is no overalp.
  this->BaseCellExtent[0] = this->CellExtent[0];
  this->BaseCellExtent[1] = this->CellExtent[1];
  this->BaseCellExtent[2] = this->CellExtent[2];
  this->BaseCellExtent[3] = this->CellExtent[3];
  this->BaseCellExtent[4] = this->CellExtent[4];
  this->BaseCellExtent[5] = this->CellExtent[5];

  this->CellIncrements[0] = 1;
  // Point extent -> cell increments.  Do not add 1.
  this->CellIncrements[1] = imageExt[1]-imageExt[0];
  this->CellIncrements[2] = this->CellIncrements[1] * (imageExt[3]-imageExt[2]);

  // As a sanity check, compare spacing and level.
  #ifdef NDEBUG
  (void)rootSpacing; // avoid a compiler warning when assert goes away
  #endif
  assert( "Spacing does not look correct for CTH AMR structure."
          && (int)(rootSpacing[0] / this->Spacing[0] + 0.5) == (1<<(this->Level))
          && (int)(rootSpacing[1] / this->Spacing[1] + 0.5) == (1<<(this->Level))
          && (int)(rootSpacing[2] / this->Spacing[2] + 0.5) == (1<<(this->Level)) );

  // This will have to change, but I will leave it for now.
  this->HalfEdges[1][0] = this->Spacing[0]*0.5;
  this->HalfEdges[1][1] = this->HalfEdges[1][2] = 0.0;
  this->HalfEdges[3][1] = this->Spacing[1]*0.5;
  this->HalfEdges[3][0] = this->HalfEdges[3][2] = 0.0;
  this->HalfEdges[5][2] = this->Spacing[2]*0.5;
  this->HalfEdges[5][0] = this->HalfEdges[5][1] = 0.0;
  for (int ii = 0; ii < 3; ++ii)
    {
    this->HalfEdges[0][ii] = -this->HalfEdges[1][ii];
    this->HalfEdges[2][ii] = -this->HalfEdges[3][ii];
    this->HalfEdges[4][ii] = -this->HalfEdges[5][ii];
    }
}

//----------------------------------------------------------------------------
// I would really like to make subclasses of blocks,
// Memory is managed differently.
// Skip much of the initialization because ghost layers produce no surface.
// Ghost layers need half edges too.
void vtkCTHFragmentConnectBlock::ComputeBaseExtent(int blockDims[3])
{
  if (this->GhostFlag)
    {
    // This computation does not work for ghost cells.
    // InitializeGhostCell should setup the base extent properly.
    return;
    }

  // Lets store the point extent of the block without any overlap.
  // We cannot assume that all blocks have an overlap of 1.
  // because the spyplot reader strips the "invalid" overlap cells from 
  // the outer surface of the data.
  int baseDim;
  int iMin, iMax;
  int tmp;
  for (int ii = 0; ii < 3; ++ii)
    {
    baseDim = blockDims[ii];
    iMin = 2* ii;
    iMax = iMin + 1;
    // This assumes that all extents are positive.  ceil is too complicated.
    tmp = this->BaseCellExtent[iMin];
    tmp = (tmp+baseDim-1) / baseDim;
    this->BaseCellExtent[iMin] = tmp*baseDim;

    tmp = this->BaseCellExtent[iMax]+1;
    tmp = tmp / baseDim;
    this->BaseCellExtent[iMax] = tmp*baseDim - 1;
    }
}


//----------------------------------------------------------------------------
// I would really like to make subclasses of blocks,
// Memory is managed differently.
// Skip much of the initialization because ghost layers produce no surface.
// Ghost layers need half edges too.
void vtkCTHFragmentConnectBlock::InitializeGhostLayer(
  unsigned char* volFraction, 
  int cellExtent[6],
  int level,
  double globalOrigin[3],
  double rootSpacing[3],
  int ownerProcessId,
  int blockId)
{
  if (this->VolumeFractionArray)
    {
    vtkGenericWarningMacro("Block alread initialized !!!");
    return;
    }

  this->ProcessId = ownerProcessId;
  this->GhostFlag = 1;
  this->BlockId = blockId;

  this->Image = 0;
  this->Level = level;
  // Skip spacing and origin.

  int numCells = (cellExtent[1]-cellExtent[0]+1)
                   *(cellExtent[3]-cellExtent[2]+1)
                   *(cellExtent[5]-cellExtent[4]+1);
  this->FragmentIds = new int[numCells];
  // Initialize fragment ids to -1 / uninitialized
  for (int ii = 0; ii < numCells; ++ii)
    {
    this->FragmentIds[ii] = -1;
    }

  // Extract what we need form the image because we do not reference it again.
  this->VolumeFractionArray = new unsigned char[numCells];
  memcpy(this->VolumeFractionArray,volFraction, numCells);

  this->CellExtent[0] = cellExtent[0];
  this->CellExtent[1] = cellExtent[1];
  this->CellExtent[2] = cellExtent[2];
  this->CellExtent[3] = cellExtent[3];
  this->CellExtent[4] = cellExtent[4];
  this->CellExtent[5] = cellExtent[5];

  // No overlap in ghost layers
  this->BaseCellExtent[0] = cellExtent[0];
  this->BaseCellExtent[1] = cellExtent[1];
  this->BaseCellExtent[2] = cellExtent[2];
  this->BaseCellExtent[3] = cellExtent[3];
  this->BaseCellExtent[4] = cellExtent[4];
  this->BaseCellExtent[5] = cellExtent[5];

  this->CellIncrements[0] = 1;
  // Point extent -> cell increments.  Do not add 1.
  this->CellIncrements[1] = cellExtent[1]-cellExtent[0]+1; 
  this->CellIncrements[2] = this->CellIncrements[1] * (cellExtent[3]-cellExtent[2]+1);

  for (int ii = 0; ii < 3; ++ii)
    {
    this->Origin[ii] = globalOrigin[ii];
    this->Spacing[ii] =  rootSpacing[ii] / (double)(1<<(this->Level));
    }

  // This will have to change, but I will leave it for now.
  this->HalfEdges[1][0] = this->Spacing[0]*0.5;
  this->HalfEdges[1][1] = this->HalfEdges[1][2] = 0.0;
  this->HalfEdges[3][1] = this->Spacing[1]*0.5;
  this->HalfEdges[3][0] = this->HalfEdges[3][2] = 0.0;
  this->HalfEdges[5][2] = this->Spacing[2]*0.5;
  this->HalfEdges[5][0] = this->HalfEdges[5][1] = 0.0;
  for (int ii = 0; ii < 3; ++ii)
    {
    this->HalfEdges[0][ii] = -this->HalfEdges[1][ii];
    this->HalfEdges[2][ii] = -this->HalfEdges[3][ii];
    this->HalfEdges[4][ii] = -this->HalfEdges[5][ii];
    }
}



//----------------------------------------------------------------------------
// Flipped a coin.  This method only addes the neighbor relation one way.
// User needs to call it twice to get the backward connection.
void vtkCTHFragmentConnectBlock::AddNeighbor(
  vtkCTHFragmentConnectBlock *neighbor, 
  int axis, 
  int maxFlag)
{
  if (maxFlag)
    { // max neighbor
    this->Neighbors[2*axis+1].push_back(neighbor);
    }
  else
    { // min neighbor
    this->Neighbors[2*axis].push_back(neighbor);
    }
}


//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::GetPointExtent(int ext[6])
{
  ext[0] = this->CellExtent[0];
  ext[1] = this->CellExtent[1]+1;
  ext[2] = this->CellExtent[2];
  ext[3] = this->CellExtent[3]+1;
  ext[4] = this->CellExtent[4];
  ext[5] = this->CellExtent[5]+1;
}
//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::GetCellExtent(int ext[6])
{
  memcpy(ext, this->CellExtent, 6*sizeof(int));
}
//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::GetCellIncrements(int incs[3])
{
  incs[0] = this->CellIncrements[0];
  incs[1] = this->CellIncrements[1];
  incs[2] = this->CellIncrements[2];
}
//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::GetBaseCellExtent(int ext[6])
{
  // Since this is taking a significant amount of time in the
  // GetNeighborIterator method, wemight consider storing
  // the base cell extent directly.
  *ext++ = this->BaseCellExtent[0];
  *ext++ = this->BaseCellExtent[1];
  *ext++ = this->BaseCellExtent[2];
  *ext++ = this->BaseCellExtent[3];
  *ext++ = this->BaseCellExtent[4];
  *ext++ = this->BaseCellExtent[5];
}

//----------------------------------------------------------------------------
//Returns a pointer to the minimum AMR Extent with is the first
// cell values that is not a ghost cell.
unsigned char* vtkCTHFragmentConnectBlock::GetBaseVolumeFractionPointer()
{
  unsigned char* ptr = this->VolumeFractionArray;

  ptr += this->CellIncrements[0] * (this->BaseCellExtent[0] 
                                    - this->CellExtent[0]);
  ptr += this->CellIncrements[1] * (this->BaseCellExtent[2] 
                                    - this->CellExtent[2]);
  ptr += this->CellIncrements[2] * (this->BaseCellExtent[4] 
                                    - this->CellExtent[4]);
  
  return ptr;
}

//----------------------------------------------------------------------------
int* vtkCTHFragmentConnectBlock::GetBaseFragmentIdPointer()
{
  int* ptr = this->FragmentIds;

  ptr += this->CellIncrements[0] * (this->BaseCellExtent[0] 
                                    - this->CellExtent[0]);
  ptr += this->CellIncrements[1] * (this->BaseCellExtent[2] 
                                    - this->CellExtent[2]);
  ptr += this->CellIncrements[2] * (this->BaseCellExtent[4] 
                                    - this->CellExtent[4]);

  return ptr;
}
//----------------------------------------------------------------------------
// returns the index from the lower left cell 
// that gets you to the first non-ghost cell
int vtkCTHFragmentConnectBlock::GetBaseFlatIndex()
{
  int idx=0;
  idx += this->CellIncrements[0] * (this->BaseCellExtent[0] 
                                    - this->CellExtent[0]);
  idx += this->CellIncrements[1] * (this->BaseCellExtent[2] 
                                    - this->CellExtent[2]);
  idx += this->CellIncrements[2] * (this->BaseCellExtent[4] 
                                    - this->CellExtent[4]);
  return idx;
}
//----------------------------------------------------------------------------
void vtkCTHFragmentConnectBlock::ExtractExtent(unsigned char* buf, int ext[6])
{
  // Initialize the buffer to 0.
  memset(buf,0,(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1));

  unsigned char* volumeFractionPointer = this->VolumeFractionArray;

  // Compute increments.
  int inc0 = this->CellIncrements[0];
  int inc1 = this->CellIncrements[1];
  int inc2 = this->CellIncrements[2];
  int cellExtent[6];
  this->GetCellExtent(cellExtent);
  unsigned char* ptr0;
  unsigned char* ptr1;
  unsigned char* ptr2;
  // Find the starting pointer.
  ptr2 = volumeFractionPointer + inc0*(ext[0]-cellExtent[0])
                               + inc1*(ext[2]-cellExtent[2])
                               + inc2*(ext[4]-cellExtent[4]);
  int xx, yy, zz;
  for (zz = ext[4]; zz <= ext[5]; ++zz)
    {
    ptr1 = ptr2;
    for (yy = ext[2]; yy <= ext[3]; ++yy)
      {
      ptr0 = ptr1;
      for (xx = ext[0]; xx <= ext[1]; ++xx)
        {
        *buf = *ptr0;
        ++buf;
        ptr0 += inc0;
        }
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }
}


//============================================================================
// A class to make finding neighbor blocks faster.
// We assume that each level has a regular array of blocks.
// Of course blocks may be missing from the array.
class vtkCTHFragmentLevel
{
public:
  vtkCTHFragmentLevel();
  ~vtkCTHFragmentLevel();

  // Description:
  // Block dimensions should be cell dimensions without overlap.
  // GridExtent is the extent of block indexes. (Block 1, block 2, ...)
  void Initialize(int gridExtent[6], int Level);

  // Description:
  // Called after initialization.
  void SetStandardBlockDimensions(int dims[3]);

  // Description:
  // This adds a block.  Return VTK_ERROR if the block is not in the
  // correct extent (VTK_OK otherwise).
  int AddBlock(vtkCTHFragmentConnectBlock* block);

  // Description:
  // Get the block at this grid index.
  vtkCTHFragmentConnectBlock* GetBlock(int xIdx, int yIdx, int zIdx);

  // Decription:
  // Get the cell dimensions of the blocks (base extent) stored in this level.
  // We assume that all blocks are the same size.
  void GetBlockDimensions(int dims[3]);

  // Description:
  // Get the extrent of the grid of blocks for this level.
  // Extent should be in cell coordinates.
  void GetGridExtent(int ext[6]);

private:
  int Level;
  int GridExtent[6];
  int BlockDimensions[3];

  vtkCTHFragmentConnectBlock** Grid;
};

//----------------------------------------------------------------------------
vtkCTHFragmentLevel::vtkCTHFragmentLevel ()
{
  this->Level = 0;
  for (int ii = 0; ii < 6; ++ii)
    {
    this->GridExtent[ii] = 0;
    }
  this->BlockDimensions[0] = 0;
  this->BlockDimensions[1] = 0;
  this->BlockDimensions[2] = 0;
  this->Grid = 0;
}
//----------------------------------------------------------------------------
vtkCTHFragmentLevel::~vtkCTHFragmentLevel()
{
  this->Level = 0;

  this->BlockDimensions[0] = 0;
  this->BlockDimensions[1] = 0;
  this->BlockDimensions[2] = 0;
  if (this->Grid)
    {
    int num = (this->GridExtent[1]-this->GridExtent[0]+1)
                * (this->GridExtent[3]-this->GridExtent[2]+1)
                * (this->GridExtent[5]-this->GridExtent[4]+1);
    for (int ii = 0; ii < num; ++ii)
      {
      if (this->Grid[ii])
        {
        // We are not the only container for blocks yet.
        //delete this->Grid[ii];
        this->Grid[ii] = 0;
        }
      }
    delete [] this->Grid;
    }
  for (int ii = 0; ii < 6; ++ii)
    {
    this->GridExtent[ii] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkCTHFragmentLevel::GetBlockDimensions(int dims[3])
{
  dims[0] = this->BlockDimensions[0];
  dims[1] = this->BlockDimensions[1];
  dims[2] = this->BlockDimensions[2];
}

//----------------------------------------------------------------------------
void vtkCTHFragmentLevel::GetGridExtent(int ext[6])
{
  for (int ii = 0; ii < 6; ++ii)
    {
    ext[ii] = this->GridExtent[ii];
    }
}

//----------------------------------------------------------------------------
void vtkCTHFragmentLevel::Initialize(
  int gridExtent[6],
  int level)
{
  if (this->Grid)
    {
    vtkGenericWarningMacro("Level already initialized.");
    return;
    }
  // Special case for a level with no blocks in it.
  if (gridExtent[0] > gridExtent[1] || gridExtent[2] > gridExtent[3] || gridExtent[4] > gridExtent[5])
    { // This should do the trick.
    gridExtent[0] = gridExtent[1] = gridExtent[2] = gridExtent[3] = gridExtent[4] = gridExtent[5] = 0;
    }

  this->Level = level;
  memcpy(this->GridExtent, gridExtent, 6*sizeof(int));
  int num = (this->GridExtent[1]-this->GridExtent[0]+1)
            * (this->GridExtent[3]-this->GridExtent[2]+1)
            * (this->GridExtent[5]-this->GridExtent[4]+1);
  this->Grid = new vtkCTHFragmentConnectBlock*[num];
  memset(this->Grid,0,num*sizeof(vtkCTHFragmentConnectBlock*));
}


//----------------------------------------------------------------------------
void vtkCTHFragmentLevel::SetStandardBlockDimensions(int dims[3])
{
  this->BlockDimensions[0] = dims[0];
  this->BlockDimensions[1] = dims[1];
  this->BlockDimensions[2] = dims[2];
}

//----------------------------------------------------------------------------
int vtkCTHFragmentLevel::AddBlock(vtkCTHFragmentConnectBlock* block)
{
  int xIdx, yIdx, zIdx;

  // First make sure the level is correct.
  // We assume that the block dimensions are correct.
  if (block->GetLevel() != this->Level)
    {
    vtkGenericWarningMacro("Wrong level.");
    return VTK_ERROR;
    }
  const int *ext;
  ext = block->GetBaseCellExtent();
  
  if (ext[0] < 0 || ext[2] < 0 || ext[4] < 0)
    {
    vtkGenericWarningMacro("I did not code this for negative extents.");
    }
  
  // Compute the extent from the left most cell value.
  xIdx = ext[0]/this->BlockDimensions[0];
  yIdx = ext[2]/this->BlockDimensions[1];
  zIdx = ext[4]/this->BlockDimensions[2];
  
  if (xIdx < this->GridExtent[0] || xIdx > this->GridExtent[1] ||
      yIdx < this->GridExtent[2] || yIdx > this->GridExtent[3] ||
      zIdx < this->GridExtent[4] || zIdx > this->GridExtent[5])
    {
    vtkGenericWarningMacro("Block index out of grid.");
    return VTK_ERROR;
    }
  
  xIdx -= this->GridExtent[0];
  yIdx -= this->GridExtent[2];
  zIdx -= this->GridExtent[4];
  int idx = xIdx+(yIdx+(zIdx*(this->GridExtent[3]-this->GridExtent[2]+1)))
                             *(this->GridExtent[1]-this->GridExtent[0]+1);

  if (this->Grid[idx])
    {
    vtkGenericWarningMacro("Overwriting block in grid");
    }
  this->Grid[idx] = block;

  return VTK_OK;
}

//----------------------------------------------------------------------------
vtkCTHFragmentConnectBlock* vtkCTHFragmentLevel::GetBlock(
  int xIdx, int yIdx, int zIdx)
{
  if (xIdx < this->GridExtent[0] || xIdx > this->GridExtent[1] ||
      yIdx < this->GridExtent[2] || yIdx > this->GridExtent[3] ||
      zIdx < this->GridExtent[4] || zIdx > this->GridExtent[5])
    {
    return 0;
    }
  xIdx -= this->GridExtent[0];
  yIdx -= this->GridExtent[2];
  zIdx -= this->GridExtent[4];
  int yInc = (this->GridExtent[1]-this->GridExtent[0]+1);
  int zInc = yInc * (this->GridExtent[3]-this->GridExtent[2]+1);
  return this->Grid[xIdx + yIdx*yInc + zIdx*zInc];
}

//============================================================================

//----------------------------------------------------------------------------
// A class to help traverse pointers acrosss block boundaries.
// I am keeping this class to a minimum because many are kept on the stack
// durring recursive connect algorithm.
class vtkCTHFragmentConnectIterator
{
public:
  vtkCTHFragmentConnectIterator() {this->Initialize();}
  ~vtkCTHFragmentConnectIterator() {this->Initialize();}
  void Initialize();

  vtkCTHFragmentConnectBlock* Block;
  unsigned char*              VolumeFractionPointer; // pointer to a specific voxel's data
  int*                        FragmentIdPointer; // pointer to a specific voxel
  int                         Index[3];      // relative to the CTH data set's index space
  int                         FlatIndex;     // relative to this block's index space
};
void vtkCTHFragmentConnectIterator::Initialize()
{
  this->Block = 0;
  this->VolumeFractionPointer = 0;
  this->FragmentIdPointer = 0;
  this->Index[0] = this->Index[1] = this->Index[2] = 0;
  this->FlatIndex=0;
}

//============================================================================

//----------------------------------------------------------------------------
// A simple first in last out ring container to hold the seeds for the 
// breadth first search.
// I am going to have the container allocate and delete its own iterators.
// It will simplify the converation from depth first to breadth first.
class vtkCTHFragmentConnectRingBuffer
{
public:
  vtkCTHFragmentConnectRingBuffer();
  ~vtkCTHFragmentConnectRingBuffer();

  void Push(vtkCTHFragmentConnectIterator* iterator);
  int Pop(vtkCTHFragmentConnectIterator* iterator);

  long GetSize() { return this->Size;}


private:

  vtkCTHFragmentConnectIterator* Ring;
  vtkCTHFragmentConnectIterator* End;
  long RingLength;
  // The first and last iterator added.
  vtkCTHFragmentConnectIterator* First;
  vtkCTHFragmentConnectIterator* Next;
  // I could do without this size, but it does not cost much.
  long Size;

  void GrowRing();
};

//----------------------------------------------------------------------------
vtkCTHFragmentConnectRingBuffer::vtkCTHFragmentConnectRingBuffer()
{
  int initialSize = 2000;
  this->Ring = new vtkCTHFragmentConnectIterator[initialSize];
  this->RingLength = initialSize;
  this->End = this->Ring + this->RingLength;
  this->First = 0;
  this->Next = this->Ring;
  this->Size = 0;
}

//----------------------------------------------------------------------------
vtkCTHFragmentConnectRingBuffer::~vtkCTHFragmentConnectRingBuffer()
{
  delete [] this->Ring;
  this->End = 0;
  this->RingLength = 0;
  this->Next = this->First = 0;
  this->Size = 0;
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnectRingBuffer::GrowRing()
{
  // Allocate a new ring.
  vtkCTHFragmentConnectIterator* newRing;
  int newRingLength = this->RingLength * 2;
  newRing = new vtkCTHFragmentConnectIterator[newRingLength*2];
  
  // Copy items into the new ring.
  int count = this->Size;
  vtkCTHFragmentConnectIterator* ptr1 = this->First;
  vtkCTHFragmentConnectIterator* ptr2 = newRing;
  while (count > 0)
    {
    *ptr2++ = *ptr1++;
    if (ptr1 == this->End)
      {
      ptr1 = this->Ring;
      }
    --count;
    }

  //cerr << "Grow ring buffer: " << newRingLength << endl;
  
  // Replace the ring.
  // Size remains the same.
  delete [] this->Ring;
  this->Ring = newRing;
  this->End = newRing + newRingLength;
  this->RingLength = newRingLength;
  this->First = newRing;
  this->Next = newRing + this->Size;
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnectRingBuffer::Push(vtkCTHFragmentConnectIterator* item)
{
  // Make the ring buffer larger if necessary.
  if (this->Size == this->RingLength)
    {
    this->GrowRing();
    }
  
  // Add the item.
  *(this->Next) = *item;
  // Special case for an empty ring.
  // We could initialize start to next to avoid this condition every push.
  if (this->Size == 0)
    {
    this->First = this->Next;
    }

  // Move the the next-available-slot pointer to the next space.
  ++this->Next;
  // End of the linear buffer moves back to the begining.
  // This makes it a ring.
  if (this->Next == this->End)
    {
    this->Next = this->Ring;
    }

  ++this->Size;
}

//----------------------------------------------------------------------------
int vtkCTHFragmentConnectRingBuffer::Pop(vtkCTHFragmentConnectIterator* item)
{
  if (this->Size == 0)
    {
    return 0;
    }
    
  *item = *(this->First);
  //this->First->Initialize();
  ++this->First;
  --this->Size;

  // End of the linear buffer moves back to the begining.
  // This makes it a ring.
  if (this->First == this->End)
    {
    this->First = this->Ring;
    }
    
  return 1;
}


//============================================================================



//----------------------------------------------------------------------------
// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
vtkCTHFragmentConnect::vtkCTHFragmentConnect()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();

  #ifdef vtkCTHFragmentConnectDEBUG
  int myProcId=this->Controller->GetLocalProcessId();
  this->MyPid=WritePidFile(this->Controller->GetCommunicator(),"cthfc.pid");
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment entering vtkCTHFragmentConnect is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // Pipeline
  // 0 output multi-block
  // 1 polydata, proc 0 fills with attributes, all others fill with empty polydata
  this->SetNumberOfOutputPorts(2);

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback( &vtkCTHFragmentConnect::SelectionModifiedCallback );
  this->SelectionObserver->SetClientData(this);

  // pv interface for material fraction
  this->MaterialArraySelection = vtkDataArraySelection::New();
  this->MaterialArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                             this->SelectionObserver );
  // pv interface for mass array
  this->MassArraySelection = vtkDataArraySelection::New();
  this->MassArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                             this->SelectionObserver );
  // pv interface for volume weighted averaged arrays
  this->VolumeWtdAvgArraySelection=vtkDataArraySelection::New();
  this->VolumeWtdAvgArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                                 this->SelectionObserver );
  // pv interface for mass weighted averaged arrays
  this->MassWtdAvgArraySelection=vtkDataArraySelection::New();
  this->MassWtdAvgArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                               this->SelectionObserver );
  // pv interface for summed arrays
  this->SummationArraySelection=vtkDataArraySelection::New();
  this->SummationArraySelection->AddObserver( vtkCommand::ModifiedEvent,
                                              this->SelectionObserver );

  this->OutputTableFileNameBase=0;
  this->WriteOutputTableFile=false;

  this->NumberOfInputBlocks = 0;
  this->InputBlocks = 0;
  this->GlobalOrigin[0]=this->GlobalOrigin[1]=this->GlobalOrigin[2]=0.0;
  this->RootSpacing[0]=this->RootSpacing[1]=this->RootSpacing[2]=1.0;

  this->FragmentId = 0;
  this->FragmentVolume = 0.0;
  this->FragmentVolumes = 0;
  this->FragmentMoment.resize(4,0.0);
  this->FragmentMoments = 0;
  this->FragmentAABBCenters=0;
  this->FragmentOBBs = 0;
  this->FragmentSplitGeometry=0;

  this->EquivalenceSet = new vtkCTHFragmentEquivalenceSet;
  this->LocalToGlobalOffsets = 0;
  this->TotalNumberOfRawFragments = 0;
  this->NumberOfResolvedFragments = 0;
  this->ResolvedFragmentCount=0;
  this->MaterialId=0;

  this->FaceNeighbors = new vtkCTHFragmentConnectIterator[32];

  this->CurrentFragmentMesh = 0;

  this->NVolumeWtdAvgs = 0;
  this->NToSum = 0;
  this->ComputeMoments=false;
  this->ComputeOBB=false;

  this->MaterialFractionThreshold = 0.5;
  this->scaledMaterialFractionThreshold = 127.5;
  this->UpperLoadingBound=1000000;

  this->Progress=0.0;
  this->ProgressMaterialInc=0.0;
  this->ProgressBlockInc=0.0;
  this->ProgressResolutionInc=0.0;
}

//----------------------------------------------------------------------------
vtkCTHFragmentConnect::~vtkCTHFragmentConnect()
{
  this->DeleteAllBlocks();
  this->Controller = 0;
  this->GlobalOrigin[0]=this->GlobalOrigin[1]=this->GlobalOrigin[2]=0.0;
  this->RootSpacing[0]=this->RootSpacing[1]=this->RootSpacing[2]=1.0;

  this->FragmentId = 0;
  this->FragmentVolume = 0.0;

  CheckAndReleaseVtkPointer(this->FragmentVolumes);
  CheckAndReleaseVtkPointer(this->FragmentMoments);
  CheckAndReleaseVtkPointer(this->FragmentAABBCenters);
  CheckAndReleaseVtkPointer(this->FragmentOBBs);
  CheckAndReleaseVtkPointer(this->FragmentSplitGeometry);
  ClearVectorOfVtkPointers(this->FragmentVolumeWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentMassWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentSums);

  delete this->EquivalenceSet;
  this->EquivalenceSet = 0;

  delete [] this->FaceNeighbors;
  this->FaceNeighbors = 0;

  // clean up PV interface
  this->MaterialArraySelection->RemoveObserver( this->SelectionObserver );
  this->MaterialArraySelection->Delete();
  this->MaterialArraySelection=0;
  //
  this->MassArraySelection->RemoveObserver( this->SelectionObserver );
  this->MassArraySelection->Delete();
  this->MassArraySelection=0;
  //
  this->VolumeWtdAvgArraySelection->RemoveObserver( this->SelectionObserver );
  this->VolumeWtdAvgArraySelection->Delete();
  this->VolumeWtdAvgArraySelection=0;
  //
  this->MassWtdAvgArraySelection->RemoveObserver( this->SelectionObserver );
  this->MassWtdAvgArraySelection->Delete();
  this->MassWtdAvgArraySelection=0;
  //
  this->SummationArraySelection->RemoveObserver( this->SelectionObserver );
  this->SummationArraySelection->Delete();
  this->SummationArraySelection=0;

  this->SelectionObserver->Delete();

  if (this->OutputTableFileNameBase!=0)
    {
    delete [] this->OutputTableFileNameBase;
    }
  this->Progress=0.0;
  this->ProgressMaterialInc=0.0;
  this->ProgressBlockInc=0.0;
  this->ProgressResolutionInc=0.0;
}

//----------------------------------------------------------------------------
// Called to delete all the block structures.
void vtkCTHFragmentConnect::DeleteAllBlocks()
{
  if (this->NumberOfInputBlocks == 0)
    {
    return;
    }

  // Ghost blocks
  int num = this->GhostBlocks.size();
  int ii;
  vtkCTHFragmentConnectBlock* block;
  for (ii = 0; ii < num; ++ii)
    {
    block = this->GhostBlocks[ii];
    delete block;
    }
  this->GhostBlocks.clear();

  // Normal Blocks
  for (ii = 0; ii < this->NumberOfInputBlocks; ++ii)
    {
    if (this->InputBlocks[ii])
      {
      delete this->InputBlocks[ii];
      this->InputBlocks[ii] = 0;
      }
    }
  if (this->InputBlocks)
    {
    delete [] this->InputBlocks;
    this->InputBlocks = 0;
    }
  this->NumberOfInputBlocks = 0;

  // levels
  int level, numLevels;
  numLevels = this->Levels.size();
  for (level = 0; level < numLevels; ++level)
    {
    if (this->Levels[level])
      {
      delete this->Levels[level];
      this->Levels[level] = 0;
      }
    }
}

//----------------------------------------------------------------------------
// Initialize blocks from multi block input.
int vtkCTHFragmentConnect::InitializeBlocks(
        vtkHierarchicalBoxDataSet* input,
        string &materialFractionArrayName,
        string &massArrayName,
        vector<string> &volumeWtdAvgArrayNames,
        vector<string> &massWtdAvgArrayNames,
        vector<string> &summedArrayNames,
        vector<string> &integratedArrayNames)
{
  int level;
  int numLevels = input->GetNumberOfLevels();
  vtkCTHFragmentConnectBlock* block;
  int myProc = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();

  // Just in case
  this->DeleteAllBlocks();

  // We need all blocks to share a common extent index.
  // We find a common origin as the minimum most point in level 0.
  // Since invalid ghost cells are removed, this should be correct.
  // However, it should not matter because any point on root grid that
  // keeps all indexes positive will do.
  this->ComputeOriginAndRootSpacing(input);

  // Create the array of blocks.
  this->NumberOfInputBlocks = this->GetNumberOfLocalBlocks(input);

  this->InputBlocks = new vtkCTHFragmentConnectBlock*[this->NumberOfInputBlocks];
  // Initilize to NULL.
  for (int blockId = 0; blockId < this->NumberOfInputBlocks; ++blockId)
    {
    this->InputBlocks[blockId] = 0;
    }

  // Initialize each block with the input image 
  // and global index coordinate system.
  int blockIndex = -1;
  this->Levels.resize(numLevels);
  for (level = 0; level < numLevels; ++level)
    {
    this->Levels[level] = new vtkCTHFragmentLevel;

    int cumulativeExt[6];
    cumulativeExt[0] = cumulativeExt[2] = cumulativeExt[4] = VTK_LARGE_INTEGER;
    cumulativeExt[1] = cumulativeExt[3] = cumulativeExt[5] = -VTK_LARGE_INTEGER;

    int numBlocks = input->GetNumberOfDataSets(level);
    for (int levelBlockId = 0; levelBlockId < numBlocks; ++levelBlockId)
      {
      vtkAMRBox box;
      vtkImageData* image = input->GetDataSet(level,levelBlockId,box);
      // TODO: We need to check the CELL_DATA and the correct volume fraction array.

      if (image)
        {
        block = this->InputBlocks[++blockIndex] = new vtkCTHFragmentConnectBlock;
        // Do we really need the block to know its id? 
        // We use it to find neighbors.  We should save pointers
        // directly in neighbor array. We also use it for debugging.
        block->Initialize(blockIndex,
                          image,
                          level,
                          this->GlobalOrigin,
                          this->RootSpacing,
                          materialFractionArrayName,
                          massArrayName,
                          volumeWtdAvgArrayNames,
                          massWtdAvgArrayNames,
                          summedArrayNames,
                          integratedArrayNames);
        // For debugging:
        block->LevelBlockId = levelBlockId;

        // Collect information about the blocks in this level.
        const int *ext;
        ext = block->GetBaseCellExtent();
        // We need the cumulative extent to determine the grid extent.
        if (cumulativeExt[0] > ext[0]) {cumulativeExt[0] = ext[0];}
        if (cumulativeExt[1] < ext[1]) {cumulativeExt[1] = ext[1];}
        if (cumulativeExt[2] > ext[2]) {cumulativeExt[2] = ext[2];}
        if (cumulativeExt[3] < ext[3]) {cumulativeExt[3] = ext[3];}
        if (cumulativeExt[4] > ext[4]) {cumulativeExt[4] = ext[4];}
        if (cumulativeExt[5] < ext[5]) {cumulativeExt[5] = ext[5];}
        }
      }

    // Expand the grid extent by 1 in all directions to accomadate ghost blocks.
    // We might have a problem with level 0 here since blockDims is not global yet.
    cumulativeExt[0] = cumulativeExt[0] / this->StandardBlockDimensions[0];
    cumulativeExt[1] = cumulativeExt[1] / this->StandardBlockDimensions[0];
    cumulativeExt[2] = cumulativeExt[2] / this->StandardBlockDimensions[0];
    cumulativeExt[3] = cumulativeExt[3] / this->StandardBlockDimensions[0];
    cumulativeExt[4] = cumulativeExt[4] / this->StandardBlockDimensions[0];
    cumulativeExt[5] = cumulativeExt[5] / this->StandardBlockDimensions[0];

    // Expand extent to cover all processes.
    if (myProc > 0)
      {
      this->Controller->Send(cumulativeExt, 6, 0, 212130);
      this->Controller->Receive(cumulativeExt, 6, 0, 212131);
      }
    else
      {
      int tmp[6];
      for (int ii = 1; ii < numProcs; ++ii)
        {
        this->Controller->Receive(tmp, 6, ii, 212130);
        if (cumulativeExt[0] > tmp[0]) {cumulativeExt[0] = tmp[0];}
        if (cumulativeExt[1] < tmp[1]) {cumulativeExt[1] = tmp[1];}
        if (cumulativeExt[2] > tmp[2]) {cumulativeExt[2] = tmp[2];}
        if (cumulativeExt[3] < tmp[3]) {cumulativeExt[3] = tmp[3];}
        if (cumulativeExt[4] > tmp[4]) {cumulativeExt[4] = tmp[4];}
        if (cumulativeExt[5] < tmp[5]) {cumulativeExt[5] = tmp[5];}
        }
      // Redistribute the global grid extent
      for (int ii = 1; ii < numProcs; ++ii)
        {
        this->Controller->Send(cumulativeExt, 6, ii, 212131);
        }
      }

    this->Levels[level]->Initialize(cumulativeExt, level);
    this->Levels[level]->SetStandardBlockDimensions(this->StandardBlockDimensions);
    }

  // Now add all the blocks to the level structures.
  int ii;
  for (ii = 0; ii < this->NumberOfInputBlocks; ++ii)
    {
    block = this->InputBlocks[ii];
    this->AddBlock(block);
    }

  //cerr << "start ghost blocks\n" << endl;

  // Broadcast all of the block meta data to all processes.
  // Setup ghost layer blocks.
  // Remove this until the local version is working again.....
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
    this->ShareGhostBlocks();
    }

  return VTK_OK;
}



//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::CheckLevelsForNeighbors(
  vtkCTHFragmentConnectBlock* block)
{
//   int exts[6] = {64, 79, 112, 127, 96, 111};
//   bool match = true;
//   for (int idx=0; idx<6; idx++)
//     {
//     if (block->GetBaseCellExtent()[idx] != exts[idx])
//       {
//       match = false;
//       break;
//       }
//     }
//   if (match)
//     {
//     cout << "Stop here" << endl;
//     }
  vtkstd::vector<vtkCTHFragmentConnectBlock*> neighbors;
  vtkCTHFragmentConnectBlock* neighbor;
  int blockIndex[3];

  // Extract the index from the block extent.
  const int *ext;
  ext = block->GetBaseCellExtent();
  blockIndex[0] = ext[0] / this->StandardBlockDimensions[0];
  blockIndex[1] = ext[2] / this->StandardBlockDimensions[1];
  blockIndex[2] = ext[4] / this->StandardBlockDimensions[2];

//   if (blockIndex[0] == 13 && blockIndex[1] == 14 && blockIndex[2] == 10)
//     {
//     cerr << "Debug.\n";
//     }

  for (int axis = 0; axis < 3; ++axis)
    {
    // The purpose for passing a list into the method is no longer.
    // We could simplify this method.
    if (ext[2*axis] == blockIndex[axis] * this->StandardBlockDimensions[axis])
      { // I had trouble with ghost blocks that did not span the 
      // entire standard block.  They had inappropriate neighbors.
      this->FindFaceNeighbors(block->GetLevel(), blockIndex, 
                              axis, 0, &neighbors);
      for (unsigned int ii = 0; ii < neighbors.size(); ++ii)
        {
        neighbor = neighbors[ii];
        block->AddNeighbor(neighbor, axis, 0);
        neighbor->AddNeighbor(block, axis, 1);
        }
      }

    if (ext[2*axis + 1] == ((blockIndex[axis]+1) * this->StandardBlockDimensions[axis])-1)
      { // I had trouble with ghost blocks that did not span the 
      // entire standard block.  They had inappropriate neighbors.
      this->FindFaceNeighbors(block->GetLevel(), blockIndex,
                             axis, 1, &neighbors);
      for (unsigned int ii = 0; ii < neighbors.size(); ++ii)
        {
        neighbor = neighbors[ii];
        block->AddNeighbor(neighbor, axis, 1);
        neighbor->AddNeighbor(block, axis, 0);
        }
      }
    }
}



//----------------------------------------------------------------------------
// trying to make a single method to find neioghbors that can be used
// to both add a new block and calculate what ghost extent we need.
// Returns 1 if at least one non-ghost neighbor exists.
int vtkCTHFragmentConnect::FindFaceNeighbors(
  unsigned int blockLevel,
  int blockIndex[3],
  int faceAxis,
  int faceMaxFlag,
  vtkstd::vector<vtkCTHFragmentConnectBlock*> *result)
{
  int retVal = 0;
  vtkCTHFragmentConnectBlock* neighbor;
  int idx[3];
  int tmp[3];
  int levelDifference;
  int p2;
  int axis1 = (faceAxis+1)%3;
  int axis2 = (faceAxis+2)%3;
  const int* ext;
  
  // Stuff to compare extent with standard block to make sure neighbors
  // actually touch.  Ghost cells can be a part of a standard block.
  int neighborExtIdx = 2*faceAxis;
  if ( ! faceMaxFlag)
    {
    ++neighborExtIdx;
    }
  int boundaryVoxelIdx;

  result->clear();

  for (unsigned int level = 0; level < this->Levels.size(); ++level)
    {
    // We convert between cell index and point index to do level conversion.
    idx[faceAxis] = blockIndex[faceAxis] + faceMaxFlag;
    idx[axis1] = blockIndex[axis1];
    idx[axis2] = blockIndex[axis2];

    if (level <= blockLevel)
      { // Neighbor block is larger than reference block.
      levelDifference = blockLevel - level;
      // If the point location is divisible by the power of two factor then the
      // face lies on the level grids boundary face and we might have a neighbor.
      if ((idx[faceAxis] >> levelDifference) << levelDifference == idx[faceAxis])
        { // Face lies on a boundary
        // Convert index to working level and back to cell index.
        tmp[0] = idx[0] >> levelDifference;
        tmp[1] = idx[1] >> levelDifference;
        tmp[2] = idx[2] >> levelDifference;
        if ( ! faceMaxFlag)
          { // min face.  Neighbor one to "left".
          tmp[faceAxis] -= 1;
          // But boundary voxel is to the "right".
          boundaryVoxelIdx = ((tmp[faceAxis]+1) * this->StandardBlockDimensions[faceAxis]) -1;
          }
        else
          { // This neighbor is to right,  boundary to left / min
          boundaryVoxelIdx = tmp[faceAxis] * this->StandardBlockDimensions[faceAxis];
          }
        neighbor = this->Levels[level]->GetBlock(tmp[0], tmp[1], tmp[2]);
        if (neighbor)
          {
          // Since ghost blocks do not extend to the edge of the grid, 
          // we need to check that the extent.touches the edge.
          ext = neighbor->GetBaseCellExtent();
          if (ext[neighborExtIdx] == boundaryVoxelIdx)
            {
            // Useful for compute required ghost extents.
            if ( ! neighbor->GetGhostFlag())
              {
              retVal = 1;
              }
            result->push_back(neighbor);
            }
          }
        }
      }
    else
      { // Neighbor block is smaller than reference block.
      // All faces will be on boundary.
      // Convert index to working level.
      levelDifference =  level - blockLevel;
      p2 = 1 << levelDifference;
      // Point index of face for easier level conversion.
      idx[0] = idx[0] << levelDifference;
      idx[1] = idx[1] << levelDifference;
      idx[2] = idx[2] << levelDifference;
      if ( ! faceMaxFlag)
        { // min face.  Neighbor one to "left".
        idx[faceAxis] -= 1;
        // But boundary voxel is to the "right".
        boundaryVoxelIdx = ((idx[faceAxis]+1) * this->StandardBlockDimensions[faceAxis]) -1;
        }
      else
        { // This neighbor is to right,  boundary to left / min
        boundaryVoxelIdx = idx[faceAxis] * this->StandardBlockDimensions[faceAxis];
        }

      // Loop over all possible blocks touching this face.
      tmp[faceAxis] = idx[faceAxis];
      for (int ii = 0; ii < p2; ++ii)
        {
        tmp[axis1] = idx[axis1] + ii;
        for (int jj = 0; jj < p2; ++jj)
          {
          tmp[axis2] = idx[axis2] + jj;
          neighbor = this->Levels[level]->GetBlock(tmp[0], tmp[1], tmp[2]);
          if (neighbor)
            {
            // Since ghost blocks do not extend to the edge of the grid, 
            // we need to check that the extent.touches the edge.
            ext = neighbor->GetBaseCellExtent();
            if (ext[neighborExtIdx] == boundaryVoxelIdx)
              {
              // Useful for compute required ghost extents.
              if ( ! neighbor->GetGhostFlag())
                {
                retVal = 1;
                }
              result->push_back(neighbor);
              }
            }
          }
        }
      }
    }
  return retVal;
}


//----------------------------------------------------------------------------
// We need ghost cells for edges and corners as well as faces.
// neighborDirection is used to specify a face, edge or corner.
// Using a 2x2x2 cube center at origin: (-1,-1,-1), (-1,-1,1) ... are corners.
// (1,1,0) is an edge, and (-1,0,0) is a face.
// Returns 1 if the neighbor exists.
int vtkCTHFragmentConnect::HasNeighbor(
  unsigned int blockLevel,
  int blockIndex[3],
  int neighborDirection[3])
{
  vtkCTHFragmentConnectBlock* neighbor;
  int idx[3];
  int levelDifference;

  // Check all levels.
  for (unsigned int level = 0; level < this->Levels.size(); ++level)
    {
    // We convert between cell index and point index to do level conversion.
    if (level <= blockLevel)
      { // Neighbor block is larger than reference block.
      levelDifference = blockLevel - level;
      // Check to see that all non zero axes lie on a grid boundary.
      int bdFlag = 1; // Is the face, edge or corner on a grid boundary?
      for (int ii = 0; ii < 3; ++ii)
        {
        switch (neighborDirection[ii])
          {
          case -1:
            idx[ii] = (blockIndex[ii] >> levelDifference) - 1;
            if ((blockIndex[ii] >> levelDifference) << levelDifference != blockIndex[ii])
              {
              bdFlag = 0;
              }
            break;
          case 1:
            idx[ii] = (blockIndex[ii] >> levelDifference) + 1;
            if (idx[ii] << levelDifference != blockIndex[ii] + 1)
              {
              bdFlag = 0;
              }
            break;
          case 0:
            idx[ii] = (blockIndex[ii] >> levelDifference);
          }
        }
      if (bdFlag)
        { // Neighbor primative direction is on grid boundary.
        neighbor = this->Levels[level]->GetBlock(idx[0], idx[1], idx[2]);
        if (neighbor && ! neighbor->GetGhostFlag())
          { // We found one.  Thats all we need.
          return 1;
          }
        }
      }
    else
      { // Neighbor block is smaller than reference block.
      // All faces will be on boundary.
      // !!! We have to loop over all axes whose direction component is 0.
      // Convert index to working level.
      levelDifference =  level - blockLevel;
      int mins[3];
      int maxs[3];
      int ix, iy, iz;

      // Compupte the range of potential neighbor indexes.
      for (int ii = 0; ii < 3; ++ii)
        {
        switch (neighborDirection[ii])
          {
          case -1:
            mins[ii] = maxs[ii] = (blockIndex[ii] << levelDifference) - 1;
            break;
          case 0:
            mins[ii] = (blockIndex[ii] << levelDifference);
            maxs[ii] = mins[ii] + (1 << levelDifference) - 1;
            break;
          case 1:
            mins[ii] = maxs[ii] = (blockIndex[ii]+1) << levelDifference;
          }
        }
      // Now do the loops.
      for (ix = mins[0]; ix <= maxs[0]; ++ix)
        {
        for (iy = mins[1]; iy <= maxs[1]; ++iy)
          {
          for (iz = mins[2]; iz <= maxs[2]; ++iz)
            {
            neighbor = this->Levels[level]->GetBlock(ix, iy, iz);
            if (neighbor && ! neighbor->GetGhostFlag())
              { // We found one.  Thats all we need.
              return 1;
              }
            }
          }
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
// count the number of local(wrt this proc) blocks.
int vtkCTHFragmentConnect::GetNumberOfLocalBlocks( 
                              vtkHierarchicalBoxDataSet *hbds)
{
  vtkCompositeDataIterator *it=hbds->NewIterator();
  it->InitTraversal();
  it->VisitOnlyLeavesOn();
  it->SkipEmptyNodesOn();
  int nLocalBlocks=0;
  while ( !it->IsDoneWithTraversal() )
    {
    ++nLocalBlocks;
    it->GoToNextItem();
    }
  it->Delete();
  return nLocalBlocks;
}

//----------------------------------------------------------------------------
// All processes must share a common origin.
// Returns the total number of blocks in all levels (this process only).
// This computes:  GlobalOrigin, RootSpacing and StandardBlockDimensions.
// StandardBlockDimensions are the size of blocks without the extra
// overlap layer put on by spyplot format.
// RootSpacing is the spacing that blocks in level 0 would have.
// GlobalOrigin is choosen so that there are no negative extents and
// base extents (without overlap/ghost buffer) lie on grid 
// (i.e.) the min base extent must be a multiple of the standardBlockDimesions.
void vtkCTHFragmentConnect::ComputeOriginAndRootSpacing(
  vtkHierarchicalBoxDataSet* input)
{
  // extract information which the reader has exported to 
  // the root node of the box hierarchy. All procs in the filter have
  // the root thus no comm needed.
  vtkFieldData *inputFd = input->GetFieldData();
  vtkDoubleArray *globalBoundsDa
    = dynamic_cast<vtkDoubleArray*>(inputFd->GetArray("GlobalBounds"));
  vtkIntArray *standardBoxSizeIa
    = dynamic_cast<vtkIntArray*>(inputFd->GetArray("GlobalBoxSize"));
  vtkIntArray *minLevelIa
    = dynamic_cast<vtkIntArray*>(inputFd->GetArray("MinLevel"));
  vtkDoubleArray *minLevelSpacingDa
    = dynamic_cast<vtkDoubleArray*>(inputFd->GetArray("MinLevelSpacing"));

  // if these are not present then the dataset 
  // is malformed.
  assert( "Incomplete FieldData on filter input." &&
          globalBoundsDa &&
          standardBoxSizeIa &&
          minLevelIa &&
          minLevelSpacingDa );

  // global bounds
  double *pd=globalBoundsDa->GetPointer(0);
  double globalBounds[6];
  for (int q=0; q<6; ++q)
    {
    globalBounds[q]=pd[q];
    }
  // standard block size
  int *pi=standardBoxSizeIa->GetPointer(0);
  for (int q=0; q<3; ++q)
    {
    this->StandardBlockDimensions[q]=pi[q]-2;
    }
  // min level in use
  int minLevel = minLevelIa->GetValue(0);
  // min level spacing
  double minLevelSpacing[3];
  pd=minLevelSpacingDa->GetPointer(0);
  for (int q=0; q<3; ++q)
    {
    minLevelSpacing[q]=pd[q];
    }
  // compute spacing of level 0/root
  int coarsen = 1 << minLevel;
  for (int q=0; q<3; ++q)
    {
    this->RootSpacing[q]=minLevelSpacing[q]*coarsen;
    }
  // set the data set origin from the global bounds
  for (int q=0; q<3; ++q)
    {
    this->GlobalOrigin[q]=globalBounds[2*q];
    }

  //TODO the following code doesn't work on small(wrt blocks) datasets
  // it looks like 3x3x3 is the smallest that will work...

  // TODO if I understod what is going on below better I could
  // probably get the requisite information from the reader.
  // From above we will keep globalBounds, RootSpacing and StandardBlockDimensions, 
  // and recompute the rest below so as not to disturb the process.

  // TODO why is globalBounds insufficient for the globalOrigin??
  // in the examples I have looked at they come out to be the same.
  // TODO do we have a case specific where they differ??
  // TODO why lowestOrigin is necessarilly from the block closest to 
  // the data set origin?? We just grab the first non empty block but does that
  // guarantee closest to ds origin?? I don't think so.
}
//----------------------------------------------------------------------------
// All processes must share a common origin.
// Returns the total number of blocks in all levels (this process only).
// This computes:  GlobalOrigin, RootSpacing and StandardBlockDimensions.
// StandardBlockDimensions are the size of blocks without the extra
// overlap layer put on by spyplot format.
// RootSpacing is the spacing that blocks in level 0 would have.
// GlobalOrigin is choosen so that there are no negative extents and
// base extents (without overlap/ghost buffer) lie on grid 
// (i.e.) the min base extent must be a multiple of the standardBlockDimesions.
int vtkCTHFragmentConnect::ComputeOriginAndRootSpacingOld(
  vtkHierarchicalBoxDataSet* input)
{
  int numLevels = input->GetNumberOfLevels();
  int numBlocks;
  int blockId;
  int totalNumberOfBlocksInThisProcess = 0;
  
  // This is a big pain.
  // We have to look through all blocks to get a minimum root origin.
  // The origin must be choosen so there are no negative indexes.
  // Negative indexes would require the use floor or ceiling function instead
  // of simple truncation.
  //  The origin must also lie on the root grid.
  // The big pain is finding the correct origin when we do not know which
  // blocks have ghost layers.  The Spyplot reader strips
  // ghost layers from outside blocks.

  // Overall processes:
  // Find the largest of all block dimensions to compute standard dimensions.
  // Save the largest block information.
  // Find the overall bounds of the data set.
  // Find one of the lowest level blocks to compute origin.

  int    lowestLevel = 0;
  double lowestSpacing[3];
  double lowestOrigin[3];
  int    lowestDims[3];
  int    largestLevel = 0;
  double largestOrigin[3];
  double largestSpacing[3];
  int    largestDims[3];
  int    largestNumCells;
  
  double globalBounds[6];

  // Temporary variables.
  double spacing[3];
  double bounds[6];
  int cellDims[3];
  int numCells;
  int ext[6];

  largestNumCells = 0;
  globalBounds[0] = globalBounds[2] = globalBounds[4] = VTK_LARGE_FLOAT;
  globalBounds[1] = globalBounds[3] = globalBounds[5] = -VTK_LARGE_FLOAT;
  lowestSpacing[0] = lowestSpacing[1] = lowestSpacing[2] = 0.0;

  // Add each block.
  for (int level = 0; level < numLevels; ++level)
    {
    numBlocks = input->GetNumberOfDataSets(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
      {
      vtkAMRBox box;
      vtkImageData* image = input->GetDataSet(level,blockId,box);
      if (image)
        {
        ++totalNumberOfBlocksInThisProcess;
        image->GetBounds(bounds);
        // Compute globalBounds.
        if (globalBounds[0] > bounds[0]) {globalBounds[0] = bounds[0];}
        if (globalBounds[1] < bounds[1]) {globalBounds[1] = bounds[1];}
        if (globalBounds[2] > bounds[2]) {globalBounds[2] = bounds[2];}
        if (globalBounds[3] < bounds[3]) {globalBounds[3] = bounds[3];}
        if (globalBounds[4] > bounds[4]) {globalBounds[4] = bounds[4];}
        if (globalBounds[5] < bounds[5]) {globalBounds[5] = bounds[5];}
        image->GetExtent(ext);
        cellDims[0] = ext[1]-ext[0]; // ext is point extent.
        cellDims[1] = ext[3]-ext[2];
        cellDims[2] = ext[5]-ext[4];
        numCells = cellDims[0] * cellDims[1] * cellDims[2];
        // Compute standard block dimensions.
        if (numCells > largestNumCells)
          {
          largestDims[0] = cellDims[0];
          largestDims[1] = cellDims[1];
          largestDims[2] = cellDims[2];
          largestNumCells = numCells;
          image->GetOrigin(largestOrigin);
          image->GetSpacing(largestSpacing);
          largestLevel = level;
          }
        // Find the lowest level block.
        image->GetSpacing(spacing);
        if (spacing[0] > lowestSpacing[0]) // Only test axis 0. Assume others agree.
          { // This is the lowest level block we have encountered.
          image->GetSpacing(lowestSpacing);
          lowestLevel = level;
          image->GetOrigin(lowestOrigin);
          lowestDims[0] = cellDims[0];
          lowestDims[1] = cellDims[1];
          lowestDims[2] = cellDims[2];
          }
        }
      }
    }

  // Send the results to process 0 that will choose the origin ...
  double dMsg[18];
  int    iMsg[9];
  int myId = 0;
  int numProcs = 1;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (controller)
    {
    numProcs = controller->GetNumberOfProcesses();
    myId = controller->GetLocalProcessId();
    if (myId > 0)
      { // Send to process 0.
      iMsg[0] = lowestLevel;
      iMsg[1] = largestLevel;
      iMsg[2] = largestNumCells;
      for (int ii= 0; ii < 3; ++ii)
        {
        iMsg[3+ii] = lowestDims[ii];
        iMsg[6+ii] = largestDims[ii];
        dMsg[ii]   = lowestSpacing[ii];
        dMsg[3+ii] = lowestOrigin[ii];
        dMsg[6+ii] = largestOrigin[ii];
        dMsg[9+ii] = largestSpacing[ii];
        dMsg[12+ii] = globalBounds[ii];
        dMsg[15+ii] = globalBounds[ii+3];
        }
      controller->Send(iMsg, 9, 0, 8973432);
      controller->Send(dMsg, 15, 0, 8973432);
      }
    else
      {
      // Collect results from all processes.
      for (int id = 1; id < numProcs; ++id)
        {
        controller->Receive(iMsg, 9, id, 8973432);
        controller->Receive(dMsg, 18, id, 8973432);
        numCells = iMsg[2];
        cellDims[0] = iMsg[6];
        cellDims[1] = iMsg[7];
        cellDims[2] = iMsg[8];
        if (numCells > largestNumCells)
          {
          largestDims[0] = cellDims[0];
          largestDims[1] = cellDims[1];
          largestDims[2] = cellDims[2];
          largestNumCells = numCells;
          largestOrigin[0] = dMsg[6];
          largestOrigin[1] = dMsg[7];
          largestOrigin[2] = dMsg[8];
          largestSpacing[0] = dMsg[9];
          largestSpacing[1] = dMsg[10];
          largestSpacing[2] = dMsg[11];
          largestLevel = iMsg[1];
          }
        // Find the lowest level block.
        spacing[0] = dMsg[0];
        spacing[1] = dMsg[1];
        spacing[2] = dMsg[2];
        if (spacing[0] > lowestSpacing[0]) // Only test axis 0. Assume others agree.
          { // This is the lowest level block we have encountered.
          lowestSpacing[0] = spacing[0];
          lowestSpacing[1] = spacing[1];
          lowestSpacing[2] = spacing[2];
          lowestLevel = iMsg[0];
          lowestOrigin[0] = dMsg[3];
          lowestOrigin[1] = dMsg[4];
          lowestOrigin[2] = dMsg[5];
          lowestDims[0] = iMsg[6];
          lowestDims[1] = iMsg[7];
          lowestDims[2] = iMsg[8];
          }
        if (globalBounds[0] > dMsg[9])  {globalBounds[0] = dMsg[9];}
        if (globalBounds[1] < dMsg[10]) {globalBounds[1] = dMsg[10];}
        if (globalBounds[2] > dMsg[11]) {globalBounds[2] = dMsg[11];}
        if (globalBounds[3] < dMsg[12]) {globalBounds[3] = dMsg[12];}
        if (globalBounds[4] > dMsg[13]) {globalBounds[4] = dMsg[13];}
        if (globalBounds[5] < dMsg[14]) {globalBounds[5] = dMsg[14];}
        }
      }
    }

  if (myId == 0)
    {
    this->StandardBlockDimensions[0] = largestDims[0]-2;
    this->StandardBlockDimensions[1] = largestDims[1]-2;
    this->StandardBlockDimensions[2] = largestDims[2]-2;
    this->RootSpacing[0] = lowestSpacing[0] * (1 << (lowestLevel));
    this->RootSpacing[1] = lowestSpacing[1] * (1 << (lowestLevel));
    this->RootSpacing[2] = lowestSpacing[2] * (1 << (lowestLevel));
    // Find the grid for the largest block.  We assume this block has the
    // extra ghost layers.
    largestOrigin[0] = largestOrigin[0] + largestSpacing[0];
    largestOrigin[1] = largestOrigin[1] + largestSpacing[1];
    largestOrigin[2] = largestOrigin[2] + largestSpacing[2];
    // Convert to the spacing of the blocks.
    largestSpacing[0] *= this->StandardBlockDimensions[0];
    largestSpacing[1] *= this->StandardBlockDimensions[1];
    largestSpacing[2] *= this->StandardBlockDimensions[2];
    // Find the point on the grid closest to the lowest level origin.
    // We do not know if this lowest level block has its ghost layers.
    // Even if the dims are one less that standard, which side is missing
    // the ghost layer!
    int idx[3];
    idx[0] = (int)(floor(0.5 + (lowestOrigin[0]-largestOrigin[0]) / largestSpacing[0]));
    idx[1] = (int)(floor(0.5 + (lowestOrigin[1]-largestOrigin[1]) / largestSpacing[1]));
    idx[2] = (int)(floor(0.5 + (lowestOrigin[2]-largestOrigin[2]) / largestSpacing[2]));
    lowestOrigin[0] = largestOrigin[0] + (double)(idx[0])*largestSpacing[0];
    lowestOrigin[1] = largestOrigin[1] + (double)(idx[1])*largestSpacing[1];
    lowestOrigin[2] = largestOrigin[2] + (double)(idx[2])*largestSpacing[2];
    // OK.  Now we have the grid for the lowest level that has a block.
    // Change the grid to be of the blocks.
    lowestSpacing[0] *= this->StandardBlockDimensions[0];
    lowestSpacing[1] *= this->StandardBlockDimensions[1];
    lowestSpacing[2] *= this->StandardBlockDimensions[2];
    
    // Change the origin so that all indexes will be positive.
    idx[0] = (int)(floor((globalBounds[0]-lowestOrigin[0]) / lowestSpacing[0]));
    idx[1] = (int)(floor((globalBounds[2]-lowestOrigin[1]) / lowestSpacing[1]));
    idx[2] = (int)(floor((globalBounds[4]-lowestOrigin[2]) / lowestSpacing[2]));
    this->GlobalOrigin[0] = lowestOrigin[0] + (double)(idx[0])*lowestSpacing[0];
    this->GlobalOrigin[1] = lowestOrigin[1] + (double)(idx[1])*lowestSpacing[1];
    this->GlobalOrigin[2] = lowestOrigin[2] + (double)(idx[2])*lowestSpacing[2];
    
    // Now send these to all the other processes and we are done!
    for (int ii = 0; ii < 3; ++ii)
      {
      dMsg[ii] = this->GlobalOrigin[ii];
      dMsg[ii+3] = this->RootSpacing[ii];
      dMsg[ii+6] = (double)(this->StandardBlockDimensions[ii]);
      }
    for (int ii = 1; ii < numProcs; ++ii)
      {
      controller->Send(dMsg, 9, ii, 8973439);
      }
    }
  else
    {
    controller->Receive(dMsg, 9, 0, 8973439);
    for (int ii = 0; ii < 3; ++ii)
      {
      this->GlobalOrigin[ii] = dMsg[ii];
      this->RootSpacing[ii] = dMsg[ii+3];
      this->StandardBlockDimensions[ii] = (int)(dMsg[ii+6]);
      }
    }

  return totalNumberOfBlocksInThisProcess;
}

//----------------------------------------------------------------------------
// This method creates ghost layer blocks across multiple processes.
void vtkCTHFragmentConnect::ShareGhostBlocks()
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myProc = this->Controller->GetLocalProcessId();
  vtkCommunicator* com  = this->Controller->GetCommunicator();

  // Bad things can happen if not all processes call 
  // MPI_Alltoallv this at the same time. (mpich)
  this->Controller->Barrier();
  
  // First share the number of blocks.
  int *blocksPerProcess = new int[numProcs];
  com->AllGather(&(this->NumberOfInputBlocks), blocksPerProcess, 1);
  //MPI_Allgather(&(this->NumberOfInputBlocks), 1, MPI_INT,
  //blocksPerProcess, 1, MPI_INT,
  //*com->GetMPIComm()->GetHandle());
  
  // Share the levels and extents of all blocks.
  // First, setup the count and displacement arrays required by AllGatherV.
  int totalNumberOfBlocks = 0;
  int *sendCounts = new int[numProcs];
  vtkIdType *recvCounts = new vtkIdType[numProcs];
  vtkIdType *displacements = new vtkIdType[numProcs];
  for (int ii = 0; ii < numProcs; ++ii)
    {
    displacements[ii] = totalNumberOfBlocks * 7;
    recvCounts[ii] = blocksPerProcess[ii] * 7;
    totalNumberOfBlocks += blocksPerProcess[ii];
    }

  // Setup the array to send to the other processes.
  int *localBlockInfo = new int[this->NumberOfInputBlocks*7];
  for (int ii = 0; ii < this->NumberOfInputBlocks; ++ii)
    {
    localBlockInfo[ii*7] = this->InputBlocks[ii]->GetLevel();
    const int *ext;
    // Lets do just the cell extent without overlap.
    // Edges and corners or overlap can cause problems when
    // neighbors are a different level.
    ext = this->InputBlocks[ii]->GetBaseCellExtent();
    for (int jj = 0; jj < 6; ++jj)
      {
      localBlockInfo[ii*7+1 + jj] = ext[jj];
      }
    }
  // Allocate the memory to receive the gathered extents.
  int *gatheredBlockInfo = new int[totalNumberOfBlocks*7];

  // TODO: check for errors ...
  com->AllGatherV(localBlockInfo, gatheredBlockInfo,
                  this->NumberOfInputBlocks*7, recvCounts, 
                  displacements);
  //MPI_Allgatherv((void*)localBlockInfo, this->NumberOfInputBlocks*7, MPI_INT, 
  //(void*)gatheredBlockInfo, recvCounts, displacements, 
  //MPI_INT, *com->GetMPIComm()->GetHandle());

  this->ComputeAndDistributeGhostBlocks(blocksPerProcess,
                                        gatheredBlockInfo,
                                        myProc,
                                        numProcs);
  // Send:
  // Process, extent, data,
  // ...
  // Receive:
  // Process, extent
  // ...


    /*

  // Compute the blocks that need to be sent between all processes.
  //this->ComputeCounts(blocksPerProcess, gatheredBlockInfo, numProcs, myId, 
  //                    sendCounts, recvCounts);
  

    
  // Now get the actual data
  int MPI_Alltoallv(void *sbuf, int *scounts, int *sdisps, MPI_Datatype sdtype, 
              void *rbuf, int *rcounts, int *rdisps, MPI_Datatype rdtype, 
              MPI_Comm comm)   
}
  
  int MPI_Allgatherv ( void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                     void *recvbuf, int *recvcounts, int *displs, 
                    MPI_Datatype recvtype, MPI_Comm comm )
*/
  
  // Determine neighbors we need to send and neighbors to receive.
  
  // AllToAllV.
  
  // Create ghost layer blocks.
  
  delete [] blocksPerProcess;
  delete [] sendCounts;
  delete [] recvCounts;
  delete [] displacements;
  delete [] localBlockInfo;
  delete [] gatheredBlockInfo;
}


//----------------------------------------------------------------------------
// Loop over all blocks from other processes.  Find all local blocks
// that touch the block.
void vtkCTHFragmentConnect::ComputeAndDistributeGhostBlocks(
  int *numBlocksInProc,
  int* blockMetaData,
  int myProc,
  int numProcs)
{
  int requestMsg[8];
  int *ext;
  int bufSize = 0;
  unsigned char* buf = 0;
  int dataSize;
  vtkCTHFragmentConnectBlock* ghostBlock;

  // Loop through the other processes.
  int* blockMetaDataPtr = blockMetaData;
  for (int otherProc = 0; otherProc < numProcs; ++otherProc)
    {
    if (otherProc == myProc)
      {
      this->HandleGhostBlockRequests();
      // Skip the metat data for this process.
      blockMetaDataPtr += + 7*numBlocksInProc[myProc];
      }
    else
      {
      // Loop through the extents in the remote process.
      for (int id = 0;  id < numBlocksInProc[otherProc]; ++id)
        {
        // Request message is (requesting process, block id, required extent)
        requestMsg[0] = myProc;
        requestMsg[1] = id;
        ext = requestMsg + 2;
        // Block meta data is level and base-cell-extent.
        int ghostBlockLevel = blockMetaDataPtr[0];
        int *ghostBlockExt = blockMetaDataPtr+1;

        if (this->ComputeRequiredGhostExtent(ghostBlockLevel, ghostBlockExt, ext))
          {
          this->Controller->Send(requestMsg, 8, otherProc, 708923);
          // Now receive the ghost block.
          dataSize = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
          if (bufSize < dataSize) 
            {
            if (buf) { delete [] buf;}
            buf = new unsigned char[dataSize];
            bufSize = dataSize;
            }
          this->Controller->Receive(buf, dataSize, otherProc, 433240);
          // Make the ghost block and add it to the grid.
          ghostBlock = new vtkCTHFragmentConnectBlock;
          ghostBlock->InitializeGhostLayer(buf, ext, ghostBlockLevel, 
                                           this->GlobalOrigin, this->RootSpacing,
                                           otherProc, id);
          // Save for deleting.
          this->GhostBlocks.push_back(ghostBlock);
          // Add to grid and connect up neighbors.
          this->AddBlock(ghostBlock);
          }
        // Move to the next block.
        blockMetaDataPtr += 7;
        }
      // Send the message that indicates we have no more requests.
      requestMsg[0] = myProc;
      requestMsg[1] = -1;
      this->Controller->Send(requestMsg, 8, otherProc, 708923);
      }
    }
  if (buf)
    {
    delete [] buf;
    }
}


//----------------------------------------------------------------------------
// Keep receiving and filling requests until all processes say they 
// are finished asking for blocks.
void vtkCTHFragmentConnect::HandleGhostBlockRequests()
{
  int requestMsg[8];
  int otherProc;
  int blockId;
  vtkCTHFragmentConnectBlock* block;
  int bufSize = 0;
  unsigned char* buf = 0;
  int dataSize;
  int* ext;

  // We do not receive requests from our own process.
  int remainingProcs = this->Controller->GetNumberOfProcesses() - 1;
  while (remainingProcs != 0)
    {
    this->Controller->Receive(requestMsg, 8, vtkMultiProcessController::ANY_SOURCE, 708923);
    otherProc = requestMsg[0];
    blockId = requestMsg[1];
    if (blockId == -1)
      {
      --remainingProcs;
      }
    else
      {
      // Find the block.
      block = this->InputBlocks[blockId];
      if (block == 0)
        { // Sanity check. This will lock up!
        vtkErrorMacro("Missing block request.");
        return;
        }
      // Crop the block.
      // Now extract the data for the ghost layer.
      ext = requestMsg+2;
      dataSize = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      if (bufSize < dataSize) 
        {
        if (buf) { delete [] buf;}
        buf = new unsigned char[dataSize];
        bufSize = dataSize;
        }
      block->ExtractExtent(buf, ext);
      // Send the block.
      this->Controller->Send(buf, dataSize, otherProc, 433240);
      }
    }
  if (buf)
    {
    delete [] buf;
    }
}

//----------------------------------------------------------------------------
// TODO: Try to not get extents supplied by existing overlap.
// Return 1 if we need this ghost block.  Ext is the part we need.
int vtkCTHFragmentConnect::ComputeRequiredGhostExtent(
  int remoteBlockLevel,
  int remoteBaseCellExt[6],
  int neededExt[6])
{
  vtkstd::vector<vtkCTHFragmentConnectBlock*> neighbors;
  int remoteBlockIndex[3];
  int remoteLayerExt[6];


  // Extract the index from the block extent.
  // Lets choose the middle in case the extent has overlap.
  remoteBlockIndex[0] = (remoteBaseCellExt[0]+remoteBaseCellExt[1]) 
                            / (2*this->StandardBlockDimensions[0]);
  remoteBlockIndex[1] = (remoteBaseCellExt[2]+remoteBaseCellExt[3]) 
                            / (2*this->StandardBlockDimensions[1]);
  remoteBlockIndex[2] = (remoteBaseCellExt[4]+remoteBaseCellExt[5]) 
                            / (2*this->StandardBlockDimensions[2]);

  neededExt[0] = neededExt[2] = neededExt[4] = VTK_LARGE_INTEGER;
  neededExt[1] = neededExt[3] = neededExt[5] = -VTK_LARGE_INTEGER;

  // These loops visit all 26 face,edge and corner neighbors.
  int direction[3];
  for (int ix = -1; ix <= 1; ++ix)
    {
    direction[0] = ix;
    for (int iy = -1; iy <= 1; ++iy)
      {
      direction[1] = iy;
      for (int iz = -1; iz <= 1; ++iz)
        {
        direction[2] = iz;
        // Skip the center of the 9x9x9 block.
        if (ix || iy || iz)
          {
          if( this->HasNeighbor(remoteBlockLevel, remoteBlockIndex, direction))
            { // A neighbor exists in this location.
            // Make sure we get ghost cells for this neighbor.
            memcpy(remoteLayerExt, remoteBaseCellExt, 6*sizeof(int));
            if (ix == -1) { remoteLayerExt[1] = remoteLayerExt[0];}
            if (ix == 1)  { remoteLayerExt[0] = remoteLayerExt[1];}
            if (iy == -1) { remoteLayerExt[3] = remoteLayerExt[2];}
            if (iy == 1)  { remoteLayerExt[2] = remoteLayerExt[3];}
            if (iz == -1) { remoteLayerExt[5] = remoteLayerExt[4];}
            if (iz == 1)  { remoteLayerExt[4] = remoteLayerExt[5];}
            // Take the union of all blocks.
            if (neededExt[0] > remoteLayerExt[0]) { neededExt[0] = remoteLayerExt[0];}
            if (neededExt[1] < remoteLayerExt[1]) { neededExt[1] = remoteLayerExt[1];}
            if (neededExt[2] > remoteLayerExt[2]) { neededExt[2] = remoteLayerExt[2];}
            if (neededExt[3] < remoteLayerExt[3]) { neededExt[3] = remoteLayerExt[3];}
            if (neededExt[4] > remoteLayerExt[4]) { neededExt[4] = remoteLayerExt[4];}
            if (neededExt[5] < remoteLayerExt[5]) { neededExt[5] = remoteLayerExt[5];}
            }
          }
        }
      }
    }
  
  if (neededExt[0] > neededExt[1] || 
      neededExt[2] > neededExt[3] || 
      neededExt[4] > neededExt[5])
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
// We need ghostblocks to connect as neighbors too.
void vtkCTHFragmentConnect::AddBlock(vtkCTHFragmentConnectBlock* block)
{
  vtkCTHFragmentLevel* level = this->Levels[block->GetLevel()];
  int dims[3];
  level->GetBlockDimensions(dims);
  block->ComputeBaseExtent(dims);
  this->CheckLevelsForNeighbors(block);
  level->AddBlock(block);
}

//
vtkPolyData *vtkCTHFragmentConnect::NewFragmentMesh()
{
  // The generated fragments for this material are placed here
  // until they are resolved and partitioned
  vtkPolyData *newPiece = vtkPolyData::New();

  // mesh
  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToFloat();
  newPiece->SetPoints(points);
  points->Delete();

  vtkCellArray* polys = vtkCellArray::New();
  newPiece->SetPolys(polys);
  polys->Delete();

  // The Id, and integrated attributes will be added later
  // as field data after the fragment pieces have been resolved.

  // Add cell array for color by scalars that are 
  // integrated. This is original data not the integration.
  for (int i=0; i<this->NToIntegrate; ++i)
    {
    vtkDoubleArray *waa=vtkDoubleArray::New();
    waa->SetName(this->IntegratedArrayNames[i].c_str());
    waa->SetNumberOfComponents(this->IntegratedArrayNComp[i]);
    newPiece->GetCellData()->AddArray(waa);
    waa->Delete();
    }

  // for debugging purposes...
  #ifndef NDEBUG
  vtkIntArray *blockIdArray = vtkIntArray::New();
  blockIdArray->SetName("BlockId");
  newPiece->GetCellData()->AddArray(blockIdArray);
  blockIdArray->Delete();

  vtkIntArray *levelArray = vtkIntArray::New();
  levelArray->SetName("Level");
  newPiece->GetCellData()->AddArray(levelArray);
  levelArray->Delete();

  vtkIntArray *procIdArray=vtkIntArray::New();
  procIdArray->SetName("ProcId");
  newPiece->GetCellData()->AddArray(procIdArray);
  procIdArray->Delete();
  #endif

  return newPiece;
}

// prepare for this pass.
// once the number of various arrays to process have been
// determined, here we build arrays to hold the results,
// clear dirty accumulators, etc...
void vtkCTHFragmentConnect::PrepareForPass(
        vtkHierarchicalBoxDataSet *hbdsInput,
        vector<string> &volumeWtdAvgArrayNames,
        vector<string> &massWtdAvgArrayNames,
        vector<string> &summedArrayNames,
        vector<string> &integratedArrayNames)
{
  this->FragmentId = 0;

  this->FragmentVolume = 0.0;
  ReNewVtkPointer(this->FragmentVolumes);
  this->FragmentVolumes->SetName("Volume");

  if (this->ComputeMoments)
    {
    this->FragmentMoment.clear();
    this->FragmentMoment.resize(4,0.0);
    ReNewVtkPointer(this->FragmentMoments);
    this->FragmentMoments->SetNumberOfComponents(4);
    this->FragmentMoments->SetName("Moments");
    }
  else
    {
    ReNewVtkPointer(this->FragmentAABBCenters);
    this->FragmentAABBCenters->SetNumberOfComponents(3);
    this->FragmentAABBCenters->SetName("Center of AABB");
    }

  if (this->ComputeOBB)
    {
    ReNewVtkPointer(this->FragmentOBBs);
    this->FragmentOBBs->SetNumberOfComponents(15);
    this->FragmentOBBs->SetName("OBB");
    }

  // Below for each integrated array we figure out if we 
  // are integrating vector or scalars, in order to build the 
  // appropriate accumulator
  vtkCompositeDataIterator *hbdsIt=hbdsInput->NewIterator();
  hbdsIt->VisitOnlyLeavesOn();
  hbdsIt->SkipEmptyNodesOn();
  hbdsIt->InitTraversal();
  vtkImageData *testImage=0;
  if ( !hbdsIt->IsDoneWithTraversal() )
    {
    testImage=dynamic_cast<vtkImageData *>(hbdsInput->GetDataSet(hbdsIt));
    }
  hbdsIt->Delete();
  // if we got a null pointer then this indicates that 
  // we do not have any blocks on this process.

  // Configure data structures
  // 1) Volume weighted average of attribute over the 
  // fragment set up containers
  this->FragmentVolumeWtdAvg.clear();
  this->FragmentVolumeWtdAvg.resize( this->NVolumeWtdAvgs );
  ClearVectorOfVtkPointers( this->FragmentVolumeWtdAvgs );
  this->FragmentVolumeWtdAvgs.resize( this->NVolumeWtdAvgs );
  // set up data array and accumulator for each weighted average
  for (int j=0; j<this->NVolumeWtdAvgs; ++j)
    {
    // data array
    const char *thisArrayName=volumeWtdAvgArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the 
    // array structure from them.
    int nComp=1;
    if (testImage)
      {
      vtkDataArray *testArray
        = testImage->GetCellData()->GetArray(thisArrayName);
      assert( "Couldn't access the named array."
              && testArray);
      nComp=testArray->GetNumberOfComponents();
      }
    this->FragmentVolumeWtdAvgs[j]=vtkDoubleArray::New();
    this->FragmentVolumeWtdAvgs[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "VolumeWeightedAverage-"
                          << thisArrayName;
    this->FragmentVolumeWtdAvgs[j]->SetName( osIntegratedArrayName.str().c_str() );
    // accumulator
    this->FragmentVolumeWtdAvg[j].resize( nComp, 0.0 );
    }
  // 2) Mass weighted average of attribute over the fragment
  // set up containers
  this->FragmentMassWtdAvg.clear();
  this->FragmentMassWtdAvg.resize(this->NMassWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentMassWtdAvgs);
  this->FragmentMassWtdAvgs.resize(this->NMassWtdAvgs);
  // set up data array and accumulator for each weighted average
  for (int j=0; j<this->NMassWtdAvgs; ++j)
    {
    // data array
    const char *thisArrayName=massWtdAvgArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the 
    // array structure from them.
    int nComp=1;
    if (testImage)
      {
      vtkDataArray *testArray
        = testImage->GetCellData()->GetArray(thisArrayName);
      assert( "Couldn't access the named array."
              && testArray);
      nComp=testArray->GetNumberOfComponents();
      }
    this->FragmentMassWtdAvgs[j]=vtkDoubleArray::New();
    this->FragmentMassWtdAvgs[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "MassWeightedAverage-"
                          << thisArrayName;
    this->FragmentMassWtdAvgs[j]->SetName( osIntegratedArrayName.str().c_str() );
    // accumulator
    this->FragmentMassWtdAvg[j].resize( nComp, 0.0 );
    }
  // 3) Summation of attribute over the fragment
  // set up containers
  this->FragmentSum.clear();
  this->FragmentSum.resize(this->NToSum);
  ClearVectorOfVtkPointers(this->FragmentSums);
  this->FragmentSums.resize(this->NToSum);
  // set up data array and accumulator for each weighted average
  for (int j=0; j<this->NToSum; ++j)
    {
    // data array
    const char *thisArrayName=summedArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the 
    // array structure from them.
    int nComp=1;
    if (testImage)
      {
      vtkDataArray *testArray
        = testImage->GetCellData()->GetArray(thisArrayName);
      assert( "Couldn't access the named array."
              && testArray);
      nComp=testArray->GetNumberOfComponents();
      }
    this->FragmentSums[j]=vtkDoubleArray::New();
    this->FragmentSums[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "Summation-"
                          << thisArrayName;
    this->FragmentSums[j]->SetName( osIntegratedArrayName.str().c_str() );
    // accumulator
    this->FragmentSum[j].resize( nComp, 0.0 );
    }

  // 4) Unique list of integrated attributes
  // Get the number of components for each. These are
  // used to copy from input to fragment cell data
  this->IntegratedArrayNComp.resize(this->NToIntegrate,0);
  for (int j=0; j<this->NToIntegrate; ++j)
    {
    // data array
    const char *thisArrayName=integratedArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the 
    // array structure from them.
    if (testImage)
      {
      vtkDataArray *testArray
        = testImage->GetCellData()->GetArray(thisArrayName);
      assert( "Couldn't access the named array."
              && testArray);
      this->IntegratedArrayNComp[j]
        = testArray->GetNumberOfComponents();
      }
    }
}

//----------------------------------------------------------------------------
// Filter operates on Materials, generating fragments by connecting
// voxels where the threshold is value is attained. It can sum 
// voxel contributions, or compute their weighted average
// it also computes the minimum bounding box of each resulting fragment
// and its center.
//TODO use same array name for frag id on multiple materials, and use global id
int vtkCTHFragmentConnect::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << this->Controller->GetLocalProcessId()
       << " entered request data." 
       << " MTime: "
       << this->GetMTime() 
       << endl;
  vtkstd::clock_t startTime=vtkstd::clock();
  #ifdef USE_VOXEL_VOLUME
  cerr << "USE_VOXEL_VOLUME\n";
  #endif
  #endif
  // get the data set which we are to process
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *hbdsInput=vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( hbdsInput==0 )
    {
    vtkErrorMacro("This filter requires a vtkHierarchicalBoxDataSet on its input.");
    return 0;
    }

  // Get the outputs
  // 0
  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // 1
  outInfo = outputVector->GetInformationObject(1);
  vtkMultiBlockDataSet *mbdsOutput1 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get arrays to process based on array selection status
  vector<string> MaterialArrayNames;
  vector<string> MassArrayNames;
  vector<string> SummedArrayNames;
  this->VolumeWtdAvgArrayNames.clear();
  this->MassWtdAvgArrayNames.clear();

  int nMaterials 
    = GetEnabledArrayNames(this->MaterialArraySelection,MaterialArrayNames);
  int nMassArrays
    = GetEnabledArrayNames(this->MassArraySelection,MassArrayNames);
  this->NVolumeWtdAvgs
    = GetEnabledArrayNames(this->VolumeWtdAvgArraySelection,
                           this->VolumeWtdAvgArrayNames);
  int nMassWtdAvgs
    = GetEnabledArrayNames(this->MassWtdAvgArraySelection,
                           this->MassWtdAvgArrayNames);
  this->NToSum
    = GetEnabledArrayNames(this->SummationArraySelection,
                           SummedArrayNames);

  // anything to do?
  if (nMaterials==0)
    {
    vtkWarningMacro("No material fraction specified.");
    return 1;
    }
  // let us look up names
  if (nMassArrays<nMaterials)
    {
    MassArrayNames.resize(nMaterials);
    vtkWarningMacro("Not all materials have mass arrays specified. "
                    << nMaterials << " materials. "
                    << nMassArrays << " mass arrays.");
    }

  // set up structure in the output data sets 
  this->BuildOutputs(mbdsOutput0,mbdsOutput1,nMaterials);

  //
  this->ProgressMaterialInc=1.0/(double)nMaterials;
  this->ProgressResolutionInc=1.0/this->ProgressMaterialInc/2.0/10.0;
  // process enabled material arrays
  for (this->MaterialId=0; this->MaterialId<nMaterials; ++this->MaterialId)
    {
    #ifdef vtkCTHFragmentConnectDEBUG
    int myProcId=this->Controller->GetLocalProcessId();
    cerr << "[" << __LINE__ << "] "
         << myProcId
         << " is processing Material "
         << this->MaterialId
         << "." << endl
         << "[" << __LINE__ << "] "
         << myProcId
         << " memory commitment at begin of pass is:"
         << endl
         << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
    #endif

    this->Progress=this->MaterialId*this->ProgressMaterialInc;
    this->UpdateProgress(this->Progress);

    // moments and mass weighted averages are only calculated 
    // where mass arrays has been selected
    this->ComputeMoments
      = this->MaterialId<nMassArrays ? true : false;
    this->NMassWtdAvgs
      = this->MaterialId<nMassArrays ? nMassWtdAvgs : 0;
    // arrays to copy to the output are affected as well
    vector<int> MergedMarkers;
    this->IntegratedArrayNames.clear();
    this->NToIntegrate
      = MergeEnabledArrayNames(this->VolumeWtdAvgArraySelection,
                               this->IntegratedArrayNames,
                               MergedMarkers);
    if (this->MaterialId<nMassArrays)
      {
      this->NToIntegrate
        += MergeEnabledArrayNames(this->MassWtdAvgArraySelection,
                                  this->IntegratedArrayNames,
                                  MergedMarkers);
      }

    // build arrays for results of attribute calculations
    this->PrepareForPass(hbdsInput,
                         this->VolumeWtdAvgArrayNames,
                         this->MassWtdAvgArrayNames,
                         SummedArrayNames,
                         this->IntegratedArrayNames);
    // 
    this->EquivalenceSet->Initialize();
    //
    this->InitializeBlocks( hbdsInput,
                            MaterialArrayNames[this->MaterialId],
                            MassArrayNames[this->MaterialId],
                            this->VolumeWtdAvgArrayNames,
                            this->MassWtdAvgArrayNames,
                            SummedArrayNames,
                            this->IntegratedArrayNames );
    //
    this->ProgressBlockInc
      = this->ProgressMaterialInc/(double)this->NumberOfInputBlocks/2.0;
    //
    int blockId;
    for (blockId = 0; blockId < this->NumberOfInputBlocks; ++blockId)
      {
      // build fragments
      this->ProcessBlock(blockId);
      }
    //char tmp[128];
    //sprintf(tmp, "C:/Law/tmp/cthSurface%d.vtp", this->Controller->GetLocalProcessId());
    //this->SaveBlockSurfaces(tmp);
    //sprintf(tmp, "C:/Law/tmp/cthGhost%d.vtp", this->Controller->GetLocalProcessId());
    //this->SaveGhostSurfaces(tmp);

    // resolve: Merge local and remote geometry 
    // correct integrated attributes, finialize integrations
    this->PrepareForResolveEquivalences();
    this->ResolveEquivalences();

    // update the resolved fragment count, so that next pass will start
    // where we left off here
    this->ResolvedFragmentCount+=this->NumberOfResolvedFragments;

    // clean after pass
    this->DeleteAllBlocks();
    //
    ReleaseVtkPointer(this->FragmentVolumes);
    if (this->ComputeMoments)
      {
      ReleaseVtkPointer(this->FragmentMoments);
      }
    else
      {
      ReleaseVtkPointer(this->FragmentAABBCenters);
      }
    if (this->ComputeOBB)
      {
      ReleaseVtkPointer(this->FragmentOBBs);
      }
    ClearVectorOfVtkPointers(this->FragmentVolumeWtdAvgs);
    ClearVectorOfVtkPointers(this->FragmentMassWtdAvgs);
    ClearVectorOfVtkPointers(this->FragmentSums);
    CheckAndReleaseVtkPointer(this->FragmentSplitGeometry);
    //
    } // for each material

  // Write all fragment attributes that we own to a text file.
  if ( this->GetWriteOutputTableFile()
        && this->OutputTableFileNameBase )
    {
    this->WriteFragmentAttributesToTextFile();
    }
  // clean up after all passes complete
  this->ResolvedFragmentIds.clear();
  this->FragmentSplitMarker.clear();
  #ifdef vtkCTHFragmentConnectDEBUG
  vtkstd::clock_t endTime=vtkstd::clock();
  cerr << "[" << __LINE__ << "] "
       << this->Controller->GetLocalProcessId()
       << " clock time ellapsed during request data "
       << (double)(endTime-startTime)/(double)CLOCKS_PER_SEC
       << " sec." << endl;
  cerr << "[" << __LINE__ << "] "
       << this->Controller->GetLocalProcessId()
       << " exited request data."
       << " MTime: " 
       << this->GetMTime() 
       << "." << endl;
  #endif
  return 1;
}

//----------------------------------------------------------------------------
int vtkCTHFragmentConnect::ProcessBlock(int blockId)
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::ProcessBlock("
               << blockId
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressBlockInc;
  this->UpdateProgress(this->Progress);


  vtkCTHFragmentConnectBlock* block = this->InputBlocks[blockId];
  if (block == 0)
    {
    return 0;
    }

  vtkCTHFragmentConnectIterator* xIterator = new vtkCTHFragmentConnectIterator;
  vtkCTHFragmentConnectIterator* yIterator = new vtkCTHFragmentConnectIterator;
  vtkCTHFragmentConnectIterator* zIterator = new vtkCTHFragmentConnectIterator;
  zIterator->Block = block;
  // set iterator to reference first non ghost cell
  zIterator->VolumeFractionPointer = block->GetBaseVolumeFractionPointer();
  zIterator->FragmentIdPointer = block->GetBaseFragmentIdPointer();
  zIterator->FlatIndex = block->GetBaseFlatIndex();

  vtkCTHFragmentConnectRingBuffer *queue = new vtkCTHFragmentConnectRingBuffer;

  // Loop through all the voxels.
  int ix, iy, iz;
  const int *ext;
  int cellIncs[3];
  block->GetCellIncrements(cellIncs);

  ext = block->GetBaseCellExtent();
  for (iz = ext[4]; iz <= ext[5]; ++iz)
    {
    zIterator->Index[2] = iz;
    *yIterator = *zIterator;  
    for (iy = ext[2]; iy <= ext[3]; ++iy)
      {
      yIterator->Index[1] = iy;
      *xIterator = *yIterator; 
      for (ix = ext[0]; ix <= ext[1]; ++ix)
        {
        xIterator->Index[0] = ix;
        // TODO loop over multiple material arrays & check if we have fragment
        // We need to then mark voxel has used in a material but available for the others.
        // We could just use which ever material has the largest fraction, then disable that
        // voxel for all, that would be easier,but not quite right for small(<50%) material
        // fractions...Not going to do that. FragmentIdPointer needs to then be an array
        // of pointers on for each material....
        if (*(xIterator->FragmentIdPointer) == -1 && 
            *(xIterator->VolumeFractionPointer) > this->scaledMaterialFractionThreshold)
          { // We have a new fragment.
          this->CurrentFragmentMesh=this->NewFragmentMesh();
          this->EquivalenceSet->AddEquivalence(this->FragmentId,this->FragmentId);
          // We have to mark every voxel we push on the queue.
          // TODO for each material, see above
          *(xIterator->FragmentIdPointer) = this->FragmentId;
          // There should be no need to clear the queue.
          queue->Push(xIterator);
          this->ConnectFragment(queue);
          // save the current fragment mesh
          // the id is implicit given by its position in the vector, but only
          // until fragments are resolved. After resolution we add addributes such 
          // as id, volume, summations averages, etc..
          this->CurrentFragmentMesh->Squeeze();
          this->FragmentMeshes.push_back( this->CurrentFragmentMesh );
          // Save the volume from the last fragment.
          this->FragmentVolumes->InsertTuple1(this->FragmentId, this->FragmentVolume);
          // clear the volume accumulator
          this->FragmentVolume = 0.0;
          if (this->ComputeMoments)
            {
            // Save the moments from the last fragment
            this->FragmentMoments->InsertTuple(this->FragmentId,
                                               &this->FragmentMoment[0]);
            // clear the moment accumulator
            FillVector(this->FragmentMoment, 0.0);
            }
          // for the volume weighted averaged scalars/vectors...
          for (int i=0; i<this->NVolumeWtdAvgs; ++i)
            {
            // update the integrated value, independent of ncomps
            this->FragmentVolumeWtdAvgs[i]->InsertTuple(this->FragmentId, 
                                                        &this->FragmentVolumeWtdAvg[i][0]);
            // clear the accumulator
            FillVector(this->FragmentVolumeWtdAvg[i],0.0);
            }
          // for the mass weighted averaged scalars/vectors...
          for (int i=0; i<this->NMassWtdAvgs; ++i)
            {
            // update the integrated value, independent of ncomps
            this->FragmentMassWtdAvgs[i]->InsertTuple(this->FragmentId,
                                                      &this->FragmentMassWtdAvg[i][0]);
            // clear the accumulator
            FillVector(this->FragmentMassWtdAvg[i],0.0);
            }
          // for the summed scalars/vectors...
          for (int i=0; i<this->NToSum; ++i)
            {
            // update the integrated value, independent of ncomps
            this->FragmentSums[i]->InsertTuple(this->FragmentId,
                                               &this->FragmentSum[i][0]);
            // clear the accumulator
            FillVector(this->FragmentSum[i],0.0);
            }
          // Move to next fragment.
          ++this->FragmentId;
          }
        xIterator->FlatIndex += cellIncs[0]; // 1/ncomp
        xIterator->VolumeFractionPointer += cellIncs[0];
        xIterator->FragmentIdPointer += cellIncs[0];
        }
      yIterator->FlatIndex += cellIncs[1]; // nx
      yIterator->VolumeFractionPointer += cellIncs[1];
      yIterator->FragmentIdPointer += cellIncs[1];
      }
    zIterator->FlatIndex += cellIncs[2]; // nx*ny
    zIterator->VolumeFractionPointer += cellIncs[2];
    zIterator->FragmentIdPointer += cellIncs[2];
    }

  delete queue;
  delete xIterator;
  delete yIterator;
  delete zIterator;

  return 1;
}


//----------------------------------------------------------------------------
// This method computes the displacement of the corner to place it near a 
// location where the interpolated volume fraction is a given threshold.
// The results are returned as displacmentFactors that scale the basis vectors
// associated with the face.  For the purpose of this computation, 
// These vectors are orthogonal and their 
// magnidutes are half the length of the voxel edges.
// The actual interpolated volume will be warped later.
// The previous version had a problem.  
// Moving along the gradient and being constrained to inside the box
// did not allow us to always hit the threshold target.
// This version moves in the direction of the average normal of original
// surface.  It essentially looks at neighbors to determine if original
// voxel surface is flat, an edge, or a corner.
// This method could probably be more efficient. General computation works
// for all gradient directions, but components are either 1 or 0.
void vtkCTHFragmentConnect::ComputeDisplacementFactors(
  vtkCTHFragmentConnectIterator* pointNeighborIterators[8], 
  double displacmentFactors[3])
{
  // DEBUGGING
  // This generates the raw voxel surface when uncommented.
  //displacmentFactors[0] = 0.0;
  //displacmentFactors[1] = 0.0;
  //displacmentFactors[2] = 0.0;
  //return;

  double v000 = pointNeighborIterators[0]->VolumeFractionPointer[0];
  double v001 = pointNeighborIterators[1]->VolumeFractionPointer[0];
  double v010 = pointNeighborIterators[2]->VolumeFractionPointer[0];
  double v011 = pointNeighborIterators[3]->VolumeFractionPointer[0];
  double v100 = pointNeighborIterators[4]->VolumeFractionPointer[0];
  double v101 = pointNeighborIterators[5]->VolumeFractionPointer[0];
  double v110 = pointNeighborIterators[6]->VolumeFractionPointer[0];
  double v111 = pointNeighborIterators[7]->VolumeFractionPointer[0];

  // cell centered data interpolated to the current node
  double centerValue = (v000+v001+v010+v011+v100+v101+v110+v111)*0.125;

  // Compute the gradient after a threshold.
  double t000 = 0.0;
  double t001 = 0.0;
  double t010 = 0.0;
  double t011 = 0.0;
  double t100 = 0.0;
  double t101 = 0.0;
  double t110 = 0.0;
  double t111 = 0.0;
  if (v000 > this->scaledMaterialFractionThreshold) { t000 = 1.0;}
  if (v001 > this->scaledMaterialFractionThreshold) { t001 = 1.0;}
  if (v010 > this->scaledMaterialFractionThreshold) { t010 = 1.0;}
  if (v011 > this->scaledMaterialFractionThreshold) { t011 = 1.0;}
  if (v100 > this->scaledMaterialFractionThreshold) { t100 = 1.0;}
  if (v101 > this->scaledMaterialFractionThreshold) { t101 = 1.0;}
  if (v110 > this->scaledMaterialFractionThreshold) { t110 = 1.0;}
  if (v111 > this->scaledMaterialFractionThreshold) { t111 = 1.0;}

  // We use the gradient ater threshold to choose a direction
  // to move the point.  After considering he discussion about
  // clamping below, we should zeroout iterators that are not
  // face connected (after threshold) to the iterator/voxel
  // that is generating this face.  We do not know which iterator 
  // that is because it was not passed in .......

  double g[3]; // Gradient at center.
  g[2] = -t000-t001-t010-t011+t100+t101+t110+t111;
  g[1] = -t000-t001+t010+t011-t100-t101+t110+t111;
  g[0] = -t000+t001-t010+t011-t100+t101-t110+t111;
  // This is unussual but it can happen with a checkerboard pattern.
  // We should break the symetry and choose a direction ...
  if (g[0] == 0.0 && g[1] == 0.0 &&  g[2] == 0.0)
    {
    displacmentFactors[0] = displacmentFactors[1] = displacmentFactors[2] = 0.0;
    return;
    }
  // If the center value is above the threshold
  // then we need to go in the negative gradient direction.
  if (centerValue > this->scaledMaterialFractionThreshold)
    {
    g[0] = -g[0];
    g[1] = -g[1];
    g[2] = -g[2];
    }

  // Scale so that the largest(smallest) component is equal to 
  // 0.5(-0.5); This puts the tip of the gradient vector on the surface
  // of a unit cube.
  double max = fabs(g[0]);
  double tmp = fabs(g[1]);
  if (tmp > max) {max = tmp;}
  tmp = fabs(g[2]);
  if (tmp > max) {max = tmp;}
  tmp = 0.5 / max;
  g[0] *= tmp;
  g[1] *= tmp;
  g[2] *= tmp;

  // g is on the surface of a unit cube. Compute interpolated surface value
  // with a tri-linear interpolation.
  double surfaceValue;
  surfaceValue = v000*(0.5-g[0])*(0.5-g[1])*(0.5-g[2])
                + v001*(0.5+g[0])*(0.5-g[1])*(0.5-g[2])
                + v010*(0.5-g[0])*(0.5+g[1])*(0.5-g[2])
                + v011*(0.5+g[0])*(0.5+g[1])*(0.5-g[2])
                + v100*(0.5-g[0])*(0.5-g[1])*(0.5+g[2])
                + v101*(0.5+g[0])*(0.5-g[1])*(0.5+g[2])
                + v110*(0.5-g[0])*(0.5+g[1])*(0.5+g[2])
                + v111*(0.5+g[0])*(0.5+g[1])*(0.5+g[2]);

  // Compute how far to the surface we must travel.
  double k = (this->scaledMaterialFractionThreshold - centerValue) / (surfaceValue - centerValue);
    // clamping caused artifacts in my test data sphere.
    // What sort of artifacts????  I forget.
    // the test spy data looks ok so lets go ahead with clamping.
    // since we only need to clamp for non manifold surfaces, the
    // ideal solution would be to let the common points diverge.
    // since we use face connectivity, the points will not be connected anyway.
    // We could also keep clean from creating non manifold surfaces.
    // I do not generate the surface in a second pass (when we have 
    // the fragment ids) because it would be too expensive.
  if (k < 0.0) 
    {
    k = 0.0;
    }
  if (k > 1.0) 
    {
    k = 1.0;
    }

  // This should give us decent displacement factors.
  k *= 2.0;
  displacmentFactors[0] = k * g[0];
  displacmentFactors[1] = k * g[1];
  displacmentFactors[2] = k * g[2];
}


//----------------------------------------------------------------------------
// Pass the non displaced corner location in the point argument.
// It will be modified with the sub voxel displacement.
void vtkCTHFragmentConnect::SubVoxelPositionCorner(
  double* point, 
  vtkCTHFragmentConnectIterator* pointNeighborIterators[8])
{
  double displacementFactors[3];
  this->ComputeDisplacementFactors(pointNeighborIterators, displacementFactors);

  // Find the smallest voxel to size the interpolation.  We use virtual
  // voxels which all have the same size.  Although this 
  // is a simplification of the real interpolation problem, there is some
  // justification for this solution.  A neighboring small voxel should
  // not influence infuence interpolated values beyond its own size.
  // However, this solution can cause the interpolation field to be
  // less smooth.  The alternative is to just interpolate to the center
  // of each voxel.  Although this approach make general interpolation
  // difficult, we are only considering corner, edge and face directions.

  // Half edges of the smallest voxel.
  double* hEdge0 = 0;
  double* hEdge1 = 0;
  double* hEdge2 = 0;
  int highestLevel = -1;
  for (int ii = 0; ii < 8; ++ii)
    {
    if (pointNeighborIterators[ii]->Block->GetLevel() > highestLevel)
      {
      highestLevel = pointNeighborIterators[ii]->Block->GetLevel();
      hEdge0 = pointNeighborIterators[ii]->Block->HalfEdges[1];
      hEdge1 = pointNeighborIterators[ii]->Block->HalfEdges[3];
      hEdge2 = pointNeighborIterators[ii]->Block->HalfEdges[5];
      }
    }

  // Apply interpolation factors.
  for (int ii = 0; ii < 3; ++ii)
    {
    // Apply the subvoxel displacements.
    point[ii] += hEdge0[ii]*displacementFactors[0]
        + hEdge1[ii]*displacementFactors[1] 
        + hEdge2[ii]*displacementFactors[2];
    }
}


//----------------------------------------------------------------------------
// The half edges define the coordinate system. They are the vectors
// from the center of a cube to a face (i.e. x, y, z).  Neighbors give 
// the volume fraction values for 18 cells (3,3,2). The completely internal
// face (between cell 4 and 13 is the face to be created. The array is
// ordered (axis0, axis1, axis2).  
// Cell 4 (1,1,0) is the voxel inside the surface.
//
// Now to fix cracks.  If neighbors are higher level, 
// I need to have more than 4 points for a face.
// I am only going to support transitions of 1 level.
void vtkCTHFragmentConnect::CreateFace(
  vtkCTHFragmentConnectIterator* in,
  vtkCTHFragmentConnectIterator* out,
  int axis, int outMaxFlag)
{
  if (in->Block == 0 || in->Block->GetGhostFlag())
    {
    return;
    }

  if (out->Block == 0)
    { // Pad the volume so we can create faces on the outside of the dataset.
    *out = *in;
    if (outMaxFlag)
      {
      ++out->Index[axis];
      }
    else
      {
      --out->Index[axis];
      }
    }

  // Add points to the output.  Create separate points for each triangle.
  // We can worry about merging points later.
  vtkCTHFragmentConnectIterator* cornerNeighbors[8];
  vtkPoints *points = this->CurrentFragmentMesh->GetPoints();  //TODO for performance store?
  vtkCellArray *polys = this->CurrentFragmentMesh->GetPolys();
  vtkIdType quadCornerIds[4];
  vtkIdType quadMidIds[4];
  vtkIdType triPtIds[3];
  vtkIdType startTriId = polys->GetNumberOfCells();

  // Avoid a warning
  quadMidIds[0] = quadMidIds[1] = quadMidIds[2] = quadMidIds[3] = 0;
  
  // Compute the corner and edge points.
  // Store the results in ivars.
  this->ComputeFacePoints(in, out,
                          axis, outMaxFlag);
  // Find the neighbor iterators.
  // Store the results in ivars.
  this->ComputeFaceNeighbors(in, out,
                             axis, outMaxFlag);

  // A word about indexing:
  // face neighbors 2x4x4 indexed face normal axis first, axis1, then axis2.
  // Some may be duplicated if they cover more than one grid element
  // Same order with point neighbors, but they are 2x2x2.
  // Face corners ordered axis1, axis2.
  // Face edge middles are 0: -axis2, 1: -axis1, 2: +axis1, 3: +axis2
  // The rational for this is the order the points are traversed
  // in the neighbor array.

  // Permute the corner neighbors to be xyz ordered.
  int inc0 = 1 << axis;
  int inc1 = 1 << ((axis+1)%3);
  int inc2 = 1 << ((axis+2)%3);
  int i0 = 0;
  int i1 = inc0;
  int i2 =      inc1; 
  int i3 = inc0+inc1; 
  int i4 =           inc2; 
  int i5 = inc0     +inc2; 
  int i6 =      inc1+inc2; 
  int i7 = inc0+inc1+inc2; 

  // Do every thing in coordinate system of face.
  cornerNeighbors[i0] = &(this->FaceNeighbors[0]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[1]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[2]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[3]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[8]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[9]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[10]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[11]);
  this->SubVoxelPositionCorner(this->FaceCornerPoints, cornerNeighbors);
  quadCornerIds[0] = points->InsertNextPoint(this->FaceCornerPoints);
  cornerNeighbors[i0] = &(this->FaceNeighbors[4]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[5]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[6]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[7]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[12]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[13]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[14]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[15]);
  this->SubVoxelPositionCorner(this->FaceCornerPoints+3, cornerNeighbors);
  quadCornerIds[1] = points->InsertNextPoint(this->FaceCornerPoints+3);
  cornerNeighbors[i0] = &(this->FaceNeighbors[16]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[17]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[18]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[19]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[24]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[25]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[26]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[27]);
  this->SubVoxelPositionCorner(this->FaceCornerPoints+6, cornerNeighbors);
  quadCornerIds[2] = points->InsertNextPoint(this->FaceCornerPoints+6);
  cornerNeighbors[i0] = &(this->FaceNeighbors[20]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[21]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[22]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[23]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[28]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[29]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[30]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[31]);
  this->SubVoxelPositionCorner(this->FaceCornerPoints+9, cornerNeighbors);
  quadCornerIds[3] = points->InsertNextPoint(this->FaceCornerPoints+9);

//   if ( quadCornerIds[3] > 45502) // 74507
//     {
//     cerr << "Debug\n";
//     }

  // Now for the mid edge point if the neighbors on that side are smaller.
  if (this->FaceEdgeFlags[0])
    {
    cornerNeighbors[i0] = &(this->FaceNeighbors[2]);
    cornerNeighbors[i1] = &(this->FaceNeighbors[3]);
    cornerNeighbors[i2] = &(this->FaceNeighbors[4]);
    cornerNeighbors[i3] = &(this->FaceNeighbors[5]);
    cornerNeighbors[i4] = &(this->FaceNeighbors[10]);
    cornerNeighbors[i5] = &(this->FaceNeighbors[11]);
    cornerNeighbors[i6] = &(this->FaceNeighbors[12]);
    cornerNeighbors[i7] = &(this->FaceNeighbors[13]);
    this->SubVoxelPositionCorner(this->FaceEdgePoints, cornerNeighbors);
    quadMidIds[0] = points->InsertNextPoint(this->FaceEdgePoints);
    }
  if (this->FaceEdgeFlags[1])
    {
    cornerNeighbors[i0] = &(this->FaceNeighbors[8]);
    cornerNeighbors[i1] = &(this->FaceNeighbors[9]);
    cornerNeighbors[i2] = &(this->FaceNeighbors[10]);
    cornerNeighbors[i3] = &(this->FaceNeighbors[11]);
    cornerNeighbors[i4] = &(this->FaceNeighbors[16]);
    cornerNeighbors[i5] = &(this->FaceNeighbors[17]);
    cornerNeighbors[i6] = &(this->FaceNeighbors[18]);
    cornerNeighbors[i7] = &(this->FaceNeighbors[19]);
    this->SubVoxelPositionCorner(this->FaceEdgePoints+3, cornerNeighbors);
    quadMidIds[1] = points->InsertNextPoint(this->FaceEdgePoints+3);
    }
  if (this->FaceEdgeFlags[2])
    {
    cornerNeighbors[i0] = &(this->FaceNeighbors[12]);
    cornerNeighbors[i1] = &(this->FaceNeighbors[13]);
    cornerNeighbors[i2] = &(this->FaceNeighbors[14]);
    cornerNeighbors[i3] = &(this->FaceNeighbors[15]);
    cornerNeighbors[i4] = &(this->FaceNeighbors[20]);
    cornerNeighbors[i5] = &(this->FaceNeighbors[21]);
    cornerNeighbors[i6] = &(this->FaceNeighbors[22]);
    cornerNeighbors[i7] = &(this->FaceNeighbors[23]);
    this->SubVoxelPositionCorner(this->FaceEdgePoints+6, cornerNeighbors);
    quadMidIds[2] = points->InsertNextPoint(this->FaceEdgePoints+6);
    }
  if (this->FaceEdgeFlags[3])
    {
    cornerNeighbors[i0] = &(this->FaceNeighbors[18]);
    cornerNeighbors[i1] = &(this->FaceNeighbors[19]);
    cornerNeighbors[i2] = &(this->FaceNeighbors[20]);
    cornerNeighbors[i3] = &(this->FaceNeighbors[21]);
    cornerNeighbors[i4] = &(this->FaceNeighbors[26]);
    cornerNeighbors[i5] = &(this->FaceNeighbors[27]);
    cornerNeighbors[i6] = &(this->FaceNeighbors[28]);
    cornerNeighbors[i7] = &(this->FaceNeighbors[29]);
    this->SubVoxelPositionCorner(this->FaceEdgePoints+9, cornerNeighbors);
    quadMidIds[3] = points->InsertNextPoint(this->FaceEdgePoints+9);
    }

  // Now there are 9 possibilities 
  // (10 if you count the two ways to triangulate the simple quad).
  // No edges, $ cases with one mid point, 4 cases with two mid points.
  // That is all because the face is always the smallest of the two in/out voxels.
  int caseIdx = this->FaceEdgeFlags[0] | (this->FaceEdgeFlags[1] << 1)
                  | (this->FaceEdgeFlags[2] << 2) | (this->FaceEdgeFlags[3] << 3);
  
  //c2 e3 c3
  //e1    e2
  //c0 e0 c1

  switch (caseIdx)
    {
    // One edge point
    case 1:
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadCornerIds[2];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadCornerIds[3];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadCornerIds[1];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 2:
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadCornerIds[3];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadCornerIds[1];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadCornerIds[0];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 4:
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadCornerIds[0];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadCornerIds[2];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadCornerIds[3];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 8:
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadCornerIds[1];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadCornerIds[0];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadCornerIds[2];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      break;
    // Two adjacent edge points
    case 3: // e0 and e1
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadMidIds[1];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadMidIds[0];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadCornerIds[3];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadCornerIds[1];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 5: // e0 and e2
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadMidIds[0];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadMidIds[2];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadCornerIds[3];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadCornerIds[2];
      triPtIds[2] = quadMidIds[0];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 10: // e1 and e3
      triPtIds[0] = quadCornerIds[2];
      triPtIds[1] = quadMidIds[3];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadMidIds[1];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadCornerIds[1];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadCornerIds[0];
      triPtIds[2] = quadMidIds[1];
      polys->InsertNextCell(3, triPtIds);
      break;
    case 12: // e2 and e3
      triPtIds[0] = quadCornerIds[3];
      triPtIds[1] = quadMidIds[2];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadMidIds[3];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[1];
      triPtIds[1] = quadCornerIds[0];
      triPtIds[2] = quadMidIds[2];
      polys->InsertNextCell(3, triPtIds);
      triPtIds[0] = quadCornerIds[0];
      triPtIds[1] = quadCornerIds[2];
      triPtIds[2] = quadMidIds[3];
      polys->InsertNextCell(3, triPtIds);
      break;
    // No edges.
    // For a simple quad, choose a triangulation.
    case 0:
      {
      // Compute length of diagonals (squared)
      // This will help us decide which way to split up the quad into triangles.
      double tmp;
      double d0011 = 0.0;
      double d0110 = 0.0;
      double *pt00 = this->FaceCornerPoints;
      double *pt01 = this->FaceCornerPoints+3;
      double *pt10 = this->FaceCornerPoints+6;
      double *pt11 = this->FaceCornerPoints+9;
      for (int ii = 0; ii < 3; ++ii)
        {
        tmp = pt00[ii]-pt11[ii];
        d0011 += tmp*tmp;
        tmp = pt01[ii]-pt10[ii];
        d0110 += tmp*tmp;
        }
      if (d0011 < d0110)
        {   
        triPtIds[0] = quadCornerIds[0];
        triPtIds[1] = quadCornerIds[1];
        triPtIds[2] = quadCornerIds[3];
        polys->InsertNextCell(3, triPtIds);
        triPtIds[0] = quadCornerIds[0];
        triPtIds[1] = quadCornerIds[3];
        triPtIds[2] = quadCornerIds[2];
        polys->InsertNextCell(3, triPtIds);
        }
      else
        {
        triPtIds[0] = quadCornerIds[1];
        triPtIds[1] = quadCornerIds[3];
        triPtIds[2] = quadCornerIds[2];
        polys->InsertNextCell(3, triPtIds);
        triPtIds[0] = quadCornerIds[2];
        triPtIds[1] = quadCornerIds[0];
        triPtIds[2] = quadCornerIds[1];
        polys->InsertNextCell(3, triPtIds);
        }
      break;
      }
    default:
      vtkErrorMacro("Unexpected edge case:" << caseIdx);
    }

  vtkIdType numTris = polys->GetNumberOfCells() - startTriId;
  // Copy the original data for the arrays being integrated
  // into the fragment surface.
  vector<double> thisTup(3);
  for (int i=0; i<this->NToIntegrate; ++i)
    {
    // original
    vtkDataArray *srcArray
      = in->Block->GetIntegratedArray(i);
    int nComps
      = srcArray->GetNumberOfComponents();
    thisTup.resize(nComps);
    CopyTuple(&thisTup[0],srcArray,nComps,in->FlatIndex);

    // fragment
    vtkDoubleArray *destArray
      = dynamic_cast<vtkDoubleArray *>(this->CurrentFragmentMesh->GetCellData()->GetArray(i));
    for (vtkIdType ii = 0; ii < numTris; ++ii)
      {
      destArray->InsertNextTuple(&thisTup[0]);
      }
    }
  // Cell data attributes for debugging.
  #ifndef NDEBUG
  vtkIntArray *levelArray
    = dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("Level"));

  vtkIntArray *blockIdArray
    = dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("BlockId"));

  vtkIntArray *procIdArray
    = dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("ProcId"));

  for (vtkIdType ii = 0; ii < numTris; ++ii)
    {
    levelArray->InsertNextValue(in->Block->GetLevel());
    blockIdArray->InsertNextValue(in->Block->LevelBlockId);
    procIdArray->InsertNextValue(this->Controller->GetLocalProcessId());
    }
  #endif
}

//----------------------------------------------------------------------------
// Computes the face and edge middle points of the shared contact face
// between the two iterators.
void vtkCTHFragmentConnect::ComputeFacePoints(
  vtkCTHFragmentConnectIterator* in,
  vtkCTHFragmentConnectIterator* out,
  int axis, int outMaxFlag)
{
  vtkCTHFragmentConnectIterator* smaller;
  double* origin;
  double* spacing;
  int maxFlag;
  int axis1 = (axis+1)%3;
  int axis2 = (axis+2)%3;
  
  // We create the smaller face of the two voxels.
  smaller = in;
  // A bit confusing.  If out iterator is max, then the face of out is min.
  maxFlag = outMaxFlag;
  if (in->Block->GetLevel() < out->Block->GetLevel())
    {
    smaller = out;
    maxFlag = !outMaxFlag;
    }
  
  origin = smaller->Block->GetOrigin();
  spacing = smaller->Block->GetSpacing();
  double halfSpacing[3];
  double faceOrigin[3];
  halfSpacing[0] = spacing[0] * 0.5;
  halfSpacing[1] = spacing[1] * 0.5;
  halfSpacing[2] = spacing[2] * 0.5;
  // find the origin of the voxel.
  faceOrigin[0] = origin[0] + (double)(smaller->Index[0]) * spacing[0];
  faceOrigin[1] = origin[1] + (double)(smaller->Index[1]) * spacing[1];
  faceOrigin[2] = origin[2] + (double)(smaller->Index[2]) * spacing[2];
  // Move the voxel origin to the face origin.
  if (maxFlag)
    {
    faceOrigin[axis] += spacing[axis];
    }

  // Now compute all of the corner points.
  // 6 9
  // 0 3
  // First set them all to the origin.
  this->FaceCornerPoints[0] = this->FaceCornerPoints[3] = 
    this->FaceCornerPoints[6] = this->FaceCornerPoints[9] = faceOrigin[0];
  this->FaceCornerPoints[1] = this->FaceCornerPoints[4] = 
    this->FaceCornerPoints[7] = this->FaceCornerPoints[10] = faceOrigin[1];
  this->FaceCornerPoints[2] = this->FaceCornerPoints[5] = 
    this->FaceCornerPoints[8] = this->FaceCornerPoints[11] = faceOrigin[2];
  // Now offset them to the corners.
  this->FaceCornerPoints[3+axis1] += spacing[axis1];
  this->FaceCornerPoints[9+axis1] += spacing[axis1];
  this->FaceCornerPoints[6+axis2] += spacing[axis2];
  this->FaceCornerPoints[9+axis2] += spacing[axis2];

  // Now do the same for the edge points
  //   3
  // 1   2
  //   0
  // First set them all to the origin.
  this->FaceEdgePoints[0] = this->FaceEdgePoints[3] = 
    this->FaceEdgePoints[6] = this->FaceEdgePoints[9] = faceOrigin[0];
  this->FaceEdgePoints[1] = this->FaceEdgePoints[4] = 
    this->FaceEdgePoints[7] = this->FaceEdgePoints[10] = faceOrigin[1];
  this->FaceEdgePoints[2] = this->FaceEdgePoints[5] = 
    this->FaceEdgePoints[8] = this->FaceEdgePoints[11] = faceOrigin[2];  
  // Now offset the points to the middle of the edges.
  this->FaceEdgePoints[axis1] += halfSpacing[axis1];
  this->FaceEdgePoints[9+axis1] += halfSpacing[axis1];
  this->FaceEdgePoints[6+axis1] += spacing[axis1];
  this->FaceEdgePoints[3+axis2] += halfSpacing[axis2];
  this->FaceEdgePoints[6+axis2] += halfSpacing[axis2];
  this->FaceEdgePoints[9+axis2] += spacing[axis2];
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::ComputeFaceNeighbors(
  vtkCTHFragmentConnectIterator* in,
  vtkCTHFragmentConnectIterator* out,
  int axis, int outMaxFlag)
{
  int axis1 = (axis+1)%3;
  int axis2 = (axis+2)%3;

  // Neighbors are one level higher than the smallest voxel.
  int faceLevel;
  int faceIndex[3];
  // We need to find the smallest face (largest level) of the two iterators.
  if (in->Block->GetLevel() > out->Block->GetLevel())
    {
    faceLevel = in->Block->GetLevel() + 1;
    faceIndex[0] = in->Index[0];
    faceIndex[1] = in->Index[1];
    faceIndex[2] = in->Index[2];
    if (outMaxFlag)
      {
      faceIndex[axis] += 1;
      }
    }
  else
    {
    faceLevel = out->Block->GetLevel() + 1;
    faceIndex[0] = out->Index[0];
    faceIndex[1] = out->Index[1];
    faceIndex[2] = out->Index[2];
    if ( ! outMaxFlag)
      {
      faceIndex[axis] += 1;
      }
    }
  faceIndex[0] = faceIndex[0] << 1;
  faceIndex[1] = faceIndex[1] << 1;
  faceIndex[2] = faceIndex[2] << 1;

  // The center four on each side of the face are always the same.
  // The face is the smallest of in and out, so there is no possibility
  // for subdivision.
  if (outMaxFlag)
    {
    this->FaceNeighbors[10] = this->FaceNeighbors[12] =
       this->FaceNeighbors[18] = this->FaceNeighbors[20] = *in;
    this->FaceNeighbors[11] = this->FaceNeighbors[13] =
       this->FaceNeighbors[19] = this->FaceNeighbors[21] = *out;
    }
  else
    {
    this->FaceNeighbors[10] = this->FaceNeighbors[12] =
       this->FaceNeighbors[18] = this->FaceNeighbors[20] = *out;
    this->FaceNeighbors[11] = this->FaceNeighbors[13] =
       this->FaceNeighbors[19] = this->FaceNeighbors[21] = *in;
    }

  // Ok, we have 24 neighbors to compute.
  // How can we do this efficiently?
  // faceIndex and faceLevel here are actually neighborIndex and neighborLevel.
  // Face index starts at (1,1,1)
  // increments: 1, 2, 8
  // Start at the corner and march around the edges.
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+3, this->FaceNeighbors+11);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+5, this->FaceNeighbors+3);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+7, this->FaceNeighbors+5);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+15, this->FaceNeighbors+7);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+23, this->FaceNeighbors+15);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+31, this->FaceNeighbors+23);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+29, this->FaceNeighbors+31);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+27, this->FaceNeighbors+29);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+25, this->FaceNeighbors+27);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+17, this->FaceNeighbors+25);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+9, this->FaceNeighbors+17);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+1, this->FaceNeighbors+9);
  //Now for the other side (min axis).
  faceIndex[axis] -= 1; // Move to the other layer
  faceIndex[axis1] += 1; // Start below reference block.
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+2, this->FaceNeighbors+10);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+4, this->FaceNeighbors+2);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+6, this->FaceNeighbors+4);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+14, this->FaceNeighbors+6);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+22, this->FaceNeighbors+14);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+30, this->FaceNeighbors+22);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+28, this->FaceNeighbors+30);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+26, this->FaceNeighbors+28);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+24, this->FaceNeighbors+26);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+16, this->FaceNeighbors+24);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+8, this->FaceNeighbors+16);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors+0, this->FaceNeighbors+8);
  
  --faceLevel;
  this->FaceEdgeFlags[0] = 0;
  if (this->FaceNeighbors[2].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[3].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[4].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[5].Block->GetLevel() > faceLevel)
    {
    this->FaceEdgeFlags[0] = 1;
    }
  this->FaceEdgeFlags[1] = 0;
  if (this->FaceNeighbors[8].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[9].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[16].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[17].Block->GetLevel() > faceLevel)
    {
    this->FaceEdgeFlags[1] = 1;
    }
  this->FaceEdgeFlags[2] = 0;
  if (this->FaceNeighbors[14].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[15].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[22].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[23].Block->GetLevel() > faceLevel)
    {
    this->FaceEdgeFlags[2] = 1;
    }
  this->FaceEdgeFlags[3] = 0;
  if (this->FaceNeighbors[26].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[27].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[28].Block->GetLevel() > faceLevel ||
      this->FaceNeighbors[29].Block->GetLevel() > faceLevel)
    {
    this->FaceEdgeFlags[3] = 1;
    }
}


//----------------------------------------------------------------------------
// Find an iterator from an index,level and a neighbor reference iterator.
void vtkCTHFragmentConnect::FindNeighbor(
  int faceIdx[3], int faceLevel, 
  vtkCTHFragmentConnectIterator* neighbor,
  vtkCTHFragmentConnectIterator* reference)
{
  // Convert the index to the level of the reference block.
  int neighborIdx[3];
  vtkCTHFragmentConnectBlock* refBlock = reference->Block;
  const int* ext;
  ext = refBlock->GetBaseCellExtent();
  int refLevel = refBlock->GetLevel();

  if (refLevel > faceLevel)
    {
    neighborIdx[0] = faceIdx[0] << (refLevel-faceLevel);
    neighborIdx[1] = faceIdx[1] << (refLevel-faceLevel);
    neighborIdx[2] = faceIdx[2] << (refLevel-faceLevel);
    }
  else
    {
    neighborIdx[0] = faceIdx[0] >> (faceLevel-refLevel);
    neighborIdx[1] = faceIdx[1] >> (faceLevel-refLevel);
    neighborIdx[2] = faceIdx[2] >> (faceLevel-refLevel);
    }

  // The index might point to the reference iterator.
  if (neighborIdx[0] == reference->Index[0] && 
      neighborIdx[1] == reference->Index[1] &&
      neighborIdx[2] == reference->Index[2])
    {
    *neighbor = *reference;
    return;
    }

  // Find the block the neighbor is in.

  int tmpLevel;
  int recheck = 1;
  while (recheck)
    {
    recheck = 0;
    // Check each axis and direction for the extent leaving the bounds.
    for (int axis = 0; axis < 3; ++axis)
      {
      int minIdx = 2*axis;
      int maxIdx = minIdx + 1;
      // Min direction
      if (neighborIdx[axis] < ext[minIdx] && 
          refBlock->GetNumberOfFaceNeighbors(minIdx) > 0)
        {
        // Move in this direction.
        refBlock = refBlock->GetFaceNeighbor(minIdx, 0);
        ext = refBlock->GetBaseCellExtent();
        // Save level for next comparison.
        tmpLevel = refBlock->GetLevel();
        if (tmpLevel > faceLevel)
          {
          neighborIdx[0] = faceIdx[0] << (tmpLevel-faceLevel);
          neighborIdx[1] = faceIdx[1] << (tmpLevel-faceLevel);
          neighborIdx[2] = faceIdx[2] << (tmpLevel-faceLevel);
          }
        else
          {
          neighborIdx[0] = faceIdx[0] >> (faceLevel-tmpLevel);
          neighborIdx[1] = faceIdx[1] >> (faceLevel-tmpLevel);
          neighborIdx[2] = faceIdx[2] >> (faceLevel-tmpLevel);
          }
        // If we changed levels, or our extent 
        // does not included index look some more.
        // Note: If we move to a higher level, previous axes may go
        // out of extent.
        if (tmpLevel > refLevel ||
            neighborIdx[axis] < ext[minIdx])
          {
          recheck = -1;
          }
        refLevel = tmpLevel;
        }

      // Max direction
      if (neighborIdx[axis] > ext[maxIdx] && 
          refBlock->GetNumberOfFaceNeighbors(maxIdx) > 0)
        {
        // Move in this direction.
        refBlock = refBlock->GetFaceNeighbor(maxIdx, 0);
        ext = refBlock->GetBaseCellExtent();
        // Save level for next comparison.
        tmpLevel = refBlock->GetLevel();
        if (tmpLevel > faceLevel)
          {
          neighborIdx[0] = faceIdx[0] << (tmpLevel-faceLevel);
          neighborIdx[1] = faceIdx[1] << (tmpLevel-faceLevel);
          neighborIdx[2] = faceIdx[2] << (tmpLevel-faceLevel);
          }
        else
          {
          neighborIdx[0] = faceIdx[0] >> (faceLevel-tmpLevel);
          neighborIdx[1] = faceIdx[1] >> (faceLevel-tmpLevel);
          neighborIdx[2] = faceIdx[2] >> (faceLevel-tmpLevel);
          }
        // If we changed levels, or our extent 
        // does not included index look some more.
        // Note: If we move to a higher level, previous axes may go
        // out of extent.
        if (tmpLevel > refLevel ||
            neighborIdx[axis] < ext[minIdx])
          {
          recheck = -1;
          }
        refLevel = tmpLevel;
        }
      }
    }

  // We have a block
  // clamp the neighbor index to pad the volume
  if (neighborIdx[0] < ext[0]) { neighborIdx[0] = ext[0];}
  if (neighborIdx[0] > ext[1]) { neighborIdx[0] = ext[1];}
  if (neighborIdx[1] < ext[2]) { neighborIdx[1] = ext[2];}
  if (neighborIdx[1] > ext[3]) { neighborIdx[1] = ext[3];}
  if (neighborIdx[2] < ext[4]) { neighborIdx[2] = ext[4];}
  if (neighborIdx[2] > ext[5]) { neighborIdx[2] = ext[5];}

  neighbor->Block = refBlock;
  neighbor->Index[0] = neighborIdx[0];
  neighbor->Index[1] = neighborIdx[1];
  neighbor->Index[2] = neighborIdx[2];
  const int *incs = refBlock->GetCellIncrements();
  int offset = (neighborIdx[0]- ext[0])*incs[0]
             + (neighborIdx[1]- ext[2])*incs[1]
             + (neighborIdx[2]- ext[4])*incs[2];
  neighbor->FragmentIdPointer = refBlock->GetBaseFragmentIdPointer() + offset;
  neighbor->VolumeFractionPointer = refBlock->GetBaseVolumeFractionPointer() + offset;
  neighbor->FlatIndex = refBlock->GetBaseFlatIndex() + offset;
}







//----------------------------------------------------------------------------
// This is for getting the neighbors necessary to place face points.
// If the next iterator is out of bounds, the last iterator is duplicated.
void vtkCTHFragmentConnect::GetNeighborIteratorPad(
  vtkCTHFragmentConnectIterator* next,
  vtkCTHFragmentConnectIterator* iterator, 
  int axis0, int maxFlag0,
  int axis1, int maxFlag1,
  int axis2, int maxFlag2)
{
  if (iterator->VolumeFractionPointer == 0)
    {
    vtkErrorMacro("Error empty input block.  Cannot find neighbor.");
    *next = *iterator;
    return;
    }
  this->GetNeighborIterator(next, iterator, 
                            axis0, maxFlag0,
                            axis1, maxFlag1,
                            axis2, maxFlag2);

  if (next->VolumeFractionPointer == 0)
    { // Next is out of bounds. Duplicate the last iterator to get a values.
    *next = *iterator;
    if (maxFlag0)
      {
      ++next->Index[axis0];
      }
    else
      {
      --next->Index[axis0];
      }
    }
}

//----------------------------------------------------------------------------
// Find the neighboring voxel and return results in "next".
// Neighbor could be in another block.
// The neighbor is along axis0 in the maxFlag0 direction.
// If the neighbor is in a higher level, then maxFlag1 and maxFlag2
// are used to determine which subvoxel to return.
// Otherwise, these arguments have no effect.
// This last feature is used to find iterators around a point for positioning.
void vtkCTHFragmentConnect::GetNeighborIterator(
  vtkCTHFragmentConnectIterator* next,
  vtkCTHFragmentConnectIterator* iterator, 
  int axis0, int maxFlag0,
  int axis1, int maxFlag1,
  int axis2, int maxFlag2)
{
  if (iterator->Block == 0)
    { // No input, no output.  This should not happen.
    vtkWarningMacro("Can not find neighbor for NULL block.");
    *next = *iterator;
    return;
    }

  const int *ext;
  ext = iterator->Block->GetBaseCellExtent();
  int incs[3];
  iterator->Block->GetCellIncrements(incs);

  if (maxFlag0 && iterator->Index[axis0] < ext[2*axis0+1])
    { // Neighbor is inside this block.
    *next = *iterator;
    next->Index[axis0] += 1;
    next->VolumeFractionPointer += incs[axis0];
    next->FragmentIdPointer += incs[axis0];
    next->FlatIndex += incs[axis0];
    return;
    }
  if (!maxFlag0 && iterator->Index[axis0] > ext[2*axis0])
    { // Neighbor is inside this block.
    *next = *iterator;
    next->Index[axis0] -= 1;
    next->VolumeFractionPointer -= incs[axis0];
    next->FragmentIdPointer -= incs[axis0];
    next->FlatIndex -= incs[axis0];
    return;
    }
  // Look for a neighboring block.
  vtkCTHFragmentConnectBlock* block;
  int num, idx;
  num = iterator->Block->GetNumberOfFaceNeighbors(2*axis0+maxFlag0);
  for (idx = 0; idx < num; ++idx)
    {
    block = iterator->Block->GetFaceNeighbor(2*axis0+maxFlag0, idx);
    // Convert index into new block level.
    next->Index[0] = iterator->Index[0];
    next->Index[1] = iterator->Index[1];
    next->Index[2] = iterator->Index[2];
    // When moving to the right, increment before changing level.

    // Negative shifts do not behave as expected!!!!
    if (iterator->Block->GetLevel() > block->GetLevel())
      { // Going to a lower level.
      if (maxFlag0)
        {
        next->Index[axis0] += 1;
        next->Index[axis0] = next->Index[axis0] >> (iterator->Block->GetLevel()-block->GetLevel());
        }
      else
        {
        next->Index[axis0] = next->Index[axis0] >> (iterator->Block->GetLevel()-block->GetLevel());
        next->Index[axis0] -= 1;
        }
      // maxFlags for axis1 and 2 do nothing in this case.
      next->Index[axis1] = next->Index[axis1] >> (iterator->Block->GetLevel()-block->GetLevel());
      next->Index[axis2] = next->Index[axis2] >> (iterator->Block->GetLevel()-block->GetLevel());
      }
    else if (iterator->Block->GetLevel() < block->GetLevel())
      { // Going to a higher level.
      if (maxFlag0)
        {
        next->Index[axis0] += 1;
        next->Index[axis0] = next->Index[axis0] << (block->GetLevel()-iterator->Block->GetLevel());
        }
      else
        {
        next->Index[axis0] = next->Index[axis0] << (block->GetLevel()-iterator->Block->GetLevel());
        next->Index[axis0] -= 1;
        }
      if (maxFlag1)
        {
        next->Index[axis1] = ((next->Index[axis1]+1) << (block->GetLevel()-iterator->Block->GetLevel())) - 1;
        }
      else
        {
        next->Index[axis1] = next->Index[axis1] << (block->GetLevel()-iterator->Block->GetLevel());
        }
      if (maxFlag2)
        {
        next->Index[axis2] = ((next->Index[axis2]+1) << (block->GetLevel()-iterator->Block->GetLevel())) - 1;
        }
      else
        {
        next->Index[axis2] = next->Index[axis2] << (block->GetLevel()-iterator->Block->GetLevel());
        }
      }
    else
      { // same level: just increment/decrement the index.
      if (maxFlag0)
        {
        next->Index[axis0] += 1;
        }
      else
        {
        next->Index[axis0] -= 1;
        }
      }

    ext = block->GetBaseCellExtent();
    if ((ext[0] <= next->Index[0] && next->Index[0] <= ext[1]) && 
        (ext[2] <= next->Index[1] && next->Index[1] <= ext[3]) &&
        (ext[4] <= next->Index[2] && next->Index[2] <= ext[5]))
      { // Neighbor voxel is in this block.
      next->Block = block;
      block->GetCellIncrements(incs);
      int offset = (next->Index[0]-ext[0])*incs[0]
                    + (next->Index[1]-ext[2])*incs[1]
                    + (next->Index[2]-ext[4])*incs[2];
      next->VolumeFractionPointer = block->GetBaseVolumeFractionPointer() + offset;
      next->FragmentIdPointer = block->GetBaseFragmentIdPointer() + offset;
      next->FlatIndex = block->GetBaseFlatIndex() + offset;
      return;
      }
    }
  // No neighbors contain this next index.
  next->Initialize();
}

// integration helper, returns 0 if the source array 
// type is unsupported.
inline
int vtkCTHFragmentConnect::Accumulate(
               double *dest,         // scalar/vector result
               vtkDataArray *src,    // array to accumulate from
               int nComps,           // 
               int srcCellIndex,     // which cell
               double weight)        // weight of contributions(one for each component)
{
  // convert cell index to array index
  int srcIndex=nComps*srcCellIndex;
  // accumulate based on data types
  switch ( src->GetDataType() )
    {
    case VTK_FLOAT:{
      float *thisTuple
        = dynamic_cast<vtkFloatArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]+=thisTuple[q]*weight;
        }}
    break;
    case VTK_DOUBLE:{
      double *thisTuple
        = dynamic_cast<vtkDoubleArray *>(src)->GetPointer(srcIndex);
      for (int q=0; q<nComps; ++q)
        {
        dest[q]+=thisTuple[q]*weight;
        }}
    break;
    default:
    assert( "This data type is unsupported."
            && 0 );
    return 0;
    break;
    }
  return 1;
}

// integration helper, returns 0 if the source array 
// type is unsupported.
inline
int vtkCTHFragmentConnect::AccumulateMoments(
               double *moments,      // =(Myz, Mxz, Mxy, m)
               vtkDataArray *massArray,//
               int srcCellIndex,     // from which cell in mass
               double *X)            // =(x,y,z)
{
  // accumulate based on data types
  switch ( massArray->GetDataType() )
    {
    case VTK_FLOAT:{
      float *mass
        = dynamic_cast<vtkFloatArray *>(massArray)->GetPointer(srcCellIndex);
      for (int q=0; q<3; ++q)
        {
        moments[q]+=mass[0]*X[q];
        }
      moments[3]+=mass[0];}
    break;
    case VTK_DOUBLE:{
      double *mass
        = dynamic_cast<vtkDoubleArray *>(massArray)->GetPointer(srcCellIndex);
      for (int q=0; q<3; ++q)
        {
        moments[q]+=mass[0]*X[q];
        }
      moments[3]+=mass[0];}
    break;
    default:
    assert( "This data type is unsupported."
            && 0 );
    return 0;
    break;
    }
  return 1;
}

//----------------------------------------------------------------------------
// Depth first search marking voxels.
// This extracts faces at the same time.
// This integrates quantities at the same time.
// This is called only when the voxel is part of a fragment.
// I tried to create a generic API to replace the hard coded conditional ifs.
void vtkCTHFragmentConnect::ConnectFragment(vtkCTHFragmentConnectRingBuffer *queue)
{ 
  while (queue->GetSize())
    {
    // Get the next voxel/iterator to search.
    vtkCTHFragmentConnectIterator iterator;
    queue->Pop(&iterator);
    // Lets integrate when we remove the iterator from the queue.
    // We could also do it when we add the iterator to the queue, but
    // the adds occur in so many places.
    if (iterator.Block->GetGhostFlag() == 0)
      {
      // accumlate fragment volume
      const double *dX=iterator.Block->GetSpacing();
      #ifdef USE_VOXEL_VOLUME
      double voxelVolumeFrac = dX[0]*dX[1]*dX[2];
      #else
      double voxelVolumeFrac
        = dX[0]*dX[1]*dX[2]*(double)(*(iterator.VolumeFractionPointer))/255.0;
      #endif
      this->FragmentVolume+=voxelVolumeFrac;
      // accumulate volume weighted average
      for (int i=0; i<this->NVolumeWtdAvgs; ++i)
        {
        vtkDataArray *arrayToIntegrate
          = iterator.Block->GetVolumeWtdAvgArray(i);
        int nComps
          = arrayToIntegrate->GetNumberOfComponents();
        this->Accumulate( &this->FragmentVolumeWtdAvg[i][0],
                          arrayToIntegrate,
                          nComps,
                          iterator.FlatIndex,
                          voxelVolumeFrac );
        }
      // accumulate mass weighted average
      // Accumulate mass and moments
      if ( this->ComputeMoments )
        {
        vtkDataArray *massArray=iterator.Block->GetMassArray();
        // mass and moments
        const double *X0=iterator.Block->GetOrigin();
        double X[3]={X0[0]+dX[0]*(0.5+iterator.Index[0]),
                     X0[1]+dX[1]*(0.5+iterator.Index[1]),
                     X0[2]+dX[2]*(0.5+iterator.Index[2])};
        this->AccumulateMoments(&this->FragmentMoment[0],
                                massArray,
                                iterator.FlatIndex,
                                X);
        // mass weighted averages
        double voxelMass;
        massArray->GetTuple(iterator.FlatIndex,&voxelMass);
        for (int i=0; i<this->NMassWtdAvgs; ++i)
          {
          vtkDataArray *arrayToIntegrate
            = iterator.Block->GetMassWtdAvgArray(i);
          int nComps
            = arrayToIntegrate->GetNumberOfComponents();
          this->Accumulate( &this->FragmentMassWtdAvg[i][0],
                            arrayToIntegrate,
                            nComps,
                            iterator.FlatIndex,
                            voxelMass );
          }
        }
      // accumulate sum
      for (int i=0; i<this->NToSum; ++i)
        {
        vtkDataArray *arrayToIntegrate
          = iterator.Block->GetArrayToSum(i);
        int nComps
          = arrayToIntegrate->GetNumberOfComponents();
        this->Accumulate( &this->FragmentSum[i][0],
                          arrayToIntegrate,
                          nComps,
                          iterator.FlatIndex,
                          1.0 );
        }
      }

    // Create another iterator on the stack for recursion.
    vtkCTHFragmentConnectIterator next;

    // Look at the face connected neighbors and recurse.
    // We are not on the border and volume fraction of neighbor is high and
    // we have not visited the voxel yet.
    for (int ii = 0; ii < 3; ++ii)
      {
      // I would make these variables to make the computation more clear, 
      // but they would accumulate on the stack.
      //axis0 = ii;
      //axis1 = (ii+1)%3;
      //axis2 = (ii+2)%3;
      //idxMin = 2*ii;
      //idxMax = 2*ii+1this->IndexMax+1;
      // "Left"/min
      this->GetNeighborIterator(&next, &iterator, ii,0, (ii+1)%3,0, (ii+2)%3,0);

      if (next.VolumeFractionPointer == 0 || 
          next.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
        {
        // Neighbor is outside of fragment.  Make a face.
        this->CreateFace(&iterator, &next, ii, 0);
        }
      else if (next.FragmentIdPointer[0] == -1)
        { // We have not visited this neighbor yet. Mark the voxel and recurse.
        *(next.FragmentIdPointer) = this->FragmentId;
        queue->Push(&next);
        }
      else
        { // The last case is that we have already visited this voxel and it
        // is in the same fragment.
        this->AddEquivalence(&iterator, &next);
        }

      // Handle the case when the new iterator is a higher level.
      // We need to loop over all the faces of the higher level that touch this face.
      // We will restrict our case to 4 neighbors (max difference in levels is 1).
      // If level skip, things should still work OK. Biggest issue is holes in surface.
      // This also sort of assumes that at most one other block touches this face.
      // Holes might appear if this is not true.
      if (next.Block && next.Block->GetLevel() > iterator.Block->GetLevel())
        {
        vtkCTHFragmentConnectIterator next2;
        // Take the first neighbor found and move +Y
        this->GetNeighborIterator(&next2, &next, (ii+1)%3,1, (ii+2)%3,0, ii,0);
        if (next2.VolumeFractionPointer == 0 || 
            next2.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
          {
          // Neighbor is outside of fragment.  Make a face.
          this->CreateFace(&iterator, &next2, ii, 0);
          }    
        else if (next2.FragmentIdPointer[0] == -1)
          { // We have not visited this neighbor yet. Mark the voxel and recurse.
         *(next2.FragmentIdPointer) = this->FragmentId;
          queue->Push(&next2);
          }
        else
          { // The last case is that we have already visited this voxel and it
          // is in the same fragment.
          this->AddEquivalence(&next2, &next);
          }
        // Take the fist iterator found and move +Z
        this->GetNeighborIterator(&next2, &next, (ii+2)%3,1, ii,0, (ii+1)%3,0);
        if (next2.VolumeFractionPointer == 0 || 
            next2.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
          {
          // Neighbor is outside of fragment.  Make a face.
          this->CreateFace(&iterator, &next2, ii, 0);
          }    
        else if (next2.FragmentIdPointer[0] == -1)
          { // We have not visited this neighbor yet. Mark the voxel and recurse.
          *(next2.FragmentIdPointer) = this->FragmentId;
          queue->Push(&next2);
          }
        else
          { // The last case is that we have already visited this voxel and it
          // is in the same fragment.
          this->AddEquivalence(&next2, &next);
          }
        // To get the +Y+Z start with the +Z iterator and move +Y put results in "next"
        if (next2.Block)
          {
          this->GetNeighborIterator(&next, &next2, (ii+1)%3,1, (ii+2)%3,0, ii,0);
          if (next.VolumeFractionPointer == 0 || 
              next.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
            {
            // Neighbor is outside of fragment.  Make a face.
            this->CreateFace(&iterator, &next, ii, 0);
            }    
          else if (next.FragmentIdPointer[0] == -1)
            { // We have not visited this neighbor yet. Mark the voxel and recurse.
            *(next.FragmentIdPointer) = this->FragmentId;
            queue->Push(&next);
            }
          else
            { // The last case is that we have already visited this voxel and it
            // is in the same fragment.
            this->AddEquivalence(&next2, &next);
            }
          }
        }

      // "Right"/max
      this->GetNeighborIterator(&next, &iterator, ii,1, (ii+1)%3,0, (ii+2)%3,0);
      if (next.VolumeFractionPointer == 0 ||
          next.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
        { // Neighbor is outside of fragment.  Make a face.
        this->CreateFace(&iterator, &next, ii, 1);
        }
      else if (next.FragmentIdPointer[0] == -1)
        { // We have not visited this neighbor yet. Mark the voxel and recurse.
        *(next.FragmentIdPointer) = this->FragmentId;
        queue->Push(&next);
        }
      else
        { // The last case is that we have already visited this voxel and it
        // is in the same fragment.
        this->AddEquivalence(&iterator, &next);
        }
      // Same case as above with the same logic to visit the
      // four smaller cells that touch this face of the current block.
      if (next.Block && next.Block->GetLevel() > iterator.Block->GetLevel())
        {
        vtkCTHFragmentConnectIterator next2;
        // Take the first neighbor found and move +Y
        this->GetNeighborIterator(&next2, &next, (ii+1)%3,1, (ii+2)%3,0, ii,0);
        if (next2.VolumeFractionPointer == 0 || 
            next2.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
          {
          // Neighbor is outside of fragment.  Make a face.
          this->CreateFace(&iterator, &next2, ii, 1);
          }    
        else if (next2.FragmentIdPointer[0] == -1)
          { // We have not visited this neighbor yet. Mark the voxel and recurse.
          *(next2.FragmentIdPointer) = this->FragmentId;
          queue->Push(&next2);
          }
        else
          { // The last case is that we have already visited this voxel and it
          // is in the same fragment.
          this->AddEquivalence(&next2, &next);
          }
        // Take the fist iterator found and move +Z
        this->GetNeighborIterator(&next2, &next, (ii+2)%3,1, ii,0, (ii+1)%3,0);
        if (next2.VolumeFractionPointer == 0 || 
            next2.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
          {
          // Neighbor is outside of fragment.  Make a face.
          this->CreateFace(&iterator, &next2, ii, 1);
          }    
        else if (next2.FragmentIdPointer[0] == -1)
          { // We have not visited this neighbor yet. Mark the voxel and recurse.
          *(next2.FragmentIdPointer) = this->FragmentId;
          queue->Push(&next2);
          }
        else
          { // The last case is that we have already visited this voxel and it
          // is in the same fragment.
          this->AddEquivalence(&next2, &next);
          }
        // To get the +Y+Z start with the +Z iterator and move +Y put results in "next"
        if (next2.Block)
          {
          this->GetNeighborIterator(&next, &next2, (ii+1)%3,1, (ii+2)%3,0, ii,0);
          if (next.VolumeFractionPointer == 0 || 
              next.VolumeFractionPointer[0] < this->scaledMaterialFractionThreshold)
            {
            // Neighbor is outside of fragment.  Make a face.
            this->CreateFace(&iterator, &next, ii, 1);
            }    
          else if (next.FragmentIdPointer[0] == -1)
            { // We have not visited this neighbor yet. Mark the voxel and recurse.
            *(next.FragmentIdPointer) = this->FragmentId;
            queue->Push(&next);
            }
          else
            { // The last case is that we have already visited this voxel and it
            // is in the same fragment.
            this->AddEquivalence(&next2, &next);
            }
          }
        }
      }
    }
}





//----------------------------------------------------------------------------
int vtkCTHFragmentConnect::FillInputPortInformation(int /*port*/,
                                                    vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
int vtkCTHFragmentConnect::FillOutputPortInformation(int port, vtkInformation *info)
{
  // There are two outputs,
  // 0: multi-block contains fragments
  // 1: polydata contains center of mass points with attributes
  switch (port)
    {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    case 1:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      assert( 0 && "Invalid output port." );
      break;
    }

  return 1;
}


//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO print state
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
// Save out the blocks surfaces so we can se what is creating so many ghost cells.
void vtkCTHFragmentConnect::SaveBlockSurfaces(const char* fileName)
{
  vtkPolyData* pd = vtkPolyData::New();
  vtkPoints* pts = vtkPoints::New();
  vtkCellArray* faces = vtkCellArray::New();
  vtkIntArray* idArray = vtkIntArray::New();
  vtkIntArray* levelArray = vtkIntArray::New();
  vtkCTHFragmentConnectBlock* block;
  const int *ext;
  vtkIdType corners[8];
  vtkIdType face[4];
  double pt[3];
  int level;
  int levelId;
  int ii;
  
  for (ii = 0; ii < this->NumberOfInputBlocks; ++ii)
    {
    block = this->InputBlocks[ii];
    ext = block->GetBaseCellExtent();
    // Ghost blocks do not have spacing.
    //double *spacing;
    //spacing = block->GetSpacing();
    double spacing[3];
    level = block->GetLevel();
    spacing[0] = this->RootSpacing[0] / (double)(1 << level);
    spacing[1] = this->RootSpacing[1] / (double)(1 << level);
    spacing[2] = this->RootSpacing[2] / (double)(1 << level);
    //spacing[0] = spacing[1] = spacing[2] = 1.0 / (double)(1 << level);
    levelId = block->LevelBlockId;
    // Insert the points.
    pt[0] = ext[0]*spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2]*spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4]*spacing[2] + this->GlobalOrigin[2];
    corners[0] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2]*spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4]*spacing[2] + this->GlobalOrigin[2];
    corners[1] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3]+1)*spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4]*spacing[2] + this->GlobalOrigin[2];
    corners[2] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3]+1)*spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4]*spacing[2] + this->GlobalOrigin[2];
    corners[3] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2]*spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5]+1)*spacing[2] + this->GlobalOrigin[2];
    corners[4] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2]*spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5]+1)*spacing[2] + this->GlobalOrigin[2];
    corners[5] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3]+1)*spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5]+1)*spacing[2] + this->GlobalOrigin[2];
    corners[6] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3]+1)*spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5]+1)*spacing[2] + this->GlobalOrigin[2];
    corners[7] = pts->InsertNextPoint(pt);
    // Now the faces.
    face[0] = corners[0];
    face[1] = corners[1];
    face[2] = corners[3];
    face[3] = corners[2];
    faces->InsertNextCell(4, face); // -z
    face[0] = corners[4];
    face[1] = corners[6];
    face[2] = corners[7];
    face[3] = corners[5];
    faces->InsertNextCell(4, face); // +z
    face[0] = corners[0];
    face[1] = corners[4];
    face[2] = corners[5];
    face[3] = corners[1];
    faces->InsertNextCell(4, face); //-y
    face[0] = corners[2];
    face[1] = corners[3];
    face[2] = corners[7];
    face[3] = corners[6];
    faces->InsertNextCell(4, face); //+y
    face[0] = corners[0];
    face[1] = corners[2];
    face[2] = corners[6];
    face[3] = corners[4];
    faces->InsertNextCell(4, face); //-x
    face[0] = corners[1];
    face[1] = corners[5];
    face[2] = corners[7];
    face[3] = corners[3];
    faces->InsertNextCell(4, face); //+x
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    }

  pd->SetPoints(pts);
  pd->SetPolys(faces);
  levelArray->SetName("Level");
  idArray->SetName("LevelBlockId");
  pd->GetCellData()->AddArray(levelArray);
  pd->GetCellData()->AddArray(idArray);

  vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
  w->SetInput(pd);
  w->SetFileName(fileName);
  w->Write();
  w->Delete();

  pd->Delete();
  pts->Delete();
  faces->Delete();
  idArray->Delete();
  levelArray->Delete();
}

//----------------------------------------------------------------------------
// Save out the blocks surfaces so we can se what is creating so many ghost cells.
void vtkCTHFragmentConnect::SaveGhostSurfaces(const char* fileName)
{
  vtkPolyData* pd = vtkPolyData::New();
  vtkPoints* pts = vtkPoints::New();
  vtkCellArray* faces = vtkCellArray::New();
  vtkIntArray* idArray = vtkIntArray::New();
  vtkIntArray* levelArray = vtkIntArray::New();
  vtkCTHFragmentConnectBlock* block;
  const int *ext;
  vtkIdType corners[8];
  vtkIdType face[4];
  double pt[3];
  int level;
  int levelId;
  double spacing;
  

  unsigned int ii;
  
  for (ii = 0; ii < this->GhostBlocks.size(); ++ii)
    {
    block = this->GhostBlocks[ii];
    ext = block->GetBaseCellExtent();
    levelId = ii;
    level = block->GetLevel();
    spacing = 1.0 / (double)(1 << level);
    // Insert the points.
    pt[0] = ext[0]*spacing;
    pt[1] = ext[2]*spacing;
    pt[2] = ext[4]*spacing;
    corners[0] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing;
    pt[1] = ext[2]*spacing;
    pt[2] = ext[4]*spacing;
    corners[1] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing;
    pt[1] = (ext[3]+1)*spacing;
    pt[2] = ext[4]*spacing;
    corners[2] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing;
    pt[1] = (ext[3]+1)*spacing;
    pt[2] = ext[4]*spacing;
    corners[3] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing;
    pt[1] = ext[2]*spacing;
    pt[2] = (ext[5]+1)*spacing;
    corners[4] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing;
    pt[1] = ext[2]*spacing;
    pt[2] = (ext[5]+1)*spacing;
    corners[5] = pts->InsertNextPoint(pt);
    pt[0] = ext[0]*spacing;
    pt[1] = (ext[3]+1)*spacing;
    pt[2] = (ext[5]+1)*spacing;
    corners[6] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1]+1)*spacing;
    pt[1] = (ext[3]+1)*spacing;
    pt[2] = (ext[5]+1)*spacing;
    corners[7] = pts->InsertNextPoint(pt);
    // Now the faces.
    face[0] = corners[0];
    face[1] = corners[1];
    face[2] = corners[3];
    face[3] = corners[2];
    faces->InsertNextCell(4, face); // -z
    face[0] = corners[4];
    face[1] = corners[6];
    face[2] = corners[7];
    face[3] = corners[5];
    faces->InsertNextCell(4, face); // +z
    face[0] = corners[0];
    face[1] = corners[4];
    face[2] = corners[5];
    face[3] = corners[1];
    faces->InsertNextCell(4, face); //-y
    face[0] = corners[2];
    face[1] = corners[3];
    face[2] = corners[7];
    face[3] = corners[6];
    faces->InsertNextCell(4, face); //+y
    face[0] = corners[0];
    face[1] = corners[2];
    face[2] = corners[6];
    face[3] = corners[4];
    faces->InsertNextCell(4, face); //-x
    face[0] = corners[1];
    face[1] = corners[5];
    face[2] = corners[7];
    face[3] = corners[3];
    faces->InsertNextCell(4, face); //+x
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    idArray->InsertNextValue(levelId);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    levelArray->InsertNextValue(level);
    }
    
  pd->SetPoints(pts);
  pd->SetPolys(faces);
  levelArray->SetName("Level");
  idArray->SetName("LevelBlockId");
  pd->GetCellData()->AddArray(levelArray);
  pd->GetCellData()->AddArray(idArray);

  vtkXMLPolyDataWriter* w = vtkXMLPolyDataWriter::New();
  w->SetInput(pd);
  w->SetFileName(fileName);
  w->Write();
  
  w->Delete();

  pd->Delete();
  pts->Delete();
  faces->Delete();
  idArray->Delete();
  levelArray->Delete();
}




//----------------------------------------------------------------------------
// observe PV interface and set modified if user makes changes
void vtkCTHFragmentConnect::SelectionModifiedCallback( vtkObject*, 
                                                       unsigned long,
                                                       void* clientdata,
                                                       void* )
{
  static_cast<vtkCTHFragmentConnect*>(clientdata)->Modified();
}

// PV interface to volume weighted average arrays
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SelectVolumeWtdAvgArray( const char *arrayName )
{
  this->VolumeWtdAvgArraySelection->AddArray( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectVolumeWtdAvgArray( const char *arrayName )
{
  this->VolumeWtdAvgArraySelection->RemoveArrayByName( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectAllVolumeWtdAvgArrays()
{
  this->VolumeWtdAvgArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetNumberOfVolumeWtdAvgArrays()
{
  return this->VolumeWtdAvgArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkCTHFragmentConnect::GetVolumeWtdAvgArrayName(int index)
{
  return this->VolumeWtdAvgArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetVolumeWtdAvgArrayStatus(const char* name)
{
  return this->VolumeWtdAvgArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetVolumeWtdAvgArrayStatus(int index)
{
  return this->VolumeWtdAvgArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetVolumeWtdAvgArrayStatus( const char* name, 
                                                       int status )
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->VolumeWtdAvgArraySelection->EnableArray(name);
    }
  else
    {
    this->VolumeWtdAvgArraySelection->DisableArray(name);
    }
}

// PV interface to volume weighted average arrays
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SelectMassWtdAvgArray( const char *arrayName )
{
  this->MassWtdAvgArraySelection->AddArray( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectMassWtdAvgArray( const char *arrayName )
{
  this->MassWtdAvgArraySelection->RemoveArrayByName( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectAllMassWtdAvgArrays()
{
  this->MassWtdAvgArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetNumberOfMassWtdAvgArrays()
{
  return this->MassWtdAvgArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkCTHFragmentConnect::GetMassWtdAvgArrayName(int index)
{
  return this->MassWtdAvgArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMassWtdAvgArrayStatus(const char* name)
{
  return this->MassWtdAvgArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMassWtdAvgArrayStatus(int index)
{
  return this->MassWtdAvgArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetMassWtdAvgArrayStatus( const char* name, 
                                                       int status )
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->MassWtdAvgArraySelection->EnableArray(name);
    }
  else
    {
    this->MassWtdAvgArraySelection->DisableArray(name);
    }
}




// PV interface to summation arrays
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SelectSummationArray( const char *arrayName )
{
  this->SummationArraySelection->AddArray( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectSummationArray( const char *arrayName )
{
  this->SummationArraySelection->RemoveArrayByName( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectAllSummationArrays()
{
  this->SummationArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetNumberOfSummationArrays()
{
  return this->SummationArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkCTHFragmentConnect::GetSummationArrayName(int index)
{
  return this->SummationArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetSummationArrayStatus(const char* name)
{
  return this->SummationArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetSummationArrayStatus(int index)
{
  return this->SummationArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetSummationArrayStatus( const char* name, 
                                                       int status )
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->SummationArraySelection->EnableArray(name);
    }
  else
    {
    this->SummationArraySelection->DisableArray(name);
    }
}


// PV interface to Material arrays
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SelectMaterialArray( const char *arrayName )
{
  this->MaterialArraySelection->AddArray( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectMaterialArray( const char *arrayName )
{
  this->MaterialArraySelection->RemoveArrayByName( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectAllMaterialArrays()
{
  this->MaterialArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetNumberOfMaterialArrays()
{
  return this->MaterialArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkCTHFragmentConnect::GetMaterialArrayName(int index)
{
  return this->MaterialArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMaterialArrayStatus(const char* name)
{
  return this->MaterialArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMaterialArrayStatus(int index)
{
  return this->MaterialArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetMaterialArrayStatus( const char* name, 
                                                    int status )
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->MaterialArraySelection->EnableArray(name);
    }
  else
    {
    this->MaterialArraySelection->DisableArray(name);
    }
}
// PV interface to Mass arrays
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SelectMassArray( const char *arrayName )
{
  this->MassArraySelection->AddArray( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectMassArray( const char *arrayName )
{
  this->MassArraySelection->RemoveArrayByName( arrayName );
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::UnselectAllMassArrays()
{
  this->MassArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetNumberOfMassArrays()
{
  return this->MassArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkCTHFragmentConnect::GetMassArrayName(int index)
{
  return this->MassArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMassArrayStatus(const char* name)
{
  return this->MassArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkCTHFragmentConnect::GetMassArrayStatus(int index)
{
  return this->MassArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetMassArrayStatus( const char* name, 
                                                    int status )
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->MassArraySelection->EnableArray(name);
    }
  else
    {
    this->MassArraySelection->DisableArray(name);
    }
}


// PV interface to the material fraction
//------------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetMaterialFractionThreshold(double fraction)
{
  vtkDebugMacro( << this->GetClassName() 
                 << " (" << this << "): setting MaterialFractionThreshold to "
                 << fraction );

  if (this->MaterialFractionThreshold != fraction)
    {
    // lower bound on MF @ 0.08
    fraction = fraction < 0.08 ? 0.08 : fraction;
    this->MaterialFractionThreshold = fraction;
    this->scaledMaterialFractionThreshold = 255.0*fraction;
    this->Modified();
    }
}

// PV interface to the upper loading bound
//------------------------------------------------------------------------------
void vtkCTHFragmentConnect::SetUpperLoadingBound(int nPolys)
{
  vtkDebugMacro( << this->GetClassName() 
                 << " (" << this << "): setting UpperLoadingBound to "
                 << nPolys );

  if (this->UpperLoadingBound != nPolys)
    {
    // clamp between -1 and 2G
    nPolys = nPolys < -1 ? -1 : nPolys;
    nPolys = nPolys > 2000000000 ? 2000000000 : nPolys;
    this->UpperLoadingBound=nPolys;
    this->Modified();
    }
}


//----------------------------------------------------------------------------
// This is for finding equivalent fragment ids within a single process.
// I thought of loops and chains but both have problems.  
// Chains can leave orphans, loops break when two nodes in the loop are 
// equated a second time.
// Lets try a directed tree
void vtkCTHFragmentConnect::AddEquivalence(
  vtkCTHFragmentConnectIterator *neighbor1,
  vtkCTHFragmentConnectIterator *neighbor2)
{
  int id1 = *(neighbor1->FragmentIdPointer);
  int id2 = *(neighbor2->FragmentIdPointer);

  if (id1 != id2 && id1 != -1 && id2 != -1)
    {
    this->EquivalenceSet->AddEquivalence(id1, id2);
    }
}


//----------------------------------------------------------------------------
// Merge fragment pieces which are split locally.
void vtkCTHFragmentConnect::ResolveLocalFragmentGeometry()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::ResolveLocalFragmentGeometry("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();

  /// Resolve id's, create local structural information 
  /// and merge split local geometry
  // up until this point a fragment's id was implicit in its location
  // in the mesh array, now the resolution process has invalidated that.
  // Create the ResolvedFragments array such that our fragments are
  // indexed by their global id, if a fragment is not local then
  // its entry is 0. And local pieces of the same fragment have been 
  // merged. As we go, build a list of what we own, so we won't
  // have to search for what we have.
  const int localToGlobal
    = this->LocalToGlobalOffsets[myProcId];

  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert( "Couldn't get the resolved fragnments."
          && resolvedFragments );
  resolvedFragments->SetNumberOfPieces(this->NumberOfResolvedFragments);

  int nFragmentPieces=this->FragmentMeshes.size();
  for (int localId=0; localId<nFragmentPieces; ++localId)
    {
    // find out this guy's global id within this material
    int globalId
      = this->EquivalenceSet->GetEquivalentSetId(localId+localToGlobal);

    // If we have a mesh that is yet unused then
    // we copy, but if not, it's a local piece that has been
    // resolved and we need to append.
    vtkPolyData *destMesh
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));
    vtkPolyData *&srcMesh=this->FragmentMeshes[localId];
    if (destMesh==0)
      {
      resolvedFragments->SetPiece(globalId, srcMesh);
      // make a note that we have a piece of this fragment
      // and assume for now that we are the owner.
      resolvedFragmentIds.push_back(globalId);
      }
    else
      {
      // merge two local pieces
      vtkAppendPolyData *apf=vtkAppendPolyData::New();
      apf->AddInput(destMesh);
      apf->AddInput(srcMesh);
      vtkPolyData *mergedMesh=apf->GetOutput();
      mergedMesh->Update();
      //mergedMesh->Register(0); // Do I have to? no because multi piece does it
      resolvedFragments->SetPiece(globalId, mergedMesh);
      apf->Delete();
      ReleaseVtkPointer(srcMesh);
      //destMesh->Delete(); // no because multi piece does it
      }
    }
  // These have been loaded into the resolved fragments
  ClearVectorOfVtkPointers(this->FragmentMeshes);
  // Squeeze
  vector<int>(resolvedFragmentIds).swap(resolvedFragmentIds);
}

//----------------------------------------------------------------------------
// Merge fragment pieces which are split across processes
// NOTE: this results in extreme memory consumption for large 
// fragments. Do not use. Re-implemented to temp. localize only 
// the points.
// NOTE: Gathering PolyData requires 1-3GB while gathering its
// points only requires 10-100MB....
void vtkCTHFragmentConnect::ResolveRemoteFragmentGeometry()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::ResolveRemoteFragmentGeometry("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  vtkCommunicator *comm=this->Controller->GetCommunicator();

  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert( "Couldn't get the resolved fragnments."
          && resolvedFragments );

  // Here we resolve fragment geomtery that is split across processes.
  // A controlling process gathers structural information regarding
  // the fragment to process distribution, creates a blue print of the
  // moves that must occur to place the split pieces on a single process.
  // These moves are executed, split geomtery is megered and local structural
  // data is updated. After all fragment's geometry entrirely on single process.
  const int controllingProcId=0;
  const int msgBase=200000;

  // anything to resolve?
  if (nProcs==1)
    {
    return;
    }

  vtkCTHFragmentPieceTransactionMatrix TM;
  TM.Initialize(this->NumberOfResolvedFragments,nProcs);

  // controler receives loading information and
  // builds the transaction matrix.
  if (myProcId==controllingProcId)
    {
    int thisMsgId=msgBase;

    // fragment indexed arrays, with number of polys
    vector<vector<vtkIdType> >loadingArrays;
    loadingArrays.resize(nProcs);
    // Gather loading arrays
    // mine
    this->BuildLoadingArray(loadingArrays[controllingProcId]);
    // others
    for (int procId=0; procId<nProcs; ++procId)
      {
      if (procId==controllingProcId)
        {
        continue;
        }
      // size of incoming
      int bufSize=0;
      comm->Receive(&bufSize,1,procId,thisMsgId);
      // incoming
      vtkIdType *buffer=new vtkIdType [bufSize];
      comm->Receive(buffer,bufSize,procId,thisMsgId+1);
      this->UnPackLoadingArray(buffer,bufSize,loadingArrays[procId]);
      delete [] buffer;
      }
    ++thisMsgId;
    ++thisMsgId;

    #ifdef vtkCTHFragmentConnectDEBUG
    // cerr << "[" << __LINE__ << "] "
    //       << controllingProcId
    //       << " loading arrays:" 
    //       << endl;
    // cerr << "[" << __LINE__ << "] "
    //       << loadingArrays 
    //       << endl;
    #endif

    // Build fragment to proc map, and priority queue
    vtkCTHFragmentToProcMap f2pm;
    f2pm.Initialize(nProcs,this->NumberOfResolvedFragments);
    vtkCTHFragmentProcessPriorityQueue Q;
    Q.Initialize(nProcs);
    vtkCTHFragmentProcessLoading *heap=Q.GetHeap();
    for (int procId=0; procId<nProcs; ++procId)
      {
      // sum up the loading contribution from all local fragments
      // and make a note of who owns what.
      vtkCTHFragmentProcessLoading &prl=heap[procId+1]; // indexed from 1
      for (int fragmentId=0;
           fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
        {
        vtkIdType loading=loadingArrays[procId][fragmentId];
        if (loading>0)
          {
          prl.UpdateLoadFactor(loading);
          f2pm.SetProcOwnsPiece(procId,fragmentId);
          }
        }
      }
    // sort the processes by loading
    Q.InitialHeapify();
    // 
    #ifdef vtkCTHFragmentConnectDEBUG
    vtkIdType loadingBefore=Q.GetTotalLoading();
    cerr << "[" << __LINE__ << "] "
         << controllingProcId
         << " total loading "
         << loadingBefore/1E6
         << " M polys."
         << endl;
    Q.PrintProcessLoading();
    vector<int> splitting(nProcs+1,0);
    int nSplit=0;
    #endif

    // Decide who needs to move and build coresponding 
    // transaction matrix
    for (int fragmentId=0; fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
      {
      // if the fragment is split then we need to move him
      int nSplitOver=f2pm.GetProcCount(fragmentId);
      if (nSplitOver>1)
        {
        #ifdef vtkCTHFragmentConnectDEBUG
        ++splitting[nSplitOver];
        ++nSplit;
        #endif
        // who has the pieces?
        vector<int> owners=f2pm.WhoHasAPiece(fragmentId);
        // how much load will he add to the recipient?
        vtkIdType loading=0;
        for (int i=0; i<nSplitOver; ++i)
          {
          loading+=loadingArrays[owners[i]][fragmentId];
          }
        // who will take him in??
        int recipient=Q.AssignFragmentToProcess(loading);

        // Add the transactions to make the move, and
        // update current owners loading to reflect the loss.
        vtkCTHFragmentPieceTransaction ta;
        for (int i=0; i<nSplitOver; ++i)
          {
          // Update loading of the previous owner.
          vtkIdType loss=loadingArrays[owners[i]][fragmentId];
          Q.UpdateProcessLoadFactor(owners[i],-loss);
          // need to move this piece?
          if (owners[i]==recipient)
            {
            continue;
            }
          // Add the requisite transactions.
          // recipient executes a recv from owner
          ta.Initialize('R',owners[i]);
          TM.PushTransaction(fragmentId,recipient,ta);
          // owner executes a send to recipient
          ta.Initialize('S',recipient);
          TM.PushTransaction(fragmentId,owners[i],ta);
          }
        }
      }
      #ifdef vtkCTHFragmentConnectDEBUG
      vtkIdType loadingAfter=Q.GetTotalLoading();
      assert("Laoding has changed during construction "
             "of the transaction matrix."
             && loadingAfter==loadingBefore );
      cerr << "[" << __LINE__ << "] "
           << controllingProcId
           << " total loading after fragment localization "
           << loadingBefore/1E6
           << " M polys."
           << endl;
      Q.PrintProcessLoading();
      cerr << "[" << __LINE__ << "] "
           << controllingProcId
           << " splitting histogram:"
           << endl;
      PrintHistogram(splitting);
      cerr << "[" << __LINE__ << "] "
           << myProcId
           << " total number of fragments "
           << this->NumberOfResolvedFragments
           << endl;
      cerr << "[" << __LINE__ << "] "
           << myProcId
           << " total number split "
           << nSplit
           << endl;
      // cerr << "[" << __LINE__ << "] "
      //      << myProcId
      //      << " the transaction matrix is:" << endl;
      // TM.Print();
      #endif
    }
  // All processes send fragment loading to controller.
  else
    {
    int thisMsgId=msgBase;

    // create and send my loading array
    vtkIdType *buffer=0;
    int bufSize=this->PackLoadingArray(buffer);
    comm->Send(&bufSize,1,controllingProcId,thisMsgId);
    ++thisMsgId;
    comm->Send(buffer,bufSize,controllingProcId,thisMsgId);
    ++thisMsgId;
    }

  // Brodcast the transaction matrix
  TM.Broadcast(comm, controllingProcId);

  // Execute transactions
  for (int fragmentId=0; fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
    {
    // get my list of transactions
    vector<vtkCTHFragmentPieceTransaction> &transactionList
      = TM.GetTransactions(fragmentId,myProcId);
    // execute
    int nTransactions=transactionList.size();
    for (int i=0; i<nTransactions; ++i)
      {
      vtkCTHFragmentPieceTransaction &ta=transactionList[i];
      /// send
      if (ta.GetType()=='S')
        {
        vtkPolyData *localMesh
          = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(fragmentId));
        assert( "Send transaction requires a mesh that is not local." 
                && localMesh!=0 );
        comm->Send(localMesh, ta.GetRemoteProc(), fragmentId);
        // update local structural info
        resolvedFragments->SetPiece(fragmentId, static_cast<vtkPolyData *>(0));
        vector<int>::iterator curEnd=resolvedFragmentIds.end();
        vector<int>::iterator newEnd
          = vtkstd::remove(resolvedFragmentIds.begin(),curEnd,fragmentId);
        resolvedFragmentIds.erase(newEnd,curEnd);
        assert( "More than one piece found." && (curEnd-newEnd)==1 );
        }
      /// receive
      else if (ta.GetType()=='R')
        {
        vtkPolyData *remoteMesh=vtkPolyData::New();
        comm->Receive(remoteMesh,ta.GetRemoteProc(),fragmentId);
        // If we have a mesh that is yet unused then
        // we copy, but if not, it's a local piece that has been
        // resolved and we need to append.
        vtkPolyData *localMesh
          = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(fragmentId));
        if (localMesh==0)
          {
          // save
          resolvedFragments->SetPiece(fragmentId, remoteMesh);
          // make a note we own this guy
          resolvedFragmentIds.push_back(fragmentId);
          }
        else
          {
          // merge two fragment pieces
          // It's more effcient to batch the appends, but the
          // majority of split fragments only have two pieces.
          vtkAppendPolyData *apf=vtkAppendPolyData::New();
          apf->AddInput(localMesh);
          apf->AddInput(remoteMesh);
          vtkPolyData *mergedMesh=apf->GetOutput();
          mergedMesh->Update();
          //mergedMesh->Register(0); no because multi piece does it
          resolvedFragments->SetPiece(fragmentId, mergedMesh);
          apf->Delete();
          //localMesh->Delete(); no because multi piece does it
          }
        remoteMesh->Delete();
        }
      else
        {
        assert("Invalid transaction type." && 0);
        }
      }
    }
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " fragment localization completed."
       << endl;
  #endif
}

//----------------------------------------------------------------------------
// Find the local id given a global id to a piece that we own. 
// If the piece is not local then return is less than 0.
int vtkCTHFragmentConnect::GlobalToLocalId(int globalId)
{
  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  int nLocal=resolvedFragmentIds.size();
  for (int i=0; i<nLocal; ++i)
    {
    if (resolvedFragmentIds[i]==globalId)
      {
      return i;
      }
    }

  return -1;
}
//----------------------------------------------------------------------------
// Identify fragments who are split across processes. These are always
// generated and coppied to the output. We also may compute various
// other geometric attributes here. For fragments whose geometry is 
// split across processes, localize points and compute geometric 
// attributes. eg OBB, AABB center.
void vtkCTHFragmentConnect::ComputeGeometricAttributes()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::ComputeGeometricAttributes("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  vtkCommunicator *comm=this->Controller->GetCommunicator();
  const int controllingProcId=0;
  const int msgBase=200000;

  // Here are the fragment pieces we own. Some are completely
  // localized while others are split across processes. Pieces
  // with split geometry will be temporarily localized(via copy), the 
  // geometry acted on as a whole and the results distributed back 
  // to piece owners. For localized fragments(these aren't pieces) 
  // the situation/ is relatively simple--computation can be made 
  // directly.
  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert( "Couldn't get the resolved fragnments."
          && resolvedFragments );

  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];
  int nLocal=resolvedFragmentIds.size();

  // Start by assuming that all fragment pieces
  // are local(these are not peices but entire fragments).
  // If we are are running client-server later we search
  // for split fragments and mark them.For any remaining
  // un-marked fragments we can compute geometric attributes
  // directly.
  vector<int> &fragmentSplitMarker
    = this->FragmentSplitMarker[this->MaterialId];
  // intilize to (bool)0 not split. ALl local ops 
  // on fragments check here. It's coppied to geometry.
  fragmentSplitMarker.resize(nLocal,0);
  //
  if (myProcId==controllingProcId)
    {
    NewVtkArrayPointer(
          this->FragmentSplitGeometry,
          1,
          this->NumberOfResolvedFragments,
          "SplitGeometry");
    // initialize to 1 (i.e. not split). This holds n-way 
    // splitting data, only available on controller. It's
    // coppied tothe stats output.
    this->FragmentSplitGeometry->FillComponent(0,1);
    }

  // Are we going to be computing anything? Or
  // just getting the split geometry markers?
  if (!this->ComputeMoments || this->ComputeOBB)
    {
    // Size the attribute arrays.
    if (!this->ComputeMoments)
      {
      this->FragmentAABBCenters->SetNumberOfComponents(3);
      this->FragmentAABBCenters->SetNumberOfTuples(nLocal);
      }
    if (this->ComputeOBB)
      {
      this->FragmentOBBs->SetNumberOfComponents(15);
      this->FragmentOBBs->SetNumberOfTuples(nLocal);
      }
    }

  // Here we localize fragment geomtery that is split across processes.
  // A controlling process gathers structural information regarding
  // the fragment to process distribution, creates a blue print of the
  // moves that must occur to place the split pieces on a single process.
  // These moves are made, computations made then results are sent
  // back to piece owners.
  if (nProcs>1)
    {
    vtkCTHFragmentPieceTransactionMatrix TM;
    TM.Initialize(this->NumberOfResolvedFragments,nProcs);

    // controler receives loading information and
    // builds the transaction matrix.
    if (myProcId==controllingProcId)
      {
      int thisMsgId=msgBase;

      // fragment indexed arrays, with number of polys
      vector<vector<vtkIdType> >loadingArrays;
      loadingArrays.resize(nProcs);
      // Gather loading arrays
      // mine
      this->BuildLoadingArray(loadingArrays[controllingProcId]);
      // others
      for (int procId=0; procId<nProcs; ++procId)
        {
        if (procId==controllingProcId)
          {
          continue;
          }
        // size of incoming
        int bufSize=0;
        comm->Receive(&bufSize,1,procId,thisMsgId);
        // incoming
        vtkIdType *buffer=new vtkIdType [bufSize];
        comm->Receive(buffer,bufSize,procId,thisMsgId+1);
        this->UnPackLoadingArray(buffer,bufSize,loadingArrays[procId]);
        delete [] buffer;
        }
      ++thisMsgId;
      ++thisMsgId;
      #ifdef vtkCTHFragmentConnectDEBUG
      cerr << "[" << __LINE__ << "] "
            << controllingProcId
            << " loading histogram:" 
            << endl;
      PrintPieceLoadingHistogram(loadingArrays);
      #endif

      // Build fragment to proc map, and priority queue
      vtkCTHFragmentToProcMap f2pm;
      f2pm.Initialize(nProcs,this->NumberOfResolvedFragments);
      vtkCTHFragmentProcessPriorityQueue Q;
      Q.Initialize(nProcs);
      vtkCTHFragmentProcessLoading *heap=Q.GetHeap();
      for (int procId=0; procId<nProcs; ++procId)
        {
        // sum up the loading contribution from all local fragments
        // and make a note of who owns what.
        vtkCTHFragmentProcessLoading &prl=heap[procId+1]; // indexed from 1
        for (int fragmentId=0;
            fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
          {
          vtkIdType loading=loadingArrays[procId][fragmentId];
          if (loading>0)
            {
            prl.UpdateLoadFactor(loading);
            f2pm.SetProcOwnsPiece(procId,fragmentId);
            }
          }
        }
      // Build minimum order heap.
      Q.InitialHeapify();
      #ifdef vtkCTHFragmentConnectDEBUG
      vtkIdType loadingBefore=Q.GetTotalLoading();
      cerr << "[" << __LINE__ << "] "
          << controllingProcId
          << " total loading before fragment localization "
          << loadingBefore/1E6
          << " M polys."
          << endl;
      Q.PrintProcessLoading();
      vector<int> splitting(nProcs+1,0);
      int nSplit=0;
      #endif

      // Who will do attribute processing for fragments
      // that are split? We can weed out processes that are
      // highly loaded and cycle through the group that 
      // remains assigning to each until all work has
      // been alloted.
      vtkCTHFragmentProcessRingBuffer procRing;
      procRing.Initialize(Q, this->UpperLoadingBound);
      #ifdef vtkCTHFragmentConnectDEBUG
      cerr << "[" << __LINE__ << "] "
          << controllingProcId
          << " process ring: ";
      procRing.Print();
      cerr << endl;
      #endif

      // Decide who needs to move and build coresponding 
      // transaction matrix. Along the way store the geometry 
      // split info (this eliminates some communication later).
      int *fragmentSplitGeometry
        = this->FragmentSplitGeometry->GetPointer(0);
      //
      for (int fragmentId=0; fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
        {
        int nSplitOver=f2pm.GetProcCount(fragmentId);
        fragmentSplitGeometry[fragmentId]=nSplitOver;
        // if the fragment is split then we need to move him
        if (nSplitOver>1)
          {
          #ifdef vtkCTHFragmentConnectDEBUG
          // splitting histogram
          ++splitting[nSplitOver];
          ++nSplit;
          #endif
          // who has the pieces?
          vector<int> owners=f2pm.WhoHasAPiece(fragmentId);
          // how much load will he add to the recipient?
          vtkIdType loading=0;
          for (int i=0; i<nSplitOver; ++i)
            {
            loading+=loadingArrays[owners[i]][fragmentId];
            }
          // who will do processing??
          int recipient=procRing.GetNextId();

          // Add the transactions to make the move, and
          // update current owners loading to reflect the loss.
          vtkCTHFragmentPieceTransaction ta;
          for (int i=0; i<nSplitOver; ++i)
            {
            // need to move this piece?
            if (owners[i]==recipient)
              {
              continue;
              }
            // Add the requisite transactions.
            // recipient executes a recv from owner
            ta.Initialize('R',owners[i]);
            TM.PushTransaction(fragmentId,recipient,ta);
            // owner executes a send to recipient
            ta.Initialize('S',recipient);
            TM.PushTransaction(fragmentId,owners[i],ta);
            }
          }
        }
        #ifdef vtkCTHFragmentConnectDEBUG
        cerr << "[" << __LINE__ << "] "
            << controllingProcId
            << " splitting:"
            << endl;
        PrintHistogram(splitting);
        cerr << "[" << __LINE__ << "] "
            << myProcId
            << " total number of fragments "
            << this->NumberOfResolvedFragments
            << endl;
        cerr << "[" << __LINE__ << "] "
            << myProcId
            << " total number split "
            << nSplit
            << endl;
        // cerr << "[" << __LINE__ << "] "
        //       << myProcId
        //       << " the transaction matrix is:" << endl;
        // TM.Print();
        #endif
      }
    // All processes send fragment loading to controller.
    else
      {
      int thisMsgId=msgBase;

      // create and send my loading array
      vtkIdType *buffer=0;
      int bufSize=this->PackLoadingArray(buffer);
      comm->Send(&bufSize,1,controllingProcId,thisMsgId);
      ++thisMsgId;
      comm->Send(buffer,bufSize,controllingProcId,thisMsgId);
      ++thisMsgId;
      }

    // Brodcast the transaction matrix
    TM.Broadcast(comm, controllingProcId);

    int nAttributeComps=0;
    if (!this->ComputeMoments)
      {
      nAttributeComps+=3;
      }
    vtkOBBTree *obbCalc=0;
    if (this->ComputeOBB)
      {
      obbCalc=vtkOBBTree::New();
      nAttributeComps+=15;
      }
    double *attributeCommBuffer=new double[nAttributeComps];
    // localize split geometry and compute attributes.
    for (int fragmentId=0; fragmentId<this->NumberOfResolvedFragments; ++fragmentId)
      {
      // point buffer
      vtkPointAccumulator<float, vtkFloatArray> accumulator;

      // get my list of transactions for this fragment
      vector<vtkCTHFragmentPieceTransaction> &transactionList
        = TM.GetTransactions(fragmentId,myProcId);

      // execute
      vtkPolyData *localMesh
        = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(fragmentId));

      int nTransactions=transactionList.size();
      if (nTransactions>0)
        {
        /// send
        if (transactionList[0].GetType()=='S')
          {
          assert("Send has more than 1 transaction."
                && nTransactions==1);
          assert("Send requires a mesh that is not local."
                && localMesh!=0 );

          // I am sending geometry, hence this is a piece
          // of a split frgament and I need to treat it as 
          // such from now on.
          int localId=this->GlobalToLocalId(fragmentId);
          fragmentSplitMarker[localId]=1;

          // Send the geometry and recvieve the results of 
          // the requested computations.
          if (!this->ComputeMoments || this->ComputeOBB)
            {
            vtkCTHFragmentPieceTransaction &ta=transactionList[0];

            // get the points of this piece
            vtkFloatArray *ptsArray
              = dynamic_cast<vtkFloatArray *>(localMesh->GetPoints()->GetData());
            const vtkIdType bytesPerPoint=3*sizeof(float);
            const vtkIdType nPoints=ptsArray->GetNumberOfTuples();
            const vtkIdType bufferSize=bytesPerPoint*nPoints;
            // prepare a comm buffer
            vtkCTHFragmentCommBuffer buffer;
            buffer.Initialize(myProcId,1,bufferSize);
            buffer.SetNumberOfTuples(0,nPoints);
            buffer.Pack(ptsArray);
            // send the buffer in two parts, first the header
            this->Controller->Send(
                    buffer.GetHeader(),
                    buffer.GetHeaderSize(),
                    ta.GetRemoteProc(),
                    fragmentId);
            // followed by points
            this->Controller->Send(
                    buffer.GetBuffer(),
                    buffer.GetBufferSize(),
                    ta.GetRemoteProc(),
                    2*fragmentId);

            // Recieve the remotely computed attributes
            this->Controller->Receive(
                    attributeCommBuffer,
                    nAttributeComps,
                    ta.GetRemoteProc(),
                    3*fragmentId);
            // save results
            double *pBuf=attributeCommBuffer;
            if (!this->ComputeMoments)
              {
              this->FragmentAABBCenters->SetTuple(localId,pBuf);
              pBuf+=3;
              }
            if (this->ComputeOBB)
              {
              this->FragmentOBBs->SetTuple(localId,pBuf);
              pBuf+=15;
              }
            }
          }
        /// receive
        else if (transactionList[0].GetType()=='R')
          {
          // This fragment is split across processes and
          // I have a piece. From now on I need to treat 
          // this fragment as split.
          int localId=-1;
          if (localMesh!=0)
            {
            localId=this->GlobalToLocalId(fragmentId);
            fragmentSplitMarker[localId]=1;
            }

          // Receive the geometry, perform the requested 
          // computations and send back the results.
          if (!this->ComputeMoments || this->ComputeOBB)
            {
            for (int i=0; i<nTransactions; ++i)
              {
              vtkCTHFragmentPieceTransaction &ta=transactionList[i];

              // prepare the comm buffer to receive attribute data
              // pertaining to a single block(material)
              vtkCTHFragmentCommBuffer buffer;
              buffer.SizeHeader(1);
              // recieve buffer's header
              this->Controller->Receive(
                      buffer.GetHeader(),
                      buffer.GetHeaderSize(),
                      ta.GetRemoteProc(),
                      fragmentId);
              // size buffer via incoming header
              buffer.SizeBuffer();
              // receive points
              this->Controller->Receive(
                      buffer.GetBuffer(),
                      buffer.GetBufferSize(),
                      ta.GetRemoteProc(),
                      2*fragmentId);
              // unpack points with an explicit copy
              vtkIdType nPoints=buffer.GetNumberOfTuples(0);
              float *writePointer=accumulator.Expand(nPoints);
              buffer.UnPack(writePointer,3,nPoints,true);
              }
            // append points that I own.
            if (localMesh!=0)
              {
              // get the points
              vtkFloatArray *ptsArray
                = dynamic_cast<vtkFloatArray *>(localMesh->GetPoints()->GetData());
              // append
              accumulator.Accumulate(ptsArray);
              }

            // Get points refernece to the gathered points.
            vtkPoints *localizedPoints=accumulator.BuildVtkPoints();

            // Compute geometric attributes.
            double *pBuf=attributeCommBuffer;
            if (!this->ComputeMoments)
              {
              // Compute center of AABB
              double bounds[6];
              localizedPoints->GetBounds(bounds);
              for (int q=0,k=0; q<3; ++q, k+=2)
                {
                pBuf[q]=(bounds[k]+bounds[k+1])/2.0;
                }
              pBuf+=3;
              }
            if (this->ComputeOBB)
              {
              // Compute OBB
              double size[3];
              // I store the results as follows:
              // (c_x,c_y,c_z),(max_x,max_y,max_z),(mid_x,mid_y,mid_z),(min_x,min_y,min_z),|max|,|mid|,|min|
              obbCalc->ComputeOBB(localizedPoints,pBuf,pBuf+3,pBuf+6,pBuf+9,size);
              // compute magnitudes
              for (int q=0; q<3; ++q)
                {
                pBuf[12+q]=0;
                }
              for (int q=0; q<3; ++q)
                {
                pBuf[12]+=pBuf[3+q]*pBuf[3+q];
                pBuf[13]+=pBuf[6+q]*pBuf[6+q];
                pBuf[14]+=pBuf[9+q]*pBuf[9+q];
                }
              for (int q=0; q<3; ++q)
                {
                pBuf[12+q]=sqrt(pBuf[12+q]);
                }
              pBuf+=15;
              }
            // send attributes back to piece owners
            for (int i=0; i<nTransactions; ++i)
              {
              vtkCTHFragmentPieceTransaction &ta=transactionList[i];
              this->Controller->Send(
                      attributeCommBuffer,
                      nAttributeComps,
                      ta.GetRemoteProc(),
                      3*fragmentId);
              }
            // If I own a piece save the results, and mark 
            // piece as split.
            if (localMesh!=0)
              {
              pBuf=attributeCommBuffer;
              if (!this->ComputeMoments)
                {
                this->FragmentAABBCenters->SetTuple(localId,pBuf);
                pBuf+=3;
                }
              if (this->ComputeOBB)
                {
                this->FragmentOBBs->SetTuple(localId,pBuf);
                pBuf+=15;
                }
              }
            #ifdef vtkCTHFragmentConnectDEBUG
            if (nTransactions>=4)
              {
              cerr << "[" << __LINE__ << "] "
                  << myProcId
                  << " memory commitment during localization of "
                  << nTransactions
                  << " pieces is:"
                  << endl
                  << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
              }
            #endif
            accumulator.Clear();
            }
          }
        else
          {
          assert("Invalid transaction type." && 0);
          }
        }
      }
    // Clean up.
    delete [] attributeCommBuffer;
    if (this->ComputeOBB)
      {
      obbCalc->Delete();
      }
    }

  // At this poiint we have identified all split frafgments
  // tenmporarily localized their geometry, computed the attributes
  // and sent results back to piece owners. Now compute geometric
  // attributes for remianing local fragments.
  if (!this->ComputeMoments)
    {
    this->ComputeLocalFragmentAABBCenters();
    }
  if (this->ComputeOBB)
    {
    this->ComputeLocalFragmentOBB();
    }

  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " computation of geometric attributes completed."
       << endl;
  #endif
}



//----------------------------------------------------------------------------
// Build the loading array for the fragment pieces that we own.
void vtkCTHFragmentConnect::BuildLoadingArray(
                vector<vtkIdType> &loadingArray)
{
  vtkMultiPieceDataSet *resolvedFragments
      = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));

  int nLocal=this->ResolvedFragmentIds[this->MaterialId].size();
  loadingArray.clear();
  loadingArray.resize(this->NumberOfResolvedFragments,0);
  for (int i=0; i<nLocal; ++i)
    {
    int globalId=this->ResolvedFragmentIds[this->MaterialId][i];

    vtkPolyData *fragment
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));

    loadingArray[globalId]=fragment->GetNumberOfPolys();
    }
}

//----------------------------------------------------------------------------
// Load a buffer containg the number of polys for each fragment
// or fragment piece that we own. Return the size in vtkIdType's
// of the packed buffer and the buffer itself. Pass in a
// pointer intialized to null, allocation is internal.
int vtkCTHFragmentConnect::PackLoadingArray(vtkIdType *&buffer)
{
  assert( "Buffer appears to have been pre-allocated."
          && buffer==0 );

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));

  int nLocal=this->ResolvedFragmentIds[this->MaterialId].size();

  vtkCTHFragmentPieceLoading pl;
  const int bufSize=pl.SIZE*nLocal;
  buffer = new vtkIdType [bufSize];
  vtkIdType *pBuf=buffer;
  for (int i=0; i<nLocal; ++i)
    {
    int globalId=this->ResolvedFragmentIds[this->MaterialId][i];

    vtkPolyData *fragment
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));

    pl.Initialize(globalId,fragment->GetNumberOfPolys());
    pl.Pack(pBuf);
    pBuf+=pl.SIZE;
    }

  return bufSize;
}

//----------------------------------------------------------------------------
// Given a fragment loading array that has been packed into an int array
// unpack. The packed loading arrays are orderd locally while the unpacked
// are ordered gloabbly by fragment id. NOTE if memory is an issue you
// could leave these locally indexed and search, rather than look up.
//
// Return the number of fragments and the unpacked array.
int vtkCTHFragmentConnect::UnPackLoadingArray(
                vtkIdType *buffer,
                int bufSize,
                vector<vtkIdType> &loadingArray)
{
  const int sizeOfPl=vtkCTHFragmentPieceLoading::SIZE;

  assert( "Buffer is null pointer." && buffer!=0 );
  assert( "Buffer size is incorrect." && bufSize%sizeOfPl==0 );

  loadingArray.clear();
  loadingArray.resize(this->NumberOfResolvedFragments,0);
  vtkIdType *pBuf=buffer;
  const int nPieces=bufSize/sizeOfPl;
  for (int i=0; i<nPieces; ++i)
    {
    vtkCTHFragmentPieceLoading pil;
    pil.UnPack(pBuf);
    loadingArray[pil.GetId()]=pil.GetLoading();
    pBuf+=sizeOfPl;
    }

  return nPieces;
}

//----------------------------------------------------------------------------
// Receive all geomteric attribute arrays from all other
// processes. Conatiners filled with the expected number
// of empty data arrays/pointers are expected.
//
// return 0 on error
int vtkCTHFragmentConnect::CollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&coaabb,
                vector<vtkDoubleArray *>&obb,
                vector<int *> &ids)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();
  const int msgBase=200000;

  // anything to do?
  if  (this->ComputeMoments 
       && !this->ComputeOBB)
    {
    return 1;
    }

  // size buffer's header's in preparation for
  // recieve. There's a buffer for each proc and
  // we're working with a single block(material) at a time.
  vtkCTHFragmentCommBuffer::SizeHeader(buffers,1);

  // gather
  for (int procId=0; procId<nProcs; ++procId)
    {
    // skip mine
    if (procId==myProcId)
      {
      continue;
      }
    int thisMsgId=msgBase;
    // receive buffer header
    this->Controller->Receive(
            buffers[procId].GetHeader(),
            buffers[procId].GetHeaderSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // size buffer to hold incomming attributes
    buffers[procId].SizeBuffer();
    // receive attribute's data
    this->Controller->Receive(
            buffers[procId].GetBuffer(),
            buffers[procId].GetBufferSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // unpack 
    // here we are setting pointer into the buffer rather than
    // explicitly copying the data.
    const unsigned int nToUnpack
      = buffers[procId].GetNumberOfTuples(0);
    // coaabb
    if (!this->ComputeMoments)
      {
      buffers[procId].UnPack(coaabb[procId],3,nToUnpack,false);
      }
    // obb
    if (this->ComputeOBB)
      {
      const unsigned int nCompsObb
        = this->FragmentOBBs->GetNumberOfComponents();
      buffers[procId].UnPack(obb[procId],nCompsObb,nToUnpack,false);
      }
    // ids
    buffers[procId].UnPack(ids[procId],1,nToUnpack,false);
    }
  return 1;
}

//----------------------------------------------------------------------------
// Send all geometric attributes for the fragments which I own
// to another process.
//
// return 0 on error
int vtkCTHFragmentConnect::SendGeometricAttributes(const int recipientProcId)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int msgBase=200000;

  // anything to do?
  if  (this->ComputeMoments 
       && !this->ComputeOBB)
    {
    return 1;
    }

  // initialize buffer
  // were working with a single block(material) at a time
  // sized in bytes, and we have to make note of how many
  // fragments we are putting in.
  const unsigned int nLocal
    = this->ResolvedFragmentIds[this->MaterialId].size();
  int totalNumberOfComps
    = (!this->ComputeMoments ? 3 : 0)   // coaabb + obb
      + (this->ComputeOBB ? this->FragmentOBBs->GetNumberOfComponents(): 0);
  const vtkIdType bufferSize  // coaabb(double) + obb(double) + ids(int)
    = nLocal*(sizeof(int)+totalNumberOfComps*sizeof(double));
  //
  vtkCTHFragmentCommBuffer buffer;
  buffer.Initialize(myProcId,1,bufferSize);
  buffer.SetNumberOfTuples(0,nLocal);

  // pack attributes into buffer
  // center of aabb
  if (!this->ComputeMoments)
    {
    buffer.Pack(this->FragmentAABBCenters);
    }
  // obb
  if (this->ComputeOBB)
    {
    buffer.Pack(this->FragmentOBBs);
    }
  // resolved fragment ids
  buffer.Pack(&this->ResolvedFragmentIds[this->MaterialId][0],1,nLocal);

  // send buffer in two parts. header then data
  int thisMsgId=msgBase;
  // buffer's header
  this->Controller->Send(
          buffer.GetHeader(),
          buffer.GetHeaderSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;
  // attribute's data
  this->Controller->Send(
          buffer.GetBuffer(),
          buffer.GetBufferSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;

  return 1;
}
//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
//
// return 0 on error
int vtkCTHFragmentConnect::PrepareToCollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&coaabb,
                vector<vtkDoubleArray *>&obb,
                vector<int *> &ids)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // buffers
  buffers.resize(nProcs);
  // coaabb
  if (!this->ComputeMoments)
    {
    ResizeVectorOfVtkPointers(coaabb,nProcs);
    coaabb[myProcId]->Delete();
    coaabb[myProcId]=this->FragmentAABBCenters;
    }
  // obb
  if (this->ComputeOBB)
    {
    ResizeVectorOfVtkPointers(obb,nProcs);
    obb[myProcId]->Delete();
    obb[myProcId]=this->FragmentOBBs;
    }
  // ids
  ids.resize(nProcs,static_cast<int *>(0));
  ids[myProcId]=&this->ResolvedFragmentIds[this->MaterialId][0];
  // note, this could be a problem if we need to update ResolvedFragmentIds
  // but we don't need to since after the gather we have everything which
  // is just sequential 0-nFragmentsResolvedFragments. Also we do need to 
  // preseve this so we know which fragment geometry we own.

  return 1;
}

//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
//
// return 0 on error
int vtkCTHFragmentConnect::CleanUpAfterCollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&coaabb,
                vector<vtkDoubleArray *>&obb,
                vector<int *> &ids)
{
  // coaabb
  if (!this->ComputeMoments)
    {
    ClearVectorOfVtkPointers(coaabb);
    }
  // obb
  if (this->ComputeOBB)
    {
    ClearVectorOfVtkPointers(obb);
    }
  // ids
  ids.clear(); // these point to existing data, do not delete
  // buffers
  buffers.clear();

  return 1;
}

//----------------------------------------------------------------------------
// Set up local arrays for merge.
//
// return 0 on error.
int vtkCTHFragmentConnect::PrepareToMergeGeometricAttributes()
{
  // coaabb
  if (!this->ComputeMoments)
    {
    NewVtkArrayPointer(
        this->FragmentAABBCenters,
        3,
        this->NumberOfResolvedFragments,
        this->FragmentAABBCenters->GetName());
    }
  // obb
  if (this->ComputeOBB)
    {
    NewVtkArrayPointer(
        this->FragmentOBBs,
        this->FragmentOBBs->GetNumberOfComponents(),
        this->NumberOfResolvedFragments,
        this->FragmentOBBs->GetName());
    }

  return 1;
}
//----------------------------------------------------------------------------
// Gather all geometric attributes to a single process.
// 
// return 0 on error
int vtkCTHFragmentConnect::GatherGeometricAttributes(
                const int recipientProcId)
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::gatherGeometricAttributes("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  if (myProcId==recipientProcId)
    {
    // collect
    vector<vtkCTHFragmentCommBuffer>buffers;
    vector<vtkDoubleArray *>coaabb;
    vector<vtkDoubleArray *>obb;
    vector<int *>ids;
    this->PrepareToCollectGeometricAttributes(buffers,coaabb,obb,ids);
    this->CollectGeometricAttributes(buffers,coaabb,obb,ids);
    // merge
    this->PrepareToMergeGeometricAttributes();
    for (int procId=0; procId<nProcs; ++procId)
      {
      vtkIdType nFromRemoteProc;
      double *pRemote;
      double *pLocal;
      int nComps;
      // coaaabb
      if (!this->ComputeMoments)
        {
        nFromRemoteProc=coaabb[procId]->GetNumberOfTuples();
        pRemote=coaabb[procId]->GetPointer(0);
        pLocal=this->FragmentAABBCenters->GetPointer(0);
        nComps=3;
        for (vtkIdType i=0; i<nFromRemoteProc; ++i)
          {
          int resIdx=nComps*ids[procId][i];
          for (int q=0; q<nComps; ++q)
            {
            pLocal[resIdx+q]=pRemote[q];
            }
          pRemote+=nComps;
          }
        }
      // obb
      if (this->ComputeOBB)
        {
        nFromRemoteProc=obb[procId]->GetNumberOfTuples();
        pRemote=obb[procId]->GetPointer(0);
        pLocal=this->FragmentOBBs->GetPointer(0);
        nComps=this->FragmentOBBs->GetNumberOfComponents();
        for (vtkIdType i=0; i<nFromRemoteProc; ++i)
          {
          int resIdx=nComps*ids[procId][i];
          for (int q=0; q<nComps; ++q)
            {
            pLocal[resIdx+q]=pRemote[q];
            }
          pRemote+=nComps;
          }
        }
      }
    this->CleanUpAfterCollectGeometricAttributes(buffers,coaabb,obb,ids);
    }
  else
    {
    this->SendGeometricAttributes(recipientProcId);
    }

  return 1;
}


//----------------------------------------------------------------------------
// Collect all of  integrated attribute arrays from all
// other processes. Need to pass in containers that are
// initialized with empty arrays.
//
// Buffers are allocated. when unpacked the attribute arrays
// are not coppied from the buffer rather they point to
// the appropriate location in the buffer. The buffers should
// be deleted by caller after arrays are not needed.
//
// return 0 on error.
int vtkCTHFragmentConnect::CollectIntegratedAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&volumes,
                vector<vtkDoubleArray *>&moments,
                vector<vector<vtkDoubleArray *> >&volumeWtdAvgs,
                vector<vector<vtkDoubleArray *> >&massWtdAvgs,
                vector<vector<vtkDoubleArray *> >&sums)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();
  const int msgBase=200000;

  // size buffer's header's
  // here we work with a single block(material)
  vtkCTHFragmentCommBuffer::SizeHeader(buffers,1);

  // receive & unpack
  for (int procId=0; procId<nProcs; ++procId)
    {
    // skip mine
    if (procId==myProcId)
      {
      continue;
      }
    int thisMsgId=msgBase;
    // recieve buffer's header
    this->Controller->Receive(
            buffers[procId].GetHeader(),
            buffers[procId].GetHeaderSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // size attribute's buffer from incoming header
    buffers[procId].SizeBuffer();
    // receive attribute's data
    this->Controller->Receive(
            buffers[procId].GetBuffer(),
            buffers[procId].GetBufferSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // unpack attribute data
    // We will point into the comm buffer rather than copy
    const unsigned int nToUnpack
      = buffers[procId].GetNumberOfTuples(0);
    // volume
    buffers[procId].UnPack(volumes[procId],1,nToUnpack,false);
    // moments
    if (this->ComputeMoments)
      {
      buffers[procId].UnPack(moments[procId],4,nToUnpack,false);
      }
    // volume weighted averages
    for (int i=0; i<this->NVolumeWtdAvgs; ++i)
      {
      const unsigned int nCompsAvg
        = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
      buffers[procId].UnPack(volumeWtdAvgs[procId][i],nCompsAvg,nToUnpack,false);
      }
    // mass weighted averages
    for (int i=0; i<this->NMassWtdAvgs; ++i)
      {
      const unsigned int nCompsAvg
        = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
      buffers[procId].UnPack(massWtdAvgs[procId][i],nCompsAvg,nToUnpack,false);
      }
    // sums
    for (int i=0; i<this->NToSum; ++i)
      {
      const unsigned int nCompsSum
        = this->FragmentSums[i]->GetNumberOfComponents();
      buffers[procId].UnPack(sums[procId][i],nCompsSum,nToUnpack,false);
      }
    }

  return 1;
}
//----------------------------------------------------------------------------
// Replaces our own interated attributes with those received
// from another process.
//
// return 0 on error.
int vtkCTHFragmentConnect::ReceiveIntegratedAttributes(
                const int sourceProcId)
{
  const int msgBase=200000;

  // prepare the comm buffer to receive attribute data
  // pertaining to a single block(material)
  vtkCTHFragmentCommBuffer buffer;
  buffer.SizeHeader(1);

  int thisMsgId=msgBase;
  // recieve buffer's header
  this->Controller->Receive(
          buffer.GetHeader(),
          buffer.GetHeaderSize(),
          sourceProcId,
          thisMsgId);
  ++thisMsgId;
  // size buffer via on incoming header
  buffer.SizeBuffer();
  // receive attribute's data
  this->Controller->Receive(
          buffer.GetBuffer(),
          buffer.GetBufferSize(),
          sourceProcId,
          thisMsgId);
  ++thisMsgId;

  // unpack attribue data, with an explicit copy 
  // into a local array
  const unsigned int nToUnPack
    = buffer.GetNumberOfTuples(0);
  // volume
  ReNewVtkArrayPointer(
          this->FragmentVolumes,
          this->FragmentVolumes->GetName());
  buffer.UnPack(this->FragmentVolumes,1,nToUnPack,true);
  // moments
  if (this->ComputeMoments)
    {
    ReNewVtkArrayPointer(
            this->FragmentMoments,
            this->FragmentMoments->GetName());
    buffer.UnPack(this->FragmentMoments,4,nToUnPack,true);
    }
  // volume weighted averages
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    int nCompsVWAvg
      = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(
            this->FragmentVolumeWtdAvgs[i],
            this->FragmentVolumeWtdAvgs[i]->GetName());
    buffer.UnPack(
            this->FragmentVolumeWtdAvgs[i],nCompsVWAvg,nToUnPack,true);
    }
  // mass weighted averages
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    int nCompsMWAvg
      = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(
            this->FragmentMassWtdAvgs[i],
            this->FragmentMassWtdAvgs[i]->GetName());
    buffer.UnPack(
            this->FragmentMassWtdAvgs[i],nCompsMWAvg,nToUnPack,true);
    }
  // sums
  for (int i=0; i<this->NToSum; ++i)
    {
    int nCompsSum
      = this->FragmentSums[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(
            this->FragmentSums[i],
            this->FragmentSums[i]->GetName());
    buffer.UnPack(
            this->FragmentSums[i],nCompsSum,nToUnPack,true);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Send my integrated attributes to another process.
//
// return 0 on error.
int vtkCTHFragmentConnect::SendIntegratedAttributes(
                const int recipientProcId)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int msgBase=200000;

  // estimate buffer size (in bytes)
  const vtkIdType nToSend
    = this->FragmentVolumes->GetNumberOfTuples();
  unsigned int totalNumberOfComps
    = 1 + (this->ComputeMoments ? 4 : 0); // volume + moments
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)  // + volume weighted averages
    {
    totalNumberOfComps
      += this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
    }
  for (int i=0; i<this->NMassWtdAvgs; ++i)  // + mass weighted averages
    {
    totalNumberOfComps
      += this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
    }
  for (int i=0; i<this->NToSum; ++i)      // + sums
    {
    totalNumberOfComps
      += this->FragmentSums[i]->GetNumberOfComponents();
    }
  const vtkIdType bufferSize
    = nToSend*totalNumberOfComps*sizeof(double);

  // prepare a comm buffer
  // Will use only a single block(material) of attribute data
  // We size the buffer, and set the number of fragments.
  vtkCTHFragmentCommBuffer buffer;
  buffer.Initialize(myProcId,1,bufferSize);
  buffer.SetNumberOfTuples(0,nToSend);

  // pack attributes into the buffer
  // volume
  buffer.Pack(this->FragmentVolumes);
  // moments
  if (this->ComputeMoments)
    {
    buffer.Pack(this->FragmentMoments);
    }
  // volume weighted averages
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    buffer.Pack(this->FragmentVolumeWtdAvgs[i]);
    }
  // mass weighted averages
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    buffer.Pack(this->FragmentMassWtdAvgs[i]);
    }
  // sums
  for (int i=0; i<this->NToSum; ++i)
    {
    buffer.Pack(this->FragmentSums[i]);
    }

  // send the buffer in two parts, first the header, followed by 
  // the attribute data
  int thisMsgId=msgBase;
  // buffer's header
  this->Controller->Send(
          buffer.GetHeader(),
          buffer.GetHeaderSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;
  // packed attributes
  this->Controller->Send(
          buffer.GetBuffer(),
          buffer.GetBufferSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;

  return 1;
}


//----------------------------------------------------------------------------
// Send my integrated attribute arrays to all other processes.
// 
// return 0 on error.
int vtkCTHFragmentConnect::BroadcastIntegratedAttributes(
                const int sourceProcId)
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::BroadcastIntegratedAttributes("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // anything to do?
  if (nProcs==1)
    {
    return 1;
    }

  // send mine to all
  if (myProcId==sourceProcId)
    {
    for (int procId=0; procId<nProcs; ++procId)
      {
      // skip me
      if (procId==sourceProcId)
        {
        continue;
        }
      this->SendIntegratedAttributes(procId);
      }
    }
  // recieve from source
  else
    {
    this->ReceiveIntegratedAttributes(sourceProcId);
    }

  return 1;
}

//----------------------------------------------------------------------------
// Create some buffers to hold the other processes attribute arrays.
// Copy our current data into these arrays as well.
//
// return 0 on error.
int vtkCTHFragmentConnect::PrepareToCollectIntegratedAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&volumes,
                vector<vtkDoubleArray *>&moments,
                vector<vector<vtkDoubleArray *> >&volumeWtdAvgs,
                vector<vector<vtkDoubleArray *> >&massWtdAvgs,
                vector<vector<vtkDoubleArray *> >&sums)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // set up for collect
  // buffers
  buffers.resize(nProcs);
  // volumes
  ResizeVectorOfVtkPointers(volumes,nProcs);
  volumes[myProcId]->Delete();
  volumes[myProcId]=this->FragmentVolumes;
  // moments
  if (this->ComputeMoments)
    {
    ResizeVectorOfVtkPointers(moments,nProcs);
    moments[myProcId]->Delete();
    moments[myProcId]=this->FragmentMoments;
    }
  // volume weighted averages
  if (this->NVolumeWtdAvgs>0)
    {
    volumeWtdAvgs.resize(nProcs);
    for (int procId=0; procId<nProcs; ++procId)
      {
      if (procId==myProcId)
        {
        volumeWtdAvgs[procId]=this->FragmentVolumeWtdAvgs;
        }
      else
        {
        ResizeVectorOfVtkPointers(volumeWtdAvgs[procId],this->NVolumeWtdAvgs);
        }
      }
    }
  // mass weighted averages
  if (this->NMassWtdAvgs>0)
    {
    massWtdAvgs.resize(nProcs);
    for (int procId=0; procId<nProcs; ++procId)
      {
      if (procId==myProcId)
        {
        massWtdAvgs[procId]=this->FragmentMassWtdAvgs;
        }
      else
        {
        ResizeVectorOfVtkPointers(massWtdAvgs[procId],this->NMassWtdAvgs);
        }
      }
    }
  // sums
  if (this->NToSum>0)
    {
    sums.resize(nProcs);
    for (int procId=0; procId<nProcs; ++procId)
      {
      if (procId==myProcId)
        {
        sums[procId]=this->FragmentSums;
        }
      else
        {
        ResizeVectorOfVtkPointers(sums[procId],this->NToSum);
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// Create some buffers to hold the other processes attribute arrays.
// Copy our current data into these arrays as well.
//
// return 0 on error.
int vtkCTHFragmentConnect::CleanUpAfterCollectIntegratedAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vtkDoubleArray *>&volumes,
                vector<vtkDoubleArray *>&moments,
                vector<vector<vtkDoubleArray *> >&volumeWtdAvgs,
                vector<vector<vtkDoubleArray *> >&massWtdAvgs,
                vector<vector<vtkDoubleArray *> >&sums)
{
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // volumes
  ClearVectorOfVtkPointers(volumes);
  // moments
  if (this->ComputeMoments)
    {
    ClearVectorOfVtkPointers(moments);
    }
  // volume weighted averages
  if (this->NVolumeWtdAvgs>0)
    {
    for (int procId=0; procId<nProcs; ++procId)
      {
      ClearVectorOfVtkPointers(volumeWtdAvgs[procId]);
      }
    }
  // mass weighted averages
  if (this->NMassWtdAvgs>0)
    {
    for (int procId=0; procId<nProcs; ++procId)
      {
      ClearVectorOfVtkPointers(massWtdAvgs[procId]);
      }
    }
  // sums
  if (this->NToSum>0)
    {
    for (int procId=0; procId<nProcs; ++procId)
      {
      ClearVectorOfVtkPointers(sums[procId]);
      }
    }
  // buffers
  buffers.clear();

  return 1;
}


//----------------------------------------------------------------------------
// Make new arrays to hold the resolved integarted attributes, 
// and initialize to zero.
//
// return 0 on error.
int vtkCTHFragmentConnect::PrepareToResolveIntegratedAttributes()
{
  int nComps;
  double *pResolved=0;
  vtkIdType bytesPerComponent
    = sizeof(double)*this->NumberOfResolvedFragments;

  // volume
  nComps=1;
  NewVtkArrayPointer(
      this->FragmentVolumes,
      nComps,
      this->NumberOfResolvedFragments,
      this->FragmentVolumes->GetName());
  pResolved=this->FragmentVolumes->GetPointer(0);
  memset( pResolved, 0, bytesPerComponent);
  // moments
  if (this->ComputeMoments)
    {
    nComps=4;
    NewVtkArrayPointer(
        this->FragmentMoments,
        nComps,
        this->NumberOfResolvedFragments,
        this->FragmentMoments->GetName());
    pResolved=this->FragmentMoments->GetPointer(0);
    memset( pResolved, 0, nComps*bytesPerComponent);
    }
  // volume weighted averages
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    nComps
      = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
    NewVtkArrayPointer(
        this->FragmentVolumeWtdAvgs[i],
        nComps,
        this->NumberOfResolvedFragments,
        this->FragmentVolumeWtdAvgs[i]->GetName());
    pResolved=this->FragmentVolumeWtdAvgs[i]->GetPointer(0);
    memset( pResolved, 0, nComps*bytesPerComponent);
    }
  // mass weighted averages
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    nComps
      = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
    NewVtkArrayPointer(
        this->FragmentMassWtdAvgs[i],
        nComps,
        this->NumberOfResolvedFragments,
        this->FragmentMassWtdAvgs[i]->GetName());
    pResolved=this->FragmentMassWtdAvgs[i]->GetPointer(0);
    memset( pResolved, 0, nComps*bytesPerComponent);
    }
  // sums
  for (int i=0; i<this->NToSum; ++i)
    {
    nComps
      = this->FragmentSums[i]->GetNumberOfComponents();
    NewVtkArrayPointer(
        this->FragmentSums[i],
        nComps,
        this->NumberOfResolvedFragments,
        this->FragmentSums[i]->GetName());
    pResolved=this->FragmentSums[i]->GetPointer(0);
    memset( pResolved, 0, nComps*bytesPerComponent);
    }

  return 1;
}

//----------------------------------------------------------------------------
// This is a gather operation that includes requisite calculations
// to finish integrations. The integrated arrays on the controlling
// process are resolved, but other procs are not updated.
// 
// return 0 on error.
int vtkCTHFragmentConnect::ResolveIntegratedAttributes(
                const int controllingProcId)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // controller gathers and resolves
  if (myProcId==controllingProcId)
    {
    vector<vtkCTHFragmentCommBuffer>buffers;
    vector<vtkDoubleArray *>volumes;
    vector<vtkDoubleArray *>moments;
    vector<vector<vtkDoubleArray *> >volumeWtdAvgs;
    vector<vector<vtkDoubleArray *> >massWtdAvgs;
    vector<vector<vtkDoubleArray *> >sums;
    // prepare collect buffers
    this->PrepareToCollectIntegratedAttributes(
      buffers,volumes,moments,volumeWtdAvgs,massWtdAvgs,sums);
    // collect
    this->CollectIntegratedAttributes(
      buffers,volumes,moments,volumeWtdAvgs,massWtdAvgs,sums);

    // prepare for resolution
    this->PrepareToResolveIntegratedAttributes();
    // resolve attributes
    // First pass we'll resolve attributes which
    // are needed to resolve other attributes (eg weights)
    int procBaseEqSetId=0;
    for (int procId=0; procId<nProcs; ++procId)
      {
      int eqSetId;
      double *pResolved;
      const double *pUnresolved;
      int nUnresolved=this->NumberOfRawFragmentsInProcess[procId];

      // volume
      pUnresolved=volumes[procId]->GetPointer(0);
      pResolved=this->FragmentVolumes->GetPointer(0);
      eqSetId=procBaseEqSetId;
      for (int i=0; i<nUnresolved; ++i)
        {
        int resIdx
          = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
        pResolved[resIdx]+=pUnresolved[0];
        ++pUnresolved;
        ++eqSetId;
        }
      // moments
      if (this->ComputeMoments)
        {
        pUnresolved=moments[procId]->GetPointer(0);
        pResolved=this->FragmentMoments->GetPointer(0);
        eqSetId=procBaseEqSetId;
        for (int i=0; i<nUnresolved; ++i)
          {
          int resIdx
            = 4*this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          for (int q=0; q<4; ++q)
            {
            pResolved[resIdx+q]+=pUnresolved[q];
            }
          pUnresolved+=4;
          ++eqSetId;
          }
        }
      procBaseEqSetId+=nUnresolved;
      }

    // Second pass, resolve attributes which depend on 
    // other attributes (eg weighted averages)
    procBaseEqSetId=0;
    for (int procId=0; procId<nProcs; ++procId)
      {
      int eqSetId;
      double *pResolved;
      const double *pUnresolved;
      int nUnresolved=this->NumberOfRawFragmentsInProcess[procId];
      // volume weighted averages
      double *pWt;  // these have to do with weights
      int nCompsWt; //
      int wtComp;   //
      for (int k=0; k<this->NVolumeWtdAvgs; ++k)
        {
        pUnresolved=volumeWtdAvgs[procId][k]->GetPointer(0);
        pResolved=this->FragmentVolumeWtdAvgs[k]->GetPointer(0);
        int nComps
          = this->FragmentVolumeWtdAvgs[k]->GetNumberOfComponents();
        pWt=this->FragmentVolumes->GetPointer(0);
        nCompsWt=1;
        wtComp=0;
        eqSetId=procBaseEqSetId;
        for (int i=0; i<nUnresolved; ++i)
          {
          int resId
            = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          int resIdx=nComps*resId;
          int wtIdx=nCompsWt*resId;
          for (int q=0; q<nComps; ++q)
            {
            pResolved[resIdx+q]+=pUnresolved[q]/pWt[wtIdx];
            }
          pUnresolved+=nComps;
          ++eqSetId;
          }
        }
      // mass weighted averages (Mx,My,Mz,Mass)
      if (this->ComputeMoments)
        {
        for (int k=0; k<this->NMassWtdAvgs; ++k)
          {
          pUnresolved=massWtdAvgs[procId][k]->GetPointer(0);
          pResolved=this->FragmentMassWtdAvgs[k]->GetPointer(0);
          int nComps
            = this->FragmentMassWtdAvgs[k]->GetNumberOfComponents();
          nCompsWt=4;
          wtComp=3;
          pWt=this->FragmentMoments->GetPointer(0)+wtComp;
          eqSetId=procBaseEqSetId;
          for (int i=0; i<nUnresolved; ++i)
            {
            int resId
              = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
            int resIdx=nComps*resId;
            int wtIdx=nCompsWt*resId;
            for (int q=0; q<nComps; ++q)
              {
              pResolved[resIdx+q]+=pUnresolved[q]/pWt[wtIdx];
              }
            pUnresolved+=nComps;
            ++eqSetId;
            }
          }
        }
      // sums
      for (int k=0; k<this->NToSum; ++k)
        {
        pUnresolved=sums[procId][k]->GetPointer(0);
        pResolved=this->FragmentSums[k]->GetPointer(0);
        int nComps
          = this->FragmentSums[k]->GetNumberOfComponents();
        eqSetId=procBaseEqSetId;
        for (int i=0; i<nUnresolved; ++i)
          {
          int resIdx
            = nComps*this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          for (int q=0; q<nComps; ++q)
            {
            pResolved[resIdx+q]+=pUnresolved[q];
            }
          pUnresolved+=nComps;
          ++eqSetId;
          }
        }
      procBaseEqSetId+=nUnresolved;
      }
    // clean up
    this->CleanUpAfterCollectIntegratedAttributes(
      buffers,volumes,moments,volumeWtdAvgs,massWtdAvgs,sums);
    }
  // others send unresolved integrated attributes 
  else
    {
    this->SendIntegratedAttributes(controllingProcId);
    }

  return 1;
}
//----------------------------------------------------------------------------
// Free memory that we won't be needing.which has been 
// allocated on our behalf.
void vtkCTHFragmentConnect::PrepareForResolveEquivalences()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::PrepareForResolveEquivalences("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  // equivalences
  this->EquivalenceSet->Squeeze();
  // volume
  this->FragmentVolumes->Squeeze();
  // moments
  if ( this->ComputeMoments )
    {
    this->FragmentMoments->Squeeze();
    }
  // volume weighted average
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    this->FragmentVolumeWtdAvgs[i]->Squeeze();
    }
  // mass weighted avg
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    this->FragmentMassWtdAvgs[i]->Squeeze();
    }
  // sum
  for (int i=0; i<this->NToSum; ++i)
    {
    this->FragmentSums[i]->Squeeze();
    }
  // geometry container
  vector<vtkPolyData *>(this->FragmentMeshes).swap(this->FragmentMeshes);
}
//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::ResolveEquivalences()
{
  int numProcs=this->Controller->GetNumberOfProcesses();
  #ifdef vtkCTHFragmentConnectDEBUG
  int myProcId=this->Controller->GetLocalProcessId();
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment entering ResolveEquivalences is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // These get resused for efer attribute we need to merge so I made the ivars.
  this->NumberOfRawFragmentsInProcess = new int[numProcs];
  this->LocalToGlobalOffsets = new int[numProcs];

  // Resolve intraprocess and extra process equivalences.
  // This also renumbers set ids to be sequential.
  this->GatherEquivalenceSets(this->EquivalenceSet);
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment after GatherEquivalenceSets is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // Gather and merge fragments for whose geometry is split
  // and build the output dataset as we go.
  this->ResolveLocalFragmentGeometry();
  ///this->ResolveRemoteFragmentGeometry();
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment after ResolveLocalFragmentGeometry is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // Accumulate contributions from fragemnts who were 
  // previously split.
  this->ResolveIntegratedAttributes(0);
  this->BroadcastIntegratedAttributes(0);
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment after ResolveIntegratedAttributes is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // Compute geometric attributes, and gather them for 
  // stats output.
  this->ComputeGeometricAttributes();
  this->GatherGeometricAttributes(0);
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment after ComputeGeometricAttributes is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  // Copy attributes into the output data sets.
  this->CopyAttributesToOutput0();
  this->CopyAttributesToOutput1();
  #ifdef vtkCTHFragmentConnectDEBUG
  cerr << "[" << __LINE__ << "] "
       << myProcId
       << " memory commitment after CopyAttributesToOutput0/1 is:"
       << endl
       << GetMemoryUsage(this->MyPid,__LINE__,myProcId);
  #endif

  delete [] this->NumberOfRawFragmentsInProcess;
  this->NumberOfRawFragmentsInProcess = 0;
  delete [] this->LocalToGlobalOffsets;
  this->LocalToGlobalOffsets = 0;
}


//----------------------------------------------------------------------------
// This also fills in the arrays NumberOfRawFragments and LocalToGlobalOffsets
// as a side effect. (also NumberOfResolvedFragments).
void vtkCTHFragmentConnect::GatherEquivalenceSets(
  vtkCTHFragmentEquivalenceSet* set)
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::GatherEquivalenceSets("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int numProcs = this->Controller->GetNumberOfProcesses();
  const int myProcId = this->Controller->GetLocalProcessId();
  const int numLocalMembers = set->GetNumberOfMembers();

  // Find a mapping between local fragment id and the global fragment ids.
  if (myProcId == 0)
    {
    this->NumberOfRawFragmentsInProcess[0] = numLocalMembers;
    for (int ii = 1; ii < numProcs; ++ii)
      {
      this->Controller->Receive(this->NumberOfRawFragmentsInProcess+ii, 1, ii, 875034);
      }
    // Now send the results back to all processes.
    for (int ii = 1; ii < numProcs; ++ii)
      {
      this->Controller->Send(this->NumberOfRawFragmentsInProcess, numProcs, ii, 875035);
      }
    }
  else
    {
    this->Controller->Send(&numLocalMembers, 1, 0, 875034);
    this->Controller->Receive(this->NumberOfRawFragmentsInProcess, numProcs, 0, 875035);
    }
  // Compute offsets.
  int totalNumberOfIds = 0;
  for (int ii = 0; ii < numProcs; ++ii)
    {
    int numIds = this->NumberOfRawFragmentsInProcess[ii];
    this->LocalToGlobalOffsets[ii] = totalNumberOfIds;
    totalNumberOfIds += numIds;
    }
  this->TotalNumberOfRawFragments = totalNumberOfIds;

  // Change the set to a global set.
  vtkCTHFragmentEquivalenceSet *globalSet = new vtkCTHFragmentEquivalenceSet;
  // This just initializes the set so that every set has one member.
  //  Every id is equivalent to itself and no others.
  if (totalNumberOfIds > 0)
    { // This crashes if there no fragemnts extracted.
    globalSet->AddEquivalence(totalNumberOfIds-1, totalNumberOfIds-1);
    }
  // Add the equivalences from our process.
  int myOffset = this->LocalToGlobalOffsets[myProcId];
  int memberSetId;
  for(int ii = 0; ii < numLocalMembers; ++ii)
    {
    memberSetId = set->GetEquivalentSetId(ii);
    // This will be inefficient until I make the scheduled improvemnt to the
    // add method (Ordered multilevel tree instead of a one level tree).
    globalSet->AddEquivalence(ii+myOffset, memberSetId+myOffset);
    }

  //cerr << myProcId << " Input set: " << endl;
  //set->Print();
  //cerr << myProcId << " global set: " << endl;
  //globalSet->Print();

  // Now add equivalents between processes.
  // Send all the ghost blocks to the process that owns the block.
  // Compare ids and add the equivalences.
  this->ShareGhostEquivalences(globalSet, this->LocalToGlobalOffsets);

  //cerr << "Global after ghost: " << myProcId << endl;
  //globalSet->Print();

  // Merge all of the processes global sets.
  // Clean the global set so that the resulting set ids are sequential.
  this->MergeGhostEquivalenceSets(globalSet);

  //cerr << "Global after merge: " << myProcId << endl;
  //globalSet->Print();

  // free what do not need
  globalSet->Squeeze();

  // Copy the equivalences to the local set for returning our results.
  // The ids will be the global ids so the GetId method will work.
  set->DeepCopy(globalSet);

  delete globalSet;
}

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::MergeGhostEquivalenceSets(
  vtkCTHFragmentEquivalenceSet* globalSet)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  int* buf = globalSet->GetPointer();
  const int numIds = globalSet->GetNumberOfMembers();

  // At this point all the sets are global and have the same number of ids.
  // Send all the sets to process 0 to be merged.
  if (myProcId > 0)
    {
    this->Controller->Send(buf, numIds, 0, 342320);
    // Now receive the final equivalences.
    this->Controller->Receive(&this->NumberOfResolvedFragments, 1, 0, 342321);
    // Domain has numIds,  range has NumberOfResolvedFragments
    this->Controller->Receive(buf, numIds, 0, 342322);
    // We have to mark the set as resolved because the set being
    // received has been resolved.  If we do not do this then
    // We cannot get the proper set id.  Using the pointer
    // here is a bad api.  TODO: Fix the API and make "Resolved" private.
    globalSet->Resolved = 1;
    return;
    }

  // Only process 0 from here out.

  int numProcs = this->Controller->GetNumberOfProcesses();
  int* tmp;
  tmp = new int[numIds];
  for (int ii = 1; ii < numProcs; ++ii)
    {
    this->Controller->Receive(tmp, numIds, ii, 342320);
    // Merge the values.
    for (int jj = 0; jj < numIds; ++jj)
      {
      if (tmp[jj] != jj)
        { // TODO: Make sure this is efficient.  Avoid n^2.
        globalSet->AddEquivalence(jj, tmp[jj]);
        }
      }
    }
  delete [] tmp;

  // Make the set ids sequential.
  this->NumberOfResolvedFragments = globalSet->ResolveEquivalences();
    
  // The pointers should still be valid.
  // The array should not resize here.
  // Number of resolved fragemnts will be smaller 
  // than TotalNumberOfRawFragments
  for (int ii = 1; ii < numProcs; ++ii)
    {
    this->Controller->Send(&this->NumberOfResolvedFragments, 1, ii, 342321);
    // Domain has numIds,  range has NumberOfResolvedFragments
    this->Controller->Send(buf, numIds, ii, 342322);
    }
 }

//----------------------------------------------------------------------------
void vtkCTHFragmentConnect::ShareGhostEquivalences(
  vtkCTHFragmentEquivalenceSet* globalSet,
  int* procOffsets)
{
  const int numProcs = this->Controller->GetNumberOfProcesses();
  const int myProcId = this->Controller->GetLocalProcessId();
  int sendMsg[8];

  // Loop through the other processes.
  for (int otherProc = 0; otherProc < numProcs; ++otherProc)
    {
    if (otherProc == myProcId)
      {
      this->ReceiveGhostFragmentIds(globalSet, procOffsets);
      }
    else
      {
      // Loop through our ghost blocks sending the 
      // ones that are owned by otherProc.
      int num = this->GhostBlocks.size();
      for (int blockId = 0; blockId < num; ++blockId)
        {
        vtkCTHFragmentConnectBlock* block = this->GhostBlocks[blockId];
        if (block && block->GetOwnerProcessId() == otherProc && block->GetGhostFlag())
          {
          sendMsg[0] = myProcId;
          // Since this is a ghost block, the remote block id
          // will be different than the id we use.
          // We just want to make it easy for the process that owns this block
          // to match the ghost block with the aoriginal.
          sendMsg[1] = block->GetBlockId();
          int* ext = sendMsg + 2;
          block->GetCellExtent(ext);
          this->Controller->Send(sendMsg, 8, otherProc, 722265);
          // Now send the fragment id array.
          int* framentIds = block->GetFragmentIdPointer();
          this->Controller->Send(framentIds, (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1),
                                 otherProc, 722266);
          } // End if ghost  block owned by other process.
        } // End loop over all blocks.
      // Send the message that indicates we have nothing more to send.
      sendMsg[0] = myProcId;
      sendMsg[1] = -1;
      this->Controller->Send(sendMsg, 8, otherProc, 722265);
      } // End if we shoud send or receive.
    } // End loop over all processes.
}

//----------------------------------------------------------------------------
// Receive all the gost blocks from remote processes and 
// find the equivalences.
void vtkCTHFragmentConnect::ReceiveGhostFragmentIds(
  vtkCTHFragmentEquivalenceSet* globalSet,
  int* procOffsets)
{
  int msg[8];
  int otherProc;
  int blockId;
  vtkCTHFragmentConnectBlock* block;
  int bufSize = 0;
  int* buf = 0;
  int dataSize;
  int* remoteExt;
  int localId, remoteId;
  const int myProcId = this->Controller->GetLocalProcessId();
  int localOffset = procOffsets[myProcId];
  int remoteOffset;

  // We do not receive requests from our own process.
  int remainingProcs = this->Controller->GetNumberOfProcesses() - 1;
  while (remainingProcs != 0)
    {
    this->Controller->Receive(msg, 8, vtkMultiProcessController::ANY_SOURCE, 722265);
    otherProc = msg[0];
    blockId = msg[1];
    if (blockId == -1)
      {
      --remainingProcs;
      }
    else
      {
      // Find the block.
      block = this->InputBlocks[blockId];
      if (block == 0)
        { // Sanity check. This will lock up!
        vtkErrorMacro("Missing block request.");
        return;
        }
      // Receive the ghost fragment ids.
      remoteExt = msg+2;
      dataSize = (remoteExt[1]-remoteExt[0]+1)
                  *(remoteExt[3]-remoteExt[2]+1)
                  *(remoteExt[5]-remoteExt[4]+1);
      if (bufSize < dataSize) 
        {
        if (buf) { delete [] buf;}
        buf = new int[dataSize];
        bufSize = dataSize;
        }
      remoteOffset = procOffsets[otherProc];
      this->Controller->Receive(buf, dataSize, otherProc, 722266);
      // We have our block, and the remote fragmentIds.
      // Now for the equivalences.
      // Loop through all of the voxels.
      int* remoteFragmentIds = buf;
      int* localFragmentIds = block->GetFragmentIdPointer();
      int localExt[6];
      int localIncs[3];
      block->GetCellExtent(localExt);
      block->GetCellIncrements(localIncs);
      int *px, *py, *pz;
      // Find the starting voxel in the local block.
      pz = localFragmentIds + (remoteExt[0]-localExt[0])*localIncs[0]
                            + (remoteExt[2]-localExt[2])*localIncs[1]
                            + (remoteExt[4]-localExt[4])*localIncs[2];
      for (int iz = remoteExt[4]; iz <= remoteExt[5]; ++iz)
        {
        py = pz;
        for (int iy = remoteExt[2]; iy <= remoteExt[3]; ++iy)
          {
          px = py;
          for (int ix = remoteExt[0]; ix <= remoteExt[1]; ++ix)
            {
            // Convert local fragment ids to global ids.
            localId = *px;
            remoteId = *remoteFragmentIds;
            if (localId >= 0 && remoteId >= 0)
              {
              globalSet->AddEquivalence(localId + localOffset, 
                                        remoteId + remoteOffset);
              }
            ++remoteFragmentIds;
            ++px;
            }
          py += localIncs[1];
          }
        pz += localIncs[2];
        }
      }
    }
  if (buf)
    {
    delete [] buf;
    }
}

//----------------------------------------------------------------------------
// After the attributes have been resolved copy them in to the 
// fragment data sets. We put each one as a 1 tuple array
// in the field data, and for now we copy each to the point data
// as well. My hope is that the point data can be eliminated.
void vtkCTHFragmentConnect::CopyAttributesToOutput0()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::CopyAttributesToOutput0("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert( "Couldn't get the resolved fragnments." 
          && resolvedFragments );

  vector<int> &fragmentSplitMarker
    = this->FragmentSplitMarker[this->MaterialId];

  int nLocalFragments=resolvedFragmentIds.size();
  for (int i=0; i<nLocalFragments; ++i)
    {
    int nComps=0;
    const char *name=0;
    const double *srcTuple=0;

    // get the fragment
    int globalId=resolvedFragmentIds[i];
    vtkPolyData *thisFragment
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));
    assert( "Fragment is not local." && thisFragment);
    int nPoints=thisFragment->GetNumberOfPoints();

    // add a attributes to field data
    vtkFieldData *fd=thisFragment->GetFieldData();
    vtkPointData *pd=thisFragment->GetPointData();
    // Fragment id
    // field data
    int resolvedGlobalId=globalId+this->ResolvedFragmentCount;
    vtkIntArray *fdId=vtkIntArray::New();
    fdId->SetName("Id");
    fdId->SetNumberOfComponents(1);
    fdId->SetNumberOfTuples(1);
    fdId->SetValue(0,resolvedGlobalId);
    fd->AddArray(fdId);
    fdId->Delete();
    // point data
    vtkIntArray *pdId=vtkIntArray::New();
    pdId->SetName("Id");
    pdId->SetNumberOfComponents(1);
    pdId->SetNumberOfTuples(nPoints);
    pdId->FillComponent(0,resolvedGlobalId);
    pd->AddArray(pdId);
    pdId->Delete();

    // Material Id
    // field data
    vtkIntArray *fdMid=vtkIntArray::New();
    fdMid->SetName("Material");
    fdMid->SetNumberOfComponents(1);
    fdMid->SetNumberOfTuples(1);
    fdMid->SetValue(0,this->MaterialId);
    fd->AddArray(fdMid);
    fdMid->Delete();
    // point data
    vtkIntArray *pdMid=vtkIntArray::New();
    pdMid->SetName("Material");
    pdMid->SetNumberOfComponents(1);
    pdMid->SetNumberOfTuples(nPoints);
    pdMid->FillComponent(0,this->MaterialId);
    pd->AddArray(pdMid);
    pdMid->Delete();

    // volume
    double V
      = this->FragmentVolumes->GetValue(globalId);
    // field data
    vtkDoubleArray *fdVol=vtkDoubleArray::New();
    fdVol->SetName("Volume");
    fdVol->SetNumberOfComponents(1);
    fdVol->SetNumberOfTuples(1);
    fdVol->SetValue(0,V);
    fd->AddArray(fdVol);
    fdVol->Delete();
    // point data
    vtkDataArray *pdVol=vtkDoubleArray::New();
    pdVol->SetName("Volume");
    pdVol->SetNumberOfComponents(1);
    pdVol->SetNumberOfTuples(nPoints);
    pdVol->FillComponent(0,V);
    pd->AddArray(pdVol);
    pdVol->Delete();

    if (this->ComputeMoments)
      {
      //Mass
      nComps=this->FragmentMoments->GetNumberOfComponents();
      name=this->FragmentMoments->GetName();
      srcTuple=this->FragmentMoments->GetTuple(globalId);
      // field data
      vtkDoubleArray *fdM=vtkDoubleArray::New();
      fdM->SetName("Mass");
      fdM->SetNumberOfComponents(1);
      fdM->SetNumberOfTuples(1);
      fdM->SetValue(0,srcTuple[3]);
      fd->AddArray(fdM);
      fdM->Delete();
      // point data
      vtkDataArray *pdM=vtkDoubleArray::New();
      pdM->SetName("Mass");
      pdM->SetNumberOfComponents(1);
      pdM->SetNumberOfTuples(nPoints);
      pdM->FillComponent(0,srcTuple[3]);
      pd->AddArray(pdM);
      pdM->Delete();

      // Center of mass
      double com[3];
      for (int q=0; q<3; ++q) 
        {// TODO this happens twice....
        com[q]=srcTuple[q]/srcTuple[3];
        }
      // field data
      vtkDoubleArray *fdCom=vtkDoubleArray::New();
      fdCom->SetName("Center of Mass");
      fdCom->SetNumberOfComponents(3);
      fdCom->SetNumberOfTuples(1);
      fdCom->SetTuple(0,com);
      fd->AddArray(fdCom);
      fdCom->Delete();
      }
    else
      {
      // Center of AABB
      srcTuple=this->FragmentAABBCenters->GetTuple(i);
      // field data
      vtkDoubleArray *fdCaabb=vtkDoubleArray::New();
      fdCaabb->SetName("Center of AABB");
      fdCaabb->SetNumberOfComponents(3);
      fdCaabb->SetNumberOfTuples(1);
      fdCaabb->SetTuple(0,srcTuple);
      fd->AddArray(fdCaabb);
      fdCaabb->Delete();
      }

    // OBBs
    if (this->ComputeOBB)
      {
      // All of the information is provided in
      // the feild data, while only dimensions are 
      // provided in the point data.
      srcTuple=this->FragmentOBBs->GetTuple(i);
      // field data
      vtkDoubleArray *fdObb=vtkDoubleArray::New();
      fdObb->SetName("OBB");
      fdObb->SetNumberOfComponents(15);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0,srcTuple);
      fd->AddArray(fdObb);
      fdObb->Delete();
      // point data
      vtkDoubleArray *pdObb=vtkDoubleArray::New();
      pdObb->SetName("OBB Dimensions");
      pdObb->SetNumberOfComponents(3);
      pdObb->SetNumberOfTuples(nPoints);
      for (int q=0; q<3; ++q)
        {
        pdObb->FillComponent(q,srcTuple[12+q]);
        }
      pd->AddArray(pdObb);
      pdObb->Delete();
      }
    // volume weighted averages
    for (int j=0; j<this->NVolumeWtdAvgs; ++j)
      {
      nComps
        = this->FragmentVolumeWtdAvgs[j]->GetNumberOfComponents();
      name
        = this->FragmentVolumeWtdAvgs[j]->GetName();
      srcTuple
        = this->FragmentVolumeWtdAvgs[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray *fdVwa=vtkDoubleArray::New();
      fdVwa->SetName(name);
      fdVwa->SetNumberOfComponents(nComps);
      fdVwa->SetNumberOfTuples(1);
      fdVwa->SetTuple(0,srcTuple);
      fd->AddArray(fdVwa);
      fdVwa->Delete();
      // point data
      vtkDoubleArray *pdVwa=vtkDoubleArray::New();
      pdVwa->SetName(name);
      pdVwa->SetNumberOfComponents(nComps);
      pdVwa->SetNumberOfTuples(nPoints);
      for (int q=0; q<nComps; ++q)
        {
        pdVwa->FillComponent(q,srcTuple[q]);
        }
      pd->AddArray(pdVwa);
      pdVwa->Delete();
      }
    // mass weighted averages
    for (int j=0; j<this->NMassWtdAvgs; ++j)
      {
      nComps
        = this->FragmentMassWtdAvgs[j]->GetNumberOfComponents();
      name
        = this->FragmentMassWtdAvgs[j]->GetName();
      srcTuple
        = this->FragmentMassWtdAvgs[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray *fdMwa=vtkDoubleArray::New();
      fdMwa->SetName(name);
      fdMwa->SetNumberOfComponents(nComps);
      fdMwa->SetNumberOfTuples(1);
      fdMwa->SetTuple(0,srcTuple);
      fd->AddArray(fdMwa);
      fdMwa->Delete();
      // point data
      vtkDoubleArray *pdMwa=vtkDoubleArray::New();
      pdMwa->SetName(name);
      pdMwa->SetNumberOfComponents(nComps);
      pdMwa->SetNumberOfTuples(nPoints);
      for (int q=0; q<nComps; ++q)
        {
        pdMwa->FillComponent(q,srcTuple[q]);
        }
      pd->AddArray(pdMwa);
      pdMwa->Delete();
      }
    // summations
    for (int j=0; j<this->NToSum; ++j)
      {
      nComps
        = this->FragmentSums[j]->GetNumberOfComponents();
      name
        = this->FragmentSums[j]->GetName();
      srcTuple
        = this->FragmentSums[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray *fdS=vtkDoubleArray::New();
      fdS->SetName(name);
      fdS->SetNumberOfComponents(nComps);
      fdS->SetNumberOfTuples(1);
      fdS->SetTuple(0,srcTuple);
      fd->AddArray(fdS);
      fdS->Delete();
      // point data
      vtkDoubleArray *pdS=vtkDoubleArray::New();
      pdS->SetName(name);
      pdS->SetNumberOfComponents(nComps);
      pdS->SetNumberOfTuples(nPoints);
      for (int q=0; q<nComps; ++q)
        {
        pdS->FillComponent(q,srcTuple[q]);
        }
      pd->AddArray(pdS);
      pdS->Delete();
      }
    // split geometry marker
    int splitMark=fragmentSplitMarker[i];
    // field data
    vtkIntArray *fdSM=vtkIntArray::New();
    fdSM->SetName("SplitGeometry");
    fdSM->SetNumberOfComponents(1);
    fdSM->SetNumberOfTuples(1);
    fdSM->SetValue(0,splitMark);
    fd->AddArray(fdSM);
    fdSM->Delete();
    // point data
    vtkIntArray *pdSM=vtkIntArray::New();
    pdSM->SetName("SplitGeometry");
    pdSM->SetNumberOfComponents(1);
    pdSM->SetNumberOfTuples(nPoints);
    pdSM->FillComponent(0,splitMark);
    pd->AddArray(pdSM);
    pdSM->Delete();
    } // for each local fragment
}

//----------------------------------------------------------------------------
// Output 1 is a multiblock with a block for each material containing
// a set of points at fragment centers. If center of mass hasn't been
// computed then the centers of axis aligned bounding boxes are used.
void vtkCTHFragmentConnect::CopyAttributesToOutput1()
{
  ostringstream progressMesg;
  progressMesg << "vtkCTHFragmentConnect::CopyAttributesToOutput1("
               << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
  this->Progress+=this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  vtkPolyData *resolvedFragmentCenters
    = dynamic_cast<vtkPolyData *>(this->ResolvedFragmentCenters->GetBlock(this->MaterialId));

  // only proc 0 will build the point set
  // all of the attributes are gathered there for
  // resolution so he has everything. Set the others to 0.
  const int procId = this->Controller->GetLocalProcessId();
  if (procId!=0)
    {
    //resolvedFragmentCenters->Delete();
    this->ResolvedFragmentCenters->SetBlock(this->MaterialId,static_cast<vtkPolyData *>(0));
    return;
    }
  // Copy the attributes into point data  
  vtkPointData *pd=resolvedFragmentCenters->GetPointData();
  vtkIntArray *ia=0;
  vtkDoubleArray *da=0;
  // 0 id
  ia=vtkIntArray::New();
  ia->SetName("Id");
  ia->SetNumberOfTuples(this->NumberOfResolvedFragments);
  for (int i=0; i<this->NumberOfResolvedFragments; ++i)
    {
    ia->SetValue(i,i+this->ResolvedFragmentCount);
    }
  pd->AddArray(ia);
  // 1 material
  ReNewVtkPointer(ia);
  ia->SetName("Material");
  ia->SetNumberOfTuples(this->NumberOfResolvedFragments);
  ia->FillComponent(0,this->MaterialId);
  pd->AddArray(ia);
  // 2 volume
  da=vtkDoubleArray::New();
  da->DeepCopy( this->FragmentVolumes );
  da->SetName( this->FragmentVolumes->GetName() );
  pd->AddArray(da);
  // 3 mass
  if (this->ComputeMoments)
    {
    ReNewVtkPointer(da);
    da->SetName("Mass");
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    da->CopyComponent(0,this->FragmentMoments,3);
    pd->AddArray(da);
    }
  // 4 obb's
  if (this->ComputeOBB)
    {
    // Bounding box (c,max,mid,min)
    ReNewVtkPointer(da);
    da->SetName("OBB");
    da->SetNumberOfComponents(12);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q=0; q<12; ++q)
      {
      da->CopyComponent(q,this->FragmentOBBs,q);
      }
    pd->AddArray(da);
    // Dimensions of OBB
    ReNewVtkPointer(da);
    da->SetName("OBB Dimensions");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q=0; q<3; ++q)
      {
      da->CopyComponent(q,this->FragmentOBBs,12+q);
      }
    pd->AddArray(da);
    }
  // 5 ... i volume weighted averages
  for (int i=0; i<this->NVolumeWtdAvgs; ++i)
    {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentVolumeWtdAvgs[i]);
    da->SetName(this->FragmentVolumeWtdAvgs[i]->GetName());
    pd->AddArray(da);
    }
  // i ... i+n mass weighted averages
  for (int i=0; i<this->NMassWtdAvgs; ++i)
    {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentMassWtdAvgs[i]);
    da->SetName(this->FragmentMassWtdAvgs[i]->GetName());
    pd->AddArray(da);
    }
  // i+n+1 ... m sums
  for (int i=0; i<this->NToSum; ++i)
    {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentSums[i]);
    da->SetName(this->FragmentSums[i]->GetName());
    pd->AddArray(da);
    }
  ReleaseVtkPointer(da);
  // m+1 split geometry markers
  ReNewVtkPointer(ia);
  ia->DeepCopy(this->FragmentSplitGeometry);
  ia->SetName("SplitGeometry");
  pd->AddArray(ia);
  ReleaseVtkPointer(ia);
  // Compute the fragment centers and build vertices.
  vtkIdTypeArray *va=vtkIdTypeArray::New();
  va->SetNumberOfTuples(2*this->NumberOfResolvedFragments);
  vtkIdType *verts
    = va->GetPointer(0);
  vtkPoints *pts=vtkPoints::New();
  // use center of mass if available
  if (this->ComputeMoments)
    {
    pts->SetDataTypeToDouble();
    da=dynamic_cast<vtkDoubleArray *>(pts->GetData());
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    double *cen
      = da->GetPointer(0);
    const double *moments
      = this->FragmentMoments->GetPointer(0);
    for (int i=0; i<this->NumberOfResolvedFragments; ++i)
      {
      verts[0]=1;
      verts[1]=i;
      verts+=2;

      for (int q=0; q<3; ++q)
        {
        cen[q]=moments[q]/moments[3];
        }
      cen+=3;
      moments+=4;
      }
    }
  // use center of axis aligned bounding box
  else
    {
    pts->SetData(this->FragmentAABBCenters);
    for (int i=0; i<this->NumberOfResolvedFragments; ++i)
      {
      verts[0]=1;
      verts[1]=i;
      verts+=2;
      }
    }
  resolvedFragmentCenters->SetPoints(pts);
  pts->Delete();
  vtkCellArray *cells=vtkCellArray::New();
  cells->SetCells(static_cast<vtkIdType>(this->NumberOfResolvedFragments),va);
  resolvedFragmentCenters->SetVerts(cells);
  cells->Delete();
  va->Delete();
}

//----------------------------------------------------------------------------
// For each fragment compute its oriented bounding box(OBB).
//
// return 0 on error.
int vtkCTHFragmentConnect::ComputeLocalFragmentOBB()
{
  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));

  vector<int> &fragmentSplitMarker
    = this->FragmentSplitMarker[this->MaterialId];

  int nLocal=resolvedFragmentIds.size();

  // OBB set up
  vtkOBBTree *obbCalc;
  double *pObb;
  obbCalc=vtkOBBTree::New();
  assert("FragmentOBBs has incorrect size."
         && this->FragmentOBBs->GetNumberOfTuples()==nLocal);
  pObb=this->FragmentOBBs->GetPointer(0);

  // Traverse the fragments we own
  for (int i=0; i<nLocal; ++i)
    {
    // skip split fragments, these have already been
    // taken care of.
    if (fragmentSplitMarker[i]==1)
      {
      pObb+=15;
      continue;
      }

    int globalId=resolvedFragmentIds[i];

    vtkPolyData *thisFragment
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));

    // compute OBB
    double size[3];
    // (c_x,c_y,c_z),(max_x,max_y,max_z),(mid_x,mid_y,mid_z),(min_x,min_y,min_z),|max|,|mid|,|min|
    obbCalc->ComputeOBB(thisFragment,pObb,pObb+3,pObb+6,pObb+9,size);
    // compute magnitudes
    for (int q=0; q<3; ++q)
      {
      pObb[12+q]=0;
      }
    for (int q=0; q<3; ++q)
      {
      pObb[12]+=pObb[3+q]*pObb[3+q];
      pObb[13]+=pObb[6+q]*pObb[6+q];
      pObb[14]+=pObb[9+q]*pObb[9+q];
      }
    for (int q=0; q<3; ++q)
      {
      pObb[12+q]=sqrt(pObb[12+q]);
      }

    pObb+=15;
    }// fragment traversal

  // OBB clean up
  obbCalc->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// For each fragment compute its axis-aligned bounding box(AABB).
//
// return 0 on error.
int vtkCTHFragmentConnect::ComputeLocalFragmentAABBCenters()
{
  vector<int> &resolvedFragmentIds
    = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet *resolvedFragments
    = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(this->MaterialId));

  vector<int> &fragmentSplitMarker
    = this->FragmentSplitMarker[this->MaterialId];

  int nLocal=resolvedFragmentIds.size();

  // AABB set up
  assert("FragmentAABBCenters is expected to be pre-allocated."
         && this->FragmentAABBCenters->GetNumberOfTuples()==nLocal);
  double aabb[6];
  double *pCoaabb
    = this->FragmentAABBCenters->GetPointer(0);

  // Traverse the fragments we own
  for (int i=0; i<nLocal; ++i)
    {
    // skip fragments with geometry split over multiple 
    // processes. These have been already taken care of.
    if (fragmentSplitMarker[i]==1)
      {
      pCoaabb+=3;
      continue;
      }

    int globalId=resolvedFragmentIds[i];

    vtkPolyData *thisFragment
      = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(globalId));

    // AABB calculation
    thisFragment->GetBounds(aabb);
    for (int q=0,k=0; q<3; ++q, k+=2)
      {
      pCoaabb[q]=(aabb[k]+aabb[k+1])/2.0;
      }
    pCoaabb+=3;
    }// fragment traversal

  return 1;
}

//----------------------------------------------------------------------------
// Configure the multiblock outputs.
// Filter has two output ports we configure both.

// 0)
// Each block is a multipiece associated with the material. The multipiece is 
// built of many polydata where each polydata is a fragment. 
// 
// 1)
// A set of poly data vertices, one for each fragment center
// with attribute data on points.
//
// for now fragments may be split across procs.
//
// return 0 if an error occurs.
int vtkCTHFragmentConnect::BuildOutputs(
                vtkMultiBlockDataSet *mbdsOutput0,
                vtkMultiBlockDataSet *mbdsOutput1,
                int nMaterials)
{
  /// 0
  // we have a multi-block which is a container for the fragment sets
  // from each material.
  this->ResolvedFragments=mbdsOutput0;
  this->ResolvedFragments->SetNumberOfBlocks(nMaterials);
  /// 1
  this->ResolvedFragmentCenters=mbdsOutput1;
  this->ResolvedFragmentCenters->SetNumberOfBlocks(nMaterials);
  for (int i=0; i<nMaterials; ++i)
    {
    /// 0
    // A multi-piece container for polydata,
    // one for each of the fragments in this material.
    vtkMultiPieceDataSet *mpds=vtkMultiPieceDataSet::New();
    this->ResolvedFragments->SetBlock(i,mpds);
    mpds->Delete();
    /// 1
    // A single poly data holding all the cell centers.
    // of fragments in this material.
    vtkPolyData *pd=vtkPolyData::New();
    this->ResolvedFragmentCenters->SetBlock(i,pd);
    pd->Delete();
    }
  // Prepare ownership lists ...
  this->ResolvedFragmentIds.clear();
  this->ResolvedFragmentIds.resize(nMaterials);
  // and the split geometry markers...
  this->FragmentSplitMarker.clear();
  this->FragmentSplitMarker.resize(nMaterials);
  // both are indexed first by material id, second
  // by local fragment id.

  // This counts total over all materials, and is used to
  // generate a unique id for each fragment.
  this->ResolvedFragmentCount=0;

  return 1;
}

//----------------------------------------------------------------------------
// Write the fragment attribute to a text file. Each process
// writes its pieces.
//
//  For now duplicates might arise, however
// the attributes are global so that they should have the same
// value.
int vtkCTHFragmentConnect::WriteFragmentAttributesToTextFile()
{
  const int myProcId=this->Controller->GetLocalProcessId();

  // open the file
  ostringstream fileName;
  fileName << this->OutputTableFileNameBase
           << "."
           << myProcId
           << ".cthfc";

  ofstream fout;
  fout.open(fileName.str().c_str());
  if ( !fout.is_open() )
    {
    vtkErrorMacro("Could not open " << fileName.str() << ".");
    return 0;
    }

  int nMaterials=this->ResolvedFragments->GetNumberOfBlocks();
  // file header
  fout << "Header:{" << endl;
  fout << "Process id:" << myProcId << endl;
  fout << "Number of fragments:" << this->ResolvedFragmentCount << endl;
  fout << "Number of materials:" << nMaterials << endl;
  fout << "}" << endl;

  for (int materialId=0; materialId<nMaterials; ++materialId)
    {
    fout << "Material " << materialId << " :{" << endl;

    // all fragments store the same info within a material 
    // so we get the first and use that to build the file header
    vector<int> &resolvedFragmentIds=this->ResolvedFragmentIds[materialId];
    vtkMultiPieceDataSet *resolvedFragments
      = dynamic_cast<vtkMultiPieceDataSet *>(this->ResolvedFragments->GetBlock(materialId));
    assert( "Couldn't get the resolved fragnments." 
             && resolvedFragments );

    // We don't have any fragments here
    if (resolvedFragmentIds.size()==0)
      {
      continue;
      }

    int firstLocalId=resolvedFragmentIds[0];
    vtkFieldData *rf0fd
     = dynamic_cast<vtkPolyData *>(resolvedFragments->GetPiece(firstLocalId))->GetFieldData();

    int nAttributes=rf0fd->GetNumberOfArrays();
    int nLocalFragments=resolvedFragmentIds.size();

    // write header
    fout << "Header:{" << endl;
    fout << "Number total:" << resolvedFragments->GetNumberOfPieces() << endl;
    fout << "Number local:" << nLocalFragments << endl;
    fout << "Number of attributes:" << nAttributes << endl;
    fout << "}" << endl;
    // write attribute names
    fout << "Attributes:{" << endl;
    for (int i=0; i<nAttributes; ++i)
      {
      vtkDataArray *aa=rf0fd->GetArray(i);
      fout << "\""
          << aa->GetName()
          << "\", "
          << aa->GetNumberOfComponents()
          << endl;
      }
    fout << "}" << endl;
    // write attributes
    fout << "Attribute Data:{" << endl;
    for (int i=0; i<nLocalFragments; ++i)
      {
      int globalId=resolvedFragmentIds[i];
      vtkFieldData *fd=resolvedFragments->GetPiece(globalId)->GetFieldData();
      // first
      vtkDataArray *aa=fd->GetArray(0);
      int nComps=aa->GetNumberOfComponents();
      vector<double> tup;
      tup.resize(nComps);
      CopyTuple(&tup[0],aa,nComps,0);
      fout << tup[0];
      for (int q=1; q<nComps; ++q)
        {
        fout << " " << tup[q];
        }
      // rest
      for (int j=1; j<nAttributes; ++j)
        {
        fout << ", ";
        aa=fd->GetArray(j);
        nComps=aa->GetNumberOfComponents();
        tup.resize(nComps);
        CopyTuple(&tup[0],aa,nComps,0);
        fout << tup[0];
        for (int q=1; q<nComps; ++q)
          {
          fout << " " << tup[q];
          }
        }
      fout << endl;
      }
    fout << "}" << endl;
    fout << "}" << endl;
    }

  fout.flush();
  fout.close();

  return 1;
}
