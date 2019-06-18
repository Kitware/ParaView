/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentationWithFaces.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometryRepresentationWithFaces.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkGeometryRepresentationWithFaces);
//----------------------------------------------------------------------------
vtkGeometryRepresentationWithFaces::vtkGeometryRepresentationWithFaces()
{
  this->BackfaceActor = vtkPVLODActor::New();
  this->BackfaceProperty = vtkProperty::New();
  this->BackfaceMapper = vtkCompositePolyDataMapper2::New();
  this->LODBackfaceMapper = vtkCompositePolyDataMapper2::New();
  this->BackfaceRepresentation = FOLLOW_FRONTFACE;

  // Since we are overriding SetupDefaults(), we need to call it again.
  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkGeometryRepresentationWithFaces::~vtkGeometryRepresentationWithFaces()
{
  this->BackfaceActor->Delete();
  this->BackfaceProperty->Delete();
  this->BackfaceMapper->Delete();
  this->LODBackfaceMapper->Delete();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::SetupDefaults()
{
  this->Superclass::SetupDefaults();

  this->BackfaceActor->SetProperty(this->BackfaceProperty);
  this->BackfaceActor->SetMapper(this->BackfaceMapper);
  this->BackfaceActor->SetLODMapper(this->LODBackfaceMapper);
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentationWithFaces::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (inInfo->Has(vtkPVRenderView::USE_LOD()))
    {
      this->LODBackfaceMapper->SetInputConnection(0, producerPort);
    }
    else
    {
      this->BackfaceMapper->SetInputConnection(0, producerPort);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  if (!val)
  {
    this->BackfaceActor->SetVisibility(0);
  }
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentationWithFaces::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->BackfaceActor);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentationWithFaces::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->BackfaceActor);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters();
  switch (this->BackfaceRepresentation)
  {
    case FOLLOW_FRONTFACE:
      this->BackfaceActor->SetVisibility(0);
      this->Property->SetBackfaceCulling(0);
      this->Property->SetFrontfaceCulling(0);
      break;

    case CULL_BACKFACE:
      this->BackfaceActor->SetVisibility(0);
      this->Property->SetBackfaceCulling(1);
      this->Property->SetFrontfaceCulling(0);
      break;

    case CULL_FRONTFACE:
      this->BackfaceActor->SetVisibility(0);
      this->Property->SetBackfaceCulling(0);
      this->Property->SetFrontfaceCulling(1);
      break;

    case SURFACE_WITH_EDGES:
      this->BackfaceActor->SetVisibility(this->GetVisibility());
      this->Property->SetBackfaceCulling(1);
      this->Property->SetFrontfaceCulling(0);
      this->BackfaceProperty->SetBackfaceCulling(0);
      this->BackfaceProperty->SetFrontfaceCulling(1);
      this->BackfaceProperty->SetEdgeVisibility(1);
      this->BackfaceProperty->SetRepresentation(VTK_SURFACE);
      break;

    default:
      this->BackfaceActor->SetVisibility(this->GetVisibility());
      this->Property->SetBackfaceCulling(1);
      this->Property->SetFrontfaceCulling(0);
      this->BackfaceProperty->SetBackfaceCulling(0);
      this->BackfaceProperty->SetFrontfaceCulling(1);
      this->BackfaceProperty->SetEdgeVisibility(0);
      this->BackfaceProperty->SetRepresentation(this->BackfaceRepresentation);
  }

  if (this->BackfaceActor->GetVisibility())
  {
    // Adjust material properties.
    double diffuse = this->Diffuse;
    double specular = this->Specular;
    double ambient = this->Ambient;

    if (this->BackfaceRepresentation != SURFACE &&
      this->BackfaceRepresentation != SURFACE_WITH_EDGES)
    {
      diffuse = 0.0;
      ambient = 1.0;
      specular = 0.0;
    }
    else if (this->Mapper->GetScalarVisibility())
    {
      specular = 0.0;
    }

    this->BackfaceProperty->SetAmbient(ambient);
    this->BackfaceProperty->SetSpecular(specular);
    this->BackfaceProperty->SetDiffuse(diffuse);

    // Copy parameters from this->Mapper
    this->BackfaceMapper->SetLookupTable(this->Mapper->GetLookupTable());
    this->BackfaceMapper->SetColorMode(this->Mapper->GetColorMode());
    this->BackfaceMapper->SetInterpolateScalarsBeforeMapping(
      this->Mapper->GetInterpolateScalarsBeforeMapping());
    this->BackfaceMapper->SetStatic(this->Mapper->GetStatic());
    this->BackfaceMapper->SetScalarVisibility(this->Mapper->GetScalarVisibility());
    this->BackfaceMapper->SelectColorArray(this->Mapper->GetArrayName());
    this->BackfaceMapper->SetScalarMode(this->Mapper->GetScalarMode());

    // Copy parameters from this->LODMapper
    this->LODBackfaceMapper->SetLookupTable(this->LODMapper->GetLookupTable());
    this->LODBackfaceMapper->SetColorMode(this->LODMapper->GetColorMode());
    this->LODBackfaceMapper->SetInterpolateScalarsBeforeMapping(
      this->LODMapper->GetInterpolateScalarsBeforeMapping());
    this->LODBackfaceMapper->SetStatic(this->LODMapper->GetStatic());
    this->LODBackfaceMapper->SetScalarVisibility(this->LODMapper->GetScalarVisibility());
    this->LODBackfaceMapper->SelectColorArray(this->LODMapper->GetArrayName());
    this->LODBackfaceMapper->SetScalarMode(this->LODMapper->GetScalarMode());

    // Copy parameters from this->Property
    this->BackfaceProperty->SetEdgeColor(this->Property->GetEdgeColor());
    this->BackfaceProperty->SetInterpolation(this->Property->GetInterpolation());
    this->BackfaceProperty->SetLineWidth(this->Property->GetLineWidth());
    this->BackfaceProperty->SetPointSize(this->Property->GetPointSize());
    this->BackfaceProperty->SetSpecularColor(this->Property->GetSpecularColor());
    this->BackfaceProperty->SetSpecularPower(this->Property->GetSpecularPower());

    // Copy parameters from this->Actor
    this->BackfaceActor->SetOrientation(this->Actor->GetOrientation());
    this->BackfaceActor->SetOrigin(this->Actor->GetOrigin());
    this->BackfaceActor->SetPickable(this->Actor->GetPickable());
    this->BackfaceActor->SetPosition(this->Actor->GetPosition());
    this->BackfaceActor->SetScale(this->Actor->GetScale());
    this->BackfaceActor->SetTexture(this->Actor->GetTexture());
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentationWithFaces::NeedsOrderedCompositing()
{
  if (this->BackfaceProperty->GetOpacity() > 0.0 && this->BackfaceProperty->GetOpacity() < 1.0)
  {
    return true;
  }

  return this->Superclass::NeedsOrderedCompositing();
}

//***************************************************************************
// Forwarded to vtkProperty(BackfaceProperty)
//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::SetBackfaceAmbientColor(double r, double g, double b)
{
  this->BackfaceProperty->SetAmbientColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::SetBackfaceDiffuseColor(double r, double g, double b)
{
  this->BackfaceProperty->SetDiffuseColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentationWithFaces::SetBackfaceOpacity(double val)
{
  this->BackfaceProperty->SetOpacity(val);
}
