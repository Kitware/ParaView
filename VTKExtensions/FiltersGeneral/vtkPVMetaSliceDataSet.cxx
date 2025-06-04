// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVMetaSliceDataSet.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkExtractGeometry.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkPVCutter.h"
#include "vtkPlane.h"

class vtkPVMetaSliceDataSet::vtkInternals
{
public:
  vtkNew<vtkPVCutter> Cutter;
  vtkNew<vtkExtractGeometry> ExtractCells;

  vtkInternals()
  {
    this->ExtractCells->SetExtractInside(1);
    this->ExtractCells->SetExtractOnlyBoundaryCells(1);
    this->ExtractCells->SetExtractBoundaryCells(1);
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMetaSliceDataSet);

//----------------------------------------------------------------------------
vtkPVMetaSliceDataSet::vtkPVMetaSliceDataSet()
{
  // Setup default configuration
  this->SetOutputType(VTK_POLY_DATA);

  this->Internal = new vtkInternals();
  this->Locator = nullptr;

  this->RegisterFilter(this->Internal->Cutter.GetPointer());
  this->RegisterFilter(this->Internal->ExtractCells.GetPointer());

  this->ImplicitFunctions[METASLICE_DATASET] = nullptr;
  this->ImplicitFunctions[METASLICE_HYPERTREEGRID] = nullptr;

  this->Superclass::SetActiveFilter(0);
}

//----------------------------------------------------------------------------
vtkPVMetaSliceDataSet::~vtkPVMetaSliceDataSet()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetImplicitFunction(vtkImplicitFunction* func)
{
  this->Internal->Cutter->SetCutFunction(func);
  this->Internal->ExtractCells->SetImplicitFunction(func);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetDataSetCutFunction(vtkImplicitFunction* func)
{
  this->ImplicitFunctions[METASLICE_DATASET] = func;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetHyperTreeGridCutFunction(vtkImplicitFunction* func)
{
  this->ImplicitFunctions[METASLICE_HYPERTREEGRID] = func;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetNumberOfContours(int nbContours)
{
  this->Internal->Cutter->SetNumberOfContours(nbContours);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetValue(int index, double value)
{
  this->Internal->Cutter->SetValue(index, value);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkAlgorithm* vtkPVMetaSliceDataSet::SetActiveFilter(int index)
{
  this->SetOutputType((index == 0) ? VTK_POLY_DATA : VTK_UNSTRUCTURED_GRID);
  this->Modified();
  return this->Superclass::SetActiveFilter(index);
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetDual(bool dual)
{
  this->Internal->Cutter->SetDual(dual);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::PreserveInputCells(int keepCellAsIs)
{
  this->SetActiveFilter(keepCellAsIs);
}

//----------------------------------------------------------------------------
void vtkPVMetaSliceDataSet::SetGenerateTriangles(int status)
{
  this->Internal->Cutter->SetGenerateTriangles(status);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVMetaSliceDataSet::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!inputVector && !inputVector[0])
  {
    return 0;
  }

  vtkInformation* info = inputVector[0]->GetInformationObject(0);

  if (info && vtkHyperTreeGrid::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT())))
  {
    this->Internal->Cutter->SetCutFunction(this->ImplicitFunctions[METASLICE_HYPERTREEGRID]);
    this->Internal->ExtractCells->SetImplicitFunction(
      this->ImplicitFunctions[METASLICE_HYPERTREEGRID]);
    // If plane is forced to be aligned to axis, the output is a vtkHyperTreeGrid
    vtkPlane* plane = vtkPlane::SafeDownCast(this->Internal->Cutter->GetCutFunction());
    if (plane && !plane->GetAxisAligned())
    {
      this->SetOutputType(VTK_POLY_DATA);
    }
    else
    {
      // PARAVIEW_DEPRECATED_IN_5_13_0("Use vtkAxisAlignedCutter instead")
      this->SetOutputType(VTK_HYPER_TREE_GRID);
    }
  }
  else
  {
    this->Internal->Cutter->SetCutFunction(this->ImplicitFunctions[METASLICE_DATASET]);
    this->Internal->ExtractCells->SetImplicitFunction(this->ImplicitFunctions[METASLICE_DATASET]);
  }

  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points.
void vtkPVMetaSliceDataSet::SetLocator(vtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  this->Locator = locator;
  this->Internal->Cutter->SetLocator(this->Locator);
  this->Modified();
}

//----------------------------------------------------------------------------
vtkIncrementalPointLocator* vtkPVMetaSliceDataSet::GetLocator()
{
  return this->Locator.Get();
}

//------------------------------------------------------------------------------
vtkMTimeType vtkPVMetaSliceDataSet::GetMTime()
{
  vtkMTimeType time;
  vtkMTimeType mTime = this->Superclass::GetMTime();

  if (this->ImplicitFunctions[METASLICE_DATASET])
  {
    time = this->ImplicitFunctions[METASLICE_DATASET]->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->ImplicitFunctions[METASLICE_HYPERTREEGRID])
  {
    time = this->ImplicitFunctions[METASLICE_HYPERTREEGRID]->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}
