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
#include "vtkAMRStreamingVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAMRStreamingPriorityQueue.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkRenderer.h"
#include "vtkResampledAMRImageSource.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkUniformGrid.h"
#include "vtkVolumeProperty.h"

vtkStandardNewMacro(vtkAMRStreamingVolumeRepresentation);
//----------------------------------------------------------------------------
vtkAMRStreamingVolumeRepresentation::vtkAMRStreamingVolumeRepresentation()
{
  this->VolumeMapper = vtkSmartVolumeMapper::New();
  this->Property = vtkVolumeProperty::New();
  this->Actor = vtkPVLODVolume::New();
  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->VolumeMapper);
  this->Resampler = vtkResampledAMRImageSource::New();
  this->Resampler->SetMaxDimensions(32, 32, 32);

  this->VolumeMapper->SetInputConnection(
    this->Resampler->GetOutputPort());

  vtkMath::UninitializeBounds(this->DataBounds);

  this->StreamingCapablePipeline = false;
  this->InStreamingUpdate = false;
  this->PriorityQueue = vtkSmartPointer<vtkAMRStreamingPriorityQueue>::New();
}

//----------------------------------------------------------------------------
vtkAMRStreamingVolumeRepresentation::~vtkAMRStreamingVolumeRepresentation()
{
  this->Resampler->Delete();
  this->VolumeMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::FillInputPortInformation(int port,
  vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_UPDATE())
    {
    // FIXME:STREAMING -- currently, only data-server rendering is supported and
    // that too in non-parallel mode.
    //vtkPVRenderView::SetPiece(inInfo, this,
    //  this->OutlineSource->GetOutputDataObject(0));
    // outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    vtkPVRenderView::SetPiece(inInfo, this, this->InputData);

    // Just let the view know of out data bounds.
    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds);
    vtkPVRenderView::SetStreamable(inInfo, this, this->StreamingCapablePipeline);
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    vtkAlgorithm* producer = producerPort->GetProducer();
    vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(
      producer->GetOutputDataObject(producerPort->GetIndex()));

    // FIXME:STREAMING - this needs to called only when the resampler hasn't
    // been updated at all. Not for every render.
    if (this->Resampler->NeedsInitialization())
      {
      this->Resampler->UpdateResampledVolume(amr);
      }
    }
  else if (request_type == vtkPVRenderView::REQUEST_STREAMING_UPDATE())
    {
    if (this->StreamingCapablePipeline)
      {
      // this is a streaming update request. request next piece.
      double view_planes[24];
      inInfo->Get(vtkPVRenderView::VIEW_PLANES(), view_planes);
      if (this->StreamingUpdate(view_planes))
        {
        vtkPVRenderView::SetNextStreamedPiece(inInfo, this, this->InputData);
        }
      }
    }
  else if (request_type == vtkPVRenderView::REQUEST_PROCESS_STREAMED_PIECE())
    {
    vtkOverlappingAMR* piece = vtkOverlappingAMR::SafeDownCast(
      vtkPVRenderView::GetCurrentStreamedPiece(inInfo, this));
    if (piece)
      {
      this->Resampler->UpdateResampledVolume(piece);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::RequestInformation(
  vtkInformation *rqst,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Determine if the input is streaming capable. A pipeline is streaming
  // capable if it provides us with COMPOSITE_DATA_META_DATA() in the
  // RequestInformation() pass. It implies that we can request arbitrary blocks
  // from the input pipeline which implies streamability.
  this->StreamingCapablePipeline = false;
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()) &&
      vtkPVView::GetEnableStreaming())
      {
      this->StreamingCapablePipeline = true;
      }
    }

  vtkStreamingStatusMacro(
    << this << ": streaming capable input pipeline? "
    << (this->StreamingCapablePipeline? "yes" : "no"));
  return this->Superclass::RequestInformation(rqst, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestUpdateExtent(request, inputVector,
      outputVector))
    {
    return 0;
    }

  for (int cc=0; cc < this->GetNumberOfInputPorts(); cc++)
    {
    for (int kk=0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
      {
      vtkInformation* info = inputVector[cc]->GetInformationObject(kk);
      if (this->InStreamingUpdate)
        {
        assert(this->PriorityQueue->IsEmpty() == false);
        int cid = static_cast<int>(this->PriorityQueue->Pop());
        vtkStreamingStatusMacro(<< this << ": requesting blocks: " << cid);
        // Request the next "group of blocks" to stream.
        info->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);
        info->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &cid, 1);
        }
      else
        {
        // let the source deliver whatever is the default. What the reader does
        // when the downstream doesn't request any particular blocks in poorly
        // defined right now. I am assuming the reader will only read the root
        // block or down to some user-specified level.
        info->Remove(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS());
        info->Remove(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()) &&
      vtkPVView::GetEnableStreaming() && !this->InStreamingUpdate)
      {
      vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(
        inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));
      this->PriorityQueue->Initialize(amr->GetAMRInfo());
      }
    }

  if (!this->InStreamingUpdate)
    {
    this->Resampler->Reset();
    }

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkOverlappingAMR* amr = vtkOverlappingAMR::GetData(inputVector[0], 0);
    amr->GetBounds(this->DataBounds);
    this->InputData = amr;
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingVolumeRepresentation::StreamingUpdate(const double view_planes[24])
{
  assert(this->InStreamingUpdate == false);
  if (!this->PriorityQueue->IsEmpty())
    {
    this->InStreamingUpdate = true;
    vtkStreamingStatusMacro(<< this << ": doing streaming-update.");

    // update the priority queue, if needed.
    this->PriorityQueue->Update(view_planes);

    this->MarkModified();
    this->Update();

    this->InStreamingUpdate = false;
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingVolumeRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return true;
    }
  return false;
}


//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetPosition(double x , double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val? 1 : 0);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetShade(bool val)
{
  this->Property->SetShade(val);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetIndependantComponents(bool val)
{
  this->Property->SetIndependentComponents(val);
}
//***************************************************************************
// Forwarded to vtkSmartVolumeMapper.
//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  this->VolumeMapper->SetRequestedRenderMode(mode);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetColorArrayName(const char* arrayname)
{
  this->VolumeMapper->SelectScalarArray(arrayname);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetColorAttributeType(int type)
{
  switch (type)
    {
  case CELL_DATA:
  case POINT_DATA:
  default:
    // since input in AMR, all cell data on AMR becomes point field on the
    // resampled data.
    this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetNumberOfSamples(int x, int y, int z)
{
  // if number of samples change, we restart the streaming. This can be
  // avoided, but it just keeps things simple for now.
  this->Resampler->SetMaxDimensions(x, y, z);
  this->MarkModified();
}
