/*=========================================================================

  Program:   ParaView
  Module:    vtkQuadRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadRepresentation.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCubeAxesRepresentation.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineRepresentation.h"
#include "vtkPVMultiSliceView.h"
#include "vtkPVQuadRenderView.h"
#include "vtkPVSession.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkSliceFriendGeometryRepresentation.h"
#include "vtkStringArray.h"
#include "vtkThreeSliceFilter.h"
#include "vtkView.h"

#include <assert.h>

vtkStandardNewMacro(vtkQuadRepresentation);
//----------------------------------------------------------------------------
vtkQuadRepresentation::vtkQuadRepresentation() : vtkCompositeSliceRepresentation()
{
  this->InternalSliceFilter->EnableProbe(1);
  this->XLabel = this->YLabel = this->ZLabel = NULL;
  this->ViewObserverId = 0;
  this->AddObserver(vtkCommand::UpdateDataEvent, this, &vtkQuadRepresentation::UpdateDataEventCallBack);
}
//----------------------------------------------------------------------------
vtkQuadRepresentation::~vtkQuadRepresentation()
{
  this->SetXLabel(NULL);
  this->SetYLabel(NULL);
  this->SetZLabel(NULL);
}

//----------------------------------------------------------------------------
void vtkQuadRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkQuadRepresentation::AddToView(vtkView* view)
{
  // Custom management of representation for QuadView
  if(vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view))
    {
    for(int i=0; i < 3; ++i)
      {
      if(this->Slices[i+1] == NULL)
        {
        continue;
        }

      if(this->ViewObserverId != 0 && this->AssociatedView)
        {
        this->AssociatedView->RemoveObserver(this->ViewObserverId);
        this->ViewObserverId = 0;
        }

      vtkPVRenderView* internalQuadView = quadView->GetOrthoRenderView(i);
      this->AssociatedView = quadView;
      this->ViewObserverId = quadView->AddObserver(vtkCommand::ModifiedEvent, this, &vtkQuadRepresentation::UpdateFromViewConfigurationCallBack);

      // Enable cube axis annotation on slice views
      this->Slices[i+1]->SetCubeAxesVisibility(quadView->GetShowCubeAxes() != 0);

      // Make the main view as master for delivery management
      quadView->AddRepresentation(this->Slices[i+1]);
      quadView->AddRepresentation(this->Slices[i+1]->GetCubeAxesRepresentation());

      // Move actor from main view to our internal view
      this->Slices[i+1]->RemoveFromView(quadView);
      this->Slices[i+1]->AddToView(internalQuadView);
      }
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkQuadRepresentation::RemoveFromView(vtkView* view)
{
  // Custom management of representation for QuadView
  if(vtkPVQuadRenderView* quadView = vtkPVQuadRenderView::SafeDownCast(view))
    {
    // Clear AssociatedView
    if(this->ViewObserverId != 0 && this->AssociatedView)
      {
      quadView->RemoveObserver(this->ViewObserverId);
      this->ViewObserverId = 0;
      }
    this->AssociatedView = NULL;

    for(int i=0; i < 3; ++i)
      {
      if(this->Slices[i+1] == NULL)
        {
        continue;
        }

      vtkPVRenderView* internalQuadView = quadView->GetOrthoRenderView(i);
      quadView->RemoveRepresentation(this->Slices[i+1]);
      quadView->RemoveRepresentation(this->Slices[i+1]->GetCubeAxesRepresentation());
      this->Slices[i+1]->RemoveFromView(internalQuadView);
      }
    }

  return this->Superclass::RemoveFromView(view);
}
//----------------------------------------------------------------------------
void vtkQuadRepresentation::UpdateFromViewConfigurationCallBack(vtkObject*, unsigned long, void*)
{
  if(this->AssociatedView)
    {
    for(int i=0; i < 3; ++i)
      {
      if(this->Slices[i+1] == NULL)
        {
        continue;
        }

      // Enable cube axis annotation on slice views
      this->Slices[i+1]->SetCubeAxesVisibility(this->AssociatedView->GetShowCubeAxes() != 0);
      }
    this->SetOutlineVisibility(this->AssociatedView->GetShowOutline() != 0);
    }
}

//----------------------------------------------------------------------------
void vtkQuadRepresentation::UpdateDataEventCallBack(vtkObject*, unsigned long, void*)
{
  if(this->AssociatedView && this->GetRenderedDataObject(0) && this->GetRenderedDataObject(0)->GetFieldData())
    {
    this->SetXLabel("X");
    this->SetYLabel("Y");
    this->SetZLabel("Z");
    vtkFieldData* fieldData = this->GetRenderedDataObject(0)->GetFieldData();
    vtkStringArray* titleX =
        vtkStringArray::SafeDownCast(fieldData->GetAbstractArray("AxisTitleForX"));
    vtkStringArray* titleY =
        vtkStringArray::SafeDownCast(fieldData->GetAbstractArray("AxisTitleForY"));
    vtkStringArray* titleZ =
        vtkStringArray::SafeDownCast(fieldData->GetAbstractArray("AxisTitleForZ"));
    vtkFloatArray* baseX =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("AxisBaseForX"));
    vtkFloatArray* baseY =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("AxisBaseForY"));
    vtkFloatArray* baseZ =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("AxisBaseForZ"));
    vtkFloatArray* transfX =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("LinearTransformForX"));
    vtkFloatArray* transfY =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("LinearTransformForY"));
    vtkFloatArray* transfZ =
        vtkFloatArray::SafeDownCast(fieldData->GetAbstractArray("LinearTransformForZ"));

    if(titleX && titleX->GetNumberOfValues() > 0)
      {
      this->SetXLabel(titleX->GetValue(0).c_str());
      }
    if(titleY && titleY->GetNumberOfValues() > 0)
      {
      this->SetYLabel(titleY->GetValue(0).c_str());
      }
    if(titleZ && titleZ->GetNumberOfValues() > 0)
      {
      this->SetZLabel(titleZ->GetValue(0).c_str());
      }

    // Update scalar information if possible
    vtkGeometryRepresentation* geoRep = vtkGeometryRepresentation::SafeDownCast(this->GetActiveRepresentation());
    if (geoRep && geoRep->GetColorArrayName() && strlen(geoRep->GetColorArrayName()) > 0)
      {
      double value = 0.0;
      const char* arrayName = geoRep->GetColorArrayName();
      if( this->InternalSliceFilter->GetProbedPointData(arrayName, value) )
        {
        this->AssociatedView->SetScalarLabel(arrayName);
        this->AssociatedView->SetScalarValue(value);
        }
      else
        {
        this->AssociatedView->SetScalarLabel(NULL);
        }
      }
    else
      {
      this->AssociatedView->SetScalarLabel(NULL);
      }

    this->AssociatedView->SetXAxisLabel(this->XLabel);
    this->AssociatedView->SetYAxisLabel(this->YLabel);
    this->AssociatedView->SetZAxisLabel(this->ZLabel);

    double tx[5] = {1,0,0,1,0};
    double ty[5] = {0,1,0,1,0};
    double tz[5] = {0,0,1,1,0};

    // Update if possible
    if(baseX)
      {
      baseX->GetTuple(0, tx);
      }
    if(transfX)
      {
      transfX->GetTuple(0, &tx[3]);
      }
    if(baseY)
      {
      baseY->GetTuple(0, ty);
      }
    if(transfY)
      {
      transfY->GetTuple(0, &ty[3]);
      }
    if(baseZ)
      {
      baseZ->GetTuple(0, tz);
      }
    if(transfZ)
      {
      transfZ->GetTuple(0, &tz[3]);
      }

    this->AssociatedView->SetTransformationForX(tx);
    this->AssociatedView->SetTransformationForY(ty);
    this->AssociatedView->SetTransformationForZ(tz);
    }
}
