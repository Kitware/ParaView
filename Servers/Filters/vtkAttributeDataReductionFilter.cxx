/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataReductionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAttributeDataReductionFilter.h"

#include "vtkObjectFactory.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkAttributeDataReductionFilter);
vtkCxxRevisionMacro(vtkAttributeDataReductionFilter, "1.7");
//-----------------------------------------------------------------------------
vtkAttributeDataReductionFilter::vtkAttributeDataReductionFilter()
{
  this->ReductionType = vtkAttributeDataReductionFilter::ADD;
  this->AttributeType = vtkAttributeDataReductionFilter::POINT_DATA |
    vtkAttributeDataReductionFilter::CELL_DATA;
}

//-----------------------------------------------------------------------------
vtkAttributeDataReductionFilter::~vtkAttributeDataReductionFilter()
{
}

//----------------------------------------------------------------------------
int vtkAttributeDataReductionFilter::FillInputPortInformation(
  int port, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return this->Superclass::FillInputPortInformation(port, info);
}

//-----------------------------------------------------------------------------
template<class iterT>
void vtkAttributeDataReductionFilterReduce(vtkAttributeDataReductionFilter* self,
  iterT* toIter, iterT* fromIter, double progress_offset, double progress_factor)
{
  int mode = self->GetReductionType();
  vtkIdType numValues = toIter->GetNumberOfValues();
  numValues = fromIter->GetNumberOfValues() < numValues ? 
    fromIter->GetNumberOfValues() : numValues;
  for (vtkIdType cc=0; cc < numValues; ++cc)
    {
    typename iterT::ValueType result = toIter->GetValue(cc);
    switch (mode)
      {
    case vtkAttributeDataReductionFilter::ADD:
      result = result + fromIter->GetValue(cc);
      break;

    case vtkAttributeDataReductionFilter::MAX:
        {
        typename iterT::ValueType v2 = fromIter->GetValue(cc);
        result = (result > v2)? result : v2;
        }
      break;

    case vtkAttributeDataReductionFilter::MIN:
        {
        typename iterT::ValueType v2 = fromIter->GetValue(cc);
        result = (result > v2)? result : v2;
        }
      break;
      }
    toIter->SetValue(cc, result);
    self->UpdateProgress(progress_offset + progress_factor * cc/numValues);
    }
}

//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void vtkAttributeDataReductionFilterReduce(vtkAttributeDataReductionFilter*,
  vtkArrayIteratorTemplate<vtkStdString>* , 
  vtkArrayIteratorTemplate<vtkStdString>*, double, double)
{
  // Cannot reduce strings.
}

//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void vtkAttributeDataReductionFilterReduce(vtkAttributeDataReductionFilter*,
  vtkBitArrayIterator* , vtkBitArrayIterator*, double, double)
{
  // Cannot reduce bit arrays.
}

//-----------------------------------------------------------------------------
int vtkAttributeDataReductionFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int cc;
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs <= 0)
    {
    output->Initialize();
    return 0;
    }

  vtkDataSet* input0 = vtkDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(
      vtkDataObject::DATA_OBJECT()));
  
  // Copy output structure from input0.
  output->CopyStructure(input0);

  // Determine which point/cell arrays are common to all inputs,
  // only those get reduced in the output.
  vtkDataSetAttributes::FieldList ptList(numInputs);
  vtkDataSetAttributes::FieldList cellList(numInputs);
  ptList.InitializeFieldList(input0->GetPointData());
  cellList.InitializeFieldList(input0->GetCellData());

  vtkIdType numPoints0 = input0->GetNumberOfPoints();
  vtkIdType numCells0 = input0->GetNumberOfCells();
  for (cc=1; cc < numInputs; ++cc)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(cc);
    if (inInfo)
      {
      vtkDataSet* inputCC = vtkDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (inputCC)
        {
        if (inputCC->GetNumberOfPoints() != numPoints0)
          {
          vtkErrorMacro(
            "Input " << cc << " num. of points does not match with input 0.");
          continue;
          }
        if (inputCC->GetNumberOfCells() != numCells0)
          {
          vtkErrorMacro(
            "Input " << cc << " num. of cells does not match with input 0.");
          continue;
          }
        // Include only field that have any arrays
        if ( inputCC->GetPointData()->GetNumberOfArrays() > 0 )
          {
          ptList.IntersectFieldList(inputCC->GetPointData());
          }
        if ( inputCC->GetCellData()->GetNumberOfArrays() > 0 )
          {
          cellList.IntersectFieldList(inputCC->GetCellData());
          }
        }
      }
    }

  vtkIdType idx;
  vtkPointData* input0PD = input0->GetPointData();
  vtkPointData *outputPD = output->GetPointData();

  if ( (this->AttributeType & vtkAttributeDataReductionFilter::POINT_DATA) == 0)
    {
    outputPD->ShallowCopy(input0PD);
    }
  else
    {
    outputPD->CopyGlobalIdsOn();
    outputPD->CopyAllocate(ptList,numPoints0);
    for (idx=0; idx < numPoints0; idx++)
      {
      // Copy 0th point data first.
      outputPD->CopyData(ptList, input0PD, 0, idx, idx);
      }
    }

  vtkCellData* input0CD = input0->GetCellData();
  vtkCellData *outputCD = output->GetCellData();
  if ((this->AttributeType & vtkAttributeDataReductionFilter::CELL_DATA) == 0)
    {
    outputCD->ShallowCopy(input0CD);
    }
  else
    {
    outputCD->CopyGlobalIdsOn();
    outputCD->CopyAllocate(cellList,numCells0);

    for (idx=0; idx< numCells0; ++idx)
      {
      outputCD->CopyData(cellList, input0CD, 0, idx, idx);
      }
    }

  this->UpdateProgress(0.1);
  double progress_offset = 0.1;
  double progress_factor = (numInputs > 1)? (0.9*0.5/(numInputs-1)) : 0;

  for (cc=1; cc < numInputs; ++cc)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(cc);
    if (!inInfo)
      {
      continue;
      }
    vtkDataSet* inputCC = vtkDataSet::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!inputCC)
      {
      continue;
      }

    int i;

    vtkPointData* inPD = inputCC->GetPointData();
    // Ignore if no arrays
    if (inPD->GetNumberOfArrays() > 0)
      {
      // Now combine this inPD with the outPD using the reduction indicated.
      if ((this->AttributeType & POINT_DATA) != 0)
        {
        for (i=0; i < ptList.GetNumberOfFields(); ++i)
          {
          if (ptList.GetFieldIndex(i) >=0)
            {
            vtkDataArray* toDA = outputPD->GetArray(ptList.GetFieldIndex(i));
            vtkDataArray* fromDA = inPD->GetArray(ptList.GetDSAIndex(cc, i));
            if (!toDA || !fromDA)
              {
              continue;
              }
            vtkSmartPointer<vtkArrayIterator> toIter;
            toIter.TakeReference(toDA->NewIterator());
            vtkSmartPointer<vtkArrayIterator> fromIter;
            fromIter.TakeReference(fromDA->NewIterator());
  
            switch (toDA->GetDataType())
              {
              vtkArrayIteratorTemplateMacro(
                vtkAttributeDataReductionFilterReduce(this,
                  static_cast<VTK_TT*>(toIter.GetPointer()), 
                  static_cast<VTK_TT*>(fromIter.GetPointer()), 
                  progress_offset, progress_factor));
            default:
              vtkWarningMacro("Cannot reduce arrays of type: " <<
                toDA->GetDataTypeAsString());
              }
            }
          }
        }
      }

    // Process cell data.
    vtkCellData* inCD = inputCC->GetCellData();
    // Ignore if no cell data
    if (inCD->GetNumberOfArrays() > 0)
      {
      progress_offset += progress_factor;
  
      if ((this->AttributeType & CELL_DATA) != 0)
        {
        // Now combine this inCD with the outCD using the reduction indicated.
        for (i=0; i < cellList.GetNumberOfFields(); ++i)
          {
          if (cellList.GetFieldIndex(i) >=0)
            {
            vtkDataArray* toDA = outputCD->GetArray(cellList.GetFieldIndex(i));
            vtkDataArray* fromDA = inCD->GetArray(cellList.GetDSAIndex(cc, i));
            if (!toDA || !fromDA)
              {
              continue;
              }
            vtkSmartPointer<vtkArrayIterator> toIter;
            toIter.TakeReference(toDA->NewIterator());
            vtkSmartPointer<vtkArrayIterator> fromIter;
            fromIter.TakeReference(fromDA->NewIterator());
  
            switch (toDA->GetDataType())
              {
              vtkArrayIteratorTemplateMacro(
                vtkAttributeDataReductionFilterReduce(this,
                  static_cast<VTK_TT*>(toIter.GetPointer()), 
                  static_cast<VTK_TT*>(fromIter.GetPointer()),
                  progress_offset, progress_factor));
            default:
              vtkWarningMacro("Cannot reduce arrays of type: " <<
                toDA->GetDataTypeAsString());
              }
            }
          }
        }
      progress_offset += progress_factor;
      }
    }

  return 1; 
}

//-----------------------------------------------------------------------------
void vtkAttributeDataReductionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ReductionType: " << this->GetReductionTypeAsString() 
    << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
