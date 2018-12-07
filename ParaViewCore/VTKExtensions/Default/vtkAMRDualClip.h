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
/**
 * @class   vtkAMRDualClip
 * @brief   Clip (with scalars) an AMR volume to unstructured grid.
 *
 * This filter clips an AMR volume but does not copy attributes yet.
 * This filter has two important features.  First is that the level
 * transitions are handled correctly, and second is that internal
 * cells are decimated.  I use a variation of degenerate points/cells
 * used for level transitions.
*/

#ifndef vtkAMRDualClip_h
#define vtkAMRDualClip_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataSet;
class vtkImageData;
class vtkUnstructuredGrid;
class vtkNonOverlappingAMR;
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

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAMRDualClip : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRDualClip* New();
  vtkTypeMacro(vtkAMRDualClip, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(IsoValue, double);
  vtkGetMacro(IsoValue, double);

  //@{
  /**
   * These are to evaluate performances. You can turn off degenerate cells
   * and multiprocess communication to see how they affect speed of execution.
   * Degenerate cells is the meshing between levels in the grid.
   */
  vtkSetMacro(EnableInternalDecimation, int);
  vtkGetMacro(EnableInternalDecimation, int);
  vtkSetMacro(EnableDegenerateCells, int);
  vtkGetMacro(EnableDegenerateCells, int);
  vtkBooleanMacro(EnableDegenerateCells, int);
  vtkSetMacro(EnableMultiProcessCommunication, int);
  vtkGetMacro(EnableMultiProcessCommunication, int);
  vtkBooleanMacro(EnableMultiProcessCommunication, int);
  //@}

  //@{
  /**
   * This flag causes blocks to share locators so there are no
   * boundary edges between blocks. It does not eliminate
   * boundary edges between processes.
   */
  vtkSetMacro(EnableMergePoints, int);
  vtkGetMacro(EnableMergePoints, int);
  vtkBooleanMacro(EnableMergePoints, int);
  //@}

  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);

protected:
  vtkAMRDualClip();
  ~vtkAMRDualClip() override;

  double IsoValue;

  // Algorithm options that may improve performance.
  int EnableInternalDecimation;
  int EnableDegenerateCells;
  int EnableMultiProcessCommunication;
  int EnableMergePoints;

  // Needed for copying cell data to point data.
  vtkUnstructuredGrid* Mesh;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void InitializeCopyAttributes(vtkNonOverlappingAMR* hbdsInput, vtkDataSet* mesh);

  /**
   * Not a pipeline function. This is a helper function that
   * allows creating a new data set given a input and a cell array name.
   */
  vtkMultiBlockDataSet* DoRequestData(vtkNonOverlappingAMR* input, const char* arrayNameToProcess);

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  void ShareBlockLocatorWithNeighbors(vtkAMRDualGridHelperBlock* block);

  void ProcessBlock(vtkAMRDualGridHelperBlock* block, int blockId, const char* arrayName);

  void ProcessDualCell(vtkAMRDualGridHelperBlock* block, int blockId, int x, int y, int z,
    vtkIdType cornerOffsets[8], vtkDataArray* volumeFractionArray);

  void InitializeLevelMask(vtkAMRDualGridHelperBlock* block);
  void ShareLevelMask(vtkAMRDualGridHelperBlock* block);
  void DistributeLevelMasks();

  // void DebugCases();
  // void PermuteCases();
  // void MirrorCases();
  // void AddGlyph(double x, double y, double z);

  // Stuff exclusively for debugging.
  vtkIntArray* BlockIdCellArray;
  vtkUnsignedCharArray* LevelMaskPointArray;

  // Ivars used to reduce method parrameters.
  vtkAMRDualGridHelper* Helper;
  vtkPoints* Points;
  vtkCellArray* Cells;

  vtkMultiProcessController* Controller;

  // I made these ivars to avoid allocating multiple times.
  // The buffer is not used too many times, but .....
  int* MessageBuffer;
  int* MessageBufferLength;

  vtkAMRDualClipLocator* BlockLocator;

private:
  vtkAMRDualClip(const vtkAMRDualClip&) = delete;
  void operator=(const vtkAMRDualClip&) = delete;
};

#endif
