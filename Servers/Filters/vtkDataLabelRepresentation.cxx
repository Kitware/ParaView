/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataLabelRepresentation.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCellCenters.h"
#include "vtkCompositeDataToUnstructuredGridFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkUnstructuredDataDeliveryFilter.h"

vtkStandardNewMacro(vtkDataLabelRepresentation);
//----------------------------------------------------------------------------
vtkDataLabelRepresentation::vtkDataLabelRepresentation()
{
  this->PointLabelVisibility = 0;
  this->CellLabelVisibility = 0;

  this->MergeBlocks = vtkCompositeDataToUnstructuredGridFilter::New();
  this->CacheKeeper = vtkPVCacheKeeper::New();
  this->DataCollector = vtkUnstructuredDataDeliveryFilter::New();

  this->PointLabelMapper = vtkLabeledDataMapper::New();
  this->PointLabelActor = vtkActor2D::New();
  this->PointLabelProperty = vtkTextProperty::New();

  this->Transform = vtkTransform::New();
  this->Transform->Identity();

  this->CellCenters = vtkCellCenters::New();
  this->CellLabelMapper = vtkLabeledDataMapper::New();
  this->CellLabelActor = vtkActor2D::New();
  this->CellLabelProperty = vtkTextProperty::New();

  this->DataCollector->SetOutputDataType(VTK_UNSTRUCTURED_GRID);

  this->CacheKeeper->SetInputConnection(this->MergeBlocks->GetOutputPort());
  this->PointLabelMapper->SetInputConnection(this->DataCollector->GetOutputPort());
  this->CellCenters->SetInputConnection(this->DataCollector->GetOutputPort());
  this->CellLabelMapper->SetInputConnection(this->CellCenters->GetOutputPort());

  this->PointLabelActor->SetMapper(this->PointLabelMapper);
  this->CellLabelActor->SetMapper(this->CellLabelMapper);

  this->PointLabelMapper->SetLabelTextProperty(this->PointLabelProperty);
  this->CellLabelMapper->SetLabelTextProperty(this->CellLabelProperty);

  this->PointLabelMapper->SetTransform(this->Transform);
  this->CellLabelMapper->SetTransform(this->Transform);

  this->PointLabelActor->SetVisibility(0);
  this->CellLabelActor->SetVisibility(0);

  this->TransformHelperProp = vtkActor::New();
  this->InitializeForCommunication();
}

//----------------------------------------------------------------------------
vtkDataLabelRepresentation::~vtkDataLabelRepresentation()
{
  this->MergeBlocks->Delete();
  this->DataCollector->Delete();
  this->PointLabelMapper->Delete();
  this->PointLabelActor->Delete();
  this->PointLabelProperty->Delete();
  this->CellCenters->Delete();
  this->CellLabelMapper->Delete();
  this->CellLabelActor->Delete();
  this->CellLabelProperty->Delete();
  this->Transform->Delete();
  this->TransformHelperProp->Delete();
  this->CacheKeeper->Delete();
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::InitializeForCommunication()
{
  vtkInformation* info = vtkInformation::New();
  info->Set(vtkPVRenderView::DATA_DISTRIBUTION_MODE(),
    vtkMPIMoveData::CLONE);
  this->DataCollector->ProcessViewRequest(info);
  info->Delete();
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->SetPointLabelVisibility(this->PointLabelVisibility);
  this->SetCellLabelVisibility(this->CellLabelVisibility);
}

//----------------------------------------------------------------------------
bool vtkDataLabelRepresentation::GetVisibility()
{
  return this->Superclass::GetVisibility() && (
    this->PointLabelVisibility || this->CellLabelVisibility);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelVisibility(int val)
{
  this->PointLabelVisibility = val;
  this->PointLabelActor->SetVisibility(val && this->GetVisibility());
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointFieldDataArrayName(const char* val)
{
  this->PointLabelMapper->SetFieldDataName(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelMode(int val)
{
  this->PointLabelMapper->SetLabelMode(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelColor(double r, double g, double b)
{
  this->PointLabelProperty->SetColor(r, g, b);
}
//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelOpacity(double val)
{
  this->PointLabelProperty->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelFontFamily(int val)
{
  this->PointLabelProperty->SetFontFamily(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelBold(int val)
{
  this->PointLabelProperty->SetBold(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelItalic(int val)
{
  this->PointLabelProperty->SetItalic(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelShadow(int val)
{
  this->PointLabelProperty->SetShadow(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelJustification(int val)
{
  this->PointLabelProperty->SetJustification(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPointLabelFontSize(int val)
{
  this->PointLabelProperty->SetFontSize(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelVisibility(int val)
{
  this->CellLabelVisibility = val;
  this->CellLabelActor->SetVisibility(val && this->GetVisibility());
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellFieldDataArrayName(const char* val)
{
  this->CellLabelMapper->SetFieldDataName(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelMode(int val)
{
  this->CellLabelMapper->SetLabelMode(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelColor(double r, double g, double b)
{
  this->CellLabelProperty->SetColor(r, g, b);
}
//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelOpacity(double val)
{
  this->CellLabelProperty->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelFontFamily(int val)
{
  this->CellLabelProperty->SetFontFamily(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelBold(int val)
{
  this->CellLabelProperty->SetBold(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelItalic(int val)
{
  this->CellLabelProperty->SetItalic(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelShadow(int val)
{
  this->CellLabelProperty->SetShadow(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelJustification(int val)
{
  this->CellLabelProperty->SetJustification(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetCellLabelFontSize(int val)
{
  this->CellLabelProperty->SetFontSize(val);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::MarkModified()
{
  this->DataCollector->Modified();
  if (!this->GetUseCache())
    {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkDataLabelRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
int vtkDataLabelRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
bool vtkDataLabelRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetNonCompositedRenderer()->AddActor(this->PointLabelActor);
    rview->GetNonCompositedRenderer()->AddActor(this->CellLabelActor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkDataLabelRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetNonCompositedRenderer()->RemoveActor(this->PointLabelActor);
    rview->GetNonCompositedRenderer()->RemoveActor(this->CellLabelActor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
int vtkDataLabelRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    this->MergeBlocks->SetInputConnection(
      this->GetInternalOutputPort());
    this->MergeBlocks->Update();
    this->DataCollector->SetInputConnection(this->CacheKeeper->GetOutputPort());
    }
  else
    {
    this->MergeBlocks->RemoveAllInputs();
    this->DataCollector->RemoveAllInputs();
    }

  // Since data-deliver mode never changes for this representation, we simply do
  // the data-delivery in RequestData itself to keep things simple.
  this->DataCollector->Update();

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::UpdateTransform()
{
  // From vtkProp3D::ComputeMatrix
  double elements[16];
  this->TransformHelperProp->GetMatrix(elements);
  this->Transform->SetMatrix(elements);
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetOrientation(double x, double y, double z)
{
  this->TransformHelperProp->SetOrientation(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetOrigin(double x, double y, double z)
{
  this->TransformHelperProp->SetOrigin(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetPosition(double x, double y, double z)
{
  this->TransformHelperProp->SetPosition(x, y, z);
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
void vtkDataLabelRepresentation::SetScale(double x, double y, double z)
{
  this->TransformHelperProp->SetScale(x, y, z);
  this->UpdateTransform();
}
