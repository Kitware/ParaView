/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRDualClip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRDualClip - Clip (with scalars) an AMR volume to unstructured grid.
// .SECTION Description
// This filter clips an AMR volume but does not copy attributes yet.
// This filter has two important features.  First is that the level
// transitions are handled correctly, and second is that interal
// cells are decimated.  I use a variation of degenerate points/cells
// used for level transitions.

#ifndef __vtkAMRDualClip_h
#define __vtkAMRDualClip_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataSet;
class vtkImageData;
class vtkUnstructuredGrid;
class vtkHierarchicalBoxDataSet;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkDoubleArray;
class vtkCellArray;
class vtkCellData;
class vtkIntArray;
class vtkMultiProcessController;
class vtkDataArraySelection;
class vtkCallbackCommand;

class vtkAMRDualGridHelper;
class vtkAMRDualGridHelperBlock;
class vtkAMRDualGridHelperFace;
class vtkAMRDualClipLocator;


class VTK_EXPORT vtkAMRDualClip : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRDualClip *New();
  vtkTypeRevisionMacro(vtkAMRDualClip,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(IsoValue, double);
  vtkGetMacro(IsoValue, double);

  // Description:
  // These are to evaluate performances. You can turn off degenerate cells
  // and multiprocess comunication to see how they affect speed of execution.
  // Degenerate cells is the meshing between levels in the grid.
  vtkSetMacro(EnableDegenerateCells,int);
  vtkGetMacro(EnableDegenerateCells,int);
  vtkBooleanMacro(EnableDegenerateCells,int);
  vtkSetMacro(EnableMultiProcessCommunication,int);
  vtkGetMacro(EnableMultiProcessCommunication,int);
  vtkBooleanMacro(EnableMultiProcessCommunication,int);

  // Description:
  // This flag causes blocks to share locators so there are no
  // boundary edges between blocks. It does not eliminate
  // boundary edges between processes.
  vtkSetMacro(EnableMergePoints,int);
  vtkGetMacro(EnableMergePoints,int);
  vtkBooleanMacro(EnableMergePoints,int);


protected:
  vtkAMRDualClip();
  ~vtkAMRDualClip();

  double IsoValue;

  // Algorithm options that may improve performance.
  int EnableDegenerateCells;
  int EnableMultiProcessCommunication;
  int EnableMergePoints;

  //BTX
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  void ShareBlockLocatorWithNeighbors(
    vtkAMRDualGridHelperBlock* block);

  void ProcessBlock(vtkAMRDualGridHelperBlock* block, int blockId);
  
  void ProcessDualCell(
    vtkAMRDualGridHelperBlock* block, int blockId,
    int marchingCase,
    int x, int y, int z,
    double values[8]);

  void InitializeLevelMask(vtkAMRDualGridHelperBlock* block);
  void ShareLevelMask(vtkAMRDualGridHelperBlock* block);


  //void DebugCases();
  //void PermuteCases();
  //void MirrorCases();
  //void AddGlyph(double x, double y, double z);
  
  // Stuff exclusively for debugging.
  vtkIntArray* BlockIdCellArray;
  vtkUnsignedCharArray* LevelMaskPointArray;

  // Ivars used to reduce method parrameters.
  vtkAMRDualGridHelper* Helper;
  vtkPoints* Points;
  vtkCellArray* Cells;

  vtkMultiProcessController *Controller;

  // I made these ivars to avoid allocating multiple times.
  // The buffer is not used too many times, but .....
  int* MessageBuffer;
  int* MessageBufferLength;

  vtkAMRDualClipLocator* BlockLocator;

private:
  vtkAMRDualClip(const vtkAMRDualClip&);  // Not implemented.
  void operator=(const vtkAMRDualClip&);  // Not implemented.
  //ETX
};

#endif
