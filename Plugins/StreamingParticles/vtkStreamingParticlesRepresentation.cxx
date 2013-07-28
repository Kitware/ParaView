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
#include "vtkStreamingParticlesRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAppendCompositeDataLeaves.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkGeometryRepresentation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkPVStreamingMacros.h"
#include "vtkPVTrivialProducer.h"
#include "vtkRenderer.h"
#include "vtkStreamingParticlesPriorityQueue.h"

#include <assert.h>

vtkStandardNewMacro(vtkStreamingParticlesRepresentation);
//----------------------------------------------------------------------------
vtkStreamingParticlesRepresentation::vtkStreamingParticlesRepresentation()
{
  this->StreamingCapablePipeline = false;
  this->InStreamingUpdate = false;
  this->UseOutline = false;

  this->PriorityQueue = vtkSmartPointer<vtkStreamingParticlesPriorityQueue>::New();
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper2>::New();

  this->Actor = vtkSmartPointer<vtkPVLODActor>::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->GetProperty()->SetRepresentationToPoints();
  this->Actor->GetProperty()->SetAmbient(1.0);
  this->Actor->GetProperty()->SetDiffuse(0.0);
  this->Actor->GetProperty()->SetSpecular(0.0);
  this->Actor->SetPickable(0); 
}

//----------------------------------------------------------------------------
vtkStreamingParticlesRepresentation::~vtkStreamingParticlesRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
int vtkStreamingParticlesRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  // always forward to superclass first. Superclass returns 0 if the
  // representation is not visible (among other things). In which case there's
  // nothing to do.
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
    vtkPVRenderView::SetGeometryBounds(inInfo, bounds);

    // The only thing extra we need to do here is that we need to let the view
    // know that this representation is streaming capable (or not).
    vtkPVRenderView::SetStreamable(inInfo, this, this->GetStreamingCapablePipeline());
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    if (this->RenderedData == NULL)
      {
      vtkStreamingStatusMacro(<< this << ": cloning delivered data.");
      vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
      vtkAlgorithm* producer = producerPort->GetProducer();

      this->RenderedData =
        producer->GetOutputDataObject(producerPort->GetIndex());
      this->Mapper->SetInputDataObject(this->RenderedData);
      }
    }
  else if (request_type == vtkPVRenderView::REQUEST_STREAMING_UPDATE())
    {
    if (this->GetStreamingCapablePipeline())
      {
      // This is a streaming update request, request next piece.
      double view_planes[24];
      inInfo->Get(vtkPVRenderView::VIEW_PLANES(), view_planes);
      if (this->StreamingUpdate(view_planes))
        {
        // since we indeed "had" a next piece to produce, give it to the view
        // so it can deliver it to the rendering nodes.
        vtkPVRenderView::SetNextStreamedPiece(
          inInfo, this, this->ProcessedPiece);
        }
      }
    }
  else if (request_type == vtkPVRenderView::REQUEST_PROCESS_STREAMED_PIECE())
    {
    vtkDataObject* piece = vtkPVRenderView::GetCurrentStreamedPiece(inInfo, this);
    if (piece)
      {
      assert (this->RenderedData != NULL);
      vtkStreamingStatusMacro( << this << ": received new piece.");

      // merge with what we are already rendering.
      vtkNew<vtkAppendCompositeDataLeaves> appender;
      appender->AddInputDataObject(piece);
      appender->AddInputDataObject(this->RenderedData);
      appender->Update();

      this->RenderedData = appender->GetOutputDataObject(0);
      this->Mapper->SetInputDataObject(this->RenderedData);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamingParticlesRepresentation::RequestInformation(vtkInformation *rqst,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
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

  vtkStreamingStatusMacro(
    << this << ": streaming capable input pipeline? "
    << (this->StreamingCapablePipeline? "yes" : "no"));
  return this->Superclass::RequestInformation(rqst, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStreamingParticlesRepresentation::RequestUpdateExtent(
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
int vtkStreamingParticlesRepresentation::RequestData(vtkInformation *rqst,
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()) &&
      this->GetStreamingCapablePipeline() &&
      !this->GetInStreamingUpdate())
      {
      // Since the representation reexecuted, it means that the input changed
      // and we should initialize our streaming.
      vtkMultiBlockDataSet* metadata= vtkMultiBlockDataSet::SafeDownCast(
        inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));
      this->PriorityQueue->Initialize(metadata);
      }
    }

  this->ProcessedPiece = 0;
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    // Do the streaming independent "transformation" of the data here, in our
    // case, generate the polydata from the input.

    // Streaming and "flip-book" caching don't make much sense together. Hence
    // we don't complicate the logic with adding support for caching for
    // animation playback.

    vtkNew<vtkPVGeometryFilter> geomFilter;
    geomFilter->SetUseOutline(this->UseOutline? 1 : 0);

    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    geomFilter->SetInputData(input);
    geomFilter->Update();
    if (!this->GetInStreamingUpdate())
      {
      vtkDataObject* output = geomFilter->GetOutputDataObject(0);
      if (!output->IsA("vtkMultiBlockDataSet"))
        {
        vtkNew<vtkMultiBlockDataSet> mb;
        mb->SetBlock(0, output);
        this->ProcessedData = mb.GetPointer();
        }
      else
        {
        this->ProcessedData = vtkMultiBlockDataSet::SafeDownCast(output);
        }
      assert(this->ProcessedData.GetPointer());

      // Collect data bounds.
      this->DataBounds.Reset();
      vtkCompositeDataIterator* iter = this->ProcessedData->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
        if (ds)
          {
          this->DataBounds.AddBounds(ds->GetBounds());
          }
        }
      iter->Delete();
      }
    else
      {
      this->ProcessedPiece = geomFilter->GetOutputDataObject(0);
      }
    }
  else
    {
    // create an empty dataset. This is needed so that view knows what dataset
    // to expect from the other processes on this node.
    this->ProcessedData = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    this->DataBounds.Reset();
    }

  if (!this->GetInStreamingUpdate())
    {
    this->RenderedData = 0;

    // provide the mapper with an empty input. This is needed only because
    // mappers die when input is NULL, currently.
    vtkNew<vtkMultiBlockDataSet> tmp;
    this->Mapper->SetInputDataObject(tmp.GetPointer());
    }

  return this->Superclass::RequestData(rqst, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkStreamingParticlesRepresentation::StreamingUpdate(const double view_planes[24])
{
  assert(this->InStreamingUpdate == false);
  if (!this->PriorityQueue->IsEmpty())
    {
    this->InStreamingUpdate = true;
    vtkStreamingStatusMacro(<< this << ": doing streaming-update.")

    // update the priority queue, if needed.
    this->PriorityQueue->Update(view_planes);

    // This ensure that the representation re-executes.
    this->MarkModified();

    // Execute the pipeline.
    this->Update();

    this->InStreamingUpdate = false;
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
int vtkStreamingParticlesRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
bool vtkStreamingParticlesRepresentation::AddToView(vtkView* view)
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
bool vtkStreamingParticlesRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StreamingCapablePipeline: " << this->StreamingCapablePipeline
    << endl;
}


//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::SetColorAttributeType(int type)
{
  switch (type)
    {
  case vtkGeometryRepresentation::CELL_DATA:
    this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case vtkGeometryRepresentation::POINT_DATA:
  default:
    this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::SetColorArrayName(const char* arrayname)
{
  if (arrayname && arrayname[0])
    {
    this->Mapper->SetScalarVisibility(1);
    this->Mapper->SelectColorArray(arrayname);
    this->Mapper->SetUseLookupTableScalarRange(1);
    }
  else
    {
    this->Mapper->SetScalarVisibility(0);
    const char* null = NULL;
    this->Mapper->SelectColorArray(null);
    }

}

//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  this->Mapper->SetLookupTable(lut);
}

//----------------------------------------------------------------------------
void vtkStreamingParticlesRepresentation::SetPointSize(double val)
{
  this->Actor->GetProperty()->SetPointSize(val);
}
