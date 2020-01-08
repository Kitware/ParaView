/*=========================================================================

  Program:   ParaView
  Module:    vtkDataObjectTreeToPointSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectTreeToPointSetFilter.h"

#include "vtkAppendDataSets.h"
#include "vtkAppendFilter.h"   // For unstructured grid output
#include "vtkAppendPolyData.h" // For polydata output
#include "vtkCleanArrays.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkType.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkDataObjectTreeToPointSetFilter);

//----------------------------------------------------------------------------
vtkDataObjectTreeToPointSetFilter::vtkDataObjectTreeToPointSetFilter()
  : SubTreeCompositeIndex(0)
  , MergePoints(true)
  , Tolerance(0.0)
  , ToleranceIsAbsolute(false)
  , OutputDataSetType(VTK_UNSTRUCTURED_GRID)
{
}

//----------------------------------------------------------------------------
vtkDataObjectTreeToPointSetFilter::~vtkDataObjectTreeToPointSetFilter()
{
}

//----------------------------------------------------------------------------
int vtkDataObjectTreeToPointSetFilter::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  if (this->OutputDataSetType != VTK_POLY_DATA && this->OutputDataSetType != VTK_UNSTRUCTURED_GRID)
  {
    vtkErrorMacro(
      "Output type '" << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
                      << "' is not supported.");
    return 0;
  }

  auto input = vtkDataObjectTree::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (input)
  {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output || (vtkDataObjectTypes::GetTypeIdFromClassName(output->GetClassName()) !=
                     this->OutputDataSetType))
    {
      vtkSmartPointer<vtkDataObject> newOutput;
      newOutput.TakeReference(vtkDataObjectTypes::NewDataObject(this->OutputDataSetType));
      info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkDataObjectTreeToPointSetFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // input
  vtkDataSet* ds = vtkDataSet::GetData(inputVector[0], 0);
  vtkDataObjectTree* dot = vtkDataObjectTree::GetData(inputVector[0], 0);

  // output
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  vtkNew<vtkAppendDataSets> appender;
  appender->SetOutputDataSetType(this->OutputDataSetType);
  appender->SetMergePoints(this->MergePoints ? 1 : 0);
  if (this->MergePoints)
  {
    appender->SetTolerance(this->Tolerance);
    appender->SetToleranceIsAbsolute(this->ToleranceIsAbsolute);
  }

  if (ds)
  {
    appender->AddInputData(ds);
  }
  else if (dot)
  {
    if (this->SubTreeCompositeIndex == 0)
    {
      this->ExecuteSubTree(dot, appender);
    }
    vtkDataObjectTreeIterator* iter =
      vtkDataObjectTreeIterator::SafeDownCast(dot->NewTreeIterator());
    iter->VisitOnlyLeavesOff();
    for (iter->InitTraversal();
         !iter->IsDoneWithTraversal() && iter->GetCurrentFlatIndex() <= this->SubTreeCompositeIndex;
         iter->GoToNextItem())
    {
      if (iter->GetCurrentFlatIndex() == this->SubTreeCompositeIndex)
      {
        vtkDataObject* curDO = iter->GetCurrentDataObject();
        vtkDataObjectTree* curDOT = vtkDataObjectTree::SafeDownCast(curDO);
        vtkDataSet* curDS = vtkUnstructuredGrid::SafeDownCast(curDO);
        if (curDS && curDS->GetNumberOfPoints() > 0)
        {
          appender->AddInputData(curDS);
        }
        else if (curDOT)
        {
          this->ExecuteSubTree(curDOT, appender);
        }
        break;
      }
    }
    iter->Delete();
  }

  if (appender->GetNumberOfInputConnections(0) > 0)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
    // this will override field data the vtkAppendFilter passed from the first
    // block. It seems like a reasonable approach, if global field data is
    // present.
    output->GetFieldData()->PassData(dot->GetFieldData());
  }

  this->RemovePartialArrays(output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataObjectTreeToPointSetFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObjectTreeToPointSetFilter::RemovePartialArrays(vtkDataSet* data)
{
  vtkNew<vtkCleanArrays> cleaner;
  cleaner->SetInputData(data);
  cleaner->Update();
  data->ShallowCopy(cleaner->GetOutput());
}

//----------------------------------------------------------------------------
void vtkDataObjectTreeToPointSetFilter::ExecuteSubTree(
  vtkDataObjectTree* dot, vtkAppendDataSets* appender)
{
  vtkDataObjectTreeIterator* iter = dot->NewTreeIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* curDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (curDS)
    {
      appender->AddInputData(curDS);
    }
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkDataObjectTreeToPointSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubTreeCompositeIndex: " << this->SubTreeCompositeIndex << "\n";
  os << indent << "MergePoints: " << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "ToleranceIsAbsolute: " << this->ToleranceIsAbsolute << "\n";
  os << indent
     << "OutputDataSetType: " << vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
     << "\n";
}
