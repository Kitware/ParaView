/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRSurface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHAMRSurface - Create a surface from the input.
// .SECTION Description
// vtkCTHAMRSurface looks at blocks across all processes, and creates
// surfaces from block faces that are not shared.  
// For now, block faces cannot be partially visible.  It is an all or
// nothing decision.  I have added the feature to make some faces invisible.
// The user can specify the visiblity of +x,-x, +y,-y, +z-z faces
// separately.

#ifndef __vtkCTHAMRSurface_h
#define __vtkCTHAMRSurface_h

#include "vtkCTHDataToPolyDataFilter.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHAMRSurface : public vtkCTHDataToPolyDataFilter
{
public:
  static vtkCTHAMRSurface *New();

  // Description;
  // All visibility flags are on by default.  Block faces are clssified
  // into six categories (+z:top, -z:bottom, +x:right, -x:left, +y:front and -y:back)
  // visbility of these groups can be set separately.
  vtkSetMacro(PositiveXVisibility, int);
  vtkGetMacro(PositiveXVisibility, int);
  vtkSetMacro(NegativeXVisibility, int);
  vtkGetMacro(NegativeXVisibility, int);
  vtkSetMacro(PositiveYVisibility, int);
  vtkGetMacro(PositiveYVisibility, int);
  vtkSetMacro(NegativeYVisibility, int);
  vtkGetMacro(NegativeYVisibility, int);
  vtkSetMacro(PositiveZVisibility, int);
  vtkGetMacro(PositiveZVisibility, int);
  vtkSetMacro(NegativeZVisibility, int);
  vtkGetMacro(NegativeZVisibility, int);

  vtkTypeRevisionMacro(vtkCTHAMRSurface,vtkCTHDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCTHAMRSurface();
  ~vtkCTHAMRSurface();

  virtual void Execute();
  int FindFace(vtkCTHData* input, int blockId, int face, 
               int ptExt[4], int pIncs[3], int cIncs[3], 
               int permutation[2]);
  void MakeFace(vtkCTHData *input, int blockId, int face,
                vtkIdType inputCellOffset, vtkIdType inputPointOffset,
                vtkPolyData *output);
  void CreateOutput(vtkCTHData* input, vtkPolyData* output, 
                    int numBlocks, int* visibleBlockFaces);
  int CheckFaceVisibility(int currentLevel, int* faceExt, int numProcs,
                          int* procNumBlocks, int** procBlock7Info,
                          int currentProc, int currentBlock,
                          int numGhostLevels);
  void ComputeOutsideFacesFromInformation(int numGhostLevels, int numProcs, 
                            int* procNumBlocks, int** procBlock7Info, 
                            int** procBlock1VisibleFaces);
  void GetBlockInformation(vtkCTHData* input, int* blockInfo);
  void GetInputBlockCellExtentWithoutGhostLevels(vtkCTHData* input, 
                                                 int blockId, int* ext);

  int PositiveXVisibility;
  int NegativeXVisibility;
  int PositiveYVisibility;
  int NegativeYVisibility;
  int PositiveZVisibility;
  int NegativeZVisibility;

private:
  void InternalImageDataCopy(vtkCTHAMRSurface *src);

private:
  vtkCTHAMRSurface(const vtkCTHAMRSurface&);  // Not implemented.
  void operator=(const vtkCTHAMRSurface&);  // Not implemented.
};


#endif



