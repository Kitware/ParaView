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
class vtkMultiPieceDataSet;
class vtkIdTypeArray;
class vtkDoubleArray;
class vtkUnsignedIntArray;

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
  // In case of Composite datasets, set the flat index of the dataset to pass.
  // The flat index must point to a non-empty, non-composite dataset or a
  // non-empty multipiece dataset for anything to be passed through. 
  // If the input is not a composite dataset, then this index is ignored.
  // If FieldType is FIELD then the CompositeDataSetIndex cannot be
  // vtkMultiPieceDataSet, it has to be a vtkDataSet.
  vtkSetMacro(CompositeDataSetIndex, unsigned int);
  vtkGetMacro(CompositeDataSetIndex, unsigned int);

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

  // Description: 
  // Get/Set a process number to read the data from.
  // ProcessID is only used when FieldType is FIELD.
  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);

  //BTX
  enum FieldDataType
    {
    CELL=0,
    POINT=1,
    FIELD=2
    };


  // Description:
  // Returns a vtkMultiPieceDataSet with vtkDataSet instances that the
  // CompositeDataSetIndex selected. If the input is not a composite dataset or
  // the index chosen is a vtkDataSet, then this creates a new
  // vtkMultiPieceDataSet and fills it with the single dataset and returns it.
  vtkMultiPieceDataSet* GetPieceToProcess(vtkDataObject*);

  // Description:
  // Computes the range of indices to be passed through.
  bool DetermineBlockIndices(vtkMultiPieceDataSet* input,
    vtkIdType& startIndex, vtkIdType& endIndex);
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

  // Description:
  // Create a default executive.
  // If the DefaultExecutivePrototype is set, a copy of it is created
  // in CreateDefaultExecutive() using NewInstance().
  // Otherwise, vtkStreamingDemandDrivenPipeline is created.
  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Passes the cell/point data in the range [startOffset, endOffset], if
  // possible. Returns the total number of tuples in the point/cell data of the
  // input.
  void PassBlock(
    vtkIdType pieceNumber,
    vtkTable* output, 
    vtkIdType &pieceOffset,
    vtkDataSet* input);

  // Description:
  // Passes the field data in the range [startOffset, endOffset], if
  // possible.
  void PassFieldDataBlock(vtkTable* output, 
    vtkIdType startOffset, vtkIdType endOffset, vtkDataSet* input);

  // Description:
  // Arrays used to put extra information in the output.
  vtkDoubleArray* PointCoordinatesArray;
  vtkIdTypeArray* StructuredCoordinatesArray;
  vtkIdTypeArray* OriginalIndicesArray;
  vtkIdTypeArray* PieceNumberArray;
  vtkUnsignedIntArray* CompositeIndexArray;

  vtkMultiProcessController* Controller;
  vtkIdType BlockSize;
  vtkIdType Block;

  int FieldType;
  vtkIdType StartIndex;
  vtkIdType EndIndex;
  int ProcessID;
  unsigned int CompositeDataSetIndex;
  vtkMultiPieceDataSet* Temporary;

  unsigned int CurrentCIndex;
  unsigned int CurrentHLevel;
  unsigned int CurrentHIndex;
private:
  vtkIndexBasedBlockFilter(const vtkIndexBasedBlockFilter&); // Not implemented
  void operator=(const vtkIndexBasedBlockFilter&); // Not implemented
//ETX
};

#endif

