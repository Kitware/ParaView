/*=========================================================================

  Program:   ParaView
  Module:    vtkGlyph3DRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlyph3DRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkGlyph3DMapper.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrowSource.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkGlyph3DRepresentation);
//----------------------------------------------------------------------------
vtkGlyph3DRepresentation::vtkGlyph3DRepresentation()
{
  this->SetNumberOfInputPorts(2);

  this->GlyphMapper = vtkGlyph3DMapper::New();
  this->LODGlyphMapper = vtkGlyph3DMapper::New();
  this->GlyphActor = vtkPVLODActor::New();

  this->DummySource = vtkPVArrowSource::New();

  this->GlyphActor->SetMapper(this->GlyphMapper);
  this->GlyphActor->SetLODMapper(this->LODGlyphMapper);
  this->GlyphActor->SetProperty(this->Property);

  this->MeshVisibility = true;
  this->SetMeshVisibility(false);

  this->GlyphMapper->SetInterpolateScalarsBeforeMapping(0);
  this->LODGlyphMapper->SetInterpolateScalarsBeforeMapping(0);
}

//----------------------------------------------------------------------------
vtkGlyph3DRepresentation::~vtkGlyph3DRepresentation()
{
  this->GlyphMapper->Delete();
  this->LODGlyphMapper->Delete();
  this->GlyphActor->Delete();
  this->DummySource->Delete();
}

//----------------------------------------------------------------------------
bool vtkGlyph3DRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->GlyphActor);
    }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkGlyph3DRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->GlyphActor);
    }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->GlyphActor->SetVisibility(val?  1 : 0);
  this->Actor->SetVisibility((val && this->MeshVisibility)? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMeshVisibility(bool val)
{
  this->MeshVisibility = val;
  this->Actor->SetVisibility((val && this->MeshVisibility)? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::FillInputPortInformation(int port,
  vtkInformation *info)
{
  if (port == 0)
    {
    return this->Superclass::FillInputPortInformation(port, info);
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[1]->GetNumberOfInformationObjects()==1)
    {
    this->GlyphMapper->SetInputConnection(1, this->GetInternalOutputPort(1));
    this->LODGlyphMapper->SetInputConnection(1, this->GetInternalOutputPort(1));
    }
  else
    {
    this->GlyphMapper->SetInputConnection(1, this->DummySource->GetOutputPort());
    this->LODGlyphMapper->SetInputConnection(1,
      this->DummySource->GetOutputPort());
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_RENDER())
    {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    vtkAlgorithmOutput* producerPortLOD = vtkPVRenderView::GetPieceProducerLOD(inInfo, this);
    this->GlyphMapper->SetInputConnection(0, producerPort);
    this->LODGlyphMapper->SetInputConnection(0, producerPortLOD);

    bool lod = this->SuppressLOD? false :
      (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    this->GlyphActor->SetEnableLOD(lod? 1 : 0);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters();
  
  if (this->Mapper->GetScalarVisibility() == 0 ||
    this->Mapper->GetScalarMode() != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
    {
    // we are not coloring the glyphs with scalars.
    const char* null = NULL;
    this->GlyphMapper->SetScalarVisibility(0);
    this->LODGlyphMapper->SetScalarVisibility(0);
    this->GlyphMapper->SelectColorArray(null);
    this->LODGlyphMapper->SelectColorArray(null);
    return;
    }

  this->GlyphMapper->SetScalarVisibility(1);
  this->GlyphMapper->SelectColorArray(this->Mapper->GetArrayName());
  this->GlyphMapper->SetUseLookupTableScalarRange(1);
  this->GlyphMapper->SetScalarMode(
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  this->LODGlyphMapper->SetScalarVisibility(1);
  this->LODGlyphMapper->SelectColorArray(this->Mapper->GetArrayName());
  this->LODGlyphMapper->SetUseLookupTableScalarRange(1);
  this->LODGlyphMapper->SetScalarMode(
    VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//**************************************************************************
// Overridden to forward to vtkGlyph3DMapper
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->GlyphMapper->SetLookupTable(val);
  this->LODGlyphMapper->SetLookupTable(val);
  this->Superclass::SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMapScalars(int val)
{
  this->GlyphMapper->SetColorMode(val);
  this->LODGlyphMapper->SetColorMode(val);
  this->Superclass::SetMapScalars(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetInterpolateScalarsBeforeMapping(int val)
{
  // The GlyphMapper does not support InterpolateScalarsBeforeMapping==1. So
  // leave it at 0.
  this->Superclass::SetInterpolateScalarsBeforeMapping(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetStatic(int val)
{
  this->GlyphMapper->SetStatic(val);
  this->LODGlyphMapper->SetStatic(val);
  this->Superclass::SetStatic(val);
}

//**************************************************************************
// Forwarded to vtkGlyph3DMapper
//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMaskArray(const char* val)
{
  this->GlyphMapper->SetMaskArray(val);
  this->LODGlyphMapper->SetMaskArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleArray(const char* val)
{
  this->GlyphMapper->SetScaleArray(val);
  this->LODGlyphMapper->SetScaleArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrientationArray(const char* val)
{
  this->GlyphMapper->SetOrientationArray(val);
  this->LODGlyphMapper->SetOrientationArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaling(bool val)
{
  this->GlyphMapper->SetScaling(val);
  this->LODGlyphMapper->SetScaling(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleMode(int val)
{
  this->GlyphMapper->SetScaleMode(val);
  this->LODGlyphMapper->SetScaleMode(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleFactor(double val)
{
  this->GlyphMapper->SetScaleFactor(val);
  this->LODGlyphMapper->SetScaleFactor(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrient(bool val)
{
  this->GlyphMapper->SetOrient(val);
  this->LODGlyphMapper->SetOrient(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrientationMode(int val)
{
  this->GlyphMapper->SetOrientationMode(val);
  this->LODGlyphMapper->SetOrientationMode(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMasking(bool val)
{
  this->GlyphMapper->SetMasking(val);
  this->LODGlyphMapper->SetMasking(val);
}

//----------------------------------------------------------------------------
double* vtkGlyph3DRepresentation::GetBounds()
{
  return this->GlyphMapper->GetBounds();
}
