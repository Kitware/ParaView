// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVRepresentedDataInformation.h"

#include "vtkAppendPolyData.h"
#include "vtkDataObject.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPolyData.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkPVRepresentedDataInformation);
//----------------------------------------------------------------------------
vtkPVRepresentedDataInformation::vtkPVRepresentedDataInformation() = default;

//----------------------------------------------------------------------------
vtkPVRepresentedDataInformation::~vtkPVRepresentedDataInformation() = default;

//----------------------------------------------------------------------------
void vtkPVRepresentedDataInformation::CopyFromObject(vtkObject* object)
{
  vtkPVDataRepresentation* repr = vtkPVDataRepresentation::SafeDownCast(object);
  if (repr)
  {
    vtkDataObject* dobj = repr->GetRenderedDataObject(0);
    if (dobj)
    {
      this->Superclass::CopyFromObject(dobj);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVRepresentedDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkCompositeDataSet> vtkPVRepresentedDataInformation::SimplifyCompositeDataSet(
  vtkCompositeDataSet* cd)
{
  if (!cd)
  {
    return nullptr;
  }
  auto cdDatasets = vtkCompositeDataSet::GetDataSets<vtkDataSet>(cd);
  // check if it's empty
  if (cdDatasets.empty())
  {
    return cd;
  }
  // check if there are only polydata
  for (auto dataset : cdDatasets)
  {
    if (dataset->GetDataObjectType() != VTK_POLY_DATA)
    {
      return cd;
    }
  }
  // process pdc
  if (auto dObjTree = vtkDataObjectTree::SafeDownCast(cd))
  {
    auto simpleTree = vtk::TakeSmartPointer(dObjTree->NewInstance());
    simpleTree->ShallowCopy(dObjTree);

    auto piecesToMerge = vtkCompositeDataSet::GetDataSets<vtkPartitionedDataSet>(simpleTree);
    for (auto piece : piecesToMerge)
    {
      auto polydatas = vtkCompositeDataSet::GetDataSets<vtkPolyData>(piece);
      if (!polydatas.empty())
      {
        vtkNew<vtkAppendPolyData> appender;
        vtkNew<vtkPolyData> output;
        appender->ExecuteAppend(output, polydatas.data(), static_cast<int>(polydatas.size()));
        output->SetFieldData(polydatas[0]->GetFieldData());
        piece->SetNumberOfPartitions(1);
        piece->SetPartition(0, output);
      }
    }
    return simpleTree;
  }
  return cd;
}
