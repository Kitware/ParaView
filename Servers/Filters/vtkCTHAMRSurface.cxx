/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRSurface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRSurface.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkMultiProcessController.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkOutlineSource.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"


vtkCxxRevisionMacro(vtkCTHAMRSurface, "1.1");
vtkStandardNewMacro(vtkCTHAMRSurface);

//----------------------------------------------------------------------------
vtkCTHAMRSurface::vtkCTHAMRSurface()
{
  this->PositiveXVisibility = 1;
  this->NegativeXVisibility = 1;
  this->PositiveYVisibility = 1;
  this->NegativeYVisibility = 1;
  this->PositiveZVisibility = 1;
  this->NegativeZVisibility = 1;
}

//----------------------------------------------------------------------------
vtkCTHAMRSurface::~vtkCTHAMRSurface()
{
}

//----------------------------------------------------------------------------
void vtkCTHAMRSurface::Execute()
{
  vtkMultiProcessController* controller;
  controller = vtkMultiProcessController::GetGlobalController();
  int numProcs = controller->GetNumberOfProcesses();
  int myId = controller->GetLocalProcessId();
  vtkCTHData* input = this->GetInput();
  vtkPolyData* output = this->GetOutput();
  int numGhostLevels = input->GetNumberOfGhostLevels();
  
  // Collect all of the block information to node zero.
  // Each block has 7 integers: level, ext[6].
  if (myId == 0)
    {
    int idx;
    int* numProcBlocks = new int[numProcs];
    int** procBlockInfo = new int*[numProcs];
    int** procVisibleBlockFaces = new int*[numProcs];
    numProcBlocks[0] = input->GetNumberOfBlocks();
    procBlockInfo[0] = new int[7*numProcBlocks[0]];
    this->GetBlockInformation(input, procBlockInfo[0]);
    procVisibleBlockFaces[0] = new int[numProcBlocks[0]];
    for (idx = 1; idx < numProcs; ++idx)
      {
      controller->Receive(numProcBlocks+idx, 1, idx, 534598);
      procBlockInfo[idx] = new int(7*numProcBlocks[idx]);
      procVisibleBlockFaces[idx] = new int(numProcBlocks[idx]);
      controller->Receive(procBlockInfo[idx], 7*numProcBlocks[myId], idx, 534599);
      }
    this->ComputeOutsideFacesFromInformation(numGhostLevels, numProcs, 
                                             numProcBlocks, procBlockInfo, 
                                             procVisibleBlockFaces);
    // Send the results back to the satellite processes.
    for (idx = 1; idx < numProcs; ++idx)
      {
      controller->Send(procVisibleBlockFaces[idx], numProcBlocks[idx], idx, 534600); 
      }
    // Generate the output.
    this->CreateOutput(input, output, numProcBlocks[0], procVisibleBlockFaces[0]);
    // Delete all of the arrays.
    for (idx = 1; idx < numProcs; ++idx)
      {
      delete [] procBlockInfo[idx];
      delete [] procVisibleBlockFaces[idx];
      }
    delete [] numProcBlocks;
    delete [] procBlockInfo;
    delete [] procVisibleBlockFaces;
    }
  else
    { // Satellite process.
    int numBlocks = input->GetNumberOfBlocks();
    int* blockInfo = new int[7*numBlocks];
    this->GetBlockInformation(input, blockInfo);
    controller->Send(&numBlocks, 1, 0, 534598);
    controller->Send(blockInfo, 7*numBlocks, 0, 534599);
    int* visibleBlockFaces = new int[numBlocks];
    controller->Receive(visibleBlockFaces, numBlocks, 0, 534600); 
    // Generate the output.
    this->CreateOutput(input, output, numBlocks, visibleBlockFaces);
    
    delete [] blockInfo;
    delete [] visibleBlockFaces;    
    }
}

//----------------------------------------------------------------------------
void vtkCTHAMRSurface::GetBlockInformation(vtkCTHData* input, int* blockInfo)
{
  int numBlocks = input->GetNumberOfBlocks();
  int idx;
  
  for (idx = 0; idx < numBlocks; ++idx)
    {
    *blockInfo++ = input->GetBlockLevel(idx);
    input->GetBlockPointExtent(idx, blockInfo);
    blockInfo += 6;
    }
}

//----------------------------------------------------------------------------
// This could get expensive because it is an N^2 operation.
// I am doing it in one process because I expect that broadcasting the
// block information to all processes would be more expensive.
void vtkCTHAMRSurface::ComputeOutsideFacesFromInformation(int numGhostLevels,
                            int numProcs, int* procNumBlocks, 
                            int** procBlock7Info, int** procBlock1VisibleFaces)
{
  int proc, block, level;
  int ext[6];
  int faceExt[6];
  int numBlocks;
  int* block7Info;
  int* block1VisibleFaces;
  unsigned char visibilityMask = 0;
  
  // Loop through the processes.
  for (proc = 0; proc < numProcs; ++proc)
    {
    // Loop through the blocks.
    numBlocks = procNumBlocks[proc];
    block7Info = procBlock7Info[proc];
    block1VisibleFaces = procBlock1VisibleFaces[proc];
    for (block = 0; block < numBlocks; ++block)
      {
      level = block7Info[0];
      memcpy(ext,block7Info+1,6*sizeof(int));
      // Get rid of ghost extent.
      if ( ext[1] > ext[0])
        {
        ext[0] += numGhostLevels;
        ext[1] -= numGhostLevels;
        }
      if (ext[3] > ext[2])
        {
        ext[2] += numGhostLevels;
        ext[3] -= numGhostLevels;
        }
      if (ext[5] > ext[4])
        { // 2d does not have a ghost extent in z.
        ext[4] += numGhostLevels;
        ext[5] -= numGhostLevels;
        }
      visibilityMask = 0;
      // Loop through the 6 faces.
      // +x:right
      faceExt[0] = ext[1];
      faceExt[1] = ext[1];
      faceExt[2] = ext[2];
      faceExt[3] = ext[3];
      faceExt[4] = ext[4];
      faceExt[5] = ext[5];
      // Quick fix for 2D
      if (this->PositiveXVisibility && ext[1] > ext[0] &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(1);
        }
      // -x:left
      faceExt[0] = ext[0];
      faceExt[1] = ext[0];
      if (this->NegativeXVisibility &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(2);
        }
      // +y:top
      faceExt[1] = ext[1];
      faceExt[2] = ext[3];
      if (this->PositiveYVisibility && ext[3] > ext[2] &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(4);
        }
      // -y:bottom
      faceExt[2] = ext[2];
      faceExt[3] = ext[2];
      if (this->NegativeYVisibility &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(8);
        }
      // +z:front
      faceExt[3] = ext[3];
      faceExt[4] = ext[5];
      if (this->PositiveZVisibility && ext[5] > ext[4] &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(16);
        }
      // -y:back
      faceExt[4] = ext[4];
      faceExt[5] = ext[4];
      if (this->NegativeZVisibility &&
          this->CheckFaceVisibility(level, faceExt, numProcs, 
                                    procNumBlocks, procBlock7Info,
                                    proc, block, numGhostLevels))
        {
        visibilityMask = visibilityMask | (unsigned char)(32);
        }
      block1VisibleFaces[block] = visibilityMask;

      // Move to the next block
      block7Info += 7;
      }
    }
}


//----------------------------------------------------------------------------
int vtkCTHAMRSurface::CheckFaceVisibility(int currentLevel, int* faceExt, int numProcs,
                                     int* procNumBlocks, int** procBlock7Info,
                                     int currentProc, int currentBlock, int numGhostLevels)
{
  int proc, block, level;
  int numBlocks;
  int* block7Info;
  
  int factor;
  int commonFaceExt[6];
  int commonBlockExt[6];
  int *ext;
  
  int xOverlap, yOverlap, zOverlap;
  int dim = 0;
  if (faceExt[0] < faceExt[1]) {++dim;}
  if (faceExt[2] < faceExt[3]) {++dim;}
  if (faceExt[4] < faceExt[5]) {++dim;}
  if (dim != 2)
    {
    return 0;
    }
  
  // Loop through the processes.
  for (proc = 0; proc < numProcs; ++proc)
    {
    // Loop through the blocks.
    numBlocks = procNumBlocks[proc];
    block7Info = procBlock7Info[proc];
    for (block = 0; block < numBlocks; ++block)
      {
      if (proc != currentProc || block != currentBlock)
        {
        level = block7Info[0];
        ext = block7Info+1;
        // convert the two extents to a common level.
        memcpy(commonFaceExt, faceExt, 6*sizeof(int));
        memcpy(commonBlockExt, ext, 6*sizeof(int));
        // Get rid of ghost extent.
        commonBlockExt[0] += numGhostLevels;
        commonBlockExt[1] -= numGhostLevels;
        commonBlockExt[2] += numGhostLevels;
        commonBlockExt[3] -= numGhostLevels;
        commonBlockExt[4] += numGhostLevels;
        commonBlockExt[5] -= numGhostLevels;
        if (currentLevel > level)
          { // convert block extent up some levels.
          factor = currentLevel - level;
          commonBlockExt[0] = commonBlockExt[0] << factor;
          commonBlockExt[1] = commonBlockExt[1] << factor;
          commonBlockExt[2] = commonBlockExt[2] << factor;
          commonBlockExt[3] = commonBlockExt[3] << factor;
          commonBlockExt[4] = commonBlockExt[4] << factor;
          commonBlockExt[5] = commonBlockExt[5] << factor;
          }
        if (currentLevel < level)
          { // convert face extent up some levels.
          factor = level - currentLevel;
          commonFaceExt[0] = commonFaceExt[0] << factor;
          commonFaceExt[1] = commonFaceExt[1] << factor;
          commonFaceExt[2] = commonFaceExt[2] << factor;
          commonFaceExt[3] = commonFaceExt[3] << factor;
          commonFaceExt[4] = commonFaceExt[4] << factor;
          commonFaceExt[5] = commonFaceExt[5] << factor;
          }
        // Check for any part of the face include in block.
        // This is tricky because sharing a single corner 
        // or edge is not enough.  Two axes have to overlap
        // and the other has to at least touch.
        xOverlap = yOverlap = zOverlap = 0;
        if (commonFaceExt[0] <= commonBlockExt[1] && commonFaceExt[1] >= commonBlockExt[0])
          {
          xOverlap = 1;
          if (commonFaceExt[0] < commonBlockExt[1] && commonFaceExt[1] > commonBlockExt[0])
            {
            xOverlap = 2;
            }
          }
        if (commonFaceExt[2] <= commonBlockExt[3] && commonFaceExt[3] >= commonBlockExt[2])
          {
          yOverlap = 1;
          if (commonFaceExt[2] < commonBlockExt[3] && commonFaceExt[3] > commonBlockExt[2])
            {
            yOverlap = 2;
            }
          }
        if (commonFaceExt[4] <= commonBlockExt[5] && commonFaceExt[5] >= commonBlockExt[4])
          {
          zOverlap = 1;
          if (commonFaceExt[4] < commonBlockExt[5] && commonFaceExt[5] > commonBlockExt[4])
            {
            zOverlap = 2;
            }
          }
        if (xOverlap + yOverlap + zOverlap >= 5)
          { // Found a block that at least partially overlaps this face.
          return 0;
          }
        }  
      // Move to the next block
      block7Info += 7;
      }
    }
  // No overlapping blocks.
  return 1;
}
//----------------------------------------------------------------------------
void vtkCTHAMRSurface::GetInputBlockCellExtentWithoutGhostLevels(vtkCTHData* input, 
                                                            int blockId, int* ext)
{
  int numGhostLevels = input->GetNumberOfGhostLevels();
  input->GetBlockCellExtent(blockId, ext);
  if (ext[1] > ext[0])
    {
    ext[0] += numGhostLevels;
    ext[1] -= numGhostLevels;
    }
  if (ext[3] > ext[2])
    {
    ext[2] += numGhostLevels;
    ext[3] -= numGhostLevels;
    }
  if (ext[5] > ext[4])
    {
    ext[4] += numGhostLevels;
    ext[5] -= numGhostLevels;
    }
}
//----------------------------------------------------------------------------
void vtkCTHAMRSurface::CreateOutput(vtkCTHData* input, vtkPolyData* output, 
                                    int numBlocks, int* visibleBlockFaces)
{
  vtkIdType numCells = 0;
  vtkIdType numPoints = 0;
  int faceMask;
  int ext[6];
  int i;
  
  // Count the number of faces to generate so we can allocate the output.
  for (i = 0; i < numBlocks; ++i)
    {
    faceMask = visibleBlockFaces[i];
    // Most blocks will have zero or one visibile faces.
    // Getting the extent multiple times for the few others
    // should not be an issue.
    if (faceMask & 1)
      { // +x
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+2)*(ext[5]-ext[4]+2);
      }
    if (faceMask & 2)
      { // -x
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[3]-ext[2]+2)*(ext[5]-ext[4]+2);
      }
    if (faceMask & 4)
      { // +y
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+2)*(ext[5]-ext[4]+2);
      }
    if (faceMask & 8)
      { // -y
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
      numPoints += (ext[1]-ext[0]+2)*(ext[5]-ext[4]+2);
      }
    if (faceMask & 16)
      { // +z
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+2)*(ext[3]-ext[2]+2);
      }
    if (faceMask & 32)
      { // -z
      this->GetInputBlockCellExtentWithoutGhostLevels(input, i, ext);
      numCells += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
      numPoints += (ext[1]-ext[0]+2)*(ext[3]-ext[2]+2);
      }
    }
    
  vtkPoints* newPoints = vtkPoints::New();
  vtkCellArray* newPolys = vtkCellArray::New();
  newPolys->Allocate(numCells * 5);
  newPoints->Allocate(numPoints);
  output->SetPoints(newPoints);
  output->SetPolys(newPolys);
  newPoints->Delete();
  newPolys->Delete();
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPoints);
 
 
  // Now loop through the blocks adding faces to the output.  
  vtkIdType inPtOffset = 0;
  vtkIdType inCellOffset = 0;
  int face;
  for (i = 0; i < numBlocks; ++i)
    {
    faceMask = visibleBlockFaces[i];
    face = faceMask & 1;
    if (face)
      { // +x
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    face = faceMask & 2;
    if (face)
      { // -x
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    face = faceMask & 4;
    if (face)
      { // +Y
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    face = faceMask & 8;
    if (face)
      { // -Y
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    face = faceMask & 16;
    if (face)
      { // +Z
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    face = faceMask & 32;
    if (face)
      { // -Z
      this->MakeFace(input, i, face, inCellOffset, inPtOffset, output);
      }
    input->GetBlockCellExtent(i, ext);
    inCellOffset += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    input->GetBlockPointExtent(i, ext);
    inPtOffset += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
}

//----------------------------------------------------------------------------
// Code for faces: 1:+x, 2:-x, 4:+y, 8:-y, 16:+z, 32: -z
void vtkCTHAMRSurface::MakeFace(vtkCTHData *input, int blockId, int face,
                       vtkIdType inputCellOffset, vtkIdType inputPointOffset,
                       vtkPolyData *output)
{
  int i, j;
  vtkIdType    inId, outId;
  vtkPoints    *outPts;
  vtkCellArray *outPolys;
  vtkPointData *inPD, *outPD;
  vtkCellData  *inCD, *outCD;
  // Information returned be the magic FindFace method.
  int          facePtExt[6]; // Axes order post permutation.
  //           0: constant offset, 1,2 face axes increments of input.
  int          inPIncs[3];
  int          inCIncs[3];
  //           Maps face axis to world axis. 
  int          permutation[3];
  
  // Generate the increments and
  // swap axes to make generation of the six faces look the same.
  // Collapsed axis goes to last axis. 
  // Handle copying of cell data (3D to 2D). 
  // Min faces are different than max faces.
  if ( ! this->FindFace(input, blockId, face, 
                        facePtExt, inPIncs, inCIncs, permutation))
    { // The extent is not 2D.
    // This condition should never happen now.
    return;
    }
  // Account for block offset of input.
  inPIncs[0] += inputPointOffset;
  inCIncs[0] += inputCellOffset;

  outPts = output->GetPoints();
  outPolys = output->GetPolys();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  // Save the relative starting point for the points.
  // We need this to compute absolute point ids
  // for the cells.
  vtkIdType outPtOffset = outPts->GetNumberOfPoints();

  // Create the new points.
  double origin[3];
  double* spacing;
  double pt[3];
  input->GetTopLevelOrigin(origin);
  spacing = input->GetBlockSpacing(blockId);
  for (j = facePtExt[2]; j <= facePtExt[3]; ++j)
    {
    for (i = facePtExt[0]; i <= facePtExt[1]; ++i)
      { 
      pt[0] = origin[0];
      pt[1] = origin[1];
      pt[2] = origin[2];
      pt[permutation[0]] += (double)(i)*spacing[permutation[0]];
      pt[permutation[1]] += (double)(j)*spacing[permutation[1]];
      pt[permutation[2]] += facePtExt[4]*spacing[permutation[2]];
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      inId = inPIncs[0] + inPIncs[1]*(i-facePtExt[0]) + inPIncs[2]*(j-facePtExt[2]);
      outPD->CopyData(inPD,inId,outId);
      }
    }

  // Create the new cells. 
  // The loop uses exclusive end condition to account for using point extent.
  vtkIdType ptIds[4];
  for (j = facePtExt[2]; j < facePtExt[3]; ++j)
    {
    for (i = facePtExt[0]; i < facePtExt[1]; ++i)
      {
      // Compute the ids of the quad points.
      // We cannot use increments because they are for input.
      ptIds[0] = outPtOffset + (j-facePtExt[2])*(facePtExt[1]-facePtExt[0]+1) 
                             + (i-facePtExt[0]);
      ptIds[1] = ptIds[0] + 1;
      ptIds[2] = ptIds[1] + (facePtExt[1]-facePtExt[0]+1);
      ptIds[3] = ptIds[2] - 1;
      outId = outPolys->InsertNextCell(4, ptIds);
      // Copy cell data.
      inId = inCIncs[0] + inCIncs[1]*(i-facePtExt[0]) + inCIncs[2]*(j-facePtExt[2]);
      outCD->CopyData(inCD,inId,outId);
      }
    }
}
  


//----------------------------------------------------------------------------
int vtkCTHAMRSurface::FindFace(vtkCTHData* input, int blockId, int face, 
                               int ptExt[6], int inPIncs[3], int inCIncs[3], 
                               int permutation[3])
{
  int ext[6];
  int numGhostLevels = input->GetNumberOfGhostLevels();
  input->GetBlockPointExtent(blockId, ext);
  int incs[3];
  
  // Compute permutation.
  switch (face)
    {
    case 1: 
      permutation[0] = 1;
      permutation[1] = 2;
      permutation[2] = 0;
      break;
    case 2:
      // -x swapped to get correct quad point ordering.
      permutation[0] = 2;
      permutation[1] = 1;
      permutation[2] = 0;
      break;
    case 4:
      permutation[0] = 2;
      permutation[1] = 0;    
      permutation[2] = 1;
      break;
    case 8:
      permutation[0] = 0;
      permutation[1] = 2;
      permutation[2] = 1;
      break;
    case 16:
      permutation[0] = 0;
      permutation[1] = 1;    
      permutation[2] = 2;
      break;
    case 32:
      permutation[0] = 1;
      permutation[1] = 0;    
      permutation[2] = 2;
      break;
    default:
      vtkErrorMacro("Unknown face code " << face);
    }
    
  // Extent of the image.
  ptExt[0] = ext[permutation[0]*2] + numGhostLevels;
  ptExt[1] = ext[permutation[0]*2 + 1] - numGhostLevels;
  ptExt[2] = ext[permutation[1]*2] + numGhostLevels;
  ptExt[3] = ext[permutation[1]*2 + 1] - numGhostLevels;
  
  if (ptExt[0] == ptExt[1] || ptExt[2] == ptExt[3])
    {
    return 0;
    }
  
  // Face point increments.
  incs[0] = 1;
  incs[1] = (ext[1]-ext[0]+1);
  incs[2] = (ext[1]-ext[0]+1) * incs[1]; 
  inPIncs[1] = incs[permutation[0]];
  inPIncs[2] = incs[permutation[1]];
  // Calculate the starting offset from min corner of the block.
  if (face == 1 || face == 4 || face == 16)
    { // max face.
    ptExt[4] = ptExt[5] = ext[permutation[2]*2+1] - numGhostLevels;
    inPIncs[0] = numGhostLevels * (inPIncs[1] + inPIncs[2]) 
           + (ptExt[4]-ext[permutation[2]*2])*incs[permutation[2]];
    }
  else
    {
    ptExt[4] = ptExt[5] = ext[permutation[2]*2];
    inPIncs[0] = numGhostLevels * (inPIncs[1] + inPIncs[2]);
    if (ext[permutation[2]*2] < ext[permutation[2]*2+1])
      {
      ptExt[4] += numGhostLevels;
      ptExt[5] += numGhostLevels;
      inPIncs[0] += numGhostLevels * incs[permutation[2]];
      }
    }

  // Face cell increments. Max 2D face cell gets the lower 3D cell.
  incs[0] = 1;
  incs[1] = (ext[1]-ext[0]);
  incs[2] = (ext[1]-ext[0]) * incs[1]; 
  inCIncs[1] = incs[permutation[0]];
  inCIncs[2] = incs[permutation[1]];
  // Calculate the starting offset from min corner of the block.
  if (face == 1 || face == 4 || face == 16)
    { // max face.
    inCIncs[0] = numGhostLevels * (inCIncs[1] + inCIncs[2]) 
        + (ptExt[4]-ext[permutation[2]*2]-1)*incs[permutation[2]];
    }
  else
    {
    inCIncs[0] = numGhostLevels * (inCIncs[1] + inCIncs[2]);
    // This is sort of a nasty way to handle ghost cells with 2d data.
    if (ext[permutation[2]*2] < ext[permutation[2]*2+1])
      {
      inCIncs[0] += numGhostLevels * incs[permutation[2]];
      }
    }

  return 1;
}




//----------------------------------------------------------------------------
void vtkCTHAMRSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PositiveXVisibility: " << this->PositiveXVisibility << endl;
  os << indent << "NegativeXVisibility: " << this->NegativeXVisibility << endl;
  os << indent << "PositiveYVisibility: " << this->PositiveYVisibility << endl;
  os << indent << "NegativeYVisibility: " << this->NegativeYVisibility << endl;
  os << indent << "PositiveZVisibility: " << this->PositiveZVisibility << endl;
  os << indent << "NegativeZVisibility: " << this->NegativeZVisibility << endl;
}

