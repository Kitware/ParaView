/*=========================================================================

  Program:   ParaView
  Module:    vtkSkyboxRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSkyboxRepresentation.h"

#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSkybox.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredExtent.h"
#include "vtkTexture.h"

#include <algorithm>

vtkStandardNewMacro(vtkSkyboxRepresentation);
//----------------------------------------------------------------------------
vtkSkyboxRepresentation::vtkSkyboxRepresentation()
{
  this->Actor = vtkSkybox::New();
  vtkNew<vtkTexture> tex;
  this->Actor->SetTexture(tex);
  this->Actor->SetProjection(vtkSkybox::Sphere);
}

//----------------------------------------------------------------------------
vtkSkyboxRepresentation::~vtkSkyboxRepresentation()
{
  this->Actor->Delete();
}

void vtkSkyboxRepresentation::SetProjection(int val)
{
  this->Actor->SetProjection(val);
}

void vtkSkyboxRepresentation::SetFloorPlane(float a, float b, float c, float d)
{
  this->Actor->SetFloorPlane(a, b, c, d);
}

void vtkSkyboxRepresentation::SetFloorRight(float a, float b, float c)
{
  this->Actor->SetFloorRight(a, b, c);
}

//----------------------------------------------------------------------------
int vtkSkyboxRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSkyboxRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // this rep looks empty by default
    outInfo->Set(vtkPVRenderView::RENDER_EMPTY_IMAGES(), 1);

    // provide the "geometry" to the view so the view can delivery it to the
    // rendering nodes as and when needed.
    vtkPVView::SetPiece(inInfo, this, this->SliceData);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      this->Actor->GetTexture()->SetInputConnection(producerPort);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSkyboxRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    this->SliceData->ShallowCopy(input);
  }
  else
  {
    this->SliceData->Initialize();
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkSkyboxRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSkyboxRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSkyboxRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSkyboxRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
}
