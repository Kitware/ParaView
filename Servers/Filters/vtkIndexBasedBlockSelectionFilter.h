/*=========================================================================

  Program:   ParaView
  Module:    vtkIndexBasedBlockSelectionFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIndexBasedBlockSelectionFilter
// .SECTION Description
// vtkIndexBasedBlockSelectionFilter is a selection algorithm that takes in a
// vtkSelection and produces a vtkSelection on the output. It is designed to be
// used in conjunction with vtkIndexBasedBlockFilter to generate a vtkSelection
// corresponding to the block produced by the vtkIndexBasedBlockFilter. This
// filter can only work on INDEX based selections.

#ifndef __vtkIndexBasedBlockSelectionFilter_h
#define __vtkIndexBasedBlockSelectionFilter_h

#include "vtkSelectionAlgorithm.h"
//BTX
#include "vtkSmartPointer.h" // needed for vtkSmartPointer
//ETX
class vtkIndexBasedBlockFilter;
class vtkMultiPieceDataSet;
class vtkMultiProcessController;

class VTK_EXPORT vtkIndexBasedBlockSelectionFilter : public vtkSelectionAlgorithm
{
public:
  static vtkIndexBasedBlockSelectionFilter* New();
  vtkTypeRevisionMacro(vtkIndexBasedBlockSelectionFilter, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

   // Description:
  // Get/Set the number of indices that fit within one block.
  void SetBlockSize(vtkIdType);
  vtkIdType GetBlockSize();

  // Description:
  // Get/Set the block to fetch.
  void SetBlock(vtkIdType);
  vtkIdType GetBlock();

  // Description:
  // In case of Composite datasets, set the flat index of the dataset to pass.
  // The flat index must point to a non-empty, non-composite dataset or a
  // non-empty multipiece dataset for anything to be passed through. 
  // If the input is not a composite dataset, then this index is ignored.
  // If FieldType is FIELD then the CompositeDataSetIndex cannot be
  // vtkMultiPieceDataSet, it has to be a vtkDataSet.
  void SetCompositeDataSetIndex(unsigned int);
  unsigned int GetCompositeDataSetIndex();
  
  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Set the field type to pass. This filter can only deal with one field type
  // at a time i.e. either cell data, or point data or field data.
  // Default is POINT_DATA_FIELD.
  void SetFieldType(int);
  int GetFieldType();

//BTX
protected:
  vtkIndexBasedBlockSelectionFilter();
  ~vtkIndexBasedBlockSelectionFilter();
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // This is called within ProcessRequest when a request asks the algorithm
  // to do its work. This is the method you should override to do whatever the
  // algorithm is designed to do. This happens during the fourth pass in the
  // pipeline execution process.
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);

  int RequestDataInternal(
    vtkIdType startIndex, vtkIdType endIndex,
    vtkSelection* input,
    vtkSelection* output);


  int RequestDataInternal(vtkSelection* input,
    vtkSelection* output, vtkMultiPieceDataSet* pieces);

  bool DetermineBlockIndices(vtkMultiPieceDataSet* input);
  vtkSelection* LocateSelection(unsigned int composite_index, vtkSelection* sel, 
    vtkDataObject* inputDO);

  vtkIndexBasedBlockFilter* BlockFilter;

  vtkMultiProcessController* Controller;
  vtkIdType StartIndex;
  vtkIdType EndIndex;

  vtkSmartPointer<vtkSelection> Temporary;
private:
  vtkIndexBasedBlockSelectionFilter(const vtkIndexBasedBlockSelectionFilter&); // Not implemented
  void operator=(const vtkIndexBasedBlockSelectionFilter&); // Not implemented

//ETX
};

#endif

