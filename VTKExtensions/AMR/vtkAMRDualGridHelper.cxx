/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDualGridHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRDualGridHelper.h"
#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkSortDataArray.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <list>
#include <vector>

#include "vtksys/SystemTools.hxx"

// Determine if we can use the MPI controller for asynchronous communication.
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#define VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif

vtkStandardNewMacro(vtkAMRDualGridHelper);

class vtkAMRDualGridHelperSeed;

// For debugging only
// vtkPolyData* DebuggingPolyData;
// vtkIntArray* DebuggingAttributes;
// static double DebuggingGlobalOrigin[3];
// static double DebuggingRootSpacing[3];

//=============================================================================
// Tags used in communication.
static const int SHARED_BLOCK_TAG = 2392734;
static const int DEGENERATE_REGION_TAG = 879015;

//=============================================================================
#if 1
namespace
{
// A convenience class for marking the start and end of a function (or any
// scope).  Simply declare the class at the start of the function and it will
// automatically call vtkTimerLog::MarkStartEvent() at the beginning and
// vtkTimerLog::MarkEndEvent() whenever it leaves regardless of where that
// happens.
class vtkTimerLogSmartMarkEvent
{
public:
  vtkTimerLogSmartMarkEvent(
    const char* eventString, vtkMultiProcessController* controller = nullptr)
    : EventString(eventString)
    , Controller(controller)
  {
    if (this->Controller)
      this->Controller->Barrier();
    vtkTimerLog::MarkStartEvent(this->EventString.c_str());
  }
  ~vtkTimerLogSmartMarkEvent()
  {
    if (this->Controller)
      this->Controller->Barrier();
    vtkTimerLog::MarkEndEvent(this->EventString.c_str());
  }

private:
  std::string EventString;
  vtkSmartPointer<vtkMultiProcessController> Controller;
  vtkTimerLogSmartMarkEvent(const vtkTimerLogSmartMarkEvent&) = delete;
  void operator=(const vtkTimerLogSmartMarkEvent&) = delete;
};
};
#endif

//============================================================================
// Helper object for getting information from AMR datasets.
// API:
// Have a block object as part of the API? Yes; Level? No.
// Initialize helper with a CTH dataset.
// Get GlobalOrigin, RootSpacing, NumberOfLevels
//     ?StandardCellDimensions(block with ghost levels)
// Get NumberOfBlocksInLevel (level);
// GetBlock(level, blockIdx)
// BlockAPI.

// Neighbors: Specify a block with level and grid position.
//     Get NumberOfNeighbors on any of the six faces.

//----------------------------------------------------------------------------
class vtkAMRDualGridHelperLevel
{
public:
  vtkAMRDualGridHelperLevel();
  ~vtkAMRDualGridHelperLevel();

  // Level is stored implicitly in the Helper,
  // but it can't hurn to have it here too.
  int Level;

  void CreateBlockFaces(vtkAMRDualGridHelperBlock* block, int x, int y, int z);
  std::vector<vtkAMRDualGridHelperBlock*> Blocks;

  // I need my own container because the 2D
  // grid can expand in all directions.
  // the block in grid index 0,0 has its origin on the global origin.
  // I think I will make this grid temporary for initialization only.
  int GridExtent[6];
  int GridIncY;
  int GridIncZ;
  vtkAMRDualGridHelperBlock** Grid;

  vtkAMRDualGridHelperBlock* AddGridBlock(int x, int y, int z, int id, vtkImageData* volume);
  vtkAMRDualGridHelperBlock* GetGridBlock(int x, int y, int z);

private:
};

//----------------------------------------------------------------------------
// Degenerate regions that span processes are kept them in a queue
// to communicate and process all at once.  This is the queue item.
class vtkAMRDualGridHelperDegenerateRegion
{
public:
  vtkAMRDualGridHelperDegenerateRegion();
  int ReceivingRegion[3];
  vtkAMRDualGridHelperBlock* SourceBlock;
  vtkDataArray* SourceArray;
  vtkAMRDualGridHelperBlock* ReceivingBlock;
  vtkDataArray* ReceivingArray;
};
vtkAMRDualGridHelperDegenerateRegion::vtkAMRDualGridHelperDegenerateRegion()
{
  this->ReceivingRegion[0] = 0;
  this->ReceivingRegion[1] = 0;
  this->ReceivingRegion[2] = 0;
  this->ReceivingBlock = this->SourceBlock = nullptr;
  this->ReceivingArray = this->SourceArray = nullptr;
}

//-----------------------------------------------------------------------------
// Simple containers for managing asynchronous communication.
#ifdef VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
struct vtkAMRDualGridHelperCommRequest
{
  vtkMPICommunicator::Request Request;
  vtkSmartPointer<vtkDataArray> Buffer;
  int SendProcess;
  int ReceiveProcess;
};

// This class is a STL list of vtkAMRDualGridHelperCommRequest structs with some
// helper methods added.
class vtkAMRDualGridHelperCommRequestList : public std::list<vtkAMRDualGridHelperCommRequest>
{
public:
  // Description:
  // Waits for all of the communication to complete.
  void WaitAll()
  {
    for (iterator i = this->begin(); i != this->end(); i++)
      i->Request.Wait();
  }
  // Description:
  // Waits for one of the communications to complete, removes it from the list,
  // and returns it.
  value_type WaitAny()
  {
    while (!this->empty())
    {
      for (iterator i = this->begin(); i != this->end(); i++)
      {
        if (i->Request.Test())
        {
          value_type retval = *i;
          this->erase(i);
          return retval;
        }
      }
      vtksys::SystemTools::Delay(1);
    }
    vtkGenericWarningMacro(<< "Nothing to wait for.");
    return value_type();
  }
};
#endif // VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS

//----------------------------------------------------------------------------
vtkAMRDualGridHelperSeed::vtkAMRDualGridHelperSeed()
{
  this->Index[0] = -1;
  this->Index[1] = -1;
  this->Index[2] = -1;
  this->FragmentId = 0;
}
//****************************************************************************
vtkAMRDualGridHelperLevel::vtkAMRDualGridHelperLevel()
{
  this->Level = 0;
  this->Grid = nullptr;
  for (int ii = 0; ii < 3; ++ii)
  {
    this->GridExtent[2 * ii] = 0;
    this->GridExtent[2 * ii + 1] = -1;
  }
}
//----------------------------------------------------------------------------
vtkAMRDualGridHelperLevel::~vtkAMRDualGridHelperLevel()
{
  int ii;
  int num = (int)(this->Blocks.size());

  this->Level = -1;
  for (ii = 0; ii < num; ++ii)
  {
    if (this->Blocks[ii])
    {
      delete this->Blocks[ii];
      this->Blocks[ii] = nullptr;
    }
  }

  for (ii = 0; ii < 6; ++ii)
  {
    this->GridExtent[ii] = 0;
  }
  // The grid does not "own" the blocks
  // so it does not need to delete them.
  if (this->Grid)
  {
    delete[] this->Grid;
    this->Grid = nullptr;
  }
}
//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRDualGridHelperLevel::GetGridBlock(int x, int y, int z)
{
  if (x < this->GridExtent[0] || x > this->GridExtent[1])
  {
    return nullptr;
  }
  if (y < this->GridExtent[2] || y > this->GridExtent[3])
  {
    return nullptr;
  }
  if (z < this->GridExtent[4] || z > this->GridExtent[5])
  {
    return nullptr;
  }

  return this->Grid[x + y * this->GridIncY + z * this->GridIncZ];
}

//----------------------------------------------------------------------------
// This method is meant to be called after all the blocks are created and
// in their level grids.  It should also be called after FindExistingFaces
// is called for this level, but before FindExisitngFaces is called for
// higher levels.
void vtkAMRDualGridHelperLevel::CreateBlockFaces(
  vtkAMRDualGridHelperBlock* block, int x, int y, int z)
{
  // avoid a warning.
  int temp = x + y + z + block->Level;
  if (temp < 1)
  {
    return;
  }

  /*
  vtkAMRDualGridHelperBlock* neighborBlock;
  if (block == 0)
    {
    return;
    }

  // The faces are for connectivity seeds between blocks.
  vtkAMRDualGridHelperFace* face;
  // -x Check for an exiting face in this level
  neighborBlock = this->GetGridBlock(x-1,y,z);
  if (neighborBlock && neighborBlock->Faces[1])
    {
    block->SetFace(0, neighborBlock->Faces[1]);
    }
  if (block->Faces[0] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 0);
    block->SetFace(0, face);
    }

  // +x Check to for an exiting face in this level
  neighborBlock = this->GetGridBlock(x+1,y,z);
  if (neighborBlock && neighborBlock->Faces[0])
    {
    block->SetFace(1, neighborBlock->Faces[0]);
    }
  if (block->Faces[1] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 1);
    block->SetFace(1, face);
    }

  // -y Check to for an exiting face in this level
  neighborBlock = this->GetGridBlock(x,y-1,z);
  if (neighborBlock && neighborBlock->Faces[3])
    {
    block->SetFace(2, neighborBlock->Faces[3]);
    }
  if (block->Faces[2] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 2);
    block->SetFace(2, face);
    }

  // +y Check to for an exiting face in this level
  neighborBlock = this->GetGridBlock(x,y+1,z);
  if (neighborBlock && neighborBlock->Faces[2])
    {
    block->SetFace(3, neighborBlock->Faces[2]);
    }
  if (block->Faces[3] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 3);
    block->SetFace(3, face);
    }

  // -z Check to for an exiting face in this level
  neighborBlock = this->GetGridBlock(x,y,z-1);
  if (neighborBlock && neighborBlock->Faces[5])
    {
    block->SetFace(4, neighborBlock->Faces[5]);
    }
  if (block->Faces[4] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 4);
    block->SetFace(4, face);
    }

  // +z Check to for an exiting face in this level
  neighborBlock = this->GetGridBlock(x,y,z+1);
  if (neighborBlock && neighborBlock->Faces[4])
    {
    block->SetFace(5, neighborBlock->Faces[4]);
    }
  if (block->Faces[5] == 0)
    { // create a new face.
    face = new vtkAMRDualGridHelperFace;
    face->InheritBlockValues(block, 5);
    block->SetFace(5, face);
    }
    */
}
//****************************************************************************
vtkAMRDualGridHelperBlock::vtkAMRDualGridHelperBlock()
{
  this->UserData = nullptr;

  // int ii;
  this->Level = 0;
  this->OriginIndex[0] = 0;
  this->OriginIndex[1] = 0;
  this->OriginIndex[2] = 0;

  this->GridIndex[0] = 0;
  this->GridIndex[1] = 0;
  this->GridIndex[2] = 0;

  this->ProcessId = vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();

  // for (ii = 0; ii < 6; ++ii)
  //  {
  //  this->Faces[ii] = 0;
  //  }
  this->Image = nullptr;
  this->CopyFlag = 0;

  this->ResetRegionBits();
}
//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock::~vtkAMRDualGridHelperBlock()
{
  if (this->UserData)
  {
    // It is not a vtkObject yet.
    // this->UserData->Delete();
    this->UserData = nullptr;
  }

  int ii;
  this->Level = 0;
  this->OriginIndex[0] = 0;
  this->OriginIndex[1] = 0;
  this->OriginIndex[2] = 0;

  // I broke down and made faces reference counted.
  for (ii = 0; ii < 6; ++ii)
  {
    // if (this->Faces[ii])
    //  {
    //  this->Faces[ii]->Unregister();
    //  this->Faces[ii] = 0;
    //  }
  }
  if (this->Image)
  {
    if (this->CopyFlag)
    { // We made a copy of the image and have to delete it.
      this->Image->Delete();
    }
    this->Image = nullptr;
  }
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperBlock::ResetRegionBits()
{

  for (int x = 0; x < 3; ++x)
  {
    for (int y = 0; y < 3; ++y)
    {
      for (int z = 0; z < 3; ++z)
      {
        // Default to own.
        this->RegionBits[x][y][z] = vtkAMRRegionBitOwner;
      }
    }
  }
  // It does not matter what the center is because we do not reference it.
  // I cannot hurt to set it consistently though.
  this->RegionBits[1][1][1] = vtkAMRRegionBitOwner;

  // Default to boundary.
  this->BoundaryBits = 63;
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelperAddBackGhostValues(
  vtkDataArray* inPtr, int inDim[3], vtkDataArray* outPtr, int outDim[3], int offset[3])
{
  int indexX, indexY, indexZ, outIndex;
  int xx, yy, zz;
  int inIncZ = inDim[0] * inDim[1];
  int inExt[6];
  int outExt[6];

  // out always has ghost.
  outExt[0] = outExt[2] = outExt[4] = -1;
  outExt[1] = outExt[0] + outDim[0] - 1;
  outExt[3] = outExt[2] + outDim[1] - 1;
  outExt[5] = outExt[4] + outDim[2] - 1;
  inExt[0] = -1 + offset[0];
  inExt[2] = -1 + offset[1];
  inExt[4] = -1 + offset[2];
  inExt[1] = inExt[0] + inDim[0] - 1;
  inExt[3] = inExt[2] + inDim[1] - 1;
  inExt[5] = inExt[4] + inDim[2] - 1;

  outIndex = 0;
  indexZ = 0;
  for (zz = outExt[4]; zz <= outExt[5]; ++zz)
  {
    indexY = indexZ;
    for (yy = outExt[2]; yy <= outExt[3]; ++yy)
    {
      indexX = indexY;
      for (xx = outExt[0]; xx <= outExt[1]; ++xx)
      {
        outPtr->SetTuple(outIndex++, inPtr->GetTuple(indexX));
        if (xx >= inExt[0] && xx < inExt[1])
        {
          ++indexX;
        }
      }
      if (yy >= inExt[2] && yy < inExt[3])
      {
        indexY += inDim[0];
      }
    }
    if (zz >= inExt[4] && zz < inExt[5])
    {
      indexZ += inIncZ;
    }
  }
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperBlock::AddBackGhostLevels(int standardBlockDimensions[3])
{
  int ii;
  int inDim[3];
  int outDim[3];
  if (this->Image == nullptr)
  {
    vtkGenericWarningMacro("Missing image.");
    return;
  }
  this->Image->GetDimensions(inDim);
  this->Image->GetDimensions(outDim);
  double origin[3];
  this->Image->GetOrigin(origin);
  double* spacing = this->Image->GetSpacing();

  // Note.  I as assume that origin index is the index of the first pixel
  // not the index of 0.

  int needToCopy = 0;
  int offset[3];
  int nCheck[3];
  int pCheck[3];
  for (ii = 0; ii < 3; ++ii)
  {
    // Conversion from point dims to cell dims.
    --inDim[ii];
    --outDim[ii];

    // Check negative axis.
    nCheck[ii] = this->OriginIndex[ii] % standardBlockDimensions[ii];
    // Check positive axis
    pCheck[ii] = (this->OriginIndex[ii] + inDim[ii]) % standardBlockDimensions[ii];
    offset[ii] = 0;
    if (nCheck[ii] == 0)
    {
      this->OriginIndex[ii] = this->OriginIndex[ii] - 1;
      origin[ii] = origin[ii] - spacing[ii];
      offset[ii] = 1;
      ++outDim[ii];
      needToCopy = 1;
    }
    if (pCheck[ii] == 0)
    {
      ++outDim[ii];
      needToCopy = 1;
    }
  }

  if (!needToCopy)
  {
    return;
  }

  vtkIdType newSize = (outDim[0] * outDim[1] * outDim[2]);

  vtkImageData* copy = vtkImageData::New();
  copy->SetDimensions(outDim[0] + 1, outDim[1] + 1, outDim[2] + 1);
  copy->SetSpacing(spacing);
  copy->SetOrigin(origin);
  // Copy only cell arrays.
  int numArrays = this->Image->GetCellData()->GetNumberOfArrays();
  for (int idx = 0; idx < numArrays; ++idx)
  {
    vtkDataArray* da = this->Image->GetCellData()->GetArray(idx);
    vtkDataArray* copyArray = vtkDataArray::SafeDownCast(da->CreateArray(da->GetDataType()));
    copyArray->SetNumberOfComponents(da->GetNumberOfComponents());
    copyArray->SetNumberOfTuples(newSize);
    copyArray->SetName(da->GetName());
    vtkAMRDualGridHelperAddBackGhostValues(da, inDim, copyArray, outDim, offset);
    copy->GetCellData()->AddArray(copyArray);
    copyArray->Delete();
  }

  this->Image = copy;
  this->CopyFlag = 1;
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperBlock::SetFace(int faceId, vtkAMRDualGridHelperFace* face)
{
  // Just in case.
  vtkAMRDualGridHelperFace* tmp = this->Faces[faceId];
  if (tmp)
  {
    --(tmp->UseCount);
    if (tmp->UseCount <= 0)
    {
      delete tmp;
    }
    this->Faces[faceId] = nullptr;
  }

  if (face)
  {
    ++(face->UseCount);
    this->Faces[faceId] = face;
  }
}

//****************************************************************************
vtkAMRDualGridHelperFace::vtkAMRDualGridHelperFace()
{
  this->Level = 0;
  this->NormalAxis = 0;
  this->OriginIndex[0] = 0;
  this->OriginIndex[1] = 0;
  this->OriginIndex[2] = 0;
  this->UseCount = 0;
}
//----------------------------------------------------------------------------
vtkAMRDualGridHelperFace::~vtkAMRDualGridHelperFace()
{
  this->Level = 0;
  this->NormalAxis = 0;
  this->OriginIndex[0] = 0;
  this->OriginIndex[1] = 0;
  this->OriginIndex[2] = 0;
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperFace::InheritBlockValues(vtkAMRDualGridHelperBlock* block, int faceIndex)
{
  // avoid warning.
  static_cast<void>(block);
  static_cast<void>(faceIndex);
  /* we are not worring about connectivity yet.
  int* ext = block->Image->GetExtent();
  this->Level = block->Level;
  this->OriginIndex[0] = block->OriginIndex[0];
  this->OriginIndex[1] = block->OriginIndex[1];
  this->OriginIndex[2] = block->OriginIndex[2];
  switch (faceIndex)
    {
    case 0:
      this->NormalAxis = 0;
      break;
    case 1:
      this->NormalAxis = 0;
      this->OriginIndex[0] += ext[1]-ext[0];
      break;
    case 2:
      this->NormalAxis = 1;
      break;
    case 3:
      this->NormalAxis = 1;
      ++this->OriginIndex[1] += ext[3]-ext[2];
      break;
    case 4:
      this->NormalAxis = 2;
      break;
    case 5:
      this->NormalAxis = 2;
      ++this->OriginIndex[2] += ext[5]-ext[4];
      break;
    }
  */
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperFace::Unregister()
{
  --this->UseCount;
  if (this->UseCount <= 0)
  {
    delete this;
  }
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelperFace::AddFragmentSeed(int level, int x, int y, int z, int fragmentId)
{
  // double pt[3];
  // This is a dual point so we need to shift it to the middle of a cell.
  //  pt[0] = DebuggingGlobalOrigin[0] + ((double)(x)+0.5) * DebuggingRootSpacing[0] /
  //  (double)(1<<level);
  //  pt[1] = DebuggingGlobalOrigin[1] + ((double)(y)+0.5) * DebuggingRootSpacing[1] /
  //  (double)(1<<level);
  //  pt[2] = DebuggingGlobalOrigin[2] + ((double)(z)+0.5) * DebuggingRootSpacing[2] /
  //  (double)(1<<level);
  //  vtkIdType ptIds[1];
  //  ptIds[0] = DebuggingPolyData->GetPoints()->InsertNextPoint(pt);
  //  DebuggingPolyData->GetVerts()->InsertNextCell(1, ptIds);
  //  DebuggingAttributes->InsertNextTuple1(ptIds[0]);

  // I expect that we will never add seeds from a different level.
  // Faces are always the lower level of the two blocks.
  // We process lower level blocks first.
  if (level != this->Level)
  {
    vtkGenericWarningMacro("Unexpected level.");
    return;
  }
  vtkAMRDualGridHelperSeed seed;
  seed.Index[0] = x;
  seed.Index[1] = y;
  seed.Index[2] = z;
  seed.FragmentId = fragmentId;

  this->FragmentIds.push_back(seed);
}
//****************************************************************************
vtkAMRDualGridHelper::vtkAMRDualGridHelper()
{
  int ii;

  this->SkipGhostCopy = 0;

  this->DataTypeSize = 8;
  this->ArrayName = nullptr;
  this->EnableDegenerateCells = 1;
  this->EnableAsynchronousCommunication = 1;
  this->NumberOfBlocksInThisProcess = 0;
  for (ii = 0; ii < 3; ++ii)
  {
    this->StandardBlockDimensions[ii] = 0;
    this->RootSpacing[ii] = 1.0;
    this->GlobalOrigin[ii] = 0.0;
  }

  this->Controller = vtkMultiProcessController::GetGlobalController();
  if (this->Controller)
  {
    this->Controller->Register(this);
  }
  else
  {
    this->Controller = vtkDummyController::New();
  }
}
//----------------------------------------------------------------------------
vtkAMRDualGridHelper::~vtkAMRDualGridHelper()
{
  int ii;
  int numberOfLevels = (int)(this->Levels.size());

  this->SetArrayName(nullptr);

  for (ii = 0; ii < numberOfLevels; ++ii)
  {
    delete this->Levels[ii];
    this->Levels[ii] = nullptr;
  }

  // Todo: See if we really need this.
  this->NumberOfBlocksInThisProcess = 0;

  this->DegenerateRegionQueue.clear();

  this->Controller->UnRegister(this);
  this->Controller = nullptr;
}
//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SkipGhostCopy: " << this->SkipGhostCopy << endl;
  os << indent << "EnableDegenerateCells: " << this->EnableDegenerateCells << endl;
  os << indent << "EnableAsynchronousCommunication: " << this->EnableAsynchronousCommunication
     << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
void vtkAMRDualGridHelper::SetController(vtkMultiProcessController* controller)
{
  if (this->Controller == controller)
    return;

  if (!controller)
  {
    // It is common to use nullptr for a multi process controller when no parallel
    // communication is needed (for example, in a serial program).  Rather than
    // have to constantly check for a nullptr pointer, use a dummy controller
    // so that we don't have to constantly check for it.
    if (!this->Controller->IsA("vtkDummyController"))
    {
      this->SetController(vtkSmartPointer<vtkDummyController>::New());
    }
    return;
  }

  // Controller is never nullptr.
  this->Controller->UnRegister(this);

  this->Controller = controller;
  controller->Register(this);

  this->Modified();
}

//----------------------------------------------------------------------------
int vtkAMRDualGridHelper::GetNumberOfBlocksInLevel(int level)
{
  if (level < 0 || level >= (int)(this->Levels.size()))
  {
    return 0;
  }
  return (int)(this->Levels[level]->Blocks.size());
}

//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRDualGridHelper::GetBlock(int level, int blockIdx)
{
  if (level < 0 || level >= (int)(this->Levels.size()))
  {
    return nullptr;
  }
  if ((int)(this->Levels[level]->Blocks.size()) <= blockIdx)
  {
    return nullptr;
  }
  return this->Levels[level]->Blocks[blockIdx];
}

//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRDualGridHelper::GetBlock(
  int level, int xGrid, int yGrid, int zGrid)
{
  if (level < 0 || level >= (int)(this->Levels.size()))
  {
    return nullptr;
  }
  return this->Levels[level]->GetGridBlock(xGrid, yGrid, zGrid);
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::AddBlock(int level, int id, vtkImageData* volume)
{
  // First compute the grid location of this block.
  double blockSize[3];
  blockSize[0] = (this->RootSpacing[0] * this->StandardBlockDimensions[0]) / (1 << level);
  blockSize[1] = (this->RootSpacing[1] * this->StandardBlockDimensions[1]) / (1 << level);
  blockSize[2] = (this->RootSpacing[2] * this->StandardBlockDimensions[2]) / (1 << level);
  double* bounds = volume->GetBounds();
  double center[3];

  center[0] = (bounds[0] + bounds[1]) * 0.5;
  center[1] = (bounds[2] + bounds[3]) * 0.5;
  center[2] = (bounds[4] + bounds[5]) * 0.5;
  int x = (int)((center[0] - this->GlobalOrigin[0]) / blockSize[0]);
  int y = (int)((center[1] - this->GlobalOrigin[1]) / blockSize[1]);
  int z = (int)((center[2] - this->GlobalOrigin[2]) / blockSize[2]);
  vtkAMRDualGridHelperBlock* block = this->Levels[level]->AddGridBlock(x, y, z, id, volume);

  // We need to set this ivar here because we need to compute the index
  // from the global origin and root spacing.  The issue is that some blocks
  // may not ghost levels.  Everything would be easier if the
  // vtk spy reader did not strip off ghost cells of the outer blocks.
  int* ext = volume->GetExtent();
  double* spacing = volume->GetSpacing();
  double origin[3];
  volume->GetOrigin(origin);
  // Move the origin to the first voxel.
  origin[0] += spacing[0] * (double)(ext[0]);
  origin[1] += spacing[1] * (double)(ext[2]);
  origin[2] += spacing[2] * (double)(ext[4]);
  // Now convert the origin into a level index.
  origin[0] -= this->GlobalOrigin[0];
  origin[1] -= this->GlobalOrigin[1];
  origin[2] -= this->GlobalOrigin[2];
  block->OriginIndex[0] = (int)(0.5 + origin[0] * (double)(1 << level) / (this->RootSpacing[0]));
  block->OriginIndex[1] = (int)(0.5 + origin[1] * (double)(1 << level) / (this->RootSpacing[1]));
  block->OriginIndex[2] = (int)(0.5 + origin[2] * (double)(1 << level) / (this->RootSpacing[2]));

  // This assumes 1 ghost layer (blocks are not completed yet so ....
  // block->OriginIndex[0] = this->StandardBlockDimensions[0] * x - 1;
  // block->OriginIndex[1] = this->StandardBlockDimensions[1] * y - 1;
  // block->OriginIndex[2] = this->StandardBlockDimensions[2] * z - 1;

  // Complete ghost levels if they have been stripped by the reader.
  block->AddBackGhostLevels(this->StandardBlockDimensions);
}

//----------------------------------------------------------------------------
vtkAMRDualGridHelperBlock* vtkAMRDualGridHelperLevel::AddGridBlock(
  int x, int y, int z, int id, vtkImageData* volume)
{
  // std::cerr << "Adding a grid block to level " << this->Level << " at " << x << " " << y << " "
  // << z << std::endl;
  // Expand the grid array if necessary.
  if (this->Grid == nullptr || x < this->GridExtent[0] || x > this->GridExtent[1] ||
    y < this->GridExtent[2] || y > this->GridExtent[3] || z < this->GridExtent[4] ||
    z > this->GridExtent[5])
  { // Reallocate
    int newExt[6];
    newExt[0] = (this->GridExtent[0] < x) ? this->GridExtent[0] : x;
    newExt[1] = (this->GridExtent[1] > x) ? this->GridExtent[1] : x;
    newExt[2] = (this->GridExtent[2] < y) ? this->GridExtent[2] : y;
    newExt[3] = (this->GridExtent[3] > y) ? this->GridExtent[3] : y;
    newExt[4] = (this->GridExtent[4] < z) ? this->GridExtent[4] : z;
    newExt[5] = (this->GridExtent[5] > z) ? this->GridExtent[5] : z;
    int yInc = newExt[1] - newExt[0] + 1;
    int zInc = (newExt[3] - newExt[2] + 1) * yInc;
    int newSize = zInc * (newExt[5] - newExt[4] + 1);
    vtkAMRDualGridHelperBlock** newGrid = new vtkAMRDualGridHelperBlock*[newSize];
    memset(newGrid, 0, newSize * sizeof(vtkAMRDualGridHelperBlock*));
    // Copy the blocks over to the new array.
    vtkAMRDualGridHelperBlock** ptr = this->Grid;
    for (int kk = this->GridExtent[4]; kk <= this->GridExtent[5]; ++kk)
    {
      for (int jj = this->GridExtent[2]; jj <= this->GridExtent[3]; ++jj)
      {
        for (int ii = this->GridExtent[0]; ii <= this->GridExtent[1]; ++ii)
        {
          newGrid[ii + (jj * yInc) + (kk * zInc)] = *ptr++;
        }
      }
    }
    memcpy(this->GridExtent, newExt, 6 * sizeof(int));
    this->GridIncY = yInc;
    this->GridIncZ = zInc;
    if (this->Grid)
    {
      delete[] this->Grid;
    }
    this->Grid = newGrid;
  }

  if (this->Grid[x + (y * this->GridIncY) + (z * this->GridIncZ)] == nullptr)
  {
    vtkAMRDualGridHelperBlock* newBlock = new vtkAMRDualGridHelperBlock();
    newBlock->Image = volume;
    newBlock->Level = this->Level;
    newBlock->BlockId = id;
    this->Grid[x + (y * this->GridIncY) + (z * this->GridIncZ)] = newBlock;
    this->Blocks.push_back(newBlock);
    newBlock->GridIndex[0] = x;
    newBlock->GridIndex[1] = y;
    newBlock->GridIndex[2] = z;
    return newBlock;
  }
  else
  {
    return this->Grid[x + (y * this->GridIncY) + (z * this->GridIncZ)];
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::CreateFaces()
{
  int* ext;
  int level, x, y, z;
  vtkAMRDualGridHelperBlock** blockPtr;
  // Start with the low levels.
  for (level = 0; level < this->GetNumberOfLevels(); ++level)
  {
    blockPtr = this->Levels[level]->Grid;
    ext = this->Levels[level]->GridExtent;
    for (z = ext[4]; z <= ext[5]; ++z)
    {
      for (y = ext[2]; y <= ext[3]; ++y)
      {
        for (x = ext[0]; x <= ext[1]; ++x)
        {
          // Look through all lower levels for existing faces.
          // Lower levels dominate.
          this->FindExistingFaces(*blockPtr, level, x, y, z);
          // Create faces that have not been used yet
          this->Levels[level]->CreateBlockFaces(*blockPtr, x, y, z);
          ++blockPtr;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::FindExistingFaces(
  vtkAMRDualGridHelperBlock* block, int level, int x, int y, int z)
{
  if (block == nullptr)
  {
    return;
  }

  vtkAMRDualGridHelperBlock* block2;
  int ii, jj, kk;
  int lowerLevel;
  int levelDifference;
  int ext[6];
  int ext2[6]; // Extent of grid in lower level.
  int ext3[6]; // Convert ext back to original level
  ext[0] = x;
  ext[1] = x + 1;
  ext[2] = y;
  ext[3] = y + 1;
  ext[4] = z;
  ext[5] = z + 1;

  // We only really need to check one level lower.
  // anything else is not allowed.
  // But what about edges and corners?
  // The degenerate cell trick should work for any level difference.
  //.(But our logic assumes 1 level difference.)
  // We will have to record the level of degeneracy.
  // Just one level for now.
  for (lowerLevel = 0; lowerLevel < level; ++lowerLevel)
  {
    levelDifference = level - lowerLevel;
    for (ii = 0; ii < 6; ++ii)
    {
      ext2[ii] = ext[ii] >> levelDifference;
      ext3[ii] = ext2[ii] << levelDifference;
    }
    // If we convert index to lower level and then back and it does
    // not change then, the different level blocks share a face.
    for (kk = -1; kk <= 1; ++kk)
    {
      for (jj = -1; jj <= 1; ++jj)
      {
        for (ii = -1; ii <= 1; ++ii)
        {
          // Somewhat convoluted logic to determine if face/edge/corner is external.
          if ((ii != -1 || ext3[0] == ext[0]) && (ii != 1 || ext3[1] == ext[1]) &&
            (jj != -1 || ext3[2] == ext[2]) && (jj != 1 || ext3[3] == ext[3]) &&
            (kk != -1 || ext3[4] == ext[4]) && (kk != 1 || ext3[5] == ext[5]))
          { // This face/edge/corner is external and may have a neighbor in the lower resolution.
            // Special handling for face structures.
            // Face structures are used for seeding connectivity between blocks.
            // Note that ext2[0] is now equal to ext2[1] (an the same for the other axes too).
            block2 = this->Levels[lowerLevel]->GetGridBlock(ext2[0], ext2[2], ext2[4]);
            if (block2)
            {
              if (ii == -1 && jj == 0 && kk == 0)
              {
                block->SetFace(0, block2->Faces[1]);
              }
              else if (ii == 1 && jj == 0 && kk == 0)
              {
                block->SetFace(1, block2->Faces[0]);
              }
              else if (jj == -1 && ii == 0 && kk == 0)
              {
                block->SetFace(2, block2->Faces[3]);
              }
              else if (jj == 1 && ii == 0 && kk == 0)
              {
                block->SetFace(3, block2->Faces[2]);
              }
              else if (kk == -1 && ii == 0 && jj == 0)
              {
                block->SetFace(4, block2->Faces[5]);
              }
              else if (kk == 1 && ii == 0 && jj == 0)
              {
                block->SetFace(5, block2->Faces[4]);
              }
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// Negotiate which blocks will be responsible for generating which shared
// regions.  Higher levels dominate lower levels.  We also set the
// neighbor bits which indicate which cells/points become degenerate.
void vtkAMRDualGridHelper::AssignSharedRegions()
{
  vtkTimerLogSmartMarkEvent markevent("AssignSharedRegions", this->Controller);

  int* ext;
  int level, x, y, z;
  vtkAMRDualGridHelperBlock** blockPtr;

  // Start with the highest levels and work down.
  for (level = this->GetNumberOfLevels() - 1; level >= 0; --level)
  {
    blockPtr = this->Levels[level]->Grid;
    ext = this->Levels[level]->GridExtent;
    // Loop through all blocks in the grid.
    // If blocks remembered their grid location, this would be easier.
    // Blocks now remember there grid xyz locations but it is good that we
    // loop over the grid.  I assume that every process visits
    // to blocks in the same order.  The 1-d block array may
    // not have the same order on all processes, but this
    // grid traversal does.
    for (z = ext[4]; z <= ext[5]; ++z)
    {
      for (y = ext[2]; y <= ext[3]; ++y)
      {
        for (x = ext[0]; x <= ext[1]; ++x)
        {
          if (*blockPtr)
          {
            this->AssignBlockSharedRegions(*blockPtr, level, x, y, z);
          }
          ++blockPtr;
        }
      }
    }
  }
}
void vtkAMRDualGridHelper::AssignBlockSharedRegions(
  vtkAMRDualGridHelperBlock* block, int blockLevel, int blockX, int blockY, int blockZ)
{
  int degeneracyLevel;
  // Loop though all the regions.
  int rx, ry, rz;
  for (rz = -1; rz <= 1; ++rz)
  {
    for (ry = -1; ry <= 1; ++ry)
    {
      for (rx = -1; rx <= 1; ++rx)
      {
        if ((rx || ry || rz) && (block->RegionBits[rx + 1][ry + 1][rz + 1] & vtkAMRRegionBitOwner))
        { // A face/edge/corner region and it has not been taken yet.
          degeneracyLevel = this->ClaimBlockSharedRegion(block, blockX, blockY, blockZ, rx, ry, rz);
          // I am using the first 7 bits to store the degenacy level difference.
          // The degenerate flag is now a mask.
          if (this->EnableDegenerateCells && degeneracyLevel < blockLevel)
          {
            unsigned char levelDiff = (unsigned char)(blockLevel - degeneracyLevel);
            if ((vtkAMRRegionBitsDegenerateMask & levelDiff) != levelDiff)
            { // Extreme level difference.
              vtkGenericWarningMacro("Could not encode level difference.");
            }
            block->RegionBits[rx + 1][ry + 1][rz + 1] =
              vtkAMRRegionBitOwner + (vtkAMRRegionBitsDegenerateMask & levelDiff);
          }
        }
      }
    }
  }
}
// Returns the grid level that the points in this region should be
// projected to.  This will cause these cells to become degenerate
// (Pyramids wedges ...) and nicely transition between levels.
int vtkAMRDualGridHelper::ClaimBlockSharedRegion(vtkAMRDualGridHelperBlock* block, int blockX,
  int blockY, int blockZ, int regionX, int regionY, int regionZ)
{
  vtkAMRDualGridHelperBlock* neighborBlock;
  vtkAMRDualGridHelperBlock* bestBlock;
  int tx, ty, tz;
  int dist, bestDist;
  int bestLevel;
  int blockLevel = block->Level;
  int startX, startY, startZ, endX, endY, endZ;
  int ix, iy, iz;
  int lowerLevel, levelDifference;
  int lowerX, lowerY, lowerZ;
  int ii;
  int ext1[6]; // Point extent of the single block
  int ext2[6]; // Point extent of block in lower level.
  int ext3[6]; // Extent2 converted back to original level.

  ext1[0] = blockX;
  ext1[1] = blockX + 1;
  ext1[2] = blockY;
  ext1[3] = blockY + 1;
  ext1[4] = blockZ;
  ext1[5] = blockZ + 1;

  // This middle of the block is this far from the region.
  // Sort of city block distance.  All region indexes are in [-1,1]
  // the multiplications is effectively computing the absolute value.
  bestDist = regionX * regionX + regionY * regionY + regionZ * regionZ;
  bestLevel = blockLevel;
  bestBlock = block;

  // Loop through all levels (except higher levels) marking
  // this regions as taken.  Higher levels have already claimed
  // their regions so it would be useless to check them.
  for (lowerLevel = blockLevel; lowerLevel >= 0; --lowerLevel)
  {
    levelDifference = blockLevel - lowerLevel;
    for (ii = 0; ii < 6; ++ii)
    {
      ext2[ii] = ext1[ii] >> levelDifference;
      ext3[ii] = ext2[ii] << levelDifference;
    }
    // If we convert index to lower level and then back and it does
    // not change then, the different level blocks share a face.
    // Somewhat convoluted logic to determine if face/edge/corner is external.
    if ((regionX == -1 && ext3[0] == ext1[0]) || (regionX == 1 && ext3[1] == ext1[1]) ||
      (regionY == -1 && ext3[2] == ext1[2]) || (regionY == 1 && ext3[3] == ext1[3]) ||
      (regionZ == -1 && ext3[4] == ext1[4]) || (regionZ == 1 && ext3[5] == ext1[5]))
    { // This face/edge/corner is on a grid boundary and may have a neighbor in this level.
      // Loop over the blocks that share this region. Faces have 2, edges 4 and corner 8.
      // This was a real pain.  I could not have a loop that would increment
      // up or down (depending on sign of regionX,regionY,regionZ).
      // Sort start and end to have loop always increment up.
      startX = startY = startZ = 0;
      endX = regionX;
      endY = regionY;
      endZ = regionZ;
      if (regionX < 0)
      {
        startX = regionX;
        endX = 0;
      }
      if (regionY < 0)
      {
        startY = regionY;
        endY = 0;
      }
      if (regionZ < 0)
      {
        startZ = regionZ;
        endZ = 0;
      }
      for (iz = startZ; iz <= endZ; ++iz)
      {
        for (iy = startY; iy <= endY; ++iy)
        {
          for (ix = startX; ix <= endX; ++ix)
          {
            // Skip the middle (non neighbor).
            if (ix || iy || iz)
            {
              lowerX = (blockX + ix) >> levelDifference;
              lowerY = (blockY + iy) >> levelDifference;
              lowerZ = (blockZ + iz) >> levelDifference;
              neighborBlock = this->Levels[lowerLevel]->GetGridBlock(lowerX, lowerY, lowerZ);
              // Problem. For internal edge ghost. Lower level is direction -1
              // So computation of distance is not correct.
              if (neighborBlock)
              {
                // Mark this face of the block as non boundary.
                if (ix == -1 && iy == 0 && iz == 0)
                { // Turn off the -x boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 62;
                  // Turn off neighbor boundary bit.  It is not necessary because the
                  // neighbor does not own the region and will not process it.
                  // However, it is confusing when debugging not to have the correct bits set.
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 61;
                }
                if (ix == 1 && iy == 0 && iz == 0)
                { // Turn off the x boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 61;
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 62;
                }
                if (ix == 0 && iy == -1 && iz == 0)
                { // Turn off the -y boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 59;
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 55;
                }
                if (ix == 0 && iy == 1 && iz == 0)
                { // Turn off the y boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 55;
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 59;
                }
                if (ix == 0 && iy == 0 && iz == -1)
                { // Turn off the -z boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 47;
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 31;
                }
                if (ix == 0 && iy == 0 && iz == 1)
                { // Turn off the z boundary bit.
                  block->BoundaryBits = block->BoundaryBits & 31;
                  neighborBlock->BoundaryBits = neighborBlock->BoundaryBits & 47;
                }

                // Vote for degeneracy level.
                if (this->EnableDegenerateCells)
                {
                  // Now remove the neighbors owner bit for this region.
                  // How do we find the region in the neighbor?
                  // Remove assignment for this region from neighbor
                  neighborBlock->RegionBits[regionX - ix - ix + 1][regionY - iy - iy + 1]
                                           [regionZ - iz - iz + 1] = 0;
                  tx = regionX - ix;
                  ty = regionY - iy;
                  tz = regionZ - iz; // all should be in [-1,1]
                  dist = tx * tx + ty * ty + tz * tz;
                  if (dist < bestDist)
                  {
                    bestLevel = lowerLevel;
                    bestDist = dist;
                    bestBlock = neighborBlock;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // If the region is degenerate and points have to be moved
  // to a lower level grid, tehn we have to copy the
  // volume fractions from the lower level grid too.
  if (this->EnableDegenerateCells && bestLevel < blockLevel)
  {
    if (block->Image == nullptr || bestBlock->Image == nullptr)
    { // Deal with remote blocks later.
      // Add the pair of blocks to a queue to copy when we get the data.
      vtkDataArray* bestBlockArray = nullptr;
      vtkDataArray* blockArray = nullptr;
      if (block->Image)
      {
        blockArray = block->Image->GetCellData()->GetArray(this->ArrayName);
      }
      if (bestBlock->Image)
      {
        bestBlockArray = bestBlock->Image->GetCellData()->GetArray(this->ArrayName);
      }
      // We will skip the transfer when blocks are in the same level because
      // the volume fraction ghost values will be correct.
      // I used to have this check inside, but we do need to copy
      // the level diffs when the neighbors are in the same level.
      // The level diffs also use this communication channel.
      // Level diffs cause the internal simplification.
      if (bestBlock->Level != block->Level)
      {
        this->QueueRegionRemoteCopy(
          regionX, regionY, regionZ, bestBlock, bestBlockArray, block, blockArray);
      }
    }
    else
    {
      if (block->CopyFlag == 0)
      { // We cannot modify our input.
        vtkImageData* copy = vtkImageData::New();
        // We only really need to deep copy the one volume fraction array.
        // All others can be shallow copied.
        copy->DeepCopy(block->Image);
        block->Image = copy;
        block->CopyFlag = 1;
        // std::cerr << "Claim block shared region results in a copy flag " << block->Image << " "
        // << bestBlock->Image  << "\n";
      }
      vtkDataArray* blockDataArray = block->Image->GetCellData()->GetArray(this->ArrayName);
      vtkDataArray* bestBlockDataArray = bestBlock->Image->GetCellData()->GetArray(this->ArrayName);
      if (blockDataArray && bestBlockDataArray)
      {
        this->CopyDegenerateRegionBlockToBlock(
          regionX, regionY, regionZ, bestBlock, bestBlockDataArray, block, blockDataArray);
      }
    }
  }

  return bestLevel;
}
void vtkAMRDualGridHelper::QueueRegionRemoteCopy(int regionX, int regionY, int regionZ,
  vtkAMRDualGridHelperBlock* lowResBlock, vtkDataArray* lowResArray,
  vtkAMRDualGridHelperBlock* highResBlock, vtkDataArray* highResArray)
{
  vtkAMRDualGridHelperDegenerateRegion dreg;
  dreg.ReceivingRegion[0] = regionX;
  dreg.ReceivingRegion[1] = regionY;
  dreg.ReceivingRegion[2] = regionZ;
  dreg.ReceivingBlock = highResBlock;
  dreg.ReceivingArray = highResArray;
  dreg.SourceBlock = lowResBlock;
  dreg.SourceArray = lowResArray;
  if (!this->SkipGhostCopy)
  {
    this->DegenerateRegionQueue.push_back(dreg);
  }
}

// Just a hack to test an assumption.
// This can be removed once we determine how the ghost values behave across
// level changes.
static int vtkDualGridHelperCheckAssumption = 0;
static int vtkDualGridHelperSkipGhostCopy = 0;

// Given source and destination process ids, returns the buffer size, in bytes,
// required to send the approprate degenerate cell information.  If 0 is
// returned, it is not necessary to transfer any information, which is common.
void vtkAMRDualGridHelper::DegenerateRegionMessageSize(
  vtkIdTypeArray* srcProcs, vtkIdTypeArray* destProcs)
{
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myProc = this->Controller->GetLocalProcessId();
  if (srcProcs->GetNumberOfTuples() != numProcs || destProcs->GetNumberOfTuples() != numProcs)
  {
    vtkErrorMacro("Internal error:"
                  " DegenerateRegionMessageSize called without proper array parameters");
    return;
  }

  // Each region is actually either 1/4 of a face, 1/2 of and edge or a corner.

  // Note: In order to minimize communication, I am rellying heavily on the fact
  // that the queue will be the same on all processes.  Message/region lengths
  // are computed implicitly.

  std::vector<vtkAMRDualGridHelperDegenerateRegion>::iterator region;
  for (region = this->DegenerateRegionQueue.begin(); region != this->DegenerateRegionQueue.end();
       region++)
  {
    vtkIdType messageLength = 0;
    if ((region->SourceBlock->ProcessId == myProc) || (region->ReceivingBlock->ProcessId == myProc))
    {
      messageLength = 11 * sizeof(int);
      vtkIdType regionSize = 1;
      if (region->ReceivingRegion[0] == 0)
      {
        // Note:  In rare cases, level difference can be larger than 1.
        // This will reserve too much memory with no real harm done.
        // Half the root dimensions, ghost layers not included.
        // Ghost layers are handled by separate edge and corner regions.
        regionSize *= (this->StandardBlockDimensions[0] >> 1);
      }
      if (region->ReceivingRegion[1] == 0)
      {
        regionSize *= (this->StandardBlockDimensions[1] >> 1);
      }
      if (region->ReceivingRegion[2] == 0)
      {
        regionSize *= (this->StandardBlockDimensions[2] >> 1);
      }
      messageLength += regionSize * this->DataTypeSize;
    }
    if (region->SourceBlock->ProcessId == myProc)
    {
      vtkIdType len = destProcs->GetValue(region->ReceivingBlock->ProcessId);
      len += messageLength;
      destProcs->SetValue(region->ReceivingBlock->ProcessId, len);
    }
    if (region->ReceivingBlock->ProcessId == myProc)
    {
      vtkIdType len = srcProcs->GetValue(region->SourceBlock->ProcessId);
      len += messageLength;
      srcProcs->SetValue(region->SourceBlock->ProcessId, len);
    }
  }
  // put the final 0,0,0 to mark the end of message
  // but if messageLength is 0 send that so we know not to send anything.
  for (int p = 0; p < numProcs; p++)
  {
    vtkIdType len = srcProcs->GetValue(p);
    if (len > 0)
    {
      len += 3 * sizeof(int);
    }
    srcProcs->SetValue(p, len);
    len = destProcs->GetValue(p);
    if (len > 0)
    {
      len += 3 * sizeof(int);
    }
    destProcs->SetValue(p, len);
  }
}

// The following three methods are all similar and should be reworked so that
// the share more code.  One possibility to to have block to block copy
// always go through an intermediate buffer (as if is were remote).
// THis should not add much overhead to the copy.

void vtkDualGridHelperCopyBlockToBlock(vtkDataArray* ptr, vtkDataArray* lowerPtr, int ext[6],
  int levelDiff, int yInc, int zInc, int highResBlockOriginIndex[3], int lowResBlockOriginIndex[3])
{
  double val;
  int xIndex, yIndex, zIndex;
  zIndex = ext[0] + yInc * ext[2] + zInc * ext[4];
  int lx, ly, lz; // x,y,z converted to lower grid indexes.
  for (int z = ext[4]; z <= ext[5]; ++z)
  {
    lz = ((z + highResBlockOriginIndex[2]) >> levelDiff) - lowResBlockOriginIndex[2];
    yIndex = zIndex;
    for (int y = ext[2]; y <= ext[3]; ++y)
    {
      ly = ((y + highResBlockOriginIndex[1]) >> levelDiff) - lowResBlockOriginIndex[1];
      xIndex = yIndex;
      for (int x = ext[0]; x <= ext[1]; ++x)
      {
        lx = ((x + highResBlockOriginIndex[0]) >> levelDiff) - lowResBlockOriginIndex[0];
        val = lowerPtr->GetTuple1(lx + ly * yInc + lz * zInc);
        // Lets see if our assumption about ghost values is correct.
        if (vtkDualGridHelperCheckAssumption && vtkDualGridHelperSkipGhostCopy &&
          ptr->GetTuple1(xIndex) != val)
        {
          // Sandia did get this message so I will default to have ghost copy on.
          //  I did not document the assumption well enough.
          vtkGenericWarningMacro("Ghost assumption incorrect.  Seams may result.");
          // Report issue once per execution.
          vtkDualGridHelperCheckAssumption = 0;
        }
        ptr->SetTuple1(xIndex, val);
        xIndex++;
      }
      yIndex += yInc;
    }
    zIndex += zInc;
  }
}
// Ghost volume fraction values are not consistent across levels.
// We need the degenerate high-res volume fractions
// to match corresponding values in low res-blocks.
// This method copies low-res values to high-res ghost blocks.
void vtkAMRDualGridHelper::CopyDegenerateRegionBlockToBlock(int regionX, int regionY, int regionZ,
  vtkAMRDualGridHelperBlock* lowResBlock, vtkDataArray* lowResArray,
  vtkAMRDualGridHelperBlock* highResBlock, vtkDataArray* highResArray)
{
  int levelDiff = highResBlock->Level - lowResBlock->Level;
  if (levelDiff == 0)
  { // double check.
    return;
  }
  if (levelDiff < 0)
  { // We added the levels in the wrong order.
    vtkGenericWarningMacro("Reverse level change.");
    return;
  }

  // Now copy low resolution into highresolution ghost layer.
  // For simplicity loop over all three axes (one will be degenerate).
  int daType = highResArray->GetDataType();

  // Lower block pointer
  if (lowResArray->GetDataType() != daType)
  {
    vtkGenericWarningMacro("Type mismatch.");
    return;
  }

  // Get the extent of the high-res region we are replacing with values from the neighbor.
  int ext[6];
  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->StandardBlockDimensions[0] + 1; // Add ghost layers back in.
  ext[3] = this->StandardBlockDimensions[1] + 1; // Add ghost layers back in.
  ext[5] = this->StandardBlockDimensions[2] + 1; // Add ghost layers back in.

  // Test an assumption used in this method.
  if (ext[0] != 0 || ext[2] != 0 || ext[4] != 0)
  {
    vtkGenericWarningMacro("Expecting min extent to be 0.");
    return;
  }
  int yInc = ext[1] - ext[0] + 1;
  int zInc = yInc * (ext[5] - ext[4] + 1);

  switch (regionX)
  {
    case -1:
      ext[1] = ext[0];
      break;
    case 0:
      ++ext[0];
      --ext[1];
      break;
    case 1:
      ext[0] = ext[1];
      break;
  }
  switch (regionY)
  {
    case -1:
      ext[3] = ext[2];
      break;
    case 0:
      ++ext[2];
      --ext[3];
      break;
    case 1:
      ext[2] = ext[3];
      break;
  }
  switch (regionZ)
  {
    case -1:
      ext[5] = ext[4];
      break;
    case 0:
      ++ext[4];
      --ext[5];
      break;
    case 1:
      ext[4] = ext[5];
      break;
  }

  vtkDualGridHelperSkipGhostCopy = this->SkipGhostCopy;
  // Assume all blocks have the same extent.
  vtkDualGridHelperCopyBlockToBlock(highResArray, lowResArray, ext, levelDiff, yInc, zInc,
    highResBlock->OriginIndex, lowResBlock->OriginIndex);
}
// Ghost volume fraction values are not consistent across levels.
// We need the degenerate high-res volume fractions
// to match corresponding values in low res-blocks.
// This method copies low-res values to high-res ghost blocks.
template <class T>
void* vtkDualGridHelperCopyBlockToMessage(
  T* messagePtr, vtkDataArray* lowerPtr, int ext[6], int yInc, int zInc)
{
  // Loop over regions values (cells/dual points) and
  // copy into message.
  for (int z = ext[4]; z <= ext[5]; ++z)
  {
    for (int y = ext[2]; y <= ext[3]; ++y)
    {
      for (int x = ext[0]; x <= ext[1]; ++x)
      {
        *messagePtr++ = static_cast<T>(lowerPtr->GetTuple1(x + y * yInc + z * zInc));
      }
    }
  }
  return messagePtr;
}
void* vtkAMRDualGridHelper::CopyDegenerateRegionBlockToMessage(
  const vtkAMRDualGridHelperDegenerateRegion& region, void* messagePtr)
{
  int regionX = region.ReceivingRegion[0];
  int regionY = region.ReceivingRegion[1];
  int regionZ = region.ReceivingRegion[2];
  vtkAMRDualGridHelperBlock* lowResBlock = region.SourceBlock;
  vtkAMRDualGridHelperBlock* highResBlock = region.ReceivingBlock;

  int levelDiff = highResBlock->Level - lowResBlock->Level;
  if (levelDiff < 0)
  { // We added the levels in the wrong order.
    vtkGenericWarningMacro("Reverse level change.");
    return messagePtr;
  }
  // Lower block pointer
  if (region.SourceArray == nullptr)
  {
    return messagePtr;
  }
  int daType = region.SourceArray->GetDataType();

  // Get the extent of the high-res region we are replacing with values from the neighbor.
  int ext[6];
  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->StandardBlockDimensions[0] + 1; // Add ghost layers back in.
  ext[3] = this->StandardBlockDimensions[1] + 1; // Add ghost layers back in.
  ext[5] = this->StandardBlockDimensions[2] + 1; // Add ghost layers back in.
  int yInc = ext[1] - ext[0] + 1;
  int zInc = yInc * (ext[5] - ext[4] + 1);

  switch (regionX)
  {
    case -1:
      ext[1] = ext[0];
      break;
    case 0:
      ++ext[0];
      --ext[1];
      break;
    case 1:
      ext[0] = ext[1];
      break;
  }
  switch (regionY)
  {
    case -1:
      ext[3] = ext[2];
      break;
    case 0:
      ++ext[2];
      --ext[3];
      break;
    case 1:
      ext[2] = ext[3];
      break;
  }
  switch (regionZ)
  {
    case -1:
      ext[5] = ext[4];
      break;
    case 0:
      ++ext[4];
      --ext[5];
      break;
    case 1:
      ext[4] = ext[5];
      break;
  }

  // Convert to the extent of the low resolution source block.
  ext[0] = ((ext[0] + highResBlock->OriginIndex[0]) >> levelDiff) - lowResBlock->OriginIndex[0];
  ext[1] = ((ext[1] + highResBlock->OriginIndex[0]) >> levelDiff) - lowResBlock->OriginIndex[0];
  ext[2] = ((ext[2] + highResBlock->OriginIndex[1]) >> levelDiff) - lowResBlock->OriginIndex[1];
  ext[3] = ((ext[3] + highResBlock->OriginIndex[1]) >> levelDiff) - lowResBlock->OriginIndex[1];
  ext[4] = ((ext[4] + highResBlock->OriginIndex[2]) >> levelDiff) - lowResBlock->OriginIndex[2];
  ext[5] = ((ext[5] + highResBlock->OriginIndex[2]) >> levelDiff) - lowResBlock->OriginIndex[2];

  int* gridPtr = static_cast<int*>(messagePtr);

  *gridPtr++ = regionX;
  *gridPtr++ = regionY;
  *gridPtr++ = regionZ;

  *gridPtr++ = lowResBlock->Level;
  *gridPtr++ = lowResBlock->GridIndex[0];
  *gridPtr++ = lowResBlock->GridIndex[1];
  *gridPtr++ = lowResBlock->GridIndex[2];

  *gridPtr++ = highResBlock->Level;
  *gridPtr++ = highResBlock->GridIndex[0];
  *gridPtr++ = highResBlock->GridIndex[1];
  *gridPtr++ = highResBlock->GridIndex[2];

  messagePtr = gridPtr;

  // Assume all blocks have the same extent.
  switch (daType)
  {
    vtkTemplateMacro(messagePtr = vtkDualGridHelperCopyBlockToMessage(
                       static_cast<VTK_TT*>(messagePtr), region.SourceArray, ext, yInc, zInc));
    default:
      vtkGenericWarningMacro("Execute: Unknown ScalarType");
      return messagePtr;
  }

  return messagePtr;
}

// Take the low res message and copy to the high res block.
template <class T>
const void* vtkDualGridHelperCopyMessageToBlock(vtkDataArray* ptr, const T* messagePtr, int ext[6],
  int messageExt[6], int levelDiff, int yInc, int zInc, int highResBlockOriginIndex[3],
  int lowResBlockOriginIndex[3], bool hackLevelFlag)
{
  int messageIncY = messageExt[1] - messageExt[0] + 1;
  int messageIncZ = messageIncY * (messageExt[3] - messageExt[2] + 1);
  // Loop over regions values (cells/dual points).
  int xIndex, yIndex, zIndex;
  zIndex = ext[0] + yInc * ext[2] + zInc * ext[4];
  int lx, ly, lz; // x,y,z converted to lower grid indexes.
  for (int z = ext[4]; z <= ext[5]; ++z)
  {
    lz =
      ((z + highResBlockOriginIndex[2]) >> levelDiff) - lowResBlockOriginIndex[2] - messageExt[4];
    yIndex = zIndex;
    for (int y = ext[2]; y <= ext[3]; ++y)
    {
      ly =
        ((y + highResBlockOriginIndex[1]) >> levelDiff) - lowResBlockOriginIndex[1] - messageExt[2];
      xIndex = yIndex;
      for (int x = ext[0]; x <= ext[1]; ++x)
      {
        lx = ((x + highResBlockOriginIndex[0]) >> levelDiff) - lowResBlockOriginIndex[0] -
          messageExt[0];
        if (hackLevelFlag)
        { // When generalizing I forgot that levels get an extra increment here.
          // Maybe it would be better if the level diff was relative to the block level!!!!  Oh
          // well.
          // It is too late to make that change. Just pass in a special case.
          ptr->SetTuple1(xIndex, messagePtr[lx + ly * messageIncY + lz * messageIncZ] + levelDiff);
        }
        else
        {
          ptr->SetTuple1(xIndex, messagePtr[lx + ly * messageIncY + lz * messageIncZ]);
        }
        xIndex++;
      }
      yIndex += yInc;
    }
    zIndex += zInc;
  }
  return messagePtr + (messageIncZ * (messageExt[5] - messageExt[4] + 1));
}

const void* vtkAMRDualGridHelper::CopyDegenerateRegionMessageToBlock(
  const vtkAMRDualGridHelperDegenerateRegion& region, const void* messagePtr,
  bool hackLevelFlag) // Make levels absolute so we can get rid of this flag.
{
  int regionX = region.ReceivingRegion[0];
  int regionY = region.ReceivingRegion[1];
  int regionZ = region.ReceivingRegion[2];
  vtkAMRDualGridHelperBlock* lowResBlock = region.SourceBlock;
  vtkAMRDualGridHelperBlock* highResBlock = region.ReceivingBlock;

  int levelDiff = highResBlock->Level - lowResBlock->Level;
  if (levelDiff < 0)
  { // We added the levels in the wrong order.
    vtkGenericWarningMacro("Reverse level change.");
    return messagePtr;
  }

  // Now copy low resolution into highresolution ghost layer.
  // For simplicity loop over all three axes (one will be degenerate).
  if (region.ReceivingArray == nullptr)
  {
    return messagePtr;
  }
  int daType = region.ReceivingArray->GetDataType();

  // Get the extent of the high-res region we are replacing with values from the neighbor.
  int ext[6];
  ext[0] = ext[2] = ext[4] = 0;
  ext[1] = this->StandardBlockDimensions[0] + 1; // Add ghost layers back in.
  ext[3] = this->StandardBlockDimensions[1] + 1; // Add ghost layers back in.
  ext[5] = this->StandardBlockDimensions[2] + 1; // Add ghost layers back in.

  // Test an assumption used in this method.
  if (ext[0] != 0 || ext[2] != 0 || ext[4] != 0)
  {
    vtkGenericWarningMacro("Expecting min extent to be 0.");
    return messagePtr;
  }
  int yInc = ext[1] - ext[0] + 1;
  int zInc = yInc * (ext[5] - ext[4] + 1);

  switch (regionX)
  {
    case -1:
      ext[1] = ext[0];
      break;
    case 0:
      ++ext[0];
      --ext[1];
      break;
    case 1:
      ext[0] = ext[1];
      break;
  }
  switch (regionY)
  {
    case -1:
      ext[3] = ext[2];
      break;
    case 0:
      ++ext[2];
      --ext[3];
      break;
    case 1:
      ext[2] = ext[3];
      break;
  }
  switch (regionZ)
  {
    case -1:
      ext[5] = ext[4];
      break;
    case 0:
      ++ext[4];
      --ext[5];
      break;
    case 1:
      ext[4] = ext[5];
      break;
  }

  // Convert to the extent of the low resolution source block.
  int messageExt[6];
  messageExt[0] =
    ((ext[0] + highResBlock->OriginIndex[0]) >> levelDiff) - lowResBlock->OriginIndex[0];
  messageExt[1] =
    ((ext[1] + highResBlock->OriginIndex[0]) >> levelDiff) - lowResBlock->OriginIndex[0];
  messageExt[2] =
    ((ext[2] + highResBlock->OriginIndex[1]) >> levelDiff) - lowResBlock->OriginIndex[1];
  messageExt[3] =
    ((ext[3] + highResBlock->OriginIndex[1]) >> levelDiff) - lowResBlock->OriginIndex[1];
  messageExt[4] =
    ((ext[4] + highResBlock->OriginIndex[2]) >> levelDiff) - lowResBlock->OriginIndex[2];
  messageExt[5] =
    ((ext[5] + highResBlock->OriginIndex[2]) >> levelDiff) - lowResBlock->OriginIndex[2];

  switch (daType)
  {
    vtkTemplateMacro(messagePtr = vtkDualGridHelperCopyMessageToBlock(region.ReceivingArray,
                       static_cast<const VTK_TT*>(messagePtr), ext, messageExt, levelDiff, yInc,
                       zInc, highResBlock->OriginIndex, lowResBlock->OriginIndex, hackLevelFlag));
    default:
      vtkGenericWarningMacro("Execute: Unknown ScalarType");
      return messagePtr;
  }
  return messagePtr;
}

// Given a buffer (of size determined by DegenerateRegionMessageSize), fill it
// with degenerate region information to be sent to the given process.
void vtkAMRDualGridHelper::MarshalDegenerateRegionMessage(void* messagePtr, int destProc)
{
  int myProcId = this->Controller->GetLocalProcessId();

  std::vector<vtkAMRDualGridHelperDegenerateRegion>::iterator region;
  for (region = this->DegenerateRegionQueue.begin(); region != this->DegenerateRegionQueue.end();
       region++)
  {
    if ((region->ReceivingBlock->ProcessId == destProc) &&
      (region->SourceBlock->ProcessId == myProcId))
    {
      messagePtr = this->CopyDegenerateRegionBlockToMessage(*region, messagePtr);
    }
  }
  int* gridPtr = static_cast<int*>(messagePtr);
  *gridPtr++ = 2;
  *gridPtr++ = 2;
  *gridPtr++ = 2;
}

// Given a buffer (of size determined by DegenerateRegionMessageSize) filled
// with degenerate region information received from the given processes, copy
// the information to the local block data structures.
void vtkAMRDualGridHelper::UnmarshalDegenerateRegionMessage(const void* messagePtr,
  int vtkNotUsed(messageLength), int vtkNotUsed(srcProc), bool hackLevelFlag)
{
  while (1)
  {
    const int* gridPtr = static_cast<const int*>(messagePtr);
    int level;
    int gridIndex[3];

    vtkAMRDualGridHelperDegenerateRegion region;
    region.ReceivingRegion[0] = *gridPtr++;
    region.ReceivingRegion[1] = *gridPtr++;
    region.ReceivingRegion[2] = *gridPtr++;

    if (region.ReceivingRegion[0] == 2 && region.ReceivingRegion[1] == 2 &&
      region.ReceivingRegion[2] == 2)
    {
      break;
    }

    level = *gridPtr++;
    gridIndex[0] = *gridPtr++;
    gridIndex[1] = *gridPtr++;
    gridIndex[2] = *gridPtr++;

    region.SourceBlock = GetBlock(level, gridIndex[0], gridIndex[1], gridIndex[2]);

    level = *gridPtr++;
    gridIndex[0] = *gridPtr++;
    gridIndex[1] = *gridPtr++;
    gridIndex[2] = *gridPtr++;

    region.ReceivingBlock = GetBlock(level, gridIndex[0], gridIndex[1], gridIndex[2]);

    if (region.ReceivingBlock->CopyFlag == 0)
    { // We cannot modify our input.
      vtkImageData* copy = vtkImageData::New();
      copy->DeepCopy(region.ReceivingBlock->Image);
      region.ReceivingBlock->Image = copy;
      region.ReceivingBlock->CopyFlag = 1;
      // std::cerr << "Unmarshal degenerate region results in a copy\n";
    }
    region.ReceivingArray = region.ReceivingBlock->Image->GetCellData()->GetArray(this->ArrayName);

    messagePtr = gridPtr;

    messagePtr = this->CopyDegenerateRegionMessageToBlock(region, messagePtr, hackLevelFlag);
  }
}

//----------------------------------------------------------------------------
// I am assuming that each block has the same extent.  If boundary ghost
// cells are removed by the reader, then I will add them back as the first
// step of initialization.
void vtkAMRDualGridHelper::ProcessRegionRemoteCopyQueue(bool hackLevelFlag)
{
  if (this->SkipGhostCopy)
  {
    return;
  }

#ifdef VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
  if (this->EnableAsynchronousCommunication && this->Controller->IsA("vtkMPIController"))
  {
    this->ProcessRegionRemoteCopyQueueMPIAsynchronous(hackLevelFlag);
    return;
  }
#endif // VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS

  this->ProcessRegionRemoteCopyQueueSynchronous(hackLevelFlag);
}

void vtkAMRDualGridHelper::ProcessRegionRemoteCopyQueueSynchronous(bool hackLevelFlag)
{
  vtkTimerLogSmartMarkEvent markevent("ProcessRegionRemoteCopyQueueSynchronous", this->Controller);

  int numProcs = this->Controller->GetNumberOfProcesses();
  int myProc = this->Controller->GetLocalProcessId();
  int procIdx;

  VTK_CREATE(vtkIdTypeArray, srcProcs);
  srcProcs->SetNumberOfValues(numProcs);
  VTK_CREATE(vtkIdTypeArray, destProcs);
  destProcs->SetNumberOfValues(numProcs);

  for (procIdx = 0; procIdx < numProcs; ++procIdx)
  {
    srcProcs->SetValue(procIdx, 0);
    destProcs->SetValue(procIdx, 0);
  }

  this->DegenerateRegionMessageSize(srcProcs, destProcs);

  vtkIdType messageLength;

  for (procIdx = 0; procIdx < numProcs; ++procIdx)
  {
    // To avoid blocking.
    // Lower processes send first and receive second.
    // Higher processes receive first and send second.
    if (procIdx < myProc)
    {
      messageLength = destProcs->GetValue(procIdx);
      if (messageLength > 0)
      {
        this->SendDegenerateRegionsFromQueueSynchronous(procIdx, messageLength);
      }
      messageLength = srcProcs->GetValue(procIdx);
      if (messageLength > 0)
      {
        this->ReceiveDegenerateRegionsFromQueueSynchronous(procIdx, messageLength, hackLevelFlag);
      }
    }
    else if (procIdx > myProc)
    {
      messageLength = srcProcs->GetValue(procIdx);
      if (messageLength > 0)
      {
        this->ReceiveDegenerateRegionsFromQueueSynchronous(procIdx, messageLength, hackLevelFlag);
      }
      messageLength = destProcs->GetValue(procIdx);
      if (messageLength > 0)
      {
        this->SendDegenerateRegionsFromQueueSynchronous(procIdx, messageLength);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::SendDegenerateRegionsFromQueueSynchronous(
  int destProc, vtkIdType messageLength)
{
  /*
    vtkIdType messageLength = this->DegenerateRegionMessageSize(
                                 this->Controller->GetLocalProcessId(), destProc);
    if (messageLength == 0)
      { // Nothing to send.
      return;
      }
  */

  // std::cerr << "ProcessDegenerates: Proc " << myProc << " sending " << messageLength << " to " <<
  // destProc << std::endl;
  // Send the message
  this->Controller->Send(&messageLength, 1, destProc, DEGENERATE_REGION_TAG);
  VTK_CREATE(vtkUnsignedCharArray, buffer);
  buffer->SetNumberOfValues(messageLength);

  this->MarshalDegenerateRegionMessage(buffer->GetPointer(0), destProc);

  this->Controller->Send(buffer->GetPointer(0), messageLength, destProc, DEGENERATE_REGION_TAG);
}

//----------------------------------------------------------------------------
void vtkAMRDualGridHelper::ReceiveDegenerateRegionsFromQueueSynchronous(
  int srcProc, vtkIdType messageLength, bool hackLevelFlag)
{
  /*
    vtkIdType messageLength = this->DegenerateRegionMessageSize(
                                  srcProc, this->Controller->GetLocalProcessId());
    if (messageLength == 0)
      { // Nothing to receive
      return;
      }
  */

  int myProc = this->Controller->GetLocalProcessId();
  // Receive the message.
  vtkIdType originalLength = messageLength;
  this->Controller->Receive(&messageLength, 1, srcProc, DEGENERATE_REGION_TAG);
  if (originalLength != messageLength)
  {
    std::cerr << "proc " << myProc << " differed from " << srcProc
              << " estimated: " << originalLength << " received: " << messageLength
              << " difference " << (messageLength - originalLength) << std::endl;
  }
  VTK_CREATE(vtkUnsignedCharArray, buffer);
  buffer->SetNumberOfValues(messageLength);

  this->Controller->Receive(buffer->GetPointer(0), messageLength, srcProc, DEGENERATE_REGION_TAG);

  this->UnmarshalDegenerateRegionMessage(
    buffer->GetPointer(0), messageLength, srcProc, hackLevelFlag);
}

#ifdef VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS

//-----------------------------------------------------------------------------
void vtkAMRDualGridHelper::ProcessRegionRemoteCopyQueueMPIAsynchronous(bool hackLevelFlag)
{
  vtkTimerLogSmartMarkEvent markevent(
    "ProcessRegionRemoteCopyQueueMPIAsynchronous", this->Controller);

  vtkMPIController* controller = vtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    vtkErrorMacro("Internal error:"
                  " ProcessRegionRemoteCopyQueueMPIAsynchronous called without"
                  " MPI controller.");
    return;
  }

  int numProcs = controller->GetNumberOfProcesses();
  int myProc = controller->GetLocalProcessId();

  vtkAMRDualGridHelperCommRequestList sendList;
  vtkAMRDualGridHelperCommRequestList receiveList;

  VTK_CREATE(vtkIdTypeArray, srcProcs);
  srcProcs->SetNumberOfValues(numProcs);
  VTK_CREATE(vtkIdTypeArray, destProcs);
  destProcs->SetNumberOfValues(numProcs);

  for (int procIdx = 0; procIdx < numProcs; procIdx++)
  {
    srcProcs->SetValue(procIdx, 0);
    destProcs->SetValue(procIdx, 0);
  }

  this->DegenerateRegionMessageSize(srcProcs, destProcs);

  vtkIdType messageLength;

  // First establish all receives.  MPI communication is more efficient if
  // the receive is posted before the send.
  for (int sendProc = 0; sendProc < numProcs; sendProc++)
  {
    if (sendProc == myProc)
      continue;
    messageLength = srcProcs->GetValue(sendProc);
    if (messageLength > 0)
    {
      this->ReceiveDegenerateRegionsFromQueueMPIAsynchronous(sendProc, messageLength, receiveList);
    }
  }

  // Next initiate all sends.
  for (int recvProc = 0; recvProc < numProcs; recvProc++)
  {
    if (recvProc == myProc)
      continue;
    messageLength = destProcs->GetValue(recvProc);
    if (messageLength > 0)
    {
      this->SendDegenerateRegionsFromQueueMPIAsynchronous(recvProc, messageLength, sendList);
    }
  }

  // Finally, finish all communications as they come in.
  this->FinishDegenerateRegionsCommMPIAsynchronous(hackLevelFlag, sendList, receiveList);
}

void vtkAMRDualGridHelper::ReceiveDegenerateRegionsFromQueueMPIAsynchronous(
  int sendProc, vtkIdType messageLength, vtkAMRDualGridHelperCommRequestList& receiveList)
{
  vtkMPIController* controller = vtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    vtkErrorMacro("Internal error:"
                  " ProcessRegionRemoteCopyQueueMPIAsynchronous called without"
                  " MPI controller.");
    return;
  }
  int myProc = controller->GetLocalProcessId();

  vtkSmartPointer<vtkCharArray> recvBuffer = vtkSmartPointer<vtkCharArray>::New();
  recvBuffer->SetNumberOfValues(messageLength);

  vtkAMRDualGridHelperCommRequest request;
  request.SendProcess = sendProc;
  request.ReceiveProcess = myProc;
  request.Buffer = recvBuffer;

  // This static cast will cause big problems if we ever have a buffer
  // larger than 2 GB.  Then again, we are unlikely to hit that without
  // running out of memory anyway.
  controller->NoBlockReceive(recvBuffer->GetPointer(0), static_cast<int>(messageLength), sendProc,
    DEGENERATE_REGION_TAG, request.Request);

  receiveList.push_back(request);
}

void vtkAMRDualGridHelper::SendDegenerateRegionsFromQueueMPIAsynchronous(
  int recvProc, vtkIdType messageLength, vtkAMRDualGridHelperCommRequestList& sendList)
{
  vtkMPIController* controller = vtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    vtkErrorMacro("Internal error:"
                  " ProcessRegionRemoteCopyQueueMPIAsynchronous called without"
                  " MPI controller.");
    return;
  }
  int myProc = controller->GetLocalProcessId();

  vtkSmartPointer<vtkCharArray> sendBuffer = vtkSmartPointer<vtkCharArray>::New();
  sendBuffer->SetNumberOfValues(messageLength);

  vtkAMRDualGridHelperCommRequest request;
  request.SendProcess = myProc;
  request.ReceiveProcess = recvProc;
  request.Buffer = sendBuffer;

  this->MarshalDegenerateRegionMessage(sendBuffer->GetPointer(0), recvProc);

  // This static cast will cause big problems if we ever have a buffer
  // larger than 2 GB.  Then again, we are unlikely to hit that without
  // running out of memory anyway.
  controller->NoBlockSend(sendBuffer->GetPointer(0), static_cast<int>(messageLength), recvProc,
    DEGENERATE_REGION_TAG, request.Request);

  sendList.push_back(request);
}

void vtkAMRDualGridHelper::FinishDegenerateRegionsCommMPIAsynchronous(bool hackLevelFlag,
  vtkAMRDualGridHelperCommRequestList& sendList, vtkAMRDualGridHelperCommRequestList& receiveList)
{
  while (!receiveList.empty())
  {
    vtkAMRDualGridHelperCommRequest request = receiveList.WaitAny();
    vtkCharArray* recvBuffer = vtkCharArray::SafeDownCast(request.Buffer);
    this->UnmarshalDegenerateRegionMessage(recvBuffer->GetPointer(0),
      recvBuffer->GetNumberOfTuples(), request.SendProcess, hackLevelFlag);
  }

  sendList.WaitAll();
}

#endif // VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS

// We need to know:
// The number of levels (to make the level structures)
// Global origin, RootSpacing, StandarBlockSize (to convert block extent to grid extent)
// Add all blocks to the level/grids and create faces along the way.

// Note:
// Reader crops invalid ghost cells off boundary blocks.
// Some blocks will have smaller extents!
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
// The array name is the cell array that is being processed by the filter.
// Ghost values have to be modified at level changes.  It could be extended to
// process multiple arrays.
int vtkAMRDualGridHelper::Initialize(vtkNonOverlappingAMR* input)
{
  vtkTimerLogSmartMarkEvent markevent("vtkAMRDualGridHelper::Initialize", this->Controller);

  int blockId, numBlocks;
  int numLevels = input->GetNumberOfLevels();

  // Create the level objects.
  this->Levels.reserve(numLevels);
  for (int ii = 0; ii < numLevels; ++ii)
  {
    vtkAMRDualGridHelperLevel* tmp = new vtkAMRDualGridHelperLevel;
    tmp->Level = ii;
    this->Levels.push_back(tmp);
  }

  // These arrays are meta information passed from the coprocessing
  // adaptor when connected to the simulation.  These are not
  // available when post-processing a file.
  vtkFieldData* inputFd = input->GetFieldData();
  vtkDoubleArray* globalBoundsDa = vtkDoubleArray::SafeDownCast(inputFd->GetArray("GlobalBounds"));
  vtkIntArray* standardBoxSizeIa = vtkIntArray::SafeDownCast(inputFd->GetArray("GlobalBoxSize"));
  vtkIntArray* minLevelIa = vtkIntArray::SafeDownCast(inputFd->GetArray("MinLevel"));
  vtkDoubleArray* minLevelSpacingDa =
    vtkDoubleArray::SafeDownCast(inputFd->GetArray("MinLevelSpacing"));
  vtkIntArray* neighbors = vtkIntArray::SafeDownCast(inputFd->GetArray("Neighbors"));

  // Take advantage of passed in global information if available
  if (globalBoundsDa && standardBoxSizeIa && minLevelIa && minLevelSpacingDa)
  {
    this->GlobalOrigin[0] = globalBoundsDa->GetValue(0);
    this->GlobalOrigin[1] = globalBoundsDa->GetValue(2);
    this->GlobalOrigin[2] = globalBoundsDa->GetValue(4);

    this->StandardBlockDimensions[0] = standardBoxSizeIa->GetValue(0) - 2;
    this->StandardBlockDimensions[1] = standardBoxSizeIa->GetValue(1) - 2;
    this->StandardBlockDimensions[2] = standardBoxSizeIa->GetValue(2) - 2;
    // For 2d case
    if (this->StandardBlockDimensions[2] < 1)
    {
      this->StandardBlockDimensions[2] = 1;
    }
    int lowestLevel = minLevelIa->GetValue(0);
    this->RootSpacing[0] = minLevelSpacingDa->GetValue(0) * (1 << (lowestLevel));
    this->RootSpacing[1] = minLevelSpacingDa->GetValue(1) * (1 << (lowestLevel));
    this->RootSpacing[2] = minLevelSpacingDa->GetValue(2) * (1 << (lowestLevel));
  }
  else
  {
    // Otherwise compute the global info by communication with all processes
    this->ComputeGlobalMetaData(input);
  }

  // Add all of the blocks
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
        this->AddBlock(level, blockId, image);
      }
    }
  }

  if (neighbors)
  {
    // if we have passed neighbor information, use this to send blocks only to those
    // All processes will only have blocks from neigbhoring processes
    vtkSortDataArray::Sort(neighbors);
    this->ShareBlocksWithNeighbors(neighbors);
  }
  else
  {
    // otherwise share block information with all processes
    // All processes will have all blocks (but not image data).
    this->ShareBlocks();
  }
  return VTK_OK;
}

int vtkAMRDualGridHelper::SetupData(vtkNonOverlappingAMR* input, const char* arrayName)
{
  vtkTimerLogSmartMarkEvent markevent("vtkAMRDualGridHelper::SetupData", this->Controller);

  int blockId, numBlocks;
  int numLevels = input->GetNumberOfLevels();

  vtkDualGridHelperCheckAssumption = 1;
  this->SetArrayName(arrayName);

  // For sending degenerate array values we need to know the type.
  // This assumes all images are the same type (of course).
  // Find the first that has data.
  for (int level = 0; level < numLevels; ++level)
  {
    numBlocks = input->GetNumberOfDataSets(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
      vtkImageData* image = input->GetDataSet(level, blockId);
      if (image)
      {
        vtkDataArray* da = image->GetCellData()->GetArray(this->ArrayName);
        if (da)
        {
          this->DataTypeSize = da->GetDataTypeSize();
          break;
        }
        else
        {
          vtkErrorMacro("Could not find the data type size.");
        }
      }
    }
  }

  // Reset all the region bits
  for (int level = 0; level < numLevels; ++level)
  {
    numBlocks = this->GetNumberOfBlocksInLevel(level);
    for (blockId = 0; blockId < numBlocks; ++blockId)
    {
      vtkAMRDualGridHelperBlock* block = this->GetBlock(level, blockId);
      block->ResetRegionBits();
    }
  }

  // Plan for meshing between blocks.
  this->AssignSharedRegions();

  // Copy regions on level boundaries between processes.
  this->ProcessRegionRemoteCopyQueue(false);

  // Setup faces for seeding connectivity between blocks.
  // this->CreateFaces();

  return VTK_OK;
}
void vtkAMRDualGridHelper::ClearRegionRemoteCopyQueue()
{
  this->DegenerateRegionQueue.clear();
}
void vtkAMRDualGridHelper::ShareBlocks()
{
  vtkTimerLogSmartMarkEvent markevent("ShareBlocks", this->Controller);

  if (this->Controller->GetNumberOfProcesses() == 1)
  {
    return;
  }

  VTK_CREATE(vtkIntArray, sendBuffer);
  // sendBuffer->SetNumberOfValues (4096);
  VTK_CREATE(vtkIntArray, recvBuffer);

  this->MarshalBlocks(sendBuffer);

  this->Controller->AllGatherV(sendBuffer, recvBuffer);

  this->UnmarshalBlocks(recvBuffer);
}

void vtkAMRDualGridHelper::ShareBlocksWithNeighbors(vtkIntArray* neighbors)
{
// Intentionally sharing twice so that we can get agreement with neighbors of neighbors
// in terms of which blocks are going to own which regions.
#ifdef VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
  if (this->EnableAsynchronousCommunication && this->Controller->IsA("vtkMPIController"))
  {
    this->ShareBlocksWithNeighborsAsynchronous(neighbors);
    this->ShareBlocksWithNeighborsAsynchronous(neighbors);
    return;
  }
#endif // VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS

  this->ShareBlocksWithNeighborsSynchronous(neighbors);
  this->ShareBlocksWithNeighborsSynchronous(neighbors);
}

#ifdef VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
void vtkAMRDualGridHelper::ShareBlocksWithNeighborsAsynchronous(vtkIntArray* neighbors)
{
  vtkTimerLogSmartMarkEvent markevent("ShareBlocksWithNeighborsAsync", this->Controller);
  if (this->Controller->GetNumberOfProcesses() == 1)
  {
    return;
  }

  vtkAMRDualGridHelperCommRequestList sendList;
  vtkAMRDualGridHelperCommRequestList receiveList;

  int myProc = this->Controller->GetLocalProcessId();

  vtkMPIController* controller = vtkMPIController::SafeDownCast(this->Controller);
  if (!controller)
  {
    vtkErrorMacro("Internal error:"
                  " ProcessRegionRemoteCopyQueueMPIAsynchronous called without"
                  " MPI controller.");
    return;
  }

  for (vtkIdType i = 0; i < neighbors->GetNumberOfTuples(); i++)
  {
    int neighborProc = neighbors->GetValue(i);

    vtkSmartPointer<vtkIntArray> recvBuffer = vtkSmartPointer<vtkIntArray>::New();
    // Set this large enough to capture the number of blocks contained in neighbors
    recvBuffer->SetNumberOfValues(131072);

    vtkAMRDualGridHelperCommRequest request;
    request.SendProcess = neighborProc;
    request.ReceiveProcess = myProc;
    request.Buffer = recvBuffer;

    controller->NoBlockReceive(
      recvBuffer->GetPointer(0), 131072, neighborProc, SHARED_BLOCK_TAG, request.Request);

    receiveList.push_back(request);
  }

  for (vtkIdType i = 0; i < neighbors->GetNumberOfTuples(); i++)
  {
    int neighborProc = neighbors->GetValue(i);

    vtkSmartPointer<vtkIntArray> sendBuffer = vtkSmartPointer<vtkIntArray>::New();
    this->MarshalBlocks(sendBuffer);

    vtkAMRDualGridHelperCommRequest request;
    request.SendProcess = myProc;
    request.ReceiveProcess = neighborProc;
    request.Buffer = sendBuffer;

    controller->NoBlockSend(sendBuffer->GetPointer(0), sendBuffer->GetNumberOfTuples(),
      neighborProc, SHARED_BLOCK_TAG, request.Request);

    sendList.push_back(request);
  }

  while (!receiveList.empty())
  {
    vtkAMRDualGridHelperCommRequest request = receiveList.WaitAny();
    vtkIntArray* buffer = vtkIntArray::SafeDownCast(request.Buffer);
    this->UnmarshalBlocksFromOne(buffer, request.SendProcess);
  }

  sendList.WaitAll();
}
#endif // VTK_AMR_DUAL_GRID_USE_MPI_ASYNCHRONOUS
void vtkAMRDualGridHelper::ShareBlocksWithNeighborsSynchronous(vtkIntArray* neighbors)
{
  vtkTimerLogSmartMarkEvent markevent("ShareBlocksWithNeighborsSync", this->Controller);
  if (this->Controller->GetNumberOfProcesses() == 1)
  {
    return;
  }

  VTK_CREATE(vtkIntArray, sendBuffer);
  VTK_CREATE(vtkIntArray, recvBuffer);
  recvBuffer->SetNumberOfValues(131072);

  int myProc = this->Controller->GetLocalProcessId();

  this->MarshalBlocks(sendBuffer);
  int messageLength = sendBuffer->GetNumberOfTuples();

  for (vtkIdType i = 0; i < neighbors->GetNumberOfTuples(); i++)
  {
    int neighborProc = neighbors->GetValue(i);
    if (neighborProc < myProc)
    {
      this->Controller->Send(
        sendBuffer->GetPointer(0), messageLength, neighborProc, SHARED_BLOCK_TAG);
      this->Controller->Receive(recvBuffer->GetPointer(0), 131072, neighborProc, SHARED_BLOCK_TAG);
    }
    else
    {
      this->Controller->Receive(recvBuffer->GetPointer(0), 131072, neighborProc, SHARED_BLOCK_TAG);
      this->Controller->Send(
        sendBuffer->GetPointer(0), messageLength, neighborProc, SHARED_BLOCK_TAG);
    }
    this->UnmarshalBlocksFromOne(recvBuffer, neighborProc);
  }
}
void vtkAMRDualGridHelper::MarshalBlocks(vtkIntArray* inBuffer)
{
  inBuffer->SetNumberOfValues(0);
  // Marshal the procs.
  // numlevels, level0NumBlocks,(gridx,gridy,gridz,...),level1NumBlocks,(...)
  int numLevels = this->GetNumberOfLevels();

  // Now create the message.
  inBuffer->InsertNextValue(numLevels);
  for (int levelIdx = 0; levelIdx < numLevels; levelIdx++)
  {
    vtkAMRDualGridHelperLevel* level = this->Levels[levelIdx];
    int numBlocks = static_cast<int>(level->Blocks.size());
    inBuffer->InsertNextValue(numBlocks);
    for (int blockIdx = 0; blockIdx < numBlocks; blockIdx++)
    {
      vtkAMRDualGridHelperBlock* block = level->Blocks[blockIdx];
      inBuffer->InsertNextValue(block->GridIndex[0]);
      inBuffer->InsertNextValue(block->GridIndex[1]);
      inBuffer->InsertNextValue(block->GridIndex[2]);
      inBuffer->InsertNextValue(block->ProcessId);
    }
  }
}
void vtkAMRDualGridHelper::UnmarshalBlocks(vtkIntArray* inBuffer)
{
  int* buffer = inBuffer->GetPointer(0);
  // Unmarshal the procs.
  // Each process sent a message of this form.
  //
  // numlevels, level0NumBlocks,(gridx,gridy,gridz,...),level1NumBlocks,(...)
  //
  // The messages from all processes are mashed together in buffer in order
  // by process id.

  int myProc = this->Controller->GetLocalProcessId();
  int numProc = this->Controller->GetNumberOfProcesses();

  for (int blockProc = 0; blockProc < numProc; blockProc++)
  {
    int numLevels = *buffer++;
    for (int levelIdx = 0; levelIdx < numLevels; levelIdx++)
    {
      int numBlocks = *buffer++;
      if (blockProc == myProc)
      {
        // Skip over my own blocks.
        buffer += 4 * numBlocks;
        continue;
      }
      vtkAMRDualGridHelperLevel* level = this->Levels[levelIdx];
      for (int blockIdx = 0; blockIdx < numBlocks; blockIdx++)
      {
        int x = *buffer++;
        int y = *buffer++;
        int z = *buffer++;

        vtkAMRDualGridHelperBlock* block = level->AddGridBlock(x, y, z, blockIdx, nullptr);
        block->ProcessId = *buffer++;

        block->OriginIndex[0] = this->StandardBlockDimensions[0] * x - 1;
        block->OriginIndex[1] = this->StandardBlockDimensions[1] * y - 1;
        block->OriginIndex[2] = this->StandardBlockDimensions[2] * z - 1;
      }
    }
  }
}
void vtkAMRDualGridHelper::UnmarshalBlocksFromOne(vtkIntArray* inBuffer, int vtkNotUsed(blockProc))
{
  int* buffer = inBuffer->GetPointer(0);
  // Unmarshal the procs.
  // Each process sent a message of this form.
  //
  // numlevels, level0NumBlocks,(gridx,gridy,gridz,...),level1NumBlocks,(...)
  //
  // The messages from all processes are mashed together in buffer in order
  // by process id.

  int numLevels = *buffer++;
  for (int levelIdx = 0; levelIdx < numLevels; levelIdx++)
  {
    int numBlocks = *buffer++;
    vtkAMRDualGridHelperLevel* level = this->Levels[levelIdx];
    for (int blockIdx = 0; blockIdx < numBlocks; blockIdx++)
    {
      int x = *buffer++;
      int y = *buffer++;
      int z = *buffer++;

      vtkAMRDualGridHelperBlock* block = level->AddGridBlock(x, y, z, blockIdx, nullptr);
      block->ProcessId = *buffer++;

      block->OriginIndex[0] = this->StandardBlockDimensions[0] * x - 1;
      block->OriginIndex[1] = this->StandardBlockDimensions[1] * y - 1;
      block->OriginIndex[2] = this->StandardBlockDimensions[2] * z - 1;
    }
  }
}

namespace
{

class vtkReduceMeta : public vtkCommunicator::Operation
{
  void Function(
    const void* A, void* B, vtkIdType vtkNotUsed(length), int vtkNotUsed(datatype)) override

  {
    const double* dmsgA = reinterpret_cast<const double*>(A);
    double* dmsgB = reinterpret_cast<double*>(B);
    if (dmsgA[0] > dmsgB[0] /* || (dmsgA[0] == dmsgB[0] && dmsgA[7] < dmsgB[7]) */)
    {
      dmsgB[0] = dmsgA[0]; // largestNumCell
      dmsgB[1] = dmsgA[1]; // largestDims 0
      dmsgB[2] = dmsgA[2]; // largestDims 1
      dmsgB[3] = dmsgA[3]; // largestDims 2
      // dmsgB[4] = dmsgA[4]; // largestOrigin 0
      // dmsgB[5] = dmsgA[5]; // largestOrigin 1
      // dmsgB[6] = dmsgA[6]; // largestOrigin 2
      // dmsgB[7] = dmsgA[7]; // largestSpacing 0
      // dmsgB[8] = dmsgA[8]; // largestSpacing 1
      // dmsgB[9] = dmsgA[9]; // largestSpacing 2
      // dmsgB[10] = dmsgA[10]; // largestLevel
    }
    if (dmsgA[4] > dmsgB[4])
    {
      dmsgB[4] = dmsgA[4]; // lowestSpacing 0
      dmsgB[5] = dmsgA[5]; // lowestSpacing 1
      dmsgB[6] = dmsgA[6]; // lowestSpacing 2
      dmsgB[7] = dmsgA[7]; // lowestLevel
      // dmsgB[15] = dmsgA[15]; // lowestOrigin 0
      // dmsgB[16] = dmsgA[16]; // lowestOrigin 1
      // dmsgB[17] = dmsgA[17]; // lowestOrigin 2
      // dmsgB[18] = dmsgA[18]; // lowestDims 0
      // // dmsgB[19] = dmsgA[19]; // lowestDims 1
      // dmsgB[20] = dmsgA[20]; // lowestDims 2
    }
    if (dmsgB[8] > dmsgA[8])
    {
      dmsgB[8] = dmsgA[8];
    } // globalBounds 0
    // if (dmsgB[22] < dmsgA[22]) { dmsgB[22] = dmsgA[22]; } // globalBounds 1
    if (dmsgB[9] > dmsgA[9])
    {
      dmsgB[9] = dmsgA[9];
    } // globalBounds 2
    // if (dmsgB[24] < dmsgA[24]) { dmsgB[24] = dmsgA[24]; } // globalBounds 3
    if (dmsgB[10] > dmsgA[10])
    {
      dmsgB[10] = dmsgA[10];
    } // globalBounds 4
    // if (dmsgB[26] < dmsgA[26]) { dmsgB[26] = dmsgA[26]; } // globalBounds 5
  }
  int Commutative() override { return 1; }
};
};

void vtkAMRDualGridHelper::ComputeGlobalMetaData(vtkNonOverlappingAMR* input)
{
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
  vtkTimerLogSmartMarkEvent markevent("ComputeGlobalMetaData", this->Controller);

  int numLevels = input->GetNumberOfLevels();
  int numBlocks;
  int blockId;

  int lowestLevel = 0;
  double lowestSpacing[3];
  double lowestOrigin[3];
  // int    lowestDims[3];
  // int    largestLevel = 0;
  double largestOrigin[3];
  double largestSpacing[3];
  int largestDims[3] = { 0, 0, 0 };
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

  this->NumberOfBlocksInThisProcess = 0;
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
        ++this->NumberOfBlocksInThisProcess;
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
          // largestLevel = level;
        }
        // Find the lowest level block.
        image->GetSpacing(spacing);
        if (spacing[0] > lowestSpacing[0]) // Only test axis 0. Assume others agree.
        {                                  // This is the lowest level block we have encountered.
          image->GetSpacing(lowestSpacing);
          lowestLevel = level;
          image->GetOrigin(lowestOrigin);
          // lowestDims[0] = cellDims[0];
          // lowestDims[1] = cellDims[1];
          // lowestDims[2] = cellDims[2];
        }
      }
    }
  }

  // Send the results to process 0 that will choose the origin ...

  const int REDUCE_MESSAGE_SIZE = 11;
  double dMsg[REDUCE_MESSAGE_SIZE];
  double dRcv[REDUCE_MESSAGE_SIZE];
  if (this->Controller->GetNumberOfProcesses() > 1)
  {

    dMsg[0] = largestNumCells;
    dMsg[1] = largestDims[0];
    dMsg[2] = largestDims[1];
    dMsg[3] = largestDims[2];
    // dMsg[4] = largestOrigin[0];
    // dMsg[5] = largestOrigin[1];
    // dMsg[6] = largestOrigin[2];
    // dMsg[7] = largestSpacing[0];
    // dMsg[8] = largestSpacing[1];
    // dMsg[9] = largestSpacing[2];
    // dMsg[10] = largestLevel;
    dMsg[4] = lowestSpacing[0];
    dMsg[5] = lowestSpacing[1];
    dMsg[6] = lowestSpacing[2];
    dMsg[7] = lowestLevel;
    // dMsg[15] = lowestOrigin[0];
    // dMsg[16] = lowestOrigin[1];
    // dMsg[17] = lowestOrigin[2];
    // dMsg[18] = lowestDims[0];
    // dMsg[19] = lowestDims[1];
    // dMsg[20] = lowestDims[2];
    dMsg[8] = globalBounds[0];
    // dMsg[22] = globalBounds[1];
    dMsg[9] = globalBounds[2];
    // dMsg[24] = globalBounds[3];
    dMsg[10] = globalBounds[4];
    // dMsg[26] = globalBounds[5];

    vtkReduceMeta operation;
    if (!this->Controller->AllReduce(dMsg, dRcv, REDUCE_MESSAGE_SIZE, &operation))
    {
      vtkErrorMacro("AllReduce failed");
    }

    largestNumCells = (int)dRcv[0];
    largestDims[0] = (int)dRcv[1];
    largestDims[1] = (int)dRcv[2];
    largestDims[2] = (int)dRcv[3];
    // largestOrigin[0] = dRcv[4];
    // largestOrigin[1] = dRcv[5];
    // largestOrigin[2] = dRcv[6];
    // largestSpacing[0] = dRcv[7];
    // largestSpacing[1] = dRcv[8];
    // largestSpacing[2] = dRcv[9];
    // largestLevel = (int)dRcv[10];
    lowestSpacing[0] = dRcv[4];
    lowestSpacing[1] = dRcv[5];
    lowestSpacing[2] = dRcv[6];
    lowestLevel = (int)dRcv[7];
    // lowestOrigin[0] = dRcv[15];
    // lowestOrigin[1] = dRcv[16];
    // lowestOrigin[2] = dRcv[17];
    // lowestDims[0] = (int)dRcv[18];
    // lowestDims[1] = (int)dRcv[19];
    // lowestDims[2] = (int)dRcv[20];
    globalBounds[0] = dRcv[8];
    // globalBounds[1] = dRcv[22];
    globalBounds[2] = dRcv[9];
    // globalBounds[3] = dRcv[24];
    globalBounds[4] = dRcv[10];
    // globalBounds[5] = dRcv[26];
  }
  this->StandardBlockDimensions[0] = largestDims[0] - 2;
  this->StandardBlockDimensions[1] = largestDims[1] - 2;
  this->StandardBlockDimensions[2] = largestDims[2] - 2;
  // For 2d case
  if (this->StandardBlockDimensions[2] < 1)
  {
    this->StandardBlockDimensions[2] = 1;
  }
  this->RootSpacing[0] = lowestSpacing[0] * (1 << (lowestLevel));
  this->RootSpacing[1] = lowestSpacing[1] * (1 << (lowestLevel));
  this->RootSpacing[2] = lowestSpacing[2] * (1 << (lowestLevel));

//    DebuggingRootSpacing[0] = this->RootSpacing[0];
//    DebuggingRootSpacing[1] = this->RootSpacing[1];
//    DebuggingRootSpacing[2] = this->RootSpacing[2];

#if 0
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
    idx[0] = (int)(floor(0.5 + (lowestOrigin[0]-largestOrigin[0]) / largestSpaci
ng[0]));
    idx[1] = (int)(floor(0.5 + (lowestOrigin[1]-largestOrigin[1]) / largestSpaci
ng[1]));
    idx[2] = (int)(floor(0.5 + (lowestOrigin[2]-largestOrigin[2]) / largestSpaci
ng[2]));

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
#endif

  // The above doesn't seem to account for some blocks being defined on ghost bounds
  this->GlobalOrigin[0] = globalBounds[0];
  this->GlobalOrigin[1] = globalBounds[2];
  this->GlobalOrigin[2] = globalBounds[4];

  //    DebuggingGlobalOrigin[0] = this->GlobalOrigin[0];
  //    DebuggingGlobalOrigin[1] = this->GlobalOrigin[1];
  //    DebuggingGlobalOrigin[2] = this->GlobalOrigin[2];
}
