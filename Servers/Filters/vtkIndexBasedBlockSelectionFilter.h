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

class vtkMultiProcessController;

class VTK_EXPORT vtkIndexBasedBlockSelectionFilter : public vtkSelectionAlgorithm
{
public:
  static vtkIndexBasedBlockSelectionFilter* New();
  vtkTypeRevisionMacro(vtkIndexBasedBlockSelectionFilter, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

   // Description:
  // Get/Set the number of indices that fit within one block.
  vtkSetMacro(BlockSize, vtkIdType);
  vtkGetMacro(BlockSize, vtkIdType);

  // Description:
  // Get/Set the block to fetch.
  vtkSetMacro(Block, vtkIdType);
  vtkGetMacro(Block, vtkIdType);

  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Set the field type to pass. This filter can only deal with one field type
  // at a time i.e. either cell data, or point data or field data.
  // Default is POINT_DATA_FIELD.
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
  void SetFieldTypeToPoint()
    { this->SetFieldType(POINT); }
  void SetFieldTypeToCell()
    { this->SetFieldType(CELL); }
  void SetFieldTypeToField()
    { this->SetFieldType(FIELD); }

  //BTX
  enum FieldDataType
    {
    CELL=0,
    POINT=1,
    FIELD=2
    };
  //ETX 

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

  bool DetermineBlockIndices();

  vtkMultiProcessController* Controller;
  vtkIdType BlockSize;
  vtkIdType Block;

  int FieldType;
  vtkIdType StartIndex;
  vtkIdType EndIndex;
private:
  vtkIndexBasedBlockSelectionFilter(const vtkIndexBasedBlockSelectionFilter&); // Not implemented
  void operator=(const vtkIndexBasedBlockSelectionFilter&); // Not implemented
//ETX
};

#endif

