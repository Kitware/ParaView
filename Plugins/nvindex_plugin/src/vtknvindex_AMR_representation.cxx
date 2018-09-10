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
#include "vtknvindex_AMR_representation.h"

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

vtkStandardNewMacro(vtknvindex_AMR_representation);
//----------------------------------------------------------------------------
vtknvindex_AMR_representation::vtknvindex_AMR_representation()
{
  this->StreamingCapablePipeline = false;
  this->InStreamingUpdate = false;

  this->PriorityQueue = vtkSmartPointer<vtkAMRStreamingPriorityQueue>::New();
  this->Resampler = vtkSmartPointer<vtkResampledAMRImageSource>::New();
  this->Resampler->SetMaxDimensions(32, 32, 32);

  this->VolumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
  this->VolumeMapper->SetInputConnection(this->Resampler->GetOutputPort());

  this->Property = vtkSmartPointer<vtkVolumeProperty>::New();
  this->Actor = vtkSmartPointer<vtkPVLODVolume>::New();
  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->VolumeMapper);

  this->ResamplingMode = vtknvindex_AMR_representation::RESAMPLE_OVER_DATA_BOUNDS;

  this->StreamingRequestSize = 50;
}

//----------------------------------------------------------------------------
vtknvindex_AMR_representation::~vtknvindex_AMR_representation()
{
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetResamplingMode(int val)
{
  if (val != this->ResamplingMode && val >= RESAMPLE_OVER_DATA_BOUNDS &&
    val <= RESAMPLE_USING_VIEW_FRUSTUM)
  {
    this->ResamplingMode = val;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtknvindex_AMR_representation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkOverlappingAMR");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::PrintSelf(ostream& os, vtkIndent indent)
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
int vtknvindex_AMR_representation::ProcessViewRequest(
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
    vtkPVRenderView::SetGeometryBounds(inInfo, bounds);

    // The only thing extra we need to do here is that we need to let the view
    // know that this representation is streaming capable (or not).
    vtkPVRenderView::SetStreamable(inInfo, this, this->GetStreamingCapablePipeline());

    // in theory, we need ordered compositing, but we are not going to support
    // parallel AMR volume rendering for now.
    // FIXME: vtknvindex_AMR_representation doesn't support parallel
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
      assert(amr != NULL);
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

  return 1;
}

//----------------------------------------------------------------------------
int vtknvindex_AMR_representation::RequestInformation(
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
int vtknvindex_AMR_representation::RequestUpdateExtent(
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
        assert(this->PriorityQueue->IsEmpty() == false);
        assert(this->StreamingRequestSize > 0);

        int* request_ids = new int[this->StreamingRequestSize];
        for (int jj = 0; jj < this->StreamingRequestSize; jj++)
        {
          int cid = static_cast<int>(this->PriorityQueue->Pop());
          // vtkStreamingStatusMacro(<< this << ": requesting blocks: " << cid);
          request_ids[jj] = cid;
        }
        // Request the next "group of blocks" to stream.
        info->Set(vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1);
        info->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), request_ids,
          this->StreamingRequestSize);
        delete[] request_ids;
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
int vtknvindex_AMR_representation::RequestData(
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

  this->ProcessedPiece = NULL;
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
bool vtknvindex_AMR_representation::StreamingUpdate(
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
      // change and it wouldn't think it needs initialization and then we dont'
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
bool vtknvindex_AMR_representation::AddToView(vtkView* view)
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
bool vtknvindex_AMR_representation::RemoveFromView(vtkView* view)
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
void vtknvindex_AMR_representation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetShade(bool val)
{
  this->Property->SetShade(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetIndependantComponents(bool val)
{
  this->Property->SetIndependentComponents(val);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->VolumeMapper->SelectScalarArray(name);

  // since input in AMR, all cell data on AMR becomes point field on the
  // resampled data.
  this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
}

//***************************************************************************
// Forwarded to vtkSmartVolumeMapper.
//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetRequestedRenderMode(int mode)
{
  this->VolumeMapper->SetRequestedRenderMode(mode);
}

//----------------------------------------------------------------------------
void vtknvindex_AMR_representation::SetNumberOfSamples(int x, int y, int z)
{
  if (x >= 10 && y >= 10 && z >= 10)
  {
    // if number of samples change, we restart the streaming. This can be
    // avoided, but it just keeps things simple for now.
    this->Resampler->SetMaxDimensions(x, y, z);
    this->MarkModified();
  }
}
