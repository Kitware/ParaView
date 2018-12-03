/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridConnectivity.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkEquivalenceSet.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <map>
#include <string>
#include <vector>

vtkStandardNewMacro(vtkRectilinearGridConnectivity);

// An extended marching cubes case table for generating cube faces (either
// truncated by iso-lines or not) in addition to iso-triangles. These two
// kinds of polygons in combination represent the surface(s) of the greater-
// than-isovalue sub-volume(s) extracted in a cube. An index in each entry
// may refer to an Interpolated Iso-Value Point (IIVP, i.e., the associated
// edge, 0 ~ 11) or one of the eight Vertices Of the Cube (VOC: 12 ~ 19).
// The constituent polygons (triangles, quads, or pentagons) of a sub-volume
// are listed successively, ending with flag -1 in sepration from another sub-
// volume that may follows. Each polygon begins with the number of the points
// followed by the specific IIVP-Ids and VOC-Ids. Flag -2 terminates the list
// of all sub-volumes, if any. The two integers in the annotation section of
// each entry indicate the case number (0 ~ 255) and the base case number (0
// ~ 15), respectively.

struct vtkRectilinearGridConnectivityMarchingCubesVolumeCases
{
  int PointIds[68];
  static int GetEntrySize() { return 68; }
  static vtkRectilinearGridConnectivityMarchingCubesVolumeCases* GetCases();
};

// ============================================================================
// ======================== Supporting Classes (begin) ========================
// ============================================================================

#define VTK_MAX_FACES_PER_CELL 12

class vtkRectilinearGridConnectivityInternal
{
public:
  int ComponentNumbersObtained;
  int NumberIntegralComponents;
  int VolumeFractionArraysType;
  double VolumeFractionValueScale;
  std::vector<int> ComponentNumbersPerArray;
  std::vector<std::string> VolumeFractionArrayNames;
  std::vector<std::string> VolumeDataAttributeNames;
  std::vector<std::string> IntegrableAttributeNames;

  int IntegrablePointDataArraysAvailable(vtkRectilinearGrid* rectGrid)
  {
    int numArays = static_cast<int>(this->IntegrableAttributeNames.size());
    int allExist = numArays;

    for (int i = 0; i < numArays && allExist; i++)
    {
      allExist = vtkDoubleArray::SafeDownCast(
                   rectGrid->GetPointData()->GetArray(this->IntegrableAttributeNames[i].c_str()))
        ? allExist
        : 0;
    }

    return allExist;
  }

  int IntegrableCellDataArraysAvailable(vtkPolyData* polyData)
  {
    int numArays = static_cast<int>(this->IntegrableAttributeNames.size());
    int allExist = numArays;

    for (int i = 0; i < numArays && allExist; i++)
    {
      allExist = vtkDoubleArray::SafeDownCast(
                   polyData->GetCellData()->GetArray(this->IntegrableAttributeNames[i].c_str()))
        ? allExist
        : 0;
    }

    return allExist;
  }
};

//============================================================================

class vtkRectilinearGridConnectivityFace
{
public:
  // It is currently assumed that there are not so many blocks, fragments, and
  // processes. In case this assumption does not hold, 'short' needs to be
  // changed to 'int'
  short BlockId;       // for intra-process inter-block fragments extraction
  short FragmentId;    // for any level fragments extraction
  short ProcessId;     // for inter-process fragments extraction
  vtkIdType PolygonId; // for any level fragments extraction
                       // global index of the face / polygon in a vtkPolyData

  // three smallest global point-Ids are used for a unique representation of a
  // polygon (triangle, quad, or pentagon) and the first one is actually the
  // index of the entry in the face hash
  vtkIdType PointId2;
  vtkIdType PointId3;

  vtkRectilinearGridConnectivityFace* NextFace; // next face in a linked list
};

//============================================================================

class vtkRectilinearGridConnectivityFaceHeap
{
public:
  vtkRectilinearGridConnectivityFaceHeap();
  ~vtkRectilinearGridConnectivityFaceHeap();

  vtkRectilinearGridConnectivityFace* NewFace();

  // The face is valid until NewFace is called or the heap destructs.
  void RecycleFace(vtkRectilinearGridConnectivityFace* face);

private:
  int HeapLength;
  int NextFaceIndex; // Which heap face is next in line.

  // Hard code the allocation size.
  // We could do fancy doubling, but why bother.
  // The cost of allocating another chink is small because
  // we do not copy.
  int NumberOfFacesPerAllocation;

  // Ivars that allow fast allocation of face objects.
  // They are allocated a few hundred at a time, and reused.
  vtkRectilinearGridConnectivityFace* Heap;
  vtkRectilinearGridConnectivityFace* RecycleBin;

  // The faces are allocated in arrays/heaps, so they need to be
  // deleted in arrays.  The first face from every array is reserved
  // and saved in a linked list so we can delete the arrays later.
  vtkRectilinearGridConnectivityFace* Heaps;

  void Allocate();
};

vtkRectilinearGridConnectivityFaceHeap::vtkRectilinearGridConnectivityFaceHeap()
{
  this->Heap = 0;
  this->Heaps = 0;
  this->HeapLength = 0;
  this->RecycleBin = 0;
  this->NextFaceIndex = 0;
  this->NumberOfFacesPerAllocation = 1000;
}

vtkRectilinearGridConnectivityFaceHeap::~vtkRectilinearGridConnectivityFaceHeap()
{
  this->RecycleBin = 0;
  this->NumberOfFacesPerAllocation = 0;

  while (this->Heaps)
  {
    vtkRectilinearGridConnectivityFace* next = this->Heaps->NextFace;
    delete[] this->Heaps;
    this->Heaps = next;
  }
}

void vtkRectilinearGridConnectivityFaceHeap::Allocate()
{
  vtkRectilinearGridConnectivityFace* newHeap =
    new vtkRectilinearGridConnectivityFace[this->NumberOfFacesPerAllocation];

  // use the first element to construct a linked list of arrays/heaps
  newHeap[0].NextFace = this->Heaps;
  this->Heaps = newHeap;

  this->HeapLength = this->NumberOfFacesPerAllocation;
  this->NextFaceIndex = 1; // skip the first (used for the Heaps linked list)
  this->Heap = newHeap;
}

// The face will be valid until NewFace is called or the heap destructs.
void vtkRectilinearGridConnectivityFaceHeap::RecycleFace(vtkRectilinearGridConnectivityFace* face)
{
  face->NextFace = this->RecycleBin;
  this->RecycleBin = face;
}

vtkRectilinearGridConnectivityFace* vtkRectilinearGridConnectivityFaceHeap::NewFace()
{
  vtkRectilinearGridConnectivityFace* face = NULL;

  // look for faces to use in the recycle bin
  if (this->RecycleBin)
  {
    face = this->RecycleBin;
    this->RecycleBin = face->NextFace;
    face->NextFace = 0;
  }
  else
  {
    // nothing to recycle --> get a face from the heap
    if (this->NextFaceIndex >= this->HeapLength)
    {
      // we need to allocate another heap
      this->Allocate();
    }

    face = this->Heap + this->NextFaceIndex++;
  }

  face->PointId2 = 0;
  face->PointId3 = 0;

  // we only need one cell id, because we delete from the hash
  // any face with two cells because it is internal
  face->BlockId = 0;
  face->NextFace = 0;
  face->PolygonId = 0;
  face->FragmentId = 0;

  return face;
}

//============================================================================

class vtkRectilinearGridConnectivityFaceHash
{
public:
  vtkRectilinearGridConnectivityFaceHash();
  ~vtkRectilinearGridConnectivityFaceHash();

  // Returns the number of faces in the hash (faces returned by iteration).
  vtkIdType GetNumberOfFaces() { return this->NumberOfFaces; }

  // Creates a hash that expects at most "numberOfPoint" points as indexes
  void Initialize(vtkIdType numberOfPoints);

  // These methods ass a face to the hash.  The four point method is for
  // convenience. The points do not need to be sorted. If this is a new face,
  // this method constructs the object and adds it to the hash. The cell id is
  // set and the face is returned. If this face is already in the hash, the
  // object is removed from the hash and is returned.  The face is automatically
  // recycled, but is valid until this method is called again.
  // Note: I will assume that the ptIds are unique (i.e. pt1 != pt2 ...)
  vtkRectilinearGridConnectivityFace* AddFace(vtkIdType pt1, vtkIdType pt2, vtkIdType pt3);
  vtkRectilinearGridConnectivityFace* AddFace(
    vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4);
  vtkRectilinearGridConnectivityFace* AddFace(
    vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4, vtkIdType pt5);

  // A way to iterate over the faces in the hash.
  void InitTraversal();
  vtkRectilinearGridConnectivityFace* GetNextFace();

  // Since the face does not store the first point id explicitley,
  // this returns the id from the iterator state.
  vtkIdType GetFirstPointIndex() { return this->IteratorIndex; }

private:
  // Keep track of the number of faces in the hash for convenience.
  // The user does not need to iterate over all faces to count them.
  vtkIdType NumberOfFaces;

  // Array indexed by faces smallest corner id.
  // Each element is a linked list of faces that share the point.
  vtkIdType NumberOfPoints;
  vtkRectilinearGridConnectivityFace** Hash;

  // Allocates faces efficiently.
  vtkRectilinearGridConnectivityFaceHeap* Heap;

  // This class is too simple and internal for a separate iterator object.
  vtkIdType IteratorIndex;
  vtkRectilinearGridConnectivityFace* IteratorCurrent;
};

vtkRectilinearGridConnectivityFaceHash::vtkRectilinearGridConnectivityFaceHash()
{
  this->Hash = 0;
  this->Heap = new vtkRectilinearGridConnectivityFaceHeap;

  this->NumberOfPoints = 0;
  this->NumberOfFaces = 0;
  this->IteratorIndex = -1;
  this->IteratorCurrent = 0;
}

vtkRectilinearGridConnectivityFaceHash::~vtkRectilinearGridConnectivityFaceHash()
{
  if (this->Hash)
  {
    delete[] this->Hash;
    this->Hash = 0;
  }

  delete this->Heap;
  this->Heap = 0;

  this->IteratorIndex = 0;
  this->IteratorCurrent = 0;
  this->NumberOfFaces = 0;
}

void vtkRectilinearGridConnectivityFaceHash::InitTraversal()
{
  // I had a decision: current points to the last or next face.
  // I chose last because it simplifies InitTraveral.
  // I just initialize the IteratorIndex as -1 (special value).
  // This assume that vtkIdType is signed!
  this->IteratorIndex = -1;
  this->IteratorCurrent = 0;
}

vtkRectilinearGridConnectivityFace* vtkRectilinearGridConnectivityFaceHash::GetNextFace()
{
  if (this->IteratorIndex >= this->NumberOfPoints)
  {
    // past the end of the hash --- it has not been initialized.
    return 0;
  }

  // traverse the linked list.
  if (this->IteratorCurrent)
  {
    this->IteratorCurrent = this->IteratorCurrent->NextFace;
  }

  // move through heap to find the next linked list if we hit the end of
  // the current linked list,
  while (this->IteratorCurrent == 0)
  {
    this->IteratorIndex++;
    if (this->IteratorIndex >= this->NumberOfPoints)
    {
      return 0;
    }

    this->IteratorCurrent = this->Hash[this->IteratorIndex];
  }

  return this->IteratorCurrent;
}

void vtkRectilinearGridConnectivityFaceHash::Initialize(vtkIdType numberOfPoints)
{
  if (this->Hash)
  {
    vtkGenericWarningMacro("You can only initialize once.\n");
    return;
  }

  this->Hash = new vtkRectilinearGridConnectivityFace*[numberOfPoints];
  this->NumberOfPoints = numberOfPoints;

  memset(this->Hash, 0, sizeof(vtkRectilinearGridConnectivityFace*) * numberOfPoints);
}

vtkRectilinearGridConnectivityFace* vtkRectilinearGridConnectivityFaceHash::AddFace(
  vtkIdType pt1, vtkIdType pt2, vtkIdType pt3)
{
  // Sort the three ids. Assume the three ids are unique (actually not)
  vtkIdType tmp;
  if (pt2 < pt1)
  {
    tmp = pt1;
    pt1 = pt2;
    pt2 = tmp;
  }

  if (pt3 < pt1)
  {
    tmp = pt1;
    pt1 = pt3;
    pt3 = tmp;
  }

  if (pt3 < pt2)
  {
    tmp = pt2;
    pt2 = pt3;
    pt3 = tmp;
  }

  // Note: we do not check if the point index is out of bounds.

  // look for the face in the hash and keep old reference for editing
  vtkRectilinearGridConnectivityFace** ref = this->Hash + pt1;
  vtkRectilinearGridConnectivityFace* face = *ref;
  while (face)
  {
    if (face->PointId2 == pt2 && face->PointId3 == pt3)
    {
      // find the face and remove it from the hash
      *ref = face->NextFace;
      face->NextFace = 0;
      this->Heap->RecycleFace(face);
      this->NumberOfFaces--;

      // the face will still be valid until the next cycle.
      return face;
    }
    ref = &face->NextFace;
    face = face->NextFace;
  }
  // this is a new face.
  face = this->Heap->NewFace();
  face->PointId2 = pt2;
  face->PointId3 = pt3;

  // add the face to the hash.
  *ref = face;
  this->NumberOfFaces++;

  return face;
}

vtkRectilinearGridConnectivityFace* vtkRectilinearGridConnectivityFaceHash::AddFace(
  vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4)
{
  // we just need to find the three smallest point ids
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

vtkRectilinearGridConnectivityFace* vtkRectilinearGridConnectivityFaceHash::AddFace(
  vtkIdType pt1, vtkIdType pt2, vtkIdType pt3, vtkIdType pt4, vtkIdType pt5)
{
  vtkIdType tempIndx;
  vtkIdType pntIndxs[5] = { pt1, pt2, pt3, pt4, pt5 };

  // determine the two largest point Ids
  for (int j = 0; j < 2; j++)
    for (int i = j + 1; i < 5; i++)
    {
      if (pntIndxs[i] > pntIndxs[j])
      {
        tempIndx = pntIndxs[i];
        pntIndxs[i] = pntIndxs[j];
        pntIndxs[j] = tempIndx;
      }
    }

  // but submit the three smallest point Ids
  return this->AddFace(pntIndxs[2], pntIndxs[3], pntIndxs[4]);
}

// ============================================================================
// ======================== Supporting Classes ( end ) ========================
// ============================================================================

//-----------------------------------------------------------------------------
vtkRectilinearGridConnectivity::vtkRectilinearGridConnectivity()
{
  this->FaceHash = NULL;
  this->DualGridBlocks = NULL;
  this->NumberOfBlocks = 0;
  this->DualGridsReady = 0;
  this->DataBlocksTime = -1.0;
  this->DualGridBounds[0] = this->DualGridBounds[2] = this->DualGridBounds[4] = VTK_DOUBLE_MAX;
  this->DualGridBounds[1] = this->DualGridBounds[3] = this->DualGridBounds[5] = VTK_DOUBLE_MIN;
  this->EquivalenceSet = NULL;
  this->FragmentValues = NULL;

  this->Controller = vtkMultiProcessController::GetGlobalController();

  this->Internal = new vtkRectilinearGridConnectivityInternal;
  this->Internal->ComponentNumbersObtained = 0;
  this->Internal->NumberIntegralComponents = 0;
  this->Internal->VolumeFractionArraysType = 0;
  this->Internal->ComponentNumbersPerArray.clear();
  this->Internal->VolumeFractionArrayNames.clear();
  this->Internal->VolumeDataAttributeNames.clear();
  this->Internal->IntegrableAttributeNames.clear();
  this->Internal->VolumeFractionValueScale = 255.0;

  this->VolumeFractionSurfaceValue = 128.0 / 255.0;
}

//-----------------------------------------------------------------------------
vtkRectilinearGridConnectivity::~vtkRectilinearGridConnectivity()
{
  this->Controller = NULL;

  if (this->Internal)
  {
    this->Internal->ComponentNumbersPerArray.clear();
    this->Internal->VolumeFractionArrayNames.clear();
    this->Internal->VolumeDataAttributeNames.clear();
    this->Internal->IntegrableAttributeNames.clear();
    delete this->Internal;
    this->Internal = NULL;
  }

  if (this->FaceHash)
  {
    delete this->FaceHash;
    this->FaceHash = NULL;
  }

  if (this->EquivalenceSet)
  {
    this->EquivalenceSet->Delete();
    this->EquivalenceSet = NULL;
  }

  if (this->FragmentValues)
  {
    this->FragmentValues->Delete();
    this->FragmentValues = NULL;
  }

  if (this->DualGridBlocks && this->NumberOfBlocks)
  {
    for (int i = 0; i < this->NumberOfBlocks; i++)
    {
      this->DualGridBlocks[i]->Delete();
      this->DualGridBlocks[i] = NULL;
    }
    delete[] this->DualGridBlocks;
    this->DualGridBlocks = NULL;
  }
}

//-----------------------------------------------------------------------------
vtkExecutive* vtkRectilinearGridConnectivity::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::RemoveAllVolumeArrayNames()
{
  this->Internal->VolumeFractionArrayNames.erase(this->Internal->VolumeFractionArrayNames.begin(),
    this->Internal->VolumeFractionArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::RemoveDoubleVolumeArrayNames()
{
  if (this->Internal->VolumeFractionArraysType != VTK_DOUBLE)
  {
    return;
  }

  this->Internal->VolumeFractionArrayNames.erase(this->Internal->VolumeFractionArrayNames.begin(),
    this->Internal->VolumeFractionArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::RemoveFloatVolumeArrayNames()
{
  if (this->Internal->VolumeFractionArraysType != VTK_FLOAT)
  {
    return;
  }

  this->Internal->VolumeFractionArrayNames.erase(this->Internal->VolumeFractionArrayNames.begin(),
    this->Internal->VolumeFractionArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::RemoveUnsignedCharVolumeArrayNames()
{
  if (this->Internal->VolumeFractionArraysType != VTK_UNSIGNED_CHAR)
  {
    return;
  }

  this->Internal->VolumeFractionArrayNames.erase(this->Internal->VolumeFractionArrayNames.begin(),
    this->Internal->VolumeFractionArrayNames.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddVolumeArrayName(char* arayName)
{
  if (arayName == 0)
  {
    return;
  }

  this->Internal->VolumeFractionArraysType = 0;
  this->Internal->VolumeFractionArrayNames.push_back(arayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddDoubleVolumeArrayName(char* arayName)
{
  if (arayName == 0)
  {
    return;
  }

  if (this->Internal->VolumeFractionArraysType != VTK_DOUBLE)
  {
    this->RemoveAllVolumeArrayNames();
    this->Internal->VolumeFractionArraysType = VTK_DOUBLE;
  }

  this->Internal->VolumeFractionArrayNames.push_back(arayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddFloatVolumeArrayName(char* arayName)
{
  if (arayName == 0)
  {
    return;
  }

  if (this->Internal->VolumeFractionArraysType != VTK_FLOAT)
  {
    this->RemoveAllVolumeArrayNames();
    this->Internal->VolumeFractionArraysType = VTK_FLOAT;
  }

  this->Internal->VolumeFractionArrayNames.push_back(arayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddUnsignedCharVolumeArrayName(char* arayName)
{
  if (arayName == 0)
  {
    return;
  }

  if (this->Internal->VolumeFractionArraysType != VTK_UNSIGNED_CHAR)
  {
    this->RemoveAllVolumeArrayNames();
    this->Internal->VolumeFractionArraysType = VTK_UNSIGNED_CHAR;
  }

  this->Internal->VolumeFractionArrayNames.push_back(arayName);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkRectilinearGridConnectivity::GetNumberOfVolumeArrays()
{
  return static_cast<int>(this->Internal->VolumeDataAttributeNames.size());
}

//-----------------------------------------------------------------------------
int vtkRectilinearGridConnectivity::GetNumberOfVolumeFractionArrays()
{
  return static_cast<int>(this->Internal->VolumeFractionArrayNames.size());
}

//-----------------------------------------------------------------------------
const char* vtkRectilinearGridConnectivity::GetVolumeFractionArrayName(int arrayIdx)
{
  if (arrayIdx < 0 || arrayIdx >= static_cast<int>(this->Internal->VolumeFractionArrayNames.size()))
  {
    return 0;
  }
  return this->Internal->VolumeFractionArrayNames[arrayIdx].c_str();
}

//-----------------------------------------------------------------------------
bool vtkRectilinearGridConnectivity::IsVolumeArray(const char* arayName)
{
  int i;
  int numArrays = static_cast<int>(this->Internal->VolumeDataAttributeNames.size());

  for (i = 0; i < numArrays; i++)
  {
    if (!strcmp(arayName, this->Internal->VolumeDataAttributeNames[i].c_str()))
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkRectilinearGridConnectivity::IsVolumeFractionArray(const char* arayName)
{
  int i;
  int numArrays = static_cast<int>(this->Internal->VolumeFractionArrayNames.size());

  for (i = 0; i < numArrays; i++)
  {
    if (!strcmp(arayName, this->Internal->VolumeFractionArrayNames[i].c_str()))
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
int vtkRectilinearGridConnectivity::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Volume Fraction Surface Value: " << this->VolumeFractionSurfaceValue << "\n";
  os << indent << "Dual Grids Ready: " << this->DualGridsReady << "\n";
  os << indent << "Number of Blocks: " << this->NumberOfBlocks << "\n";
  os << indent << "Data Blocks Time: " << this->DataBlocksTime << "\n";
  os << indent << "Dual Grid Bounds: " << this->DualGridBounds[0] << ", " << this->DualGridBounds[1]
     << "; " << this->DualGridBounds[2] << ", " << this->DualGridBounds[3] << "; "
     << this->DualGridBounds[4] << ", " << this->DualGridBounds[5] << ".\n";
}

//-----------------------------------------------------------------------------
int vtkRectilinearGridConnectivity::CheckVolumeDataArrays(
  vtkRectilinearGrid** recGrids, int numGrids)
{
  if (!recGrids || numGrids <= 0)
  {
    vtkErrorMacro(<< "vtkRectilinearGrid array NULL or numGrids <= 0 " << endl);
    return 0;
  }

  int i, j;
  int arayType = -1;
  int tempType = 0;
  int beNormal = 1;
  int numFracs = 0;
  int numArays = 0;
  const char** aryNames = NULL;
  const char* arayName = NULL;
  vtkDataArray* cellAray = NULL;

  // check the number of arrays and specific array names

  if ((numArays = recGrids[0]->GetCellData()->GetNumberOfArrays()) <
    (numFracs = this->GetNumberOfVolumeFractionArrays()))
  {
    vtkErrorMacro(<< "Insufficient number of cell data arrays" << endl);
    return 0;
  }

  for (i = 0; i < numFracs; i++)
  {
    arayName = this->GetVolumeFractionArrayName(i);
    if (recGrids[0]->GetCellData()->GetArray(arayName) == NULL)
    {
      arayName = NULL;
      vtkErrorMacro(<< "Cell data array " << arayName << " not found." << endl);
      return 0;
    }
    arayName = NULL;
  }

  aryNames = new const char*[numArays];
  for (i = 0; i < numArays; i++)
  {
    aryNames[i] = recGrids[0]->GetCellData()->GetArrayName(i);
  }

  for (j = 1; j < numGrids && beNormal; j++)
  {
    beNormal = !(recGrids[j]->GetCellData()->GetNumberOfArrays() - numArays);

    for (i = 0; i < numArays && beNormal; i++)
    {
      beNormal = !strcmp(aryNames[i], recGrids[j]->GetCellData()->GetArrayName(i));
    }
  }

  if (!beNormal)
  {
    for (i = 0; i < numArays; i++)
    {
      aryNames[i] = NULL;
    }
    delete[] aryNames;
    aryNames = NULL;

    vtkErrorMacro(<< "Blocks inconsistent in the number of arrays "
                  << "or array names." << endl);
    return 0;
  }

  // check the volume fraction array(s)

  for (j = 0; j < numFracs && beNormal; j++)
  {
    arayName = this->GetVolumeFractionArrayName(j);
    for (i = 0; i < numGrids; i++)
    {
      cellAray = recGrids[i]->GetCellData()->GetArray(arayName);
      tempType = cellAray->GetDataType();
      if (tempType != VTK_FLOAT && tempType != VTK_DOUBLE && tempType != VTK_UNSIGNED_CHAR)
      {
        beNormal = 0;
        vtkErrorMacro(<< "Data type expected to be VTK_DOUBLE, VTK_FLOAT "
                      << "or VTK_UNSIGNED_CHAR." << endl);
        break;
      }

      if (arayType < 0)
      {
        arayType = tempType;
        this->Internal->VolumeFractionValueScale = (tempType == VTK_UNSIGNED_CHAR) ? 255.0 : 1.0;
      }

      if (arayType >= 0 && arayType != tempType)
      {
        beNormal = 0;
        vtkErrorMacro(<< "Volume fraction arrays inconsistent in the "
                      << "data type" << endl);
        break;
      }
      cellAray = NULL;
    }
    arayName = NULL;
  }

  if (beNormal && this->Internal->VolumeDataAttributeNames.empty())
  {
    for (i = 0; i < numArays; i++)
    {
      if (strcmp(aryNames[i], vtkDataSetAttributes::GhostArrayName()))
      {
        // note that the ghost array is a hidden data array
        this->Internal->VolumeDataAttributeNames.push_back(aryNames[i]);

        if (!strstr(aryNames[i], "raction") && !this->IsVolumeFractionArray(aryNames[i]))
        {
          this->Internal->IntegrableAttributeNames.push_back(aryNames[i]);
        }
      }
    }
  }

  for (i = 0; i < numArays; i++)
  {
    aryNames[i] = NULL;
  }
  delete[] aryNames;
  aryNames = NULL;

  return beNormal;
}

//-----------------------------------------------------------------------------
int vtkRectilinearGridConnectivity::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // check the number of volume fraction arrays
  if (this->GetNumberOfVolumeFractionArrays() == 0)
  {
    vtkWarningMacro(<< "At least one volume fraction array expected for "
                    << "extracting fragments." << endl);
    return 0;
  }

  // check the output
  vtkInformation* outInfor = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* outputMB =
    vtkMultiBlockDataSet::SafeDownCast(outInfor->Get(vtkDataObject::DATA_OBJECT()));
  if (!outputMB)
  {
    outInfor = NULL;
    vtkErrorMacro(<< "Output vtkMultiBlockDataSet NULL." << endl);
    return 0;
  }

  // check the input
  int inputIdx = 0;
  int numBlcks = 0;
  vtkRectilinearGrid** recGrids = NULL;
  vtkInformation* inputInf = inputVector[0]->GetInformationObject(0);
  vtkDataObject* pDataObj = inputInf->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdsInput = vtkCompositeDataSet::SafeDownCast(pDataObj);
  vtkRectilinearGrid* recInput = vtkRectilinearGrid::SafeDownCast(pDataObj);
  vtkCompositeDataIterator* cdIterat = NULL;

  if (recInput)
  {
    // a vtkRectilinearGrid dataset
    numBlcks = 1;
    recGrids = new vtkRectilinearGrid*[1];
    recGrids[0] = recInput;
  }
  else if (cdsInput)
  {
    // a vtkComposisteDataset that may contain some vtkRectilinearGrid blocks

    // obtain the number of vtkRectilinearGrid blocks
    numBlcks = 0;
    cdIterat = cdsInput->NewIterator();
    cdIterat->GoToFirstItem();
    while (!cdIterat->IsDoneWithTraversal())
    {
      if (vtkRectilinearGrid::SafeDownCast(cdIterat->GetCurrentDataObject()))
      {
        numBlcks++;
      }
      cdIterat->GoToNextItem();
    }

    // allocate an array to store these vtkRectilinearGrid blocks
    recGrids = new vtkRectilinearGrid*[numBlcks];
    cdIterat->GoToFirstItem();
    while (!cdIterat->IsDoneWithTraversal())
    {
      pDataObj = cdIterat->GetCurrentDataObject();
      recInput = vtkRectilinearGrid::SafeDownCast(pDataObj);
      if (recInput)
      {
        recGrids[inputIdx++] = recInput;
      }
      else if (pDataObj)
      {
        vtkWarningMacro(<< "Filed to handle block of type " << pDataObj->GetClassName()
                        << " --- block skipped." << endl);
      }

      cdIterat->GoToNextItem();
    }

    cdIterat->Delete();
    cdIterat = NULL;
  }
  else if (pDataObj)
  {
    // the input dataset is neither vtkCompositeDataSet nor vtkRectilinearGrid
    inputInf = NULL;
    outInfor = NULL;
    pDataObj = NULL;
    cdsInput = NULL;
    recInput = NULL;
    vtkErrorMacro(<< "Failed to handle dataset of type " << pDataObj->GetClassName() << endl);
    return 0;
  }

  // obtain the current time step to determine if the dual grids are ready
  double timeStep = 0.0;
  inputInf = pDataObj->GetInformation();
  if (inputInf && inputInf->Has(vtkDataObject::DATA_TIME_STEP()))
  {
    timeStep = inputInf->Get(vtkDataObject::DATA_TIME_STEP());
  }
  else if (outInfor && outInfor->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    timeStep = outInfor->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  }

  if (timeStep == this->DataBlocksTime)
  {
    this->DualGridsReady = 1;
  }
  else
  {
    this->DualGridsReady = 0;
    this->DataBlocksTime = timeStep;
  }

  inputInf = NULL;
  outInfor = NULL;
  pDataObj = NULL;
  cdsInput = NULL;
  recInput = NULL;

  // create vtkPolyData objects to be attached to the output
  int i;
  int numParts = this->GetNumberOfVolumeFractionArrays();
  vtkPolyData** theParts = new vtkPolyData*[numParts];
  outputMB->SetNumberOfBlocks(numParts);
  for (i = 0; i < numParts; i++)
  {
    theParts[i] = vtkPolyData::New();
    outputMB->SetBlock(i, theParts[i]);
  }
  outputMB = NULL;

  // note that some processes may not be assigned with any block if there are
  // more processes than the blocks and in this case nothing can be  done
  // except for asking the (remote) process to send an empty vtkPolyData to
  // the root process which is always waiting for a fixed number of results
  // from the remote processes
  if (!recGrids || !numBlcks)
  {
    // recGrids might be NULL and numBlcks might be zero in multi-process mode

    if (this->Controller->GetLocalProcessId() &&   // a root process
      this->Controller->GetNumberOfProcesses() > 1 // multi-process
      )
    {
      for (i = 0; i < numParts; i++)
      {
        this->Controller->Send(theParts[i], 0, 890831 + i);
      }
    }

    for (i = 0; i < numParts; i++)
    {
      theParts[i]->Delete();
      theParts[i] = NULL;
    }
    delete[] theParts;
    theParts = NULL;

    return 1;
  }

  // If the time step has been updated (new data blocks have been loaded, which
  // invalidates the existing dual grids), we need to check the volume arrays,
  // update the dual grids, and obtain the bounding box.
  if (!this->DualGridsReady)
  {
    // verify the consistent volume data arrays contained in all blocks and
    // obtain the data type of the volume fraction arrays

    if (this->CheckVolumeDataArrays(recGrids, numBlcks) == 0)
    {
      for (inputIdx = 0; inputIdx < numBlcks; inputIdx++)
      {
        recGrids[inputIdx] = NULL;
      }
      delete[] recGrids;
      recGrids = NULL;

      for (i = 0; i < numParts; i++)
      {
        theParts[i]->Delete();
        theParts[i] = NULL;
      }
      delete[] theParts;
      theParts = NULL;

      vtkErrorMacro(<< "Error with volume data arrays --- Fragments extraction "
                    << "cancelled." << endl);
      return 0;
    }

    // update the dual-grid block(s) and obtain the bounding box, if necessary

    // destroy the obsolete dual grid block(s)
    if (this->DualGridBlocks && this->NumberOfBlocks)
    {
      for (i = 0; i < this->NumberOfBlocks; i++)
      {
        this->DualGridBlocks[i]->Delete();
        this->DualGridBlocks[i] = NULL;
      }
      delete[] this->DualGridBlocks;
      this->DualGridBlocks = NULL;
    }

    // allocate a new array of dual grids
    this->DualGridsReady = 1;
    this->NumberOfBlocks = numBlcks;
    this->DualGridBlocks = new vtkRectilinearGrid*[numBlcks];

    // clear the bounding box
    double* rcBounds = NULL;
    this->DualGridBounds[0] = this->DualGridBounds[2] = this->DualGridBounds[4] = VTK_DOUBLE_MAX;
    this->DualGridBounds[1] = this->DualGridBounds[3] = this->DualGridBounds[5] = VTK_DOUBLE_MIN;

    // compute the bounidng box and create the dual grids
    for (i = 0; i < numBlcks; i++)
    {
      rcBounds = recGrids[i]->GetBounds();
      this->DualGridBounds[0] =
        (rcBounds[0] < this->DualGridBounds[0]) ? rcBounds[0] : this->DualGridBounds[0];
      this->DualGridBounds[2] =
        (rcBounds[2] < this->DualGridBounds[2]) ? rcBounds[2] : this->DualGridBounds[2];
      this->DualGridBounds[4] =
        (rcBounds[4] < this->DualGridBounds[4]) ? rcBounds[4] : this->DualGridBounds[4];
      this->DualGridBounds[1] =
        (rcBounds[1] > this->DualGridBounds[1]) ? rcBounds[1] : this->DualGridBounds[1];
      this->DualGridBounds[3] =
        (rcBounds[3] > this->DualGridBounds[3]) ? rcBounds[3] : this->DualGridBounds[3];
      this->DualGridBounds[5] =
        (rcBounds[5] > this->DualGridBounds[5]) ? rcBounds[5] : this->DualGridBounds[5];
      rcBounds = NULL;

      this->DualGridBlocks[i] = vtkRectilinearGrid::New();
      this->CreateDualRectilinearGrid(recGrids[i], this->DualGridBlocks[i]);
    }
  }

  // deallocate the pointers to the original grid blocks (with cell data)
  for (i = 0; i < numBlcks; i++)
  {
    recGrids[i] = NULL;
  }
  delete[] recGrids;
  recGrids = NULL;

  // extract fragments based on the volume fraction array(s)
  for (i = 0; i < numParts; i++)
  {
    this->ExtractFragments(this->DualGridBlocks, numBlcks, this->DualGridBounds, i, theParts[i]);
    theParts[i]->Delete();
    theParts[i] = NULL;
  }
  delete[] theParts;
  theParts = NULL;

  return 1;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ExtractFragments(vtkRectilinearGrid** dualGrds, int numBlcks,
  double boundBox[6], unsigned char partIndx, vtkPolyData* polyData)
{
  if (!dualGrds || numBlcks <= 0 || !polyData || !this->GetVolumeFractionArrayName(partIndx))
  {
    vtkErrorMacro(<< "Input vtkRectilinearGrid array (dualGrds) or output "
                  << "vtkPolyData (polyData) NULL, invalid number of blocks "
                  << "or invalid volume fraction array name." << endl);
    return;
  }

  int i;
  int* maxFsize = NULL;
  vtkPolyData** surfaces = NULL;
  vtkPolyData* plyHedra = NULL;
  vtkPoints* mbPoints = NULL;
  vtkIncrementalOctreePointLocator* mbPntLoc = NULL;

  mbPoints = vtkPoints::New();
  mbPntLoc = vtkIncrementalOctreePointLocator::New();
  mbPntLoc->SetTolerance(0.0001);
  mbPntLoc->InitPointInsertion(mbPoints, boundBox, 20000);

  // Process each vtkRectilinearGrid dataset and extract individual greater-
  // than-isovalue sub-volumes (polyhedra in the form of vtkPolyData) on a
  // per-cube basis and write the result to the corresponding vtkPolyData.

  maxFsize = new int[numBlcks];
  surfaces = new vtkPolyData*[numBlcks];
  for (i = 0; i < numBlcks; i++)
  {
    plyHedra = vtkPolyData::New();
    surfaces[i] = vtkPolyData::New();

    // perform marching cubes on the dual grid to obtain the greater-than-
    // isovalue polyhedra, of which each 2D polygon is assigned with a global
    // volume Id
    this->ExtractFragmentPolyhedra(dualGrds[i], this->GetVolumeFractionArrayName(partIndx),
      this->VolumeFractionSurfaceValue * this->Internal->VolumeFractionValueScale, plyHedra);

    // # clear and re-init EquivalenceSet
    // # clear and re-init the face hash with the number of points contained
    //   in the polyhedra
    // # add each face of the polyhedra to the face hash, with the block-based
    //   local point Id as the face hash entry / index, assign it with the face
    //   index (in the polyhedra, via PolygonId) for late access to the original
    //   2D polygon in the polyhedra, and assign it with the volume index (in
    //   the polyhedra, via VolumeId)
    // # resolve the polygons of the polyhedra in the face hash
    // # obtain the remaining / exterior faces from the face hash and group them
    //   based on the local (block-based) fragment Id
    // # Given each exterior face extracted from the face hash, gain access to
    //   the original 2D polygon in the polyhedra, insert it to the output
    //   vtkPolyData. The points are also inserted to the output polygon and
    //   a global Id is assigned to each point as the point data attribute
    this->ExtractFragmentPolygons(i, maxFsize[i], plyHedra, surfaces[i], mbPntLoc);

    plyHedra->Delete();
    plyHedra = NULL;
  }

  // The equivalenceSet keeps track of fragment ids and determines which
  // fragment ids need to be combined into a single fragment.
  if (this->EquivalenceSet)
  {
    this->EquivalenceSet->Delete();
    this->EquivalenceSet = NULL;
  }
  this->EquivalenceSet = vtkEquivalenceSet::New();

  // Allocate a vtkDoubleArray to maintain the attributes of each fragment
  if (this->FragmentValues)
  {
    this->FragmentValues->Delete();
    this->FragmentValues = NULL;
  }
  this->FragmentValues = vtkDoubleArray::New();
  this->FragmentValues->SetNumberOfComponents(
    this->Internal->NumberIntegralComponents + 1); // material volume

  this->InitializeFaceHash(surfaces, numBlcks);
  this->AddPolygonsToFaceHash(surfaces, maxFsize, numBlcks);
  this->ResolveEquivalentFragments();
  this->GenerateOutputFromSingleProcess(surfaces, numBlcks, partIndx, polyData);

  // memory deallocation
  mbPntLoc->Delete();
  mbPoints->Delete();
  mbPntLoc = NULL;
  mbPoints = NULL;

  delete[] maxFsize;
  maxFsize = NULL;

  for (i = 0; i < numBlcks; i++)
  {
    surfaces[i]->Delete();
    surfaces[i] = NULL;
  }
  delete[] surfaces;
  surfaces = NULL;

  // So far each process (either a remote process or the root process) has
  // processed the block(s), if any (some processes may not be assigned with
  // any blocks at all), that are assigned to it. Exterior surfaces of the
  // extracted fragments, if any, are stored in 'polyData' (possibly empty).

  // ------------------------------------------------------------ //
  // -------- Let's consinder inter-process issues below -------- //
  // ------------------------------------------------------------ //

  int procIndx = 0;
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs > 1)
  {
    if (this->Controller->GetLocalProcessId() != 0)
    {
      // this is a remote process
      this->Controller->Send(polyData, 0, 890831 + partIndx);
      polyData->Initialize();
    }
    else
    {
      // this is the root process

      // NOTE: Since this is the root process collecting the extraction results
      // from remote processes, argument 'boundBox' (only specific to the root
      // process) is not valid any more for combining these multiple results.
      // we need to compute the global bounding box covering all the extraction
      // results. An invalid bounding box would crash the point locator.

      // allocate an array of vtkPolyData objects (tempPlys for computing the
      // global bounding box, otherwise it would not be allocated as an array)
      maxFsize = new int[numProcs]; // max fragment size
      vtkPolyData** tempPlys = new vtkPolyData*[numProcs];
      vtkPolyData** procPlys = new vtkPolyData*[numProcs];
      for (i = 0; i < numProcs; i++)
      {
        tempPlys[i] = vtkPolyData::New();
        procPlys[i] = vtkPolyData::New();
      }

      // collect the extraction results from the remote processes
      tempPlys[0]->DeepCopy(polyData);
      polyData->Initialize();
      for (procIndx = 1; procIndx < numProcs; procIndx++)
      {
        this->Controller->Receive(tempPlys[procIndx], procIndx, 890831 + partIndx);
      }

      // obtain the global bounding box (note that vtkPolyData objects provided
      // by some processes including the root process might be just empty)
      double* localBox = NULL;
      double globalBB[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
        VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
      for (i = 0; i < numProcs; i++)
      {
        if (tempPlys[i]->GetNumberOfPoints())
        {
          localBox = tempPlys[i]->GetBounds();
          globalBB[0] = (localBox[0] < globalBB[0]) ? localBox[0] : globalBB[0];
          globalBB[2] = (localBox[2] < globalBB[2]) ? localBox[2] : globalBB[2];
          globalBB[4] = (localBox[4] < globalBB[4]) ? localBox[4] : globalBB[4];
          globalBB[1] = (localBox[1] > globalBB[1]) ? localBox[1] : globalBB[1];
          globalBB[3] = (localBox[3] > globalBB[3]) ? localBox[3] : globalBB[3];
          globalBB[5] = (localBox[5] > globalBB[5]) ? localBox[5] : globalBB[5];
          localBox = NULL;
        }
      }

      // create a global point locator used to assign unique point Ids for
      // combining fragments extracted from multiple processes
      mbPoints = vtkPoints::New();
      mbPntLoc = vtkIncrementalOctreePointLocator::New();
      mbPntLoc->SetTolerance(0.0001);
      mbPntLoc->InitPointInsertion(mbPoints, globalBB);

      // generate inter-process vtkPolyData objects for merging fragments
      for (procIndx = 0; procIndx < numProcs; procIndx++)
      {
        this->CreateInterProcessPolygons(
          tempPlys[procIndx], procPlys[procIndx], mbPntLoc, maxFsize[procIndx]);
        tempPlys[procIndx]->Delete();
        tempPlys[procIndx] = NULL;
      }
      delete[] tempPlys;
      tempPlys = NULL;

      // create an equivalence set for removing multiple fragments
      if (this->EquivalenceSet)
      {
        this->EquivalenceSet->Delete();
        this->EquivalenceSet = NULL;
      }
      this->EquivalenceSet = vtkEquivalenceSet::New();

      // allocate a vtkDoubleArray to maintain the attributes of each fragment
      if (this->FragmentValues)
      {
        this->FragmentValues->Delete();
        this->FragmentValues = NULL;
      }
      this->FragmentValues = vtkDoubleArray::New();
      this->FragmentValues->SetNumberOfComponents(
        this->Internal->NumberIntegralComponents + 1); // material volume

      // execute the pipeline of inter-process faces resolution
      this->InitializeFaceHash(procPlys, numProcs);
      this->AddInterProcessPolygonsToFaceHash(procPlys, maxFsize, numProcs);
      this->ResolveEquivalentFragments();
      this->GenerateOutputFromMultiProcesses(procPlys, numProcs, partIndx, polyData);

      // memory deallocation specific to the inter-process module
      mbPntLoc->Delete();
      mbPoints->Delete();
      mbPntLoc = NULL;
      mbPoints = NULL;

      for (i = 0; i < numProcs; i++)
      {
        procPlys[i]->Delete();
        procPlys[i] = NULL;
      }
      delete[] procPlys;
      delete[] maxFsize;
      procPlys = NULL;
      maxFsize = NULL;
    }
  }

  // memory deallocation
  if (this->FaceHash)
  {
    delete this->FaceHash;
    this->FaceHash = NULL;
  }

  if (this->EquivalenceSet)
  {
    this->EquivalenceSet->Delete();
    this->EquivalenceSet = NULL;
  }

  if (this->FragmentValues)
  {
    this->FragmentValues->Delete();
    this->FragmentValues = NULL;
  }
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::CreateDualRectilinearGrid(
  vtkRectilinearGrid* rectGrid, vtkRectilinearGrid* dualGrid)
{
  if (!rectGrid || !dualGrid)
  {
    vtkErrorMacro(<< "Input rectGrid or output dualGrid NULL." << endl);
    return;
  }

  int i, j, k, m, n;
  int numArays;
  int jCellInc;
  int kCellInc;
  int rcShiftJ;
  int rcShiftK;
  int rCellIdx; // Rectilinear CELL InDeX
  int dPntIndx; // Dual grid PoiNT INDeX
  int* numComps;
  int rectDims[3];
  int dualDims[3];
  double theCords[2];
  double tempCord;
  double* xSpacing = NULL;
  double* ySpacing = NULL;
  double* zSpacing = NULL;
  vtkDataArray* rXcoords = NULL;
  vtkDataArray* rYcoords = NULL;
  vtkDataArray* rZcoords = NULL;
  vtkDataArray** rcArrays = NULL;
  vtkDoubleArray* dXcoords = NULL;
  vtkDoubleArray* dYcoords = NULL;
  vtkDoubleArray* dZcoords = NULL;
  vtkDoubleArray** dpArrays = NULL;
  vtkDoubleArray* dVolumes = NULL;

  // get the input grid
  rectGrid->GetDimensions(rectDims);
  rXcoords = rectGrid->GetXCoordinates();
  rYcoords = rectGrid->GetYCoordinates();
  rZcoords = rectGrid->GetZCoordinates();

  // For dual vtkRectilinearGrids, the cells between grid line (gridDim
  // - 3) and grid line (gridDim - 2) of the former half and the cells
  // between grid line 0 and grid line 1 of the latter half are at the
  // ghost level. In other words, there are two grid lines or one row of
  // cells at the ghost level. Without skipping these cells, the polygons
  // resulting from marching cubes over them for the former and the latter
  // would be sent to the face hash twice and then be mistakenly removed
  // as internal faces to leave seams. To address this issue, the size of
  // the dual grid needs to be gridDim - 2.

  // create the output grid
  dualDims[0] = rectDims[0] - 2;
  dualDims[1] = rectDims[1] - 2;
  dualDims[2] = rectDims[2] - 2;
  dXcoords = vtkDoubleArray::New();
  dYcoords = vtkDoubleArray::New();
  dZcoords = vtkDoubleArray::New();
  dVolumes = vtkDoubleArray::New();
  xSpacing = new double[dualDims[0]];
  ySpacing = new double[dualDims[1]];
  zSpacing = new double[dualDims[2]];

  // array of x coordinates
  dXcoords->SetNumberOfComponents(1);
  dXcoords->SetNumberOfTuples(dualDims[0]);
  tempCord = rXcoords->GetComponent(0, 0);
  for (i = 0; i < dualDims[0]; i++)
  {
    theCords[0] = tempCord;
    theCords[1] = tempCord = rXcoords->GetComponent(i + 1, 0);
    xSpacing[i] = theCords[1] - theCords[0];
    dXcoords->SetComponent(i, 0, (theCords[0] + theCords[1]) * 0.5);
  }

  // array of y coordinates
  dYcoords->SetNumberOfComponents(1);
  dYcoords->SetNumberOfTuples(dualDims[1]);
  tempCord = rYcoords->GetComponent(0, 0);
  for (i = 0; i < dualDims[1]; i++)
  {
    theCords[0] = tempCord;
    theCords[1] = tempCord = rYcoords->GetComponent(i + 1, 0);
    ySpacing[i] = theCords[1] - theCords[0];
    dYcoords->SetComponent(i, 0, (theCords[0] + theCords[1]) * 0.5);
  }

  // array of z coordinates
  dZcoords->SetNumberOfComponents(1);
  dZcoords->SetNumberOfTuples(dualDims[2]);
  tempCord = rZcoords->GetComponent(0, 0);
  for (i = 0; i < dualDims[2]; i++)
  {
    theCords[0] = tempCord;
    theCords[1] = tempCord = rZcoords->GetComponent(i + 1, 0);
    zSpacing[i] = theCords[1] - theCords[0];
    dZcoords->SetComponent(i, 0, (theCords[0] + theCords[1]) * 0.5);
  }

  // gain access to the cell data arrays of the original grid and use them to
  // create point data arrays attached to the dual grid
  numArays = rectGrid->GetCellData()->GetNumberOfArrays();
  numComps = new int[numArays];
  rcArrays = new vtkDataArray*[numArays];
  dpArrays = new vtkDoubleArray*[numArays];
  for (i = 0; i < numArays; i++)
  {
    rcArrays[i] = rectGrid->GetCellData()->GetArray(i);
    numComps[i] = rcArrays[i]->GetNumberOfComponents();
    dpArrays[i] = vtkDoubleArray::New();
    dpArrays[i]->SetName(rcArrays[i]->GetName());
    dpArrays[i]->SetNumberOfComponents(numComps[i]);
    dpArrays[i]->SetNumberOfTuples(dualDims[0] * dualDims[1] * dualDims[2]);
  }

  // create an array of geomtric volumes (of the cells of the original grid)
  // as the point data of the dual grid
  dVolumes->SetName("GeometricVolume");
  dVolumes->SetNumberOfComponents(1);
  dVolumes->SetNumberOfTuples(dualDims[0] * dualDims[1] * dualDims[2]);

  dPntIndx = 0;
  rCellIdx = 0;
  rcShiftJ = 0;
  rcShiftK = 0;
  jCellInc = (rectDims[0] - 1);
  kCellInc = (rectDims[0] - 1) * (rectDims[1] - 1);
  for (k = 0, rCellIdx = 0, dPntIndx = 0; k < dualDims[2]; k++, rcShiftK += kCellInc)
    for (j = 0, rcShiftJ = 0; j < dualDims[1]; j++, rcShiftJ += jCellInc)
      for (i = 0; i < dualDims[0]; i++, dPntIndx++)
      {
        rCellIdx = rcShiftK + rcShiftJ + i;
        dVolumes->SetComponent(dPntIndx, 0, xSpacing[i] * ySpacing[j] * zSpacing[k]);

        for (m = 0; m < numArays; m++)
          for (n = 0; n < numComps[m]; n++)
          {
            dpArrays[m]->SetComponent(dPntIndx, n, rcArrays[m]->GetComponent(rCellIdx, n));
          }
      }

  // set the dual grid
  dualGrid->SetDimensions(dualDims);
  dualGrid->SetXCoordinates(dXcoords);
  dualGrid->SetYCoordinates(dYcoords);
  dualGrid->SetZCoordinates(dZcoords);
  dualGrid->GetPointData()->AddArray(dVolumes);
  for (i = 0; i < numArays; i++)
  {
    dualGrid->GetPointData()->AddArray(dpArrays[i]);
    dpArrays[i]->Delete();
    dpArrays[i] = NULL;
    rcArrays[i] = NULL;
  }
  delete[] dpArrays;
  delete[] rcArrays;
  delete[] numComps;
  dpArrays = NULL;
  rcArrays = NULL;
  numComps = NULL;

  // memory de-allocation
  dXcoords->Delete();
  dYcoords->Delete();
  dZcoords->Delete();
  dVolumes->Delete();
  delete[] xSpacing;
  delete[] ySpacing;
  delete[] zSpacing;
  dXcoords = NULL;
  dYcoords = NULL;
  dZcoords = NULL;
  dVolumes = NULL;
  xSpacing = NULL;
  ySpacing = NULL;
  zSpacing = NULL;
  rXcoords = NULL;
  rYcoords = NULL;
  rZcoords = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ExtractFragmentPolyhedra(
  vtkRectilinearGrid* rectGrid, const char* fracName, double isoValue, vtkPolyData* plyHedra)
{
  if (!rectGrid || !plyHedra || !this->Internal->IntegrablePointDataArraysAvailable(rectGrid) ||
    vtkDoubleArray::SafeDownCast(rectGrid->GetPointData()->GetArray(fracName)) == NULL ||
    vtkDoubleArray::SafeDownCast(rectGrid->GetPointData()->GetArray("GeometricVolume")) == NULL)
  {
    vtkErrorMacro(<< "Input vtkRectilinearGrid, point data GeometricVolume, "
                  << "integrable point data arrays, or output vtkPolyData "
                  << "NULL." << endl);
    return;
  }

  // IIP: Interpolated Iso-value Point --- the on-edge iso-value point obtained
  //      via interpolation. Each IIP is indicated by the associated edge index
  //      (0 ~ 11) in the LUT. In contrast, each vertex of the cube is referred
  //      to, in the LUT, by an index (12 ~ 19) that is a translated version of
  //      the original index (0 ~ 7, in the cube).

  int i, j, k, m, n, a, c;
  int numArays;
  int numCells;    // number of polygons of an MC sub-volume
  int vtxIndex;    // vertices referenced by an MC sub-volume
  int tmpPtIds[3]; // three point-Ids of a quad
  int valuAdrs[4]; // address of a value in an array
  int dataDims[3];
  int numbHits[8]; // number of hits of a vertex on a sub-volume
  int vtxIndxs[2];
  int pntReady[20]; // point coordinates ready?
  int lutPtIdx = 0;
  int nPlyPnts = 0; // number of points forming a polygon
  int estiSize = 0;
  int caseIndx = 0;
  int sliceSiz = 0;
  int pntKindx = 0;
  int pntJindx = 0;
  int pntIndex = 0;
  int volIndex = 0;
  int* numComps = NULL; // number of components
  int* volPtIds = NULL;
  double* volFracs = NULL;
  double* geomVols = NULL;   // geometric volumes of the original cells
  double** ptValues = NULL;  // arrays / pointer
  double*** lastVals = NULL; // arrays / components / 8 nodes
  double*** nodeVals = NULL; // arrays / components / 8 nodes
  double*** acumVals = NULL; // accumulated values a vertex scatters
  double** integVal = NULL;  // integrated attributes of a sub-volume
  double integVol = 0.0;     // integrated attribute of a sub-volume
  double interplt = 0.0;     // fraction value used for interpolation
  double dataBbox[6];
  double acumVols[8]; // accumulated volume a vertex scatters
  double lastFrcs[8]; // to reuse quad scalars (1, 2, 5, 6 only)
  double nodeFrcs[8]; // fractions of the ORIGINAL hexas (now nodes)
  double lastVols[8]; // to reuse --- similar to lastFrcs
  double nodeVols[8]; // volumes of the ORIGINAL hexas (now nodes)
  double lastCord[3];
  double vtxCords[8][3];  // VerTeX (0 ~ 7)
  double pntCords[20][3]; // IIPs (0 ~ 11) and vertices (12 ~ 19)
  vtkIdType cellIdxs[12]; // polygon-/face-indices of a sub-volume
  vtkIdType plyPtIds[5];  // PointT IDs of a PoLYgon
  vtkIdType cellIndx = 0;
  vtkPoints* surfPnts = NULL;
  vtkDataArray* pXcoords = NULL;
  vtkDataArray* pYcoords = NULL;
  vtkDataArray* pZcoords = NULL;
  vtkCellArray* surfaces = NULL;
  vtkIdTypeArray* uniVIdxs = NULL;
  vtkDoubleArray* mVolumes = NULL;  // material volumes: SIGMA(fraction * volume)
  vtkDoubleArray** volArays = NULL; // arrays / pointer
  vtkIncrementalOctreePointLocator* pntAdder = NULL;

  static int EDGEVTXS[12][2] = { { 0, 1 }, { 1, 2 }, { 3, 2 }, { 0, 3 }, { 4, 5 }, { 5, 6 },
    { 7, 6 }, { 4, 7 }, { 0, 4 }, { 1, 5 }, { 3, 7 },
    { 2, 6 } }; // two VerTeXS (vertices) of an EDGE

  static int CASEMASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  vtkRectilinearGridConnectivityMarchingCubesVolumeCases* volCases =
    vtkRectilinearGridConnectivityMarchingCubesVolumeCases::GetCases();

  // gain access to arrays of the 3D coordinates and point data arrays (material
  // volume fraction, non-fraction attributes, and geometric volume) and create
  // a set of vtkDoubleArray objects for integration of all data attributes, of
  // which each might have multiple components
  pXcoords = rectGrid->GetXCoordinates();
  pYcoords = rectGrid->GetYCoordinates();
  pZcoords = rectGrid->GetZCoordinates();
  volFracs =
    vtkDoubleArray::SafeDownCast(rectGrid->GetPointData()->GetArray(fracName))->GetPointer(0);
  geomVols = vtkDoubleArray::SafeDownCast(rectGrid->GetPointData()->GetArray("GeometricVolume"))
               ->GetPointer(0);

  numArays = int(this->Internal->IntegrableAttributeNames.size());
  numComps = new int[numArays];
  ptValues = new double*[numArays];
  integVal = new double*[numArays];
  acumVals = new double**[numArays];
  lastVals = new double**[numArays];
  nodeVals = new double**[numArays];
  for (a = 0; a < numArays; a++)
  {
    vtkDoubleArray* tempAray = vtkDoubleArray::SafeDownCast(
      rectGrid->GetPointData()->GetArray(this->Internal->IntegrableAttributeNames[a].c_str()));
    ptValues[a] = tempAray->GetPointer(0);
    numComps[a] = tempAray->GetNumberOfComponents();
    integVal[a] = new double[numComps[a]];
    acumVals[a] = new double*[numComps[a]];
    lastVals[a] = new double*[numComps[a]];
    nodeVals[a] = new double*[numComps[a]];
    for (c = 0; c < numComps[a]; c++)
    {
      acumVals[a][c] = new double[8];
      lastVals[a][c] = new double[8];
      nodeVals[a][c] = new double[8];
    }
    tempAray = NULL;
  }
  if (this->Internal->ComponentNumbersObtained == 0)
  {
    this->Internal->ComponentNumbersObtained = 1;
    this->Internal->NumberIntegralComponents = 0;
    for (a = 0; a < numArays; a++)
    {
      this->Internal->NumberIntegralComponents += numComps[a];
      this->Internal->ComponentNumbersPerArray.push_back(numComps[a]);
    }
  }

  // create a vtkPoints for all the points of the fragment surfaces
  rectGrid->GetBounds(dataBbox);
  rectGrid->GetDimensions(dataDims);
  sliceSiz = dataDims[0] * dataDims[1];
  estiSize = sliceSiz * dataDims[2];
  estiSize = estiSize / 1024 * 1024;
  estiSize = (estiSize < 1024) ? 1024 : estiSize;
  surfPnts = vtkPoints::New();
  surfPnts->Allocate(estiSize, estiSize >> 1);

  // create a vtkIncrementalOctreePointLocator and attach it to the vtkPoints
  // such that the point locator will reject duplicates as points are inserted
  // to the vtkPoints. From now on the point locator serves as a proxy of the
  // vtkPoints to collect 3D points.
  pntAdder = vtkIncrementalOctreePointLocator::New();
  pntAdder->SetTolerance(0.0001);
  pntAdder->InitPointInsertion(surfPnts, dataBbox, estiSize);

  // create a vtkCellArray for the surfaces of greater-than-isovalue sub-volumes
  surfaces = vtkCellArray::New();
  surfaces->Allocate(estiSize, estiSize >> 1);

  // Create a vtkIdTypeArray for the global volume Ids assigned to the surfaces.
  // In fact the volume Ids might not necessarily be global since their ultimate
  // goal is to allow AddPolygonsToFaceHash() to determine which polygons / faces
  // form a volume.
  uniVIdxs = vtkIdTypeArray::New();
  uniVIdxs->SetName("VolumeId");
  uniVIdxs->Allocate(estiSize, estiSize >> 1);

  // create a vtkDoubleArray of material volumes for the surfaces
  mVolumes = vtkDoubleArray::New();
  mVolumes->SetName("MaterialVolume");
  mVolumes->Allocate(estiSize, estiSize >> 1);

  // create vtkDoubleArray objects to integrate non-fraction volume arrays and
  volArays = new vtkDoubleArray*[numArays];
  for (a = 0; a < numArays; a++)
  {
    volArays[a] = vtkDoubleArray::New();
    volArays[a]->SetName(this->Internal->IntegrableAttributeNames[a].c_str());
    volArays[a]->SetNumberOfComponents(numComps[a]);
    volArays[a]->Allocate(estiSize, estiSize >> 1);
  }

  // marching cubes to create surfaces for the greater-than-isovalue sub-volumes
  pntKindx = -sliceSiz;
  lastCord[2] = pZcoords->GetComponent(0, 0); // for reusing z-coordinate
  for (k = 0; k < dataDims[2] - 1; k++)
  {
    pntKindx += sliceSiz;
    vtxCords[0][2] = lastCord[2];
    vtxCords[6][2] = lastCord[2] = pZcoords->GetComponent(k + 1, 0);

    pntJindx = -dataDims[0];
    lastCord[1] = pYcoords->GetComponent(0, 0); // for reusing y-coordinate
    for (j = 0; j < dataDims[1] - 1; j++)
    {
      pntJindx += dataDims[0];
      vtxCords[0][1] = lastCord[1];
      vtxCords[6][1] = lastCord[1] = pYcoords->GetComponent(j + 1, 0);

      lastCord[0] = pXcoords->GetComponent(0, 0); // for reusing x-coordinate

      // The attribute values at the vertices of the beginning quad on the
      // current row are obtained here in support of reusing them on a per quad
      // basis while marching cubes along a row. This beginning quad is taken as
      // the right quad of the "previous" cube on the row. Note only #1, #2, #5,
      // and #6 are used while 8 units are allocated for easy access purposes.
      pntIndex = pntKindx + pntJindx + 0; // i = 0: the starting quad
      tmpPtIds[0] = pntIndex + dataDims[0];
      tmpPtIds[1] = pntIndex + sliceSiz;
      tmpPtIds[2] = pntIndex + sliceSiz + dataDims[0];

      lastFrcs[1] = volFracs[pntIndex];
      lastFrcs[2] = volFracs[tmpPtIds[0]];
      lastFrcs[5] = volFracs[tmpPtIds[1]];
      lastFrcs[6] = volFracs[tmpPtIds[2]];

      lastVols[1] = geomVols[pntIndex];
      lastVols[2] = geomVols[tmpPtIds[0]];
      lastVols[5] = geomVols[tmpPtIds[1]];
      lastVols[6] = geomVols[tmpPtIds[2]];

      for (a = 0; a < numArays; a++)
      {
        valuAdrs[0] = numComps[a] * pntIndex;
        valuAdrs[1] = numComps[a] * tmpPtIds[0];
        valuAdrs[2] = numComps[a] * tmpPtIds[1];
        valuAdrs[3] = numComps[a] * tmpPtIds[2];
        for (c = 0; c < numComps[a]; c++)
        {
          lastVals[a][c][1] = ptValues[a][valuAdrs[0] + c];
          lastVals[a][c][2] = ptValues[a][valuAdrs[1] + c];
          lastVals[a][c][5] = ptValues[a][valuAdrs[2] + c];
          lastVals[a][c][6] = ptValues[a][valuAdrs[3] + c];
        }
      }

      for (i = 0; i < dataDims[0] - 1; i++)
      {
        // obtain the attribute values at the cube's eight vertices

        // obtain the left quad of the current cube by reusing
        // the right quad of the previous cube on the this row
        nodeFrcs[0] = lastFrcs[1];
        nodeFrcs[3] = lastFrcs[2];
        nodeFrcs[4] = lastFrcs[5];
        nodeFrcs[7] = lastFrcs[6];

        nodeVols[0] = lastVols[1];
        nodeVols[3] = lastVols[2];
        nodeVols[4] = lastVols[5];
        nodeVols[7] = lastVols[6];

        for (a = 0; a < numArays; a++)
          for (c = 0; c < numComps[a]; c++)
          {
            nodeVals[a][c][0] = lastVals[a][c][1];
            nodeVals[a][c][3] = lastVals[a][c][2];
            nodeVals[a][c][4] = lastVals[a][c][5];
            nodeVals[a][c][7] = lastVals[a][c][6];
          }

        // gain access to the right quad (in x axis) of the current cube
        pntIndex = pntKindx + pntJindx + i + 1; // 1: the right quad
        tmpPtIds[0] = pntIndex + dataDims[0];
        tmpPtIds[1] = pntIndex + sliceSiz;
        tmpPtIds[2] = pntIndex + sliceSiz + dataDims[0];

        // obtain the right quad of the current cube by accessing the data
        // array and update the buffer of the right quad for the next cube
        nodeFrcs[1] = lastFrcs[1] = volFracs[pntIndex];
        nodeFrcs[2] = lastFrcs[2] = volFracs[tmpPtIds[0]];
        nodeFrcs[5] = lastFrcs[5] = volFracs[tmpPtIds[1]];
        nodeFrcs[6] = lastFrcs[6] = volFracs[tmpPtIds[2]];

        nodeVols[1] = lastVols[1] = geomVols[pntIndex];
        nodeVols[2] = lastVols[2] = geomVols[tmpPtIds[0]];
        nodeVols[5] = lastVols[5] = geomVols[tmpPtIds[1]];
        nodeVols[6] = lastVols[6] = geomVols[tmpPtIds[2]];

        for (a = 0; a < numArays; a++)
        {
          valuAdrs[0] = numComps[a] * pntIndex;
          valuAdrs[1] = numComps[a] * tmpPtIds[0];
          valuAdrs[2] = numComps[a] * tmpPtIds[1];
          valuAdrs[3] = numComps[a] * tmpPtIds[2];
          for (c = 0; c < numComps[a]; c++)
          {
            nodeVals[a][c][1] = lastVals[a][c][1] = ptValues[a][valuAdrs[0] + c];
            nodeVals[a][c][2] = lastVals[a][c][2] = ptValues[a][valuAdrs[1] + c];
            nodeVals[a][c][5] = lastVals[a][c][5] = ptValues[a][valuAdrs[2] + c];
            nodeVals[a][c][6] = lastVals[a][c][6] = ptValues[a][valuAdrs[3] + c];
          }
        }

        // update the x-coordinates of #0 (the near) and #6 (the far)
        // NOTE: the following two lines MUST be above the 'continue' switch
        // as they are used to transfer point coordinates for reuse purposes
        // (the transfer must not be interrupted even if the cube is skipped)
        vtxCords[0][0] = lastCord[0];
        vtxCords[6][0] = lastCord[0] = pXcoords->GetComponent(i + 1, 0);

        // determine the case index
        for (caseIndx = 0, m = 0; m < 8; m++)
        {
          if (nodeFrcs[m] >= isoValue)
          {
            caseIndx |= CASEMASK[m];
          }
        }

        // early exit unless there is any greater-than-isovalue sub-volume
        // OR this is a ghost-level cell
        if (caseIndx == 0)
        {
          continue;
        }

        // get the 3D coordinates of the six vertices
        // #0 (the near) and #6 (the far) have been assigned above
        vtxCords[1][0] = vtxCords[6][0];
        vtxCords[1][1] = vtxCords[0][1];
        vtxCords[1][2] = vtxCords[0][2];

        vtxCords[2][0] = vtxCords[6][0];
        vtxCords[2][1] = vtxCords[6][1];
        vtxCords[2][2] = vtxCords[0][2];

        vtxCords[3][0] = vtxCords[0][0];
        vtxCords[3][1] = vtxCords[6][1];
        vtxCords[3][2] = vtxCords[0][2];

        vtxCords[4][0] = vtxCords[0][0];
        vtxCords[4][1] = vtxCords[0][1];
        vtxCords[4][2] = vtxCords[6][2];

        vtxCords[5][0] = vtxCords[6][0];
        vtxCords[5][1] = vtxCords[0][1];
        vtxCords[5][2] = vtxCords[6][2];

        vtxCords[7][0] = vtxCords[0][0];
        vtxCords[7][1] = vtxCords[6][1];
        vtxCords[7][2] = vtxCords[6][2];

        // todo: add code here to compute normals / gradients

        // Fill pntCords[12] ~ pntCords[19] with vtxCords and set their flags.
        // Note that we store the IIPs and vertices in a single array of 3D
        // coordinates to avoid intense if-statements. The array begins with
        // 12 IIPs (0 ~ 11), followed by 8 vertices (12 ~ 19).
        for (m = 0; m < 8; m++)
        {
          pntReady[m + 12] = 1;
          pntCords[m + 12][0] = vtxCords[m][0];
          pntCords[m + 12][1] = vtxCords[m][1];
          pntCords[m + 12][2] = vtxCords[m][2];

          // clear the hit counters and value-scattering buckets
          numbHits[m] = 0;
          acumVols[m] = 0.0;
          for (a = 0; a < numArays; a++)
            for (c = 0; c < numComps[a]; c++)
            {
              acumVals[a][c][m] = 0.0;
            }
        }

        // Clear the IIP flags --- what we really care about via pntReady.
        // This means that the coordinates of the to-be-referenced IIPs
        // need to be computed when they are first referenced.
        for (m = 0; m < 12; m++)
        {
          pntReady[m] = 0;
        }

        // gain access to the target LUT entry
        volPtIds = (volCases + caseIndx)->PointIds;

        // process each ploygon (either an iso-triangle or a cube face)
        // that is described in this LUT entry
        m = 0;
        numCells = 0;             // clear the number of polygons of this sub-volume
        while (volPtIds[m] != -2) // flag -1 never comes to this line
        {
          // get the number of points forming a polygon (<= 5)
          nPlyPnts = volPtIds[m++];

          // access each point (either an IIP or a vertex) of the polygon
          for (n = 0; n < nPlyPnts; n++)
          {
            // get the internal (LUT-based) index of this point
            lutPtIdx = volPtIds[m++];

            // Obtain the coordinates of an IIP if it is still unavailable.
            // Note that only an IIP's coordinates might be unknown since
            // those of the 8 vertices have been determined above as their
            // flags indicate.
            if (pntReady[lutPtIdx] == 0)
            {
              // now lutPtIdx is guaranteed to fall within [0, 11]

              // obtain the iso-value point coordinates via interpolation
              pntReady[lutPtIdx] = 1;
              vtxIndxs[0] = EDGEVTXS[lutPtIdx][0];
              vtxIndxs[1] = EDGEVTXS[lutPtIdx][1];
              interplt = (isoValue - nodeFrcs[vtxIndxs[0]]) /
                (nodeFrcs[vtxIndxs[1]] - nodeFrcs[vtxIndxs[0]]);

              pntCords[lutPtIdx][0] = vtxCords[vtxIndxs[0]][0] +
                interplt * (vtxCords[vtxIndxs[1]][0] - vtxCords[vtxIndxs[0]][0]);
              pntCords[lutPtIdx][1] = vtxCords[vtxIndxs[0]][1] +
                interplt * (vtxCords[vtxIndxs[1]][1] - vtxCords[vtxIndxs[0]][1]);
              pntCords[lutPtIdx][2] = vtxCords[vtxIndxs[0]][2] +
                interplt * (vtxCords[vtxIndxs[1]][2] - vtxCords[vtxIndxs[0]][2]);
            }

            // let the vertex scatter the attribute values to the sub-volume
            if (lutPtIdx >= 12)
            {
              vtxIndex = lutPtIdx - 12;
              numbHits[vtxIndex]++;
              double theVolum = nodeFrcs[vtxIndex] * nodeVols[vtxIndex];
              acumVols[vtxIndex] += theVolum;
              for (a = 0; a < numArays; a++)
                for (c = 0; c < numComps[a]; c++)
                {
                  acumVals[a][c][vtxIndex] += nodeVals[a][c][vtxIndex] * theVolum;
                }
            }

            // If possible, insert this point to the vtkPoints and assign it
            // with a global Id as the point data attribute.
            pntAdder->InsertUniquePoint(pntCords[lutPtIdx], plyPtIds[n]);
          } // end of accessing each point of the polygon

          // Now that the hybrid points (IIPs and vertices, forming a polygon)
          // have been inserted to the vtkPoints, let's insert the polygon.
          // Even though this may be a degenerate polygon, we still need to
          // keep it, which will be then sent to the face hash for combining
          // sub-volumes to create a single fragment. Rejection of degenerate
          // polygons may lead to wrong fragment extraction as they separate
          // two sub-volumes, preventing them from being combined together.
          cellIndx = surfaces->InsertNextCell(nPlyPnts, plyPtIds);

          // attach the volume Id as a cell data value to this polygon
          uniVIdxs->InsertValue(cellIndx, volIndex);

          // record the cell index for deferred subvolume-dependent attribute
          // integration, of which the result will be assigned to such a cell
          cellIdxs[numCells++] = cellIndx;

          // handle flag -1 (to proceed with a new volume)
          if (volPtIds[m] == -1)
          {
            m++;
            volIndex++;

            // initialize the attribute integration results
            integVol = 0.0;
            for (a = 0; a < numArays; a++)
              for (c = 0; c < numComps[a]; c++)
              {
                integVal[a][c] = 0.0;
              }

            // collect the attribute values from the buckets
            for (n = 0; n < 8; n++)
            {
              if (numbHits[n])
              {
                double hitsNumb = 1.0 / numbHits[n];
                integVol += acumVols[n] * hitsNumb;
                for (a = 0; a < numArays; a++)
                  for (c = 0; c < numComps[a]; c++)
                  {
                    integVal[a][c] += acumVals[a][c][n] * hitsNumb;
                  }
              }

              // clear the hit counters and value-scattering buckets for the
              // next sub-volume
              numbHits[n] = 0;
              acumVols[n] = 0.0;
              for (a = 0; a < numArays; a++)
                for (c = 0; c < numComps[a]; c++)
                {
                  acumVals[a][c][n] = 0.0;
                }
            }

            // normalize the sum (one grid point is shared by 8 cells -- 0.125)
            integVol *= 0.125;
            for (a = 0; a < numArays; a++)
              for (c = 0; c < numComps[a]; c++)
              {
                integVal[a][c] *= 0.125;
              }

            // assign the integration values to each surface of the sub-volume
            for (n = 0; n < numCells; n++)
            {
              mVolumes->InsertValue(cellIdxs[n], integVol);
              for (a = 0; a < numArays; a++)
              {
                volArays[a]->InsertTypedTuple(cellIdxs[n], integVal[a]);
              }
            }

            // clear the number of polygons for the next sub-volume
            numCells = 0;
          }
        } // end while ( volPtIds[m] != -2 )

        // flag -2 (the LUT entry end) means that we have just got a new volume
        // please each LUT entry (except for entry #0: no any sub-volume is
        // extracted) should have a sub-volume extracted.
        volIndex++;

        // initialize the attribute integration results
        integVol = 0.0;
        for (a = 0; a < numArays; a++)
          for (c = 0; c < numComps[a]; c++)
          {
            integVal[a][c] = 0.0;
          }

        // collect the attribute values from the buckets
        for (n = 0; n < 8; n++)
        {
          if (numbHits[n])
          {
            double hitsNumb = 1.0 / numbHits[n];
            integVol += acumVols[n] * hitsNumb;
            for (a = 0; a < numArays; a++)
              for (c = 0; c < numComps[a]; c++)
              {
                integVal[a][c] += acumVals[a][c][n] * hitsNumb;
              }
          }
        }

        // normalize the sum (one grid point is shared by 8 cells -- 0.125)
        integVol *= 0.125;
        for (a = 0; a < numArays; a++)
          for (c = 0; c < numComps[a]; c++)
          {
            integVal[a][c] *= 0.125;
          }

        // assign the integration values to each surface of the sub-volume
        for (n = 0; n < numCells; n++)
        {
          mVolumes->InsertValue(cellIdxs[n], integVol);
          for (a = 0; a < numArays; a++)
          {
            volArays[a]->InsertTypedTuple(cellIdxs[n], integVal[a]);
          }
        }

      } // for each i
    }   // for each j
  }     // for each k

  // fill the output vtkPolyData
  plyHedra->SetPoints(surfPnts);
  plyHedra->SetPolys(surfaces);
  plyHedra->GetCellData()->SetGlobalIds(uniVIdxs);
  plyHedra->GetCellData()->AddArray(mVolumes);
  for (a = 0; a < numArays; a++)
  {
    plyHedra->GetCellData()->AddArray(volArays[a]);
  }
  plyHedra->Squeeze();

  // memory de-allocation
  for (a = 0; a < numArays; a++)
  {
    for (c = 0; c < numComps[a]; c++)
    {
      delete[] lastVals[a][c];
      delete[] nodeVals[a][c];
      delete[] acumVals[a][c];
      lastVals[a][c] = NULL;
      nodeVals[a][c] = NULL;
      acumVals[a][c] = NULL;
    }
    delete[] lastVals[a];
    delete[] nodeVals[a];
    delete[] acumVals[a];
    delete[] integVal[a];
    volArays[a]->Delete();
    lastVals[a] = NULL;
    nodeVals[a] = NULL;
    acumVals[a] = NULL;
    integVal[a] = NULL;
    volArays[a] = NULL;
    ptValues[a] = NULL;
  }
  delete[] numComps;
  delete[] lastVals;
  delete[] nodeVals;
  delete[] acumVals;
  delete[] integVal;
  delete[] volArays;
  delete[] ptValues;
  numComps = NULL;
  lastVals = NULL;
  nodeVals = NULL;
  acumVals = NULL;
  integVal = NULL;
  volArays = NULL;
  ptValues = NULL;

  surfPnts->Delete();
  surfaces->Delete();
  pntAdder->Delete();
  uniVIdxs->Delete();
  mVolumes->Delete();
  surfPnts = NULL;
  surfaces = NULL;
  pntAdder = NULL;
  uniVIdxs = NULL;
  mVolumes = NULL;

  volPtIds = NULL;
  volFracs = NULL;
  geomVols = NULL;
  pXcoords = NULL;
  pYcoords = NULL;
  pZcoords = NULL;
  volCases = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::InitializeFaceHash(vtkPolyData* plyHedra)
{
  if (!plyHedra)
  {
    vtkErrorMacro(<< "vtkPolyData NULL." << endl);
    return;
  }

  if (this->FaceHash)
  {
    delete this->FaceHash;
    this->FaceHash = NULL;
  }

  int hashSize = plyHedra->GetPoints()->GetNumberOfPoints();
  hashSize = (hashSize < 1) ? 1 : hashSize;

  this->FaceHash = new vtkRectilinearGridConnectivityFaceHash;
  this->FaceHash->Initialize(hashSize);
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddPolygonsToFaceHash(int blockIdx, vtkPolyData* plyHedra)
{
  // process the vtkPolyData and add its 2D polygons (faces) to the hash

  // make sure the vtkPolyData (set of polyhedra) contains cell data attributes
  // global volume Ids and material volume
  if (plyHedra == NULL || !this->Internal->IntegrableCellDataArraysAvailable(plyHedra) ||
    vtkIdTypeArray::SafeDownCast(plyHedra->GetCellData()->GetArray("VolumeId")) == NULL ||
    vtkDoubleArray::SafeDownCast(plyHedra->GetCellData()->GetArray("MaterialVolume")) == NULL)
  {
    vtkErrorMacro(<< "Input vtkPolyData (plyHedra), cell data VolumeId "
                  << "or MaterialVolume NULL, or integrable cell data "
                  << "attributes not found from the input." << endl);
    return;
  }

  int i, a, c;
  int procIndx = this->Controller->GetLocalProcessId();
  int theShift = 0;
  int bufIndex = 0;
  int numArays = 0;
  int tupleSiz = 0;        // number of integrated components
  int newIndex = 0;        // index of a new face
  int fragIndx = 1;        // next fragment Id and 0 for removing faces
  int minIndex = 1;        // the smallest fragment Id so far
  int* numComps = NULL;    // number of integrated components
  double* tupleBuf = NULL; // integrated component values
  double** attrPtrs = NULL;
  vtkCell* thisFace = NULL;   // a 2D polygon (instead of a 3D cell)
  vtkIdType numFaces = 0;     // number of 2D polygons in a vtkPolyData
  vtkIdType volIndex = 0;     // global volume Id attached to a 2D polygon
  vtkIdType* vIdxsPtr = NULL; // array of global volume Ids
  vtkDoubleArray* theArray = NULL;
  vtkRectilinearGridConnectivityFace* hashFace = NULL; // a face in the hash
  vtkRectilinearGridConnectivityFace* newFaces[VTK_MAX_FACES_PER_CELL];
  for (i = 0; i < VTK_MAX_FACES_PER_CELL; i++)
    newFaces[i] = NULL;

  // determine the number of integrated components (including the material
  // volume) to be saved to the global fragment attributes array and
  // allocate a buffer for a tuple
  tupleSiz = this->Internal->NumberIntegralComponents + 1;
  tupleBuf = new double[tupleSiz];
  memset(tupleBuf, 0, sizeof(double) * tupleSiz);

  // gain access to the arrays of integrated attributes
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  numComps = new int[numArays];
  attrPtrs = new double*[numArays];
  numComps[0] = 1;
  attrPtrs[0] = vtkDoubleArray::SafeDownCast(plyHedra->GetCellData()->GetArray("MaterialVolume"))
                  ->GetPointer(0);
  for (a = 1; a < numArays; a++)
  {
    theArray = vtkDoubleArray::SafeDownCast(
      plyHedra->GetCellData()->GetArray(this->Internal->IntegrableAttributeNames[a - 1].c_str()));
    attrPtrs[a] = theArray->GetPointer(0);
    numComps[a] = theArray->GetNumberOfComponents();
    theArray = NULL;
  }

  // the vtkPolyData stores separated 2D polygons (triangles, quadrilaterlas,
  // and pentagons, of which each is though coupled with a global volume Id as
  // the cell data attribute to convey the connectivity of the polygons of the
  // same volume) to represent the surface of each greater-than-isovalue sub-
  // volume.
  vIdxsPtr =
    vtkIdTypeArray::SafeDownCast(plyHedra->GetCellData()->GetArray("VolumeId"))->GetPointer(0);

  i = 0;
  volIndex = -1;
  numFaces = plyHedra->GetNumberOfCells();
  while (i < numFaces) // for each separated 2D polygon (face)
  {
    // "0 < numFaces" guarantees that vIdxsPtr is not NULL

    // note that each cell is a 2D polygon (instead of a 3D cell) and we
    // have to use the cell data attribute, i.e., the attached volume Id,
    // to combine separated 2D polygons to reconstruct a volume.
    if (vIdxsPtr[i] != volIndex)
    {
      // this is the first face of a NEW volume --- init some variables
      newIndex = 0;
      minIndex = fragIndx;
      volIndex = vIdxsPtr[i]; // grouping faces based on the volume Id

      // obtain the attribute values of the sub-volume via the first polygon
      bufIndex = 0;
      for (a = 0; a < numArays; a++)
      {
        theShift = i * numComps[a];
        for (c = 0; c < numComps[a]; c++)
        {
          tupleBuf[bufIndex++] = attrPtrs[a][theShift + c];
        }
      }
    }

    while (vIdxsPtr[i] == volIndex) // for each face of the current volume
    {
      // this is really a face of the current volume: add it to the hash
      hashFace = NULL;
      thisFace = plyHedra->GetCell(i);
      switch (thisFace->GetNumberOfPoints())
      {
        case 3:
          hashFace = this->FaceHash->AddFace(
            thisFace->GetPointId(0), thisFace->GetPointId(1), thisFace->GetPointId(2));
          break;

        case 4:
          hashFace = this->FaceHash->AddFace(thisFace->GetPointId(0), thisFace->GetPointId(1),
            thisFace->GetPointId(2), thisFace->GetPointId(3));
          break;

        case 5:
          hashFace = this->FaceHash->AddFace(thisFace->GetPointId(0), thisFace->GetPointId(1),
            thisFace->GetPointId(2), thisFace->GetPointId(3), thisFace->GetPointId(4));
          break;

        default:
          hashFace = NULL;
          vtkWarningMacro("Invalid number of points: face ignoired.");
          break;
      }
      thisFace = NULL;

      if (hashFace)
      {
        // this face has been added to the hash (unnecessarily the first time
        // --- the same face may have been added to the hash as the constituent
        // polygon of another sub-volume and in this case this face is called
        // an 'internal' face)

        if (hashFace->FragmentId > 0)
        {
          // This is an internal face. It has been removed from the hash when
          // the hash attempts to accept it for the second time, though it is
          // accessible until a new face is allocated from the recycle bin.

          if (hashFace->FragmentId != minIndex && minIndex < fragIndx)
          {
            // This face (X) is not the first one of this volume (R, otherwise
            // minIndex == fragIndx would hold). In fact, there has been a face
            // (Y, of this volume R) that is shared by this volume (R) and
            // a second volume (S, otherwise minIndx == fragIndx would hold).
            // In addition, this face (X) is shared by this volume (R) and a
            // third volume (T, which though has not been merged with volume S,
            // otherwise hashFace->FragmentId == minIndex would hold). In a word,
            // this volume (R) is connected with two currently un-merged volumes
            // S and T. Thus we need to make the fragment Ids of volumes S and T
            // equivalent to each other.
            this->EquivalenceSet->AddEquivalence(minIndex, hashFace->FragmentId);
          }

          // keep track of the smallest fragment id to use for this volume
          if (minIndex > hashFace->FragmentId) // --- case A
          {
            // The first face (certainly internal, since hashFace->FragmentId
            // > 0 holds above) of this volume is guaranteed to come here. In
            // addition, non-first internal faces (of this volume) that are
            // shared by new volumes also come here. In either case, minIndex
            // is updated below to reflect the smallest fragment Id so far and
            // will be assigned to those subsequent new faces of this volume.
            minIndex = hashFace->FragmentId;
          }
        }
        else
        {
          // this is a new face (hashFace->FragmentId is inited to be 0)
          hashFace->BlockId = blockIdx;
          hashFace->PolygonId = i;
          hashFace->ProcessId = procIndx;

          // save this new face until we process all the faces of this
          // volume to determine the smallest fragment id for this volume
          if (newIndex >= VTK_MAX_FACES_PER_CELL)
          {
            vtkErrorMacro(<< "Too many faces for a greater-than-isovalue "
                          << "sub-volume." << endl);
          }
          else
          {
            newFaces[newIndex++] = hashFace;
          }
        } // end if a new face is added to the hash
      }   // end if the input face is valid

      // process the next 2D polygon by updating the index of the face
      i++;
      hashFace = NULL;

    } // for each face of a volume

    // The current face (2D polygon) belongs to a new volume. Before processing
    // it in the next cycle (for each separated 2D polygon), we need to do some
    // thing for the volume that we have just reconstructed / recognized.

    if (minIndex == fragIndx)
    {
      // This is an isolated volume (possibly the first volume of a fragment)
      // since no any neighboring volume has been found (otherwise minIndex
      // would have been updated to be less than fragIndx in case A above).
      // The code below ensures the correct number of equivalence members.
      this->EquivalenceSet->AddEquivalence(fragIndx, fragIndx);
      fragIndx++;
    }

    // update the smallest fragment Id used so far
    minIndex = this->EquivalenceSet->GetEquivalentSetId(minIndex);

    // Label the new faces of the volume with the final (smallest) fragment id.
    for (int k = 0; k < newIndex; k++)
    {
      newFaces[k]->FragmentId = minIndex;
    }

    // fragment attributes integration
    this->IntegrateFragmentAttributes(minIndex, tupleSiz, tupleBuf);

  } // for each separated 2D polygon

  for (i = 0; i < numArays; i++)
  {
    attrPtrs[i] = NULL;
  }
  delete[] attrPtrs;
  delete[] numComps;
  delete[] tupleBuf;
  attrPtrs = NULL;
  numComps = NULL;
  tupleBuf = NULL;
  vIdxsPtr = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::IntegrateFragmentAttributes(
  int fragIndx, int numComps, double* attrVals)
{
  // note this function may be called with non-successive values of fragIndx

  double* attrsPtr = NULL;
  vtkIdType arrayIdx = 0;
  vtkIdType fragSize = this->FragmentValues->GetNumberOfTuples();

  if (fragSize <= fragIndx)
  {
    vtkIdType xtntSize = (fragIndx << 1) + 200;
    this->FragmentValues->Resize(xtntSize);
    this->FragmentValues->SetNumberOfTuples(fragIndx + 1);

    attrsPtr = this->FragmentValues->GetPointer(fragSize * numComps);
    for (arrayIdx = fragSize * numComps; arrayIdx < xtntSize * numComps; arrayIdx++)
    {
      *attrsPtr++ = 0.0;
    }
  }

  attrsPtr = this->FragmentValues->GetPointer(fragIndx * numComps);
  for (arrayIdx = 0; arrayIdx < numComps; arrayIdx++, attrsPtr++, attrVals++)
  {
    (*attrsPtr) += (*attrVals);
  }
  attrsPtr = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ResolveIntegratedFragmentAttributes()
{
  if (!this->EquivalenceSet->Resolved)
  {
    vtkErrorMacro(<< "Equivalences not resolved." << endl);
    return;
  }

  if (this->FragmentValues->GetNumberOfTuples() < this->EquivalenceSet->GetNumberOfMembers())
  {
    vtkErrorMacro(<< "More partial fragments than volume entries." << endl);
    return;
  }

  int i;
  int numComps = this->FragmentValues->GetNumberOfComponents();
  vtkIdType setIndex = 0;
  vtkIdType initials = this->FragmentValues->GetNumberOfTuples();
  vtkIdType resolves = this->EquivalenceSet->GetNumberOfResolvedSets();
  vtkDoubleArray* tmpArray = vtkDoubleArray::New();
  tmpArray->SetNumberOfComponents(numComps);
  tmpArray->SetNumberOfTuples(resolves);
  memset(tmpArray->GetPointer(0), 0, resolves * numComps * sizeof(double));

  double* arayPtr1 = NULL;
  double* arayPtr0 = this->FragmentValues->GetPointer(0);
  for (vtkIdType j = 0; j < initials; j++)
  {
    setIndex = this->EquivalenceSet->GetEquivalentSetId(j);
    for (arayPtr1 = tmpArray->GetPointer(setIndex * numComps), i = 0; i < numComps;
         i++, arayPtr0++, arayPtr1++)
    {
      (*arayPtr1) += (*arayPtr0);
    }
  }

  this->FragmentValues->Delete();
  this->FragmentValues = tmpArray; // to use shallow copy? xxx

  tmpArray = NULL;
  arayPtr0 = NULL;
  arayPtr1 = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ResolveFaceFragmentIds()
{
  vtkRectilinearGridConnectivityFace* thisFace = NULL;
  this->FaceHash->InitTraversal();

  while ((thisFace = this->FaceHash->GetNextFace()))
  {
    thisFace->FragmentId = this->EquivalenceSet->GetEquivalentSetId(thisFace->FragmentId);
  }

  thisFace = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ResolveEquivalentFragments()
{
  this->EquivalenceSet->ResolveEquivalences();
  this->ResolveIntegratedFragmentAttributes();
  this->ResolveFaceFragmentIds();
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::ExtractFragmentPolygons(int blockIdx, int& maxFsize,
  vtkPolyData* plyHedra, vtkPolyData* polygons, vtkIncrementalOctreePointLocator* gPtIdGen)
{
  if (!plyHedra || !polygons || !gPtIdGen ||
    !this->Internal->IntegrableCellDataArraysAvailable(plyHedra))
  {
    vtkErrorMacro(<< "Input marching cubes vtkPolyData, output fragments "
                  << "vtkPolyData, or point locator (a global point Id "
                  << "generator) NULL, or integrable cell data attributes "
                  << "not found from input vtkPolyData." << endl);
    return;
  }

  // create a vtkEquivalenceSet to keep track of fragment ids for the polygons
  // of this polyhedra (resulting from marching cubes) and to determine which
  // fragment-ids need to be combined into a single one for merging fragments.
  if (this->EquivalenceSet)
  {
    this->EquivalenceSet->Delete();
    this->EquivalenceSet = NULL;
  }
  this->EquivalenceSet = vtkEquivalenceSet::New();

  // create a face hash and initialize it with the number of points
  this->InitializeFaceHash(plyHedra);

  // get the number of integrated components (including the material volume)
  // to be saved to and extracted from the global fragment attributes array
  int tupleSiz = this->Internal->NumberIntegralComponents + 1;

  // allocate a vtkDoubleArray to maintain the attributes of each fragment
  if (this->FragmentValues)
  {
    this->FragmentValues->Delete();
    this->FragmentValues = NULL;
  }
  this->FragmentValues = vtkDoubleArray::New();
  this->FragmentValues->SetNumberOfComponents(tupleSiz);

  // add all polygons of the polyhedra to the face hash
  this->AddPolygonsToFaceHash(blockIdx, plyHedra);

  // extract the exterior surfaces / polygons of the fragments
  this->ResolveEquivalentFragments();

  // Obtain fragments' exterior surfaces / polygons of the polyhedra (the
  // greater-than-isovalue sub-volumes) through the face hash and export
  // these exterior surfaces to a vtkPolyData ('polygons').

  int i;
  int theShift;
  int numArays;
  int numFaces;
  int numbPnts; // for a face
  int* numComps = NULL;
  double* tupleBuf;   // integrated component values
  double pntCoord[3]; // a face point
  double thisBbox[6];
  vtkIdType globePId;         // global Id of a point on exterior face
  vtkIdType cellIndx;         // for the output vtkPolyData
  vtkIdType facePIds[5];      // point Ids of a face (at most 5)
  vtkCell* faceCell = NULL;   // a face from the polyhedra
  vtkPoints* hedraPts = NULL; // vtkPoints of the polyhedra
  vtkPoints* polyPnts = NULL;
  vtkCellArray* plyCells = NULL;
  vtkIntArray* fragIdxs = NULL;
  vtkIdTypeArray* uniPIdxs = NULL;
  vtkDoubleArray* theArray = NULL;
  vtkDoubleArray** attrVals = NULL;

  vtkIncrementalOctreePointLocator* pntAdder = NULL;   // for this block only
  vtkRectilinearGridConnectivityFace* thisFace = NULL; // face from the hash
  std::vector<vtkRectilinearGridConnectivityFace*>* theGroup = NULL;
  std::vector<vtkRectilinearGridConnectivityFace*>::iterator faceItrt;
  std::map<int,
    std::vector<vtkRectilinearGridConnectivityFace*> >
    faceGrps; // faces grouped by the fragment Id
  std::map<int, std::vector<vtkRectilinearGridConnectivityFace*> >::iterator grpItrat;

  // First we need to retrieve the exterior faces (with non-zero fragment Ids)
  // from the face hash and put them in a temporary buffer in which they are
  // grouped by the fragment Id. Once the original polygons (triangles, quads,
  // and pentagons) are obtained from the marching-cubes' output (plyHedra),
  // these fragment-Ids will be attached to the polygons as a cell data
  // attribute. This cell data attribute will then be exploited in the final
  // fragments resolution process to recognize each 'macro' volume (i.e., a
  // fragment) that is made up of the (exterior) polygons with the same fragment
  // Id. In this way, the same fragment Id can be maintained for the constituent
  // exterior polygons of each fragment.

  numFaces = 0;
  this->FaceHash->InitTraversal();
  while ((thisFace = this->FaceHash->GetNextFace()))
  {
    if (thisFace->FragmentId > 0)
    {
      numFaces++;
      grpItrat = faceGrps.find(thisFace->FragmentId);

      if (grpItrat == faceGrps.end())
      {
        // create a faces group for this new fragment and add this face to it
        std::vector<vtkRectilinearGridConnectivityFace*> macroVol;
        macroVol.push_back(thisFace);
        faceGrps[thisFace->FragmentId] = macroVol;
      }
      else
      {
        // add this face to the target group
        grpItrat->second.push_back(thisFace);
      }
    }
  }
  thisFace = NULL;

  // the vtkPoints of the output vtkPolyData
  polyPnts = vtkPoints::New();
  polyPnts->Allocate(numFaces << 1, numFaces);

  // a local (block-dependent) point locator used to insert the points
  // of the exterior polygons to the output vtkPolyData
  plyHedra->GetBounds(thisBbox); // fortunately the bounds keep unchanged
  pntAdder = vtkIncrementalOctreePointLocator::New();
  pntAdder->InitPointInsertion(polyPnts, thisBbox, numFaces << 1);

  // array of global point Ids (one per unique point)
  uniPIdxs = vtkIdTypeArray::New();
  uniPIdxs->SetName("GlobalNodeId");
  uniPIdxs->Allocate(numFaces << 1, numFaces);

  // the polygons / cells of the output vtkPolyData (with exterior faces only)
  plyCells = vtkCellArray::New();
  plyCells->Allocate(numFaces, numFaces >> 4);

  // array of fragment Ids (one per constituent polygon)
  // here the fragment Ids are unnecessarily global since they are used for
  // grouping the exterior polygons only
  fragIdxs = vtkIntArray::New();
  fragIdxs->SetName("FragmentId");
  fragIdxs->Allocate(numFaces, numFaces >> 4);

  // allocate a buffer for a tuple of integrated component values (including
  // the material volume) to be extracted from the global fragment attributes
  // array and create a set of vtkDoubleArray objects to collect the integrated
  // attribute values which are attached to the output polygons as the cell data
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  tupleBuf = new double[tupleSiz];
  numComps = new int[numArays];
  attrVals = new vtkDoubleArray*[numArays];
  numComps[0] = 1;
  attrVals[0] = vtkDoubleArray::New();
  attrVals[0]->SetName("MaterialVolume");
  attrVals[0]->SetNumberOfComponents(1);
  attrVals[0]->Allocate(numFaces, numFaces >> 4);
  for (i = 1; i < numArays; i++)
  {
    theArray = vtkDoubleArray::SafeDownCast(
      plyHedra->GetCellData()->GetArray(this->Internal->IntegrableAttributeNames[i - 1].c_str()));
    numComps[i] = theArray->GetNumberOfComponents();
    attrVals[i] = vtkDoubleArray::New();
    attrVals[i]->SetName(theArray->GetName());
    attrVals[i]->SetNumberOfComponents(numComps[i]);
    attrVals[i]->Allocate(numFaces, numFaces >> 4);
    theArray = NULL;
  }

  // Now that all exterior faces (though each with incomplete information ---
  // only three point-Ids are recorded) are grouped in the temporary buffer, we
  // need to retrieve them to obtain the original polygons which are assigned
  // with fragment-Ids as the cell data before being inserted to the output.

  maxFsize = 1;
  hedraPts = plyHedra->GetPoints();
  for (grpItrat = faceGrps.begin(); grpItrat != faceGrps.end(); grpItrat++)
  {
    // the exterior polygons with the same fragment Id, i.e., the exterior
    // polygons of the same fragment, are inserted to the output vtkPolyData
    // one by one successively

    theGroup = &(grpItrat->second);
    maxFsize = (static_cast<int>(theGroup->size()) > maxFsize) ? static_cast<int>(theGroup->size())
                                                               : maxFsize;

    for (faceItrt = theGroup->begin(); faceItrt != theGroup->end(); faceItrt++)
    {
      // gain access to the face (with incomplete info) that is guaranteed to
      // be exterior since we threw away internal faces when creating the
      // grouped faces
      thisFace = *faceItrt;

      // We have re-defined the meaning of vtkRectilinearGridConnectivityFace::
      // PolygonId, which is the polygon Id in the vtkPolyData (the polyhedra).
      // In this way, we can get direct access to the original polygon without
      // resorting to the reconstructed 3D cell (volume) at all.
      faceCell = plyHedra->GetCell(thisFace->PolygonId);

      // add ALL of the points of the face to the output vtkPolyData
      numbPnts = faceCell->GetNumberOfPoints();
      for (i = 0; i < numbPnts; i++)
      {
        hedraPts->GetPoint(faceCell->GetPointId(i), pntCoord);

        // if possible, insert this point to the vtkPoints and assign it
        // with a global Id as the point data attribute
        if (pntAdder->InsertUniquePoint(pntCoord, facePIds[i]))
        {
          gPtIdGen->InsertUniquePoint(pntCoord, globePId);
          uniPIdxs->InsertValue(facePIds[i], globePId);
        }
      }

      // add the original face to the output vtkPolyData (the PolygonId is
      // not useful any more and is ignored below)
      cellIndx = plyCells->InsertNextCell(numbPnts, facePIds);
      fragIdxs->InsertValue(cellIndx, thisFace->FragmentId);
      this->FragmentValues->GetTypedTuple(thisFace->FragmentId, tupleBuf);
      for (theShift = 0, i = 0; i < numArays; i++)
      {
        attrVals[i]->InsertTypedTuple(cellIndx, tupleBuf + theShift);
        theShift += numComps[i];
      }

      thisFace = NULL;
      faceCell = NULL;

      // Set NULL to this entry to avoid the face from being destructed
      // when the vector (theGroup) is removed from the map (groups). We
      // will use 'delete this->FaceHash' later to destroy all the faces.
      *faceItrt = NULL;
    }

    theGroup->clear();
    theGroup = NULL;
  }
  faceGrps.clear();
  hedraPts = NULL;

  // fill the output vtkPolyData --- polygons
  polygons->SetPoints(polyPnts);
  polygons->SetPolys(plyCells);
  polygons->GetPointData()->SetGlobalIds(uniPIdxs);
  polygons->GetCellData()->AddArray(fragIdxs); // for the final resolution
  for (i = 0; i < numArays; i++)
  {
    polygons->GetCellData()->AddArray(attrVals[i]);
    attrVals[i]->Delete();
    attrVals[i] = NULL;
  }
  polygons->Squeeze();

  // memory deallocation
  pntAdder->Delete();
  polyPnts->Delete();
  plyCells->Delete();
  uniPIdxs->Delete();
  fragIdxs->Delete();
  delete[] attrVals;
  delete[] numComps;
  delete[] tupleBuf;

  pntAdder = NULL;
  polyPnts = NULL;
  plyCells = NULL;
  uniPIdxs = NULL;
  fragIdxs = NULL;
  attrVals = NULL;
  numComps = NULL;
  tupleBuf = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::InitializeFaceHash(vtkPolyData** plyDatas, int numPolys)
{
  int i;
  vtkIdType maxIndex = 0;

  // find the maximum global point Id to initialize the face hash
  for (i = 0; i < numPolys; i++)
  {
    // The if-statement below is a MUST since an input vtkPolyData may be just
    // 'empty'. This is the case with both single-process mode and multi-process
    // mode if no any polygon is extracted from the marching-cubes process or
    // else no any polygon remains after in-hash polygons resolution (removal
    // of internal faces). Note that a process may be assigned with no any block
    // at all.
    if (vtkIdTypeArray::SafeDownCast(plyDatas[i]->GetPointData()->GetArray("GlobalNodeId")) == NULL)
    {
      vtkDebugMacro(<< "Point data GlobalNodeId not found in "
                    << "vtkPolyData #" << i << endl);
      continue;
    }

    vtkIdType pntindex = 0;
    vtkIdType numbPnts = plyDatas[i]->GetNumberOfPoints();
    vtkIdType* ptIdsPtr =
      vtkIdTypeArray::SafeDownCast(plyDatas[i]->GetPointData()->GetArray("GlobalNodeId"))
        ->GetPointer(0);

    for (pntindex = 0; pntindex < numbPnts; pntindex++, ptIdsPtr++)
    {
      maxIndex = (*ptIdsPtr > maxIndex) ? (*ptIdsPtr) : maxIndex;
    }
  }

  if (this->FaceHash)
  {
    delete this->FaceHash;
    this->FaceHash = NULL;
  }
  this->FaceHash = new vtkRectilinearGridConnectivityFaceHash;
  this->FaceHash->Initialize(maxIndex + 1);
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddPolygonsToFaceHash(
  vtkPolyData** plyDatas, int* maxFsize, int numPolys)
{
  if (!plyDatas || !maxFsize)
  {
    vtkErrorMacro("Input vtkPolyData array (plyDatas) or maxFsize NULL.");
    return;
  }

  int i, j, k, a, c;
  int procIndx = this->Controller->GetLocalProcessId();
  int theShift = 0;
  int bufIndex = 0;
  int numArays = 0;
  int tupleSiz = 0;        // number of integrated components
  int newIndex = 0;        // index of a new face of the local fragment
  int minIndex = 1;        // the smallest (inter-block) fragment Id
  int fragIndx = 1;        // next inter-block fragment Id and 0 for
                           // removing faces
  int* lfIdsPtr = NULL;    // array of local fragment Ids
  int* numComps = NULL;    // number of integrated components
  double* tupleBuf = NULL; // integrated component values
  double** attrPtrs = NULL;
  vtkCell* thisFace = NULL;   // a 2D polygon (not a 3D cell)
  vtkIdType numFaces = 0;     // number of 2D polygons of an input vtkPolyData
  vtkIdType localFId = 0;     // Id of the local fragment being processed
  vtkIdType numbPnts = 0;     // for a face
  vtkIdType pointIds[5];      // point Ids of a face (at most 5 points)
  vtkIdType* ptIdsPtr = NULL; // array of point Ids
  vtkDoubleArray* theArray = NULL;
  vtkRectilinearGridConnectivityFace* hashFace = NULL; // a face in the hash
  vtkRectilinearGridConnectivityFace** newFaces = NULL;

  // determine the number of integrated components (including the material
  // volume) to be saved to the global fragment attributes array and allocate a
  // buffer for a tuple
  tupleSiz = this->Internal->NumberIntegralComponents + 1;
  tupleBuf = new double[tupleSiz];
  memset(tupleBuf, 0, sizeof(double) * tupleSiz);

  // allocate pointers for access to the arrays of integrated attributes
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  numComps = new int[numArays];
  attrPtrs = new double*[numArays];
  numComps[0] = 1;
  attrPtrs[0] = NULL;
  for (a = 1; a < numArays; a++)
  {
    numComps[a] = 0;
    attrPtrs[a] = NULL;
  }

  // process each vtkPolyData and add its 2D polygons (faces) to the hash
  for (j = 0; j < numPolys; j++)
  {
    // each vtkPolyData stores individual 2D polygons (triangles, quads, and
    // pentagons) of which each is though coupled with a local / block-based
    // fragment Id as the cell data attribute to convey the connectivity of
    // the exterior polygons of the same fragment --- 'macro volume'

    if (vtkIdTypeArray::SafeDownCast(plyDatas[j]->GetPointData()->GetArray("GlobalNodeId")) ==
        NULL ||
      vtkIntArray::SafeDownCast(plyDatas[j]->GetCellData()->GetArray("FragmentId")) == NULL ||
      vtkDoubleArray::SafeDownCast(plyDatas[j]->GetCellData()->GetArray("MaterialVolume")) ==
        NULL ||
      !this->Internal->IntegrableCellDataArraysAvailable(plyDatas[j]))
    {
      vtkDebugMacro(<< "Point data GlobalNodeId, cell data FragmentId, "
                    << "MaterialVolume, or integrable fragemnt "
                    << "attributes not found in vtkPolyData #" << j << endl);
      continue;
    }

    // gain access to global node Ids, global volume Ids, and local fragment Ids
    ptIdsPtr = vtkIdTypeArray::SafeDownCast(plyDatas[j]->GetPointData()->GetArray("GlobalNodeId"))
                 ->GetPointer(0);
    lfIdsPtr =
      vtkIntArray::SafeDownCast(plyDatas[j]->GetCellData()->GetArray("FragmentId"))->GetPointer(0);
    attrPtrs[0] =
      vtkDoubleArray::SafeDownCast(plyDatas[j]->GetCellData()->GetArray("MaterialVolume"))
        ->GetPointer(0);
    for (a = 1; a < numArays; a++)
    {
      theArray = vtkDoubleArray::SafeDownCast(plyDatas[j]->GetCellData()->GetArray(
        this->Internal->IntegrableAttributeNames[a - 1].c_str()));
      attrPtrs[a] = theArray->GetPointer(0);
      numComps[a] = theArray->GetNumberOfComponents();
      theArray = NULL;
    }

    // given the maximum size of a fragment, i.e., the maximum number of
    // faces per fragment in this vtkPolyData, allocate a buffer to maintain
    // the possible new faces of a single fragment
    newFaces = new vtkRectilinearGridConnectivityFace*[maxFsize[j]];
    for (k = 0; k < maxFsize[j]; k++)
    {
      newFaces[k] = NULL;
    }

    i = 0;
    localFId = -1;
    numFaces = plyDatas[j]->GetNumberOfCells();
    while (i < numFaces) // for each individual 2D polygon (face)
    {
      // "0 < numFaces" guarantees ptIdsPtr, and lfIdsPtr are not NULL

      // note that each cell is a 2D polygon (instead of a 3D cell) and we
      // have to use the cell data attribute, i.e., the local fragment Id,
      // to combine individual 2D polygons to reconstruct a 'macro volume'
      if (lfIdsPtr[i] != localFId)
      {
        // the first face of a NEW 'macro' volume --- init some variables
        newIndex = 0;
        minIndex = fragIndx;
        localFId = lfIdsPtr[i]; // grouping faces via the local fragment Id

        // obtain the attribute value of the sub-volume via the first polygon
        bufIndex = 0;
        for (a = 0; a < numArays; a++)
        {
          theShift = i * numComps[a];
          for (c = 0; c < numComps[a]; c++)
          {
            tupleBuf[bufIndex++] = attrPtrs[a][theShift + c];
          }
        }
      }

      while (lfIdsPtr[i] == localFId) // for each face of the 'macro' volume
      {
        // this is a face of the current 'macro' volume
        hashFace = NULL;
        thisFace = plyDatas[j]->GetCell(i);
        numbPnts = thisFace->GetNumberOfPoints();

        // add the face to the hash
        if (numbPnts == 3)
        {
          pointIds[0] = ptIdsPtr[thisFace->GetPointId(0)];
          pointIds[1] = ptIdsPtr[thisFace->GetPointId(1)];
          pointIds[2] = ptIdsPtr[thisFace->GetPointId(2)];
          hashFace = this->FaceHash->AddFace(pointIds[0], pointIds[1], pointIds[2]);
        }
        else if (numbPnts == 4)
        {
          pointIds[0] = ptIdsPtr[thisFace->GetPointId(0)];
          pointIds[1] = ptIdsPtr[thisFace->GetPointId(1)];
          pointIds[2] = ptIdsPtr[thisFace->GetPointId(2)];
          pointIds[3] = ptIdsPtr[thisFace->GetPointId(3)];
          hashFace = this->FaceHash->AddFace(pointIds[0], pointIds[1], pointIds[2], pointIds[3]);
        }
        else if (numbPnts == 5)
        {
          pointIds[0] = ptIdsPtr[thisFace->GetPointId(0)];
          pointIds[1] = ptIdsPtr[thisFace->GetPointId(1)];
          pointIds[2] = ptIdsPtr[thisFace->GetPointId(2)];
          pointIds[3] = ptIdsPtr[thisFace->GetPointId(3)];
          pointIds[4] = ptIdsPtr[thisFace->GetPointId(4)];
          hashFace = this->FaceHash->AddFace(
            pointIds[0], pointIds[1], pointIds[2], pointIds[3], pointIds[4]);
        }
        else
        {
          hashFace = NULL;
          vtkWarningMacro("Face ignored due to invalid number of points.");
        }
        thisFace = NULL;

        if (hashFace)
        {
          // this face has been added to the hash and it is not necessarily the
          // first time --- the same face may have been added to the hash as the
          // constituent polygon of another sub-volume ('macro') and in this case
          // this face is called an 'internal' face

          if (hashFace->FragmentId > 0)
          {
            // This is an internal face. It has been removed from the hash when
            // the hash attempts to accept it for the second time, though it is
            // accessible until a new face is allocated from the recycle bin.

            if (hashFace->FragmentId != minIndex && minIndex < fragIndx)
            {
              // This face (X) is not the first one of this 'macro' volume (R,
              // otherwise minIndex == fragIndx would hold). In fact, there has
              // been a face (Y, of this 'macro' volume R) that is shared by this
              // 'macro' volume (R) and a second 'macro' volume (S, otherwise
              // minIndx == fragIndx would hold). In addition, this face (X) is
              // shared by this 'macro' volume (R) and a third 'macro' volume (T,
              // which though has not been merged with 'macro' volume S, otherwise
              // hashFace->FragmentId == minIndex would hold). In a word, this
              // 'macro' volume (R) is connected with two currently un-merged 'macro'
              // volumes S and T. Thus we need to make the fragment Ids of 'macro'
              // volumes S and T equivalent to each other.
              this->EquivalenceSet->AddEquivalence(minIndex, hashFace->FragmentId);
            }

            // keep track of the smallest fragment id to use for this 'macro' volume
            if (minIndex > hashFace->FragmentId) // --- case A
            {
              // The first face (certainly internal, since hashFace->FragmentId
              // > 0 holds above) of this 'macro' volume is guaranteed to come here.
              // In addition, non-first internal faces (of this 'macro' volume) that
              // are shared by new 'macro' volumes also come here. In either case,
              // minIndex is updated below to reflect the smallest fragment Id so
              // far and will be assigned to those subsequent new faces of this
              // 'macro' volume.
              minIndex = hashFace->FragmentId;
            }
          }
          else
          {
            // this is a new face (hashFace->FragmentId is inited to be 0)
            hashFace->BlockId = j;
            hashFace->PolygonId = i;
            hashFace->ProcessId = procIndx;

            // save this new face until we process all the faces of this
            // 'macro' volume to determine the smallest fragment id
            newFaces[newIndex++] = hashFace;

          } // end if a new face is added to the hash
        }   // end if the input face is valid

        // process the next 2D polygon by updating the index of the face
        i++;
        hashFace = NULL;

      } // for each face of a 'macro' volume

      // The current face (2D polygon) belongs to a new 'macro' volume. Before
      // processing it in the next cycle (for each separated 2D polygon), we
      // need to do some thing for the 'macro' volume that we have just recognized.

      if (minIndex == fragIndx)
      {
        // This is an isolated 'macro' volume (possibly the first 'macro' volume
        // of a fragment) since no any neighboring 'macro' volume has been found
        // (otherwise minIndex would have been updated to be less than fragIndx
        // in case A above). The code below ensures the correct number of
        // equivalence members.
        this->EquivalenceSet->AddEquivalence(fragIndx, fragIndx);
        fragIndx++;
      }

      // update the smallest fragment Id used so far
      minIndex = this->EquivalenceSet->GetEquivalentSetId(minIndex);

      // Label the new faces of the 'macro' volume with the final (smallest)
      // fragment id.
      for (k = 0; k < newIndex; k++)
      {
        newFaces[k]->FragmentId = minIndex;
      }

      // fragment attributes integration
      this->IntegrateFragmentAttributes(minIndex, tupleSiz, tupleBuf);

    } // for each individual 2D polygon

    // clean up the buffer of new faces
    for (k = 0; k < maxFsize[j]; k++)
    {
      newFaces[k] = NULL;
    }
    delete[] newFaces;
    newFaces = NULL;

    for (i = 0; i < numArays; i++)
    {
      attrPtrs[i] = NULL;
    }
    ptIdsPtr = NULL;
    lfIdsPtr = NULL;
  } // for each input vtkPolyData

  delete[] attrPtrs;
  delete[] numComps;
  delete[] tupleBuf;
  attrPtrs = NULL;
  numComps = NULL;
  tupleBuf = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::GenerateOutputFromSingleProcess(
  vtkPolyData** surfaces, int numSurfs, unsigned char partIndx, vtkPolyData* polyData)
{
  // Retrieve part of the surfaces (the exterior surfaces of the fragments) of
  // the greater-than-isovalue sub-volumes through the face hash and export
  // these exterior surfaces to a vtkPolyData.

  if (!surfaces || !polyData)
  {
    vtkErrorMacro(<< "surfaces or polyData NULL" << endl);
    return;
  }

  int i, j;
  int theShift;
  int numArays;
  int tupleSiz; // number of integrated components
  int degnerat;
  int numbPnts; // for a face
  int* numComps = NULL;
  double pntCoord[3];      // a face point
  double* tupleBuf = NULL; // integrated component values
  double* rcBounds = NULL;
  double mbBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  vtkIdType facePIds[5];      // point Ids of a face (at most 5)
  vtkCell* faceCell = NULL;   // a face from the vtkPolyData
  vtkPoints* surfPnts = NULL; // vtkPoints attached to vtkPolyData
  vtkPoints* polyPnts = NULL;
  vtkCellArray* polygons = NULL;
  vtkIntArray* fragIdxs = NULL;
  vtkDoubleArray** attrVals = NULL;
  vtkUnsignedCharArray* partIdxs = NULL;
  vtkIncrementalOctreePointLocator* pntAdder = NULL;
  vtkRectilinearGridConnectivityFace* thisFace = NULL;

  // points and polygons (cells)
  polyPnts = vtkPoints::New();
  polygons = vtkCellArray::New();

  // array of fragment Ids (one per constituent polygon)
  fragIdxs = vtkIntArray::New();
  fragIdxs->SetName("FragmentId");

  // array of part indices (one per constituent polygon, part index is
  // determined by the material volume fraction index)
  partIdxs = vtkUnsignedCharArray::New();
  partIdxs->SetName("Part Index");

  // allocate a buffer for a tuple of integrated component values (including
  // the material volume) to be extracted from the global fragment attributes
  // array and create a set of vtkDoubleArray objects to collect the integrated
  // attribute values which are attached to the output polygons as the cell data
  tupleSiz = this->Internal->NumberIntegralComponents + 1;
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  tupleBuf = new double[tupleSiz];
  numComps = new int[numArays];
  attrVals = new vtkDoubleArray*[numArays];
  numComps[0] = 1;
  attrVals[0] = vtkDoubleArray::New();
  attrVals[0]->SetName("MaterialVolume");
  attrVals[0]->SetNumberOfComponents(1);
  for (i = 1; i < numArays; i++)
  {
    numComps[i] = this->Internal->ComponentNumbersPerArray[i - 1];
    attrVals[i] = vtkDoubleArray::New();
    attrVals[i]->SetName(this->Internal->IntegrableAttributeNames[i - 1].c_str());
    attrVals[i]->SetNumberOfComponents(numComps[i]);
  }

  // create a point locator to maintain all the points accessed from
  // the original vtkPolyData (the exterior polygons of the fragments)
  pntAdder = vtkIncrementalOctreePointLocator::New();
  for (i = 0; i < numSurfs; i++)
  {
    rcBounds = surfaces[i]->GetBounds();
    mbBounds[0] = (rcBounds[0] < mbBounds[0]) ? rcBounds[0] : mbBounds[0];
    mbBounds[2] = (rcBounds[2] < mbBounds[2]) ? rcBounds[2] : mbBounds[2];
    mbBounds[4] = (rcBounds[4] < mbBounds[4]) ? rcBounds[4] : mbBounds[4];
    mbBounds[1] = (rcBounds[1] > mbBounds[1]) ? rcBounds[1] : mbBounds[1];
    mbBounds[3] = (rcBounds[3] > mbBounds[3]) ? rcBounds[3] : mbBounds[3];
    mbBounds[5] = (rcBounds[5] > mbBounds[5]) ? rcBounds[5] : mbBounds[5];
    rcBounds = NULL;
  }
  pntAdder->SetTolerance(0.0001);
  pntAdder->InitPointInsertion(polyPnts, mbBounds, 10000);

  // retrieve each face maintained in the hash
  this->FaceHash->InitTraversal();
  while ((thisFace = this->FaceHash->GetNextFace()))
  {
    // Skip the faces masked (with 0) by the inter-process resolution.
    // Any face with a zero fragment index is an internal face and
    // will not be present in the output.
    if (thisFace->FragmentId > 0)
    {
      // This is an exterior face of some fragment. Note that we have
      // re-defined the meaning of vtkRectilinearGridConnectivityFace::
      // PolygonId, which is just the polygon Id in the vtkPolyData. In this
      // way, we can get direct access to the original polygon without
      // resorting to the reconstructed 3D cell (volume) at all.
      surfPnts = surfaces[thisFace->BlockId]->GetPoints();
      faceCell = surfaces[thisFace->BlockId]->GetCell(thisFace->PolygonId);

      // Let's just duplicate points and we could set up a point map
      // between the input and the output.
      numbPnts = faceCell->GetNumberOfPoints();
      if (numbPnts > 5)
      {
        numbPnts = 5;
        vtkWarningMacro(<< "Not triangle, quad, or pentagon." << endl);
      }

      // add the points of the face to the output vtkPolyData
      for (i = 0; i < numbPnts; i++)
      {
        surfPnts->GetPoint(faceCell->GetPointId(i), pntCoord);
        pntAdder->InsertUniquePoint(pntCoord, facePIds[i]);
      }

      // As we are now collecting polygons for rendering (unless in multi-
      // process mode), polygons that degenerate to lines or points need to
      // be rejected. This rejection does not affect the fragment extraction
      // result at all while guaranteeing the generation of cell normals.
      //
      // It is assumed that polygon degeneration seldom ocurs and therefore
      // the lack of early exit (when evaluating an if-statement) does not
      // cause a negative effect while the use of a single comparison can
      // speed up the whole check.
      degnerat = 0;
      for (i = 0; i < numbPnts - 1; i++)
        for (j = i + 1; j < numbPnts; j++)
        {
          degnerat += static_cast<int>(!(facePIds[i] - facePIds[j]));
        }

      if (numbPnts - degnerat >= 3)
      {
        // it is a triangle, quad, or pentagon (line is rejected)

        // add the original face to the output vtkPolyData
        polygons->InsertNextCell(numbPnts, facePIds);
        fragIdxs->InsertNextValue(thisFace->FragmentId);
        partIdxs->InsertNextValue(partIndx);
        this->FragmentValues->GetTypedTuple(thisFace->FragmentId, tupleBuf);
        for (theShift = 0, i = 0; i < numArays; i++)
        {
          attrVals[i]->InsertNextTypedTuple(tupleBuf + theShift);
          theShift += numComps[i];
        }
      }

    } // end if it is an exterior face
  }   // end loop over faces in the hash

  thisFace = NULL;
  surfPnts = NULL;
  faceCell = NULL;

  // fill the output vtkPolyData
  polyData->SetPoints(polyPnts);
  polyData->SetPolys(polygons);
  polyData->GetCellData()->AddArray(fragIdxs);
  polyData->GetCellData()->AddArray(partIdxs);
  for (i = 0; i < numArays; i++)
  {
    polyData->GetCellData()->AddArray(attrVals[i]);
    attrVals[i]->Delete();
    attrVals[i] = NULL;
  }
  polyData->Squeeze();

  // memory deallocation
  pntAdder->Delete();
  polyPnts->Delete();
  polygons->Delete();
  fragIdxs->Delete();
  partIdxs->Delete();
  delete[] attrVals;
  delete[] numComps;
  delete[] tupleBuf;

  pntAdder = NULL;
  polyPnts = NULL;
  polygons = NULL;
  fragIdxs = NULL;
  partIdxs = NULL;
  attrVals = NULL;
  numComps = NULL;
  tupleBuf = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::CreateInterProcessPolygons(vtkPolyData* fragPoly,
  vtkPolyData* procPoly, vtkIncrementalOctreePointLocator* gPtIdGen, int& maxFsize)
{
  // Given the fragments extraction result (fragPoly) from a process, group the
  // the polygons based on the fragment Id before writing them to the output
  // vtkPolyData in group-wise order. In addition, a global point Id is assigned
  // to each point to support the subsequent inter-process polygons-resolution.

  if (!fragPoly || !procPoly || !gPtIdGen)
  {
    vtkErrorMacro(<< "Input vtkPolyData (fragPoly), point locator (gPtIdGen), "
                  << "or output vtkPolyData (procPoly) NULL." << endl);
    return;
  }

  maxFsize = 1; // must be initialized before the if-statement below

  if (vtkIntArray::SafeDownCast(fragPoly->GetCellData()->GetArray("FragmentId")) == NULL ||
    vtkDoubleArray::SafeDownCast(fragPoly->GetCellData()->GetArray("MaterialVolume")) == NULL ||
    !this->Internal->IntegrableCellDataArraysAvailable(fragPoly))
  {
    vtkDebugMacro(<< "Cell data FragmentId, MaterialVolume, or integrated "
                  << "fragment attributes not found from the extraction "
                  << "result of the process." << endl);
    return;
  }

  int i;
  int numArays;
  int numbPnts;
  int numCells;
  int inCellId;
  int cellIndx;
  int* fIdxsPtr = NULL;
  int* numComps = NULL;
  double** attrPtrs = NULL;
  double pntCoord[3];
  vtkPoints* polyPnts = NULL;
  vtkIdType globalId = 0;
  vtkIntArray* uniFIdxs = NULL;
  vtkCellArray* polygons = NULL;
  vtkIdTypeArray* uniPIdxs = NULL;
  vtkDoubleArray* theArray = NULL;
  vtkDoubleArray** attrVals = NULL;

  std::vector<int>* theGroup = NULL;
  std::vector<int>::iterator cellItrt;
  std::map<int, std::vector<int> > cellGrps;
  std::map<int, std::vector<int> >::iterator grpItrat;

  // number of points and that of cells
  numbPnts = fragPoly->GetNumberOfPoints();
  numCells = fragPoly->GetNumberOfCells();

  // copy the points
  polyPnts = vtkPoints::New();
  polyPnts->DeepCopy(fragPoly->GetPoints());

  // allocate an array of cells
  polygons = vtkCellArray::New();
  polygons->Allocate(numCells);

  // allocate five arrays of data attributes
  uniPIdxs = vtkIdTypeArray::New();
  uniPIdxs->SetName("GlobalNodeId");
  uniPIdxs->SetNumberOfTuples(numbPnts);
  uniPIdxs->SetNumberOfComponents(1);

  uniFIdxs = vtkIntArray::New();
  uniFIdxs->SetName("FragmentId");
  uniFIdxs->SetNumberOfTuples(numCells);
  uniFIdxs->SetNumberOfComponents(1);

  // gain access to various data attributes and create a set of vtkDoubleArray
  // objects to collect the integrated attribute values (including the material
  // volume) which are forwarded to the output polygons as the cell data
  fIdxsPtr =
    vtkIntArray::SafeDownCast(fragPoly->GetCellData()->GetArray("FragmentId"))->GetPointer(0);

  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  numComps = new int[numArays];
  attrPtrs = new double*[numArays];
  attrVals = new vtkDoubleArray*[numArays];
  attrPtrs[0] = vtkDoubleArray::SafeDownCast(fragPoly->GetCellData()->GetArray("MaterialVolume"))
                  ->GetPointer(0);
  numComps[0] = 1;
  attrVals[0] = vtkDoubleArray::New();
  attrVals[0]->SetName("MaterialVolume");
  attrVals[0]->SetNumberOfComponents(1);
  attrVals[0]->SetNumberOfTuples(numCells);
  for (i = 1; i < numArays; i++)
  {
    theArray = vtkDoubleArray::SafeDownCast(
      fragPoly->GetCellData()->GetArray(this->Internal->IntegrableAttributeNames[i - 1].c_str()));
    attrPtrs[i] = theArray->GetPointer(0);
    numComps[i] = theArray->GetNumberOfComponents();
    attrVals[i] = vtkDoubleArray::New();
    attrVals[i]->SetName(theArray->GetName());
    attrVals[i]->SetNumberOfComponents(numComps[i]);
    attrVals[i]->SetNumberOfTuples(numCells);
    theArray = NULL;
  }

  // fill the GlobalNodeId array
  for (i = 0; i < numbPnts; i++)
  {
    fragPoly->GetPoint(i, pntCoord);
    gPtIdGen->InsertUniquePoint(pntCoord, globalId);
    uniPIdxs->SetComponent(i, 0, globalId);
  }

  // group the cells based on the fragment Id
  for (i = 0; i < numCells; i++)
  {
    grpItrat = cellGrps.find(fIdxsPtr[i]);

    if (grpItrat == cellGrps.end())
    {
      // create a group (of cells) for this new fragment and add this cell to it
      std::vector<int> cellGrup;
      cellGrup.push_back(i);
      cellGrps[fIdxsPtr[i]] = cellGrup;
    }
    else
    {
      // add this cell to the target group
      grpItrat->second.push_back(i);
    }
  }

  // retrieve the cells in group-wise order, copy their data attributes to
  // the output vtkPolyData, and obtain the size of the largest fragment
  // (in terms of the number of polygons in a fragment)
  cellIndx = 0;
  for (grpItrat = cellGrps.begin(); grpItrat != cellGrps.end(); grpItrat++)
  {
    theGroup = &(grpItrat->second);
    maxFsize = (static_cast<int>(theGroup->size()) > maxFsize) ? static_cast<int>(theGroup->size())
                                                               : maxFsize;

    for (cellItrt = theGroup->begin(); cellItrt != theGroup->end(); cellItrt++)
    {
      inCellId = *cellItrt;
      polygons->InsertNextCell(fragPoly->GetCell(inCellId)->GetPointIds());
      uniFIdxs->SetComponent(cellIndx, 0, fIdxsPtr[inCellId]);
      for (i = 0; i < numArays; i++)
      {
        attrVals[i]->SetTypedTuple(cellIndx, attrPtrs[i] + inCellId * numComps[i]);
      }

      cellIndx++;
    }

    theGroup->clear();
    theGroup = NULL;
  }
  cellGrps.clear();

  // fill the output vtkPolyData
  procPoly->SetPoints(polyPnts);
  procPoly->SetPolys(polygons);
  procPoly->GetPointData()->SetGlobalIds(uniPIdxs);
  procPoly->GetCellData()->AddArray(uniFIdxs);
  for (i = 0; i < numArays; i++)
  {
    procPoly->GetCellData()->AddArray(attrVals[i]);
    attrVals[i]->Delete();
    attrVals[i] = NULL;
    attrPtrs[i] = NULL;
  }
  procPoly->Squeeze();

  // memory deallocation
  polyPnts->Delete();
  polygons->Delete();
  uniFIdxs->Delete();
  uniPIdxs->Delete();
  delete[] attrVals;
  delete[] attrPtrs;
  delete[] numComps;

  polyPnts = NULL;
  polygons = NULL;
  uniFIdxs = NULL;
  uniPIdxs = NULL;
  attrVals = NULL;
  fIdxsPtr = NULL;
  attrVals = NULL;
  attrPtrs = NULL;
  numComps = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::AddInterProcessPolygonsToFaceHash(
  vtkPolyData** procPlys, int* maxFsize, int numProcs)
{
  if (!procPlys || !maxFsize)
  {
    vtkErrorMacro("Input vtkPolyData array (procPlys) or maxFsize NULL." << endl);
    return;
  }

  int i, j, k, a, c;
  int theShift = 0;
  int bufIndex = 0;
  int numArays = 0;
  int tupleSiz = 0;        // number of integrated components
  int newIndex = 0;        // index of a new face of a local fragment
  int minIndex = 1;        // the smallest (inter-process) fragment Id
  int fragIndx = 1;        // next inter-process fragment Id and 0 for
                           // removing faces
  int* fIdxsPtr = NULL;    // array of local fragment Ids
  int* numComps = NULL;    // number of integrated components
  double* tupleBuf = NULL; // integrated component values
  double** attrPtrs = NULL;
  vtkCell* thisFace = NULL;   // a 2D polygon (not a 3D cell)
  vtkIdType numFaces = 0;     // number of 2D polygons of an input vtkPolyData
  vtkIdType procFIdx = 0;     // Id of the local fragment being processed
  vtkIdType numbPnts = 0;     // for a face / polygon
  vtkIdType pointIds[5];      // point Ids of a face (at most 5 points)
  vtkIdType* pIdxsPtr = NULL; // array of point Ids
  vtkDoubleArray* theArray = NULL;
  vtkRectilinearGridConnectivityFace* hashFace = NULL; // a face in the hash
  vtkRectilinearGridConnectivityFace** newFaces = NULL;

  // determine the number of integrated components (including the material
  // volume) to be saved to the global fragment attributes array and allocate
  // a buffer for a tuple
  tupleSiz = this->Internal->NumberIntegralComponents + 1;
  tupleBuf = new double[tupleSiz];
  memset(tupleBuf, 0, sizeof(double) * tupleSiz);

  // allocate pointers for access to the arrays of integrated attributes
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  numComps = new int[numArays];
  attrPtrs = new double*[numArays];
  numComps[0] = 1;
  attrPtrs[0] = NULL;
  for (a = 1; a < numArays; a++)
  {
    numComps[a] = 0;
    attrPtrs[a] = NULL;
  }

  // process each vtkPolyData and add 2D polygons (faces) to the global hash
  for (j = 0; j < numProcs; j++)
  {
    // each vtkPolyData stores individual 2D polygons (triangles, quads, and
    // pentagons) of which each is though coupled with a local / process-based
    // fragment Id as the cell data attribute to convey the connectivity of
    // the exterior polygons of the same fragment --- 'macro volume'

    // Make sure this vtkPolyData has global node Ids, block Ids, fragment Ids,
    // and volume Ids. The vtkPolyData returned by a process (possibly assigned
    // with no any block at all due to the number of processes greater than that
    // of blocks) after local in-hash polygons-resolution may be just 'empty'.
    if (vtkIdTypeArray::SafeDownCast(procPlys[j]->GetPointData()->GetArray("GlobalNodeId")) ==
        NULL ||
      vtkIntArray::SafeDownCast(procPlys[j]->GetCellData()->GetArray("FragmentId")) == NULL ||
      vtkDoubleArray::SafeDownCast(procPlys[j]->GetCellData()->GetArray("MaterialVolume")) ==
        NULL ||
      !this->Internal->IntegrableCellDataArraysAvailable(procPlys[j]))
    {
      vtkDebugMacro(<< "Point data GlobalNodeId, cell data FragmentId, "
                    << "MaterialVolume, or integrable fragment "
                    << "attributes not found in vtkPolyData #" << j << endl);
      continue;
    }

    // gain access to global node Ids, block Ids, fragment Ids, and volume Ids
    pIdxsPtr = vtkIdTypeArray::SafeDownCast(procPlys[j]->GetPointData()->GetArray("GlobalNodeId"))
                 ->GetPointer(0);
    fIdxsPtr =
      vtkIntArray::SafeDownCast(procPlys[j]->GetCellData()->GetArray("FragmentId"))->GetPointer(0);
    attrPtrs[0] =
      vtkDoubleArray::SafeDownCast(procPlys[j]->GetCellData()->GetArray("MaterialVolume"))
        ->GetPointer(0);
    for (a = 1; a < numArays; a++)
    {
      theArray = vtkDoubleArray::SafeDownCast(procPlys[j]->GetCellData()->GetArray(
        this->Internal->IntegrableAttributeNames[a - 1].c_str()));
      attrPtrs[a] = theArray->GetPointer(0);
      numComps[a] = theArray->GetNumberOfComponents();
      theArray = NULL;
    }

    // given the maximum size of a fragment, i.e., the maximum number of
    // faces per fragment in this vtkPolyData, allocate a buffer to maintain
    // the possible new faces of a single fragment
    newFaces = new vtkRectilinearGridConnectivityFace*[maxFsize[j]];
    for (k = 0; k < maxFsize[j]; k++)
    {
      newFaces[k] = NULL;
    }

    i = 0;
    procFIdx = -1;
    numFaces = procPlys[j]->GetNumberOfCells();
    while (i < numFaces) // for each individual 2D polygon (face)
    {
      // "0 < numFaces" guarantees pIdxsPtr and fIdxsPtr are not NULL

      // note that each cell is a 2D polygon (instead of a 3D cell) and we
      // have to use the cell data attribute, i.e., the local fragment Id,
      // to combine individual 2D polygons to reconstruct a 'macro volume'
      if (fIdxsPtr[i] != procFIdx)
      {
        // the first face of a NEW 'macro' volume --- init some variables
        newIndex = 0;
        minIndex = fragIndx;
        procFIdx = fIdxsPtr[i]; // group faces via the local fragment Id

        // obtain the attribute values of the sub-volume via the first polygon
        bufIndex = 0;
        for (a = 0; a < numArays; a++)
        {
          theShift = i * numComps[a];
          for (c = 0; c < numComps[a]; c++)
          {
            tupleBuf[bufIndex++] = attrPtrs[a][theShift + c];
          }
        }
      }

      while (fIdxsPtr[i] == procFIdx) // for each face of the 'macro' volume
      {
        // this is a face of the current 'macro' volume
        hashFace = NULL;
        thisFace = procPlys[j]->GetCell(i);
        numbPnts = thisFace->GetNumberOfPoints();

        // add the face to the hash
        if (numbPnts == 3)
        {
          pointIds[0] = pIdxsPtr[thisFace->GetPointId(0)];
          pointIds[1] = pIdxsPtr[thisFace->GetPointId(1)];
          pointIds[2] = pIdxsPtr[thisFace->GetPointId(2)];
          hashFace = this->FaceHash->AddFace(pointIds[0], pointIds[1], pointIds[2]);
        }
        else if (numbPnts == 4)
        {
          pointIds[0] = pIdxsPtr[thisFace->GetPointId(0)];
          pointIds[1] = pIdxsPtr[thisFace->GetPointId(1)];
          pointIds[2] = pIdxsPtr[thisFace->GetPointId(2)];
          pointIds[3] = pIdxsPtr[thisFace->GetPointId(3)];
          hashFace = this->FaceHash->AddFace(pointIds[0], pointIds[1], pointIds[2], pointIds[3]);
        }
        else if (numbPnts == 5)
        {
          pointIds[0] = pIdxsPtr[thisFace->GetPointId(0)];
          pointIds[1] = pIdxsPtr[thisFace->GetPointId(1)];
          pointIds[2] = pIdxsPtr[thisFace->GetPointId(2)];
          pointIds[3] = pIdxsPtr[thisFace->GetPointId(3)];
          pointIds[4] = pIdxsPtr[thisFace->GetPointId(4)];
          hashFace = this->FaceHash->AddFace(
            pointIds[0], pointIds[1], pointIds[2], pointIds[3], pointIds[4]);
        }
        else
        {
          hashFace = NULL;
          vtkWarningMacro("Face ignored due to invalid number of points.");
        }
        thisFace = NULL;

        if (hashFace)
        {
          // this face has been added to the hash and it is not necessarily the
          // first time --- the same face may have been added to the hash as the
          // constituent polygon of another sub-volume ('macro') and in this case
          // this face is called an 'internal' face

          if (hashFace->FragmentId > 0)
          {
            // This is an internal face. It has been removed from the hash when
            // the hash attempts to accept it for the second time, though it is
            // accessible until a new face is allocated from the recycle bin.

            if (hashFace->FragmentId != minIndex && minIndex < fragIndx)
            {
              // This face (X) is not the first one of this 'macro' volume (R,
              // otherwise minIndex == fragIndx would hold). In fact, there has
              // been a face (Y, of this 'macro' volume R) that is shared by this
              // 'macro' volume (R) and a second 'macro' volume (S, otherwise
              // minIndx == fragIndx would hold). In addition, this face (X) is
              // shared by this 'macro' volume (R) and a third 'macro' volume (T,
              // which though has not been merged with 'macro' volume S, otherwise
              // hashFace->FragmentId == minIndex would hold). In a word, this
              // 'macro' volume (R) is connected with two currently un-merged 'macro'
              // volumes S and T. Thus we need to make the fragment Ids of 'macro'
              // volumes S and T equivalent to each other.
              this->EquivalenceSet->AddEquivalence(minIndex, hashFace->FragmentId);
            }

            // keep track of the smallest fragment id to use for this 'macro' volume
            if (minIndex > hashFace->FragmentId) // --- case A
            {
              // The first face (certainly internal, since hashFace->FragmentId
              // > 0 holds above) of this 'macro' volume is guaranteed to come here.
              // In addition, non-first internal faces (of this 'macro' volume) that
              // are shared by new 'macro' volumes also come here. In either case,
              // minIndex is updated below to reflect the smallest fragment Id so
              // far and will be assigned to those subsequent new faces of this
              // 'macro' volume.
              minIndex = hashFace->FragmentId;
            }
          }
          else
          {
            // this is a new face (hashFace->FragmentId is inited to be 0)
            hashFace->PolygonId = i;
            hashFace->ProcessId = j;

            // save this new face until we process all the faces of this
            // 'macro' volume to determine the smallest fragment id
            newFaces[newIndex++] = hashFace;

          } // end if a new face is added to the hash
        }   // end if the input face is valid

        // process the next 2D polygon by updating the index of the face
        i++;
        hashFace = NULL;

      } // for each face of a 'macro' volume

      // The current face (2D polygon) belongs to a new 'macro' volume. Before
      // processing it in the next cycle (for each separated 2D polygon), we
      // need to do some thing for the 'macro' volume that we have just recognized.

      if (minIndex == fragIndx)
      {
        // This is an isolated 'macro' volume (possibly the first 'macro' volume
        // of a fragment) since no any neighboring 'macro' volume has been found
        // (otherwise minIndex would have been updated to be less than fragIndx
        // in case A above). The code below ensures the correct number of
        // equivalence members.
        this->EquivalenceSet->AddEquivalence(fragIndx, fragIndx);
        fragIndx++;
      }

      // update the smallest fragment Id used so far
      minIndex = this->EquivalenceSet->GetEquivalentSetId(minIndex);

      // Label the new faces of the 'macro' volume with the final (smallest)
      // fragment id.
      for (k = 0; k < newIndex; k++)
      {
        newFaces[k]->FragmentId = minIndex;
      }

      // fragment attributes integration
      this->IntegrateFragmentAttributes(minIndex, tupleSiz, tupleBuf);

    } // for each individual 2D polygon

    // clean up the buffer of new faces
    for (k = 0; k < maxFsize[j]; k++)
    {
      newFaces[k] = NULL;
    }
    delete[] newFaces;
    newFaces = NULL;

    for (a = 0; a < numArays; a++)
    {
      attrPtrs[a] = NULL;
    }
    pIdxsPtr = NULL;
    fIdxsPtr = NULL;
  } // for each input vtkPolyData

  delete[] attrPtrs;
  delete[] numComps;
  delete[] tupleBuf;
  attrPtrs = NULL;
  numComps = NULL;
  tupleBuf = NULL;
}

//-----------------------------------------------------------------------------
void vtkRectilinearGridConnectivity::GenerateOutputFromMultiProcesses(
  vtkPolyData** procPlys, int numProcs, unsigned char partIndx, vtkPolyData* polyData)
{
  // Access the global face hash to obtain the exterior surfaces (with non-zero
  // fragment Ids) of the fragments extracted by multiple processes and export
  // these final exterior surfaces to a vtkPolyData.

  if (!procPlys || !polyData)
  {
    vtkErrorMacro(<< "Input vtkPolyData array (procPlys) or output vtkPolyData "
                  << "(polyData) NULL." << endl);
    return;
  }

  int i, j;
  int theShift;
  int numArays;
  int tupleSiz; // number of integrated components
  int degnerat;
  int numbPnts; // for an exterior face
  int* numComps = NULL;
  double pntCoord[3];
  double* tupleBuf = NULL; // integrated component values
  double* rcBounds = NULL;
  double mbBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  vtkCell* faceCell = NULL;   // a polygon of a vtkPolyData
  vtkIdType facePIds[5];      // point Ids of a polygon (at most 5)
  vtkPoints* surfPnts = NULL; // vtkPoints attached to a vtkPolyData
  vtkPoints* polyPnts = NULL;
  vtkIntArray* fragIdxs = NULL;
  vtkIntArray* procIdxs = NULL;
  vtkCellArray* polygons = NULL;
  vtkDoubleArray** attrVals = NULL;
  vtkUnsignedCharArray* partIdxs = NULL;
  vtkIncrementalOctreePointLocator* pntAdder = NULL;
  vtkRectilinearGridConnectivityFace* thisFace = NULL;

  // output points and polygons (cells)
  polyPnts = vtkPoints::New();
  polygons = vtkCellArray::New();

  // array of fragment Ids (one per constituent polygon)
  fragIdxs = vtkIntArray::New();
  fragIdxs->SetName("FragmentId");

  // array of process Ids (one per constituent polygon)
  procIdxs = vtkIntArray::New();
  procIdxs->SetName("ProcessId");

  // array of part indices (one per constituent polygon, part index is
  // determined by the material volume fraction index)
  partIdxs = vtkUnsignedCharArray::New();
  partIdxs->SetName("Part Index");

  // allocate a buffer for a tuple of integrated component values (including
  // the material volume) to be extracted from the global fragment attributes
  // array and create a set of vtkDoubleArray objects to collect the integrated
  // attribute values which are attached to the output polygons as the cell data
  tupleSiz = this->Internal->NumberIntegralComponents + 1;
  numArays = int(this->Internal->IntegrableAttributeNames.size()) + 1;
  tupleBuf = new double[tupleSiz];
  numComps = new int[numArays];
  attrVals = new vtkDoubleArray*[numArays];
  numComps[0] = 1;
  attrVals[0] = vtkDoubleArray::New();
  attrVals[0]->SetName("MaterialVolume");
  attrVals[0]->SetNumberOfComponents(1);
  for (i = 1; i < numArays; i++)
  {
    numComps[i] = this->Internal->ComponentNumbersPerArray[i - 1];
    attrVals[i] = vtkDoubleArray::New();
    attrVals[i]->SetName(this->Internal->IntegrableAttributeNames[i - 1].c_str());
    attrVals[i]->SetNumberOfComponents(numComps[i]);
  }

  // create a point locator to maintain all the points of the output
  pntAdder = vtkIncrementalOctreePointLocator::New();
  for (i = 0; i < numProcs; i++)
  {
    rcBounds = procPlys[i]->GetBounds();
    mbBounds[0] = (rcBounds[0] < mbBounds[0]) ? rcBounds[0] : mbBounds[0];
    mbBounds[2] = (rcBounds[2] < mbBounds[2]) ? rcBounds[2] : mbBounds[2];
    mbBounds[4] = (rcBounds[4] < mbBounds[4]) ? rcBounds[4] : mbBounds[4];
    mbBounds[1] = (rcBounds[1] > mbBounds[1]) ? rcBounds[1] : mbBounds[1];
    mbBounds[3] = (rcBounds[3] > mbBounds[3]) ? rcBounds[3] : mbBounds[3];
    mbBounds[5] = (rcBounds[5] > mbBounds[5]) ? rcBounds[5] : mbBounds[5];
    rcBounds = NULL;
  }
  pntAdder->SetTolerance(0.0001);
  pntAdder->InitPointInsertion(polyPnts, mbBounds, 10000);

  // retrieve each face maintained in the global hash
  this->FaceHash->InitTraversal();
  while ((thisFace = this->FaceHash->GetNextFace()))
  {
    // Skip the faces masked (with 0) by the inter-process resolution.
    // Any face with a zero fragment index is an internal face and
    // will not be present in the output.
    if (thisFace->FragmentId > 0)
    {
      // This is an exterior face of some fragment. Note that we have
      // re-defined the meaning of vtkRectilinearGridConnectivityFace::
      // PolygonId, which is just the polygon Id in the associated vtkPolyData
      // (specified by ProcessId --- not BlockId any more).
      surfPnts = procPlys[thisFace->ProcessId]->GetPoints();
      faceCell = procPlys[thisFace->ProcessId]->GetCell(thisFace->PolygonId);

      numbPnts = faceCell->GetNumberOfPoints();
      if (numbPnts > 5)
      {
        numbPnts = 5;
        vtkWarningMacro(<< "Not triangle, quad, or pentagon." << endl);
      }

      // add the points of the polygon to the output vtkPolyData
      for (i = 0; i < numbPnts; i++)
      {
        surfPnts->GetPoint(faceCell->GetPointId(i), pntCoord);
        pntAdder->InsertUniquePoint(pntCoord, facePIds[i]);
      }

      // As we are now collecting polygons for rendering, polygons that
      // degenerate to lines or points need to be rejected. This rejection
      // does not affect the fragment extraction result while guaranteeing
      // the generation of cell normals.
      //
      // It is assumed that polygon degeneration seldom ocurs and therefore
      // the lack of early exit (when evaluating an if-statement) does not
      // cause a negative effect while the use of a single comparison can
      // speed up the whole check.
      degnerat = 0;
      for (i = 0; i < numbPnts - 1; i++)
        for (j = i + 1; j < numbPnts; j++)
        {
          degnerat += static_cast<int>(!(facePIds[i] - facePIds[j]));
        }

      if (numbPnts - degnerat >= 3)
      {
        // it is a triangle, quad, or pentagon (line is rejected)

        // add the original polygon and the associated cell data attributes of
        // interest to the output vtkPolyData
        polygons->InsertNextCell(numbPnts, facePIds);
        fragIdxs->InsertNextValue(thisFace->FragmentId);
        procIdxs->InsertNextValue(thisFace->ProcessId);
        partIdxs->InsertNextValue(partIndx);
        this->FragmentValues->GetTypedTuple(thisFace->FragmentId, tupleBuf);
        for (theShift = 0, i = 0; i < numArays; i++)
        {
          attrVals[i]->InsertNextTypedTuple(tupleBuf + theShift);
          theShift += numComps[i];
        }
      }

    } // end if it is an exterior face
  }   // end loop over faces in the hash

  thisFace = NULL;
  surfPnts = NULL;
  faceCell = NULL;

  // fill the output vtkPolyData
  polyData->SetPoints(polyPnts);
  polyData->SetPolys(polygons);
  polyData->GetCellData()->AddArray(fragIdxs);
  polyData->GetCellData()->AddArray(procIdxs);
  polyData->GetCellData()->AddArray(partIdxs);
  for (i = 0; i < numArays; i++)
  {
    polyData->GetCellData()->AddArray(attrVals[i]);
    attrVals[i]->Delete();
    attrVals[i] = NULL;
  }
  polyData->Squeeze();

  // memory deallocation
  pntAdder->Delete();
  polyPnts->Delete();
  polygons->Delete();
  fragIdxs->Delete();
  procIdxs->Delete();
  partIdxs->Delete();
  delete[] attrVals;
  delete[] numComps;
  delete[] tupleBuf;

  pntAdder = NULL;
  polyPnts = NULL;
  polygons = NULL;
  fragIdxs = NULL;
  procIdxs = NULL;
  partIdxs = NULL;
  attrVals = NULL;
  numComps = NULL;
  tupleBuf = NULL;
}

//-----------------------------------------------------------------------------
// An extended marching cubes case table for generating cube faces (either
// truncated by iso-lines or not) in addition to iso-triangles. These two
// kinds of polygons in combination represent the surface(s) of the greater-
// than-isovalue sub-volume(s) extracted in a cube. An index in each entry
// may refer to an Interpolated Iso-Value Point (IIVP, i.e., the associated
// edge, 0 ~ 11) or one of the eight Vertices Of the Cube (VOC: 12 ~ 19).
// The constituent polygons (triangles, quads, or pentagons) of a sub-volume
// are listed successively, ending with flag -1 in sepration from another sub-
// volume that may follows. Each polygon begins with the number of the points
// followed by the specific IIVP-Ids and VOC-Ids. Flag -2 terminates the list
// of all sub-volumes, if any. The two integers in the annotation section of
// each entry indicate the case number (0 ~ 255) and the base case number (0
// ~ 15), respectively.

#include "vtkRectilinearGridConnectivityCases.cxx"

//-----------------------------------------------------------------------------
vtkRectilinearGridConnectivityMarchingCubesVolumeCases*
vtkRectilinearGridConnectivityMarchingCubesVolumeCases::GetCases()
{
  return RECTILINEAR_GRID_CONNECTIVITY_MARCHING_CUBES_VOLUME_CASES;
}
