/*=========================================================================

  Program:   ParaView
  Module:    vtkIndexBasedBlockFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIndexBasedBlockFilter - Filter that convert a field attributes of 
// a dataobject to a vtkTable, passing only the selected block of attributes.
// .SECTION Description
// vtkIndexBasedBlockFilter is used to pass a vtkTable of field data associated
// with a dataobject/dataset for a selected block. 

#ifndef __vtkIndexBasedBlockFilter_h
#define __vtkIndexBasedBlockFilter_h

#include "vtkTableAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkIndexBasedBlockFilter : public vtkTableAlgorithm
{
public:
  static vtkIndexBasedBlockFilter* New();
  vtkTypeRevisionMacro(vtkIndexBasedBlockFilter, vtkTableAlgorithm);
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
  void SetFieldTypeToPointData()
    { this->SetFieldType(POINT_DATA_FIELD); }
  void SetFieldTypeToCellData()
    { this->SetFieldType(CELL_DATA_FIELD); }
  void SetFieldTypeToFieldData()
    { this->SetFieldType(DATA_OBJECT_FIELD); }

  //BTX
  enum FieldDataType
    {
    DATA_OBJECT_FIELD=0,
    POINT_DATA_FIELD=1,
    CELL_DATA_FIELD=2
    };
  //ETX

//BTX
protected:
  vtkIndexBasedBlockFilter();
  ~vtkIndexBasedBlockFilter();

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
  vtkIndexBasedBlockFilter(const vtkIndexBasedBlockFilter&); // Not implemented
  void operator=(const vtkIndexBasedBlockFilter&); // Not implemented
//ETX
};

#endif

