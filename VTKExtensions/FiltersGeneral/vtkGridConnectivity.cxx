/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGridConnectivity.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkEquivalenceSet.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

// Distributed:
// Find the max process global point id (face hash).
// Create a map of fragment id/process.
// Send face structures to process 0.
// Add remote faces to face hash (mapping fragment ids).

// Arbitrary maximum.  Cells are 3D.
#define VTK_MAX_FACES_PER_CELL 12

/*
Fragment with integration that works on distributed unstrucutred grids.

Fragment Id Array.
What if we loop over all cells.
  look at neighbors to find existing fragment id.
  Assign new one of none found.
  Add an equivalence if two or more are found.
We have a separate array of fragment ids to assign equivalences.

How about multiple processes?
  Using global cell indexes, we will create a face neighbor array.
  Then just send the fragment ids of the boundary cells to process 0.
  Process 0 will update the equivalence array
  and send the results back to the other processes.

How do we compute/store the global connectivity?
  we could do it on the fly from point to cell references.
6 maximum faces.

Send the point (global id, fragment id) ordered pairs.
or  for each face (fragment id, corner ids).
N^2 => no good.
Array over global point ids.  pointing to cell id?  Fragment id?

Worry aobut this later.  Get something working...


*/

vtkStandardNewMacro(vtkGridConnectivity);

//=============================================================================
// This code implements the face hash that keeps track of external faces and
// there links back to cells.  Only worry about the three lowest corner
// point ids.  Assume that any two faces that share three points are
// the same face.

// It would be nice to have a face id, but lets just keep the
// struct pointer as an id since the object is internal anyway.
// This class is getting a little chunky.
// The surface should be much smaller than the input volume,
// so the memory should not be an issue.
class vtkGridConnectivityFace
{
public:
  // We only need to keep track of one cell.  We delete from the hash
  // any face with two cells because it is internal.
  short ProcessId;
  int BlockId;
  // The index of the cell in the block/
  vtkIdType CellId;
  // The index of the face in the cell.
  unsigned char FaceId;
  // There is no need to keep an array of cell fragemnts.
  // Storing it in the face should be good enough.
  int FragmentId;

  // This is used for merging hashes from multiple processes.
  // It is the index of the face in the original process.
  // I use it to create a mask to return to the original process.
  vtkIdType MarshalId;

  // Linked list.
  vtkGridConnectivityFace* NextFace;

  // The three smallest ids for indexing the face.
  // First id is implicit in the hash.  These are global point ids.
  vtkIdType CornerId2;
  vtkIdType CornerId3;
};

//=============================================================================

class vtkGridConnectivityFaceHeap
{
public:
  vtkGridConnectivityFaceHeap();
  ~vtkGridConnectivityFaceHeap();

  vtkGridConnectivityFace* NewFace();
  // The face will be valid until NewFace is called or the heap destructs.
  void RecycleFace(vtkGridConnectivityFace* face);

private:
  // Hard code the allocation size.
  // We could do fancy doubling, but why bother.
  // The cost of allocating another chink is small because
  // we do not copy.
  int NumberOfFacesPerAllocation;
  void Allocate();

  // Ivars that allow fast allocation of face objects.
  // They are allocated a few hundred at a time, and reused.
  vtkGridConnectivityFace* RecycleBin;
  vtkGridConnectivityFace* Heap;
  int HeapLength;
  // Which heap face is next in line.
  int NextFaceIndex;

  // The faces are allocated in arrays/heaps, so they need to be
  // deleted in arrays.  The first face from every array is reserved
  // and saved in a linked list so we can delete the arrays later.
  vtkGridConnectivityFace* Heaps;
};

vtkGridConnectivityFaceHeap::vtkGridConnectivityFaceHeap()
{
  this->NumberOfFacesPerAllocation = 1000;
  this->RecycleBin = nullptr;
  this->Heap = nullptr;
  this->HeapLength = 0;
  this->NextFaceIndex = 0;
  this->Heaps = nullptr;
}

vtkGridConnectivityFaceHeap::~vtkGridConnectivityFaceHeap()
{
  this->NumberOfFacesPerAllocation = 0;
  this->RecycleBin = nullptr;
  while (this->Heaps)
  {
    vtkGridConnectivityFace* next = this->Heaps->NextFace;
    delete[] this->Heaps;
    this->Heaps = next;
  }
}

void vtkGridConnectivityFaceHeap::Allocate()
{
  vtkGridConnectivityFace* newHeap = new vtkGridConnectivityFace[this->NumberOfFacesPerAllocation];
  // Use the first element to construct a lined list of arrays/heaps.
  newHeap[0].NextFace = this->Heaps;
  this->Heaps = newHeap;

  this->HeapLength = this->NumberOfFacesPerAllocation;
  this->NextFaceIndex = 1; // Skip the first which is used for the Heaps linked list.
  this->Heap = newHeap;
}

// The face will be valid until NewFace is called or the heap destructs.
void vtkGridConnectivityFaceHeap::RecycleFace(vtkGridConnectivityFace* face)
{
  // I do not initialize the face values because the face will be reference
  // even after it is recycled.  Actually, only the fragment pointer is actually referenced.

  // The recylcle bin is a linked list of faces.
  face->NextFace = this->RecycleBin;
  this->RecycleBin = face;
}

vtkGridConnectivityFace* vtkGridConnectivityFaceHeap::NewFace()
{
  vtkGridConnectivityFace* face;

  // First look for faces to use in the recycle bin.
  if (this->RecycleBin)
  {
    face = this->RecycleBin;
    this->RecycleBin = face->NextFace;
    face->NextFace = nullptr;
  }
  else
  {
    // Nothing to recycle.  Get a face from the heap.
    if (this->NextFaceIndex >= this->HeapLength)
    {
      // We need to allocate another heap.
      this->Allocate();
    }
    face = this->Heap + this->NextFaceIndex++;
  }
  face->CornerId2 = 0;
  face->CornerId3 = 0;
  // We only need one cell id, because we delete from the hash
  // any face with two cells because it is internal.
  face->BlockId = 0;
  face->CellId = 0;
  face->FaceId = 0;
  face->FragmentId = 0;
  face->NextFace = nullptr;

  return face;
}

//=============================================================================
class vtkGridConnectivityFaceHash
{
public:
  vtkGridConnectivityFaceHash();
  ~vtkGridConnectivityFaceHash();

  // Returns the number of faces in the hash (faces returned by iteration).
  vtkIdType GetNumberOfFaces() { return this->NumberOfFaces; }

  // This creates a hash that expects at most "numberOfPoint" points
  // as indexes.
  void Initialize(vtkIdType numberOfPoints);

  // These methods ass a face to the hash.  The four point method is for convenience.
  // The points do not need to be sorted.
  // If this is a new face, this method constructs the object and adds it to the hash.
  // The cell id is set and the face is returned.
  // If this face is already in the hash, the object is removed from the hash
  // and is returned.  The face is automatically recycled, but is valid until
  // this method is called again.
  // Note: I will assume that the ptIds are unique (i.e. pt1 != pt2 ...)
  vtkGridConnectivityFace* AddFace(vtkIdType pt1, vtkIdType pt2, vtkIdType pt3);
  vtkGridConnectivityFace* AddFace(vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4);

  // A way to iterate over the faces in the hash.
  void InitTraversal();
  // Return 0 when finished.
  vtkGridConnectivityFace* GetNextFace();
  // Since the face does not store the first point id explicitley,
  // this returns the id from the iterator state.
  vtkIdType GetFirstPointIndex() { return this->IteratorIndex; }

private:
  // Keep track of the number of faces in the hash for convenience.
  // The user does not need to iterate over all faces to count them.
  vtkIdType NumberOfFaces;

  // Array indexed by faces smallest corner id.
  // Each element is a linked list of faces that share the point.
  vtkGridConnectivityFace** Hash;
  vtkIdType NumberOfPoints;

  // Allocates faces efficiently.
  vtkGridConnectivityFaceHeap* Heap;

  // This class is too simple and internal for a separate iterator object.
  vtkIdType IteratorIndex;
  vtkGridConnectivityFace* IteratorCurrent;
};

vtkGridConnectivityFaceHash::vtkGridConnectivityFaceHash()
{
  this->Hash = nullptr;
  this->NumberOfPoints = 0;
  this->Heap = new vtkGridConnectivityFaceHeap;

  this->IteratorIndex = -1;
  this->IteratorCurrent = nullptr;
  this->NumberOfFaces = 0;
}

vtkGridConnectivityFaceHash::~vtkGridConnectivityFaceHash()
{
  if (this->Hash)
  {
    delete[] this->Hash;
    this->Hash = nullptr;
  }
  delete this->Heap;
  this->Heap = nullptr;
  this->IteratorIndex = 0;
  this->IteratorCurrent = nullptr;
  this->NumberOfFaces = 0;
}

void vtkGridConnectivityFaceHash::InitTraversal()
{
  // I had a decision: current points to the last or next face.
  // I chose last because it simplifies InitTraveral.
  // I just initialize the IteratorIndex as -1 (special value).
  // This assume that vtkIdType is signed!
  this->IteratorIndex = -1;
  this->IteratorCurrent = nullptr;
}

vtkGridConnectivityFace* vtkGridConnectivityFaceHash::GetNextFace()
{
  if (this->IteratorIndex >= this->NumberOfPoints)
  { // Past the end of the hash.  User must not have initialized.
    return nullptr;
  }
  // Traverse the linked list.
  if (this->IteratorCurrent)
  {
    this->IteratorCurrent = this->IteratorCurrent->NextFace;
  }
  // If we hit the end of the linked list,  move through heap to
  // find the next linked list.
  while (this->IteratorCurrent == nullptr)
  {
    ++this->IteratorIndex;
    if (this->IteratorIndex >= this->NumberOfPoints)
    {
      return nullptr;
    }
    this->IteratorCurrent = this->Hash[this->IteratorIndex];
  }

  return this->IteratorCurrent;
}

void vtkGridConnectivityFaceHash::Initialize(vtkIdType numberOfPoints)
{
  if (this->Hash)
  {
    vtkGenericWarningMacro("You can only initialize once.\n");
    return;
  }
  this->Hash = new vtkGridConnectivityFace*[numberOfPoints];
  this->NumberOfPoints = numberOfPoints;
  memset(this->Hash, 0, sizeof(vtkGridConnectivityFace*) * numberOfPoints);
}

vtkGridConnectivityFace* vtkGridConnectivityFaceHash::AddFace(
  vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4)
{
  // Find the three smallest point ids.
  if (pt1 > pt2 && pt1 > pt3 && pt1 > pt4)
  {
    return this->AddFace(pt2, pt3, pt4);
  }
  if (pt2 > pt3 && pt2 > pt4)
  {
    return this->AddFace(pt1, pt3, pt4);
  }
  if (pt3 > pt4)
  {
    return this->AddFace(pt1, pt2, pt4);
  }
  return this->AddFace(pt1, pt2, pt3);
}

vtkGridConnectivityFace* vtkGridConnectivityFaceHash::AddFace(
  vtkIdType pt1, vtkIdType pt2, vtkIdType pt3)
{
  // Sort the three ids.
  // Assume the three ids are unique.
  if (pt2 < pt1)
  {
    vtkIdType tmp = pt1;
    pt1 = pt2;
    pt2 = tmp;
  }
  if (pt3 < pt1)
  {
    vtkIdType tmp = pt1;
    pt1 = pt3;
    pt3 = tmp;
  }
  if (pt3 < pt2)
  {
    vtkIdType tmp = pt2;
    pt2 = pt3;
    pt3 = tmp;
  }

  // Note: We do not check if the point index is out of bounds.

  // Now look for the face in the hash.
  vtkGridConnectivityFace** ref = this->Hash + pt1; // Keep old ref for editing.
  vtkGridConnectivityFace* face = *ref;
  while (face)
  {
    if (face->CornerId2 == pt2 && face->CornerId3 == pt3)
    {
      // Found the face.
      // Remove it from the hash.
      *ref = face->NextFace;
      face->NextFace = nullptr;
      this->Heap->RecycleFace(face);
      --this->NumberOfFaces;
      // The face will still be valid until the next cycle.
      return face;
    }
    ref = &face->NextFace;
    face = face->NextFace;
  }
  // This is a new face.
  face = this->Heap->NewFace();
  face->CornerId2 = pt2;
  face->CornerId3 = pt3;
  // Add the face to the hash.
  *ref = face;

  ++this->NumberOfFaces;

  return face;
}

//=============================================================================

// Algorithm:
// Loop through cells.
//   Loop though faces of cell.
//     Build a list of face structures pointing to other cells.
//     Use a hash to find faces.  Hash only keeps faces with one cell.
//   Look for the lowest fragment Id.
//   If none, create a new fragment id.
//   Integrate current cell into fragment.
//   Equate all other connecting fragment ids.
// Resolve fragments.
// Extract all remaining faces from hash and create polydata.
// Each face structure remembers input,cell,face indexes.

//-----------------------------------------------------------------------------
vtkGridConnectivity::vtkGridConnectivity()
{
  this->EquivalenceSet = nullptr;
  this->FragmentVolumes = nullptr;
  this->FaceHash = nullptr;
  this->Controller = vtkMultiProcessController::GetGlobalController();
  this->ProcessId = this->Controller ? this->Controller->GetLocalProcessId() : 0;
}

//-----------------------------------------------------------------------------
vtkGridConnectivity::~vtkGridConnectivity()
{
  this->Controller = nullptr;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkGridConnectivity::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkGridConnectivity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkGridConnectivity::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
// Returns 1/true if the input has all of the needed arrays.
int vtkGridConnectivity::CheckInput(vtkUnstructuredGrid* input)
{
  vtkDataArray* a = input->GetPointData()->GetGlobalIds();
  if (a && (a->GetDataType() == VTK_ID_TYPE || a->GetDataType() == VTK_INT))
  {
    return 1;
  }
  vtkWarningMacro("Missing pedigree node array.");
  return 0;
}

//----------------------------------------------------------------------------
// This method computes partial fragments for a local process.
// Partial fragments are connected cells that are discovered by a simple
// pass through the cells (looking at neighbors).  I expect that the number
// of partial fragments will be close to the number of final fragments.
// The equivalence set will be used to merge partial fragments that touch.
// This method also integrates arrays for partial fragments.
template <class T>
void vtkGridConnectivityExecuteProcess(vtkGridConnectivity* self, vtkUnstructuredGrid* inputs[],
  int numberOfInputs, int processId, vtkGridConnectivityFaceHash* faceHash,
  vtkEquivalenceSet* equivalenceSet, T* globalPtIdPtr)
{
  // Essentially a count of the fragment ids we have used so far.
  // We start counting from 1 so 0 can be a special value used to remove faces.
  int nextFragmentId = 1;

  // I used to allocate a cell array for fragment ids but
  // THIS ARRAY WAS NOT NECESSARY.  WE JUST KEEP THE FRAGMENT IDS
  // IN THE FACE STRUCTURES!
  // The only issue we would have is that we do not assign the fragment id
  // unitl after all the faces fragemntId pointser have been set.
  // The pointer sets the face fragment id after the fact.
  // We could loop over the faces twice.
  // int* cellFragments = new int[totalNumberOfCellsInProcess];
  // int* cellFragmentPtr = cellFragments;

  // Loop through all cells of all inputs adding all faces to hash.
  // Select fragment id for each cell based on neighbors.  We are hoping that
  // cell order will be spatial and will not be random.
  for (int ii = 0; ii < numberOfInputs; ++ii)
  {
    vtkDataArray* a = inputs[ii]->GetPointData()->GetGlobalIds();
    void* ptr = a->GetVoidPointer(0);
    globalPtIdPtr = static_cast<T*>(ptr);
    vtkIdType numCells = inputs[ii]->GetNumberOfCells();
    // The status array is a mask that identifies unused cells.
    vtkDoubleArray* statusArray =
      vtkDoubleArray::SafeDownCast(inputs[ii]->GetCellData()->GetArray("STATUS"));
    // locate cell-ghost levels if present. We are going to skip over ghost
    // cells.
    vtkUnsignedCharArray* ghostArray = inputs[ii]->GetCellGhostArray();
    if (ghostArray &&
      (ghostArray->GetNumberOfComponents() != 1 || ghostArray->GetNumberOfTuples() != numCells))
    {
      vtkGenericWarningMacro("Poorly formed ghost cells. Ignoring them.");
      ghostArray = nullptr;
    }
    double* statusPtr = nullptr;
    if (statusArray)
    {
      statusPtr = statusArray->GetPointer(0);
    }
    for (vtkIdType jj = 0; jj < numCells; ++jj)
    {
      if (ghostArray && ghostArray->GetValue(jj) & vtkDataSetAttributes::DUPLICATECELL)
      {
        continue;
      }
      if (statusPtr == nullptr || *statusPtr++ == 0.0)
      {
        // Loop through faces of the cell.
        // This might be an performance bottle neck. (cell api)
        vtkCell* cell = inputs[ii]->GetCell(jj);
        int numFaces = cell->GetNumberOfFaces();
        vtkGridConnectivityFace* newFaces[VTK_MAX_FACES_PER_CELL];
        int numNewFaces = 0;
        // As we create / find faces, keep track of the smallest fragment id.
        int minFragmentId = nextFragmentId;
        for (int kk = 0; kk < numFaces; ++kk)
        {
          vtkGridConnectivityFace* face;
          vtkCell* faceCell = cell->GetFace(kk);
          vtkIdType numPoints = faceCell->GetNumberOfPoints();
          if (numPoints == 3)
          {
            vtkIdType ptId1 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(0)]);
            vtkIdType ptId2 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(1)]);
            vtkIdType ptId3 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(2)]);
            face = faceHash->AddFace(ptId1, ptId2, ptId3);
          }
          else if (numPoints == 4)
          {
            vtkIdType ptId1 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(0)]);
            vtkIdType ptId2 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(1)]);
            vtkIdType ptId3 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(2)]);
            vtkIdType ptId4 = static_cast<vtkIdType>(globalPtIdPtr[faceCell->GetPointId(3)]);
            face = faceHash->AddFace(ptId1, ptId2, ptId3, ptId4);
          }
          else
          {
            vtkGenericWarningMacro("Face ignored.");
            face = nullptr;
          }
          if (face)
          {
            if (face->FragmentId > 0)
            { // face is attached to another cell.
              // It has been removed from the hash because it is internal.
              // It is valid until we create another face. (recycle bin)
              if (face->FragmentId != minFragmentId && minFragmentId < nextFragmentId)
              {
                // This cell connects two fragments, we need
                // to make the fragment ids equivalent.
                equivalenceSet->AddEquivalence(minFragmentId, face->FragmentId);
              }
              // Keep track of the smallest fragment id to use for this cell.
              if (minFragmentId > face->FragmentId)
              {
                minFragmentId = face->FragmentId;
              }
            } // end if shared face removed from hash.
            else
            { // Face is new.  Add our cell info.
              // These are needed to create the surface in the second stage.
              face->ProcessId = processId;
              face->BlockId = ii;
              face->CellId = jj;
              face->FaceId = kk;
              // We need to save all new faces until we know the final fragment id.
              if (numNewFaces >= VTK_MAX_FACES_PER_CELL)
              {
                vtkGenericWarningMacro("Too many faces.");
              }
              else
              {
                newFaces[numNewFaces++] = face;
              }
            } // end if new face added to hash.
          }   // end if valid input face
        }     // Faces
        // Is this the start of a new fragment?
        if (minFragmentId == nextFragmentId)
        { // Cell has no neighbors (traversed yet). New fragment id.
          // Make sure the equivalence set has the correct number of members.
          equivalenceSet->AddEquivalence(nextFragmentId, nextFragmentId);
          nextFragmentId++;
        }
        // I do not think that the equivalence set has a more up to date id,
        // but it cannot hurt to check/
        minFragmentId = equivalenceSet->GetEquivalentSetId(minFragmentId);
        // Label the faces with the fragment id we computed.
        for (int kk = 0; kk < numNewFaces; ++kk)
        {
          newFaces[kk]->FragmentId = minFragmentId;
        }
        // Integrare volume of cell into fragemnt volume array.
        self->IntegrateCellVolume(cell, minFragmentId, inputs[ii], jj);
      } // if status
    }   // for cells
  }     // for input
}

//----------------------------------------------------------------------------
template <class T>
vtkIdType vtkGridConnectivityComputeMax(T* ptr, vtkIdType num)
{
  T max = 0;
  while (num-- > 0)
  {
    if (*ptr > max)
    {
      max = *ptr;
    }
    ++ptr;
  }
  return static_cast<vtkIdType>(max);
}

//-----------------------------------------------------------------------------
void vtkGridConnectivity::InitializeFaceHash(vtkUnstructuredGrid** inputs, int numberOfInputs)
{
  vtkIdType maxId = 0;

  // We need to find the maximum global point Id to initialize the face hash.
  for (int ii = 0; ii < numberOfInputs; ++ii)
  {
    vtkDataArray* a = inputs[ii]->GetPointData()->GetGlobalIds();
    void* ptr = a->GetVoidPointer(0);
    vtkIdType numIds = a->GetNumberOfTuples();
    maxId = 0;
    this->GlobalPointIdType = a->GetDataType();
    switch (this->GlobalPointIdType)
    {
      vtkTemplateMacro(maxId = vtkGridConnectivityComputeMax(static_cast<VTK_TT*>(ptr), numIds));
      default:
        vtkErrorMacro("ThreadedRequestData: Unknown input ScalarType");
        return;
    }
  }

  // Now we need to compute the maximum of all processes.
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (this->Controller->GetLocalProcessId() != 0)
  {
    this->Controller->Send(&maxId, 1, 0, 897324);
    // Only process 0 needs a hash to hold all procs faces.
  }
  else
  {
    for (int ii = 1; ii < numProcs; ++ii)
    {
      vtkIdType tmp;
      this->Controller->Receive(&tmp, 1, ii, 897324);
      if (tmp > maxId)
      {
        maxId = tmp;
      }
    }
  }

  if (this->FaceHash)
  {
    delete this->FaceHash;
  }
  this->FaceHash = new vtkGridConnectivityFaceHash;
  this->FaceHash->Initialize(maxId + 1);
}

//-----------------------------------------------------------------------------
// We may want to selectively integrate arrays in the future.
void vtkGridConnectivity::InitializeIntegrationArrays(
  vtkUnstructuredGrid** inputs, int numberOfInputs)
{
  // The fragment volume array integrates the volume
  // for each fragment id.
  this->FragmentVolumes = vtkDoubleArray::New();

  // This assumes every input has the same arrays.
  int arrayIndex;
  if (numberOfInputs <= 0)
  {
    return;
  }
  int numCellArrays = inputs[0]->GetCellData()->GetNumberOfArrays();
  for (arrayIndex = 0; arrayIndex < numCellArrays; ++arrayIndex)
  {
    // Only double ararys for now.
    vtkDataArray* tmp = inputs[0]->GetCellData()->GetArray(arrayIndex);
    vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(tmp);
    // We do not handle integrating vectors yet.
    if (da && da->GetNumberOfComponents() == 1 && strcmp(da->GetName(), "STATUS") != 0)
    {
      vtkSmartPointer<vtkDoubleArray> integrationArray;
      integrationArray = vtkSmartPointer<vtkDoubleArray>::New();
      integrationArray->SetName(da->GetName());
      this->CellAttributesIntegration.push_back(integrationArray);
    }
  }

  // only supports double arrays
  int numPointArrays = inputs[0]->GetPointData()->GetNumberOfArrays();
  for (arrayIndex = 0; arrayIndex < numPointArrays; ++arrayIndex)
  {
    vtkDoubleArray* da =
      vtkDoubleArray::SafeDownCast(inputs[0]->GetPointData()->GetArray(arrayIndex));

    if (da)
    {
      vtkSmartPointer<vtkDoubleArray> integrationArray;
      integrationArray = vtkSmartPointer<vtkDoubleArray>::New();
      integrationArray->SetName(da->GetName());
      integrationArray->SetNumberOfComponents(da->GetNumberOfComponents());
      this->PointAttributesIntegration.push_back(integrationArray);
    }
  }
}

//-----------------------------------------------------------------------------
// The standard execute method.
int vtkGridConnectivity::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* mbdsOutput =
    vtkMultiBlockDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!mbdsOutput)
  {
    return 0;
  }
  vtkPolyData* output = vtkPolyData::New();
  mbdsOutput->SetNumberOfBlocks(1);
  mbdsOutput->SetBlock(0, output);
  output->Delete();

  // Create a simple array of input blocks.
  //  This will simplify creating faces from the hash.
  int numberOfInputs = 0;
  vtkUnstructuredGrid** inputs = nullptr;
  // We need this to allocate the fragment array.
  vtkIdType totalNumberOfCellsInProcess = 0;

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* doInput = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(doInput);
  vtkUnstructuredGrid* ugInput = vtkUnstructuredGrid::SafeDownCast(doInput);

  if (ugInput)
  {
    if (this->CheckInput(ugInput))
    {
      numberOfInputs = 1;
      inputs = new vtkUnstructuredGrid*[1];
      inputs[0] = ugInput;
      totalNumberOfCellsInProcess = ugInput->GetNumberOfCells();
    }
  }
  else if (hdInput)
  {
    // Count the number of inputs
    vtkCompositeDataIterator* iter = hdInput->NewIterator();
    iter->GoToFirstItem();
    numberOfInputs = 0;
    while (!iter->IsDoneWithTraversal())
    {
      ugInput = vtkUnstructuredGrid::SafeDownCast(iter->GetCurrentDataObject());
      if (ugInput && this->CheckInput(ugInput))
      {
        ++numberOfInputs;
      }
      iter->GoToNextItem();
    }
    // Now make the list.
    inputs = new vtkUnstructuredGrid*[numberOfInputs];
    int inputIdx = 0;
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      ugInput = vtkUnstructuredGrid::SafeDownCast(dobj);
      if (ugInput && this->CheckInput(ugInput))
      {
        inputs[inputIdx++] = ugInput;
        totalNumberOfCellsInProcess += ugInput->GetNumberOfCells();
      }
      else
      {
        if (dobj)
        {
          vtkWarningMacro("This filter cannot handle sub-datasets of type : "
            << dobj->GetClassName() << ". Skipping block");
        }
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }
  else
  {
    if (doInput)
    {
      vtkWarningMacro("This filter cannot handle data of type : " << doInput->GetClassName());
    }
  }

  // The equivalenceSet keeps track of fragment ids and which
  // fragment ids need to be combined into a single fragment.
  this->EquivalenceSet = vtkEquivalenceSet::New();
  // Create the integration arrays that hold the volume and
  // integrated values for each attribute.  The arrays
  // are initialized to 0 and indexed by fragment id.
  this->InitializeIntegrationArrays(inputs, numberOfInputs);
  // We need to know the maximum globalNodeId (across all processes)
  // to initialize the face hash.  This methods computes it and initializes.
  this->InitializeFaceHash(inputs, numberOfInputs);

  switch (this->GlobalPointIdType)
  {
    vtkTemplateMacro(vtkGridConnectivityExecuteProcess(this, inputs, numberOfInputs,
      this->ProcessId, this->FaceHash, this->EquivalenceSet, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro("ExecuteProcess: Unknown input ScalarType");
      return 0;
  }

  // Deal with distributed data. Send all polygons to a single process.
  // Find equivalences in process 0.
  // If this becomes a bottleneck,  we could build up an occupancy map indexed
  // by global node ids.
  // This also combines the volume integration of the partial fragment volumes
  // into final volumes indexed by the resolved fragment ids.
  // Note: the ids start from 1.  This is because we started assigning partial fragment ids
  // from 1 so the equivalence set has a entry for 0 even though it is not used.
  this->ResolveProcessesFaces();

  // Use the face hash and integration data to generate the output surface.
  this->GenerateOutput(output, inputs);

  delete[] inputs;
  // The check is not necessary.  It will always be allocated by this point.
  if (this->FaceHash)
  {
    delete this->FaceHash;
  }
  this->FaceHash = nullptr;
  this->EquivalenceSet->Delete();
  this->EquivalenceSet = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
// Use the face hash and integration data to generate the output surface.
void vtkGridConnectivity::GenerateOutput(vtkPolyData* output, vtkUnstructuredGrid* inputs[])
{
  int numCellArrays, numPointArrays;
  vtkDoubleArray* da;
  vtkDoubleArray* outArray;
  // Use the face hash to create cells in the output.
  // Only outside faces will be left in the hash because all faces attached
  // to two cells were removed.
  this->FaceHash->InitTraversal();
  vtkGridConnectivityFace* face;
  vtkIntArray* cellFragmentIdArray = vtkIntArray::New();
  cellFragmentIdArray->SetName("FragmentId");
  vtkDoubleArray* volumeArray = vtkDoubleArray::New();
  volumeArray->SetName("Volume");

  // Create all of the integration arrays.
  numCellArrays = static_cast<int>(this->CellAttributesIntegration.size());
  for (int ii = 0; ii < numCellArrays; ++ii)
  {
    da = this->CellAttributesIntegration[ii];
    outArray = vtkDoubleArray::New();
    outArray->SetName(da->GetName());
    output->GetCellData()->AddArray(outArray);
    outArray->Delete();
  }

  numPointArrays = static_cast<int>(this->PointAttributesIntegration.size());
  for (int ii = 0; ii < numPointArrays; ++ii)
  {
    da = this->PointAttributesIntegration[ii];
    outArray = vtkDoubleArray::New();
    outArray->SetName(da->GetName());
    outArray->SetNumberOfComponents(da->GetNumberOfComponents());
    output->GetPointData()->AddArray(outArray);
    outArray->Delete();
  }

  vtkPoints* outPoints = vtkPoints::New();
  output->SetPoints(outPoints);
  vtkCellArray* outCells = vtkCellArray::New();
  output->SetPolys(outCells);

  // For debugging
  vtkIdTypeArray* blockIdArray = vtkIdTypeArray::New();
  blockIdArray->SetName("BlockId");
  vtkIdTypeArray* cellIdArray = vtkIdTypeArray::New();
  cellIdArray->SetName("CellId");

  while ((face = this->FaceHash->GetNextFace()))
  {
    // Skip the faces that were masked by the interprocess resolution.
    // The special mask value is 0.  Fragment index starts at 1.
    if (face->FragmentId > 0)
    { // Valid face
      vtkUnstructuredGrid* input = inputs[face->BlockId];
      vtkPoints* inputPoints = input->GetPoints();
      vtkCell* cell = input->GetCell(face->CellId);
      vtkCell* cellFace = cell->GetFace(face->FaceId);
      // Lets duplicate points for now.
      // We could set up a point map between input and output later.
      vtkIdType numFacePts = cellFace->GetNumberOfPoints();
      if (numFacePts > 4)
      {
        numFacePts = 4;
        vtkWarningMacro("Polygon too big.");
      }
      vtkIdType outCellPtIds[4];
      for (int ii = 0; ii < numFacePts; ++ii)
      {
        vtkIdType ptId = cellFace->GetPointId(ii);
        double pt[3];
        inputPoints->GetPoint(ptId, pt);
        outCellPtIds[ii] = outPoints->InsertNextPoint(pt);
      }
      outCells->InsertNextCell(numFacePts, outCellPtIds);
      cellFragmentIdArray->InsertNextValue(face->FragmentId);

      // There is no need to pass the fragment ids though the
      // equivalence set because the faces have been changed when
      // the equivalence set was resolved.
      volumeArray->InsertNextValue(this->FragmentVolumes->GetValue(face->FragmentId));

      // Insert values for the integration arrays.
      for (int ii = 0; ii < numCellArrays; ++ii)
      {
        da = this->CellAttributesIntegration[ii];
        outArray = vtkDoubleArray::SafeDownCast(output->GetCellData()->GetArray(da->GetName()));
        if (outArray == nullptr)
        {
          vtkErrorMacro("Missing cell integration array.");
          continue;
        }

        outArray->InsertNextValue(da->GetValue(face->FragmentId));
      }

      // Insert values for the integration arrays.
      for (int ii = 0; ii < numPointArrays; ++ii)
      {
        da = this->PointAttributesIntegration[ii];
        outArray = vtkDoubleArray::SafeDownCast(output->GetPointData()->GetArray(da->GetName()));
        if (outArray == nullptr)
        {
          vtkErrorMacro("Missing point integration array.");
          continue;
        }
        for (vtkIdType p = 0; p < numFacePts; ++p)
        {
          outArray->InsertNextTuple(face->FragmentId, da);
        }
      }

      // These arrays are just for debugging.
      blockIdArray->InsertNextValue(face->BlockId);
      cellIdArray->InsertNextValue(face->CellId);
      // Worry about arrays later.
    } // end if face is on exterior
  }   // end loop over faces in hash.
  output->GetCellData()->SetScalars(cellFragmentIdArray);
  output->GetCellData()->AddArray(volumeArray);

  output->GetCellData()->AddArray(blockIdArray);
  output->GetCellData()->AddArray(cellIdArray);
  this->FragmentVolumes->SetName("Fragment Volume");
  output->GetFieldData()->AddArray(this->FragmentVolumes);

  // Add all of the integration arrays to field data.
  // Should we change the names to integrated...?
  numCellArrays = static_cast<int>(this->CellAttributesIntegration.size());
  for (int ii = 0; ii < numCellArrays; ++ii)
  {
    da = this->CellAttributesIntegration[ii];
    output->GetFieldData()->AddArray(da);
  }

  // Add all of the integration arrays to field data.
  // Should we change the names to integrated...?
  numPointArrays = static_cast<int>(this->PointAttributesIntegration.size());
  for (int ii = 0; ii < numPointArrays; ++ii)
  {
    da = this->PointAttributesIntegration[ii];
    output->GetFieldData()->AddArray(da);
  }

  cellFragmentIdArray->Delete();
  volumeArray->Delete();
  this->FragmentVolumes->Delete();
  this->FragmentVolumes = nullptr;
  this->CellAttributesIntegration.clear();
  this->PointAttributesIntegration.clear();

  blockIdArray->Delete();
  cellIdArray->Delete();
  outPoints->Delete();
  outCells->Delete();
}

// To Extend this to integrate all arrays, we will first start with
// Cell attributes.
// We will have to make FragmentVolumes into an array with one entry
// each attribute.
// We will have to transmit each array around just like the volumes.
//----------------------------------------------------------------------------
// Compute the volume for this cell and add it to the fragment volume arrray.
// Initialize or extend the array if necessary.
void vtkGridConnectivity::IntegrateCellVolume(
  vtkCell* cell, int fragmentId, vtkUnstructuredGrid* input, vtkIdType cellIndex)
{
  double* volumePtr;

  if (cell->GetCellDimension() != 3)
  {
    vtkErrorMacro("Expecting only 3d cells.");
    return;
  }

  // Make sure the fragment volume array is big enough.
  vtkIdType length = this->FragmentVolumes->GetNumberOfTuples();
  if (length <= fragmentId)
  {
    vtkIdType newLength = fragmentId * 2 + 200;
    this->FragmentVolumes->Resize(newLength);
    this->FragmentVolumes->SetNumberOfTuples(fragmentId + 1);
    // Initialize new values to 0.0
    volumePtr = this->FragmentVolumes->GetPointer(length);
    for (vtkIdType ii = length; ii < newLength; ++ii)
    {
      *volumePtr++ = 0.0;
    }
    // Resize all of the integration arrays to be the same.
    int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
    for (int ii = 0; ii < numArrays; ++ii)
    {
      vtkDoubleArray* da = this->CellAttributesIntegration[ii];
      da->Resize(newLength);
      da->SetNumberOfTuples(fragmentId + 1);
      // Initialize new values to 0.0
      volumePtr = da->GetPointer(length);
      for (vtkIdType jj = length; jj < newLength; ++jj)
      {
        *volumePtr++ = 0.0;
      }
    }

    // resize all of the point integration arrays
    numArrays = static_cast<int>(this->PointAttributesIntegration.size());
    for (int ii = 0; ii < numArrays; ++ii)
    {
      vtkDoubleArray* da = this->PointAttributesIntegration[ii];
      da->Resize(newLength);
      da->SetNumberOfTuples(fragmentId + 1);
      // Initialize new values to 0.0
      for (vtkIdType jj = length; jj < newLength; ++jj)
      {
        for (int comp = 0; comp < da->GetNumberOfComponents(); ++comp)
        {
          da->SetComponent(jj, comp, 0.0);
        }
      }
    }
  }

  // Now compute the volume of the cell.
  int cellType = cell->GetCellType();
  double cellVolume = 0.0;
  switch (cellType)
  {
    case VTK_HEXAHEDRON:
      cellVolume = this->IntegrateHex(cell, input, fragmentId);
      break;
    case VTK_VOXEL:
      cellVolume = this->IntegrateVoxel(cell, input, fragmentId);
      break;
    case VTK_TETRA:
      cellVolume = this->IntegrateTetrahedron(cell, input, fragmentId);
      break;
    default:
    {
      cell->Triangulate(1, this->CellPointIds, this->CellPoints);
      cellVolume = this->IntegrateGeneral3DCell(cell, input, fragmentId);
    }
  }

  // update the volume integration
  volumePtr = this->FragmentVolumes->GetPointer(fragmentId);
  *volumePtr += cellVolume;

  // Integrate all of the cell arrays.
  int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
  for (int ii = 0; ii < numArrays; ++ii)
  {
    vtkDoubleArray* da = this->CellAttributesIntegration[ii];
    vtkDoubleArray* inputArray =
      vtkDoubleArray::SafeDownCast(input->GetCellData()->GetArray(da->GetName()));
    if (inputArray == nullptr)
    {
      vtkErrorMacro("Missing integration array.");
      continue;
    }
    double* ptr = da->GetPointer(fragmentId);
    double attributeValue = inputArray->GetValue(cellIndex);
    *ptr += attributeValue * cellVolume;
  }
}

//----------------------------------------------------------------------------
// This method expects that every process has raw (unresolved) equivalence set
// faces and arrays.  At the end of this method, process 0 has face hash,
// equivalence set and arrays merged from all processes, and resolved.
// Every thing looks like it was generated locally (single arrays...).
// Faces in the root process hash have a processId and marshalId to indicate
// the source process.  This is important because only the source process has
// the inputs used to create the surface geometry.  The marshalId is the index
// of the face in the originating process hash.
// All processes call this method at the same time (potential to hang).
// The fragmentIdMap is an empty array allocated byu the caller.
// The function that maps the local fragment ids to global fragment ids
// is returned in the array.
void vtkGridConnectivity::CollectFacesAndArraysToRootProcess(
  int* fragmentIdMap, int* fragmentNumFaces)
{
  vtkIdType msg1[2];
  vtkIdType numFaces;
  vtkIdType msgLength;
  vtkIdType* msg2;
  vtkIdType* msgPtr;
  vtkIdType corner1, corner2, corner3;
  vtkIdType blockId, faceId, cellId, fragmentId;
  vtkGridConnectivityFace* face;
  int numberOfFragments;

  if (this->Controller->GetLocalProcessId() != 0)
  { // Remote process
    // Resolve equivaleneces Before we send the faces.
    // This is important because we do not send the equivalence set to process 0.
    this->ResolveEquivalentFragments();
    // Marshal the hash and send it to process 0.
    numFaces = this->FaceHash->GetNumberOfFaces();
    // Number of fragments.  I assuem this is the
    // resolved number of fragments, but it will little difference.
    vtkIdType numFragments = this->EquivalenceSet->GetNumberOfResolvedSets();
    msg1[0] = numFragments;
    msg1[1] = numFaces;
    this->Controller->Send(msg1, 2, 0, 890831);
    if (numFaces > 0)
    {
      // Lets put everything into a single vtkIdType array
      msgLength = numFaces * 7;
      msg2 = new vtkIdType[msgLength];
      msgPtr = msg2;
      this->FaceHash->InitTraversal();
      while ((face = this->FaceHash->GetNextFace()))
      {
        *msgPtr++ = this->FaceHash->GetFirstPointIndex();
        *msgPtr++ = face->CornerId2;
        *msgPtr++ = face->CornerId3;
        *msgPtr++ = face->BlockId;
        *msgPtr++ = face->CellId;
        *msgPtr++ = face->FaceId;
        *msgPtr++ = face->FragmentId;
      }
      this->Controller->Send(msg2, msgLength, 0, 344897);
      delete[] msg2;
      // Now send the integration arrays (volume).
      this->Controller->Send(this->FragmentVolumes->GetPointer(0), numFragments, 0, 634780);

      // send all point and cell information
      int tag = 634780;
      int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
      this->Controller->Send(&numArrays, 1, 0, ++tag);
      for (int i = 0; i < numArrays; ++i)
      {
        this->Controller->Send(this->CellAttributesIntegration.at(i), 0, ++tag);
      }

      numArrays = static_cast<int>(this->PointAttributesIntegration.size());
      this->Controller->Send(&numArrays, 1, 0, ++tag);
      for (int i = 0; i < numArrays; ++i)
      {
        this->Controller->Send(this->PointAttributesIntegration.at(i), 0, ++tag);
      }
    } // endif numFaces > 0
  }   // end if not process 0
  else
  { // Process is 0
    // Build up a map to make each processes fragment
    int numProcs = this->Controller->GetNumberOfProcesses();
    fragmentIdMap[0] = 0;
    fragmentIdMap[1] = this->EquivalenceSet->GetNumberOfMembers();
    fragmentNumFaces[0] = 0;
    for (int procIdx = 1; procIdx < numProcs; ++procIdx)
    { // loop over every process.
      // Receive
      this->Controller->Receive(msg1, 2, procIdx, 890831);
      numberOfFragments = msg1[0];
      numFaces = msg1[1];
      // Create a map to assign global fragment ids.
      fragmentIdMap[procIdx + 1] = fragmentIdMap[procIdx] + numberOfFragments;
      fragmentNumFaces[procIdx] = numFaces;
      if (numFaces > 0)
      {
        // Receive faces from a remote process.
        msgLength = numFaces * 7;
        msg2 = new vtkIdType[msgLength];
        this->Controller->Receive(msg2, msgLength, procIdx, 344897);
        msgPtr = msg2;
        for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
        {
          corner1 = *msgPtr++;
          corner2 = *msgPtr++;
          corner3 = *msgPtr++;
          blockId = *msgPtr++;
          cellId = *msgPtr++;
          faceId = *msgPtr++;
          fragmentId = *msgPtr++;
          // Translate the fragmentId to global value.
          fragmentId += fragmentIdMap[procIdx];
          // Add the face to the hash.
          face = this->FaceHash->AddFace(corner1, corner2, corner3);
          if (face->FragmentId > 0)
          { // face is attached to another cell.
            // It has been removed from the hash because it is internal.
            // It is valid until we create another face. (recycle bin)
            // This cell connects two fragments, we need
            // to make the fragment ids equivalent.
            this->EquivalenceSet->AddEquivalence(fragmentId, face->FragmentId);
          }
          else
          { // Face is new.  Add our cell info.
            // These are needed to create the surface in the second stage.
            face->ProcessId = procIdx;
            face->BlockId = blockId;
            face->CellId = cellId;
            face->FaceId = faceId;
            face->FragmentId = fragmentId;
            face->MarshalId = faceIdx;
          } // end if face is new or shared.
        }   // end loop over faces in messages
        // Now receive the integration arrays and add them to ours.
        this->FragmentVolumes->Resize(fragmentIdMap[procIdx + 1]);
        // I hope the vtkDataArray allocates more than we ask.
        this->FragmentVolumes->SetNumberOfTuples(fragmentIdMap[procIdx + 1]);
        // Read the array right into the end of our array.
        this->Controller->Receive(this->FragmentVolumes->GetPointer(fragmentIdMap[procIdx]),
          numberOfFragments, procIdx, 634780);

        // get all the point and cell information from each processor
        // we don't use gather, since it presumes equal length arrays on each processor
        int tag = 634780;
        int numArrays = 0;
        vtkDoubleArray* da;
        vtkDoubleArray* rec;
        vtkIdType daSize;
        vtkIdType recSize;

        this->Controller->Receive(&numArrays, 1, procIdx, ++tag);
        for (int i = 0; i < numArrays; ++i)
        {
          rec = vtkDoubleArray::New();
          this->Controller->Receive(rec, procIdx, ++tag);

          // now that we have the array from the client, append it to our array
          da = this->CellAttributesIntegration.at(i);
          daSize = da->GetNumberOfTuples();
          recSize = rec->GetNumberOfTuples();
          da->Resize(daSize + recSize);
          da->SetNumberOfTuples(daSize + recSize);
          for (int j = 0; j < recSize; ++j)
          {
            da->SetValue(daSize + j, rec->GetValue(j));
          }
          rec->Delete();
        }

        this->Controller->Receive(&numArrays, 1, procIdx, ++tag);
        for (int i = 0; i < numArrays; ++i)
        {
          rec = vtkDoubleArray::New();
          this->Controller->Receive(rec, procIdx, ++tag);

          // now that we have the array from the client, append it to our array
          da = this->PointAttributesIntegration.at(i);
          daSize = da->GetNumberOfTuples();
          recSize = rec->GetNumberOfTuples();
          da->Resize(daSize + recSize);
          da->SetNumberOfTuples(daSize + recSize);
          for (int j = 0; j < recSize; ++j)
          {
            for (int k = 0; k < da->GetNumberOfComponents(); ++k)
            {
              da->SetComponent(daSize + j, k, rec->GetComponent(j, k));
            }
          }
          rec->Delete();
        }
      } // endif numFace > 0
    }   // end loop over processes
  }     // end if process 0.

  if (this->Controller->GetLocalProcessId() == 0)
  {
    // Resolve all the fragments, faces and arrays for all processes.
    this->ResolveEquivalentFragments();
  }
}

//----------------------------------------------------------------------------
// This method expects every process to have local faces, equivalences set
// ind integration arrays.
// At the end, the equivalence set and integration arrays are resolved
// across all processes and the faces shared between processes (internal)
// have been removed from the hash.
//
// The algorithm is:  Collect number of faces to process 0.
// Create a map from process fragment ids to global fragment ids.
// Collect face hashes to process 0 and merge into 1.
// Send a mask/fragmentIdArray indexed by original process fragment id
// back to originating process.
// The only issue I ran into is returning triangles to their original
// process.  I am going to create a mask/fragment array to return.
// The hash has to remember the original process face index to
// construct the mask.
// For resolving integration arrays, Each process needs:
// A full map of localFragmentIds,procOwner to globalFragId,procOwner.
void vtkGridConnectivity::ResolveProcessesFaces()
{
  vtkIdType numFaces;
  int numProcs = this->Controller->GetNumberOfProcesses();
  int* fragmentIdMap = new int[numProcs + 1];
  int* fragmentNumFaces = new int[numProcs + 1];

  this->CollectFacesAndArraysToRootProcess(fragmentIdMap, fragmentNumFaces);

  // Now send the mask and arrays back to the remote processes.
  // It may not be necessary to send the entire volume array to all processes,
  // but it is easy.  There should not be too many fragments anyway.
  if (this->Controller->GetLocalProcessId() != 0)
  { // Remote process
    // Now receive the triangles that survived the resolution process.
    numFaces = this->FaceHash->GetNumberOfFaces();
    int* fragmentIds = new int[numFaces];
    int* fragmentIdPtr = fragmentIds;
    if (numFaces)
    {
      this->Controller->Receive(fragmentIds, numFaces, 0, 234301);
      // Get rid of faces shared by multiple processe and
      // set the resolved fragment ids.
      this->FaceHash->InitTraversal();
      vtkGridConnectivityFace* face;
      while ((face = this->FaceHash->GetNextFace()))
      {
        // I do not want to remove the face from the hash because
        // we are in the middle of traversing the hash.
        // It would not work the way the iterator is implemented.
        // The invalid fragment id (value 0) will be enough to skip faces.
        face->FragmentId = *fragmentIdPtr++;
      }
      delete[] fragmentIds;
      vtkIdType numFragments;
      this->Controller->Receive(&numFragments, 1, 0, 909034);
      this->FragmentVolumes->SetNumberOfTuples(numFragments);
      this->Controller->Receive(this->FragmentVolumes->GetPointer(0), numFragments, 0, 909035);

      // receive all point and cell information
      int tag = 909035;
      int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
      for (int i = 0; i < numArrays; ++i)
      {
        this->Controller->Receive(this->CellAttributesIntegration.at(i), 0, ++tag);
      }

      numArrays = static_cast<int>(this->PointAttributesIntegration.size());
      for (int i = 0; i < numArrays; ++i)
      {
        this->Controller->Receive(this->PointAttributesIntegration.at(i), 0, ++tag);
      }
    } // endif numFaces != 0
  }   // endif remote process.
  else
  { // if process 0 (root)
    // Now we have to distribute the new face hash back
    // to the remote processes.  We have to loop through the hash
    // to construct the face mask for each process.
    // Lets create them one by one (looping many times) rather
    // than all at once.  The code will be simpler and
    // it should not be too expensive.
    for (int procIdx = 1; procIdx < numProcs; ++procIdx)
    {
      // decode the number of faces from the offsets.
      vtkGridConnectivityFace* face;
      numFaces = fragmentNumFaces[procIdx];
      if (numFaces)
      { // just in case new or MPI does not like 0 length arrays.
        // Construct the mask / message
        int* faceMask = new int[numFaces];
        // Initialize the mask to "empty set".
        // I am going to index the final fragments starting at 1
        // so I can use 0 as the special "remove face" value.
        memset(faceMask, 0, numFaces * sizeof(int));
        // Loop over the faces in the hash.
        this->FaceHash->InitTraversal();
        while ((face = this->FaceHash->GetNextFace()) != nullptr)
        {
          if (face->ProcessId == procIdx)
          {
            faceMask[face->MarshalId] = face->FragmentId;
          } // end if face belongs to process.
        }   // endloop face in hash
        // Send the mask to the remote process.
        this->Controller->Send(faceMask, numFaces, procIdx, 234301);
        delete[] faceMask;

        // We need to send the volume array to the remote processes too.
        vtkIdType numFragments = this->FragmentVolumes->GetNumberOfTuples();
        this->Controller->Send(&numFragments, 1, procIdx, 909034);
        this->Controller->Send(this->FragmentVolumes->GetPointer(0), numFragments, procIdx, 909035);

        // send all point and cell information
        int tag = 909035;
        int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
        for (int i = 0; i < numArrays; ++i)
        {
          this->Controller->Send(this->CellAttributesIntegration.at(i), procIdx, ++tag);
        }

        numArrays = static_cast<int>(this->PointAttributesIntegration.size());
        for (int i = 0; i < numArrays; ++i)
        {
          this->Controller->Send(this->PointAttributesIntegration.at(i), procIdx, ++tag);
        }
      } // endif numFaces > 0
    }   // endloop procIdx
  }     // endelse process 0 (root)

  /*
      // Terms:  PartialFragmentId: Local process frag id before resolution.
      //         GlobalFragmentId:  PartialFragmentId offset to form global id.
      //         FinalFragmentId:

      // Now that we know the NumberOfFinal fragments, collect the volumes.
      vtkDoubleArray* globalVolumes = vtkDoubleArray::New();
      globalVolumes->SetNumberOfValues(
      for (int procIdx = 1; procIdx < numProcs; ++procIdx)
        { // loop over every process.


    double* msgVolume;

        // Now receive the volume array index on partial fragmentIds.
        this->Controller->Receive(msgVolume, numberOfFragments,procIdx,729987);

  */

  delete[] fragmentIdMap;
  delete[] fragmentNumFaces;
}

//----------------------------------------------------------------------------
// This method expects faces in the has, unresolved equivalence set and
// integration arrays (volume) indexed bny the partial fragments.
// This method resolves the fragments, changes the fragment ids in the faces
// and merges entries in the volume array for merged fragments.  The new
// volume array is indexed by the resolved fragments.
void vtkGridConnectivity::ResolveEquivalentFragments()
{
  this->EquivalenceSet->ResolveEquivalences();
  this->ResolveIntegrationArrays();
  this->ResolveFaceFragmentIds();
}

//----------------------------------------------------------------------------
// This method expects that the equivalence set has been resolved,
// but the integration arrays (volume) are still indexed by the partial
// volume.  After this method, new integration arrays are indexed by the
// resolved equivalent ids.  All partial ids that belong to a set
// are summed to get the output integration values.
//
// Create a new fragment volume array indexed by the resolved fragment ids.
// Merge intermediate fragment volume array to get final values.
void vtkGridConnectivity::ResolveIntegrationArrays()
{
  if (!this->EquivalenceSet->Resolved)
  {
    vtkErrorMacro("Equivalences not resolved.");
    return;
  }

  vtkDoubleArray* newVolumes = vtkDoubleArray::New();
  int numSets = this->EquivalenceSet->GetNumberOfResolvedSets();
  newVolumes->SetNumberOfTuples(numSets);
  // Initialize all values to 0 to start sumation.
  memset(newVolumes->GetPointer(0), 0, numSets * sizeof(double));
  // Loop over all the partial fragments summing volumes.
  int numMembers = this->EquivalenceSet->GetNumberOfMembers();
  if (this->FragmentVolumes->GetNumberOfTuples() < numMembers)
  {
    vtkErrorMacro("More partial fragments than volume entries.");
    return;
  }
  double* partialVolumePtr = this->FragmentVolumes->GetPointer(0);
  double* finalVolumePtr = newVolumes->GetPointer(0);
  for (int ii = 0; ii < numMembers; ++ii)
  {
    int setId = this->EquivalenceSet->GetEquivalentSetId(ii);
    finalVolumePtr[setId] += *partialVolumePtr;
    // update to the next fragment volume
    ++partialVolumePtr;
  }

  // Now replace the old partial fragment volume array with the
  // new total fragment volume array.
  this->FragmentVolumes->Delete();
  this->FragmentVolumes = newVolumes;

  // we now need to update all the cell integration arrays
  // do not update when the EquivalenceSet matches an item to itself
  int numArrays = static_cast<int>(this->CellAttributesIntegration.size());
  for (int j = 0; j < numArrays; ++j)
  {
    vtkDoubleArray* da = this->CellAttributesIntegration[j];
    for (int i = 0; i < da->GetNumberOfTuples(); ++i)
    {
      int setId = this->EquivalenceSet->GetEquivalentSetId(i);
      if (i != setId)
      {
        double* oldIntegrationPtr = da->GetPointer(i);
        double* resolvedPtr = da->GetPointer(setId);
        *resolvedPtr += *oldIntegrationPtr;
      }
    }
    // now that we have shrunken the array, actual resize it
    da->Resize(numSets);
  }

  // update point attributes
  numArrays = static_cast<int>(this->PointAttributesIntegration.size());
  for (int j = 0; j < numArrays; ++j)
  {
    vtkDoubleArray* da = this->PointAttributesIntegration[j];
    for (int i = 0; i < da->GetNumberOfTuples(); ++i)
    {
      int setId = this->EquivalenceSet->GetEquivalentSetId(i);
      if (i != setId)
      {
        for (int k = 0; k < da->GetNumberOfComponents(); ++k)
        {
          double old = da->GetComponent(i, k);
          double resolved = da->GetComponent(setId, k);
          resolved += old;
          da->SetComponent(setId, k, resolved);
        }
      }
    }
    // now that we have shrunken the array, actual resize it
    da->Resize(numSets);
  }
}

//----------------------------------------------------------------------------
// This method expects that the equivalence set has been resolved,
// but the faces in the has still have partial fragment ids.
// At the end of this method, all the face fragment ids have been passed
// through the equivalence set so that they point to the resolved fragments.
void vtkGridConnectivity::ResolveFaceFragmentIds()
{
  vtkGridConnectivityFace* face;
  this->FaceHash->InitTraversal();
  while ((face = this->FaceHash->GetNextFace()))
  {
    face->FragmentId = this->EquivalenceSet->GetEquivalentSetId(face->FragmentId);
  }
}

//-----------------------------------------------------------------------------
double vtkGridConnectivity::ComputeTetrahedronVolume(
  double* pts0, double* pts1, double* pts2, double* pts3)
{
  double a[3], b[3], c[3], n[3];
  int i;
  // Compute the principle vectors around pt0 and the
  // centroid
  for (i = 0; i < 3; i++)
  {
    a[i] = pts1[i] - pts0[i];
    b[i] = pts2[i] - pts0[i];
    c[i] = pts3[i] - pts0[i];
  }

  // Calculate the volume of the tet which is 1/6 * the box product
  vtkMath::Cross(a, b, n);
  return fabs(vtkMath::Dot(c, n) / 6.0);
}
//-----------------------------------------------------------------------------
void vtkGridConnectivity::ComputePointIntegration(vtkUnstructuredGrid* input, vtkIdType pt0Id,
  vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double volume, int fragmentId)
{
  // Integrate all of the point arrays.
  int numArrays = static_cast<int>(this->PointAttributesIntegration.size());
  for (int i = 0; i < numArrays; ++i)
  {
    vtkDoubleArray* da = this->PointAttributesIntegration[i];
    vtkDoubleArray* inputArray =
      vtkDoubleArray::SafeDownCast(input->GetPointData()->GetArray(da->GetName()));
    if (inputArray == nullptr)
    {
      vtkErrorMacro("Missing integration array.");
      continue;
    }

    double sum, outValue;
    for (int j = 0; j < inputArray->GetNumberOfComponents(); ++j)
    {
      sum = inputArray->GetComponent(pt0Id, j);
      sum += inputArray->GetComponent(pt1Id, j);
      sum += inputArray->GetComponent(pt2Id, j);
      sum += inputArray->GetComponent(pt3Id, j);

      outValue = da->GetComponent(fragmentId, j);
      outValue += ((sum * 0.25) * volume);
      da->SetComponent(fragmentId, j, outValue);
    }
  }
}

//-----------------------------------------------------------------------------
double vtkGridConnectivity::IntegrateTetrahedron(
  vtkCell* tetra, vtkUnstructuredGrid* input, int fragmentId)
{
  double pts[4][3];
  vtkPoints* points = tetra->GetPoints();
  points->GetPoint(0, pts[0]);
  points->GetPoint(1, pts[1]);
  points->GetPoint(2, pts[2]);
  points->GetPoint(3, pts[3]);

  double volume = this->ComputeTetrahedronVolume(pts[0], pts[1], pts[2], pts[3]);

  vtkIdType ptIds[4];
  ptIds[0] = tetra->GetPointId(0);
  ptIds[1] = tetra->GetPointId(1);
  ptIds[2] = tetra->GetPointId(2);
  ptIds[3] = tetra->GetPointId(3);
  this->ComputePointIntegration(input, ptIds[0], ptIds[1], ptIds[2], ptIds[3], volume, fragmentId);
  return volume;
}

//-----------------------------------------------------------------------------
// For axis aligned hexahedral cells
double vtkGridConnectivity::IntegrateHex(vtkCell* hex, vtkUnstructuredGrid* input, int fragmentId)
{
  vtkPoints* points = hex->GetPoints();
  double pts[8][3];
  points->GetPoint(0, pts[0]);
  points->GetPoint(1, pts[1]);
  points->GetPoint(2, pts[2]);
  points->GetPoint(3, pts[3]);
  points->GetPoint(4, pts[4]);
  points->GetPoint(5, pts[5]);
  points->GetPoint(6, pts[6]);
  points->GetPoint(7, pts[7]);

  vtkIdType ptIds[8];
  ptIds[0] = hex->GetPointId(0);
  ptIds[1] = hex->GetPointId(1);
  ptIds[2] = hex->GetPointId(2);
  ptIds[3] = hex->GetPointId(3);
  ptIds[4] = hex->GetPointId(4);
  ptIds[5] = hex->GetPointId(5);
  ptIds[6] = hex->GetPointId(6);
  ptIds[7] = hex->GetPointId(7);

  // For volume, I will tetrahedralize and add the volume of each tetra.
  double volume = 0.0, tetraVolume = 0.0;
  tetraVolume = this->ComputeTetrahedronVolume(pts[0], pts[1], pts[3], pts[4]);
  this->ComputePointIntegration(
    input, ptIds[0], ptIds[1], ptIds[3], ptIds[4], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[5], pts[6], pts[1], pts[4]);
  this->ComputePointIntegration(
    input, ptIds[5], ptIds[6], ptIds[1], ptIds[4], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[7], pts[6], pts[4], pts[3]);
  this->ComputePointIntegration(
    input, ptIds[7], ptIds[6], ptIds[4], ptIds[3], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[1], pts[6], pts[2], pts[3]);
  this->ComputePointIntegration(
    input, ptIds[1], ptIds[6], ptIds[2], ptIds[3], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[4], pts[6], pts[1], pts[3]);
  this->ComputePointIntegration(
    input, ptIds[4], ptIds[6], ptIds[1], ptIds[3], tetraVolume, fragmentId);
  volume += tetraVolume;

  return volume;
}

//-----------------------------------------------------------------------------
// For axis aligned hexahedral cells
double vtkGridConnectivity::IntegrateVoxel(
  vtkCell* voxel, vtkUnstructuredGrid* input, int fragmentId)
{
  vtkPoints* points = voxel->GetPoints();
  double pts[8][3];
  points->GetPoint(0, pts[0]);
  points->GetPoint(1, pts[1]);
  points->GetPoint(2, pts[2]);
  points->GetPoint(3, pts[3]);
  points->GetPoint(4, pts[4]);
  points->GetPoint(5, pts[5]);
  points->GetPoint(6, pts[6]);
  points->GetPoint(7, pts[7]);

  vtkIdType ptIds[8];
  ptIds[0] = voxel->GetPointId(0);
  ptIds[1] = voxel->GetPointId(1);
  ptIds[2] = voxel->GetPointId(2);
  ptIds[3] = voxel->GetPointId(3);
  ptIds[4] = voxel->GetPointId(4);
  ptIds[5] = voxel->GetPointId(5);
  ptIds[6] = voxel->GetPointId(6);
  ptIds[7] = voxel->GetPointId(7);

  // For volume, I will tetrahedralize and add the volume of each tetra.
  double volume = 0.0, tetraVolume = 0.0;
  tetraVolume = this->ComputeTetrahedronVolume(pts[0], pts[1], pts[2], pts[4]);
  this->ComputePointIntegration(
    input, ptIds[0], ptIds[1], ptIds[2], ptIds[4], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[5], pts[7], pts[1], pts[4]);
  this->ComputePointIntegration(
    input, ptIds[5], ptIds[7], ptIds[1], ptIds[4], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[6], pts[7], pts[4], pts[2]);
  this->ComputePointIntegration(
    input, ptIds[6], ptIds[7], ptIds[4], ptIds[2], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[1], pts[7], pts[3], pts[2]);
  this->ComputePointIntegration(
    input, ptIds[1], ptIds[7], ptIds[3], ptIds[2], tetraVolume, fragmentId);
  volume += tetraVolume;

  tetraVolume = this->ComputeTetrahedronVolume(pts[4], pts[7], pts[1], pts[2]);
  this->ComputePointIntegration(
    input, ptIds[4], ptIds[7], ptIds[1], ptIds[2], tetraVolume, fragmentId);
  volume += tetraVolume;

  return volume;
}

//-----------------------------------------------------------------------------
double vtkGridConnectivity::IntegrateGeneral3DCell(
  vtkCell* vtkNotUsed(cell), vtkUnstructuredGrid*, int)
{
  /*
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (nPnts % 4)
    {
    vtkWarningMacro("Number of points ("
                    << nPnts << ") is not divisiable by 4 - skipping "
                    << " 3D Cell: " << cellId);
    return;
    }

  vtkIdType tetIdx = 0;
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;

  while (tetIdx < nPnts)
    {
    pt1Id = ptIds->GetId(tetIdx++);
    pt2Id = ptIds->GetId(tetIdx++);
    pt3Id = ptIds->GetId(tetIdx++);
    pt4Id = ptIds->GetId(tetIdx++);
    this->IntegrateTetrahedron(input, output, cellId, pt1Id, pt2Id, pt3Id,
                               pt4Id);
    }
  */
  vtkWarningMacro("Complex cell not handled.");

  return 0.0;
}
