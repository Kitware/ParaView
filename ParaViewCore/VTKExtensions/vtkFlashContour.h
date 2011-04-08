/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlashContour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFlashContour - Contour of a flash AMR volume.
// .SECTION Description
// This filter takes a cell data array and generates a polydata
// surface.  


#ifndef __vtkFlashContour_h
#define __vtkFlashContour_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkImageData;
class vtkPoints;
class vtkCellArray;
class vtkUnsignedCharArray;
class vtkPolyData;
class vtkDoubleArray;
class vtkIntArray;

class VTK_EXPORT vtkFlashContour : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFlashContour *New();
  vtkTypeMacro(vtkFlashContour,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(IsoValue, double);
  vtkGetMacro(IsoValue, double);

  vtkSetStringMacro(PassAttribute);
  vtkGetStringMacro(PassAttribute);

protected:
  vtkFlashContour();
  ~vtkFlashContour();

  double IsoValue;
  char* PassAttribute;
  vtkDoubleArray* PassArray;

  // Just for debugging.
  vtkIntArray *BlockIdCellArray;
  int CurrentBlockId;
  // A couple cell arrays to help determine where I should refine.
  vtkUnsignedCharArray *LevelCellArray;
  unsigned char CurrentLevel;
  // Instead of maximum depth, compute the different between the 
  // maximum depth and the current depth.
  vtkUnsignedCharArray *RemainingDepthCellArray;
  unsigned char RemainingDepth;
  unsigned char ComputeBranchDepth(int globalBlockId);

  vtkPoints *Points;
  vtkCellArray *Faces;
  vtkPolyData *Mesh;

  char* CellArrayNameToProcess;
  vtkSetStringMacro(CellArrayNameToProcess);

  //BTX
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  void PropogateNeighbors(int neighbors[3][3][3], int x, int y, int z);

  // Save some ivars to reduce arguments to recursive methods.
  int  NumberOfGlobalBlocks;
  int* GlobalLevelArray;
  int* GlobalChildrenArray;
  int* GlobalNeighborArray;
  int* GlobalToLocalMap;

  void RecurseTree(int neighborhood[3][3][3], vtkMultiBlockDataSet* input);
  void ProcessBlock(vtkImageData* block);
  void ProcessCell(const double *origin, const double *spacing, 
                   const double *cornerValues, const double *passValues);
  void ProcessNeighborhoodSharedRegion(
    int neighborhood[3][3][3], 
    int r[3], 
    vtkMultiBlockDataSet *input);
  void ProcessSharedRegion(
    int regionDims[3],
    double* cornerPtrs[8], int incs[3],
    double cornerPoints[32], double cornerSpacings[32], 
    int cornerLevelDiffs[8],
    double* passPtrs[8]);
  void ProcessDegenerateCell(
    double  cornerPoints[32], 
    double* cornerPtrs[8],
    double* passPtrs[8]);
  void ProcessCellFinal(
    const double cornerPoints[32], 
    const double cornerValues[8],
    int          cubeCase,
    const double passValues[8]);


private:
  vtkFlashContour(const vtkFlashContour&);  // Not implemented.
  void operator=(const vtkFlashContour&);  // Not implemented.
  //ETX
};

#endif
