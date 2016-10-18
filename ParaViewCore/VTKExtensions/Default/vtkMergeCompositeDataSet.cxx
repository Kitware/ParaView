/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeCompositeDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeCompositeDataSet.h"

#include "vtkCellArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkMergeCompositeDataSet);

//-----------------------------------------------------------------------------

vtkMergeCompositeDataSet::vtkMergeCompositeDataSet()
{
}

vtkMergeCompositeDataSet::~vtkMergeCompositeDataSet()
{
}

void vtkMergeCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

int vtkMergeCompositeDataSet::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
template <class IT, class OT>
void vtkDeepCopyArrayOfDifferentType(
  IT* input, OT* output, vtkIdType id, vtkIdType numTuples, vtkIdType nComp)
{
  output += id * nComp;
  vtkIdType i = numTuples * nComp;
  while (i--)
  {
    output[i] = static_cast<OT>(input[i]);
  }
}

//----------------------------------------------------------------------------
template <class IT>
void vtkDeepCopySwitchOnOutput(
  IT* input, vtkDataArray* da, vtkIdType id, vtkIdType numTuples, vtkIdType nComp)
{
  void* output = da->GetVoidPointer(0);

  switch (da->GetDataType())
  {
    vtkTemplateMacro(
      vtkDeepCopyArrayOfDifferentType(input, static_cast<VTK_TT*>(output), id, numTuples, nComp));
    default:
      vtkGenericWarningMacro("Unsupported data type " << da->GetDataType() << "!");
  }
}

//-----------------------------------------------------------------------------

int vtkMergeCompositeDataSet::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the info objects.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input->IsA("vtkPointSet") || input->IsA("vtkGraph"))
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkCompositeDataSet* cdsInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints* points = vtkPoints::New();
  vtkDataArray* pointsArray = points->GetData();
  vtkPointData* pointData = output->GetPointData();

  int curId = 0;
  vtkCompositeDataIterator* iter = cdsInput->NewIterator();
  bool uninitialized = true;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkPointSet* ps = vtkPointSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ps && ps->GetNumberOfPoints())
    {
      vtkDataArray* psData = ps->GetPoints()->GetData();
      if (uninitialized)
      {
        points->SetDataType(ps->GetPoints()->GetDataType());
        pointsArray->SetNumberOfComponents(psData->GetNumberOfComponents());
        points->SetNumberOfPoints(cdsInput->GetNumberOfPoints());
        pointData->SetNumberOfTuples(cdsInput->GetNumberOfPoints());
        uninitialized = false;
      }
      else
      {
        if (pointsArray->GetNumberOfComponents() != psData->GetNumberOfComponents())
        {
          vtkErrorMacro(
            "Incompatible number of components " << psData->GetNumberOfComponents() << "!");
          continue;
        }
      }

      vtkIdType psNumberOfPoints = ps->GetNumberOfPoints();
      switch (ps->GetPoints()->GetDataType())
      {
        vtkTemplateMacro(
          vtkDeepCopySwitchOnOutput(static_cast<VTK_TT*>(ps->GetPoints()->GetVoidPointer(0)),
            pointsArray, curId, psData->GetNumberOfTuples(), psData->GetNumberOfComponents()));
        default:
          vtkErrorMacro("Unsupported data type " << ps->GetPoints()->GetDataType() << "!");
          continue;
      }
      vtkDataSetAttributes* inputPtData = ps->GetPointData();
      /*
      cout << "NB point array: " << inputPtData->GetNumberOfArrays() << endl;
      for( int i= 0; i < inputPtData->GetNumberOfArrays() ; ++i)
        {
        cout << "    " << inputPtData->GetArrayName(i) << endl;
        }
      */
      /*
      pointData->CopyAllocate(inputPtData, psNumberOfPoints);
      for(int i = 0; i < psNumberOfPoints; ++i)
        {
        pointData->CopyData(inputPtData, i, curId + i);
        }
      */
      for (int i = 0; i < inputPtData->GetNumberOfArrays(); ++i)
      {
        if (pointData->GetAbstractArray(inputPtData->GetArrayName(i)) == NULL)
        {
          vtkAbstractArray* data = inputPtData->GetAbstractArray(i)->NewInstance();
          data->SetNumberOfComponents(inputPtData->GetAbstractArray(i)->GetNumberOfComponents());
          data->SetName(inputPtData->GetAbstractArray(i)->GetName());
          if (inputPtData->GetAbstractArray(i)->HasInformation())
          {
            data->CopyInformation(inputPtData->GetAbstractArray(i)->GetInformation(), /*deep=*/1);
          }
          pointData->AddArray(data);
          data->Delete();
          data->SetNumberOfTuples(cdsInput->GetNumberOfPoints());
          for (int j = 0; j < curId; ++j)
          {
            // data->SetTuple(i, emptyTuple);
            memset(data->GetVoidPointer(j), 0,
              data->GetNumberOfComponents() * data->GetElementComponentSize());
          }
        }
      }
      // pointData->CopyStructure(inputPtData);

      for (int k = 0; k < pointData->GetNumberOfArrays(); k++)
      {
        vtkAbstractArray* outputArray = pointData->GetArray(k);
        vtkAbstractArray* inputPtArray = inputPtData->GetAbstractArray(pointData->GetArrayName(k));

        if (inputPtArray)
        {
          for (int i = 0; i < inputPtData->GetNumberOfTuples(); ++i)
          {
            // pointData->InsertNextTuple(i, inputPtData);
            // pointData->SetTuple(i+curId, i, inputPtData);
            outputArray->SetTuple(i + curId, i, inputPtArray);
          }
        }
        else
        {
          for (int i = 0; i < inputPtData->GetNumberOfTuples(); ++i)
          {
            memset(outputArray->GetVoidPointer(i), 0,
              outputArray->GetNumberOfComponents() * outputArray->GetElementComponentSize());
          }
        }
      }
      curId += psNumberOfPoints;
    }
  }
  iter->Delete();
  output->SetPoints(points);
  points->Delete();
  vtkIdType numPoints = points->GetNumberOfPoints();

  VTK_CREATE(vtkCellArray, cells);
  cells->Allocate(2 * numPoints);

  for (vtkIdType i = 0; i < numPoints; i++)
  {
    cells->InsertNextCell(1, &i);
  }
  output->SetVerts(cells);
  /*
    cout << "res NB point array: " << pointData->GetNumberOfArrays() << endl;
    for( int i= 0; i < pointData->GetNumberOfArrays() ; ++i)
      {
      cout << "    " << pointData->GetArrayName(i) << endl;
      }
  */
  return 1;
}
