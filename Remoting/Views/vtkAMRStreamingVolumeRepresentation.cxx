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

#include "vtkAMRStreamingPriorityQueue.h"
#include "vtkAMRVolumeMapper.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkResampledAMRImageSource.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkUniformGrid.h"
#include "vtkVolumeProperty.h"

vtkStandardNewMacro(vtkAMRStreamingVolumeRepresentation);
//----------------------------------------------------------------------------
vtkAMRStreamingVolumeRepresentation::vtkAMRStreamingVolumeRepresentation()
{
  this->StreamingCapablePipeline = false;
  this->InStreamingUpdate = false;

  this->PriorityQueue = vtkSmartPointer<vtkAMRStreamingPriorityQueue>::New();
  this->Resampler = vtkSmartPointer<vtkResampledAMRImageSource>::New();
  this->Resampler->SetMaxDimensions(32, 32, 32);

  this->VolumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
  this->VolumeMapper->SetInputConnection(this->Resampler->GetOutputPort());
  this->AMRVolumeMapper = vtkSmartPointer<vtkAMRVolumeMapper>::New();
  this->AMRVolumeMapper->SetInputConnection(this->Resampler->GetOutputPort());

  this->Property = vtkSmartPointer<vtkVolumeProperty>::New();
  this->Actor = vtkSmartPointer<vtkPVLODVolume>::New();
  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->VolumeMapper);

  this->ResamplingMode = vtkAMRStreamingVolumeRepresentation::RESAMPLE_OVER_DATA_BOUNDS;

  this->StreamingRequestSize = 50;
}

//----------------------------------------------------------------------------
vtkAMRStreamingVolumeRepresentation::~vtkAMRStreamingVolumeRepresentation()
{
  this->AMRVolumeMapper->SetInputConnection(nullptr);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetResamplingMode(int val)
{
  if (val != this->ResamplingMode && val >= RESAMPLE_OVER_DATA_BOUNDS &&
    val <= RESAMPLE_USING_VIEW_FRUSTUM)
  {
    this->ResamplingMode = val;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ResamplingMode: ";
  switch (this->ResamplingMode)
  {
    case RESAMPLE_OVER_DATA_BOUNDS:
      os << "RESAMPLE_OVER_DATA_BOUNDS" << endl;
      break;

    case RESAMPLE_USING_VIEW_FRUSTUM:
      os << "RESAMPLE_USING_VIEW_FRUSTUM" << endl;
      break;

    default:
      os << "(invalid)" << endl;
  }
  os << indent << "StreamingRequestSize: " << this->StreamingRequestSize << endl;
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // Standard representation stuff, first.
    // 1. Provide the data being rendered.
    vtkPVRenderView::SetPiece(inInfo, this, this->ProcessedData);

    // 2. Provide the bounds.
    double bounds[6];
    this->DataBounds.GetBounds(bounds);

    // Just let the view know of out data bounds.
    vtkPVRenderView::SetGeometryBounds(inInfo, this, bounds);

    // The only thing extra we need to do here is that we need to let the view
    // know that this representation is streaming capable (or not).
    vtkPVRenderView::SetStreamable(inInfo, this, this->GetStreamingCapablePipeline());

    // in theory, we need ordered compositing, but we are not going to support
    // parallel AMR volume rendering for now.
    // FIXME: vtkAMRStreamingVolumeRepresentation doesn't support parallel
    // volume rendering. We support parallel-server+local+rendering,
    // single-server+remote-rendering and builtin configurations.
  }
  else if (request_type == vtkPVRenderView::REQUEST_STREAMING_UPDATE())
  {
    if (this->GetStreamingCapablePipeline())
    {
      // This is a streaming update request, request next piece.
      double view_planes[24];
      inInfo->Get(vtkPVRenderView::VIEW_PLANES(), view_planes);
      vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(inInfo->Get(vtkPVRenderView::VIEW()));
      if (this->StreamingUpdate(view, view_planes))
      {
        // since we indeed "had" a next piece to produce, give it to the view
        // so it can deliver it to the rendering nodes.
        vtkPVRenderView::SetNextStreamedPiece(inInfo, this, this->ProcessedPiece);
      }
    }
  }

  else if (request_type == vtkPVView::REQUEST_RENDER() ||
    request_type == vtkPVRenderView::REQUEST_PROCESS_STREAMED_PIECE())
  {
    if (this->Resampler->NeedsInitialization())
    {
      vtkStreamingStatusMacro(<< this << ": cloning delivered data.");
      vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
      vtkAlgorithm* producer = producerPort->GetProducer();
      vtkOverlappingAMR* amr =
        vtkOverlappingAMR::SafeDownCast(producer->GetOutputDataObject(producerPort->GetIndex()));
      assert(amr != nullptr);
      this->Resampler->UpdateResampledVolume(amr);
    }

    if (request_type == vtkPVRenderView::REQUEST_PROCESS_STREAMED_PIECE())
    {
      vtkOverlappingAMR* piece =
        vtkOverlappingAMR::SafeDownCast(vtkPVRenderView::GetCurrentStreamedPiece(inInfo, this));
      if (piece)
      {
        this->Resampler->UpdateResampledVolume(piece);
      }
    }
  }

  // setup amr volume mapper for OSPRay
  vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
  vtkAlgorithm* producer = producerPort->GetProducer();
  vtkOverlappingAMR* amr =
    vtkOverlappingAMR::SafeDownCast(producer->GetOutputDataObject(producerPort->GetIndex()));
  this->AMRVolumeMapper->SetInputData(amr);

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(inInfo->Get(vtkPVRenderView::VIEW()));
  if (view->GetEnableOSPRay())
  {
    this->Actor->SetMapper(this->AMRVolumeMapper);
  }
  else
  {
    this->Actor->SetMapper(this->VolumeMapper);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::RequestInformation(
  vtkInformation* rqst, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Determine if the input is streaming capable. A pipeline is streaming
  // capable if it provides us with COMPOSITE_DATA_META_DATA() in the
  // RequestInformation() pass. It implies that we can request arbitrary blocks
  // from the input pipeline which implies stream-ability.

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

  vtkStreamingStatusMacro(<< this << ": streaming capable input pipeline? "
                          << (this->StreamingCapablePipeline ? "yes" : "no"));
  return this->Superclass::RequestInformation(rqst, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkAMRStreamingVolumeRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestUpdateExtent(request, inputVector, outputVector))
  {
    return 0;
  }

  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      vtkInformation* info = inputVector[cc]->GetInformationObject(kk);
      if (this->InStreamingUpdate)
      {
        std::vector<int> request_ids(this->StreamingRequestSize + 1);
        int nbIds = 0;
        while (nbIds < this->StreamingRequestSize && !this->PriorityQueue->IsEmpty())
        {
          int cid = static_cast<int>(this->PriorityQueue->Pop());
          // vtkStreamingStatusMacro(<< this << ": requesting blocks: " << cid);
          request_ids[nbIds++] = cid;
        }
        // Request the next "group of blocks" to stream.
        info->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);
        info->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), request_ids.data(), nbIds);
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
int vtkAMRStreamingVolumeRepresentation::RequestData(
  vtkInformation* rqst, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()) &&
      this->GetStreamingCapablePipeline() && !this->GetInStreamingUpdate())
    {
      // Since the representation reexecuted, it means that the input changed
      // and we should initialize our streaming.
      vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(
        inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));
      this->PriorityQueue->Initialize(amr->GetAMRInfo());
    }
  }

  if (!this->GetInStreamingUpdate())
  {
    this->Resampler->Reset();
  }

  this->ProcessedPiece = nullptr;
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    // Do the streaming independent "transformation" of the data here, in our
    // case, generate the outline from the input data.
    // To keep things simple here, we don't bother about the "flip-book" caching
    // support used for animation playback.
    vtkOverlappingAMR* input = vtkOverlappingAMR::GetData(inputVector[0], 0);
    if (!this->GetInStreamingUpdate())
    {
      this->ProcessedData = vtkSmartPointer<vtkOverlappingAMR>::New();
      this->ProcessedData->ShallowCopy(input);

      double bounds[6];
      input->GetBounds(bounds);
      this->DataBounds.SetBounds(bounds);
    }
    else
    {
      this->ProcessedPiece = input;
    }
  }
  else
  {
    // create an empty dataset. This is needed so that view knows what dataset
    // to expect from the other processes on this node.
    vtkSmartPointer<vtkOverlappingAMR> amr = vtkSmartPointer<vtkOverlappingAMR>::New();

    // FIXME: Currently, an empty overlapping AMR causes segfaults in the rest of the
    // pipeline. Until that's fixed, we initialize the dataset with 1 level and
    // 0 blocks.
    int blocks = 0;
    amr->Initialize(1, &blocks);
    this->ProcessedData = amr;
    this->DataBounds.Reset();
  }

  return this->Superclass::RequestData(rqst, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkAMRStreamingVolumeRepresentation::StreamingUpdate(
  vtkPVRenderView* view, const double view_planes[24])
{
  assert(this->InStreamingUpdate == false);
  if (!this->PriorityQueue->IsEmpty())
  {
    this->InStreamingUpdate = true;
    // vtkStreamingStatusMacro(<< this << ": doing streaming-update.");

    if (this->ResamplingMode == RESAMPLE_USING_VIEW_FRUSTUM && view &&
      view->GetRenderWindow()->GetDesiredUpdateRate() < 1)
    {
      // vtkStreamingStatusMacro(<< this << ": compute new volume bounds.");

      double data_bounds[6];
      this->DataBounds.GetBounds(data_bounds);

      double bounds[6];
      if (vtkAMRVolumeMapper::ComputeResamplerBoundsFrustumMethod(
            view->GetActiveCamera(), view->GetRenderer(), data_bounds, bounds))
      {
        // vtkStreamingStatusMacro(<< this << ": computed volume bounds : "
        //  << bounds[0] << ", " << bounds[1] << ", " << bounds[2] << ", "
        //  << bounds[3] << ", " << bounds[4] << ", " << bounds[5] << ".");
        this->Resampler->SetSpatialBounds(bounds);
      }
      // if the bounds changed from last time, we reset the priority queue as
      // well. If the bounds didn't change, then the resampler mtime won't
      // change and it wouldn't think it needs initialization and then we don't
      // need to reinitialize the priority queue.
      if (this->Resampler->NeedsInitialization())
      {
        vtkStreamingStatusMacro(<< this << ": reinitializing priority queue.");
        this->PriorityQueue->Reinitialize();
      }
    }

    // update the priority queue, if needed.
    this->PriorityQueue->Update(view_planes, this->Resampler->GetSpatialBounds());

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
    return this->Superclass::AddToView(rview);
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
    return this->Superclass::RemoveFromView(rview);
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
void vtkAMRStreamingVolumeRepresentation::SetPosition(double x, double y, double z)
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
  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
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

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->VolumeMapper->SelectScalarArray(name);
  this->AMRVolumeMapper->SelectScalarArray(name);

  // since input in AMR, all cell data on AMR becomes point field on the
  // resampled data.
  this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  this->AMRVolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
}

//***************************************************************************
// Forwarded to vtkSmartVolumeMapper.
//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  this->VolumeMapper->SetRequestedRenderMode(mode);
}

//----------------------------------------------------------------------------
void vtkAMRStreamingVolumeRepresentation::SetNumberOfSamples(int x, int y, int z)
{
  if (x >= 10 && y >= 10 && z >= 10)
  {
    // if number of samples change, we restart the streaming. This can be
    // avoided, but it just keeps things simple for now.
    this->Resampler->SetMaxDimensions(x, y, z);
    this->MarkModified();
  }
}
