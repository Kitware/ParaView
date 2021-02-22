/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMaterialInterfaceFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaterialInterfaceFilter.h"

// Pipeline & VTK
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
// PV interface
#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkMath.h"
// Data sets
#include "vtkAMRBox.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPolyData.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
// Data types, Arrays & Containers
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCollection.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkMaterialInterfaceIdList.h"
#include "vtkMaterialInterfacePieceLoading.h"
#include "vtkMaterialInterfacePieceTransaction.h"
#include "vtkMaterialInterfacePieceTransactionMatrix.h"
#include "vtkMaterialInterfaceProcessLoading.h"
#include "vtkMaterialInterfaceProcessRing.h"
#include "vtkMaterialInterfaceToProcMap.h"
#include "vtkPointAccumulator.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
// IO & IPC
#include "vtkDataSetWriter.h"
#include "vtkMaterialInterfaceCommBuffer.h"
#include "vtkXMLPolyDataWriter.h"
// Filters
#include "vtkAppendPolyData.h"
#include "vtkAppendPolyData.h"
#include "vtkCleanPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkContourFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkOBBTree.h"
#include "vtkTriangleFilter.h"
// STL
#include <fstream>
using std::ofstream;
#include <sstream>
using std::ostringstream;
#include <vector>
using std::vector;
#include <string>
using std::string;
#include "algorithm"
// ansi c
#include <ctime>
#include <math.h>
// other
#include "vtkMaterialInterfaceUtilities.h"

// for paraview cliping
#include "vtkPlane.h"
#include "vtkSphere.h"

class InitializeVolumeFractrionArray;

vtkStandardNewMacro(vtkMaterialInterfaceFilter);
vtkCxxSetObjectMacro(vtkMaterialInterfaceFilter, ClipFunction, vtkImplicitFunction);

// NOTE:
// The filter could be improved by folding in ResolveIntegratedAttributes method into
// the ResolveGemoetric attributes method. This will be more efficient because
// only fragments which are split across processes need special treatment and
// ResolveGeometricAttributes already handles this.

// NOTE:
//
// 1: We have three tasks:  Connect and index face connected cells.
//    Create faces of connected cells.
//    Share points but not all points.  Split points to make topology manifold.
// 2: Connectivity is straight forward depth or breadth first search.
// 3: We can have a special id for not visited and a special id for empty.
// 4: Depth first is easier but we must protect for stack overflow.
// 5: For the surface, we are simply going to take the surface of the voxels
//    and place the vertices based on volume fraction values.
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
//      We will use the fragment ids in the ghost layers to create an equivalency relation
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
namespace
{
vtkUniformGrid* GetReferenceGrid(vtkNonOverlappingAMR* amrds)
{
  unsigned int numLevels = amrds->GetNumberOfLevels();
  for (unsigned int l = 0; l < numLevels; ++l)
  {
    unsigned int numDatasets = amrds->GetNumberOfDataSets(l);
    for (unsigned int dataIdx = 0; dataIdx < numDatasets; ++dataIdx)
    {
      vtkUniformGrid* refGrid = amrds->GetDataSet(l, dataIdx);
      if (refGrid != nullptr)
      {
        return (refGrid);
      }
    } // END for all datasets
  }   // END for all number of levels

  // This process has no grids
  return nullptr;
}

// Construct a list of the selected array names
int GetEnabledArrayNames(vtkDataArraySelection* das, vector<string>& names)
{
  int nEnabled = das->GetNumberOfArraysEnabled();
  names.resize(nEnabled);
  int nArraysTotal = das->GetNumberOfArrays();
  for (int i = 0, j = 0; i < nArraysTotal; ++i)
  {
    // skip disabled arrays
    if (!das->GetArraySetting(i))
    {
      continue;
    }
    // save names of enabled arrays, inc name count
    names[j] = das->GetArrayName(i);
    ++j;
  }
  return nEnabled;
}
};
//============================================================================
// A class that implements an equivalent set.  It is used to combine fragments
// from different processes.
//
// I believe that this class is a strictly ordered tree of equivalences.
// Every member points to its own id or an id smaller than itself.
class vtkMaterialInterfaceEquivalenceSet
{
public:
  vtkMaterialInterfaceEquivalenceSet();
  ~vtkMaterialInterfaceEquivalenceSet();

  void Print();

  void Initialize();
  void AddEquivalence(int id1, int id2);

  // The length of the equivalent array...
  int GetNumberOfMembers() { return this->EquivalenceArray->GetNumberOfTuples(); }

  // Return the id of the equivalent set.
  int GetEquivalentSetId(int memberId);

  // Equivalent set ids are reassinged to be sequential.
  // You cannot add anymore equivalences after this is called.
  int ResolveEquivalences();

  void DeepCopy(vtkMaterialInterfaceEquivalenceSet* in);

  // Needed for sending the set over MPI.
  // Be very careful with the pointer.
  int* GetPointer() { return this->EquivalenceArray->GetPointer(0); }

  // Free unused memory
  void Squeeze() { this->EquivalenceArray->Squeeze(); }

  // Report used memory
  vtkIdType Capacity() { return this->EquivalenceArray->Capacity(); }

  // We should fix the pointer API and hide this ivar.
  int Resolved;

private:
  // To merge connected framgments that have different ids because they were
  // traversed by different processes or passes.
  vtkIntArray* EquivalenceArray;

  // Return the id of the equivalent set.
  int GetReference(int memberId);
  void EquateInternal(int id1, int id2);
};

//----------------------------------------------------------------------------
vtkMaterialInterfaceEquivalenceSet::vtkMaterialInterfaceEquivalenceSet()
{
  this->Resolved = 0;
  this->EquivalenceArray = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkMaterialInterfaceEquivalenceSet::~vtkMaterialInterfaceEquivalenceSet()
{
  this->Resolved = 0;
  if (this->EquivalenceArray)
  {
    this->EquivalenceArray->Delete();
    this->EquivalenceArray = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceEquivalenceSet::Initialize()
{
  this->Resolved = 0;
  this->EquivalenceArray->Initialize();
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceEquivalenceSet::DeepCopy(vtkMaterialInterfaceEquivalenceSet* in)
{
  this->Resolved = in->Resolved;
  this->EquivalenceArray->DeepCopy(in->EquivalenceArray);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceEquivalenceSet::Print()
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
int vtkMaterialInterfaceEquivalenceSet::GetEquivalentSetId(int memberId)
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
int vtkMaterialInterfaceEquivalenceSet::GetReference(int memberId)
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
void vtkMaterialInterfaceEquivalenceSet::AddEquivalence(int id1, int id2)
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
void vtkMaterialInterfaceEquivalenceSet::EquateInternal(int id1, int id2)
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
int vtkMaterialInterfaceEquivalenceSet::ResolveEquivalences()
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
      this->EquivalenceArray->SetValue(ii, newId);
    }
  }
  this->Resolved = 1;
  // cerr << "Final number of equivalent sets: " << count << endl;

  return count;
}

//============================================================================
// Helper object to clip hexahedra with implicit half sphere.
class vtkMaterialInterfaceFilterHalfSphere
{
public:
  vtkMaterialInterfaceFilterHalfSphere();

  double ComputeTriangleProjectionArea(double pt1[3], double pt2[3], double pt3[3], double zPlane);
  double* GetCasePoint(
    int pointIdx, double bds[6], double cVals[8], double points[28][3], int pointFlags[28]);
  double EvaluateHalfSpherePoint(double pt[3]);
  double EvaluateHalfSphereBox(double bounds[6]);

  double Center[3];
  int ClipWithSphere;
  double SphereRadius;
  int ClipWithPlane;
  double PlaneNormal[3];

  // int ConvertCasePointToIndex(double point[3], int pointCode[3]);
  // void GenerateClippedVoxelCaseTable();
};

//============================================================================
// A class to hold block information. Input is not HierarichalBox so I need
// to extract and store this extra information for each block.
class vtkMaterialInterfaceFilterBlock
{
public:
  vtkMaterialInterfaceFilterBlock();
  ~vtkMaterialInterfaceFilterBlock();

  // For normal (local) blocks.
  void Initialize(int blockId, vtkImageData* imageBlock, int level, double globalOrigin[3],
    double rootSapcing[3], string& volumeFractionArrayName, string& massArrayName,
    vector<string>& volumeWtdAvgArrayNames, vector<string>& massWtdAvgArrayNames,
    vector<string>& summedArrayNames, vector<string>& integratedArrayNames,
    int invertVolumeFraction, vtkMaterialInterfaceFilterHalfSphere* sphere);

  void InitializeVolumeFractionArray(int invertVolumeFraction,
    vtkMaterialInterfaceFilterHalfSphere* sphere, vtkDataArray* volumeFractionArray);

  // Determine the extent without overlap using the standard block
  // dimensions. (ie indexes region excluding ghosts)
  // And the overlapping dimension (usually 1 or 0)
  void ComputeBaseExtent(int blockDims[3], unsigned char levelOfGhostLayer);

  // For ghost layers received by other processes.
  // The argument list here is getting a bit long ....
  void InitializeGhostLayer(unsigned char* volFraction, int cellExtent[6], int level,
    double globalOrigin[3], double rootSpacing[3], int ownerProcessId, int blockId);
  // This adds a block as our neighbor and makes the reciprical connection too.
  // It is used to connect ghost layer blocks.  Ghost blocks need the reciprical
  // connection to continue the connectivity search.
  // I am hoping this will make the number of fragments to merge smaller.
  void AddNeighbor(vtkMaterialInterfaceFilterBlock* block, int axis, int maxFlag);

  void GetPointExtent(int ext[6]);
  void GetCellExtent(int ext[6]);
  void GetCellIncrements(int incs[3]);
  const int* GetCellIncrements() { return this->CellIncrements; }
  // Get the cell extents that exclude the ghosts.
  void GetBaseCellExtent(int ext[6]);
  // This was a major time consumer so use the pointer directly.
  const int* GetBaseCellExtent() { return this->BaseCellExtent; }

  unsigned char GetGhostFlag() { return this->GhostFlag; }
  // Information saved for ghost cells that makes it easier to
  // resolve equivalent fragment ids.
  int GetOwnerProcessId() { return this->ProcessId; }
  int GetBlockId() { return this->BlockId; }

  // Returns a pointer to the minimum AMR Extent with is the first
  // cell values that is not a ghost cell.
  unsigned char* GetBaseVolumeFractionPointer();
  int* GetBaseFragmentIdPointer();
  int GetBaseFlatIndex();
  int* GetFragmentIdPointer() { return this->FragmentIds; }
  int GetLevel() { return this->Level; }
  double* GetSpacing() { return this->Spacing; }
  double* GetOrigin() { return this->Origin; }
  // Faces are indexed: 0=xMin, 1=xMax, 2=yMin, 3=yMax, 4=zMin, 5=zMax
  int GetNumberOfFaceNeighbors(int face) { return static_cast<int>(this->Neighbors[face].size()); }
  //
  vtkMaterialInterfaceFilterBlock* GetFaceNeighbor(int face, int neighborId)
  {
    return (vtkMaterialInterfaceFilterBlock*)(this->Neighbors[face][neighborId]);
  }
  //
  vtkDataArray* GetVolumeWtdAvgArray(int id)
  {
    assert(id >= 0 && id < this->NVolumeWtdAvgs);
    return this->VolumeWtdAvgArrays[id];
  }
  //
  vtkDataArray* GetMassWtdAvgArray(int id)
  {
    assert(id >= 0 && id < this->NMassWtdAvgs);
    return this->MassWtdAvgArrays[id];
  }
  //
  vtkDataArray* GetIntegratedArray(int id)
  {
    assert(id >= 0 && id < this->NToIntegrate);
    return this->IntegratedArrays[id];
  }
  //
  vtkDataArray* GetArrayToSum(int id)
  {
    assert(id >= 0 && id < this->NToSum);
    return this->ArraysToSum[id];
  }
  //
  vtkDataArray* GetMassArray() { return this->MassArray; }

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
  int* FragmentIds;
  // The original image block (worry about rectilinear grids later).
  // We have two ways of managing the memory.  We only keep the
  // image around to keep a reference.
  vtkImageData* Image;
  // We keep a pointer to the volulme fraction array.
  // We have to copy the array and manage memory ourself
  // if it is a ghost block, or we are inverting or clipping
  // the volume fraction with a half sphere.
  unsigned char* VolumeFractionArray;
  int WeHaveToDeleteTheVolumeFractionMemory;
  // arrays to integrate
  // we need to know the name, and number of components
  // during integration/resolution process
  vector<vtkDataArray*> IntegratedArrays;
  int NToIntegrate;
  vector<vtkDataArray*> VolumeWtdAvgArrays;
  int NVolumeWtdAvgs;
  vector<vtkDataArray*> MassWtdAvgArrays;
  int NMassWtdAvgs;
  vector<vtkDataArray*> ArraysToSum;
  int NToSum;
  vtkDataArray* MassArray; // do not delete
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
  double Origin[3];  // lower left xyz of this block
  int Level;
  // Store index to neiboring blocks.
  // There may be more than one neighbor because higher levels.
  std::vector<vtkMaterialInterfaceFilterBlock*> Neighbors[6];
};

//----------------------------------------------------------------------------
vtkMaterialInterfaceFilterBlock::vtkMaterialInterfaceFilterBlock()
{
  this->GhostFlag = 0;
  this->Image = nullptr;
  this->VolumeFractionArray = nullptr;
  this->WeHaveToDeleteTheVolumeFractionMemory = 0;
  this->Level = 0;
  this->CellIncrements[0] = this->CellIncrements[1] = this->CellIncrements[2] = 0;
  for (int ii = 0; ii < 6; ++ii)
  {
    this->CellExtent[ii] = 0;
    this->BaseCellExtent[ii] = 0;
  }
  this->FragmentIds = nullptr;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->NToIntegrate = 0;
  this->NVolumeWtdAvgs = 0;
  this->NMassWtdAvgs = 0;
  this->NToSum = 0;
}
//----------------------------------------------------------------------------
vtkMaterialInterfaceFilterBlock::~vtkMaterialInterfaceFilterBlock()
{
  if (this->Image)
  {
    this->Image->UnRegister(nullptr);
    this->Image = nullptr;
  }
  if (this->VolumeFractionArray && this->WeHaveToDeleteTheVolumeFractionMemory)
  { // Memory was allocated without an image.
    delete[] this->VolumeFractionArray;
  }
  this->VolumeFractionArray = nullptr;

  this->Level = 0;

  for (int ii = 0; ii < 6; ++ii)
  {
    this->CellExtent[ii] = 0;
    this->BaseCellExtent[ii] = 0;
  }
  if (this->FragmentIds != nullptr)
  {
    delete[] this->FragmentIds;
    this->FragmentIds = nullptr;
  }
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  this->NToIntegrate = 0;
  this->NVolumeWtdAvgs = 0;
  this->NMassWtdAvgs = 0;
  this->NToSum = 0;
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::InitializeVolumeFractionArray(int invertVolumeFraction,
  vtkMaterialInterfaceFilterHalfSphere* implicitFunction, vtkDataArray* volumeFractionArray)
{
  double tmp;
  unsigned char* inPtr = (unsigned char*)(volumeFractionArray->GetVoidPointer(0));

  if (implicitFunction == nullptr)
  {
    this->VolumeFractionArray = inPtr;
    this->WeHaveToDeleteTheVolumeFractionMemory = 0;
  }

  // Create a new volume fraction array.
  int ext[6];
  this->GetCellExtent(ext);
  double* origin = this->GetOrigin();
  double* spacing = this->GetSpacing();
  double bounds[6];
  // Note that we are allocating for the cell array and ext is for cells.
  this->VolumeFractionArray =
    new unsigned char[(ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1)];
  this->WeHaveToDeleteTheVolumeFractionMemory = 1;
  // Copy and modify the array.
  unsigned char* outPtr = this->VolumeFractionArray;
  for (int z = ext[4]; z <= ext[5]; ++z)
  {
    bounds[4] = (double)(z)*spacing[2] + origin[2];
    bounds[5] = bounds[4] + spacing[2];
    for (int y = ext[2]; y <= ext[3]; ++y)
    {
      bounds[2] = (double)(y)*spacing[1] + origin[1];
      bounds[3] = bounds[2] + spacing[1];
      for (int x = ext[0]; x <= ext[1]; ++x)
      {
        bounds[0] = (double)(x)*spacing[1] + origin[0];
        bounds[1] = bounds[0] + spacing[0];
        tmp = (double)(*inPtr++);
        // Invert the material if the flag is set.
        if (invertVolumeFraction)
        {
          tmp = 255.0 - tmp;
        }
        if (implicitFunction)
        {
          tmp = tmp * implicitFunction->EvaluateHalfSphereBox(bounds);
        }
        *outPtr++ = (unsigned char)(tmp);
      }
    }
  }
}

//----------------------------------------------------------------------------
// The argument list is getting long!
void vtkMaterialInterfaceFilterBlock::Initialize(int blockId, vtkImageData* image, int level,
  double globalOrigin[3], double rootSpacing[3], string& volumeFractionArrayName,
  string& massArrayName, vector<string>& volumeWtdAvgArrayNames,
  vector<string>& massWtdAvgArrayNames, vector<string>& summedArrayNames,
  vector<string>& integratedArrayNames, int invertVolumeFraction,
  vtkMaterialInterfaceFilterHalfSphere* sphere)
{
  if (this->VolumeFractionArray)
  {
    vtkGenericWarningMacro("Block already initialized !!!");
    return;
  }
  if (image == nullptr)
  {
    vtkGenericWarningMacro("No image to initialize with.");
    return;
  }

  this->BlockId = blockId;
  this->Image = image;
  this->Image->Register(nullptr);
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

  // get pointers to arrays to volume weighted average
  this->NVolumeWtdAvgs = static_cast<int>(volumeWtdAvgArrayNames.size());
  this->VolumeWtdAvgArrays.clear();
  this->VolumeWtdAvgArrays.resize(this->NVolumeWtdAvgs, nullptr);
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    this->VolumeWtdAvgArrays[i] =
      this->Image->GetCellData()->GetArray(volumeWtdAvgArrayNames[i].c_str());
    //
    assert("\nCould not find array to weighted average.\n" && this->VolumeWtdAvgArrays[i]);
  }
  // get pointers to arrays to mass weighted average
  this->NMassWtdAvgs = static_cast<int>(massWtdAvgArrayNames.size());
  this->MassWtdAvgArrays.clear();
  this->MassWtdAvgArrays.resize(this->NMassWtdAvgs, nullptr);
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    this->MassWtdAvgArrays[i] =
      this->Image->GetCellData()->GetArray(massWtdAvgArrayNames[i].c_str());
    //
    assert("\nCould not find array to weighted average.\n" && this->MassWtdAvgArrays[i]);
  }
  // get pointers to arrays to directly copy to the
  // output.ie Integrated arrays
  this->NToIntegrate = static_cast<int>(integratedArrayNames.size());
  this->IntegratedArrays.clear();
  this->IntegratedArrays.resize(this->NToIntegrate, nullptr);
  for (int i = 0; i < this->NToIntegrate; ++i)
  {
    this->IntegratedArrays[i] =
      this->Image->GetCellData()->GetArray(integratedArrayNames[i].c_str());

    assert("\nCould not find array to integrate.\n" && this->IntegratedArrays[i]);
  }
  // get pointers to arrays to sum
  this->NToSum = static_cast<int>(summedArrayNames.size());
  this->ArraysToSum.clear();
  this->ArraysToSum.resize(this->NToSum, nullptr);
  for (int i = 0; i < this->NToSum; ++i)
  {
    this->ArraysToSum[i] = this->Image->GetCellData()->GetArray(summedArrayNames[i].c_str());

    assert("\nCould not find array to sum.\n" && this->ArraysToSum[i]);
  }
  // Get the mass array, if it is provided then certain
  // calculations which depend on it will be unavailable.
  this->MassArray = nullptr;
  if (!massArrayName.empty())
  {
    this->MassArray = this->Image->GetCellData()->GetArray(massArrayName.c_str());
  }

  // Since some codes do not use convention that all blocks share an origin,
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

  // Handle the case of 2D input blocks.
  if (this->BaseCellExtent[4] > this->BaseCellExtent[5])
  {
    this->BaseCellExtent[4] = this->BaseCellExtent[5] = 0;
  }

  this->CellIncrements[0] = 1;
  // Point extent -> cell increments.  Do not add 1.
  this->CellIncrements[1] = imageExt[1] - imageExt[0];
  this->CellIncrements[2] = this->CellIncrements[1] * (imageExt[3] - imageExt[2]);

// As a sanity check, compare spacing and level.
#ifdef NDEBUG
  (void)rootSpacing; // avoid a compiler warning when assert goes away
#endif
  assert("Spacing does not look correct for AMR structure." &&
    (int)(rootSpacing[0] / this->Spacing[0] + 0.5) == (1 << (this->Level)) &&
    (int)(rootSpacing[1] / this->Spacing[1] + 0.5) == (1 << (this->Level)));

  //  2D grids do not follow the same pattern for the z axis.
  //  && (int)(rootSpacing[2] / this->Spacing[2] + 0.5) == (1<<(this->Level)) );

  // This will have to change, but I will leave it for now.
  this->HalfEdges[1][0] = this->Spacing[0] * 0.5;
  this->HalfEdges[1][1] = this->HalfEdges[1][2] = 0.0;
  this->HalfEdges[3][1] = this->Spacing[1] * 0.5;
  this->HalfEdges[3][0] = this->HalfEdges[3][2] = 0.0;
  this->HalfEdges[5][2] = this->Spacing[2] * 0.5;
  this->HalfEdges[5][0] = this->HalfEdges[5][1] = 0.0;
  for (int ii = 0; ii < 3; ++ii)
  {
    this->HalfEdges[0][ii] = -this->HalfEdges[1][ii];
    this->HalfEdges[2][ii] = -this->HalfEdges[3][ii];
    this->HalfEdges[4][ii] = -this->HalfEdges[5][ii];
  }

  // get a pointer to the volume fraction data
  // We can mdify the volume fractin array for clipping.
  vtkDataArray* volumeFractionArray =
    this->Image->GetCellData()->GetArray(volumeFractionArrayName.c_str());
  assert("Could not find volume fraction array." && volumeFractionArray);
  this->InitializeVolumeFractionArray(invertVolumeFraction, sphere, volumeFractionArray);
}

//----------------------------------------------------------------------------
// I would really like to make subclasses of blocks,
// Memory is managed differently.
// Skip much of the initialization because ghost layers produce no surface.
// Ghost layers need half edges too.
// ----
// This method is used to remove ghost layer of each block as ghost layers
// are managed by the filter itself.
// scth reader always provide one row that is shared by the next block
// which is not the case for regular Non overlapping AMR.
// ----
void vtkMaterialInterfaceFilterBlock::ComputeBaseExtent(
  int blockDims[3], unsigned char levelOfGhostLayer)
{
  if (this->GhostFlag || levelOfGhostLayer == 0)
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
  int minExt, maxExt;
  int nbLevel = static_cast<int>(levelOfGhostLayer);
  for (int ii = 0; ii < 3; ++ii)
  {
    baseDim = blockDims[ii];
    iMin = 2 * ii;
    iMax = iMin + 1;

    // This assumes that all extents are positive.  ceil is too complicated.
    minExt = this->BaseCellExtent[iMin];
    minExt = (minExt + baseDim - nbLevel) / baseDim;
    minExt = minExt * baseDim;

    maxExt = this->BaseCellExtent[iMax] + 1;
    maxExt = maxExt / baseDim;
    maxExt = maxExt * baseDim - nbLevel;

    if (minExt < maxExt)
    {
      this->BaseCellExtent[iMin] = minExt;
      this->BaseCellExtent[iMax] = maxExt;
    }
    else
    {
      cerr << "vtkMaterialInterfaceFilter.cxx:" << __LINE__
           << " Invalid Block Ghost Level information. " << (int)levelOfGhostLayer << endl;
    }
  }
}

//----------------------------------------------------------------------------
// I would really like to make subclasses of blocks,
// Memory is managed differently.
// Skip much of the initialization because ghost layers produce no surface.
// Ghost layers need half edges too.
void vtkMaterialInterfaceFilterBlock::InitializeGhostLayer(unsigned char* volFraction,
  int cellExtent[6], int level, double globalOrigin[3], double rootSpacing[3], int ownerProcessId,
  int blockId)
{
  if (this->VolumeFractionArray)
  {
    vtkGenericWarningMacro("Block already initialized !!!");
    return;
  }

  this->ProcessId = ownerProcessId;
  this->GhostFlag = 1;
  this->BlockId = blockId;

  this->Image = nullptr;
  this->Level = level;
  // Skip spacing and origin.

  int numCells = (cellExtent[1] - cellExtent[0] + 1) * (cellExtent[3] - cellExtent[2] + 1) *
    (cellExtent[5] - cellExtent[4] + 1);
  this->FragmentIds = new int[numCells];
  // Initialize fragment ids to -1 / uninitialized
  for (int ii = 0; ii < numCells; ++ii)
  {
    this->FragmentIds[ii] = -1;
  }

  // Extract what we need form the image because we do not reference it again.
  this->VolumeFractionArray = new unsigned char[numCells];
  memcpy(this->VolumeFractionArray, volFraction, numCells);

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
  this->CellIncrements[1] = cellExtent[1] - cellExtent[0] + 1;
  this->CellIncrements[2] = this->CellIncrements[1] * (cellExtent[3] - cellExtent[2] + 1);

  for (int ii = 0; ii < 3; ++ii)
  {
    this->Origin[ii] = globalOrigin[ii];
    this->Spacing[ii] = rootSpacing[ii] / (double)(1 << (this->Level));
  }

  // This will have to change, but I will leave it for now.
  this->HalfEdges[1][0] = this->Spacing[0] * 0.5;
  this->HalfEdges[1][1] = this->HalfEdges[1][2] = 0.0;
  this->HalfEdges[3][1] = this->Spacing[1] * 0.5;
  this->HalfEdges[3][0] = this->HalfEdges[3][2] = 0.0;
  this->HalfEdges[5][2] = this->Spacing[2] * 0.5;
  this->HalfEdges[5][0] = this->HalfEdges[5][1] = 0.0;
  for (int ii = 0; ii < 3; ++ii)
  {
    this->HalfEdges[0][ii] = -this->HalfEdges[1][ii];
    this->HalfEdges[2][ii] = -this->HalfEdges[3][ii];
    this->HalfEdges[4][ii] = -this->HalfEdges[5][ii];
  }
}

//----------------------------------------------------------------------------
// Flipped a coin.  This method only adds the neighbor relation one way.
// User needs to call it twice to get the backward connection.
void vtkMaterialInterfaceFilterBlock::AddNeighbor(
  vtkMaterialInterfaceFilterBlock* neighbor, int axis, int maxFlag)
{
  if (maxFlag)
  { // max neighbor
    this->Neighbors[2 * axis + 1].push_back(neighbor);
  }
  else
  { // min neighbor
    this->Neighbors[2 * axis].push_back(neighbor);
  }
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::GetPointExtent(int ext[6])
{
  ext[0] = this->CellExtent[0];
  ext[1] = this->CellExtent[1] + 1;
  ext[2] = this->CellExtent[2];
  ext[3] = this->CellExtent[3] + 1;
  ext[4] = this->CellExtent[4];
  ext[5] = this->CellExtent[5] + 1;
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::GetCellExtent(int ext[6])
{
  memcpy(ext, this->CellExtent, 6 * sizeof(int));
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::GetCellIncrements(int incs[3])
{
  incs[0] = this->CellIncrements[0];
  incs[1] = this->CellIncrements[1];
  incs[2] = this->CellIncrements[2];
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::GetBaseCellExtent(int ext[6])
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
// Returns a pointer to the minimum AMR Extent with is the first
// cell values that is not a ghost cell.
unsigned char* vtkMaterialInterfaceFilterBlock::GetBaseVolumeFractionPointer()
{
  unsigned char* ptr = this->VolumeFractionArray;

  ptr += this->CellIncrements[0] * (this->BaseCellExtent[0] - this->CellExtent[0]);
  ptr += this->CellIncrements[1] * (this->BaseCellExtent[2] - this->CellExtent[2]);
  ptr += this->CellIncrements[2] * (this->BaseCellExtent[4] - this->CellExtent[4]);

  return ptr;
}

//----------------------------------------------------------------------------
int* vtkMaterialInterfaceFilterBlock::GetBaseFragmentIdPointer()
{
  int* ptr = this->FragmentIds;

  ptr += this->CellIncrements[0] * (this->BaseCellExtent[0] - this->CellExtent[0]);
  ptr += this->CellIncrements[1] * (this->BaseCellExtent[2] - this->CellExtent[2]);
  ptr += this->CellIncrements[2] * (this->BaseCellExtent[4] - this->CellExtent[4]);

  return ptr;
}
//----------------------------------------------------------------------------
// returns the index from the lower left cell
// that gets you to the first non-ghost cell
int vtkMaterialInterfaceFilterBlock::GetBaseFlatIndex()
{
  int idx = 0;
  idx += this->CellIncrements[0] * (this->BaseCellExtent[0] - this->CellExtent[0]);
  idx += this->CellIncrements[1] * (this->BaseCellExtent[2] - this->CellExtent[2]);
  idx += this->CellIncrements[2] * (this->BaseCellExtent[4] - this->CellExtent[4]);
  return idx;
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterBlock::ExtractExtent(unsigned char* buf, int ext[6])
{
  // Initialize the buffer to 0.
  memset(buf, 0, (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1));

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
  ptr2 = volumeFractionPointer + inc0 * (ext[0] - cellExtent[0]) + inc1 * (ext[2] - cellExtent[2]) +
    inc2 * (ext[4] - cellExtent[4]);
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
class vtkMaterialInterfaceLevel
{
public:
  vtkMaterialInterfaceLevel();
  ~vtkMaterialInterfaceLevel();

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
  int AddBlock(vtkMaterialInterfaceFilterBlock* block);

  // Description:
  // Get the block at this grid index.
  vtkMaterialInterfaceFilterBlock* GetBlock(int xIdx, int yIdx, int zIdx);

  // Description:
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

  vtkMaterialInterfaceFilterBlock** Grid;
};

//----------------------------------------------------------------------------
vtkMaterialInterfaceLevel::vtkMaterialInterfaceLevel()
{
  this->Level = 0;
  for (int ii = 0; ii < 6; ++ii)
  {
    this->GridExtent[ii] = 0;
  }
  this->BlockDimensions[0] = 0;
  this->BlockDimensions[1] = 0;
  this->BlockDimensions[2] = 0;
  this->Grid = nullptr;
}
//----------------------------------------------------------------------------
vtkMaterialInterfaceLevel::~vtkMaterialInterfaceLevel()
{
  this->Level = 0;

  this->BlockDimensions[0] = 0;
  this->BlockDimensions[1] = 0;
  this->BlockDimensions[2] = 0;
  if (this->Grid)
  {
    int num = (this->GridExtent[1] - this->GridExtent[0] + 1) *
      (this->GridExtent[3] - this->GridExtent[2] + 1) *
      (this->GridExtent[5] - this->GridExtent[4] + 1);
    for (int ii = 0; ii < num; ++ii)
    {
      if (this->Grid[ii])
      {
        // We are not the only container for blocks yet.
        // delete this->Grid[ii];
        this->Grid[ii] = nullptr;
      }
    }
    delete[] this->Grid;
  }
  for (int ii = 0; ii < 6; ++ii)
  {
    this->GridExtent[ii] = 0;
  }
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceLevel::GetBlockDimensions(int dims[3])
{
  dims[0] = this->BlockDimensions[0];
  dims[1] = this->BlockDimensions[1];
  dims[2] = this->BlockDimensions[2];
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceLevel::GetGridExtent(int ext[6])
{
  for (int ii = 0; ii < 6; ++ii)
  {
    ext[ii] = this->GridExtent[ii];
  }
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceLevel::Initialize(int gridExtent[6], int level)
{
  if (this->Grid)
  {
    vtkGenericWarningMacro("Level already initialized.");
    return;
  }
  // Special case for a level with no blocks in it.
  if (gridExtent[0] > gridExtent[1] || gridExtent[2] > gridExtent[3] ||
    gridExtent[4] > gridExtent[5])
  { // This should do the trick.
    gridExtent[0] = gridExtent[1] = gridExtent[2] = gridExtent[3] = gridExtent[4] = gridExtent[5] =
      0;
  }

  this->Level = level;
  memcpy(this->GridExtent, gridExtent, 6 * sizeof(int));
  int num = (this->GridExtent[1] - this->GridExtent[0] + 1) *
    (this->GridExtent[3] - this->GridExtent[2] + 1) *
    (this->GridExtent[5] - this->GridExtent[4] + 1);
  this->Grid = new vtkMaterialInterfaceFilterBlock*[num];
  memset(this->Grid, 0, num * sizeof(vtkMaterialInterfaceFilterBlock*));
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceLevel::SetStandardBlockDimensions(int dims[3])
{
  this->BlockDimensions[0] = dims[0];
  this->BlockDimensions[1] = dims[1];
  this->BlockDimensions[2] = dims[2];
}

//----------------------------------------------------------------------------
int vtkMaterialInterfaceLevel::AddBlock(vtkMaterialInterfaceFilterBlock* block)
{
  int xIdx, yIdx, zIdx;

  // First make sure the level is correct.
  // We assume that the block dimensions are correct.
  if (block->GetLevel() != this->Level)
  {
    vtkGenericWarningMacro("Wrong level.");
    return VTK_ERROR;
  }
  const int* ext;
  ext = block->GetBaseCellExtent();

  if (ext[0] < 0 || ext[2] < 0 || ext[4] < 0)
  {
    vtkGenericWarningMacro("I did not code this for negative extents.");
  }

  // Compute the extent from the left most cell value.
  xIdx = ext[0] / this->BlockDimensions[0];
  yIdx = ext[2] / this->BlockDimensions[1];
  zIdx = ext[4] / this->BlockDimensions[2];

  if (xIdx < this->GridExtent[0] || xIdx > this->GridExtent[1] || yIdx < this->GridExtent[2] ||
    yIdx > this->GridExtent[3] || zIdx < this->GridExtent[4] || zIdx > this->GridExtent[5])
  {
    vtkGenericWarningMacro("Block index out of grid.");
    return VTK_ERROR;
  }

  xIdx -= this->GridExtent[0];
  yIdx -= this->GridExtent[2];
  zIdx -= this->GridExtent[4];
  int idx = xIdx +
    (yIdx + (zIdx * (this->GridExtent[3] - this->GridExtent[2] + 1))) *
      (this->GridExtent[1] - this->GridExtent[0] + 1);

  if (this->Grid[idx])
  {
    vtkGenericWarningMacro("Overwriting block in grid");
  }
  this->Grid[idx] = block;

  return VTK_OK;
}

//----------------------------------------------------------------------------
vtkMaterialInterfaceFilterBlock* vtkMaterialInterfaceLevel::GetBlock(int xIdx, int yIdx, int zIdx)
{
  if (xIdx < this->GridExtent[0] || xIdx > this->GridExtent[1] || yIdx < this->GridExtent[2] ||
    yIdx > this->GridExtent[3] || zIdx < this->GridExtent[4] || zIdx > this->GridExtent[5])
  {
    return nullptr;
  }
  xIdx -= this->GridExtent[0];
  yIdx -= this->GridExtent[2];
  zIdx -= this->GridExtent[4];
  int yInc = (this->GridExtent[1] - this->GridExtent[0] + 1);
  int zInc = yInc * (this->GridExtent[3] - this->GridExtent[2] + 1);
  return this->Grid[xIdx + yIdx * yInc + zIdx * zInc];
}

//============================================================================

//----------------------------------------------------------------------------
// A class to help traverse pointers acrosss block boundaries.
// I am keeping this class to a minimum because many are kept on the stack
// during recursive connect algorithm.
class vtkMaterialInterfaceFilterIterator
{
public:
  vtkMaterialInterfaceFilterIterator() { this->Initialize(); }
  ~vtkMaterialInterfaceFilterIterator() { this->Initialize(); }
  void Initialize();

  vtkMaterialInterfaceFilterBlock* Block;
  unsigned char* VolumeFractionPointer; // pointer to a specific voxel's data
  int* FragmentIdPointer;               // pointer to a specific voxel
  int Index[3];                         // relative to the data set's index space
  int FlatIndex;                        // relative to this block's index space
};
void vtkMaterialInterfaceFilterIterator::Initialize()
{
  this->Block = nullptr;
  this->VolumeFractionPointer = nullptr;
  this->FragmentIdPointer = nullptr;
  this->Index[0] = this->Index[1] = this->Index[2] = 0;
  this->FlatIndex = 0;
}

//============================================================================

//----------------------------------------------------------------------------
// A simple first in last out ring container to hold the seeds for the
// breadth first search.
// I am going to have the container allocate and delete its own iterators.
// It will simplify the converation from depth first to breadth first.
class vtkMaterialInterfaceFilterRingBuffer
{
public:
  vtkMaterialInterfaceFilterRingBuffer();
  ~vtkMaterialInterfaceFilterRingBuffer();

  void Push(vtkMaterialInterfaceFilterIterator* iterator);
  int Pop(vtkMaterialInterfaceFilterIterator* iterator);

  long GetSize() { return this->Size; }

private:
  vtkMaterialInterfaceFilterIterator* Ring;
  vtkMaterialInterfaceFilterIterator* End;
  long RingLength;
  // The first and last iterator added.
  vtkMaterialInterfaceFilterIterator* First;
  vtkMaterialInterfaceFilterIterator* Next;
  // I could do without this size, but it does not cost much.
  long Size;

  void GrowRing();
};

//----------------------------------------------------------------------------
vtkMaterialInterfaceFilterRingBuffer::vtkMaterialInterfaceFilterRingBuffer()
{
  int initialSize = 2000;
  this->Ring = new vtkMaterialInterfaceFilterIterator[initialSize];
  this->RingLength = initialSize;
  this->End = this->Ring + this->RingLength;
  this->First = nullptr;
  this->Next = this->Ring;
  this->Size = 0;
}

//----------------------------------------------------------------------------
vtkMaterialInterfaceFilterRingBuffer::~vtkMaterialInterfaceFilterRingBuffer()
{
  delete[] this->Ring;
  this->End = nullptr;
  this->RingLength = 0;
  this->Next = this->First = nullptr;
  this->Size = 0;
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterRingBuffer::GrowRing()
{
  // Allocate a new ring.
  vtkMaterialInterfaceFilterIterator* newRing;
  int newRingLength = this->RingLength * 2;
  newRing = new vtkMaterialInterfaceFilterIterator[newRingLength * 2];

  // Copy items into the new ring.
  int count = this->Size;
  vtkMaterialInterfaceFilterIterator* ptr1 = this->First;
  vtkMaterialInterfaceFilterIterator* ptr2 = newRing;
  while (count > 0)
  {
    *ptr2++ = *ptr1++;
    if (ptr1 == this->End)
    {
      ptr1 = this->Ring;
    }
    --count;
  }

  // cerr << "Grow ring buffer: " << newRingLength << endl;

  // Replace the ring.
  // Size remains the same.
  delete[] this->Ring;
  this->Ring = newRing;
  this->End = newRing + newRingLength;
  this->RingLength = newRingLength;
  this->First = newRing;
  this->Next = newRing + this->Size;
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilterRingBuffer::Push(vtkMaterialInterfaceFilterIterator* item)
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
  // End of the linear buffer moves back to the beginning.
  // This makes it a ring.
  if (this->Next == this->End)
  {
    this->Next = this->Ring;
  }

  ++this->Size;
}

//----------------------------------------------------------------------------
int vtkMaterialInterfaceFilterRingBuffer::Pop(vtkMaterialInterfaceFilterIterator* item)
{
  if (this->Size == 0)
  {
    return 0;
  }

  *item = *(this->First);
  // this->First->Initialize();
  ++this->First;
  --this->Size;

  // End of the linear buffer moves back to the beginning.
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
vtkMaterialInterfaceFilter::vtkMaterialInterfaceFilter()
{
  this->Controller = vtkMultiProcessController::GetGlobalController();

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->InitializeBlocksTimer = vtkSmartPointer<vtkTimerLog>::New();
  this->ShareGhostBlocksTimer = vtkSmartPointer<vtkTimerLog>::New();
  this->NumberOfBlocks = 0;
  this->NumberOfGhostBlocks = 0;
  this->ProcessBlocksTimer = vtkSmartPointer<vtkTimerLog>::New();
  this->ResolveEquivalencesTimer = vtkSmartPointer<vtkTimerLog>::New();
#endif

#ifdef vtkMaterialInterfaceFilterDEBUG
  int myProcId = this->Controller->GetLocalProcessId();
  this->MyPid = WritePidFile(this->Controller->GetCommunicator(), "mif.pid");
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment entering vtkMaterialInterfaceFilter is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

// Pipeline
// 0 output multi-block -> multi-piece -> polydata
// 1 multiblock -> polydata, proc 0 fills with attributes, all others fill with empty polydata
// 2 multiblock -> polydata, proc 0 builds representation of OBBs, all others empty
#ifdef vtkMaterialInterfaceFilterDEBUG
  this->SetNumberOfOutputPorts(3);
#else
  this->SetNumberOfOutputPorts(2);
#endif

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkMaterialInterfaceFilter::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);

  // pv interface for material fraction
  this->MaterialArraySelection = vtkDataArraySelection::New();
  this->MaterialArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  // pv interface for mass array
  this->MassArraySelection = vtkDataArraySelection::New();
  this->MassArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  // pv interface for volume weighted averaged arrays
  this->VolumeWtdAvgArraySelection = vtkDataArraySelection::New();
  this->VolumeWtdAvgArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  // pv interface for mass weighted averaged arrays
  this->MassWtdAvgArraySelection = vtkDataArraySelection::New();
  this->MassWtdAvgArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);
  // pv interface for summed arrays
  this->SummationArraySelection = vtkDataArraySelection::New();
  this->SummationArraySelection->AddObserver(vtkCommand::ModifiedEvent, this->SelectionObserver);

  this->OutputBaseName = nullptr;
  this->WriteGeometryOutput = false;
  this->WriteStatisticsOutput = false;

  this->NumberOfInputBlocks = 0;
  this->InputBlocks = nullptr;
  this->GlobalOrigin[0] = this->GlobalOrigin[1] = this->GlobalOrigin[2] = 0.0;
  this->RootSpacing[0] = this->RootSpacing[1] = this->RootSpacing[2] = 1.0;

  this->FragmentId = 0;
  this->FragmentVolume = 0.0;
  this->FragmentVolumes = nullptr;
  this->FragmentMoment.resize(4, 0.0);
  this->FragmentMoments = nullptr;
  this->FragmentAABBCenters = nullptr;
  this->FragmentOBBs = nullptr;
  this->FragmentSplitGeometry = nullptr;

  // Keep depth of crater along clip plane normal.
  this->ClipDepthMax = 0.0;
  this->ClipDepthMin = VTK_FLOAT_MAX;
  this->ClipDepthMaximums = nullptr;
  this->ClipDepthMinimums = nullptr;

  this->EquivalenceSet = new vtkMaterialInterfaceEquivalenceSet;
  this->LocalToGlobalOffsets = nullptr;
  this->TotalNumberOfRawFragments = 0;
  this->NumberOfResolvedFragments = 0;
  this->ResolvedFragmentCount = 0;
  this->MaterialId = 0;

  this->ResolvedFragments = nullptr;
  this->ResolvedFragmentCenters = nullptr;
  this->ResolvedFragmentOBBs = nullptr;

  this->FaceNeighbors = new vtkMaterialInterfaceFilterIterator[32];

  this->CurrentFragmentMesh = nullptr;

  this->NVolumeWtdAvgs = 0;
  this->NToSum = 0;
  this->ComputeMoments = false;
  this->ComputeOBB = false;

  this->MaterialFractionThreshold = 0.5;
  this->scaledMaterialFractionThreshold = 127.5;
  this->UpperLoadingBound = 1000000;

  this->Progress = 0.0;
  this->ProgressMaterialInc = 0.0;
  this->ProgressBlockInc = 0.0;
  this->ProgressResolutionInc = 0.0;

  // Crater extraction variables.
  this->ClipFunction = nullptr;
  this->ClipCenter[0] = 0.0;
  this->ClipCenter[1] = 0.0;
  this->ClipCenter[2] = 0.0;
  this->ClipWithSphere = 0;
  this->ClipRadius = 1.0;
  this->ClipWithPlane = 0;
  this->ClipPlaneVector[0] = 0.0;
  this->ClipPlaneVector[1] = 0.0;
  this->ClipPlaneVector[2] = 1.0;

  // Variable that will invert a material.
  this->InvertVolumeFraction = 0;

  // 1 Layer of ghost cell by block by default
  this->BlockGhostLevel = 1;
}

//----------------------------------------------------------------------------
vtkMaterialInterfaceFilter::~vtkMaterialInterfaceFilter()
{
  this->DeleteAllBlocks();
  this->Controller = nullptr;
  this->GlobalOrigin[0] = this->GlobalOrigin[1] = this->GlobalOrigin[2] = 0.0;
  this->RootSpacing[0] = this->RootSpacing[1] = this->RootSpacing[2] = 1.0;

  this->FragmentId = 0;
  this->FragmentVolume = 0.0;
  this->ClipDepthMax = 0.0;
  this->ClipDepthMin = VTK_FLOAT_MAX;

  this->SetClipFunction(nullptr);

  CheckAndReleaseVtkPointer(this->ClipDepthMaximums);
  CheckAndReleaseVtkPointer(this->ClipDepthMinimums);
  CheckAndReleaseVtkPointer(this->FragmentVolumes);
  CheckAndReleaseVtkPointer(this->FragmentMoments);
  CheckAndReleaseVtkPointer(this->FragmentAABBCenters);
  CheckAndReleaseVtkPointer(this->FragmentOBBs);
  CheckAndReleaseVtkPointer(this->FragmentSplitGeometry);
  ClearVectorOfVtkPointers(this->FragmentVolumeWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentMassWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentSums);

  delete this->EquivalenceSet;
  this->EquivalenceSet = nullptr;

  delete[] this->FaceNeighbors;
  this->FaceNeighbors = nullptr;

  // clean up PV interface
  this->MaterialArraySelection->RemoveObserver(this->SelectionObserver);
  this->MaterialArraySelection->Delete();
  this->MaterialArraySelection = nullptr;
  //
  this->MassArraySelection->RemoveObserver(this->SelectionObserver);
  this->MassArraySelection->Delete();
  this->MassArraySelection = nullptr;
  //
  this->VolumeWtdAvgArraySelection->RemoveObserver(this->SelectionObserver);
  this->VolumeWtdAvgArraySelection->Delete();
  this->VolumeWtdAvgArraySelection = nullptr;
  //
  this->MassWtdAvgArraySelection->RemoveObserver(this->SelectionObserver);
  this->MassWtdAvgArraySelection->Delete();
  this->MassWtdAvgArraySelection = nullptr;
  //
  this->SummationArraySelection->RemoveObserver(this->SelectionObserver);
  this->SummationArraySelection->Delete();
  this->SummationArraySelection = nullptr;

  this->SelectionObserver->Delete();

  if (this->OutputBaseName != nullptr)
  {
    delete[] this->OutputBaseName;
  }
  this->Progress = 0.0;
  this->ProgressMaterialInc = 0.0;
  this->ProgressBlockInc = 0.0;
  this->ProgressResolutionInc = 0.0;
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
vtkMTimeType vtkMaterialInterfaceFilter::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  // There are two outputs,
  // 0: multi-block contains fragments
  // 1: polydata contains center of mass points with attributes
  // 2: polydata contains OBB.
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    case 1:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
#ifdef vtkMaterialInterfaceFilterDEBUG
    case 2:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
#endif
    default:
      assert(0 && "Invalid output port.");
      break;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Called to delete all the block structures.
void vtkMaterialInterfaceFilter::DeleteAllBlocks()
{
  if (this->NumberOfInputBlocks == 0)
  {
    return;
  }

  // Ghost blocks
  int num = static_cast<int>(this->GhostBlocks.size());
  int ii;
  vtkMaterialInterfaceFilterBlock* block;
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
      this->InputBlocks[ii] = nullptr;
    }
  }
  if (this->InputBlocks)
  {
    delete[] this->InputBlocks;
    this->InputBlocks = nullptr;
  }
  this->NumberOfInputBlocks = 0;

  // levels
  int level, numLevels;
  numLevels = static_cast<int>(this->Levels.size());
  for (level = 0; level < numLevels; ++level)
  {
    if (this->Levels[level])
    {
      delete this->Levels[level];
      this->Levels[level] = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
// Initialize blocks from multi block input.
int vtkMaterialInterfaceFilter::InitializeBlocks(vtkNonOverlappingAMR* input,
  string& materialFractionArrayName, string& massArrayName, vector<string>& volumeWtdAvgArrayNames,
  vector<string>& massWtdAvgArrayNames, vector<string>& summedArrayNames,
  vector<string>& integratedArrayNames)
{
  int level;
  int numLevels = input->GetNumberOfLevels();
  vtkMaterialInterfaceFilterBlock* block;
  int myProc = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  vtkMaterialInterfaceFilterHalfSphere* sphere = nullptr;

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->InitializeBlocksTimer->StartTimer();
#endif

  // leaving this logic alone rather than moving it into the
  // this->ClipFunction conditional because I don't know enough of the class to
  // know how important the Clip* ivars are ( Robert Maynard, dec 1 2010).
  if (this->ClipWithPlane || this->ClipWithSphere)
  {
    sphere = new vtkMaterialInterfaceFilterHalfSphere;
    // Set up the implicit function for clipping.
    sphere->Center[0] = this->ClipCenter[0];
    sphere->Center[1] = this->ClipCenter[1];
    sphere->Center[2] = this->ClipCenter[2];
    sphere->ClipWithSphere = this->ClipWithSphere;
    sphere->SphereRadius = this->ClipRadius;
    sphere->ClipWithPlane = this->ClipWithPlane;
    sphere->PlaneNormal[0] = this->ClipPlaneVector[0];
    sphere->PlaneNormal[1] = this->ClipPlaneVector[1];
    sphere->PlaneNormal[2] = this->ClipPlaneVector[2];
    vtkMath::Normalize(sphere->PlaneNormal);
  }

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

  this->InputBlocks = new vtkMaterialInterfaceFilterBlock*[this->NumberOfInputBlocks];
  // Initialize to nullptr.
  for (int blockId = 0; blockId < this->NumberOfInputBlocks; ++blockId)
  {
    this->InputBlocks[blockId] = nullptr;
  }

  // Initialize each block with the input image
  // and global index coordinate system.
  int blockIndex = -1;
  this->Levels.resize(numLevels);
  for (level = 0; level < numLevels; ++level)
  {
    this->Levels[level] = new vtkMaterialInterfaceLevel;

    int cumulativeExt[6];
    cumulativeExt[0] = cumulativeExt[2] = cumulativeExt[4] = VTK_INT_MAX;
    cumulativeExt[1] = cumulativeExt[3] = cumulativeExt[5] = -VTK_INT_MAX;

    int numBlocks = input->GetNumberOfDataSets(level);
    for (int levelBlockId = 0; levelBlockId < numBlocks; ++levelBlockId)
    {
      //      vtkAMRBox box;
      //      vtkImageData* image = input->GetDataSet(level,levelBlockId,box);
      vtkImageData* image = input->GetDataSet(level, levelBlockId);
      // TODO: We need to check the CELL_DATA and the correct volume fraction array.

      if (image)
      {
        block = this->InputBlocks[++blockIndex] = new vtkMaterialInterfaceFilterBlock;
        // Do we really need the block to know its id?
        // We use it to find neighbors.  We should save pointers
        // directly in neighbor array. We also use it for debugging.
        block->Initialize(blockIndex, image, level, this->GlobalOrigin, this->RootSpacing,
          materialFractionArrayName, massArrayName, volumeWtdAvgArrayNames, massWtdAvgArrayNames,
          summedArrayNames, integratedArrayNames, this->InvertVolumeFraction, sphere);
        // For debugging:
        block->LevelBlockId = levelBlockId;

        // Collect information about the blocks in this level.
        const int* ext;
        ext = block->GetBaseCellExtent();
        // We need the cumulative extent to determine the grid extent.
        if (cumulativeExt[0] > ext[0])
        {
          cumulativeExt[0] = ext[0];
        }
        if (cumulativeExt[1] < ext[1])
        {
          cumulativeExt[1] = ext[1];
        }
        if (cumulativeExt[2] > ext[2])
        {
          cumulativeExt[2] = ext[2];
        }
        if (cumulativeExt[3] < ext[3])
        {
          cumulativeExt[3] = ext[3];
        }
        if (cumulativeExt[4] > ext[4])
        {
          cumulativeExt[4] = ext[4];
        }
        if (cumulativeExt[5] < ext[5])
        {
          cumulativeExt[5] = ext[5];
        }
      }
    }

    // Expand the grid extent by 1 in all directions to accommodate ghost blocks.
    // We might have a problem with level 0 here since blockDims is not global yet.
    cumulativeExt[0] = cumulativeExt[0] / this->StandardBlockDimensions[0];
    cumulativeExt[1] = cumulativeExt[1] / this->StandardBlockDimensions[0];
    cumulativeExt[2] = cumulativeExt[2] / this->StandardBlockDimensions[1];
    cumulativeExt[3] = cumulativeExt[3] / this->StandardBlockDimensions[1];
    cumulativeExt[4] = cumulativeExt[4] / this->StandardBlockDimensions[2];
    cumulativeExt[5] = cumulativeExt[5] / this->StandardBlockDimensions[2];

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
        if (cumulativeExt[0] > tmp[0])
        {
          cumulativeExt[0] = tmp[0];
        }
        if (cumulativeExt[1] < tmp[1])
        {
          cumulativeExt[1] = tmp[1];
        }
        if (cumulativeExt[2] > tmp[2])
        {
          cumulativeExt[2] = tmp[2];
        }
        if (cumulativeExt[3] < tmp[3])
        {
          cumulativeExt[3] = tmp[3];
        }
        if (cumulativeExt[4] > tmp[4])
        {
          cumulativeExt[4] = tmp[4];
        }
        if (cumulativeExt[5] < tmp[5])
        {
          cumulativeExt[5] = tmp[5];
        }
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
  delete sphere;
  sphere = nullptr;

  // Now add all the blocks to the level structures.
  int ii;
  for (ii = 0; ii < this->NumberOfInputBlocks; ++ii)
  {
    block = this->InputBlocks[ii];
    this->AddBlock(block, this->GetBlockGhostLevel());
  }

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->InitializeBlocksTimer->StopTimer();
#endif

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->ShareGhostBlocksTimer->StartTimer();
#endif

// cerr << "start ghost blocks\n" << endl;

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->NumberOfBlocks = this->NumberOfInputBlocks;
#endif

  // Broadcast all of the block meta data to all processes.
  // Setup ghost layer blocks.
  // Remove this until the local version is working again.....
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->ShareGhostBlocks();
  }

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->ShareGhostBlocksTimer->StopTimer();
#endif

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::CheckLevelsForNeighbors(vtkMaterialInterfaceFilterBlock* block)
{
  //   int exts[6] = {64, 79,112, 127, 96,111};
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
  std::vector<vtkMaterialInterfaceFilterBlock*> neighbors;
  vtkMaterialInterfaceFilterBlock* neighbor;
  int blockIndex[3];

  // Extract the index from the block extent.
  const int* ext;
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
    if (ext[2 * axis] == blockIndex[axis] * this->StandardBlockDimensions[axis])
    { // I had trouble with ghost blocks that did not span the
      // entire standard block.  They had inappropriate neighbors.
      this->FindFaceNeighbors(block->GetLevel(), blockIndex, axis, 0, &neighbors);
      for (unsigned int ii = 0; ii < neighbors.size(); ++ii)
      {
        neighbor = neighbors[ii];
        block->AddNeighbor(neighbor, axis, 0);
        neighbor->AddNeighbor(block, axis, 1);
      }
    }

    if (ext[2 * axis + 1] == ((blockIndex[axis] + 1) * this->StandardBlockDimensions[axis]) - 1)
    { // I had trouble with ghost blocks that did not span the
      // entire standard block.  They had inappropriate neighbors.
      this->FindFaceNeighbors(block->GetLevel(), blockIndex, axis, 1, &neighbors);
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
int vtkMaterialInterfaceFilter::FindFaceNeighbors(unsigned int blockLevel, int blockIndex[3],
  int faceAxis, int faceMaxFlag, std::vector<vtkMaterialInterfaceFilterBlock*>* result)
{
  int retVal = 0;
  vtkMaterialInterfaceFilterBlock* neighbor;
  int idx[3];
  int tmp[3];
  int levelDifference;
  int p2;
  int axis1 = (faceAxis + 1) % 3;
  int axis2 = (faceAxis + 2) % 3;
  const int* ext;

  // Stuff to compare extent with standard block to make sure neighbors
  // actually touch.  Ghost cells can be a part of a standard block.
  int neighborExtIdx = 2 * faceAxis;
  if (!faceMaxFlag)
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
        if (!faceMaxFlag)
        { // min face.  Neighbor one to "left".
          tmp[faceAxis] -= 1;
          // But boundary voxel is to the "right".
          boundaryVoxelIdx = ((tmp[faceAxis] + 1) * this->StandardBlockDimensions[faceAxis]) - 1;
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
            if (!neighbor->GetGhostFlag())
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
      levelDifference = level - blockLevel;
      p2 = 1 << levelDifference;
      // Point index of face for easier level conversion.
      idx[0] = idx[0] << levelDifference;
      idx[1] = idx[1] << levelDifference;
      idx[2] = idx[2] << levelDifference;
      if (!faceMaxFlag)
      { // min face.  Neighbor one to "left".
        idx[faceAxis] -= 1;
        // But boundary voxel is to the "right".
        boundaryVoxelIdx = ((idx[faceAxis] + 1) * this->StandardBlockDimensions[faceAxis]) - 1;
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
              if (!neighbor->GetGhostFlag())
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
int vtkMaterialInterfaceFilter::HasNeighbor(
  unsigned int blockLevel, int blockIndex[3], int neighborDirection[3])
{
  vtkMaterialInterfaceFilterBlock* neighbor;
  int idx[3] = { 0, 0, 0 };
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
      { // Neighbor primitive direction is on grid boundary.
        neighbor = this->Levels[level]->GetBlock(idx[0], idx[1], idx[2]);
        if (neighbor && !neighbor->GetGhostFlag())
        { // We found one.  That's all we need.
          return 1;
        }
      }
    }
    else
    { // Neighbor block is smaller than reference block.
      // All faces will be on boundary.
      // !!! We have to loop over all axes whose direction component is 0.
      // Convert index to working level.
      levelDifference = level - blockLevel;
      int mins[3] = { 0, 0, 0 };
      int maxs[3] = { 0, 0, 0 };
      int ix = 0, iy = 0, iz = 0;

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
            mins[ii] = maxs[ii] = (blockIndex[ii] + 1) << levelDifference;
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
            if (neighbor && !neighbor->GetGhostFlag())
            { // We found one.  That's all we need.
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
int vtkMaterialInterfaceFilter::GetNumberOfLocalBlocks(vtkNonOverlappingAMR* hbds)
{
  vtkCompositeDataIterator* it = hbds->NewIterator();
  it->InitTraversal();
  it->SkipEmptyNodesOn();
  int nLocalBlocks = 0;
  while (!it->IsDoneWithTraversal())
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
// GlobalOrigin is chosen so that there are no negative extents and
// base extents (without overlap/ghost buffer) lie on grid
// (i.e.) the min base extent must be a multiple of the standardBlockDimesions.
void vtkMaterialInterfaceFilter::ComputeOriginAndRootSpacing(vtkNonOverlappingAMR* input)
{
  // extract information which the reader has exported to
  // the root node of the box hierarchy. All procs in the filter have
  // the root thus no comm needed.
  vtkFieldData* inputFd = input->GetFieldData();
  vtkDoubleArray* globalBoundsDa = dynamic_cast<vtkDoubleArray*>(inputFd->GetArray("GlobalBounds"));
  vtkIntArray* standardBoxSizeIa = dynamic_cast<vtkIntArray*>(inputFd->GetArray("GlobalBoxSize"));
  vtkIntArray* minLevelIa = dynamic_cast<vtkIntArray*>(inputFd->GetArray("MinLevel"));
  vtkDoubleArray* minLevelSpacingDa =
    dynamic_cast<vtkDoubleArray*>(inputFd->GetArray("MinLevelSpacing"));

  // Update ghostLayer if available
  if (vtkUnsignedCharArray* blockGhostLayer =
        vtkUnsignedCharArray::SafeDownCast(inputFd->GetArray("GhostLayer")))
  {
    this->SetBlockGhostLevel(blockGhostLayer->GetValue(0));
  }
  else if (globalBoundsDa == nullptr)
  {
    // Update ghostLayer if available
    // CAUTION: as no global fields were provided we assume that it is not the
    // spcth reader and therefore we don't have any ghost layer.
    this->SetBlockGhostLevel(0);
  }

  // if these are not present then the dataset
  // is malformed.
  if (globalBoundsDa == nullptr || standardBoxSizeIa == nullptr || minLevelIa == nullptr ||
    minLevelSpacingDa == nullptr)
  {
    // Need to figure out this information
    int nbProcs = this->Controller->GetNumberOfProcesses();

    // - GlobalOrigin
    double localBounds[6];
    double* receivedBounds = new double[nbProcs * 6];
    input->GetBounds(localBounds);
    this->Controller->AllGather(localBounds, receivedBounds, 6);
    for (int i = 0; i < 3; ++i)
    {
      double globalValue = VTK_DOUBLE_MAX;
      double localValue;
      for (int j = 0; j < nbProcs; ++j)
      {
        localValue = receivedBounds[6 * j + i * 2];
        globalValue = (globalValue < localValue) ? globalValue : localValue;
      }

      // Update GlobalOrigin
      this->GlobalOrigin[i] = globalValue;
    }
    delete[] receivedBounds;

    // - The value of the lower level inside the AMR dataset
    unsigned int nbLevels = input->GetNumberOfLevels();
    unsigned int globalNbLevels = 0;
    this->Controller->Reduce(&nbLevels, &globalNbLevels, 1, vtkCommunicator::MIN_OP, 0);
    this->Controller->Broadcast(&globalNbLevels, 1, 0);
    assert("Invalid number of levels" && globalNbLevels > 0);
    unsigned int minLevel = globalNbLevels - 1;
    int coarsen = 1 << minLevel;

    // - Global Box Size (Each block MUST have the same dimension
    //   -> This is the dimension of a block in the higher level)
    // - The spacing for that lower AMR level
    int localBlockSize[3] = { -1, -1, -1 };
    int* receivedBlockSize = new int[nbProcs * 3];
    double localSpacing[3] = { 0, 0, 0 };
    double* receivedSpacing = new double[nbProcs * 3];
    unsigned int dsIdxSize = input->GetNumberOfDataSets(minLevel);
    for (unsigned int dsIdx = 0; dsIdx < dsIdxSize; ++dsIdx)
    {
      vtkUniformGrid* grid = input->GetDataSet(minLevel, dsIdx);
      if (grid)
      {
        grid->GetDimensions(localBlockSize);
        grid->GetSpacing(localSpacing);
      }
    }
    this->Controller->AllGather(localBlockSize, receivedBlockSize, 3);
    this->Controller->AllGather(localSpacing, receivedSpacing, 3);
    for (int i = 0; i < 3; ++i)
    {
      double globalSpacing = 0;
      double globalValue = -1;
      double localValue;
      for (int j = 0; j < nbProcs; ++j)
      {
        // Spacing
        globalSpacing =
          (globalSpacing < receivedSpacing[3 * j + i]) ? receivedSpacing[3 * j + i] : globalSpacing;

        // Block size
        localValue = receivedBlockSize[3 * j + i];
        globalValue = (globalValue == -1 || globalValue == localValue) ? localValue : globalValue;
        assert("Block size must be the same across all processes" && globalValue == localValue);
      }

      // Update block dimension
      this->StandardBlockDimensions[i] =
        (globalValue > 1 + this->BlockGhostLevel) ? (globalValue - 1 - this->BlockGhostLevel) : 1;

      // Extract root spacing (Level 0)
      this->RootSpacing[i] = globalSpacing * coarsen;
    }
    delete[] receivedBlockSize;
    delete[] receivedSpacing;
  }
  else
  {
    // Find out the coarsen for level 0
    int coarsen = 1 << minLevelIa->GetValue(0);

    for (int i = 0; i < 3; ++i)
    {
      // Extract block dimension + Fix block dimension in case of 2d case
      this->StandardBlockDimensions[i] =
        standardBoxSizeIa->GetValue(i) - 1 - (int)this->BlockGhostLevel;
      this->StandardBlockDimensions[i] =
        (this->StandardBlockDimensions[i] < 1) ? 1 : this->StandardBlockDimensions[i];

      // Extract global origin
      this->GlobalOrigin[i] = globalBoundsDa->GetValue(i * 2);

      // Extract root spacing (Level 0)
      this->RootSpacing[i] = minLevelSpacingDa->GetValue(i) * coarsen;
    }
  }

  // TODO the following code doesn't work on small(wrt blocks) datasets
  // it looks like 3x3x3 is the smallest that will work...

  // TODO if I understod what is going on below better I could
  // probably get the requisite information from the reader.
  // From above we will keep globalBounds, RootSpacing and StandardBlockDimensions,
  // and recompute the rest below so as not to disturb the process.

  // TODO why is globalBounds insufficient for the globalOrigin??
  // in the examples I have looked at they come out to be the same.
  // TODO do we have a case specific where they differ??
  // TODO why lowestOrigin is necessarily from the block closest to
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
// GlobalOrigin is chosen so that there are no negative extents and
// base extents (without overlap/ghost buffer) lie on grid
// (i.e.) the min base extent must be a multiple of the standardBlockDimesions.
int vtkMaterialInterfaceFilter::ComputeOriginAndRootSpacingOld(vtkNonOverlappingAMR* input)
{
  int numLevels = input->GetNumberOfLevels();
  int numBlocks;
  int blockId;
  int totalNumberOfBlocksInThisProcess = 0;

  // This is a big pain.
  // We have to look through all blocks to get a minimum root origin.
  // The origin must be chosen so there are no negative indexes.
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

  int lowestLevel = 0;
  double lowestSpacing[3];
  double lowestOrigin[3];
  int lowestDims[3];
  int largestLevel = 0;
  double largestOrigin[3];
  double largestSpacing[3];
  int largestDims[3];
  int largestNumCells;

  double globalBounds[6];

  // Temporary variables.
  double spacing[3];
  double bounds[6];
  int cellDims[3];
  int numCells;
  int ext[6];

  largestNumCells = 0;
  globalBounds[0] = globalBounds[2] = globalBounds[4] = VTK_FLOAT_MAX;
  globalBounds[1] = globalBounds[3] = globalBounds[5] = -VTK_FLOAT_MAX;
  lowestSpacing[0] = lowestSpacing[1] = lowestSpacing[2] = 0.0;

  // Add each block.
  for (int level = 0; level < numLevels; ++level)
  {
    numBlocks = input->GetNumberOfDataSets(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
      //      vtkAMRBox box;
      //      vtkImageData* image = input->GetDataSet(level,blockId,box);
      vtkImageData* image = input->GetDataSet(level, blockId);
      if (image)
      {
        ++totalNumberOfBlocksInThisProcess;
        image->GetBounds(bounds);
        // Compute globalBounds.
        if (globalBounds[0] > bounds[0])
        {
          globalBounds[0] = bounds[0];
        }
        if (globalBounds[1] < bounds[1])
        {
          globalBounds[1] = bounds[1];
        }
        if (globalBounds[2] > bounds[2])
        {
          globalBounds[2] = bounds[2];
        }
        if (globalBounds[3] < bounds[3])
        {
          globalBounds[3] = bounds[3];
        }
        if (globalBounds[4] > bounds[4])
        {
          globalBounds[4] = bounds[4];
        }
        if (globalBounds[5] < bounds[5])
        {
          globalBounds[5] = bounds[5];
        }
        image->GetExtent(ext);
        cellDims[0] = ext[1] - ext[0]; // ext is point extent.
        cellDims[1] = ext[3] - ext[2];
        cellDims[2] = ext[5] - ext[4];
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
        {                                  // This is the lowest level block we have encountered.
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
  int iMsg[9];
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
      for (int ii = 0; ii < 3; ++ii)
      {
        iMsg[3 + ii] = lowestDims[ii];
        iMsg[6 + ii] = largestDims[ii];
        dMsg[ii] = lowestSpacing[ii];
        dMsg[3 + ii] = lowestOrigin[ii];
        dMsg[6 + ii] = largestOrigin[ii];
        dMsg[9 + ii] = largestSpacing[ii];
        dMsg[12 + ii] = globalBounds[ii];
        dMsg[15 + ii] = globalBounds[ii + 3];
      }
      controller->Send(iMsg, 9, 0, 973432);
      controller->Send(dMsg, 18, 0, 973432);
    }
    else
    {
      // Collect results from all processes.
      for (int id = 1; id < numProcs; ++id)
      {
        controller->Receive(iMsg, 9, id, 973432);
        controller->Receive(dMsg, 18, id, 973432);
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
        {                                  // This is the lowest level block we have encountered.
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
        if (globalBounds[0] > dMsg[9])
        {
          globalBounds[0] = dMsg[9];
        }
        if (globalBounds[1] < dMsg[10])
        {
          globalBounds[1] = dMsg[10];
        }
        if (globalBounds[2] > dMsg[11])
        {
          globalBounds[2] = dMsg[11];
        }
        if (globalBounds[3] < dMsg[12])
        {
          globalBounds[3] = dMsg[12];
        }
        if (globalBounds[4] > dMsg[13])
        {
          globalBounds[4] = dMsg[13];
        }
        if (globalBounds[5] < dMsg[14])
        {
          globalBounds[5] = dMsg[14];
        }
      }
    }
  }

  if (myId == 0)
  {
    this->StandardBlockDimensions[0] = largestDims[0] - 1 - static_cast<int>(this->BlockGhostLevel);
    this->StandardBlockDimensions[1] = largestDims[1] - 1 - static_cast<int>(this->BlockGhostLevel);
    this->StandardBlockDimensions[2] = largestDims[2] - 1 - static_cast<int>(this->BlockGhostLevel);
    // For 2d case
    if (this->StandardBlockDimensions[2] < 1)
    {
      this->StandardBlockDimensions[2] = 1;
    }
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
    idx[0] = (int)(floor(0.5 + (lowestOrigin[0] - largestOrigin[0]) / largestSpacing[0]));
    idx[1] = (int)(floor(0.5 + (lowestOrigin[1] - largestOrigin[1]) / largestSpacing[1]));
    idx[2] = (int)(floor(0.5 + (lowestOrigin[2] - largestOrigin[2]) / largestSpacing[2]));
    lowestOrigin[0] = largestOrigin[0] + (double)(idx[0]) * largestSpacing[0];
    lowestOrigin[1] = largestOrigin[1] + (double)(idx[1]) * largestSpacing[1];
    lowestOrigin[2] = largestOrigin[2] + (double)(idx[2]) * largestSpacing[2];
    // OK.  Now we have the grid for the lowest level that has a block.
    // Change the grid to be of the blocks.
    lowestSpacing[0] *= this->StandardBlockDimensions[0];
    lowestSpacing[1] *= this->StandardBlockDimensions[1];
    lowestSpacing[2] *= this->StandardBlockDimensions[2];

    // Change the origin so that all indexes will be positive.
    idx[0] = (int)(floor((globalBounds[0] - lowestOrigin[0]) / lowestSpacing[0]));
    idx[1] = (int)(floor((globalBounds[2] - lowestOrigin[1]) / lowestSpacing[1]));
    idx[2] = (int)(floor((globalBounds[4] - lowestOrigin[2]) / lowestSpacing[2]));
    this->GlobalOrigin[0] = lowestOrigin[0] + (double)(idx[0]) * lowestSpacing[0];
    this->GlobalOrigin[1] = lowestOrigin[1] + (double)(idx[1]) * lowestSpacing[1];
    this->GlobalOrigin[2] = lowestOrigin[2] + (double)(idx[2]) * lowestSpacing[2];

    // Now send these to all the other processes and we are done!
    for (int ii = 0; ii < 3; ++ii)
    {
      dMsg[ii] = this->GlobalOrigin[ii];
      dMsg[ii + 3] = this->RootSpacing[ii];
      dMsg[ii + 6] = (double)(this->StandardBlockDimensions[ii]);
    }
    for (int ii = 1; ii < numProcs; ++ii)
    {
      controller->Send(dMsg, 9, ii, 973439);
    }
  }
  else
  {
    controller->Receive(dMsg, 9, 0, 973439);
    for (int ii = 0; ii < 3; ++ii)
    {
      this->GlobalOrigin[ii] = dMsg[ii];
      this->RootSpacing[ii] = dMsg[ii + 3];
      this->StandardBlockDimensions[ii] = (int)(dMsg[ii + 6]);
    }
  }

  return totalNumberOfBlocksInThisProcess;
}

//----------------------------------------------------------------------------
// This method creates ghost layer blocks across multiple processes.
void vtkMaterialInterfaceFilter::ShareGhostBlocks()
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myProc = this->Controller->GetLocalProcessId();
  vtkCommunicator* com = this->Controller->GetCommunicator();

  // Bad things can happen if not all processes call
  // MPI_Alltoallv this at the same time. (mpich)
  this->Controller->Barrier();

  // First share the number of blocks.
  int* blocksPerProcess = new int[numProcs];
  com->AllGather(&(this->NumberOfInputBlocks), blocksPerProcess, 1);
  // MPI_Allgather(&(this->NumberOfInputBlocks), 1, MPI_INT,
  // blocksPerProcess, 1, MPI_INT,
  //*com->GetMPIComm()->GetHandle());

  // Share the levels and extents of all blocks.
  // First, setup the count and displacement arrays required by AllGatherV.
  int totalNumberOfBlocks = 0;
  int* sendCounts = new int[numProcs];
  vtkIdType* recvCounts = new vtkIdType[numProcs];
  vtkIdType* displacements = new vtkIdType[numProcs];
  for (int ii = 0; ii < numProcs; ++ii)
  {
    displacements[ii] = totalNumberOfBlocks * 7;
    recvCounts[ii] = blocksPerProcess[ii] * 7;
    totalNumberOfBlocks += blocksPerProcess[ii];
  }

  // Setup the array to send to the other processes.
  int* localBlockInfo = new int[this->NumberOfInputBlocks * 7];
  for (int ii = 0; ii < this->NumberOfInputBlocks; ++ii)
  {
    localBlockInfo[ii * 7] = this->InputBlocks[ii]->GetLevel();
    const int* ext;
    // Lets do just the cell extent without overlap.
    // Edges and corners or overlap can cause problems when
    // neighbors are a different level.
    ext = this->InputBlocks[ii]->GetBaseCellExtent();
    for (int jj = 0; jj < 6; ++jj)
    {
      localBlockInfo[ii * 7 + 1 + jj] = ext[jj];
    }
  }
  // Allocate the memory to receive the gathered extents.
  int* gatheredBlockInfo = new int[totalNumberOfBlocks * 7];

  // TODO: check for errors ...
  com->AllGatherV(
    localBlockInfo, gatheredBlockInfo, this->NumberOfInputBlocks * 7, recvCounts, displacements);
  // MPI_Allgatherv((void*)localBlockInfo, this->NumberOfInputBlocks*7, MPI_INT,
  //(void*)gatheredBlockInfo, recvCounts, displacements,
  // MPI_INT, *com->GetMPIComm()->GetHandle());

  this->ComputeAndDistributeGhostBlocks(blocksPerProcess, gatheredBlockInfo, myProc, numProcs);
// Send:
// Process, extent, data,
// ...
// Receive:
// Process, extent
// ...

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->NumberOfGhostBlocks = this->GhostBlocks.size();
#endif

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

  delete[] blocksPerProcess;
  delete[] sendCounts;
  delete[] recvCounts;
  delete[] displacements;
  delete[] localBlockInfo;
  delete[] gatheredBlockInfo;
}

//----------------------------------------------------------------------------
// Loop over all blocks from other processes.  Find all local blocks
// that touch the block.
void vtkMaterialInterfaceFilter::ComputeAndDistributeGhostBlocks(
  int* numBlocksInProc, int* blockMetaData, int myProc, int numProcs)
{
  int requestMsg[8];
  int* ext;
  int bufSize = 0;
  unsigned char* buf = nullptr;
  int dataSize;
  vtkMaterialInterfaceFilterBlock* ghostBlock;

  // Loop through the other processes.
  int* blockMetaDataPtr = blockMetaData;
  for (int otherProc = 0; otherProc < numProcs; ++otherProc)
  {
    if (otherProc == myProc)
    {
      this->HandleGhostBlockRequests();
      // Skip the metat data for this process.
      blockMetaDataPtr += +7 * numBlocksInProc[myProc];
    }
    else
    {
      // Loop through the extents in the remote process.
      for (int id = 0; id < numBlocksInProc[otherProc]; ++id)
      {
        // Request message is (requesting process, block id, required extent)
        requestMsg[0] = myProc;
        requestMsg[1] = id;
        ext = requestMsg + 2;
        // Block meta data is level and base-cell-extent.
        int ghostBlockLevel = blockMetaDataPtr[0];
        int* ghostBlockExt = blockMetaDataPtr + 1;

        if (this->ComputeRequiredGhostExtent(ghostBlockLevel, ghostBlockExt, ext))
        {
          this->Controller->Send(requestMsg, 8, otherProc, 708923);
          // Now receive the ghost block.
          dataSize = (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
          if (bufSize < dataSize)
          {
            if (buf)
            {
              delete[] buf;
            }
            buf = new unsigned char[dataSize];
            bufSize = dataSize;
          }
          this->Controller->Receive(buf, dataSize, otherProc, 433240);
          // Make the ghost block and add it to the grid.
          ghostBlock = new vtkMaterialInterfaceFilterBlock;
          ghostBlock->InitializeGhostLayer(
            buf, ext, ghostBlockLevel, this->GlobalOrigin, this->RootSpacing, otherProc, id);
          // Save for deleting.
          this->GhostBlocks.push_back(ghostBlock);
          // Add to grid and connect up neighbors.
          this->AddBlock(ghostBlock, this->GetBlockGhostLevel());
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
    delete[] buf;
  }
}

//----------------------------------------------------------------------------
// Keep receiving and filling requests until all processes say they
// are finished asking for blocks.
void vtkMaterialInterfaceFilter::HandleGhostBlockRequests()
{
  int requestMsg[8];
  int otherProc;
  int blockId;
  vtkMaterialInterfaceFilterBlock* block;
  int bufSize = 0;
  unsigned char* buf = nullptr;
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
      if (block == nullptr)
      { // Sanity check. This will lock up!
        vtkErrorMacro("Missing block request.");
        return;
      }
      // Crop the block.
      // Now extract the data for the ghost layer.
      ext = requestMsg + 2;
      dataSize = (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
      if (bufSize < dataSize)
      {
        if (buf)
        {
          delete[] buf;
        }
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
    delete[] buf;
  }
}

//----------------------------------------------------------------------------
// TODO: Try to not get extents supplied by existing overlap.
// Return 1 if we need this ghost block.  Ext is the part we need.
int vtkMaterialInterfaceFilter::ComputeRequiredGhostExtent(
  int remoteBlockLevel, int remoteBaseCellExt[6], int neededExt[6])
{
  std::vector<vtkMaterialInterfaceFilterBlock*> neighbors;
  int remoteBlockIndex[3];
  int remoteLayerExt[6];

  // Extract the index from the block extent.
  // Lets choose the middle in case the extent has overlap.
  remoteBlockIndex[0] =
    (remoteBaseCellExt[0] + remoteBaseCellExt[1]) / (2 * this->StandardBlockDimensions[0]);
  remoteBlockIndex[1] =
    (remoteBaseCellExt[2] + remoteBaseCellExt[3]) / (2 * this->StandardBlockDimensions[1]);
  remoteBlockIndex[2] =
    (remoteBaseCellExt[4] + remoteBaseCellExt[5]) / (2 * this->StandardBlockDimensions[2]);

  neededExt[0] = neededExt[2] = neededExt[4] = VTK_INT_MAX;
  neededExt[1] = neededExt[3] = neededExt[5] = -VTK_INT_MAX;

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
          if (this->HasNeighbor(remoteBlockLevel, remoteBlockIndex, direction))
          { // A neighbor exists in this location.
            // Make sure we get ghost cells for this neighbor.
            memcpy(remoteLayerExt, remoteBaseCellExt, 6 * sizeof(int));
            if (ix == -1)
            {
              remoteLayerExt[1] = remoteLayerExt[0];
            }
            if (ix == 1)
            {
              remoteLayerExt[0] = remoteLayerExt[1];
            }
            if (iy == -1)
            {
              remoteLayerExt[3] = remoteLayerExt[2];
            }
            if (iy == 1)
            {
              remoteLayerExt[2] = remoteLayerExt[3];
            }
            if (iz == -1)
            {
              remoteLayerExt[5] = remoteLayerExt[4];
            }
            if (iz == 1)
            {
              remoteLayerExt[4] = remoteLayerExt[5];
            }
            // Take the union of all blocks.
            if (neededExt[0] > remoteLayerExt[0])
            {
              neededExt[0] = remoteLayerExt[0];
            }
            if (neededExt[1] < remoteLayerExt[1])
            {
              neededExt[1] = remoteLayerExt[1];
            }
            if (neededExt[2] > remoteLayerExt[2])
            {
              neededExt[2] = remoteLayerExt[2];
            }
            if (neededExt[3] < remoteLayerExt[3])
            {
              neededExt[3] = remoteLayerExt[3];
            }
            if (neededExt[4] > remoteLayerExt[4])
            {
              neededExt[4] = remoteLayerExt[4];
            }
            if (neededExt[5] < remoteLayerExt[5])
            {
              neededExt[5] = remoteLayerExt[5];
            }
          }
        }
      }
    }
  }

  if (neededExt[0] > neededExt[1] || neededExt[2] > neededExt[3] || neededExt[4] > neededExt[5])
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// We need ghostblocks to connect as neighbors too.
void vtkMaterialInterfaceFilter::AddBlock(
  vtkMaterialInterfaceFilterBlock* block, unsigned char levelOfGhostLayer)
{
  vtkMaterialInterfaceLevel* level = this->Levels[block->GetLevel()];
  int dims[3];
  level->GetBlockDimensions(dims);
  block->ComputeBaseExtent(dims, levelOfGhostLayer);
  this->CheckLevelsForNeighbors(block);
  level->AddBlock(block);
}

//
vtkPolyData* vtkMaterialInterfaceFilter::NewFragmentMesh()
{
  // The generated fragments for this material are placed here
  // until they are resolved and partitioned
  vtkPolyData* newPiece = vtkPolyData::New();

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
  for (int i = 0; i < this->NToIntegrate; ++i)
  {
    vtkDoubleArray* waa = vtkDoubleArray::New();
    waa->SetName(this->IntegratedArrayNames[i].c_str());
    waa->SetNumberOfComponents(this->IntegratedArrayNComp[i]);
    newPiece->GetCellData()->AddArray(waa);
    waa->Delete();
  }

// for debugging purposes...
#ifdef vtkMaterialInterfaceFilterDEBUG
  vtkIntArray* blockIdArray = vtkIntArray::New();
  blockIdArray->SetName("BlockId");
  newPiece->GetCellData()->AddArray(blockIdArray);
  blockIdArray->Delete();

  vtkIntArray* levelArray = vtkIntArray::New();
  levelArray->SetName("Level");
  newPiece->GetCellData()->AddArray(levelArray);
  levelArray->Delete();

  vtkIntArray* procIdArray = vtkIntArray::New();
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
void vtkMaterialInterfaceFilter::PrepareForPass(vtkNonOverlappingAMR* hbdsInput,
  vector<string>& volumeWtdAvgArrayNames, vector<string>& massWtdAvgArrayNames,
  vector<string>& summedArrayNames, vector<string>& integratedArrayNames)
{
  this->FragmentId = 0;

  this->FragmentVolume = 0.0;
  ReNewVtkPointer(this->FragmentVolumes);
  this->FragmentVolumes->SetName("Volume");

  if (this->ClipWithPlane)
  {
    this->ClipDepthMax = 0.0;
    this->ClipDepthMin = VTK_FLOAT_MAX;
    ReNewVtkPointer(this->ClipDepthMaximums);
    ReNewVtkPointer(this->ClipDepthMinimums);
    this->ClipDepthMaximums->SetName("ClipDepthMax");
    this->ClipDepthMinimums->SetName("ClipDepthMin");
  }

  if (this->ComputeMoments)
  {
    this->FragmentMoment.clear();
    this->FragmentMoment.resize(4, 0.0);
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
  vtkCompositeDataIterator* hbdsIt = hbdsInput->NewIterator();
  hbdsIt->SkipEmptyNodesOn();
  hbdsIt->InitTraversal();
  vtkImageData* testImage = nullptr;
  if (!hbdsIt->IsDoneWithTraversal())
  {
    //    testImage=dynamic_cast<vtkImageData *>(hbdsInput->GetDataSet(hbdsIt));
    testImage = vtkUniformGrid::SafeDownCast(hbdsInput->GetDataSet(hbdsIt));
  }
  hbdsIt->Delete();
  // if we got a null pointer then this indicates that
  // we do not have any blocks on this process.

  // Configure data structures
  // 1) Volume weighted average of attribute over the
  // fragment set up containers
  this->FragmentVolumeWtdAvg.clear();
  this->FragmentVolumeWtdAvg.resize(this->NVolumeWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentVolumeWtdAvgs);
  this->FragmentVolumeWtdAvgs.resize(this->NVolumeWtdAvgs);
  // set up data array and accumulator for each weighted average
  for (int j = 0; j < this->NVolumeWtdAvgs; ++j)
  {
    // data array
    const char* thisArrayName = volumeWtdAvgArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the
    // array structure from them.
    int nComp = 1;
    if (testImage)
    {
      vtkDataArray* testArray = testImage->GetCellData()->GetArray(thisArrayName);
      assert("Couldn't access the named array." && testArray);
      nComp = testArray->GetNumberOfComponents();
    }
    this->FragmentVolumeWtdAvgs[j] = vtkDoubleArray::New();
    this->FragmentVolumeWtdAvgs[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "VolumeWeightedAverage-" << thisArrayName;
    this->FragmentVolumeWtdAvgs[j]->SetName(osIntegratedArrayName.str().c_str());
    // accumulator
    this->FragmentVolumeWtdAvg[j].resize(nComp, 0.0);
  }
  // 2) Mass weighted average of attribute over the fragment
  // set up containers
  this->FragmentMassWtdAvg.clear();
  this->FragmentMassWtdAvg.resize(this->NMassWtdAvgs);
  ClearVectorOfVtkPointers(this->FragmentMassWtdAvgs);
  this->FragmentMassWtdAvgs.resize(this->NMassWtdAvgs);
  // set up data array and accumulator for each weighted average
  for (int j = 0; j < this->NMassWtdAvgs; ++j)
  {
    // data array
    const char* thisArrayName = massWtdAvgArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the
    // array structure from them.
    int nComp = 1;
    if (testImage)
    {
      vtkDataArray* testArray = testImage->GetCellData()->GetArray(thisArrayName);
      assert("Couldn't access the named array." && testArray);
      nComp = testArray->GetNumberOfComponents();
    }
    this->FragmentMassWtdAvgs[j] = vtkDoubleArray::New();
    this->FragmentMassWtdAvgs[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "MassWeightedAverage-" << thisArrayName;
    this->FragmentMassWtdAvgs[j]->SetName(osIntegratedArrayName.str().c_str());
    // accumulator
    this->FragmentMassWtdAvg[j].resize(nComp, 0.0);
  }
  // 3) Summation of attribute over the fragment
  // set up containers
  this->FragmentSum.clear();
  this->FragmentSum.resize(this->NToSum);
  ClearVectorOfVtkPointers(this->FragmentSums);
  this->FragmentSums.resize(this->NToSum);
  // set up data array and accumulator for each weighted average
  for (int j = 0; j < this->NToSum; ++j)
  {
    // data array
    const char* thisArrayName = summedArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the
    // array structure from them.
    int nComp = 1;
    if (testImage)
    {
      vtkDataArray* testArray = testImage->GetCellData()->GetArray(thisArrayName);
      assert("Couldn't access the named array." && testArray);
      nComp = testArray->GetNumberOfComponents();
    }
    this->FragmentSums[j] = vtkDoubleArray::New();
    this->FragmentSums[j]->SetNumberOfComponents(nComp);
    ostringstream osIntegratedArrayName;
    osIntegratedArrayName << "Summation-" << thisArrayName;
    this->FragmentSums[j]->SetName(osIntegratedArrayName.str().c_str());
    // accumulator
    this->FragmentSum[j].resize(nComp, 0.0);
  }

  // 4) Unique list of integrated attributes
  // Get the number of components for each. These are
  // used to copy from input to fragment cell data
  this->IntegratedArrayNComp.resize(this->NToIntegrate, 0);
  for (int j = 0; j < this->NToIntegrate; ++j)
  {
    // data array
    const char* thisArrayName = integratedArrayNames[j].c_str();
    // if we have blocks on this process, we need to copy the
    // array structure from them.
    if (testImage)
    {
      vtkDataArray* testArray = testImage->GetCellData()->GetArray(thisArrayName);
      assert("Couldn't access the named array." && testArray);
      this->IntegratedArrayNComp[j] = testArray->GetNumberOfComponents();
    }
  }
}

//----------------------------------------------------------------------------
// Filter operates on Materials, generating fragments by connecting
// voxels where the threshold is value is attained. It can sum
// voxel contributions, or compute their weighted average
// it also computes the minimum bounding box of each resulting fragment
// and its center.
// TODO use same array name for frag id on multiple materials, and use global id
int vtkMaterialInterfaceFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  this->NumberOfBlocks = 0;
  this->NumberOfGhostBlocks = 0;
#endif

  if (this->ClipFunction)
  {
    vtkSphere* s = vtkSphere::SafeDownCast(this->ClipFunction);
    vtkPlane* p = vtkPlane::SafeDownCast(this->ClipFunction);
    if (s)
    {
      this->ClipWithSphere = 1;
      this->ClipWithPlane = 0;
      s->GetCenter(this->ClipCenter);
      this->ClipRadius = s->GetRadius();
    }
    else if (p)
    {
      this->ClipWithSphere = 0;
      this->ClipWithPlane = 1;
      this->ClipRadius = 1;
      p->GetOrigin(this->ClipCenter);
      p->GetNormal(this->ClipPlaneVector);
      p->GetNormal(this->ClipPlaneNormal);
      vtkMath::Normalize(this->ClipPlaneNormal);
    }
    else
    {
      vtkErrorMacro("Only Sphere and Plane clip function currently supported.");
    }
  }

#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << this->Controller->GetLocalProcessId()
       << " entered request data."
       << " MTime: " << this->GetMTime() << endl;
  std::clock_t startTime = std::clock();
#ifdef USE_VOXEL_VOLUME
  cerr << "USE_VOXEL_VOLUME\n";
#endif
#endif
  // get the data set which we are to process
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* hbdsInput =
    vtkNonOverlappingAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Get the outputs
  // 0
  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // 1
  outInfo = outputVector->GetInformationObject(1);
  vtkMultiBlockDataSet* mbdsOutput1 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
#ifdef vtkMaterialInterfaceFilterDEBUG
  // 2
  outInfo = outputVector->GetInformationObject(2);
  vtkMultiBlockDataSet* mbdsOutput2 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
#else
  vtkMultiBlockDataSet* mbdsOutput2 = nullptr;
#endif

  // Get arrays to process based on array selection status
  vector<string> MaterialArrayNames;
  vector<string> MassArrayNames;
  vector<string> SummedArrayNames;
  this->VolumeWtdAvgArrayNames.clear();
  this->MassWtdAvgArrayNames.clear();

  int nMaterials = GetEnabledArrayNames(this->MaterialArraySelection, MaterialArrayNames);
  int nMassArrays = GetEnabledArrayNames(this->MassArraySelection, MassArrayNames);
  this->NVolumeWtdAvgs =
    GetEnabledArrayNames(this->VolumeWtdAvgArraySelection, this->VolumeWtdAvgArrayNames);
  int nMassWtdAvgs =
    GetEnabledArrayNames(this->MassWtdAvgArraySelection, this->MassWtdAvgArrayNames);
  this->NToSum = GetEnabledArrayNames(this->SummationArraySelection, SummedArrayNames);

  // anything to do?
  if (nMaterials == 0)
  {
    vtkWarningMacro("No material fraction specified.");
    return 1;
  }
  // let us look up names
  if (nMassArrays < nMaterials)
  {
    MassArrayNames.resize(nMaterials);
    vtkWarningMacro("Not all materials have mass arrays specified. "
      << nMaterials << " materials. " << nMassArrays << " mass arrays.");
  }

  // set up structure in the output data sets
  this->BuildOutputs(mbdsOutput0, mbdsOutput1, mbdsOutput2, nMaterials);

  //
  this->ProgressMaterialInc = 1.0 / (double)nMaterials;
#ifdef vtkMaterialInterfaceFilterDebug
  this->ProgressResolutionInc = 1.0 / this->ProgressMaterialInc / 2.0 / 12.0;
#else
  this->ProgressResolutionInc = 1.0 / this->ProgressMaterialInc / 2.0 / 11.0;
#endif

  // process enabled material arrays
  for (this->MaterialId = 0; this->MaterialId < nMaterials; ++this->MaterialId)
  {
#ifdef vtkMaterialInterfaceFilterDEBUG
    int myProcId = this->Controller->GetLocalProcessId();
    cerr << "[" << __LINE__ << "] " << myProcId << " is processing Material " << this->MaterialId
         << "." << endl
         << "[" << __LINE__ << "] " << myProcId << " memory commitment at begin of pass is:" << endl
         << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

    this->Progress = this->MaterialId * this->ProgressMaterialInc;
    this->UpdateProgress(this->Progress);

    // moments and mass weighted averages are only calculated
    // where mass arrays has been selected
    this->ComputeMoments = this->MaterialId < nMassArrays ? true : false;
    this->NMassWtdAvgs = this->MaterialId < nMassArrays ? nMassWtdAvgs : 0;
    // arrays to copy to the output are affected as well

    // We used to compute the Union of the data array that were needed but now
    // we simply pass all possible array so the user can use other one to color by
    this->IntegratedArrayNames.clear();
    this->NToIntegrate = 0;

    if (hbdsInput != nullptr)
    {
      // Extract all compatible cell data so they could be used by the user
      // to color by. Previously only the array that were used for computation
      // was kept
      vtkUniformGrid* ds = GetReferenceGrid(hbdsInput);
      if (ds)
      {
        this->NToIntegrate = ds->GetCellData()->GetNumberOfArrays();
        int substract = 0;
        for (int i = 0; i < this->NToIntegrate; ++i)
        {
          switch (ds->GetCellData()->GetArray(i)->GetDataType())
          {
            case VTK_FLOAT:
            case VTK_DOUBLE:
            case VTK_UNSIGNED_INT:
            case VTK_INT:
            {
              const char* arrayName = ds->GetCellData()->GetArrayName(i);
              this->IntegratedArrayNames.push_back(arrayName);
            }
            break;
            default:
              ++substract;
              break;
          }
        }
        this->NToIntegrate -= substract;
      }

      // build arrays for results of attribute calculations
      this->PrepareForPass(hbdsInput, this->VolumeWtdAvgArrayNames, this->MassWtdAvgArrayNames,
        SummedArrayNames, this->IntegratedArrayNames);
      //
      this->EquivalenceSet->Initialize();
      //
      this->InitializeBlocks(hbdsInput, MaterialArrayNames[this->MaterialId],
        MassArrayNames[this->MaterialId], this->VolumeWtdAvgArrayNames, this->MassWtdAvgArrayNames,
        SummedArrayNames, this->IntegratedArrayNames);
    }
    else
    {
      // Deal with rectilinear grid
      /*vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inInfo);
      if (input)
        {
        vtkCompositeDataIterator* iter = input->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
          vtkRectilinearGrid* dataSet = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
          }
        }
      */
      vtkErrorMacro("This filter requires a vtkNonOverlappingAMR on its input.");
      return 0;
    }

    //
    this->ProgressBlockInc = this->ProgressMaterialInc / (double)this->NumberOfInputBlocks / 2.0;
//
#ifdef vtkMaterialInterfaceFilterPROFILE
    // Lets profile to see what takes the most time for large number of processes.
    this->ProcessBlocksTimer->StartTimer();
#endif
    int blockId;
    for (blockId = 0; blockId < this->NumberOfInputBlocks; ++blockId)
    {
      // build fragments
      this->ProcessBlock(blockId);
    }
#ifdef vtkMaterialInterfaceFilterPROFILE
    // Lets profile to see what takes the most time for large number of processes.
    this->ProcessBlocksTimer->StopTimer();
#endif
// char tmp[128];
// sprintf(tmp, "C:/Law/tmp/mifSurface%d.vtp", this->Controller->GetLocalProcessId());
// this->SaveBlockSurfaces(tmp);
// sprintf(tmp, "C:/Law/tmp/mifGhost%d.vtp", this->Controller->GetLocalProcessId());
// this->SaveGhostSurfaces(tmp);

#ifdef vtkMaterialInterfaceFilterPROFILE
    // Lets profile to see what takes the most time for large number of processes.
    this->ResolveEquivalencesTimer->StartTimer();
#endif

    // resolve: Merge local and remote geometry
    // correct integrated attributes, finialize integrations
    this->PrepareForResolveEquivalences();
    this->ResolveEquivalences();

#ifdef vtkMaterialInterfaceFilterPROFILE
    // Lets profile to see what takes the most time for large number of processes.
    this->ResolveEquivalencesTimer->StopTimer();
#endif

    // update the resolved fragment count, so that next pass will start
    // where we left off here
    this->ResolvedFragmentCount += this->NumberOfResolvedFragments;

    // clean after pass
    this->DeleteAllBlocks();
    //
    ReleaseVtkPointer(this->FragmentVolumes);
    if (this->ClipWithPlane)
    {
      ReleaseVtkPointer(this->ClipDepthMaximums);
      ReleaseVtkPointer(this->ClipDepthMinimums);
    }
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
  if (this->GetWriteStatisticsOutput() && this->OutputBaseName)
  {
    this->WriteStatisticsOutputToTextFile();
  }
  if (this->GetWriteGeometryOutput() && this->OutputBaseName)
  {
    this->WriteGeometryOutputToTextFile();
  }
  // clean up after all passes complete
  this->ResolvedFragmentIds.clear();
  this->FragmentSplitMarker.clear();
#ifdef vtkMaterialInterfaceFilterDEBUG
  std::clock_t endTime = std::clock();
  cerr << "[" << __LINE__ << "] " << this->Controller->GetLocalProcessId()
       << " clock time elapsed during request data "
       << (double)(endTime - startTime) / (double)CLOCKS_PER_SEC << " sec." << endl;
  cerr << "[" << __LINE__ << "] " << this->Controller->GetLocalProcessId()
       << " exited request data."
       << " MTime: " << this->GetMTime() << "." << endl;
#endif

#ifdef vtkMaterialInterfaceFilterPROFILE
  // Lets profile to see what takes the most time for large number of processes.
  double initializeTime, shareGhostBlocksTime, processBlocksTime, resolveEquivalencesTime;
  unsigned long numberOfBlocks, numberOfGhostBlocks;
  initializeTime = this->InitializeBlocksTimer->GetElapsedTime();
  shareGhostBlocksTime = this->ShareGhostBlocksTimer->GetElapsedTime();
  processBlocksTime = this->ProcessBlocksTimer->GetElapsedTime();
  resolveEquivalencesTime = this->ResolveEquivalencesTimer->GetElapsedTime();
  numberOfBlocks = this->NumberOfBlocks;
  numberOfGhostBlocks = this->NumberOfGhostBlocks;
  if (this->Controller == 0)
  {
    cout << "InitializeTime: " << initializeTime << endl;
    cout << "ShareGhostBlocksTime: " << shareGhostBlocksTime << endl;
    cout << "NumberOfBlocks: " << numberOfBlocks << endl;
    cout << "NumberOfGhostBlocks: " << numberOfGhostBlocks << endl;
    cout << "ProcessBlocksTime: " << processBlocksTime << endl;
    cout << "ResolveEquivalencesTimer: " << resolveEquivalencesTime << endl;
  }
  else
  {
    int numProcs = this->Controller->GetNumberOfProcesses();
    if (this->Controller->GetLocalProcessId() == 0)
    {
      cout << "Process 0: \n";
      cout << "  InitializeTime: " << initializeTime << endl;
      cout << "  ShareGhostBlocksTime: " << shareGhostBlocksTime << endl;
      cout << "  NumberOfBlocks: " << numberOfBlocks << endl;
      cout << "  NumberOfGhostBlocks: " << numberOfGhostBlocks << endl;
      cout << "  ProcessBlocksTime: " << processBlocksTime << endl;
      cout << "  ResolveEquivalencesTimer: " << resolveEquivalencesTime << endl;
      for (int procIdx = 1; procIdx < numProcs; ++procIdx)
      {
        this->Controller->Receive(&initializeTime, 1, procIdx, 234908);
        this->Controller->Receive(&shareGhostBlocksTime, 1, procIdx, 234909);
        this->Controller->Receive(&processBlocksTime, 1, procIdx, 234910);
        this->Controller->Receive(&resolveEquivalencesTime, 1, procIdx, 234911);
        this->Controller->Receive(&numberOfBlocks, 1, procIdx, 234912);
        this->Controller->Receive(&numberOfGhostBlocks, 1, procIdx, 234913);
        cout << "Process " << procIdx << ": \n";
        cout << "  InitializeTime: " << initializeTime << endl;
        cout << "  ShareGhostBlocksTime: " << shareGhostBlocksTime << endl;
        cout << "  NumberOfBlocks: " << numberOfBlocks << endl;
        cout << "  NumberOfGhostBlocks: " << numberOfGhostBlocks << endl;
        cout << "  ProcessBlocksTime: " << processBlocksTime << endl;
        cout << "  ResolveEquivalencesTimer: " << resolveEquivalencesTime << endl;
      }
    }
    else
    {
      this->Controller->Send(&initializeTime, 1, 0, 234908);
      this->Controller->Send(&shareGhostBlocksTime, 1, 0, 234909);
      this->Controller->Send(&processBlocksTime, 1, 0, 234910);
      this->Controller->Send(&resolveEquivalencesTime, 1, 0, 234911);
      this->Controller->Send(&numberOfBlocks, 1, 0, 234912);
      this->Controller->Send(&numberOfGhostBlocks, 1, 0, 234913);
    }
  }
#endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::ProcessBlock(int blockId)
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::ProcessBlock(" << blockId << ") , Material "
               << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressBlockInc;
  this->UpdateProgress(this->Progress);

  vtkMaterialInterfaceFilterBlock* block = this->InputBlocks[blockId];
  if (block == nullptr)
  {
    return 0;
  }

  vtkMaterialInterfaceFilterIterator* xIterator = new vtkMaterialInterfaceFilterIterator;
  vtkMaterialInterfaceFilterIterator* yIterator = new vtkMaterialInterfaceFilterIterator;
  vtkMaterialInterfaceFilterIterator* zIterator = new vtkMaterialInterfaceFilterIterator;
  zIterator->Block = block;
  // set iterator to reference first non ghost cell
  zIterator->VolumeFractionPointer = block->GetBaseVolumeFractionPointer();
  zIterator->FragmentIdPointer = block->GetBaseFragmentIdPointer();
  zIterator->FlatIndex = block->GetBaseFlatIndex();

  vtkMaterialInterfaceFilterRingBuffer* queue = new vtkMaterialInterfaceFilterRingBuffer;

  // Loop through all the voxels.
  int ix, iy, iz;
  const int* ext;
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
        //
        if (*(xIterator->FragmentIdPointer) == -1 &&
          *(xIterator->VolumeFractionPointer) > this->scaledMaterialFractionThreshold)
        { // We have a new fragment.
          this->CurrentFragmentMesh = this->NewFragmentMesh();
          this->EquivalenceSet->AddEquivalence(this->FragmentId, this->FragmentId);
          // We have to mark every voxel we push on the queue.
          *(xIterator->FragmentIdPointer) = this->FragmentId;
          // There should be no need to clear the queue.
          queue->Push(xIterator);
          this->ConnectFragment(queue);
          // save the current fragment mesh
          // the id is implicit given by its position in the vector, but only
          // until fragments are resolved. After resolution we add addributes such
          // as id, volume, summations averages, etc..
          this->CurrentFragmentMesh->Squeeze();
          this->FragmentMeshes.push_back(this->CurrentFragmentMesh);
          // Save the volume from the last fragment.
          this->FragmentVolumes->InsertTuple1(this->FragmentId, this->FragmentVolume);
          if (this->ClipWithPlane)
          {
            this->ClipDepthMaximums->InsertTuple1(this->FragmentId, this->ClipDepthMax);
            this->ClipDepthMinimums->InsertTuple1(this->FragmentId, this->ClipDepthMin);
          }
          // clear the volume accumulator
          this->FragmentVolume = 0.0;
          this->ClipDepthMax = 0.0;
          this->ClipDepthMin = VTK_FLOAT_MAX;
          if (this->ComputeMoments)
          {
            // Save the moments from the last fragment
            this->FragmentMoments->InsertTuple(this->FragmentId, &this->FragmentMoment[0]);
            // clear the moment accumulator
            FillVector(this->FragmentMoment, 0.0);
          }
          // for the volume weighted averaged scalars/vectors...
          for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
          {
            // update the integrated value, independent of ncomps
            this->FragmentVolumeWtdAvgs[i]->InsertTuple(
              this->FragmentId, &this->FragmentVolumeWtdAvg[i][0]);
            // clear the accumulator
            FillVector(this->FragmentVolumeWtdAvg[i], 0.0);
          }
          // for the mass weighted averaged scalars/vectors...
          for (int i = 0; i < this->NMassWtdAvgs; ++i)
          {
            // update the integrated value, independent of ncomps
            this->FragmentMassWtdAvgs[i]->InsertTuple(
              this->FragmentId, &this->FragmentMassWtdAvg[i][0]);
            // clear the accumulator
            FillVector(this->FragmentMassWtdAvg[i], 0.0);
          }
          // for the summed scalars/vectors...
          for (int i = 0; i < this->NToSum; ++i)
          {
            // update the integrated value, independent of ncomps
            this->FragmentSums[i]->InsertTuple(this->FragmentId, &this->FragmentSum[i][0]);
            // clear the accumulator
            FillVector(this->FragmentSum[i], 0.0);
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

// We conserver neighbor relations and put the reference (in)
// block in position 0, and the out block in position 1.
// The face being generated is between 0 and 1.
static int MIF_FRAGMENT_CONNECT_CORNER_PERMUTATION[8][3][8] = {
  { { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 2, 1, 3, 4, 6, 5, 7 }, { 0, 4, 2, 6, 1, 5, 3, 7 } },
  { { 1, 0, 3, 2, 5, 4, 7, 6 }, { 1, 3, 0, 2, 5, 7, 4, 6 }, { 1, 5, 3, 7, 0, 4, 2, 6 } },
  { { 2, 3, 0, 1, 6, 7, 4, 5 }, { 2, 0, 3, 1, 6, 4, 7, 5 }, { 2, 6, 0, 4, 3, 7, 1, 5 } },
  { { 3, 2, 1, 0, 7, 6, 5, 4 }, { 3, 1, 2, 0, 7, 5, 6, 4 }, { 3, 7, 2, 6, 1, 5, 0, 4 } },
  { { 4, 5, 6, 7, 0, 1, 2, 3 }, { 4, 6, 5, 7, 0, 2, 1, 3 }, { 4, 0, 6, 2, 5, 1, 7, 3 } },
  { { 5, 4, 7, 6, 1, 0, 3, 2 }, { 5, 7, 4, 6, 1, 3, 0, 2 }, { 5, 1, 7, 3, 4, 0, 6, 2 } },
  { { 6, 7, 2, 3, 4, 5, 0, 1 }, { 6, 4, 2, 0, 7, 5, 3, 1 }, { 6, 2, 7, 3, 4, 0, 5, 1 } },
  { { 7, 6, 5, 4, 3, 2, 1, 0 }, { 7, 5, 6, 4, 3, 1, 2, 0 }, { 7, 3, 5, 1, 6, 2, 4, 0 } }
};

// Blank out voxels not connected to 0.
// Only 127 because 0 is always on.
static int MIF_FRAGMENT_CONNECT_CORNER_MASK[64][8] = {
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 0 0 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 0 0 0 0
  { 0, 0, 0, 1, 0, 0, 0, 0 }, // 1 0 0 1 0 0 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 0 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 0 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 0 0 0
  { 0, 0, 0, 1, 0, 0, 0, 0 }, // 1 0 0 1 1 0 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 0 0 0
  { 0, 0, 0, 0, 0, 1, 0, 0 }, // 1 0 0 0 0 1 0 0
  { 0, 0, 0, 0, 0, 1, 0, 0 }, // 1 0 1 0 0 1 0 0
  { 0, 0, 0, 1, 0, 1, 0, 0 }, // 1 0 0 1 0 1 0 0
  { 0, 0, 0, 0, 0, 1, 0, 0 }, // 1 0 1 1 0 1 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 1 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 1 0 0
  { 0, 0, 0, 1, 0, 0, 0, 0 }, // 1 0 0 1 1 1 0 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 1 0 0
  { 0, 0, 0, 0, 0, 0, 1, 0 }, // 1 0 0 0 0 0 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 0 0 1 0
  { 0, 0, 0, 1, 0, 0, 1, 0 }, // 1 0 0 1 0 0 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 0 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 0 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 0 1 0
  { 0, 0, 0, 1, 0, 0, 0, 0 }, // 1 0 0 1 1 0 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 0 1 0
  { 0, 0, 0, 0, 0, 1, 1, 0 }, // 1 0 0 0 0 1 1 0
  { 0, 0, 0, 0, 0, 1, 0, 0 }, // 1 0 1 0 0 1 1 0
  { 0, 0, 0, 1, 0, 1, 1, 0 }, // 1 0 0 1 0 1 1 0
  { 0, 0, 0, 0, 0, 1, 0, 0 }, // 1 0 1 1 0 1 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 1 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 1 1 0
  { 0, 0, 0, 1, 0, 0, 0, 0 }, // 1 0 0 1 1 1 1 0
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 1 1 0
  { 0, 0, 0, 0, 0, 0, 0, 1 }, // 1 0 0 0 0 0 0 1
  { 0, 0, 0, 0, 0, 0, 0, 1 }, // 1 0 1 0 0 0 0 1
  { 0, 0, 0, 1, 0, 0, 0, 1 }, // 1 0 0 1 0 0 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 0 0 1
  { 0, 0, 0, 0, 0, 0, 0, 1 }, // 1 0 0 0 1 0 0 1
  { 0, 0, 0, 0, 0, 0, 0, 1 }, // 1 0 1 0 1 0 0 1
  { 0, 0, 0, 1, 0, 0, 0, 1 }, // 1 0 0 1 1 0 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 0 0 1
  { 0, 0, 0, 0, 0, 1, 0, 1 }, // 1 0 0 0 0 1 0 1
  { 0, 0, 0, 0, 0, 1, 0, 1 }, // 1 0 1 0 0 1 0 1
  { 0, 0, 0, 1, 0, 1, 0, 1 }, // 1 0 0 1 0 1 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 1 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 1 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 1 0 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 1 1 1 0 1
  { 0, 0, 0, 0, 0, 0, 2, 0 }, // 1 0 1 1 1 1 0 1 // special case 47 here.
  { 0, 0, 0, 0, 0, 0, 1, 1 }, // 1 0 0 0 0 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 0 0 1 1
  { 0, 0, 0, 1, 0, 0, 1, 1 }, // 1 0 0 1 0 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 1 1 0 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 1 0 1 1
  { 0, 0, 0, 0, 0, 1, 1, 1 }, // 1 0 0 0 0 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 0 1 1 1
  { 0, 0, 0, 1, 0, 1, 1, 1 }, // 1 0 0 1 0 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 1 0 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 0 1 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 1 0 1 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }, // 1 0 0 1 1 1 1 1
  { 0, 0, 0, 0, 0, 0, 0, 0 }  // 1 0 1 1 1 1 1 1
};

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
// The return value indicates that an edge may be non manifold.
// It returns the y or z axis index of the edge that may be non manifold.
int vtkMaterialInterfaceFilter::ComputeDisplacementFactors(
  vtkMaterialInterfaceFilterIterator* pointNeighborIterators[8], double displacmentFactors[3],
  int rootNeighborIdx, int faceIdx)
{
  // DEBUGGING
  // This generates the raw voxel surface when uncommented.
  // displacmentFactors[0] = 0.0;
  // displacmentFactors[1] = 0.0;
  // displacmentFactors[2] = 0.0;
  // return;

  // double v000 = pointNeighborIterators[0]->VolumeFractionPointer[0];
  // double v001 = pointNeighborIterators[1]->VolumeFractionPointer[0];
  // double v010 = pointNeighborIterators[2]->VolumeFractionPointer[0];
  // double v011 = pointNeighborIterators[3]->VolumeFractionPointer[0];
  // double v100 = pointNeighborIterators[4]->VolumeFractionPointer[0];
  // double v101 = pointNeighborIterators[5]->VolumeFractionPointer[0];
  // double v110 = pointNeighborIterators[6]->VolumeFractionPointer[0];
  // double v111 = pointNeighborIterators[7]->VolumeFractionPointer[0];
  double v[8];
  v[0] = pointNeighborIterators[0]->VolumeFractionPointer[0];
  v[1] = pointNeighborIterators[1]->VolumeFractionPointer[0];
  v[2] = pointNeighborIterators[2]->VolumeFractionPointer[0];
  v[3] = pointNeighborIterators[3]->VolumeFractionPointer[0];
  v[4] = pointNeighborIterators[4]->VolumeFractionPointer[0];
  v[5] = pointNeighborIterators[5]->VolumeFractionPointer[0];
  v[6] = pointNeighborIterators[6]->VolumeFractionPointer[0];
  v[7] = pointNeighborIterators[7]->VolumeFractionPointer[0];
  // To eliminate non manifold surfaces, I am going to perform
  // connectivity on the 2x2x2 volume and mask out voxels
  // that are not face connected.  Edge connected surfaces
  // will pull away from each other.
  // First we need to permute the 8 values so that the inside
  // voxel adjacent to the face is in position 0.

  // Note: in order to fix the case (all high except two opposite corners),
  // we permute so that the face is between 0 and 1.
  // 0 should always be on, and 1 should always be off.

  int* permutation = MIF_FRAGMENT_CONNECT_CORNER_PERMUTATION[rootNeighborIdx][faceIdx];
  // Now compute the mask case index after permutation.
  int caseIdx = 0;
  // if (v[permutation[0]] <= this->scaledMaterialFractionThreshold) { vtkErrorMacro("0 should
  // always be inside");}
  // This next condition does actually occur because of padding for boundary condition.
  // We want to pad with the same volume fraction so that the sub-voxel positioning does
  // not move the points normal to the surface, but we still want to create the surface.
  // if (v[permutation[1]] > this->scaledMaterialFractionThreshold) {vtkErrorMacro("1 should always
  // be outside");}
  if (v[permutation[2]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 1;
  }
  if (v[permutation[3]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 2;
  }
  if (v[permutation[4]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 4;
  }
  if (v[permutation[5]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 8;
  }
  if (v[permutation[6]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 16;
  }
  if (v[permutation[7]] > this->scaledMaterialFractionThreshold)
  {
    caseIdx += 32;
  }
  int* mask = MIF_FRAGMENT_CONNECT_CORNER_MASK[caseIdx];

  // if (mask[0]) { v[permutation[0]] = 0.0;} // mask[0] is always 0
  // Does setting to 0 cause degenerate triangles?
  // 1/4 is arbitrary value less than 1/2.
  if (mask[1] == 1)
  {
    v[permutation[1]] *= 0.25;
  }
  if (mask[2] == 1)
  {
    v[permutation[2]] *= 0.25;
  }
  if (mask[3] == 1)
  {
    v[permutation[3]] *= 0.25;
  }
  if (mask[4] == 1)
  {
    v[permutation[4]] *= 0.25;
  }
  if (mask[5] == 1)
  {
    v[permutation[5]] *= 0.25;
  }
  if (mask[6] == 1)
  {
    v[permutation[6]] *= 0.25;
  }
  if (mask[7] == 1)
  {
    v[permutation[7]] *= 0.25;
  }
  // This is for the unique case with two opposite corners low and
  // everything else high.  Keep the two surfaces from touching.
  if (mask[6] == 2)
  {
    v[permutation[6]] = this->scaledMaterialFractionThreshold + v[permutation[6]];
  }

  // cell centered data interpolated to the current node
  double centerValue = (v[0] + v[1] + v[2] + v[3] + v[4] + v[5] + v[6] + v[7]) * 0.125;

  // Compute the gradient after a threshold.
  double t000 = 0.0;
  double t001 = 0.0;
  double t010 = 0.0;
  double t011 = 0.0;
  double t100 = 0.0;
  double t101 = 0.0;
  double t110 = 0.0;
  double t111 = 0.0;
  if (v[0] > this->scaledMaterialFractionThreshold)
  {
    t000 = 1.0;
  }
  if (v[1] > this->scaledMaterialFractionThreshold)
  {
    t001 = 1.0;
  }
  if (v[2] > this->scaledMaterialFractionThreshold)
  {
    t010 = 1.0;
  }
  if (v[3] > this->scaledMaterialFractionThreshold)
  {
    t011 = 1.0;
  }
  if (v[4] > this->scaledMaterialFractionThreshold)
  {
    t100 = 1.0;
  }
  if (v[5] > this->scaledMaterialFractionThreshold)
  {
    t101 = 1.0;
  }
  if (v[6] > this->scaledMaterialFractionThreshold)
  {
    t110 = 1.0;
  }
  if (v[7] > this->scaledMaterialFractionThreshold)
  {
    t111 = 1.0;
  }

  // We use the gradient ater threshold to choose a direction
  // to move the point.  After considering he discussion about
  // clamping below, we should zeroout iterators that are not
  // face connected (after threshold) to the iterator/voxel
  // that is generating this face.  We do not know which iterator
  // that is because it was not passed in .......

  double g[3]; // Gradient at center.
  g[2] = -t000 - t001 - t010 - t011 + t100 + t101 + t110 + t111;
  g[1] = -t000 - t001 + t010 + t011 - t100 - t101 + t110 + t111;
  g[0] = -t000 + t001 - t010 + t011 - t100 + t101 - t110 + t111;
  // This is unusual but it can happen with a checkerboard pattern.
  // We should break the symmetry and choose a direction ...
  // The masking should take care of this, so this case should never occur.
  // It does occur at boundaries when all voxels (even padded ones)
  // are high.
  if (g[0] == 0.0 && g[1] == 0.0 && g[2] == 0.0)
  {
    // vtkWarningMacro("Masking failed.");
    displacmentFactors[0] = displacmentFactors[1] = displacmentFactors[2] = 0.0;
    return 0;
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
  if (tmp > max)
  {
    max = tmp;
  }
  tmp = fabs(g[2]);
  if (tmp > max)
  {
    max = tmp;
  }
  tmp = 0.5 / max;
  g[0] *= tmp;
  g[1] *= tmp;
  g[2] *= tmp;

  // g is on the surface of a unit cube.

  // We have a problem that shows up when clipping with an off axis plane.
  // Volume fraction increases as square or cube of displacement along
  // diagonal gradients.  We assume linear interpolation which causes
  // ugly bumps on the surface.  It is most visible at resolution boundaries
  // because we assume interpolation occuse on the scale of the smallest cell
  // which is not correct for the larger cells.
  // I do not know if a dual-grid surface algorithm would have the same problem.

  // Compute interpolated surface value
  // with a tri-linear interpolation.
  double surfaceValue;
  surfaceValue = v[0] * (0.5 - g[0]) * (0.5 - g[1]) * (0.5 - g[2]) +
    v[1] * (0.5 + g[0]) * (0.5 - g[1]) * (0.5 - g[2]) +
    v[2] * (0.5 - g[0]) * (0.5 + g[1]) * (0.5 - g[2]) +
    v[3] * (0.5 + g[0]) * (0.5 + g[1]) * (0.5 - g[2]) +
    v[4] * (0.5 - g[0]) * (0.5 - g[1]) * (0.5 + g[2]) +
    v[5] * (0.5 + g[0]) * (0.5 - g[1]) * (0.5 + g[2]) +
    v[6] * (0.5 - g[0]) * (0.5 + g[1]) * (0.5 + g[2]) +
    v[7] * (0.5 + g[0]) * (0.5 + g[1]) * (0.5 + g[2]);

  // Compute how far to the surface we must travel.
  double k = (this->scaledMaterialFractionThreshold - centerValue) / (surfaceValue - centerValue);

  // Assume square (x^2) interpolation gives a little better off axis clipping.
  // k = sqrt(k);

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

  if (caseIdx == 46 || caseIdx == 54 || caseIdx == 62)
  {
    return 2;
  }
  if (caseIdx == 43 || caseIdx == 57 || caseIdx == 59)
  {
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
// Pass the non displaced corner location in the point argument.
// It will be modified with the sub voxel displacement.
// The return value indicates that an edge may be non manifold.
// It returns the y or z axis index of the edge that may be non manifold.
int vtkMaterialInterfaceFilter::SubVoxelPositionCorner(double* point,
  vtkMaterialInterfaceFilterIterator* pointNeighborIterators[8], int rootNeighborIdx, int faceAxis)
{
  int retVal;

  double displacementFactors[3];
  retVal = this->ComputeDisplacementFactors(
    pointNeighborIterators, displacementFactors, rootNeighborIdx, faceAxis);

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
  double* hEdge0 = nullptr;
  double* hEdge1 = nullptr;
  double* hEdge2 = nullptr;
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
    point[ii] += hEdge0[ii] * displacementFactors[0] + hEdge1[ii] * displacementFactors[1] +
      hEdge2[ii] * displacementFactors[2];
  }

  if (this->ClipWithPlane)
  {
    double projection;
    projection = (point[0] - this->ClipCenter[0]) * this->ClipPlaneNormal[0];
    projection += (point[1] - this->ClipCenter[1]) * this->ClipPlaneNormal[1];
    projection += (point[2] - this->ClipCenter[2]) * this->ClipPlaneNormal[2];
    if (this->ClipDepthMax < projection)
    {
      this->ClipDepthMax = projection;
    }
    if (this->ClipDepthMin > projection)
    {
      this->ClipDepthMin = projection;
    }
  }

  return retVal;
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
void vtkMaterialInterfaceFilter::CreateFace(vtkMaterialInterfaceFilterIterator* in,
  vtkMaterialInterfaceFilterIterator* out, int axis, int outMaxFlag)
{
  if (in->Block == nullptr || in->Block->GetGhostFlag())
  {
    return;
  }

  if (out->Block == nullptr)
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
  vtkMaterialInterfaceFilterIterator* cornerNeighbors[8];
  vtkPoints* points = this->CurrentFragmentMesh->GetPoints(); // TODO for performance store?
  vtkCellArray* polys = this->CurrentFragmentMesh->GetPolys();
  vtkIdType quadCornerIds[4];
  vtkIdType quadMidIds[4];
  vtkIdType triPtIds[3];
  vtkIdType startTriId = polys->GetNumberOfCells();

  // Avoid a warning
  quadMidIds[0] = quadMidIds[1] = quadMidIds[2] = quadMidIds[3] = 0;

  // Compute the corner and edge points (before subpixel positioning).
  // Store the results in ivars.
  this->ComputeFacePoints(in, out, axis, outMaxFlag);
  // Find the neighbor iterators.
  // Store the results in ivars.
  this->ComputeFaceNeighbors(in, out, axis, outMaxFlag);

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
  int inc1 = 1 << ((axis + 1) % 3);
  int inc2 = 1 << ((axis + 2) % 3);
  int i0 = 0;
  int i1 = inc0;
  int i2 = inc1;
  int i3 = inc0 + inc1;
  int i4 = inc2;
  int i5 = inc0 + inc2;
  int i6 = inc1 + inc2;
  int i7 = inc0 + inc1 + inc2;

  int manifoldIssue[4];

  // Do every thing in coordinate system of face.
  // Find the neighbors of each point to position to point off the grid.
  // Now to avoid non-manifold surfaces, I need to avoid merging
  // points from voxels that only share an edge (or corner)
  // We need to determine which index (0-7) represents the
  // voxel right inside the face we are creating.  This is
  // to perform connectivity on the 2x2x2 point neighbors.
  int inNeighborIdx;

  cornerNeighbors[i0] = &(this->FaceNeighbors[0]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[1]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[2]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[3]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[8]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[9]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[10]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[11]);
  inNeighborIdx = outMaxFlag ? i6 : i7; // Face neighbor 10 or 11
  manifoldIssue[0] =
    this->SubVoxelPositionCorner(this->FaceCornerPoints, cornerNeighbors, inNeighborIdx, axis);
  // 1 =>
  quadCornerIds[0] = points->InsertNextPoint(this->FaceCornerPoints);
  cornerNeighbors[i0] = &(this->FaceNeighbors[4]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[5]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[6]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[7]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[12]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[13]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[14]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[15]);
  inNeighborIdx = outMaxFlag ? i4 : i5; // Face neighbor 12 or 13
  manifoldIssue[1] =
    this->SubVoxelPositionCorner(this->FaceCornerPoints + 3, cornerNeighbors, inNeighborIdx, axis);
  quadCornerIds[1] = points->InsertNextPoint(this->FaceCornerPoints + 3);
  cornerNeighbors[i0] = &(this->FaceNeighbors[16]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[17]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[18]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[19]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[24]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[25]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[26]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[27]);
  inNeighborIdx = outMaxFlag ? i2 : i3; // Face neighbor 18 or 19
  manifoldIssue[2] =
    this->SubVoxelPositionCorner(this->FaceCornerPoints + 6, cornerNeighbors, inNeighborIdx, axis);
  quadCornerIds[2] = points->InsertNextPoint(this->FaceCornerPoints + 6);
  cornerNeighbors[i0] = &(this->FaceNeighbors[20]);
  cornerNeighbors[i1] = &(this->FaceNeighbors[21]);
  cornerNeighbors[i2] = &(this->FaceNeighbors[22]);
  cornerNeighbors[i3] = &(this->FaceNeighbors[23]);
  cornerNeighbors[i4] = &(this->FaceNeighbors[28]);
  cornerNeighbors[i5] = &(this->FaceNeighbors[29]);
  cornerNeighbors[i6] = &(this->FaceNeighbors[30]);
  cornerNeighbors[i7] = &(this->FaceNeighbors[31]);
  inNeighborIdx = outMaxFlag ? i0 : i1; // Face neighbor 20 or 21
  manifoldIssue[3] =
    this->SubVoxelPositionCorner(this->FaceCornerPoints + 9, cornerNeighbors, inNeighborIdx, axis);
  quadCornerIds[3] = points->InsertNextPoint(this->FaceCornerPoints + 9);

  // If both corners of an edge have an issue, the we need an extra
  // point on the edge to generate a hole.
  // Create a permutation to convert the world coordinate system axes
  // into the coordinate system of the 2x4x4 face neighbor array.
  int tmp[3];
  tmp[axis] = 0;
  tmp[(axis + 1) % 3] = 1;
  tmp[(axis + 2) % 3] = 2;
  if (manifoldIssue[0] != 0 && manifoldIssue[1] != 0 && tmp[manifoldIssue[0]] == 1 &&
    tmp[manifoldIssue[1]] == 1)
  {
    this->FaceEdgeFlags[0] = 1;
  }

  if (manifoldIssue[0] != 0 && manifoldIssue[2] != 0 && tmp[manifoldIssue[0]] == 2 &&
    tmp[manifoldIssue[2]] == 2)
  {
    this->FaceEdgeFlags[1] = 1;
  }
  if (manifoldIssue[1] != 0 && manifoldIssue[3] != 0 && tmp[manifoldIssue[1]] == 2 &&
    tmp[manifoldIssue[3]] == 2)
  {
    this->FaceEdgeFlags[2] = 1;
  }
  if (manifoldIssue[2] != 0 && manifoldIssue[3] && tmp[manifoldIssue[2]] == 1 &&
    tmp[manifoldIssue[3]] == 1)
  {
    this->FaceEdgeFlags[3] = 1;
  }

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
    // Two choices here (10, 12) because they both are the same voxel.
    inNeighborIdx = outMaxFlag ? i4 : i5;
    this->SubVoxelPositionCorner(this->FaceEdgePoints, cornerNeighbors, inNeighborIdx, axis);
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
    // Two choices here (10, 18) because they both are the same voxel.
    inNeighborIdx = outMaxFlag ? i2 : i3;
    this->SubVoxelPositionCorner(this->FaceEdgePoints + 3, cornerNeighbors, inNeighborIdx, axis);
    quadMidIds[1] = points->InsertNextPoint(this->FaceEdgePoints + 3);
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
    // Two choices here (12, 20) because they both are the same voxel.
    inNeighborIdx = outMaxFlag ? i0 : i1;
    this->SubVoxelPositionCorner(this->FaceEdgePoints + 6, cornerNeighbors, inNeighborIdx, axis);
    quadMidIds[2] = points->InsertNextPoint(this->FaceEdgePoints + 6);
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
    // Two choices here (18, 20) because they both are the same voxel.
    inNeighborIdx = outMaxFlag ? i0 : i1;
    this->SubVoxelPositionCorner(this->FaceEdgePoints + 9, cornerNeighbors, inNeighborIdx, axis);
    quadMidIds[3] = points->InsertNextPoint(this->FaceEdgePoints + 9);
  }

  // Now there are 9 possibilities
  // (10 if you count the two ways to triangulate the simple quad).
  // No edges, $ cases with one mid point, 4 cases with two mid points.
  // That is all because the face is always the smallest of the two in/out voxels.
  int caseIdx = this->FaceEdgeFlags[0] | (this->FaceEdgeFlags[1] << 1) |
    (this->FaceEdgeFlags[2] << 2) | (this->FaceEdgeFlags[3] << 3);

  // c2 e3 c3
  // e1    e2
  // c0 e0 c1

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
      double d0011 = 0.0;
      double d0110 = 0.0;
      double* pt00 = this->FaceCornerPoints;
      double* pt01 = this->FaceCornerPoints + 3;
      double* pt10 = this->FaceCornerPoints + 6;
      double* pt11 = this->FaceCornerPoints + 9;
      for (int ii = 0; ii < 3; ++ii)
      {
        double tmp2 = pt00[ii] - pt11[ii];
        d0011 += tmp2 * tmp2;
        tmp2 = pt01[ii] - pt10[ii];
        d0110 += tmp2 * tmp2;
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
  for (int i = 0; i < this->NToIntegrate; ++i)
  {
    // original
    vtkDataArray* srcArray = in->Block->GetIntegratedArray(i);
    int nComps = srcArray->GetNumberOfComponents();
    thisTup.resize(nComps);
    CopyTuple(&thisTup[0], srcArray, nComps, in->FlatIndex);

    // fragment
    vtkDoubleArray* destArray =
      dynamic_cast<vtkDoubleArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray(i));
    for (vtkIdType ii = 0; ii < numTris; ++ii)
    {
      destArray->InsertNextTuple(&thisTup[0]);
    }
  }
// Cell data attributes for debugging.
#ifdef vtkMaterialInterfaceFilterDEBUG
  vtkIntArray* levelArray =
    dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("Level"));

  vtkIntArray* blockIdArray =
    dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("BlockId"));

  vtkIntArray* procIdArray =
    dynamic_cast<vtkIntArray*>(this->CurrentFragmentMesh->GetCellData()->GetArray("ProcId"));

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
void vtkMaterialInterfaceFilter::ComputeFacePoints(vtkMaterialInterfaceFilterIterator* in,
  vtkMaterialInterfaceFilterIterator* out, int axis, int outMaxFlag)
{
  vtkMaterialInterfaceFilterIterator* smaller;
  double* origin;
  double* spacing;
  int maxFlag;
  int axis1 = (axis + 1) % 3;
  int axis2 = (axis + 2) % 3;

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
  this->FaceCornerPoints[0] = this->FaceCornerPoints[3] = this->FaceCornerPoints[6] =
    this->FaceCornerPoints[9] = faceOrigin[0];
  this->FaceCornerPoints[1] = this->FaceCornerPoints[4] = this->FaceCornerPoints[7] =
    this->FaceCornerPoints[10] = faceOrigin[1];
  this->FaceCornerPoints[2] = this->FaceCornerPoints[5] = this->FaceCornerPoints[8] =
    this->FaceCornerPoints[11] = faceOrigin[2];
  // Now offset them to the corners.
  this->FaceCornerPoints[3 + axis1] += spacing[axis1];
  this->FaceCornerPoints[9 + axis1] += spacing[axis1];
  this->FaceCornerPoints[6 + axis2] += spacing[axis2];
  this->FaceCornerPoints[9 + axis2] += spacing[axis2];

  // Now do the same for the edge points
  //   3
  // 1   2
  //   0
  // First set them all to the origin.
  this->FaceEdgePoints[0] = this->FaceEdgePoints[3] = this->FaceEdgePoints[6] =
    this->FaceEdgePoints[9] = faceOrigin[0];
  this->FaceEdgePoints[1] = this->FaceEdgePoints[4] = this->FaceEdgePoints[7] =
    this->FaceEdgePoints[10] = faceOrigin[1];
  this->FaceEdgePoints[2] = this->FaceEdgePoints[5] = this->FaceEdgePoints[8] =
    this->FaceEdgePoints[11] = faceOrigin[2];
  // Now offset the points to the middle of the edges.
  this->FaceEdgePoints[axis1] += halfSpacing[axis1];
  this->FaceEdgePoints[9 + axis1] += halfSpacing[axis1];
  this->FaceEdgePoints[6 + axis1] += spacing[axis1];
  this->FaceEdgePoints[3 + axis2] += halfSpacing[axis2];
  this->FaceEdgePoints[6 + axis2] += halfSpacing[axis2];
  this->FaceEdgePoints[9 + axis2] += spacing[axis2];
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::ComputeFaceNeighbors(vtkMaterialInterfaceFilterIterator* in,
  vtkMaterialInterfaceFilterIterator* out, int axis, int outMaxFlag)
{
  int axis1 = (axis + 1) % 3;
  int axis2 = (axis + 2) % 3;

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
    if (!outMaxFlag)
    {
      faceIndex[axis] += 1;
    }
  }
  faceIndex[0] = faceIndex[0] << 1;
  faceIndex[1] = faceIndex[1] << 1;
  faceIndex[2] = faceIndex[2] << 1;

  // Note:  It would really nice to get rid of the outMaxFlag
  // and have the in voxel at 10, 12, 18 and 20.
  // To avoid breaking anything, I will leave it.
  // however, it looks like filling the arrays would be pretty easy
  // with the switch.  Possible the half neighbors might need to
  // be negated.

  // The center four on each side of the face are always the same.
  // The face is the smallest of in and out, so there is no possibility
  // for subdivision.
  if (outMaxFlag)
  {
    this->FaceNeighbors[10] = this->FaceNeighbors[12] = this->FaceNeighbors[18] =
      this->FaceNeighbors[20] = *in;
    this->FaceNeighbors[11] = this->FaceNeighbors[13] = this->FaceNeighbors[19] =
      this->FaceNeighbors[21] = *out;
  }
  else
  {
    this->FaceNeighbors[10] = this->FaceNeighbors[12] = this->FaceNeighbors[18] =
      this->FaceNeighbors[20] = *out;
    this->FaceNeighbors[11] = this->FaceNeighbors[13] = this->FaceNeighbors[19] =
      this->FaceNeighbors[21] = *in;
  }

  // Ok, we have 24 neighbors to compute.
  // How can we do this efficiently?
  // faceIndex and faceLevel here are actually neighborIndex and neighborLevel.
  // Face index starts at (1,1,1)
  // increments: 1, 2, 8
  // Start at the corner and march around the edges.
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 3, this->FaceNeighbors + 11);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 5, this->FaceNeighbors + 3);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 7, this->FaceNeighbors + 5);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 15, this->FaceNeighbors + 7);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 23, this->FaceNeighbors + 15);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 31, this->FaceNeighbors + 23);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 29, this->FaceNeighbors + 31);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 27, this->FaceNeighbors + 29);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 25, this->FaceNeighbors + 27);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 17, this->FaceNeighbors + 25);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 9, this->FaceNeighbors + 17);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 1, this->FaceNeighbors + 9);
  // Now for the other side (min axis).
  faceIndex[axis] -= 1;  // Move to the other layer
  faceIndex[axis1] += 1; // Start below reference block.
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 2, this->FaceNeighbors + 10);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 4, this->FaceNeighbors + 2);
  faceIndex[axis1] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 6, this->FaceNeighbors + 4);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 14, this->FaceNeighbors + 6);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 22, this->FaceNeighbors + 14);
  faceIndex[axis2] += 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 30, this->FaceNeighbors + 22);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 28, this->FaceNeighbors + 30);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 26, this->FaceNeighbors + 28);
  faceIndex[axis1] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 24, this->FaceNeighbors + 26);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 16, this->FaceNeighbors + 24);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 8, this->FaceNeighbors + 16);
  faceIndex[axis2] -= 1;
  this->FindNeighbor(faceIndex, faceLevel, this->FaceNeighbors + 0, this->FaceNeighbors + 8);

  // Split edges if neighbors are a higher level than face.
  --faceLevel;
  this->FaceEdgeFlags[0] = 0;
  // Checking equivalences (this->FaceNeighbor[2] != this->FaceNeighbor[4])
  // May be faster and work fine.
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
void vtkMaterialInterfaceFilter::FindNeighbor(int faceIdx[3], int faceLevel,
  vtkMaterialInterfaceFilterIterator* neighbor, vtkMaterialInterfaceFilterIterator* reference)
{
  // Convert the index to the level of the reference block.
  int refIdx[3];
  vtkMaterialInterfaceFilterBlock* refBlock;
  const int* refExt;
  int refLevel;
  int refDist;
  vtkMaterialInterfaceFilterBlock* neighborBlock;
  long neighborDist;
  int neighborLevel;
  const int* neighborExt;

  refBlock = reference->Block;
  refExt = refBlock->GetBaseCellExtent();
  refLevel = refBlock->GetLevel();

  if (refLevel > faceLevel)
  {
    refIdx[0] = faceIdx[0] << (refLevel - faceLevel);
    refIdx[1] = faceIdx[1] << (refLevel - faceLevel);
    refIdx[2] = faceIdx[2] << (refLevel - faceLevel);
  }
  else
  {
    refIdx[0] = faceIdx[0] >> (faceLevel - refLevel);
    refIdx[1] = faceIdx[1] >> (faceLevel - refLevel);
    refIdx[2] = faceIdx[2] >> (faceLevel - refLevel);
  }

  // The index might point to the reference iterator.
  if (refIdx[0] == reference->Index[0] && refIdx[1] == reference->Index[1] &&
    refIdx[2] == reference->Index[2])
  {
    *neighbor = *reference;
    return;
  }

  // Find the block the neighbor is in.

  // I had trouble traversing into ghost blocks that were
  // not connected to the block I wanted.
  // I am going to define a proximity measure that is independent of
  // level.
  refDist = this->ComputeProximity(faceIdx, faceLevel, refExt, refLevel);

  // The distance may never reach 0 because on the boundary blocks
  // there is no neighbor.  We pad the block by returning closest
  // block possible.
  int changed = 1;

  while (refDist > 0 && changed)
  {
    changed = 0;
    // Check each axis and direction for neighbor blocks
    // that are closer to the face index.
    for (int axis = 0; axis < 3; ++axis)
    {
      int numNeighbors;
      int minIdx = 2 * axis;
      int maxIdx = minIdx + 1;
      // Min direction
      numNeighbors = refBlock->GetNumberOfFaceNeighbors(minIdx);
      if (refIdx[axis] < refExt[minIdx] && numNeighbors > 0)
      {
        // Look through all the neighbors move when it gets us closer to  goal.
        // Soon as the ref block changes, start over.
        for (int neighborIdx = 0; neighborIdx < numNeighbors && !changed; ++neighborIdx)
        {
          neighborBlock = refBlock->GetFaceNeighbor(minIdx, neighborIdx);
          neighborExt = neighborBlock->GetBaseCellExtent();
          neighborLevel = neighborBlock->GetLevel();
          neighborDist = this->ComputeProximity(faceIdx, faceLevel, neighborExt, neighborLevel);
          if (neighborDist < refDist)
          {
            changed = 1;
            refBlock = neighborBlock;
            refExt = neighborExt;
            if (neighborLevel > faceLevel)
            {
              refIdx[0] = faceIdx[0] << (neighborLevel - faceLevel);
              refIdx[1] = faceIdx[1] << (neighborLevel - faceLevel);
              refIdx[2] = faceIdx[2] << (neighborLevel - faceLevel);
            }
            else
            {
              refIdx[0] = faceIdx[0] >> (faceLevel - neighborLevel);
              refIdx[1] = faceIdx[1] >> (faceLevel - neighborLevel);
              refIdx[2] = faceIdx[2] >> (faceLevel - neighborLevel);
            }
            refLevel = neighborLevel;
            refDist = neighborDist;
          }
        }
      }
      // Max direction
      numNeighbors = refBlock->GetNumberOfFaceNeighbors(maxIdx);
      if (refIdx[axis] > refExt[maxIdx] && numNeighbors > 0)
      {
        // Look through all the neighbors move when it gets us closer to  goal.
        // Soon as the ref block changes, start over.
        for (int neighborIdx = 0; neighborIdx < numNeighbors && !changed; ++neighborIdx)
        {
          neighborBlock = refBlock->GetFaceNeighbor(maxIdx, neighborIdx);
          neighborExt = neighborBlock->GetBaseCellExtent();
          neighborLevel = neighborBlock->GetLevel();
          neighborDist = this->ComputeProximity(faceIdx, faceLevel, neighborExt, neighborLevel);
          if (neighborDist < refDist)
          {
            changed = 1;
            refBlock = neighborBlock;
            refExt = neighborExt;
            if (neighborLevel > faceLevel)
            {
              refIdx[0] = faceIdx[0] << (neighborLevel - faceLevel);
              refIdx[1] = faceIdx[1] << (neighborLevel - faceLevel);
              refIdx[2] = faceIdx[2] << (neighborLevel - faceLevel);
            }
            else
            {
              refIdx[0] = faceIdx[0] >> (faceLevel - neighborLevel);
              refIdx[1] = faceIdx[1] >> (faceLevel - neighborLevel);
              refIdx[2] = faceIdx[2] >> (faceLevel - neighborLevel);
            }
            refLevel = neighborLevel;
            refDist = neighborDist;
          }
        }
      }
    }
  }

  // We have a block
  // clamp the neighbor index to pad the volume
  if (refIdx[0] < refExt[0])
  {
    refIdx[0] = refExt[0];
  }
  if (refIdx[0] > refExt[1])
  {
    refIdx[0] = refExt[1];
  }
  if (refIdx[1] < refExt[2])
  {
    refIdx[1] = refExt[2];
  }
  if (refIdx[1] > refExt[3])
  {
    refIdx[1] = refExt[3];
  }
  if (refIdx[2] < refExt[4])
  {
    refIdx[2] = refExt[4];
  }
  if (refIdx[2] > refExt[5])
  {
    refIdx[2] = refExt[5];
  }

  neighbor->Block = refBlock;
  neighbor->Index[0] = refIdx[0];
  neighbor->Index[1] = refIdx[1];
  neighbor->Index[2] = refIdx[2];
  const int* incs = refBlock->GetCellIncrements();
  int offset = (refIdx[0] - refExt[0]) * incs[0] + (refIdx[1] - refExt[2]) * incs[1] +
    (refIdx[2] - refExt[4]) * incs[2];
  neighbor->FragmentIdPointer = refBlock->GetBaseFragmentIdPointer() + offset;
  neighbor->VolumeFractionPointer = refBlock->GetBaseVolumeFractionPointer() + offset;
  neighbor->FlatIndex = refBlock->GetBaseFlatIndex() + offset;
}

//----------------------------------------------------------------------------
long vtkMaterialInterfaceFilter::ComputeProximity(
  const int faceIdx[3], int faceLevel, const int ext[6], int refLevel)
{
  // We need a distance that is independent of level.
  // All neighbors should be no more than one level away from the faces level.
  // Lets use 2 above for a safety margin.
  long distance = 0;
  long min;
  long max;
  long idx;

  idx = faceIdx[0] << 2;
  min = ext[0] << (2 + faceLevel - refLevel);
  max = ((ext[1] + 1) << (2 + faceLevel - refLevel)) - 1;
  if (idx < min)
  {
    distance += (min - idx);
  }
  else if (idx > max)
  {
    distance += (idx - max);
  }

  idx = faceIdx[1] << 2;
  min = ext[2] << (2 + faceLevel - refLevel);
  max = ((ext[3] + 1) << (2 + faceLevel - refLevel)) - 1;
  if (idx < min)
  {
    distance += (min - idx);
  }
  else if (idx > max)
  {
    distance += (idx - max);
  }

  idx = faceIdx[2] << 2;
  min = ext[4] << (2 + faceLevel - refLevel);
  max = ((ext[5] + 1) << (2 + faceLevel - refLevel)) - 1;
  if (idx < min)
  {
    distance += (min - idx);
  }
  else if (idx > max)
  {
    distance += (idx - max);
  }

  return distance;
}

//----------------------------------------------------------------------------
// This is for getting the neighbors necessary to place face points.
// If the next iterator is out of bounds, the last iterator is duplicated.
void vtkMaterialInterfaceFilter::GetNeighborIteratorPad(vtkMaterialInterfaceFilterIterator* next,
  vtkMaterialInterfaceFilterIterator* iterator, int axis0, int maxFlag0, int axis1, int maxFlag1,
  int axis2, int maxFlag2)
{
  if (iterator->VolumeFractionPointer == nullptr)
  {
    vtkErrorMacro("Error empty input block.  Cannot find neighbor.");
    *next = *iterator;
    return;
  }
  this->GetNeighborIterator(next, iterator, axis0, maxFlag0, axis1, maxFlag1, axis2, maxFlag2);

  if (next->VolumeFractionPointer == nullptr)
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
void vtkMaterialInterfaceFilter::GetNeighborIterator(vtkMaterialInterfaceFilterIterator* next,
  vtkMaterialInterfaceFilterIterator* iterator, int axis0, int maxFlag0, int axis1, int maxFlag1,
  int axis2, int maxFlag2)
{
  if (iterator->Block == nullptr)
  { // No input, no output.  This should not happen.
    vtkWarningMacro("Can not find neighbor for NULL block.");
    *next = *iterator;
    return;
  }

  const int* ext;
  ext = iterator->Block->GetBaseCellExtent();
  int incs[3];
  iterator->Block->GetCellIncrements(incs);

  if (maxFlag0 && iterator->Index[axis0] < ext[2 * axis0 + 1])
  { // Neighbor is inside this block.
    *next = *iterator;
    next->Index[axis0] += 1;
    next->VolumeFractionPointer += incs[axis0];
    next->FragmentIdPointer += incs[axis0];
    next->FlatIndex += incs[axis0];
    return;
  }
  if (!maxFlag0 && iterator->Index[axis0] > ext[2 * axis0])
  { // Neighbor is inside this block.
    *next = *iterator;
    next->Index[axis0] -= 1;
    next->VolumeFractionPointer -= incs[axis0];
    next->FragmentIdPointer -= incs[axis0];
    next->FlatIndex -= incs[axis0];
    return;
  }
  // Look for a neighboring block.
  vtkMaterialInterfaceFilterBlock* block;
  int num, idx;
  num = iterator->Block->GetNumberOfFaceNeighbors(2 * axis0 + maxFlag0);
  for (idx = 0; idx < num; ++idx)
  {
    block = iterator->Block->GetFaceNeighbor(2 * axis0 + maxFlag0, idx);
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
        next->Index[axis0] =
          next->Index[axis0] >> (iterator->Block->GetLevel() - block->GetLevel());
      }
      else
      {
        next->Index[axis0] =
          next->Index[axis0] >> (iterator->Block->GetLevel() - block->GetLevel());
        next->Index[axis0] -= 1;
      }
      // maxFlags for axis1 and 2 do nothing in this case.
      next->Index[axis1] = next->Index[axis1] >> (iterator->Block->GetLevel() - block->GetLevel());
      next->Index[axis2] = next->Index[axis2] >> (iterator->Block->GetLevel() - block->GetLevel());
    }
    else if (iterator->Block->GetLevel() < block->GetLevel())
    { // Going to a higher level.
      if (maxFlag0)
      {
        next->Index[axis0] += 1;
        next->Index[axis0] = next->Index[axis0]
          << (block->GetLevel() - iterator->Block->GetLevel());
      }
      else
      {
        next->Index[axis0] = next->Index[axis0]
          << (block->GetLevel() - iterator->Block->GetLevel());
        next->Index[axis0] -= 1;
      }
      if (maxFlag1)
      {
        next->Index[axis1] =
          ((next->Index[axis1] + 1) << (block->GetLevel() - iterator->Block->GetLevel())) - 1;
      }
      else
      {
        next->Index[axis1] = next->Index[axis1]
          << (block->GetLevel() - iterator->Block->GetLevel());
      }
      if (maxFlag2)
      {
        next->Index[axis2] =
          ((next->Index[axis2] + 1) << (block->GetLevel() - iterator->Block->GetLevel())) - 1;
      }
      else
      {
        next->Index[axis2] = next->Index[axis2]
          << (block->GetLevel() - iterator->Block->GetLevel());
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
      int offset = (next->Index[0] - ext[0]) * incs[0] + (next->Index[1] - ext[2]) * incs[1] +
        (next->Index[2] - ext[4]) * incs[2];
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
inline int vtkMaterialInterfaceFilter::Accumulate(double* dest, // scalar/vector result
  vtkDataArray* src,                                            // array to accumulate from
  int nComps,                                                   //
  int srcCellIndex,                                             // which cell
  double weight) // weight of contributions(one for each component)
{
  // convert cell index to array index
  int srcIndex = nComps * srcCellIndex;
  // accumulate based on data types
  switch (src->GetDataType())
  {
    case VTK_FLOAT:
    {
      float* thisTuple = dynamic_cast<vtkFloatArray*>(src)->GetPointer(srcIndex);
      for (int q = 0; q < nComps; ++q)
      {
        dest[q] += thisTuple[q] * weight;
      }
    }
    break;
    case VTK_DOUBLE:
    {
      double* thisTuple = dynamic_cast<vtkDoubleArray*>(src)->GetPointer(srcIndex);
      for (int q = 0; q < nComps; ++q)
      {
        dest[q] += thisTuple[q] * weight;
      }
    }
    break;
    default:
      assert("This data type is unsupported." && 0);
      return 0;
      break;
  }
  return 1;
}

// integration helper, returns 0 if the source array
// type is unsupported.
inline int vtkMaterialInterfaceFilter::AccumulateMoments(double* moments, // =(Myz, Mxz, Mxy, m)
  vtkDataArray* massArray,                                                //
  int srcCellIndex,                                                       // from which cell in mass
  double* X)                                                              // =(x,y,z)
{
  // accumulate based on data types
  switch (massArray->GetDataType())
  {
    case VTK_FLOAT:
    {
      float* mass = dynamic_cast<vtkFloatArray*>(massArray)->GetPointer(srcCellIndex);
      for (int q = 0; q < 3; ++q)
      {
        moments[q] += mass[0] * X[q];
      }
      moments[3] += mass[0];
    }
    break;
    case VTK_DOUBLE:
    {
      double* mass = dynamic_cast<vtkDoubleArray*>(massArray)->GetPointer(srcCellIndex);
      for (int q = 0; q < 3; ++q)
      {
        moments[q] += mass[0] * X[q];
      }
      moments[3] += mass[0];
    }
    break;
    default:
      assert("This data type is unsupported." && 0);
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
void vtkMaterialInterfaceFilter::ConnectFragment(vtkMaterialInterfaceFilterRingBuffer* queue)
{
  while (queue->GetSize())
  {
    // Get the next voxel/iterator to search.
    vtkMaterialInterfaceFilterIterator iterator;
    queue->Pop(&iterator);
    // Lets integrate when we remove the iterator from the queue.
    // We could also do it when we add the iterator to the queue, but
    // the adds occur in so many places.
    if (iterator.Block->GetGhostFlag() == 0)
    {
      // accumulate fragment volume
      const double* dX = iterator.Block->GetSpacing();
#ifdef USE_VOXEL_VOLUME
      double voxelVolumeFrac = dX[0] * dX[1] * dX[2];
#else
      double voxelVolumeFrac =
        dX[0] * dX[1] * dX[2] * (double)(*(iterator.VolumeFractionPointer)) / 255.0;
#endif
      this->FragmentVolume += voxelVolumeFrac;
      // The clip depth is accumulated in SubvoxelPositionCorner.
      // accumulate volume weighted average
      for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
      {
        vtkDataArray* arrayToIntegrate = iterator.Block->GetVolumeWtdAvgArray(i);
        int nComps = arrayToIntegrate->GetNumberOfComponents();
        this->Accumulate(&this->FragmentVolumeWtdAvg[i][0], arrayToIntegrate, nComps,
          iterator.FlatIndex, voxelVolumeFrac);
      }
      // accumulate mass weighted average
      // Accumulate mass and moments
      if (this->ComputeMoments)
      {
        vtkDataArray* massArray = iterator.Block->GetMassArray();
        // mass and moments
        const double* X0 = iterator.Block->GetOrigin();
        double X[3] = { X0[0] + dX[0] * (0.5 + iterator.Index[0]),
          X0[1] + dX[1] * (0.5 + iterator.Index[1]), X0[2] + dX[2] * (0.5 + iterator.Index[2]) };
        this->AccumulateMoments(&this->FragmentMoment[0], massArray, iterator.FlatIndex, X);
        // mass weighted averages
        double voxelMass;
        massArray->GetTuple(iterator.FlatIndex, &voxelMass);
        for (int i = 0; i < this->NMassWtdAvgs; ++i)
        {
          vtkDataArray* arrayToIntegrate = iterator.Block->GetMassWtdAvgArray(i);
          int nComps = arrayToIntegrate->GetNumberOfComponents();
          this->Accumulate(&this->FragmentMassWtdAvg[i][0], arrayToIntegrate, nComps,
            iterator.FlatIndex, voxelMass);
        }
      }
      // accumulate sum
      for (int i = 0; i < this->NToSum; ++i)
      {
        vtkDataArray* arrayToIntegrate = iterator.Block->GetArrayToSum(i);
        int nComps = arrayToIntegrate->GetNumberOfComponents();
        this->Accumulate(
          &this->FragmentSum[i][0], arrayToIntegrate, nComps, iterator.FlatIndex, 1.0);
      }
    }

    // Create another iterator on the stack for recursion.
    vtkMaterialInterfaceFilterIterator next;

    // Look at the face connected neighbors and recurse.
    // We are not on the border and volume fraction of neighbor is high and
    // we have not visited the voxel yet.
    for (int ii = 0; ii < 3; ++ii)
    {
      // I would make these variables to make the computation more clear,
      // but they would accumulate on the stack.
      // axis0 = ii;
      // axis1 = (ii+1)%3;
      // axis2 = (ii+2)%3;
      // idxMin = 2*ii;
      // idxMax = 2*ii+1this->IndexMax+1;
      // "Left"/min
      this->GetNeighborIterator(&next, &iterator, ii, 0, (ii + 1) % 3, 0, (ii + 2) % 3, 0);

      if (next.VolumeFractionPointer == nullptr ||
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
        vtkMaterialInterfaceFilterIterator next2;
        bool threeDimFlag = next.Block->GetBaseCellExtent()[4] < next.Block->GetBaseCellExtent()[5];
        // Take the first neighbor found and move +Y
        if (ii != 1 || threeDimFlag)
        { // stupid after the fact way of dealing with 2d AMR input.
          this->GetNeighborIterator(&next2, &next, (ii + 1) % 3, 1, (ii + 2) % 3, 0, ii, 0);
          if (next2.VolumeFractionPointer == nullptr ||
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
        }
        // Take the fist iterator found and move +Z
        if (ii != 0 || threeDimFlag)
        { // stupid after the fact way of dealing with 2d AMR input.
          this->GetNeighborIterator(&next2, &next, (ii + 2) % 3, 1, ii, 0, (ii + 1) % 3, 0);
          if (next2.VolumeFractionPointer == nullptr ||
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
        }
        // To get the +Y+Z start with the +Z iterator and move +Y put results in "next"
        if (next2.Block && threeDimFlag)
        {
          this->GetNeighborIterator(&next, &next2, (ii + 1) % 3, 1, (ii + 2) % 3, 0, ii, 0);
          if (next.VolumeFractionPointer == nullptr ||
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
      this->GetNeighborIterator(&next, &iterator, ii, 1, (ii + 1) % 3, 0, (ii + 2) % 3, 0);
      if (next.VolumeFractionPointer == nullptr ||
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
        vtkMaterialInterfaceFilterIterator next2;
        bool threeDimFlag = next.Block->GetBaseCellExtent()[4] < next.Block->GetBaseCellExtent()[5];
        // Take the first neighbor found and move +Y
        if (ii != 1 || threeDimFlag)
        { // stupid after the fact way of dealing with 2d AMR input.
          this->GetNeighborIterator(&next2, &next, (ii + 1) % 3, 1, (ii + 2) % 3, 0, ii, 0);
          if (next2.VolumeFractionPointer == nullptr ||
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
        }
        // Take the fist iterator found and move +Z
        if (ii != 0 || threeDimFlag)
        { // stupid after the fact way of dealing with 2d AMR input.
          this->GetNeighborIterator(&next2, &next, (ii + 2) % 3, 1, ii, 0, (ii + 1) % 3, 0);
          if (next2.VolumeFractionPointer == nullptr ||
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
        }
        // To get the +Y+Z start with the +Z iterator and move +Y put results in "next"
        if (next2.Block && threeDimFlag)
        {
          this->GetNeighborIterator(&next, &next2, (ii + 1) % 3, 1, (ii + 2) % 3, 0, ii, 0);
          if (next.VolumeFractionPointer == nullptr ||
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
void vtkMaterialInterfaceFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO print state
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// Save out the blocks surfaces so we can se what is creating so many ghost cells.
void vtkMaterialInterfaceFilter::SaveBlockSurfaces(const char* fileName)
{
  vtkPolyData* pd = vtkPolyData::New();
  vtkPoints* pts = vtkPoints::New();
  vtkCellArray* faces = vtkCellArray::New();
  vtkIntArray* idArray = vtkIntArray::New();
  vtkIntArray* levelArray = vtkIntArray::New();
  vtkMaterialInterfaceFilterBlock* block;
  const int* ext;
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
    // double *spacing;
    // spacing = block->GetSpacing();
    double spacing[3];
    level = block->GetLevel();
    spacing[0] = this->RootSpacing[0] / (double)(1 << level);
    spacing[1] = this->RootSpacing[1] / (double)(1 << level);
    spacing[2] = this->RootSpacing[2] / (double)(1 << level);
    // spacing[0] = spacing[1] = spacing[2] = 1.0 / (double)(1 << level);
    levelId = block->LevelBlockId;
    // Insert the points.
    pt[0] = ext[0] * spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2] * spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4] * spacing[2] + this->GlobalOrigin[2];
    corners[0] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2] * spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4] * spacing[2] + this->GlobalOrigin[2];
    corners[1] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3] + 1) * spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4] * spacing[2] + this->GlobalOrigin[2];
    corners[2] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3] + 1) * spacing[1] + this->GlobalOrigin[1];
    pt[2] = ext[4] * spacing[2] + this->GlobalOrigin[2];
    corners[3] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2] * spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5] + 1) * spacing[2] + this->GlobalOrigin[2];
    corners[4] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing[0] + this->GlobalOrigin[0];
    pt[1] = ext[2] * spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5] + 1) * spacing[2] + this->GlobalOrigin[2];
    corners[5] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3] + 1) * spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5] + 1) * spacing[2] + this->GlobalOrigin[2];
    corners[6] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing[0] + this->GlobalOrigin[0];
    pt[1] = (ext[3] + 1) * spacing[1] + this->GlobalOrigin[1];
    pt[2] = (ext[5] + 1) * spacing[2] + this->GlobalOrigin[2];
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
  w->SetInputData(pd);
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
void vtkMaterialInterfaceFilter::SaveGhostSurfaces(const char* fileName)
{
  vtkPolyData* pd = vtkPolyData::New();
  vtkPoints* pts = vtkPoints::New();
  vtkCellArray* faces = vtkCellArray::New();
  vtkIntArray* idArray = vtkIntArray::New();
  vtkIntArray* levelArray = vtkIntArray::New();
  vtkMaterialInterfaceFilterBlock* block;
  const int* ext;
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
    pt[0] = ext[0] * spacing;
    pt[1] = ext[2] * spacing;
    pt[2] = ext[4] * spacing;
    corners[0] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing;
    pt[1] = ext[2] * spacing;
    pt[2] = ext[4] * spacing;
    corners[1] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing;
    pt[1] = (ext[3] + 1) * spacing;
    pt[2] = ext[4] * spacing;
    corners[2] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing;
    pt[1] = (ext[3] + 1) * spacing;
    pt[2] = ext[4] * spacing;
    corners[3] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing;
    pt[1] = ext[2] * spacing;
    pt[2] = (ext[5] + 1) * spacing;
    corners[4] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing;
    pt[1] = ext[2] * spacing;
    pt[2] = (ext[5] + 1) * spacing;
    corners[5] = pts->InsertNextPoint(pt);
    pt[0] = ext[0] * spacing;
    pt[1] = (ext[3] + 1) * spacing;
    pt[2] = (ext[5] + 1) * spacing;
    corners[6] = pts->InsertNextPoint(pt);
    pt[0] = (ext[1] + 1) * spacing;
    pt[1] = (ext[3] + 1) * spacing;
    pt[2] = (ext[5] + 1) * spacing;
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
  w->SetInputData(pd);
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
void vtkMaterialInterfaceFilter::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkMaterialInterfaceFilter*>(clientdata)->Modified();
}

// PV interface to volume weighted average arrays
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SelectVolumeWtdAvgArray(const char* arrayName)
{
  this->VolumeWtdAvgArraySelection->AddArray(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectVolumeWtdAvgArray(const char* arrayName)
{
  this->VolumeWtdAvgArraySelection->RemoveArrayByName(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectAllVolumeWtdAvgArrays()
{
  this->VolumeWtdAvgArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetNumberOfVolumeWtdAvgArrays()
{
  return this->VolumeWtdAvgArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkMaterialInterfaceFilter::GetVolumeWtdAvgArrayName(int index)
{
  return this->VolumeWtdAvgArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetVolumeWtdAvgArrayStatus(const char* name)
{
  return this->VolumeWtdAvgArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetVolumeWtdAvgArrayStatus(int index)
{
  return this->VolumeWtdAvgArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetVolumeWtdAvgArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
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
void vtkMaterialInterfaceFilter::SelectMassWtdAvgArray(const char* arrayName)
{
  this->MassWtdAvgArraySelection->AddArray(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectMassWtdAvgArray(const char* arrayName)
{
  this->MassWtdAvgArraySelection->RemoveArrayByName(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectAllMassWtdAvgArrays()
{
  this->MassWtdAvgArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetNumberOfMassWtdAvgArrays()
{
  return this->MassWtdAvgArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkMaterialInterfaceFilter::GetMassWtdAvgArrayName(int index)
{
  return this->MassWtdAvgArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMassWtdAvgArrayStatus(const char* name)
{
  return this->MassWtdAvgArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMassWtdAvgArrayStatus(int index)
{
  return this->MassWtdAvgArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetMassWtdAvgArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
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
void vtkMaterialInterfaceFilter::SelectSummationArray(const char* arrayName)
{
  this->SummationArraySelection->AddArray(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectSummationArray(const char* arrayName)
{
  this->SummationArraySelection->RemoveArrayByName(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectAllSummationArrays()
{
  this->SummationArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetNumberOfSummationArrays()
{
  return this->SummationArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkMaterialInterfaceFilter::GetSummationArrayName(int index)
{
  return this->SummationArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetSummationArrayStatus(const char* name)
{
  return this->SummationArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetSummationArrayStatus(int index)
{
  return this->SummationArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetSummationArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
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
void vtkMaterialInterfaceFilter::SelectMaterialArray(const char* arrayName)
{
  this->MaterialArraySelection->AddArray(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectMaterialArray(const char* arrayName)
{
  this->MaterialArraySelection->RemoveArrayByName(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectAllMaterialArrays()
{
  this->MaterialArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetNumberOfMaterialArrays()
{
  return this->MaterialArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkMaterialInterfaceFilter::GetMaterialArrayName(int index)
{
  return this->MaterialArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMaterialArrayStatus(const char* name)
{
  return this->MaterialArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMaterialArrayStatus(int index)
{
  return this->MaterialArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetMaterialArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
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
void vtkMaterialInterfaceFilter::SelectMassArray(const char* arrayName)
{
  this->MassArraySelection->AddArray(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectMassArray(const char* arrayName)
{
  this->MassArraySelection->RemoveArrayByName(arrayName);
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::UnselectAllMassArrays()
{
  this->MassArraySelection->RemoveAllArrays();
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetNumberOfMassArrays()
{
  return this->MassArraySelection->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkMaterialInterfaceFilter::GetMassArrayName(int index)
{
  return this->MassArraySelection->GetArrayName(index);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMassArrayStatus(const char* name)
{
  return this->MassArraySelection->ArrayIsEnabled(name);
}

//-----------------------------------------------------------------------------
int vtkMaterialInterfaceFilter::GetMassArrayStatus(int index)
{
  return this->MassArraySelection->GetArraySetting(index);
}

//-----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetMassArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if (status)
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
void vtkMaterialInterfaceFilter::SetMaterialFractionThreshold(double fraction)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting MaterialFractionThreshold to " << fraction);

  if (this->MaterialFractionThreshold != fraction)
  {
    // lower bound on MF @ 0.08
    fraction = fraction < 0.08 ? 0.08 : fraction;
    this->MaterialFractionThreshold = fraction;
    this->scaledMaterialFractionThreshold = 255.0 * fraction;
    this->Modified();
  }
}

// PV interface to the upper loading bound
//------------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::SetUpperLoadingBound(int nPolys)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting UpperLoadingBound to "
                << nPolys);

  if (this->UpperLoadingBound != nPolys)
  {
    // clamp between -1 and 2G
    nPolys = nPolys < -1 ? -1 : nPolys;
    nPolys = nPolys > 2000000000 ? 2000000000 : nPolys;
    this->UpperLoadingBound = nPolys;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// This is for finding equivalent fragment ids within a single process.
// I thought of loops and chains but both have problems.
// Chains can leave orphans, loops break when two nodes in the loop are
// equated a second time.
// Lets try a directed tree
void vtkMaterialInterfaceFilter::AddEquivalence(
  vtkMaterialInterfaceFilterIterator* neighbor1, vtkMaterialInterfaceFilterIterator* neighbor2)
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
void vtkMaterialInterfaceFilter::ResolveLocalFragmentGeometry()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::ResolveLocalFragmentGeometry("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
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
  const int localToGlobal = this->LocalToGlobalOffsets[myProcId];

  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert("Couldn't get the resolved fragnments." && resolvedFragments);
  resolvedFragments->SetNumberOfPieces(this->NumberOfResolvedFragments);

  int nFragmentPieces = static_cast<int>(this->FragmentMeshes.size());
  for (int localId = 0; localId < nFragmentPieces; ++localId)
  {
    // find out this guy's global id within this material
    int globalId = this->EquivalenceSet->GetEquivalentSetId(localId + localToGlobal);

    // If we have a mesh that is yet unused then
    // we copy, but if not, it's a local piece that has been
    // resolved and we need to append.
    vtkPolyData* destMesh = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));
    vtkPolyData*& srcMesh = this->FragmentMeshes[localId];
    if (destMesh == nullptr)
    {
      resolvedFragments->SetPiece(globalId, srcMesh);
      // make a note that we have a piece of this fragment
      // and assume for now that we are the owner.
      resolvedFragmentIds.push_back(globalId);
    }
    else
    {
      // merge two local pieces
      vtkAppendPolyData* apf = vtkAppendPolyData::New();
      apf->AddInputData(destMesh);
      apf->AddInputData(srcMesh);
      apf->Update();
      vtkPolyData* mergedMesh = apf->GetOutput();

      // mergedMesh->Register(0); // Do I have to? no because multi piece does it
      resolvedFragments->SetPiece(globalId, mergedMesh);
      apf->Delete();
      ReleaseVtkPointer(srcMesh);
      // destMesh->Delete(); // no because multi piece does it
    }
  }
  // These have been loaded into the resolved fragments
  ClearVectorOfVtkPointers(this->FragmentMeshes);

  // Cull empty fragments. In some cases (eg. a fragment ends up
  // entirely contained some process's ghost blocks) there
  // can be  empty fragments. We will throw them out here
  // as they will be non empty on at least one other process.
  vector<int>::iterator start = resolvedFragmentIds.begin();
  vector<int>::iterator end = resolvedFragmentIds.end();
  vector<int>::iterator shortEnd = end;
  int nLocal = static_cast<int>(resolvedFragmentIds.size());
  for (int localId = 0; localId < nLocal; ++localId)
  {
    int globalId = resolvedFragmentIds[localId];
    vtkPolyData* fragmentMesh = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));
    // Empty fragment?
    if (fragmentMesh->GetNumberOfPoints() == 0)
    {
      // move id to end of the vector
      shortEnd = remove(start, shortEnd, globalId);
      // remove from output data set
      resolvedFragments->SetPiece(globalId, nullptr);
#ifdef vtkMaterialInterfaceFilterDEBUG
      cerr << "[" << __LINE__ << "] " << myProcId << " fragment " << globalId
           << " is empty and will be ignored." << endl;
#endif
    }
  }
  // Resize the id vector to reflect loss of removed ids.
  resolvedFragmentIds.erase(shortEnd, end);
  // Squeeze
  vector<int>(resolvedFragmentIds).swap(resolvedFragmentIds);
}

//----------------------------------------------------------------------------
// Remove duplicate point from local meshes.
void vtkMaterialInterfaceFilter::CleanLocalFragmentGeometry()
{
// TODO  If we clean the
// data and turn transparency on MPI throws an
// error which causes all of the servers to
// terminate. so far I have only seen this
// with np=4 cth-med.
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::CleanLocalFragmentGeometry("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert("Couldn't get the resolved fragnments." && resolvedFragments);
  resolvedFragments->SetNumberOfPieces(this->NumberOfResolvedFragments);

  // Only need to merge points.
  vtkCleanPolyData* cpd = vtkCleanPolyData::New();
// These caused some visual effects(rounded corners etc...)
// cpd->ConvertLinesToPointsOff();
// cpd->ConvertPolysToLinesOff();
// cpd->ConvertStripsToPolysOff();
// cpd->PointMergingOn();

#ifdef vtkMaterialInterfaceFilterDEBUG
  const int myProcId = this->Controller->GetLocalProcessId();
  vtkIdType nInitial = 0;
  vtkIdType nFinal = 0;
#endif
  // clean each frgament mesh we own.
  int nLocal = static_cast<int>(resolvedFragmentIds.size());
  for (int localId = 0; localId < nLocal; ++localId)
  {
    // get the material id
    int fragmentId = resolvedFragmentIds[localId];
    // get the fragment
    vtkPolyData* fragmentMesh = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(fragmentId));
#ifdef vtkMaterialInterfaceFilterDEBUG
    nInitial += fragmentMesh->GetNumberOfPoints();
#endif
    // clean duplicate points
    cpd->SetInputData(fragmentMesh);
    cpd->Update();
    vtkPolyData* cleanedFragmentMesh = cpd->GetOutput();

#ifdef vtkMaterialInterfaceFilterDEBUG
    nFinal += cleanedFragmentMesh->GetNumberOfPoints();
#endif
    // Free unused resources
    cleanedFragmentMesh->Squeeze();
    // Copy and swap dirty old mesh for new cleaned mesh.
    vtkPolyData* cleanedFragmentMeshOut = vtkPolyData::New();
    cleanedFragmentMeshOut->ShallowCopy(cleanedFragmentMesh);
    resolvedFragments->SetPiece(fragmentId, cleanedFragmentMeshOut);
    cleanedFragmentMeshOut->Delete();
  }
  cpd->Delete();
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId << " cleaned " << nInitial - nFinal
       << " points from local fragments. ("
       << (int)(100.0 * (1.0 - (double)nFinal / (double)nInitial) + 0.5) << "%)" << endl;
#endif
}

//----------------------------------------------------------------------------
// Identify fragments who are split across processes. These are always
// generated and copied to the output. We also may compute various
// other geometric attributes here. For fragments whose geometry is
// split across processes, localize points and compute geometric
// attributes. eg OBB, AABB center.
void vtkMaterialInterfaceFilter::ComputeGeometricAttributes()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::ComputeGeometricAttributes("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  vtkCommunicator* comm = this->Controller->GetCommunicator();
  const int controllingProcId = 0;
  const int msgBase = 200000;

  // Here are the fragment pieces we own. Some are completely
  // localized while others are split across processes. Pieces
  // with split geometry will be temporarily localized(via copy), the
  // geometry acted on as a whole and the results distributed back
  // to piece owners. For localized fragments(these aren't pieces)
  // the situation/ is relatively simple--computation can be made
  // directly.
  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert("Couldn't get the resolved fragnments." && resolvedFragments);

  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];
  int nLocal = static_cast<int>(resolvedFragmentIds.size());

  // Start by assuming that all fragment pieces
  // are local(these are not pieces but entire fragments).
  // If we are are running client-server later we search
  // for split fragments and mark them.For any remaining
  // un-marked fragments we can compute geometric attributes
  // directly.
  vector<int>& fragmentSplitMarker = this->FragmentSplitMarker[this->MaterialId];
  // intilize to (bool)0 not split. All local ops
  // on fragments check here. It's copied to geometry.
  fragmentSplitMarker.resize(nLocal, 0);
  //
  if (myProcId == controllingProcId)
  {
    NewVtkArrayPointer(
      this->FragmentSplitGeometry, 1, this->NumberOfResolvedFragments, "SplitGeometry");
    // initialize to 1 (i.e. not split). This holds n-way
    // splitting data, only available on controller. It's
    // copied to the stats output.
    this->FragmentSplitGeometry->FillComponent(0, 1);
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
  if (nProcs > 1)
  {
    vtkMaterialInterfacePieceTransactionMatrix TM;
    TM.Initialize(this->NumberOfResolvedFragments, nProcs);

    // controller receives loading information and
    // builds the transaction matrix.
    if (myProcId == controllingProcId)
    {
      int thisMsgId = msgBase;

      // fragment indexed arrays, with number of polys
      vector<vector<vtkIdType> > loadingArrays;
      loadingArrays.resize(nProcs);
      // Gather loading arrays
      // mine
      this->BuildLoadingArray(loadingArrays[controllingProcId]);
      // others
      for (int procId = 0; procId < nProcs; ++procId)
      {
        if (procId == controllingProcId)
        {
          continue;
        }
        // size of incoming
        int bufSize = 0;
        comm->Receive(&bufSize, 1, procId, thisMsgId);
        // incoming
        vtkIdType* buffer = new vtkIdType[bufSize];
        comm->Receive(buffer, bufSize, procId, thisMsgId + 1);
        this->UnPackLoadingArray(buffer, bufSize, loadingArrays[procId]);
        delete[] buffer;
      }
      ++thisMsgId;
      ++thisMsgId;
#ifdef vtkMaterialInterfaceFilterDEBUG
      cerr << "[" << __LINE__ << "] " << controllingProcId << " loading histogram:" << endl;
      PrintPieceLoadingHistogram(loadingArrays);
#endif

      // Build fragment to proc map
      vtkMaterialInterfaceToProcMap f2pm;
      f2pm.Initialize(nProcs, this->NumberOfResolvedFragments);
      // Build process loading heap.
      vector<vtkMaterialInterfaceProcessLoading> heap(nProcs);
      for (int procId = 0; procId < nProcs; ++procId)
      {
        // sum up the loading contribution from all local fragments
        // and make a note of who owns what.
        vtkMaterialInterfaceProcessLoading& prl = heap[procId];
        prl.Initialize(procId, 0);
        for (int fragmentId = 0; fragmentId < this->NumberOfResolvedFragments; ++fragmentId)
        {
          vtkIdType loading = loadingArrays[procId][fragmentId];
          if (loading > 0)
          {
            prl.UpdateLoadFactor(loading);
            f2pm.SetProcOwnsPiece(procId, fragmentId);
          }
        }
      }
      // heap sort, least loaded process is in element 0.
      // Now we can make intelligent decisions about where to
      // gather geometric attributes. At this point we can but don't ;)
      partial_sort(heap.begin(), heap.end(), heap.end());

#ifdef vtkMaterialInterfaceFilterDEBUG
      cerr << "[" << __LINE__ << "] " << controllingProcId
           << " total loading before fragment localization:" << endl
           << heap;
      vector<int> splitting(nProcs + 1, 0);
      int nSplit = 0;
#endif

      // Who will do attribute processing for fragments
      // that are split? We can weed out processes that are
      // highly loaded and cycle through the group that
      // remains assigning to each until all work has
      // been allotted.
      vtkMaterialInterfaceProcessRing procRing;
      procRing.Initialize(heap, this->UpperLoadingBound);
#ifdef vtkMaterialInterfaceFilterDEBUG
      cerr << "[" << __LINE__ << "] " << controllingProcId << " process ring: ";
      procRing.Print();
      cerr << endl;
#endif

      // Decide who needs to move and build corresponding
      // transaction matrix. Along the way store the geometry
      // split info (this eliminates some communication later).
      int* fragmentSplitGeometry = this->FragmentSplitGeometry->GetPointer(0);
      //
      for (int fragmentId = 0; fragmentId < this->NumberOfResolvedFragments; ++fragmentId)
      {
        int nSplitOver = f2pm.GetProcCount(fragmentId);
        fragmentSplitGeometry[fragmentId] = nSplitOver;
        // if the fragment is split then we need to move him
        if (nSplitOver > 1)
        {
#ifdef vtkMaterialInterfaceFilterDEBUG
          // splitting histogram
          ++splitting[nSplitOver];
          ++nSplit;
#endif
          // who has the pieces?
          vector<int> owners = f2pm.WhoHasAPiece(fragmentId);
          // how much load will he add to the recipient?
          vtkIdType loading = 0;
          for (int i = 0; i < nSplitOver; ++i)
          {
            loading += loadingArrays[owners[i]][fragmentId];
          }
          // who will do processing??
          int recipient = procRing.GetNextId();

          // Add the transactions to make the move, and
          // update current owners loading to reflect the loss.
          vtkMaterialInterfacePieceTransaction ta;
          for (int i = 0; i < nSplitOver; ++i)
          {
            // need to move this piece?
            if (owners[i] == recipient)
            {
              continue;
            }
            // Add the requisite transactions.
            // recipient executes a recv from owner
            ta.Initialize('R', owners[i]);
            TM.PushTransaction(fragmentId, recipient, ta);
            // owner executes a send to recipient
            ta.Initialize('S', recipient);
            TM.PushTransaction(fragmentId, owners[i], ta);
          }
        }
      }
#ifdef vtkMaterialInterfaceFilterDEBUG
      cerr << "[" << __LINE__ << "] " << controllingProcId << " splitting:" << endl;
      PrintHistogram(splitting);
      cerr << "[" << __LINE__ << "] " << myProcId << " total number of fragments "
           << this->NumberOfResolvedFragments << endl;
      cerr << "[" << __LINE__ << "] " << myProcId << " total number split " << nSplit << endl;
// cerr << "[" << __LINE__ << "] "
//       << myProcId
//       << " the transaction matrix is:" << endl;
// TM.Print();
#endif
    }
    // All processes send fragment loading to controller.
    else
    {
      int thisMsgId = msgBase;

      // create and send my loading array
      vtkIdType* buffer = nullptr;
      int bufSize = this->PackLoadingArray(buffer);
      comm->Send(&bufSize, 1, controllingProcId, thisMsgId);
      ++thisMsgId;
      comm->Send(buffer, bufSize, controllingProcId, thisMsgId);
      ++thisMsgId;
    }

    // Broadcast the transaction matrix
    TM.Broadcast(comm, controllingProcId);

    // Prepare for a bunch of inverse searches through
    // local fragment ids. i.e. given a global id find the
    // local id.
    vtkMaterialInterfaceIdList idList;
    idList.Initialize(resolvedFragmentIds, false);

    // Prepare for various computations.
    int nAttributeComps = 0;
    if (!this->ComputeMoments)
    {
      nAttributeComps += 3;
    }
    vtkOBBTree* obbCalc = nullptr;
    if (this->ComputeOBB)
    {
      obbCalc = vtkOBBTree::New();
      nAttributeComps += 15;
    }
    double* attributeCommBuffer = new double[nAttributeComps];
    // localize split geometry and compute attributes.
    for (int fragmentId = 0; fragmentId < this->NumberOfResolvedFragments; ++fragmentId)
    {
      // point buffer
      vtkPointAccumulator<float, vtkFloatArray> accumulator;

      // get my list of transactions for this fragment
      vector<vtkMaterialInterfacePieceTransaction>& transactionList =
        TM.GetTransactions(fragmentId, myProcId);

      // execute
      vtkPolyData* localMesh = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(fragmentId));

      int nTransactions = static_cast<int>(transactionList.size());
      if (nTransactions > 0)
      {
        /// send
        if (transactionList[0].GetType() == 'S')
        {
          assert("Send has more than 1 transaction." && nTransactions == 1);
          assert("Send requires a mesh that is not local." && localMesh != 0);

          // I am sending geometry, hence this is a piece
          // of a split frgament and I need to treat it as
          // such from now on.
          int localId = idList.GetLocalId(fragmentId);
          assert("Fragment id not found." && localId != -1);
          fragmentSplitMarker[localId] = 1;

          // Send the geometry and recvieve the results of
          // the requested computations.
          if (!this->ComputeMoments || this->ComputeOBB)
          {
            vtkMaterialInterfacePieceTransaction& ta = transactionList[0];

            // get the points of this piece
            vtkFloatArray* ptsArray =
              dynamic_cast<vtkFloatArray*>(localMesh->GetPoints()->GetData());
            const vtkIdType bytesPerPoint = 3 * sizeof(float);
            const vtkIdType nPoints = ptsArray->GetNumberOfTuples();
            const vtkIdType bufferSize = bytesPerPoint * nPoints;
            // prepare a comm buffer
            vtkMaterialInterfaceCommBuffer buffer;
            buffer.Initialize(myProcId, 1, bufferSize);
            buffer.SetNumberOfTuples(0, nPoints);
            buffer.Pack(ptsArray);
            // send the buffer in two parts, first the header
            this->Controller->Send(
              buffer.GetHeader(), buffer.GetHeaderSize(), ta.GetRemoteProc(), fragmentId);
            // followed by points
            this->Controller->Send(
              buffer.GetBuffer(), buffer.GetBufferSize(), ta.GetRemoteProc(), 2 * fragmentId);

            // Receive the remotely computed attributes
            this->Controller->Receive(
              attributeCommBuffer, nAttributeComps, ta.GetRemoteProc(), 3 * fragmentId);
            // save results
            double* pBuf = attributeCommBuffer;
            if (!this->ComputeMoments)
            {
              this->FragmentAABBCenters->SetTuple(localId, pBuf);
              pBuf += 3;
            }
            if (this->ComputeOBB)
            {
              this->FragmentOBBs->SetTuple(localId, pBuf);
              pBuf += 15;
            }
          }
        }
        /// receive
        else if (transactionList[0].GetType() == 'R')
        {
          // This fragment is split across processes and
          // I have a piece. From now on I need to treat
          // this fragment as split.
          int localId = -1;
          if (localMesh != nullptr)
          {
            localId = idList.GetLocalId(fragmentId);
            assert("Fragment id not found." && localId != -1);
            fragmentSplitMarker[localId] = 1;
          }

          // Receive the geometry, perform the requested
          // computations and send back the results.
          if (!this->ComputeMoments || this->ComputeOBB)
          {
            for (int i = 0; i < nTransactions; ++i)
            {
              vtkMaterialInterfacePieceTransaction& ta = transactionList[i];

              // prepare the comm buffer to receive attribute data
              // pertaining to a single block(material)
              vtkMaterialInterfaceCommBuffer buffer;
              buffer.SizeHeader(1);
              // receive buffer's header
              this->Controller->Receive(
                buffer.GetHeader(), buffer.GetHeaderSize(), ta.GetRemoteProc(), fragmentId);
              // size buffer via incoming header
              buffer.SizeBuffer();
              // receive points
              this->Controller->Receive(
                buffer.GetBuffer(), buffer.GetBufferSize(), ta.GetRemoteProc(), 2 * fragmentId);
              // unpack points with an explicit copy
              vtkIdType nPoints = buffer.GetNumberOfTuples(0);
              float* writePointer = accumulator.Expand(nPoints);
              buffer.UnPack(writePointer, 3, nPoints, true);
            }
            // append points that I own.
            if (localMesh != nullptr)
            {
              // get the points
              vtkFloatArray* ptsArray =
                dynamic_cast<vtkFloatArray*>(localMesh->GetPoints()->GetData());
              // append
              accumulator.Accumulate(ptsArray);
            }

            // Get the gathered points in vtk form.
            vtkPoints* localizedPoints = accumulator.BuildVtkPoints();
            // Get the AABB and compute its center.
            double aabb[6];
            localizedPoints->GetBounds(aabb);
            double aabbCen[3];
            for (int q = 0, k = 0; q < 3; ++q, k += 2)
            {
              aabbCen[q] = (aabb[k] + aabb[k + 1]) / 2.0;
            }
            double* pBuf = attributeCommBuffer;
            if (!this->ComputeMoments)
            {
              pBuf[0] = aabbCen[0];
              pBuf[1] = aabbCen[1];
              pBuf[2] = aabbCen[2];
              pBuf += 3;
            }
            if (this->ComputeOBB)
            {
              // Compute OBB
              double size[3];
              // I store the results as follows:
              // (c_x,c_y,c_z),(max_x,max_y,max_z),(mid_x,mid_y,mid_z),(min_x,min_y,min_z),|max|,|mid|,|min|
              obbCalc->ComputeOBB(localizedPoints, pBuf, pBuf + 3, pBuf + 6, pBuf + 9, size);
              // compute magnitudes
              for (int q = 0; q < 3; ++q)
              {
                pBuf[12 + q] = 0;
              }
              for (int q = 0; q < 3; ++q)
              {
                pBuf[12] += pBuf[3 + q] * pBuf[3 + q];
                pBuf[13] += pBuf[6 + q] * pBuf[6 + q];
                pBuf[14] += pBuf[9 + q] * pBuf[9 + q];
              }
              for (int q = 0; q < 3; ++q)
              {
                pBuf[12 + q] = sqrt(pBuf[12 + q]);
              }
              // The vtkOBBTree computes axes using covariance
              // which doesn't work well for amr data. Ideally
              // we want the MVBB, so if the AABB is smaller than
              // the OBB, use the AABB instead.
              double obbVolume = pBuf[12] * pBuf[13] * pBuf[14];
              double aabbDx[3];
              aabbDx[0] = aabb[1] - aabb[0];
              aabbDx[1] = aabb[3] - aabb[2];
              aabbDx[2] = aabb[5] - aabb[4];
              double aabbVolume = fabs(aabbDx[0] * aabbDx[1] * aabbDx[2]);
              if (aabbVolume < obbVolume)
              {
                vtkWarningMacro("AABB volume is less than OBB volume, using AABB."
                  << " Block Id:" << this->MaterialId
                  << " Fragment Id:" << this->NumberOfResolvedFragments + fragmentId << endl);
                // corner
                pBuf[0] = aabb[0];
                pBuf[1] = aabb[2];
                pBuf[2] = aabb[4];
                // sort largest to smallest, and track which of min,mid,max
                // are in the x,y, or z directions.
                int maxComp = 0;
                int midComp = 1;
                int minComp = 2;
                if (fabs(aabbDx[0]) < fabs(aabbDx[2]))
                {
                  double tmpDx = aabbDx[0];
                  aabbDx[0] = aabbDx[2];
                  aabbDx[2] = tmpDx;
                  int tmpComp = maxComp;
                  maxComp = minComp;
                  minComp = tmpComp;
                }
                if (fabs(aabbDx[1]) < fabs(aabbDx[2]))
                {
                  double tmpDx = aabbDx[1];
                  aabbDx[1] = aabbDx[2];
                  aabbDx[2] = tmpDx;
                  int tmpComp = midComp;
                  midComp = minComp;
                  minComp = tmpComp;
                }
                if (fabs(aabbDx[0]) < fabs(aabbDx[1]))
                {
                  double tmpDx = aabbDx[0];
                  aabbDx[0] = aabbDx[1];
                  aabbDx[1] = tmpDx;
                  int tmpComp = maxComp;
                  maxComp = midComp;
                  midComp = tmpComp;
                }
                memset(pBuf + 3, 0, 9 * sizeof(double));
                // Set sorted offsets ...
                pBuf[3 + maxComp] = aabbDx[0];
                pBuf[6 + midComp] = aabbDx[1];
                pBuf[9 + minComp] = aabbDx[2];
                // & magnitudes.
                pBuf[12] = fabs(aabbDx[0]);
                pBuf[13] = fabs(aabbDx[1]);
                pBuf[14] = fabs(aabbDx[2]);
              }
              pBuf += 15;
            }

            // send attributes back to piece owners
            for (int i = 0; i < nTransactions; ++i)
            {
              vtkMaterialInterfacePieceTransaction& ta = transactionList[i];
              this->Controller->Send(
                attributeCommBuffer, nAttributeComps, ta.GetRemoteProc(), 3 * fragmentId);
            }
            // If I own a piece save the results, and mark
            // piece as split.
            if (localMesh != nullptr)
            {
              pBuf = attributeCommBuffer;
              if (!this->ComputeMoments)
              {
                this->FragmentAABBCenters->SetTuple(localId, pBuf);
                pBuf += 3;
              }
              if (this->ComputeOBB)
              {
                this->FragmentOBBs->SetTuple(localId, pBuf);
                pBuf += 15;
              }
            }
#ifdef vtkMaterialInterfaceFilterDEBUG
            if (nTransactions >= 4)
            {
              cerr << "[" << __LINE__ << "] " << myProcId
                   << " memory commitment during localization of " << nTransactions
                   << " pieces is:" << endl
                   << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
            }
#endif
            localizedPoints->Delete();
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
    delete[] attributeCommBuffer;
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

#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId << " computation of geometric attributes completed."
       << endl;
#endif
}

//----------------------------------------------------------------------------
// Build the loading array for the fragment pieces that we own.
void vtkMaterialInterfaceFilter::BuildLoadingArray(vector<vtkIdType>& loadingArray)
{
  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));

  int nLocal = static_cast<int>(this->ResolvedFragmentIds[this->MaterialId].size());
  loadingArray.clear();
  loadingArray.resize(this->NumberOfResolvedFragments, 0);
  for (int i = 0; i < nLocal; ++i)
  {
    int globalId = this->ResolvedFragmentIds[this->MaterialId][i];

    vtkPolyData* fragment = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));

    loadingArray[globalId] = fragment->GetNumberOfCells();
  }
}

//----------------------------------------------------------------------------
// Load a buffer containing the number of polys for each fragment
// or fragment piece that we own. Return the size in vtkIdType's
// of the packed buffer and the buffer itself. Pass in a
// pointer initialized to null, allocation is internal.
int vtkMaterialInterfaceFilter::PackLoadingArray(vtkIdType*& buffer)
{
  assert("Buffer appears to have been pre-allocated." && buffer == nullptr);

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));

  int nLocal = static_cast<int>(this->ResolvedFragmentIds[this->MaterialId].size());

  vtkMaterialInterfacePieceLoading pl;
  const int bufSize = pl.SIZE * nLocal;
  buffer = new vtkIdType[bufSize];
  vtkIdType* pBuf = buffer;
  for (int i = 0; i < nLocal; ++i)
  {
    int globalId = this->ResolvedFragmentIds[this->MaterialId][i];

    vtkPolyData* fragment = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));

    pl.Initialize(globalId, fragment->GetNumberOfCells());
    pl.Pack(pBuf);
    pBuf += pl.SIZE;
  }

  return bufSize;
}

//----------------------------------------------------------------------------
// Given a fragment loading array that has been packed into an int array
// unpack. The packed loading arrays are ordered locally while the unpacked
// are ordered gloabbly by fragment id. NOTE if memory is an issue you
// could leave these locally indexed and search, rather than look up.
//
// Return the number of fragments and the unpacked array.
int vtkMaterialInterfaceFilter::UnPackLoadingArray(
  vtkIdType* buffer, int bufSize, vector<vtkIdType>& loadingArray)
{
  const int sizeOfPl = vtkMaterialInterfacePieceLoading::SIZE;

  assert("Buffer is null pointer." && buffer != 0);
  assert("Buffer size is incorrect." && bufSize % sizeOfPl == 0);

  loadingArray.clear();
  loadingArray.resize(this->NumberOfResolvedFragments, 0);
  vtkIdType* pBuf = buffer;
  const int nPieces = bufSize / sizeOfPl;
  for (int i = 0; i < nPieces; ++i)
  {
    vtkMaterialInterfacePieceLoading pil;
    pil.UnPack(pBuf);
    loadingArray[pil.GetId()] = pil.GetLoading();
    pBuf += sizeOfPl;
  }

  return nPieces;
}

//----------------------------------------------------------------------------
// Receive all geomteric attribute arrays from all other
// processes. Containers filled with the expected number
// of empty data arrays/pointers are expected.
//
// return 0 on error
int vtkMaterialInterfaceFilter::CollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& coaabb,
  vector<vtkDoubleArray*>& obb, vector<int*>& ids)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  const int msgBase = 200000;

  // anything to do?
  if (this->ComputeMoments && !this->ComputeOBB)
  {
    return 1;
  }

  // size buffer's header's in preparation for
  // receive. There's a buffer for each proc and
  // we're working with a single block(material) at a time.
  vtkMaterialInterfaceCommBuffer::SizeHeader(buffers, 1);

  // gather
  for (int procId = 0; procId < nProcs; ++procId)
  {
    // skip mine
    if (procId == myProcId)
    {
      continue;
    }
    int thisMsgId = msgBase;
    // receive buffer header
    this->Controller->Receive(
      buffers[procId].GetHeader(), buffers[procId].GetHeaderSize(), procId, thisMsgId);
    ++thisMsgId;
    // size buffer to hold incoming attributes
    buffers[procId].SizeBuffer();
    // receive attribute's data
    this->Controller->Receive(
      buffers[procId].GetBuffer(), buffers[procId].GetBufferSize(), procId, thisMsgId);
    ++thisMsgId;
    // unpack
    // here we are setting pointer into the buffer rather than
    // explicitly copying the data.
    const unsigned int nToUnpack = buffers[procId].GetNumberOfTuples(0);
    // coaabb
    if (!this->ComputeMoments)
    {
      buffers[procId].UnPack(coaabb[procId], 3, nToUnpack, false);
    }
    // obb
    if (this->ComputeOBB)
    {
      const unsigned int nCompsObb = this->FragmentOBBs->GetNumberOfComponents();
      buffers[procId].UnPack(obb[procId], nCompsObb, nToUnpack, false);
    }
    // ids
    buffers[procId].UnPack(ids[procId], 1, nToUnpack, false);
  }
  return 1;
}

//----------------------------------------------------------------------------
// Send all geometric attributes for the fragments which I own
// to another process.
//
// return 0 on error
int vtkMaterialInterfaceFilter::SendGeometricAttributes(const int recipientProcId)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int msgBase = 200000;

  // anything to do?
  if (this->ComputeMoments && !this->ComputeOBB)
  {
    return 1;
  }

  // initialize buffer
  // were working with a single block(material) at a time
  // sized in bytes, and we have to make note of how many
  // fragments we are putting in.
  const unsigned int nLocal =
    static_cast<unsigned int>(this->ResolvedFragmentIds[this->MaterialId].size());
  int totalNumberOfComps = (!this->ComputeMoments ? 3 : 0) // coaabb + obb
    + (this->ComputeOBB ? this->FragmentOBBs->GetNumberOfComponents() : 0);
  const vtkIdType bufferSize // coaabb(double) + obb(double) + ids(int)
    = nLocal * (sizeof(int) + totalNumberOfComps * sizeof(double));
  //
  vtkMaterialInterfaceCommBuffer buffer;
  buffer.Initialize(myProcId, 1, bufferSize);
  buffer.SetNumberOfTuples(0, nLocal);

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
  buffer.Pack(&this->ResolvedFragmentIds[this->MaterialId][0], 1, nLocal);

  // send buffer in two parts. header then data
  int thisMsgId = msgBase;
  // buffer's header
  this->Controller->Send(buffer.GetHeader(), buffer.GetHeaderSize(), recipientProcId, thisMsgId);
  ++thisMsgId;
  // attribute's data
  this->Controller->Send(buffer.GetBuffer(), buffer.GetBufferSize(), recipientProcId, thisMsgId);
  ++thisMsgId;

  return 1;
}
//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
//
// return 0 on error
int vtkMaterialInterfaceFilter::PrepareToCollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& coaabb,
  vector<vtkDoubleArray*>& obb, vector<int*>& ids)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // buffers
  buffers.resize(nProcs);
  // coaabb
  if (!this->ComputeMoments)
  {
    ResizeVectorOfVtkPointers(coaabb, nProcs);
    coaabb[myProcId]->Delete();
    coaabb[myProcId] = this->FragmentAABBCenters;
  }
  // obb
  if (this->ComputeOBB)
  {
    ResizeVectorOfVtkPointers(obb, nProcs);
    obb[myProcId]->Delete();
    obb[myProcId] = this->FragmentOBBs;
  }
  // ids
  ids.resize(nProcs, static_cast<int*>(nullptr));
  if (this->ResolvedFragmentIds[this->MaterialId].size() != 0)
  {
    ids[myProcId] = &(this->ResolvedFragmentIds[this->MaterialId][0]);
  }
  else
  {
    ids[myProcId] = nullptr;
  }

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
int vtkMaterialInterfaceFilter::CleanUpAfterCollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& coaabb,
  vector<vtkDoubleArray*>& obb, vector<int*>& ids)
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
int vtkMaterialInterfaceFilter::PrepareToMergeGeometricAttributes()
{
  // coaabb
  if (!this->ComputeMoments)
  {
    NewVtkArrayPointer(this->FragmentAABBCenters, 3, this->NumberOfResolvedFragments,
      this->FragmentAABBCenters->GetName());
  }
  // obb
  if (this->ComputeOBB)
  {
    NewVtkArrayPointer(this->FragmentOBBs, this->FragmentOBBs->GetNumberOfComponents(),
      this->NumberOfResolvedFragments, this->FragmentOBBs->GetName());
  }

  return 1;
}
//----------------------------------------------------------------------------
// Gather all geometric attributes to a single process.
//
// return 0 on error
int vtkMaterialInterfaceFilter::GatherGeometricAttributes(const int recipientProcId)
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::gatherGeometricAttributes("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  if (myProcId == recipientProcId)
  {
    // collect
    vector<vtkMaterialInterfaceCommBuffer> buffers;
    vector<vtkDoubleArray*> coaabb;
    vector<vtkDoubleArray*> obb;
    vector<int*> ids;
    this->PrepareToCollectGeometricAttributes(buffers, coaabb, obb, ids);
    this->CollectGeometricAttributes(buffers, coaabb, obb, ids);
    // merge
    this->PrepareToMergeGeometricAttributes();
    for (int procId = 0; procId < nProcs; ++procId)
    {
      vtkIdType nFromRemoteProc;
      double* pRemote;
      double* pLocal;
      int nComps;
      // coaaabb
      if (!this->ComputeMoments)
      {
        nFromRemoteProc = coaabb[procId]->GetNumberOfTuples();
        pRemote = coaabb[procId]->GetPointer(0);
        pLocal = this->FragmentAABBCenters->GetPointer(0);
        nComps = 3;
        for (vtkIdType i = 0; i < nFromRemoteProc; ++i)
        {
          int resIdx = nComps * ids[procId][i];
          for (int q = 0; q < nComps; ++q)
          {
            pLocal[resIdx + q] = pRemote[q];
          }
          pRemote += nComps;
        }
      }
      // obb
      if (this->ComputeOBB)
      {
        nFromRemoteProc = obb[procId]->GetNumberOfTuples();
        pRemote = obb[procId]->GetPointer(0);
        pLocal = this->FragmentOBBs->GetPointer(0);
        nComps = this->FragmentOBBs->GetNumberOfComponents();
        for (vtkIdType i = 0; i < nFromRemoteProc; ++i)
        {
          int resIdx = nComps * ids[procId][i];
          for (int q = 0; q < nComps; ++q)
          {
            pLocal[resIdx + q] = pRemote[q];
          }
          pRemote += nComps;
        }
      }
    }
    this->CleanUpAfterCollectGeometricAttributes(buffers, coaabb, obb, ids);
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
// are not copied from the buffer rather they point to
// the appropriate location in the buffer. The buffers should
// be deleted by caller after arrays are not needed.
//
// return 0 on error.
int vtkMaterialInterfaceFilter::CollectIntegratedAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& volumes,
  vector<vtkDoubleArray*>& clipDepthMaxs, vector<vtkDoubleArray*>& clipDepthMins,
  vector<vtkDoubleArray*>& moments, vector<vector<vtkDoubleArray*> >& volumeWtdAvgs,
  vector<vector<vtkDoubleArray*> >& massWtdAvgs, vector<vector<vtkDoubleArray*> >& sums)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  const int msgBase = 200000;

  // size buffer's header's
  // here we work with a single block(material)
  vtkMaterialInterfaceCommBuffer::SizeHeader(buffers, 1);

  // receive & unpack
  for (int procId = 0; procId < nProcs; ++procId)
  {
    // skip mine
    if (procId == myProcId)
    {
      continue;
    }
    int thisMsgId = msgBase;
    // receive buffer's header
    this->Controller->Receive(
      buffers[procId].GetHeader(), buffers[procId].GetHeaderSize(), procId, thisMsgId);
    ++thisMsgId;
    // size attribute's buffer from incoming header
    buffers[procId].SizeBuffer();
    // receive attribute's data
    this->Controller->Receive(
      buffers[procId].GetBuffer(), buffers[procId].GetBufferSize(), procId, thisMsgId);
    ++thisMsgId;
    // unpack attribute data
    // We will point into the comm buffer rather than copy
    const unsigned int nToUnpack = buffers[procId].GetNumberOfTuples(0);
    // volume
    buffers[procId].UnPack(volumes[procId], 1, nToUnpack, false);
    if (this->ClipWithPlane)
    {
      buffers[procId].UnPack(clipDepthMaxs[procId], 1, nToUnpack, false);
      buffers[procId].UnPack(clipDepthMins[procId], 1, nToUnpack, false);
    }
    // moments
    if (this->ComputeMoments)
    {
      buffers[procId].UnPack(moments[procId], 4, nToUnpack, false);
    }
    // volume weighted averages
    for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
    {
      const unsigned int nCompsAvg = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
      buffers[procId].UnPack(volumeWtdAvgs[procId][i], nCompsAvg, nToUnpack, false);
    }
    // mass weighted averages
    for (int i = 0; i < this->NMassWtdAvgs; ++i)
    {
      const unsigned int nCompsAvg = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
      buffers[procId].UnPack(massWtdAvgs[procId][i], nCompsAvg, nToUnpack, false);
    }
    // sums
    for (int i = 0; i < this->NToSum; ++i)
    {
      const unsigned int nCompsSum = this->FragmentSums[i]->GetNumberOfComponents();
      buffers[procId].UnPack(sums[procId][i], nCompsSum, nToUnpack, false);
    }
  }

  return 1;
}
//----------------------------------------------------------------------------
// Replaces our own interated attributes with those received
// from another process.
//
// return 0 on error.
int vtkMaterialInterfaceFilter::ReceiveIntegratedAttributes(const int sourceProcId)
{
  const int msgBase = 200000;

  // prepare the comm buffer to receive attribute data
  // pertaining to a single block(material)
  vtkMaterialInterfaceCommBuffer buffer;
  buffer.SizeHeader(1);

  int thisMsgId = msgBase;
  // receive buffer's header
  this->Controller->Receive(buffer.GetHeader(), buffer.GetHeaderSize(), sourceProcId, thisMsgId);
  ++thisMsgId;
  // size buffer via on incoming header
  buffer.SizeBuffer();
  // receive attribute's data
  this->Controller->Receive(buffer.GetBuffer(), buffer.GetBufferSize(), sourceProcId, thisMsgId);
  ++thisMsgId;

  // unpack attribute data, with an explicit copy
  // into a local array
  const unsigned int nToUnPack = buffer.GetNumberOfTuples(0);
  // volume
  ReNewVtkArrayPointer(this->FragmentVolumes, this->FragmentVolumes->GetName());
  buffer.UnPack(this->FragmentVolumes, 1, nToUnPack, true);
  // Clip Depth
  if (this->ClipWithPlane)
  {
    ReNewVtkArrayPointer(this->ClipDepthMaximums, this->ClipDepthMaximums->GetName());
    buffer.UnPack(this->ClipDepthMaximums, 1, nToUnPack, true);
    ReNewVtkArrayPointer(this->ClipDepthMinimums, this->ClipDepthMinimums->GetName());
    buffer.UnPack(this->ClipDepthMinimums, 1, nToUnPack, true);
  }
  // moments
  if (this->ComputeMoments)
  {
    ReNewVtkArrayPointer(this->FragmentMoments, this->FragmentMoments->GetName());
    buffer.UnPack(this->FragmentMoments, 4, nToUnPack, true);
  }
  // volume weighted averages
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    int nCompsVWAvg = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(this->FragmentVolumeWtdAvgs[i], this->FragmentVolumeWtdAvgs[i]->GetName());
    buffer.UnPack(this->FragmentVolumeWtdAvgs[i], nCompsVWAvg, nToUnPack, true);
  }
  // mass weighted averages
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    int nCompsMWAvg = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(this->FragmentMassWtdAvgs[i], this->FragmentMassWtdAvgs[i]->GetName());
    buffer.UnPack(this->FragmentMassWtdAvgs[i], nCompsMWAvg, nToUnPack, true);
  }
  // sums
  for (int i = 0; i < this->NToSum; ++i)
  {
    int nCompsSum = this->FragmentSums[i]->GetNumberOfComponents();
    ReNewVtkArrayPointer(this->FragmentSums[i], this->FragmentSums[i]->GetName());
    buffer.UnPack(this->FragmentSums[i], nCompsSum, nToUnPack, true);
  }

  return 1;
}

//----------------------------------------------------------------------------
// Send my integrated attributes to another process.
//
// return 0 on error.
int vtkMaterialInterfaceFilter::SendIntegratedAttributes(const int recipientProcId)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int msgBase = 200000;

  // estimate buffer size (in bytes)
  const vtkIdType nToSend = this->FragmentVolumes->GetNumberOfTuples();
  unsigned int totalNumberOfComps = 1 + (this->ComputeMoments ? 4 : 0); // volume + moments
  if (this->ClipWithPlane)
  {
    totalNumberOfComps += this->ClipDepthMaximums->GetNumberOfComponents();
    totalNumberOfComps += this->ClipDepthMinimums->GetNumberOfComponents();
  }
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i) // + volume weighted averages
  {
    totalNumberOfComps += this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
  }
  for (int i = 0; i < this->NMassWtdAvgs; ++i) // + mass weighted averages
  {
    totalNumberOfComps += this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
  }
  for (int i = 0; i < this->NToSum; ++i) // + sums
  {
    totalNumberOfComps += this->FragmentSums[i]->GetNumberOfComponents();
  }
  const vtkIdType bufferSize = nToSend * totalNumberOfComps * sizeof(double);

  // prepare a comm buffer
  // Will use only a single block(material) of attribute data
  // We size the buffer, and set the number of fragments.
  vtkMaterialInterfaceCommBuffer buffer;
  buffer.Initialize(myProcId, 1, bufferSize);
  buffer.SetNumberOfTuples(0, nToSend);

  // pack attributes into the buffer
  // volume
  buffer.Pack(this->FragmentVolumes);
  if (this->ClipWithPlane)
  {
    buffer.Pack(this->ClipDepthMaximums);
    buffer.Pack(this->ClipDepthMinimums);
  }
  // moments
  if (this->ComputeMoments)
  {
    buffer.Pack(this->FragmentMoments);
  }
  // volume weighted averages
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    buffer.Pack(this->FragmentVolumeWtdAvgs[i]);
  }
  // mass weighted averages
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    buffer.Pack(this->FragmentMassWtdAvgs[i]);
  }
  // sums
  for (int i = 0; i < this->NToSum; ++i)
  {
    buffer.Pack(this->FragmentSums[i]);
  }

  // send the buffer in two parts, first the header, followed by
  // the attribute data
  int thisMsgId = msgBase;
  // buffer's header
  this->Controller->Send(buffer.GetHeader(), buffer.GetHeaderSize(), recipientProcId, thisMsgId);
  ++thisMsgId;
  // packed attributes
  this->Controller->Send(buffer.GetBuffer(), buffer.GetBufferSize(), recipientProcId, thisMsgId);
  ++thisMsgId;

  return 1;
}

//----------------------------------------------------------------------------
// Send my integrated attribute arrays to all other processes.
//
// return 0 on error.
int vtkMaterialInterfaceFilter::BroadcastIntegratedAttributes(const int sourceProcId)
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::BroadcastIntegratedAttributes("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // anything to do?
  if (nProcs == 1)
  {
    return 1;
  }

  // send mine to all
  if (myProcId == sourceProcId)
  {
    for (int procId = 0; procId < nProcs; ++procId)
    {
      // skip me
      if (procId == sourceProcId)
      {
        continue;
      }
      this->SendIntegratedAttributes(procId);
    }
  }
  // receive from source
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
int vtkMaterialInterfaceFilter::PrepareToCollectIntegratedAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& volumes,
  vector<vtkDoubleArray*>& clipDepthMaxs, vector<vtkDoubleArray*>& clipDepthMins,
  vector<vtkDoubleArray*>& moments, vector<vector<vtkDoubleArray*> >& volumeWtdAvgs,
  vector<vector<vtkDoubleArray*> >& massWtdAvgs, vector<vector<vtkDoubleArray*> >& sums)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // set up for collect
  // buffers
  buffers.resize(nProcs);
  // volumes
  ResizeVectorOfVtkPointers(volumes, nProcs);
  volumes[myProcId]->Delete();
  volumes[myProcId] = this->FragmentVolumes;
  // ClipDepths
  if (this->ClipWithPlane)
  {
    ResizeVectorOfVtkPointers(clipDepthMaxs, nProcs);
    clipDepthMaxs[myProcId]->Delete();
    clipDepthMaxs[myProcId] = this->ClipDepthMaximums;
    ResizeVectorOfVtkPointers(clipDepthMins, nProcs);
    clipDepthMins[myProcId]->Delete();
    clipDepthMins[myProcId] = this->ClipDepthMinimums;
  }
  // moments
  if (this->ComputeMoments)
  {
    ResizeVectorOfVtkPointers(moments, nProcs);
    moments[myProcId]->Delete();
    moments[myProcId] = this->FragmentMoments;
  }
  // volume weighted averages
  if (this->NVolumeWtdAvgs > 0)
  {
    volumeWtdAvgs.resize(nProcs);
    for (int procId = 0; procId < nProcs; ++procId)
    {
      if (procId == myProcId)
      {
        volumeWtdAvgs[procId] = this->FragmentVolumeWtdAvgs;
      }
      else
      {
        ResizeVectorOfVtkPointers(volumeWtdAvgs[procId], this->NVolumeWtdAvgs);
      }
    }
  }
  // mass weighted averages
  if (this->NMassWtdAvgs > 0)
  {
    massWtdAvgs.resize(nProcs);
    for (int procId = 0; procId < nProcs; ++procId)
    {
      if (procId == myProcId)
      {
        massWtdAvgs[procId] = this->FragmentMassWtdAvgs;
      }
      else
      {
        ResizeVectorOfVtkPointers(massWtdAvgs[procId], this->NMassWtdAvgs);
      }
    }
  }
  // sums
  if (this->NToSum > 0)
  {
    sums.resize(nProcs);
    for (int procId = 0; procId < nProcs; ++procId)
    {
      if (procId == myProcId)
      {
        sums[procId] = this->FragmentSums;
      }
      else
      {
        ResizeVectorOfVtkPointers(sums[procId], this->NToSum);
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
int vtkMaterialInterfaceFilter::CleanUpAfterCollectIntegratedAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vtkDoubleArray*>& volumes,
  vector<vtkDoubleArray*>& clipDepthMaxs, vector<vtkDoubleArray*>& clipDepthMins,
  vector<vtkDoubleArray*>& moments, vector<vector<vtkDoubleArray*> >& volumeWtdAvgs,
  vector<vector<vtkDoubleArray*> >& massWtdAvgs, vector<vector<vtkDoubleArray*> >& sums)
{
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // volumes
  ClearVectorOfVtkPointers(volumes);
  // clip depths
  if (this->ClipWithPlane)
  {
    ClearVectorOfVtkPointers(clipDepthMaxs);
    ClearVectorOfVtkPointers(clipDepthMins);
  }
  // moments
  if (this->ComputeMoments)
  {
    ClearVectorOfVtkPointers(moments);
  }
  // volume weighted averages
  if (this->NVolumeWtdAvgs > 0)
  {
    for (int procId = 0; procId < nProcs; ++procId)
    {
      ClearVectorOfVtkPointers(volumeWtdAvgs[procId]);
    }
  }
  // mass weighted averages
  if (this->NMassWtdAvgs > 0)
  {
    for (int procId = 0; procId < nProcs; ++procId)
    {
      ClearVectorOfVtkPointers(massWtdAvgs[procId]);
    }
  }
  // sums
  if (this->NToSum > 0)
  {
    for (int procId = 0; procId < nProcs; ++procId)
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
int vtkMaterialInterfaceFilter::PrepareToResolveIntegratedAttributes()
{
  int nComps;
  double* pResolved = nullptr;
  vtkIdType bytesPerComponent = sizeof(double) * this->NumberOfResolvedFragments;

  // volume
  nComps = 1;
  NewVtkArrayPointer(this->FragmentVolumes, nComps, this->NumberOfResolvedFragments,
    this->FragmentVolumes->GetName());
  pResolved = this->FragmentVolumes->GetPointer(0);
  memset(pResolved, 0, bytesPerComponent);

  // clip depths
  if (this->ClipWithPlane)
  {
    nComps = 1;
    NewVtkArrayPointer(this->ClipDepthMaximums, nComps, this->NumberOfResolvedFragments,
      this->ClipDepthMaximums->GetName());
    pResolved = this->ClipDepthMaximums->GetPointer(0);
    memset(pResolved, 0, bytesPerComponent);
    NewVtkArrayPointer(this->ClipDepthMinimums, nComps, this->NumberOfResolvedFragments,
      this->ClipDepthMinimums->GetName());
    pResolved = this->ClipDepthMinimums->GetPointer(0);
    memset(pResolved, 0, bytesPerComponent);
  }

  // moments
  if (this->ComputeMoments)
  {
    nComps = 4;
    NewVtkArrayPointer(this->FragmentMoments, nComps, this->NumberOfResolvedFragments,
      this->FragmentMoments->GetName());
    pResolved = this->FragmentMoments->GetPointer(0);
    memset(pResolved, 0, nComps * bytesPerComponent);
  }
  // volume weighted averages
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    nComps = this->FragmentVolumeWtdAvgs[i]->GetNumberOfComponents();
    NewVtkArrayPointer(this->FragmentVolumeWtdAvgs[i], nComps, this->NumberOfResolvedFragments,
      this->FragmentVolumeWtdAvgs[i]->GetName());
    pResolved = this->FragmentVolumeWtdAvgs[i]->GetPointer(0);
    memset(pResolved, 0, nComps * bytesPerComponent);
  }
  // mass weighted averages
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    nComps = this->FragmentMassWtdAvgs[i]->GetNumberOfComponents();
    NewVtkArrayPointer(this->FragmentMassWtdAvgs[i], nComps, this->NumberOfResolvedFragments,
      this->FragmentMassWtdAvgs[i]->GetName());
    pResolved = this->FragmentMassWtdAvgs[i]->GetPointer(0);
    memset(pResolved, 0, nComps * bytesPerComponent);
  }
  // sums
  for (int i = 0; i < this->NToSum; ++i)
  {
    nComps = this->FragmentSums[i]->GetNumberOfComponents();
    NewVtkArrayPointer(this->FragmentSums[i], nComps, this->NumberOfResolvedFragments,
      this->FragmentSums[i]->GetName());
    pResolved = this->FragmentSums[i]->GetPointer(0);
    memset(pResolved, 0, nComps * bytesPerComponent);
  }

  return 1;
}

//----------------------------------------------------------------------------
// This is a gather operation that includes requisite calculations
// to finish integrations. The integrated arrays on the controlling
// process are resolved, but other procs are not updated.
//
// return 0 on error.
int vtkMaterialInterfaceFilter::ResolveIntegratedAttributes(const int controllingProcId)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // controller gathers and resolves
  if (myProcId == controllingProcId)
  {
    vector<vtkMaterialInterfaceCommBuffer> buffers;
    vector<vtkDoubleArray*> volumes;
    vector<vtkDoubleArray*> clipDepthMaxs;
    vector<vtkDoubleArray*> clipDepthMins;
    vector<vtkDoubleArray*> moments;
    vector<vector<vtkDoubleArray*> > volumeWtdAvgs;
    vector<vector<vtkDoubleArray*> > massWtdAvgs;
    vector<vector<vtkDoubleArray*> > sums;
    // prepare collect buffers
    this->PrepareToCollectIntegratedAttributes(
      buffers, volumes, clipDepthMaxs, clipDepthMins, moments, volumeWtdAvgs, massWtdAvgs, sums);
    // collect
    this->CollectIntegratedAttributes(
      buffers, volumes, clipDepthMaxs, clipDepthMins, moments, volumeWtdAvgs, massWtdAvgs, sums);

    // prepare for resolution
    this->PrepareToResolveIntegratedAttributes();
    // resolve attributes
    // First pass we'll resolve attributes which
    // are needed to resolve other attributes (eg weights)
    int procBaseEqSetId = 0;
    for (int procId = 0; procId < nProcs; ++procId)
    {
      int eqSetId;
      double* pResolved;
      const double* pUnresolved;
      int nUnresolved = this->NumberOfRawFragmentsInProcess[procId];

      // volume
      pUnresolved = volumes[procId]->GetPointer(0);
      pResolved = this->FragmentVolumes->GetPointer(0);
      eqSetId = procBaseEqSetId;
      for (int i = 0; i < nUnresolved; ++i)
      {
        int resIdx = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
        pResolved[resIdx] += pUnresolved[0];
        ++pUnresolved;
        ++eqSetId;
      }
      // clip depth
      if (this->ClipWithPlane)
      {
        pUnresolved = clipDepthMaxs[procId]->GetPointer(0);
        pResolved = this->ClipDepthMaximums->GetPointer(0);
        eqSetId = procBaseEqSetId;
        for (int i = 0; i < nUnresolved; ++i)
        {
          int resIdx = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          pResolved[resIdx] += pUnresolved[0];
          ++pUnresolved;
          ++eqSetId;
        }
        pUnresolved = clipDepthMins[procId]->GetPointer(0);
        pResolved = this->ClipDepthMinimums->GetPointer(0);
        eqSetId = procBaseEqSetId;
        for (int i = 0; i < nUnresolved; ++i)
        {
          int resIdx = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          pResolved[resIdx] += pUnresolved[0];
          ++pUnresolved;
          ++eqSetId;
        }
      }
      // moments
      if (this->ComputeMoments)
      {
        pUnresolved = moments[procId]->GetPointer(0);
        pResolved = this->FragmentMoments->GetPointer(0);
        eqSetId = procBaseEqSetId;
        for (int i = 0; i < nUnresolved; ++i)
        {
          int resIdx = 4 * this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          for (int q = 0; q < 4; ++q)
          {
            pResolved[resIdx + q] += pUnresolved[q];
          }
          pUnresolved += 4;
          ++eqSetId;
        }
      }
      procBaseEqSetId += nUnresolved;
    }

    // Second pass, resolve attributes which depend on
    // other attributes (eg weighted averages)
    procBaseEqSetId = 0;
    for (int procId = 0; procId < nProcs; ++procId)
    {
      int eqSetId;
      double* pResolved;
      const double* pUnresolved;
      int nUnresolved = this->NumberOfRawFragmentsInProcess[procId];
      // volume weighted averages
      double* pWt;  // these have to do with weights
      int nCompsWt; //
      int wtComp;   //
      for (int k = 0; k < this->NVolumeWtdAvgs; ++k)
      {
        pUnresolved = volumeWtdAvgs[procId][k]->GetPointer(0);
        pResolved = this->FragmentVolumeWtdAvgs[k]->GetPointer(0);
        int nComps = this->FragmentVolumeWtdAvgs[k]->GetNumberOfComponents();
        pWt = this->FragmentVolumes->GetPointer(0);
        nCompsWt = 1;
        wtComp = 0;
        eqSetId = procBaseEqSetId;
        for (int i = 0; i < nUnresolved; ++i)
        {
          int resId = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          int resIdx = nComps * resId;
          int wtIdx = nCompsWt * resId;
          for (int q = 0; q < nComps; ++q)
          {
            pResolved[resIdx + q] += pUnresolved[q] / pWt[wtIdx];
          }
          pUnresolved += nComps;
          ++eqSetId;
        }
      }
      // mass weighted averages (Mx,My,Mz,Mass)
      if (this->ComputeMoments)
      {
        for (int k = 0; k < this->NMassWtdAvgs; ++k)
        {
          pUnresolved = massWtdAvgs[procId][k]->GetPointer(0);
          pResolved = this->FragmentMassWtdAvgs[k]->GetPointer(0);
          int nComps = this->FragmentMassWtdAvgs[k]->GetNumberOfComponents();
          nCompsWt = 4;
          wtComp = 3;
          pWt = this->FragmentMoments->GetPointer(0) + wtComp;
          eqSetId = procBaseEqSetId;
          for (int i = 0; i < nUnresolved; ++i)
          {
            int resId = this->EquivalenceSet->GetEquivalentSetId(eqSetId);
            int resIdx = nComps * resId;
            int wtIdx = nCompsWt * resId;
            for (int q = 0; q < nComps; ++q)
            {
              pResolved[resIdx + q] += pUnresolved[q] / pWt[wtIdx];
            }
            pUnresolved += nComps;
            ++eqSetId;
          }
        }
      }
      // sums
      for (int k = 0; k < this->NToSum; ++k)
      {
        pUnresolved = sums[procId][k]->GetPointer(0);
        pResolved = this->FragmentSums[k]->GetPointer(0);
        int nComps = this->FragmentSums[k]->GetNumberOfComponents();
        eqSetId = procBaseEqSetId;
        for (int i = 0; i < nUnresolved; ++i)
        {
          int resIdx = nComps * this->EquivalenceSet->GetEquivalentSetId(eqSetId);
          for (int q = 0; q < nComps; ++q)
          {
            pResolved[resIdx + q] += pUnresolved[q];
          }
          pUnresolved += nComps;
          ++eqSetId;
        }
      }
      procBaseEqSetId += nUnresolved;
    }
    // clean up
    this->CleanUpAfterCollectIntegratedAttributes(
      buffers, volumes, clipDepthMaxs, clipDepthMins, moments, volumeWtdAvgs, massWtdAvgs, sums);
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
void vtkMaterialInterfaceFilter::PrepareForResolveEquivalences()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::PrepareForResolveEquivalences("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  // equivalences
  this->EquivalenceSet->Squeeze();
  // volume
  this->FragmentVolumes->Squeeze();
  // Clip depth
  if (this->ClipWithPlane && this->ClipDepthMaximums)
  {
    this->ClipDepthMaximums->Squeeze();
    this->ClipDepthMinimums->Squeeze();
  }
  // moments
  if (this->ComputeMoments)
  {
    this->FragmentMoments->Squeeze();
  }
  // volume weighted average
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    this->FragmentVolumeWtdAvgs[i]->Squeeze();
  }
  // mass weighted avg
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    this->FragmentMassWtdAvgs[i]->Squeeze();
  }
  // sum
  for (int i = 0; i < this->NToSum; ++i)
  {
    this->FragmentSums[i]->Squeeze();
  }
  // geometry container
  vector<vtkPolyData*>(this->FragmentMeshes).swap(this->FragmentMeshes);
}
//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::ResolveEquivalences()
{
  int numProcs = this->Controller->GetNumberOfProcesses();
#ifdef vtkMaterialInterfaceFilterDEBUG
  int myProcId = this->Controller->GetLocalProcessId();
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment entering ResolveEquivalences is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // These get resused for efer attribute we need to merge so I made the ivars.
  this->NumberOfRawFragmentsInProcess = new int[numProcs];
  this->LocalToGlobalOffsets = new int[numProcs];

  // Resolve intraprocess and extra process equivalences.
  // This also renumbers set ids to be sequential.
  this->GatherEquivalenceSets(this->EquivalenceSet);
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after GatherEquivalenceSets is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // Gather and merge fragments for whose geometry is split
  // and build the output dataset as we go.
  this->ResolveLocalFragmentGeometry();
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after ResolveLocalFragmentGeometry is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // Clean duplicate points
  this->CleanLocalFragmentGeometry();
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after CleanLocalFragmentGeometry is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // Accumulate contributions from fragemnts who were
  // previously split.
  this->ResolveIntegratedAttributes(0);
  this->BroadcastIntegratedAttributes(0);
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after ResolveIntegratedAttributes is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // Compute geometric attributes, and gather them for
  // stats output.
  this->ComputeGeometricAttributes();
  this->GatherGeometricAttributes(0);
#ifdef vtkMaterialInterfaceFilterDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after ComputeGeometricAttributes is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  // Copy attributes into the output data sets.
  this->CopyAttributesToOutput0();
  this->CopyAttributesToOutput1();
#ifdef vtkMaterialInterfaceFilterDEBUG
  this->CopyAttributesToOutput2();
  cerr << "[" << __LINE__ << "] " << myProcId
       << " memory commitment after CopyAttributesToOutput0/1 is:" << endl
       << GetMemoryUsage(this->MyPid, __LINE__, myProcId);
#endif

  delete[] this->NumberOfRawFragmentsInProcess;
  this->NumberOfRawFragmentsInProcess = nullptr;
  delete[] this->LocalToGlobalOffsets;
  this->LocalToGlobalOffsets = nullptr;
}

//----------------------------------------------------------------------------
// This also fills in the arrays NumberOfRawFragments and LocalToGlobalOffsets
// as a side effect. (also NumberOfResolvedFragments).
void vtkMaterialInterfaceFilter::GatherEquivalenceSets(vtkMaterialInterfaceEquivalenceSet* set)
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::GatherEquivalenceSets("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
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
      this->Controller->Receive(this->NumberOfRawFragmentsInProcess + ii, 1, ii, 875034);
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
  vtkMaterialInterfaceEquivalenceSet* globalSet = new vtkMaterialInterfaceEquivalenceSet;
  // This just initializes the set so that every set has one member.
  //  Every id is equivalent to itself and no others.
  if (totalNumberOfIds > 0)
  { // This crashes if there no fragemnts extracted.
    globalSet->AddEquivalence(totalNumberOfIds - 1, totalNumberOfIds - 1);
  }
  // Add the equivalences from our process.
  int myOffset = this->LocalToGlobalOffsets[myProcId];
  int memberSetId;
  for (int ii = 0; ii < numLocalMembers; ++ii)
  {
    memberSetId = set->GetEquivalentSetId(ii);
    // This will be inefficient until I make the scheduled improvemnt to the
    // add method (Ordered multilevel tree instead of a one level tree).
    globalSet->AddEquivalence(ii + myOffset, memberSetId + myOffset);
  }

  // cerr << myProcId << " Input set: " << endl;
  // set->Print();
  // cerr << myProcId << " global set: " << endl;
  // globalSet->Print();

  // Now add equivalents between processes.
  // Send all the ghost blocks to the process that owns the block.
  // Compare ids and add the equivalences.
  this->ShareGhostEquivalences(globalSet, this->LocalToGlobalOffsets);

  // cerr << "Global after ghost: " << myProcId << endl;
  // globalSet->Print();

  // Merge all of the processes global sets.
  // Clean the global set so that the resulting set ids are sequential.
  this->MergeGhostEquivalenceSets(globalSet);

  // cerr << "Global after merge: " << myProcId << endl;
  // globalSet->Print();

  // free what do not need
  globalSet->Squeeze();

  // Copy the equivalences to the local set for returning our results.
  // The ids will be the global ids so the GetId method will work.
  set->DeepCopy(globalSet);

  delete globalSet;
}

//----------------------------------------------------------------------------
void vtkMaterialInterfaceFilter::MergeGhostEquivalenceSets(
  vtkMaterialInterfaceEquivalenceSet* globalSet)
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
  delete[] tmp;

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
void vtkMaterialInterfaceFilter::ShareGhostEquivalences(
  vtkMaterialInterfaceEquivalenceSet* globalSet, int* procOffsets)
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
      int num = static_cast<int>(this->GhostBlocks.size());
      for (int blockId = 0; blockId < num; ++blockId)
      {
        vtkMaterialInterfaceFilterBlock* block = this->GhostBlocks[blockId];
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
          this->Controller->Send(framentIds,
            (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1), otherProc,
            722266);
        } // End if ghost  block owned by other process.
      }   // End loop over all blocks.
      // Send the message that indicates we have nothing more to send.
      sendMsg[0] = myProcId;
      sendMsg[1] = -1;
      this->Controller->Send(sendMsg, 8, otherProc, 722265);
    } // End if we should send or receive.
  }   // End loop over all processes.
}

//----------------------------------------------------------------------------
// Receive all the gost blocks from remote processes and
// find the equivalences.
void vtkMaterialInterfaceFilter::ReceiveGhostFragmentIds(
  vtkMaterialInterfaceEquivalenceSet* globalSet, int* procOffsets)
{
  int msg[8];
  int otherProc;
  int blockId;
  vtkMaterialInterfaceFilterBlock* block;
  int bufSize = 0;
  int* buf = nullptr;
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
      if (block == nullptr)
      { // Sanity check. This will lock up!
        vtkErrorMacro("Missing block request.");
        return;
      }
      // Receive the ghost fragment ids.
      remoteExt = msg + 2;
      dataSize = (remoteExt[1] - remoteExt[0] + 1) * (remoteExt[3] - remoteExt[2] + 1) *
        (remoteExt[5] - remoteExt[4] + 1);
      if (bufSize < dataSize)
      {
        if (buf)
        {
          delete[] buf;
        }
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
      pz = localFragmentIds + (remoteExt[0] - localExt[0]) * localIncs[0] +
        (remoteExt[2] - localExt[2]) * localIncs[1] + (remoteExt[4] - localExt[4]) * localIncs[2];
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
              globalSet->AddEquivalence(localId + localOffset, remoteId + remoteOffset);
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
    delete[] buf;
  }
}

//----------------------------------------------------------------------------
// After the attributes have been resolved copy them in to the
// fragment data sets. We put each one as a 1 tuple array
// in the field data, and for now we copy each to the point data
// as well. My hope is that the point data can be eliminated.
void vtkMaterialInterfaceFilter::CopyAttributesToOutput0()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::CopyAttributesToOutput0("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));
  assert("Couldn't get the resolved fragnments." && resolvedFragments);

#ifdef vtkMaterialInterfaceFilterDEBUG
  vector<int>& fragmentSplitMarker = this->FragmentSplitMarker[this->MaterialId];
#endif

  int nLocalFragments = static_cast<int>(resolvedFragmentIds.size());
  for (int i = 0; i < nLocalFragments; ++i)
  {
    int nComps = 0;
    const char* name = nullptr;
    const double* srcTuple = nullptr;

    // get the fragment
    int globalId = resolvedFragmentIds[i];
    vtkPolyData* thisFragment = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));
    assert("Fragment is not local." && thisFragment);
    int nPoints = thisFragment->GetNumberOfPoints();

    // add a attributes to field data
    vtkFieldData* fd = thisFragment->GetFieldData();
    vtkPointData* pd = thisFragment->GetPointData();
    // Fragment id
    // field data
    int resolvedGlobalId = globalId + this->ResolvedFragmentCount;
    vtkIntArray* fdId = vtkIntArray::New();
    fdId->SetName("Id");
    fdId->SetNumberOfComponents(1);
    fdId->SetNumberOfTuples(1);
    fdId->SetValue(0, resolvedGlobalId);
    fd->AddArray(fdId);
    fdId->Delete();
    // point data
    vtkIntArray* pdId = vtkIntArray::New();
    pdId->SetName("Id");
    pdId->SetNumberOfComponents(1);
    pdId->SetNumberOfTuples(nPoints);
    pdId->FillComponent(0, resolvedGlobalId);
    pd->AddArray(pdId);
    pdId->Delete();

    // Material Id
    // field data
    vtkIntArray* fdMid = vtkIntArray::New();
    fdMid->SetName("Material");
    fdMid->SetNumberOfComponents(1);
    fdMid->SetNumberOfTuples(1);
    fdMid->SetValue(0, this->MaterialId);
    fd->AddArray(fdMid);
    fdMid->Delete();
    // point data
    vtkIntArray* pdMid = vtkIntArray::New();
    pdMid->SetName("Material");
    pdMid->SetNumberOfComponents(1);
    pdMid->SetNumberOfTuples(nPoints);
    pdMid->FillComponent(0, this->MaterialId);
    pd->AddArray(pdMid);
    pdMid->Delete();

    // volume
    double V = this->FragmentVolumes->GetValue(globalId);
    // field data
    vtkDoubleArray* fdVol = vtkDoubleArray::New();
    fdVol->SetName("Volume");
    fdVol->SetNumberOfComponents(1);
    fdVol->SetNumberOfTuples(1);
    fdVol->SetValue(0, V);
    fd->AddArray(fdVol);
    fdVol->Delete();
    // point data
    vtkDataArray* pdVol = vtkDoubleArray::New();
    pdVol->SetName("Volume");
    pdVol->SetNumberOfComponents(1);
    pdVol->SetNumberOfTuples(nPoints);
    pdVol->FillComponent(0, V);
    pd->AddArray(pdVol);
    pdVol->Delete();

    // clip depth
    if (this->ClipWithPlane)
    {
      double D;
      D = this->ClipDepthMaximums->GetValue(globalId);
      // field data
      vtkDoubleArray* fdDMax = vtkDoubleArray::New();
      fdDMax->SetName("ClipDepthMax");
      fdDMax->SetNumberOfComponents(1);
      fdDMax->SetNumberOfTuples(1);
      fdDMax->SetValue(0, D);
      fd->AddArray(fdDMax);
      fdDMax->Delete();
      // point data
      vtkDataArray* pdDMax = vtkDoubleArray::New();
      pdDMax->SetName("ClipDepthMax");
      pdDMax->SetNumberOfComponents(1);
      pdDMax->SetNumberOfTuples(nPoints);
      pdDMax->FillComponent(0, D);
      pd->AddArray(pdDMax);
      pdDMax->Delete();
      D = this->ClipDepthMinimums->GetValue(globalId);
      // field data
      vtkDoubleArray* fdDMin = vtkDoubleArray::New();
      fdDMin->SetName("ClipDepthMin");
      fdDMin->SetNumberOfComponents(1);
      fdDMin->SetNumberOfTuples(1);
      fdDMin->SetValue(0, D);
      fd->AddArray(fdDMin);
      fdDMin->Delete();
      // point data
      vtkDataArray* pdDMin = vtkDoubleArray::New();
      pdDMin->SetName("ClipDepthMin");
      pdDMin->SetNumberOfComponents(1);
      pdDMin->SetNumberOfTuples(nPoints);
      pdDMin->FillComponent(0, D);
      pd->AddArray(pdDMin);
      pdDMin->Delete();
    }

    if (this->ComputeMoments)
    {
      // Mass
      nComps = this->FragmentMoments->GetNumberOfComponents();
      name = this->FragmentMoments->GetName();
      srcTuple = this->FragmentMoments->GetTuple(globalId);
      // field data
      vtkDoubleArray* fdM = vtkDoubleArray::New();
      fdM->SetName("Mass");
      fdM->SetNumberOfComponents(1);
      fdM->SetNumberOfTuples(1);
      fdM->SetValue(0, srcTuple[3]);
      fd->AddArray(fdM);
      fdM->Delete();
      // point data
      vtkDataArray* pdM = vtkDoubleArray::New();
      pdM->SetName("Mass");
      pdM->SetNumberOfComponents(1);
      pdM->SetNumberOfTuples(nPoints);
      pdM->FillComponent(0, srcTuple[3]);
      pd->AddArray(pdM);
      pdM->Delete();

      // Center of mass
      double com[3];
      for (int q = 0; q < 3; ++q)
      { // TODO this happens twice....
        com[q] = srcTuple[q] / srcTuple[3];
      }
      // field data
      vtkDoubleArray* fdCom = vtkDoubleArray::New();
      fdCom->SetName("Center of Mass");
      fdCom->SetNumberOfComponents(3);
      fdCom->SetNumberOfTuples(1);
      fdCom->SetTuple(0, com);
      fd->AddArray(fdCom);
      fdCom->Delete();
    }
    else
    {
      // Center of AABB
      srcTuple = this->FragmentAABBCenters->GetTuple(i);
      // field data
      vtkDoubleArray* fdCaabb = vtkDoubleArray::New();
      fdCaabb->SetName("Center of AABB");
      fdCaabb->SetNumberOfComponents(3);
      fdCaabb->SetNumberOfTuples(1);
      fdCaabb->SetTuple(0, srcTuple);
      fd->AddArray(fdCaabb);
      fdCaabb->Delete();
    }

    // OBBs
    if (this->ComputeOBB)
    {
      // All of the information is provided in
      // the feild data, while only dimensions are
      // provided in the point data.
      srcTuple = this->FragmentOBBs->GetTuple(i);
      // Field data
      vtkDoubleArray* fdObb = vtkDoubleArray::New();
      fdObb->SetName("Bounding Box Origin");
      fdObb->SetNumberOfComponents(3);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0, srcTuple);
      fd->AddArray(fdObb);
      srcTuple += 3;
      ReNewVtkPointer(fdObb);
      fdObb->SetName("Bounding Box Axis 1");
      fdObb->SetNumberOfComponents(3);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0, srcTuple);
      fd->AddArray(fdObb);
      srcTuple += 3;
      ReNewVtkPointer(fdObb);
      fdObb->SetName("Bounding Box Axis 2");
      fdObb->SetNumberOfComponents(3);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0, srcTuple);
      fd->AddArray(fdObb);
      srcTuple += 3;
      ReNewVtkPointer(fdObb);
      fdObb->SetName("Bounding Box Axis 3");
      fdObb->SetNumberOfComponents(3);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0, srcTuple);
      fd->AddArray(fdObb);
      srcTuple += 3;
      ReNewVtkPointer(fdObb);
      fdObb->SetName("Bounding Box Length");
      fdObb->SetNumberOfComponents(3);
      fdObb->SetNumberOfTuples(1);
      fdObb->SetTuple(0, srcTuple);
      fd->AddArray(fdObb);
      fdObb->Delete();
      // point data
      vtkDoubleArray* pdObb = vtkDoubleArray::New();
      pdObb->SetName("Bounding Box Length");
      pdObb->SetNumberOfComponents(3);
      pdObb->SetNumberOfTuples(nPoints);
      for (int q = 0; q < 3; ++q)
      {
        pdObb->FillComponent(q, srcTuple[q]);
      }
      pd->AddArray(pdObb);
      pdObb->Delete();
    }
    // volume weighted averages
    for (int j = 0; j < this->NVolumeWtdAvgs; ++j)
    {
      nComps = this->FragmentVolumeWtdAvgs[j]->GetNumberOfComponents();
      name = this->FragmentVolumeWtdAvgs[j]->GetName();
      srcTuple = this->FragmentVolumeWtdAvgs[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray* fdVwa = vtkDoubleArray::New();
      fdVwa->SetName(name);
      fdVwa->SetNumberOfComponents(nComps);
      fdVwa->SetNumberOfTuples(1);
      fdVwa->SetTuple(0, srcTuple);
      fd->AddArray(fdVwa);
      fdVwa->Delete();
      // point data
      vtkDoubleArray* pdVwa = vtkDoubleArray::New();
      pdVwa->SetName(name);
      pdVwa->SetNumberOfComponents(nComps);
      pdVwa->SetNumberOfTuples(nPoints);
      for (int q = 0; q < nComps; ++q)
      {
        pdVwa->FillComponent(q, srcTuple[q]);
      }
      pd->AddArray(pdVwa);
      pdVwa->Delete();
    }
    // mass weighted averages
    for (int j = 0; j < this->NMassWtdAvgs; ++j)
    {
      nComps = this->FragmentMassWtdAvgs[j]->GetNumberOfComponents();
      name = this->FragmentMassWtdAvgs[j]->GetName();
      srcTuple = this->FragmentMassWtdAvgs[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray* fdMwa = vtkDoubleArray::New();
      fdMwa->SetName(name);
      fdMwa->SetNumberOfComponents(nComps);
      fdMwa->SetNumberOfTuples(1);
      fdMwa->SetTuple(0, srcTuple);
      fd->AddArray(fdMwa);
      fdMwa->Delete();
      // point data
      vtkDoubleArray* pdMwa = vtkDoubleArray::New();
      pdMwa->SetName(name);
      pdMwa->SetNumberOfComponents(nComps);
      pdMwa->SetNumberOfTuples(nPoints);
      for (int q = 0; q < nComps; ++q)
      {
        pdMwa->FillComponent(q, srcTuple[q]);
      }
      pd->AddArray(pdMwa);
      pdMwa->Delete();
    }
    // summations
    for (int j = 0; j < this->NToSum; ++j)
    {
      nComps = this->FragmentSums[j]->GetNumberOfComponents();
      name = this->FragmentSums[j]->GetName();
      srcTuple = this->FragmentSums[j]->GetTuple(globalId);

      // field data
      vtkDoubleArray* fdS = vtkDoubleArray::New();
      fdS->SetName(name);
      fdS->SetNumberOfComponents(nComps);
      fdS->SetNumberOfTuples(1);
      fdS->SetTuple(0, srcTuple);
      fd->AddArray(fdS);
      fdS->Delete();
      // point data
      vtkDoubleArray* pdS = vtkDoubleArray::New();
      pdS->SetName(name);
      pdS->SetNumberOfComponents(nComps);
      pdS->SetNumberOfTuples(nPoints);
      for (int q = 0; q < nComps; ++q)
      {
        pdS->FillComponent(q, srcTuple[q]);
      }
      pd->AddArray(pdS);
      pdS->Delete();
    }
// split geometry marker
#ifdef vtkMaterialInterfaceFilterDEBUG
    int splitMark = fragmentSplitMarker[i];
    // field data
    vtkIntArray* fdSM = vtkIntArray::New();
    fdSM->SetName("SplitGeometry");
    fdSM->SetNumberOfComponents(1);
    fdSM->SetNumberOfTuples(1);
    fdSM->SetValue(0, splitMark);
    fd->AddArray(fdSM);
    fdSM->Delete();
    // point data
    vtkIntArray* pdSM = vtkIntArray::New();
    pdSM->SetName("SplitGeometry");
    pdSM->SetNumberOfComponents(1);
    pdSM->SetNumberOfTuples(nPoints);
    pdSM->FillComponent(0, splitMark);
    pd->AddArray(pdSM);
    pdSM->Delete();
#endif
  } // for each local fragment
}

//----------------------------------------------------------------------------
// Output 1 is a multiblock with a block for each material containing
// a set of points at fragment centers. If center of mass hasn't been
// computed then the centers of axis aligned bounding boxes are used.
void vtkMaterialInterfaceFilter::CopyAttributesToOutput1()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::CopyAttributesToOutput1("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  vtkPolyData* resolvedFragmentCenters =
    dynamic_cast<vtkPolyData*>(this->ResolvedFragmentCenters->GetBlock(this->MaterialId));

  // only proc 0 will build the point set
  // all of the attributes are gathered there for
  // resolution so he has everything. Set the others to 0.
  const int procId = this->Controller->GetLocalProcessId();
  if (procId != 0)
  {
    // resolvedFragmentCenters->Delete();
    this->ResolvedFragmentCenters->SetBlock(this->MaterialId, static_cast<vtkPolyData*>(nullptr));
    return;
  }
  // Copy the attributes into point data
  vtkPointData* pd = resolvedFragmentCenters->GetPointData();
  vtkIntArray* ia = nullptr;
  vtkDoubleArray* da = nullptr;
  // 0 id
  ia = vtkIntArray::New();
  ia->SetName("Id");
  ia->SetNumberOfTuples(this->NumberOfResolvedFragments);
  for (int i = 0; i < this->NumberOfResolvedFragments; ++i)
  {
    ia->SetValue(i, i + this->ResolvedFragmentCount);
  }
  pd->AddArray(ia);
  // 1 material
  ReNewVtkPointer(ia);
  ia->SetName("Material");
  ia->SetNumberOfTuples(this->NumberOfResolvedFragments);
  ia->FillComponent(0, this->MaterialId);
  pd->AddArray(ia);
  // 2 volume
  da = vtkDoubleArray::New();
  da->DeepCopy(this->FragmentVolumes);
  da->SetName(this->FragmentVolumes->GetName());
  pd->AddArray(da);
  // Clip Depth
  if (this->ClipWithPlane)
  {
    da = vtkDoubleArray::New();
    da->DeepCopy(this->ClipDepthMaximums);
    da->SetName(this->ClipDepthMaximums->GetName());
    pd->AddArray(da);
    da = vtkDoubleArray::New();
    da->DeepCopy(this->ClipDepthMinimums);
    da->SetName(this->ClipDepthMinimums->GetName());
    pd->AddArray(da);
  }
  // 3 mass
  if (this->ComputeMoments)
  {
    ReNewVtkPointer(da);
    da->SetName("Mass");
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    da->CopyComponent(0, this->FragmentMoments, 3);
    pd->AddArray(da);
  }
  // 4 obb's
  if (this->ComputeOBB)
  {
    int idx = 0;
    ReNewVtkPointer(da);
    da->SetName("Bounding Box Origin");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q = 0; q < 3; ++q, ++idx)
    {
      da->CopyComponent(q, this->FragmentOBBs, idx);
    }
    pd->AddArray(da);
    ReNewVtkPointer(da);
    da->SetName("Bounding Box Axis 1");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q = 0; q < 3; ++q, ++idx)
    {
      da->CopyComponent(q, this->FragmentOBBs, idx);
    }
    pd->AddArray(da);
    ReNewVtkPointer(da);
    da->SetName("Bounding Box Axis 2");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q = 0; q < 3; ++q, ++idx)
    {
      da->CopyComponent(q, this->FragmentOBBs, idx);
    }
    pd->AddArray(da);
    ReNewVtkPointer(da);
    da->SetName("Bounding Box Axis 3");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q = 0; q < 3; ++q, ++idx)
    {
      da->CopyComponent(q, this->FragmentOBBs, idx);
    }
    pd->AddArray(da);
    ReNewVtkPointer(da);
    da->SetName("Bounding Box Length");
    da->SetNumberOfComponents(3);
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    for (int q = 0; q < 3; ++q, ++idx)
    {
      da->CopyComponent(q, this->FragmentOBBs, idx);
    }
    pd->AddArray(da);
  }
  // 5 ... i volume weighted averages
  for (int i = 0; i < this->NVolumeWtdAvgs; ++i)
  {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentVolumeWtdAvgs[i]);
    da->SetName(this->FragmentVolumeWtdAvgs[i]->GetName());
    pd->AddArray(da);
  }
  // i ... i+n mass weighted averages
  for (int i = 0; i < this->NMassWtdAvgs; ++i)
  {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentMassWtdAvgs[i]);
    da->SetName(this->FragmentMassWtdAvgs[i]->GetName());
    pd->AddArray(da);
  }
  // i+n+1 ... m sums
  for (int i = 0; i < this->NToSum; ++i)
  {
    ReNewVtkPointer(da);
    da->DeepCopy(this->FragmentSums[i]);
    da->SetName(this->FragmentSums[i]->GetName());
    pd->AddArray(da);
  }
  ReleaseVtkPointer(da);
// m+1 split geometry markers
#ifdef vtkMaterialInterfaceFilterDEBUG
  ReNewVtkPointer(ia);
  ia->DeepCopy(this->FragmentSplitGeometry);
  ia->SetName("SplitGeometry");
  pd->AddArray(ia);
#endif
  ReleaseVtkPointer(ia);
  // Compute the fragment centers and build vertices.
  vtkIdTypeArray* va = vtkIdTypeArray::New();
  va->SetNumberOfTuples(2 * this->NumberOfResolvedFragments);
  vtkIdType* verts = va->GetPointer(0);
  vtkPoints* pts = vtkPoints::New();
  // use center of mass if available
  if (this->ComputeMoments)
  {
    pts->SetDataTypeToDouble();
    da = dynamic_cast<vtkDoubleArray*>(pts->GetData());
    da->SetNumberOfTuples(this->NumberOfResolvedFragments);
    double* cen = da->GetPointer(0);
    const double* moments = this->FragmentMoments->GetPointer(0);
    for (int i = 0; i < this->NumberOfResolvedFragments; ++i)
    {
      verts[0] = 1;
      verts[1] = i;
      verts += 2;

      for (int q = 0; q < 3; ++q)
      {
        cen[q] = moments[q] / moments[3];
      }
      cen += 3;
      moments += 4;
    }
  }
  // use center of axis aligned bounding box
  else
  {
    pts->SetData(this->FragmentAABBCenters);
    for (int i = 0; i < this->NumberOfResolvedFragments; ++i)
    {
      verts[0] = 1;
      verts[1] = i;
      verts += 2;
    }
  }
  resolvedFragmentCenters->SetPoints(pts);
  pts->Delete();
  vtkCellArray* cells = vtkCellArray::New();
  cells->SetCells(static_cast<vtkIdType>(this->NumberOfResolvedFragments), va);
  resolvedFragmentCenters->SetVerts(cells);
  cells->Delete();
  va->Delete();
}

//----------------------------------------------------------------------------
// This output was added to validate the OBB code. Its is a
// multiblock of polydata. On the controlling process (eg 0), this
// output will contain polydata for each OBB.
void vtkMaterialInterfaceFilter::CopyAttributesToOutput2()
{
#ifdef vtkMaterialInterfaceFilterDEBUG
  ostringstream progressMesg;
  progressMesg << "vtkMaterialInterfaceFilter::CopyAttributesToOutput2("
               << ") , Material " << this->MaterialId;
  this->SetProgressText(progressMesg.str().c_str());
#endif
  this->Progress += this->ProgressResolutionInc;
  this->UpdateProgress(this->Progress);

  // Nothing to do if there are no OBB's.
  if (!this->ComputeOBB)
  {
    return;
  }

  // only proc 0 will build the OBB representations
  // all of the attributes are gathered there for
  // resolution so he has everything. Set the others to 0.
  const int procId = this->Controller->GetLocalProcessId();
  if (procId != 0)
  {
    // resolvedFragmentCenters->Delete();
    this->ResolvedFragmentOBBs->SetBlock(this->MaterialId, static_cast<vtkPolyData*>(nullptr));
    return;
  }

  // We will graphically represent each OBB with 2 triangle strips.
  // Each cover three sides like a hot dog bun. 8 points are needed
  // for each strip.
  vtkIdType basePtId = 0;
  vtkPoints* repPts = vtkPoints::New();
  repPts->SetDataTypeToDouble();
  repPts->SetNumberOfPoints(8 * this->NumberOfResolvedFragments);
  vtkCellArray* repStrips = vtkCellArray::New();
  // repStrips->SetNumberOfCells(2*this->NumberOfResolvedFragments);
  // for each fragment build its obb's representation
  for (int fragmentId = 0; fragmentId < this->NumberOfResolvedFragments; ++fragmentId)
  {
    // indexes into obb tuple.
    const int x0 = 0;
    const int d0 = 3;
    const int d1 = 6;
    const int d2 = 9;
    // pt ids.
    vtkIdType a = basePtId;
    vtkIdType b = basePtId + 1;
    vtkIdType c = basePtId + 2;
    vtkIdType d = basePtId + 3;
    vtkIdType e = basePtId + 4;
    vtkIdType f = basePtId + 5;
    vtkIdType g = basePtId + 6;
    vtkIdType h = basePtId + 7;

    // Get the fragment obb tuple.
    double obb[15];
    this->FragmentOBBs->GetTuple(fragmentId, obb);

    // Construct the 8 corners of the OBB.
    double pts[3];
    // a
    pts[0] = obb[x0];
    pts[1] = obb[x0 + 1];
    pts[2] = obb[x0 + 2];
    repPts->SetPoint(a, pts);
    // b
    pts[0] = obb[x0] + obb[d0];
    pts[1] = obb[x0 + 1] + obb[d0 + 1];
    pts[2] = obb[x0 + 2] + obb[d0 + 2];
    repPts->SetPoint(b, pts);
    // c
    pts[0] = obb[x0] + obb[d0] + obb[d1];
    pts[1] = obb[x0 + 1] + obb[d0 + 1] + obb[d1 + 1];
    pts[2] = obb[x0 + 2] + obb[d0 + 2] + obb[d1 + 2];
    repPts->SetPoint(c, pts);
    // d
    pts[0] = obb[x0] + obb[d1];
    pts[1] = obb[x0 + 1] + obb[d1 + 1];
    pts[2] = obb[x0 + 2] + obb[d1 + 2];
    repPts->SetPoint(d, pts);
    // e
    pts[0] = obb[x0] + obb[d2];
    pts[1] = obb[x0 + 1] + obb[d2 + 1];
    pts[2] = obb[x0 + 2] + obb[d2 + 2];
    repPts->SetPoint(e, pts);
    // f
    pts[0] = obb[x0] + obb[d0] + obb[d2];
    pts[1] = obb[x0 + 1] + obb[d0 + 1] + obb[d2 + 1];
    pts[2] = obb[x0 + 2] + obb[d0 + 2] + obb[d2 + 2];
    repPts->SetPoint(f, pts);
    // g
    pts[0] = obb[x0] + obb[d0] + obb[d1] + obb[d2];
    pts[1] = obb[x0 + 1] + obb[d0 + 1] + obb[d1 + 1] + obb[d2 + 1];
    pts[2] = obb[x0 + 2] + obb[d0 + 2] + obb[d1 + 2] + obb[d2 + 2];
    repPts->SetPoint(g, pts);
    // h
    pts[0] = obb[x0] + obb[d1] + obb[d2];
    pts[1] = obb[x0 + 1] + obb[d1 + 1] + obb[d2 + 1];
    pts[2] = obb[x0 + 2] + obb[d1 + 2] + obb[d2 + 2];
    repPts->SetPoint(h, pts);

    // Build the repStrips' cells.
    // wrapper 1
    repStrips->InsertNextCell(8);
    repStrips->InsertCellPoint(d);
    repStrips->InsertCellPoint(a);
    repStrips->InsertCellPoint(c);
    repStrips->InsertCellPoint(b);
    repStrips->InsertCellPoint(g);
    repStrips->InsertCellPoint(f);
    repStrips->InsertCellPoint(h);
    repStrips->InsertCellPoint(e);

    // wrapper 2
    repStrips->InsertNextCell(8);
    repStrips->InsertCellPoint(c);
    repStrips->InsertCellPoint(g);
    repStrips->InsertCellPoint(d);
    repStrips->InsertCellPoint(h);
    repStrips->InsertCellPoint(a);
    repStrips->InsertCellPoint(e);
    repStrips->InsertCellPoint(b);
    repStrips->InsertCellPoint(f);

    // next strip
    basePtId += 8;
  }

  // Place the obb represntations (points and cells) into
  // the output.
  vtkPolyData* resolvedFragmentOBBs =
    dynamic_cast<vtkPolyData*>(this->ResolvedFragmentOBBs->GetBlock(this->MaterialId));
  resolvedFragmentOBBs->SetPoints(repPts);
  resolvedFragmentOBBs->SetStrips(repStrips);
  repPts->Delete();
  repStrips->Delete();
}

//----------------------------------------------------------------------------
// For each fragment compute its oriented bounding box(OBB).
//
// return 0 on error.
int vtkMaterialInterfaceFilter::ComputeLocalFragmentOBB()
{
  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));

  vector<int>& fragmentSplitMarker = this->FragmentSplitMarker[this->MaterialId];

  int nLocal = static_cast<int>(resolvedFragmentIds.size());

  // OBB set up
  vtkOBBTree* obbCalc;
  double* pObb;
  obbCalc = vtkOBBTree::New();
  assert("FragmentOBBs has incorrect size." && this->FragmentOBBs->GetNumberOfTuples() == nLocal);
  pObb = this->FragmentOBBs->GetPointer(0);

  // Traverse the fragments we own
  for (int i = 0; i < nLocal; ++i)
  {
    // skip split fragments, these have already been
    // taken care of.
    if (fragmentSplitMarker[i] == 1)
    {
      pObb += 15;
      continue;
    }

    // get fragment mesh
    int globalId = resolvedFragmentIds[i];
    vtkPolyData* thisFragment = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));

    // compute OBB
    double size[3];
    // (c_x,c_y,c_z),(max_x,max_y,max_z),(mid_x,mid_y,mid_z),(min_x,min_y,min_z),|max|,|mid|,|min|
    obbCalc->ComputeOBB(thisFragment, pObb, pObb + 3, pObb + 6, pObb + 9, size);
    // obbCalc->ComputeOBB(thisFragment->GetPoints(),pObb,pObb+3,pObb+6,pObb+9,size);

    // compute magnitudes
    for (int q = 0; q < 3; ++q)
    {
      pObb[12 + q] = 0;
    }
    for (int q = 0; q < 3; ++q)
    {
      pObb[12] += pObb[3 + q] * pObb[3 + q];
      pObb[13] += pObb[6 + q] * pObb[6 + q];
      pObb[14] += pObb[9 + q] * pObb[9 + q];
    }
    for (int q = 0; q < 3; ++q)
    {
      pObb[12 + q] = sqrt(pObb[12 + q]);
    }

    pObb += 15;
  } // fragment traversal

  // OBB clean up
  obbCalc->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// For each fragment compute its axis-aligned bounding box(AABB).
//
// return 0 on error.
int vtkMaterialInterfaceFilter::ComputeLocalFragmentAABBCenters()
{
  vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[this->MaterialId];

  vtkMultiPieceDataSet* resolvedFragments =
    dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(this->MaterialId));

  vector<int>& fragmentSplitMarker = this->FragmentSplitMarker[this->MaterialId];

  int nLocal = static_cast<int>(resolvedFragmentIds.size());

  // AABB set up
  assert("FragmentAABBCenters is expected to be pre-allocated." &&
    this->FragmentAABBCenters->GetNumberOfTuples() == nLocal);
  double aabb[6];
  double* pCoaabb = this->FragmentAABBCenters->GetPointer(0);

  // Traverse the fragments we own
  for (int i = 0; i < nLocal; ++i)
  {
    // skip fragments with geometry split over multiple
    // processes. These have been already taken care of.
    if (fragmentSplitMarker[i] == 1)
    {
      pCoaabb += 3;
      continue;
    }

    int globalId = resolvedFragmentIds[i];

    vtkPolyData* thisFragment = dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(globalId));

    // AABB calculation
    thisFragment->GetBounds(aabb);
    for (int q = 0, k = 0; q < 3; ++q, k += 2)
    {
      pCoaabb[q] = (aabb[k] + aabb[k + 1]) / 2.0;
    }
    pCoaabb += 3;
  } // fragment traversal

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
int vtkMaterialInterfaceFilter::BuildOutputs(vtkMultiBlockDataSet* mbdsOutput0,
  vtkMultiBlockDataSet* mbdsOutput1, vtkMultiBlockDataSet* mbdsOutput2, int nMaterials)
{
  /// 0
  // we have a multi-block which is a container for the fragment sets
  // from each material.
  this->ResolvedFragments = mbdsOutput0;
  this->ResolvedFragments->SetNumberOfBlocks(nMaterials);
  /// 1
  this->ResolvedFragmentCenters = mbdsOutput1;
  this->ResolvedFragmentCenters->SetNumberOfBlocks(nMaterials);
/// 2
#ifdef vtkMaterialInterfaceFilterDEBUG
  if (this->ComputeOBB)
  {
    this->ResolvedFragmentOBBs = mbdsOutput2;
    this->ResolvedFragmentOBBs->SetNumberOfBlocks(nMaterials);
  }
#else
  (void)mbdsOutput2; // Fixes compiler warning.
#endif

  for (int i = 0; i < nMaterials; ++i)
  {
    /// 0
    // A multi-piece container for polydata,
    // one for each of the fragments in this material.
    vtkMultiPieceDataSet* mpds = vtkMultiPieceDataSet::New();
    this->ResolvedFragments->SetBlock(i, mpds);
    mpds->Delete();
    /// 1
    // A single poly data holding all the cell centers.
    // of fragments in this material.
    vtkPolyData* pdFcc = vtkPolyData::New();
    this->ResolvedFragmentCenters->SetBlock(i, pdFcc);
    pdFcc->Delete();
/// 2
#ifdef vtkMaterialInterfaceFilterDEBUG
    // A single poly data holding OBBs of fragments
    // in this material.
    if (this->ComputeOBB)
    {
      vtkPolyData* pdOBB = vtkPolyData::New();
      this->ResolvedFragmentOBBs->SetBlock(i, pdOBB);
      pdOBB->Delete();
    }
#endif
  }
  /// Global information (i.e. maintained across materials)
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
  this->ResolvedFragmentCount = 0;

  return 1;
}

//----------------------------------------------------------------------------
// Write the fragment attribute to a text file. Each process
// writes its pieces.
//
//  For now duplicates might arise, however
// the attributes are global so that they should have the same
// value.
int vtkMaterialInterfaceFilter::WriteGeometryOutputToTextFile()
{
  const int myProcId = this->Controller->GetLocalProcessId();

  // open the file
  ostringstream fileName;
  fileName << this->OutputBaseName << "." << myProcId << ".miffc.G";

  ofstream fout;
  fout.open(fileName.str().c_str());
  if (!fout.is_open())
  {
    vtkErrorMacro("Could not open " << fileName.str() << ".");
    return 0;
  }

  int nMaterials = this->ResolvedFragments->GetNumberOfBlocks();
  // file header
  fout << "Header:{" << endl;
  fout << "Process id:" << myProcId << endl;
  fout << "Number of fragments:" << this->ResolvedFragmentCount << endl;
  fout << "Number of materials:" << nMaterials << endl;
  fout << "}" << endl;

  for (int materialId = 0; materialId < nMaterials; ++materialId)
  {
    fout << "Material " << materialId << " :{" << endl;

    // all fragments store the same info within a material
    // so we get the first and use that to build the file header
    vector<int>& resolvedFragmentIds = this->ResolvedFragmentIds[materialId];
    vtkMultiPieceDataSet* resolvedFragments =
      dynamic_cast<vtkMultiPieceDataSet*>(this->ResolvedFragments->GetBlock(materialId));
    assert("Couldn't get the resolved fragnments." && resolvedFragments);

    // We don't have any fragments here
    if (resolvedFragmentIds.size() == 0)
    {
      continue;
    }

    int firstLocalId = resolvedFragmentIds[0];
    vtkFieldData* rf0fd =
      dynamic_cast<vtkPolyData*>(resolvedFragments->GetPiece(firstLocalId))->GetFieldData();

    int nAttributes = rf0fd->GetNumberOfArrays();
    int nLocalFragments = static_cast<int>(resolvedFragmentIds.size());

    // write header
    fout << "Header:{" << endl;
    fout << "Number total:" << resolvedFragments->GetNumberOfPieces() << endl;
    fout << "Number local:" << nLocalFragments << endl;
    fout << "Number of attributes:" << nAttributes << endl;
    fout << "}" << endl;
    // write attribute names
    fout << "Attributes:{" << endl;
    for (int i = 0; i < nAttributes; ++i)
    {
      vtkDataArray* aa = rf0fd->GetArray(i);
      fout << "\"" << aa->GetName() << "\", " << aa->GetNumberOfComponents() << endl;
    }
    fout << "}" << endl;
    // write attributes
    fout << "Attribute Data:{" << endl;
    for (int i = 0; i < nLocalFragments; ++i)
    {
      int globalId = resolvedFragmentIds[i];
      vtkFieldData* fd = resolvedFragments->GetPiece(globalId)->GetFieldData();
      // first
      vtkDataArray* aa = fd->GetArray(0);
      int nComps = aa->GetNumberOfComponents();
      vector<double> tup;
      tup.resize(nComps);
      CopyTuple(&tup[0], aa, nComps, 0);
      fout << tup[0];
      for (int q = 1; q < nComps; ++q)
      {
        fout << " " << tup[q];
      }
      // rest
      for (int j = 1; j < nAttributes; ++j)
      {
        fout << ", ";
        aa = fd->GetArray(j);
        nComps = aa->GetNumberOfComponents();
        tup.resize(nComps);
        CopyTuple(&tup[0], aa, nComps, 0);
        fout << tup[0];
        for (int q = 1; q < nComps; ++q)
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

//----------------------------------------------------------------------------
// Write the fragment attribute attached ot the statistics output
// to a text file. This is roughly a csv format except that column headers
// are written one per line follwed by the number of components per tuple.
int vtkMaterialInterfaceFilter::WriteStatisticsOutputToTextFile()
{
  // Only proc 0 has the stats output.
  int myProcId = this->Controller->GetLocalProcessId();
  if (myProcId != 0)
  {
    return true;
  }

  // open the file
  ostringstream fileName;
  fileName << this->OutputBaseName << ".miffc.S";

  ofstream fout;
  fout.open(fileName.str().c_str());
  if (!fout.is_open())
  {
    vtkErrorMacro("Could not open " << fileName.str() << ".");
    return 0;
  }
  fout.setf(std::ios::scientific, std::ios::floatfield);
  fout.precision(6);

  // Write the csv file in block order. The format will be:
  // AttributeName1 nAttributeComponents1
  // ...
  // AttributeNameN nAttributeComponentsN
  // centers1, attribute1 0, ... , attributeN N
  // ...
  // centersN, attribute1 0, ... , attributeN N
  //
  // NOTE: Centers, Material Id and fargmentId are always written.
  int nMaterials = this->ResolvedFragmentCenters->GetNumberOfBlocks();
  bool hasHeader = false;
  for (int materialId = 0; materialId < nMaterials; ++materialId)
  {
    vtkPolyData* fragmentCenters =
      dynamic_cast<vtkPolyData*>(this->ResolvedFragmentCenters->GetBlock(materialId));

    int nFragments = fragmentCenters->GetNumberOfPoints();
    if (nFragments > 0)
    {
      vtkPointData* pd = fragmentCenters->GetPointData();
      int nArrays = pd->GetNumberOfArrays();
      // The first material that has any fragments will be
      // used to write the header.
      if (!hasHeader)
      {
        // Write the field names.
        fout << "\"Centers\", 3" << endl;
        for (int arrayId = 0; arrayId < nArrays; ++arrayId)
        {
          vtkDataArray* da = pd->GetArray(arrayId);
          fout << "\"" << da->GetName() << "\", " << da->GetNumberOfComponents() << endl;
        }
        hasHeader = true;
      }
      // At each vertex write its coordinates and attributes.
      vtkDoubleArray* cen = dynamic_cast<vtkDoubleArray*>(fragmentCenters->GetPoints()->GetData());
      double* pCen = cen->GetPointer(0);
      for (int localId = 0; localId < nFragments; ++localId)
      {
        fout << pCen[0] << " " << pCen[1] << " " << pCen[2];
        for (int arrayId = 0; arrayId < nArrays; ++arrayId)
        {
          vtkDataArray* a = pd->GetArray(arrayId);
          int nComps = a->GetNumberOfComponents();
          int idx = nComps * localId;
          vtkIntArray* ia = nullptr;
          vtkDoubleArray* da = nullptr;
          if ((ia = dynamic_cast<vtkIntArray*>(a)) != nullptr)
          {
            int* pIa = ia->GetPointer(0);
            pIa += idx;
            for (int q = 0; q < nComps; ++q)
            {
              fout << ", " << pIa[q];
            }
          }
          else if ((da = dynamic_cast<vtkDoubleArray*>(a)) != nullptr)
          {
            double* pDa = da->GetPointer(0);
            pDa += idx;
            for (int q = 0; q < nComps; ++q)
            {
              fout << ", " << pDa[q];
            }
          }
        }
        fout << endl;
        pCen += 3;
      }
    }
  }
  fout.flush();
  fout.close();

  return 1;
}

//============================================================================
// Implementation of the object that computes the fraction of a box clipped
// by a half sphere implicit funtcion.
// The algorithm is to use a case table to construct the clipped polyhedra.
// The case table is similar to marching cubes, but the result is a list
// of triangles for the closed polyhedra.  We then use the projected areas
// to compute the volume of the polyhedra (similar to mass properties filter).
//============================================================================

vtkMaterialInterfaceFilterHalfSphere::vtkMaterialInterfaceFilterHalfSphere()
{
  // Parameters for clipping with a sphere or half sphere
  this->ClipWithPlane = 1;
  this->ClipWithSphere = 0;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;
  this->SphereRadius = 1.0;
  this->PlaneNormal[0] = 0.0;
  this->PlaneNormal[1] = 0.0;
  this->PlaneNormal[2] = 1.0;
}

struct vtkHexClipCase
{ // Maximum of 8 triangles with 1 terminating -1.
  int TriangleList[25];
};

// 0: all out,  255: all in.
// corners for case 0-7:
// (0,0,0)(1,0,0)(1,1,0),(0,1,0)(0,0,1)(1,0,1)(1,1,1),(0,1,1)
// edges 0-11:
// (0,0,.)(1,0,.),(0,1,.)(1,1,.)(0,|,0)(1,|,0),(0,|,1)(1,|,1)(-,0,0)(-,1,0)(-,0,1)(-,1,0)
// Corner points for surface triangles arbitrary starting index 20-27 same order as above.
// Order of triangle points: Right hand rule points in.
// We skipped triangels that do not contribute to the area calculation:
// Side faces that project to a line on a z plane. Triangles on the base (dZ = 0).
#include "vtkMaterialInterfaceFilterCases.cxx"

//----------------------------------------------------------------------------
double vtkMaterialInterfaceFilterHalfSphere::ComputeTriangleProjectionArea(
  double pt1[3], double pt2[3], double pt3[3], double zPlane)
{
  // The volume under the triangle is the average z time the area of the
  // projected triangle.  Use a reference z plane to avoid numerical issues.
  double zSum = ((pt1[2] - zPlane) + (pt1[2] - zPlane) + (pt1[2] - zPlane));

  // Compute the area of the triangle projected on the zPlane.
  double v1[3], v2[3], cross[3];
  v1[0] = pt1[0] - pt3[0];
  v1[1] = pt1[1] - pt3[1];
  v1[2] = 0.0;
  v2[0] = pt2[0] - pt3[0];
  v2[1] = pt2[1] - pt3[1];
  v2[2] = 0.0;
  vtkMath::Cross(v1, v2, cross);
  // Magnitude of the cross is twice the area of the projected triangle.
  // We have to compute the sign of the area by using the orientation of the
  // triangle.  Right hand rule -> positive, left->Negative.

  return cross[2] * zSum / 6.0;
}

//----------------------------------------------------------------------------
// Evaluate the half sphere over a hex cell.
// Pass in the bounds of the axis aligned hex cell, and the method returns
// the fraction of the cell inside the half sphere.
double vtkMaterialInterfaceFilterHalfSphere::EvaluateHalfSphereBox(double bounds[6])
{
  // Try to make a quick decision on whether to test the box.
  if (this->ClipWithSphere)
  {
    if (bounds[0] > this->Center[0] + this->SphereRadius ||
      bounds[1] < this->Center[0] - this->SphereRadius ||
      bounds[2] > this->Center[1] + this->SphereRadius ||
      bounds[3] < this->Center[1] - this->SphereRadius ||
      bounds[4] > this->Center[2] + this->SphereRadius ||
      bounds[5] < this->Center[2] - this->SphereRadius)
    { // Outside of sphere bounding box.
      return 0.0;
    }
  }
  // We could test the inscribed box and the plane too.

  // We evaluate corners independently, but it should not cost to much extra
  // time.  The only way I can see sharing corner values is by creating
  // another array.  Four ighbors may share the same corner point.
  double pt[3];
  double cornerValues[8];

  pt[0] = bounds[0];
  pt[1] = bounds[2];
  pt[2] = bounds[4];
  cornerValues[0] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[1];
  cornerValues[1] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[0];
  pt[1] = bounds[3];
  cornerValues[2] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[1];
  cornerValues[3] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[0];
  pt[1] = bounds[2];
  pt[2] = bounds[5];
  cornerValues[4] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[1];
  cornerValues[5] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[0];
  pt[1] = bounds[3];
  cornerValues[6] = this->EvaluateHalfSpherePoint(pt);
  pt[0] = bounds[1];
  cornerValues[7] = this->EvaluateHalfSpherePoint(pt);

  // Compute marching cubes case.  Case is based on voxel point order.
  unsigned char clipCase = 0;
  if (cornerValues[0] < 0.0)
  {
    clipCase += 1;
  }
  if (cornerValues[1] < 0.0)
  {
    clipCase += 2;
  }
  if (cornerValues[2] < 0.0)
  {
    clipCase += 4;
  }
  if (cornerValues[3] < 0.0)
  {
    clipCase += 8;
  }
  if (cornerValues[4] < 0.0)
  {
    clipCase += 16;
  }
  if (cornerValues[5] < 0.0)
  {
    clipCase += 32;
  }
  if (cornerValues[6] < 0.0)
  {
    clipCase += 64;
  }
  if (cornerValues[7] < 0.0)
  {
    clipCase += 128;
  }
  if (clipCase == 0)
  {
    return 0.0;
  }
  if (clipCase == 255)
  {
    return 1.0;
  }
  // Use case to look up the list of triangles for the closed surface of the
  // clipped cell.  Triangle points can be cell corners of points on edges.
  // 0-11 are edges:(x,0,0)(x,1,0)(x,0,1)(x,1,1)(0,y,0)(1,y,0)(0,y,1)(1,y,1)...
  // 20-27 are corners:(0,0,0)(1,0,0)(0,1,0)(1,1,0)(0,0,1)(1,0,1)(0,1,1) ...

  vtkHexClipCase* hexCase = VTK_HEX_CLIP_TRICASES + clipCase;
  int* tris = hexCase->TriangleList;

  // Loop through the triangles computing volume.
  // Compute points on demand.
  int pointFlags[28];
  memset((void*)(pointFlags), 0, 28 * sizeof(int));
  double points[28][3];
  double volume = 0;
  double* pt1;
  double* pt2;
  double* pt3;
  while (*tris > -1)
  {
    pt1 = this->GetCasePoint(*tris++, bounds, cornerValues, points, pointFlags);
    pt2 = this->GetCasePoint(*tris++, bounds, cornerValues, points, pointFlags);
    pt3 = this->GetCasePoint(*tris++, bounds, cornerValues, points, pointFlags);
    volume += this->ComputeTriangleProjectionArea(pt1, pt2, pt3, bounds[4]);
  }

  // Now compute a fraction of coverage.
  return volume / ((bounds[1] - bounds[0]) * (bounds[3] - bounds[2]) * (bounds[5] - bounds[4]));
}

//----------------------------------------------------------------------------
double* vtkMaterialInterfaceFilterHalfSphere::GetCasePoint(
  int pointIdx, double bds[6], double cVals[8], double points[28][3], int pointFlags[28])
{
  double* pt = points[pointIdx];

  if (pointFlags[pointIdx])
  {
    return pt;
  }
  pointFlags[pointIdx] = 1;
  if (pointIdx >= 20)
  { // Corner
    pointIdx = pointIdx - 20;
    if (pointIdx & 1)
    {
      pt[0] = bds[1];
    }
    else
    {
      pt[0] = bds[0];
    }
    if (pointIdx & 2)
    {
      pt[1] = bds[3];
    }
    else
    {
      pt[1] = bds[2];
    }
    if (pointIdx & 4)
    {
      pt[2] = bds[5];
    }
    else
    {
      pt[2] = bds[4];
    }
    return pt;
  }
  // Just use a switch for computing the edge intersections.
  switch (pointIdx)
  {
    case 0:
    { // (0,0,.)
      pt[0] = bds[0];
      pt[1] = bds[2];
      pt[2] = bds[4] + (bds[5] - bds[4]) * (cVals[0] / (cVals[0] - cVals[4]));
      break;
    }
    case 1:
    { // (1,0,.)
      pt[0] = bds[1];
      pt[1] = bds[2];
      pt[2] = bds[4] + (bds[5] - bds[4]) * (cVals[1] / (cVals[1] - cVals[5]));
      break;
    }
    case 2:
    { // (0,1,.)
      pt[0] = bds[0];
      pt[1] = bds[3];
      pt[2] = bds[4] + (bds[5] - bds[4]) * (cVals[2] / (cVals[2] - cVals[6]));
      break;
    }
    case 3:
    { // (1,1,.)
      pt[0] = bds[1];
      pt[1] = bds[3];
      pt[2] = bds[4] + (bds[5] - bds[4]) * (cVals[3] / (cVals[3] - cVals[7]));
      break;
    }
    case 4:
    { // (0,|,0)
      pt[0] = bds[0];
      pt[1] = bds[2] + (bds[3] - bds[2]) * (cVals[0] / (cVals[0] - cVals[2]));
      pt[2] = bds[4];
      break;
    }
    case 5:
    { // (1,|,0)
      pt[0] = bds[1];
      pt[1] = bds[2] + (bds[3] - bds[2]) * (cVals[1] / (cVals[1] - cVals[3]));
      pt[2] = bds[4];
      break;
    }
    case 6:
    { // (0,|,1)
      pt[0] = bds[0];
      pt[1] = bds[2] + (bds[3] - bds[2]) * (cVals[4] / (cVals[4] - cVals[6]));
      pt[2] = bds[5];
      break;
    }
    case 7:
    { // (1,|,1)
      pt[0] = bds[1];
      pt[1] = bds[2] + (bds[3] - bds[2]) * (cVals[5] / (cVals[5] - cVals[7]));
      pt[2] = bds[5];
      break;
    }
    case 8:
    { // (-,0,0)
      pt[0] = bds[0] + (bds[1] - bds[0]) * (cVals[0] / (cVals[0] - cVals[1]));
      pt[1] = bds[2];
      pt[2] = bds[4];
      break;
    }
    case 9:
    { // (-,1,0)
      pt[0] = bds[0] + (bds[1] - bds[0]) * (cVals[2] / (cVals[2] - cVals[3]));
      pt[1] = bds[3];
      pt[2] = bds[4];
      break;
    }
    case 10:
    { // (-,0,1)
      pt[0] = bds[0] + (bds[1] - bds[0]) * (cVals[4] / (cVals[4] - cVals[5]));
      pt[1] = bds[2];
      pt[2] = bds[5];
      break;
    }
    case 11:
    { // (-,1,1)
      pt[0] = bds[0] + (bds[1] - bds[0]) * (cVals[6] / (cVals[6] - cVals[7]));
      pt[1] = bds[3];
      pt[2] = bds[5];
      break;
    }
    default:
    {
      vtkGenericWarningMacro("Unknown point code: " << pointIdx);
    }
  }
  return pt;
}

//----------------------------------------------------------------------------
// Evaluate the half sphere function at a point.
double vtkMaterialInterfaceFilterHalfSphere::EvaluateHalfSpherePoint(double pt[3])
{
  // Move the center of the sphere to the origin.
  double tmp[3];
  tmp[0] = pt[0] - this->Center[0];
  tmp[1] = pt[1] - this->Center[1];
  tmp[2] = pt[2] - this->Center[2];

  double sphereDist = 0.0;
  if (this->ClipWithSphere)
  {
    // First compute the distance squared from the sphere.
    sphereDist = tmp[0] * tmp[0];
    sphereDist += tmp[1] * tmp[1];
    sphereDist += tmp[2] * tmp[2];
    // Subtract the radius to put the surface at 0.0 and positive inside.
    sphereDist = sqrt(sphereDist) - this->SphereRadius;
    if (this->ClipWithPlane == 0)
    {
      return sphereDist;
    }
  }

  // Now the plane.
  double planeDot;
  planeDot =
    this->PlaneNormal[0] * tmp[0] + this->PlaneNormal[1] * tmp[1] + this->PlaneNormal[2] * tmp[2];
  planeDot = -planeDot;
  if (this->ClipWithSphere == 0)
  {
    return planeDot;
  }

  double combined;
  // Take the maximum (union) for inversion.
  combined = planeDot;
  if (combined < sphereDist)
  {
    combined = sphereDist;
  }

  return combined;
}
