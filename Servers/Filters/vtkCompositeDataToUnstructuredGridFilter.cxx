/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeDataToUnstructuredGridFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataToUnstructuredGridFilter.h"

#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkCompositeDataToUnstructuredGridFilter);
vtkCxxRevisionMacro(vtkCompositeDataToUnstructuredGridFilter, "1.1");
//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::vtkCompositeDataToUnstructuredGridFilter()
{
  this->SubTreeCompositeIndex = 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataToUnstructuredGridFilter::~vtkCompositeDataToUnstructuredGridFilter()
{
}

//----------------------------------------------------------------------------
int vtkCompositeDataToUnstructuredGridFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkCompositeDataSet* cd = vtkCompositeDataSet::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

  if (ug)
    {
    output->ShallowCopy(ug);
    return 1;
    }

  if (!cd)
    {
    vtkErrorMacro("Input must either be vtkUnstructuredGrid "
      "or vtkCompositeDataSet.");
    return 0;
    }

  if (this->SubTreeCompositeIndex == 0)
    {
    this->ExecuteSubTree(cd, output);
    }

  vtkCompositeDataIterator* iter = cd->NewIterator();
  iter->VisitOnlyLeavesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal() && 
    iter->GetCurrentFlatIndex() <= this->SubTreeCompositeIndex;
    iter->GoToNextItem())
    {
    if (iter->GetCurrentFlatIndex() == this->SubTreeCompositeIndex)
      {
      vtkDataObject* curDO = iter->GetCurrentDataObject();
      vtkCompositeDataSet* curCD = vtkCompositeDataSet::SafeDownCast(curDO);
      vtkUnstructuredGrid* curUG = vtkUnstructuredGrid::SafeDownCast(curDO);
      if (curUG)
        {
        output->ShallowCopy(curUG);
        }
      else if (curCD)
        {
        this->ExecuteSubTree(curCD, output);
        }
      break;
      }
    }
  iter->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::ExecuteSubTree(
  vtkCompositeDataSet* curCD, vtkUnstructuredGrid* output)
{
  vtkAppendFilter* appender = vtkAppendFilter::New();
  int counter = 0;

  vtkCompositeDataIterator* iter2 = curCD->NewIterator();
  for (iter2->InitTraversal(); !iter2->IsDoneWithTraversal();
    iter2->GoToNextItem())
    {
    vtkUnstructuredGrid* curUG = 
      vtkUnstructuredGrid::SafeDownCast(iter2->GetCurrentDataObject());
    if (curUG)
      {
      appender->AddInput(curUG);
      counter++;
      }
    }
  iter2->Delete();

  if (counter > 0)
    {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
    }
  appender->Delete();
}

//----------------------------------------------------------------------------
int vtkCompositeDataToUnstructuredGridFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataToUnstructuredGridFilter::PrintSelf(ostream& os,
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SubTreeCompositeIndex: " 
    << this->SubTreeCompositeIndex << endl;
}


