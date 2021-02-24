/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMetaSliceDataSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMetaSliceDataSet.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkExtractGeometry.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkPVCutter.h"
#include "vtkPVPlane.h"
#include "vtkPlane.h"
#include "vtkSmartPointer.h"

class vtkPVMetaSliceDataSet::vtkInternals
{
public:
  vtkNew<vtkPVCutter> Cutter;
  vtkNew<vtkExtractGeometry> ExtractCells;
  vtkNew<vtkMergePoints> MergeLocator;
  vtkNew<vtkNonMergingPointLocator> NonMergeLocator;

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

  this->RegisterFilter(this->Internal->Cutter.GetPointer());
  this->RegisterFilter(this->Internal->ExtractCells.GetPointer());

  this->AxisCut = false;

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
void vtkPVMetaSliceDataSet::SetMergePoints(bool status)
{
  this->Internal->Cutter->SetLocator(status
      ? static_cast<vtkPointLocator*>(this->Internal->MergeLocator.Get())
      : this->Internal->NonMergeLocator.Get());
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

  if (vtkHyperTreeGrid::SafeDownCast(
        inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT())))
  {
    this->Internal->Cutter->SetCutFunction(this->ImplicitFunctions[METASLICE_HYPERTREEGRID]);
    this->Internal->ExtractCells->SetImplicitFunction(
      this->ImplicitFunctions[METASLICE_HYPERTREEGRID]);
    // If plane is forced to be aligned to axis, the output is a vtkHyperTreeGrid
    vtkPVPlane* plane = vtkPVPlane::SafeDownCast(this->Internal->Cutter->GetCutFunction());
    if (plane && !plane->GetAxisAligned())
    {
      this->SetOutputType(VTK_POLY_DATA);
    }
    else
    {
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
