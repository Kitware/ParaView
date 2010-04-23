/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGlyphFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGlyphFilter.h"

#include "vtkAppendPolyData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkCompositeDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkPVGlyphFilter);

//-----------------------------------------------------------------------------
vtkPVGlyphFilter::vtkPVGlyphFilter()
{
  this->SetColorModeToColorByScalar();
  this->SetScaleModeToScaleByVector();
  this->MaskPoints = vtkMaskPoints::New();
  this->RandomMode = this->MaskPoints->GetRandomMode();
  this->MaximumNumberOfPoints = 5000;
  this->NumberOfProcesses = vtkMultiProcessController::GetGlobalController() ?
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() : 1;
  this->UseMaskPoints = 1;
  this->InputIsUniformGrid = 0;

  this->BlockOnRatio = 0;
  this->BlockMaxNumPts = 0;
  this->BlockPointCounter = 0;
  this->BlockNumGlyphedPts = 0;
  this->BlockGlyphAllPoints=0;
}

//-----------------------------------------------------------------------------
vtkPVGlyphFilter::~vtkPVGlyphFilter()
{
  if(this->MaskPoints)
    {
    this->MaskPoints->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::SetRandomMode(int mode)
{
  if (mode==this->MaskPoints->GetRandomMode())
    {
    // no change
    return;
    }
  // Store random mode to so that we don't have to call
  // MaskPoints->GetRandomMode() in tight loop.
  this->MaskPoints->SetRandomMode(mode);
  this->RandomMode = mode;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkPVGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::SetUseMaskPoints(int useMaskPoints)
{
  if (useMaskPoints==this->UseMaskPoints)
    {
    // no change
    return;
    }
  this->UseMaskPoints=useMaskPoints;
  this->BlockGlyphAllPoints= !this->UseMaskPoints;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilter::FillInputPortInformation(int port,
                                               vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");

  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVGlyphFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
vtkIdType vtkPVGlyphFilter::GatherTotalNumberOfPoints(vtkIdType localNumPts)
{
  // Although this is not perfectly process invariant, it is better
  // than we had before (divide by number of processes).
  vtkIdType totalNumPts = localNumPts;
  vtkMultiProcessController *controller = 
    vtkMultiProcessController::GetGlobalController();
  if (controller)
    {
    vtkIdType tmp;
    // This could be done much easier with MPI specific calls.
    if (controller->GetLocalProcessId() == 0)
      {
      int i;
      // Sum points on all processes.
      for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
        {
        controller->Receive(&tmp, 1, i, vtkProcessModule::GlyphNPointsGather);
        totalNumPts += tmp;
        }
      // Send results back to all processes.
      for (i = 1; i < controller->GetNumberOfProcesses(); ++i)
        {
        controller->Send(&totalNumPts, 1, 
                         i, vtkProcessModule::GlyphNPointsScatter);
        }
      }
    else
      {
      controller->Send(&localNumPts, 1, 
                       0, vtkProcessModule::GlyphNPointsGather);
      controller->Receive(&totalNumPts, 1, 
                          0, vtkProcessModule::GlyphNPointsScatter);
      }
    }

  return totalNumPts;
}

//-----------------------------------------------------------------------------
int vtkPVGlyphFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->BlockOnRatio = 0;

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet *hdInput = 
    vtkCompositeDataSet::SafeDownCast(input);
  if (hdInput) 
    {
    return this->RequestCompositeData(request, inputVector, outputVector);
    }
  // turn on all (we restore before returning). This affects
  // our implementation of IsPointVisible, for non-composite
  // data, we use vtkMaskFilter and glyph every point it 
  // returns.
  this->BlockGlyphAllPoints=1;

  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!dsInput)
    {
    if (input)
      {
      vtkErrorMacro("This filter cannot process input of type: "
                    << input->GetClassName());
      }
    return 0;
    }

  // Glyph everything? 
  if (!this->UseMaskPoints)
    {
    // yes.
    int retVal
      = this->Superclass::RequestData(request, inputVector, outputVector);
    this->BlockGlyphAllPoints= !this->UseMaskPoints;
    return retVal;
    }

  // Glyph a subset.
  vtkIdType maxNumPts = this->MaximumNumberOfPoints;
  vtkIdType numPts = dsInput->GetNumberOfPoints();
  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  // What fraction of the points will this processes get allocated?
  maxNumPts
    = (vtkIdType)((double)(maxNumPts)*(double)(numPts)/(double)(totalNumPts));

  maxNumPts = (maxNumPts < 1) ? 1 : maxNumPts;

  vtkInformationVector* inputVs[2];

  vtkInformationVector* inputV = inputVector[0];
  inputVs[0] = vtkInformationVector::New();
  inputVs[0]->SetNumberOfInformationObjects(1);
  vtkInformation* newInInfo = vtkInformation::New();
  newInInfo->Copy(inputV->GetInformationObject(0));
  inputVs[0]->SetInformationObject(0, newInInfo);
  newInInfo->Delete();
  inputVs[1] = inputVector[1];

  int retVal
    = this->MaskAndExecute(numPts, maxNumPts, dsInput,
                           request, inputVs, outputVector);
  this->BlockGlyphAllPoints= !this->UseMaskPoints;

  inputVs[0]->Delete();
  return retVal;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilter::MaskAndExecute(vtkIdType numPts, vtkIdType maxNumPts,
                                     vtkDataSet* input,
                                     vtkInformation* request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)

{
  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->MaskPoints->SetInput(inputCopy);
  inputCopy->Delete();

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->MaskPoints->SetMaximumNumberOfPoints(maxNumPts);
  this->MaskPoints->SetOnRatio(numPts / maxNumPts);

  vtkInformation *maskPointsInfo =
    this->MaskPoints->GetExecutive()->GetOutputInformation(0);
  maskPointsInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  maskPointsInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  maskPointsInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  this->MaskPoints->Update();

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkDataObject::DATA_OBJECT(), this->MaskPoints->GetOutput());

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// We are overloading this so that blanking will be supported
// otehrwise we could use vtkMaskPoints filter.
int vtkPVGlyphFilter::IsPointVisible(vtkDataSet* ds, vtkIdType ptId)
{
  if (this->BlockGlyphAllPoints==1)
    {
    return 1;
    }

  // check if point has been blanked. If so skip it and
  // do not count it.
  if (this->InputIsUniformGrid)
    {
    vtkUniformGrid* ug = static_cast<vtkUniformGrid*>(ds);
    if(!ug->IsPointVisible(ptId))
      {
      return 0;
      }
    }

  // Have we glyphed enough points yet? And are we
  // at the next point? If so we'll return 1 indicating
  // that this point should be glyphed and compute the
  // next point.
  int pointIsVisible=0;
  if (this->BlockNumGlyphedPts<this->BlockMaxNumPts 
      && this->BlockPointCounter==this->BlockNextPoint)
    {
    this->BlockNumGlyphedPts++;
    if (this->RandomMode)
      {
      double r
      = vtkMath::Random(this->BlockSampleStride,2.0*this->BlockSampleStride-1.0);
      this->BlockNextPoint+=static_cast<vtkIdType>(r+0.5);
      }
    else
      {
      this->BlockNextPoint+=this->BlockSampleStride;
      }
    pointIsVisible=1;
    }

  // Count all non-blanked points.
  ++this->BlockPointCounter;
  return pointIsVisible;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilter::RequestCompositeData(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  // input
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkCompositeDataSet *hdInput
  =vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  //
  // if (hdInput->GetNumberOfChildren()>this->MaximumNumberOfPoints)
  //   {
  //   vtkErrorMacro("Small sample size. "
  //                 "At least one point per block will be used.");
  //   }

  // output
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output
    = vtkPolyData::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    vtkErrorMacro("Expected vtkPolyData in output.");
    return 0;
    }

  // Get number of points we have in this block
  // and the number of points in all blocks.
  vtkIdType numPts = hdInput->GetNumberOfPoints();
  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  vtkAppendPolyData* append = vtkAppendPolyData::New();
  int numInputs = 0;

  vtkInformationVector* inputVs[2];

  vtkInformationVector* inputV = inputVector[0];
  inputVs[0] = vtkInformationVector::New();
  inputVs[0]->SetNumberOfInformationObjects(1);
  vtkInformation* newInInfo = vtkInformation::New();
  newInInfo->Copy(inputV->GetInformationObject(0));
  inputVs[0]->SetInformationObject(0, newInInfo);
  newInInfo->FastDelete();
  inputVs[1] = inputVector[1];

  int retVal = 1;
  this->InputIsUniformGrid = 0;

  vtkCompositeDataIterator* iter = hdInput->NewIterator();

  while(!iter->IsDoneWithTraversal())
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds)
      {
      vtkPolyData* tmpOut = vtkPolyData::New();

      // Uniform grids might be blanked, we make a note if we 
      // have a uniform grid to facilitate blanking friendly
      // glyph sampling.
      if (ds->IsA("vtkUniformGrid"))
        {
        this->InputIsUniformGrid = 1;
        }
      else
        {
        this->InputIsUniformGrid = 0;
        }

      // Certain AMR data sets provide blanking information
      // We will skip blanked points and glyph what's
      // left if any.
      vtkIdType numBlankedPts = 0;
      vtkInformation* blockInfo = iter->GetCurrentMetaData();
      if (blockInfo)
        {
        if
        (blockInfo->Has(vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS()))
          {
          numBlankedPts
            = blockInfo->Get(vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS());
          }
        }

      // When masking points we evenly sample, skipping a fixed number
      // in between each glyph or we vary our stride randomly between
      // 1 and 2*stride-1. This is *not* a random sampling of the points
      double nPtsNotBlanked
        = static_cast<double>(ds->GetNumberOfPoints() - numBlankedPts);
      double nPtsVisibleOverAll
        = static_cast<double>(this->MaximumNumberOfPoints);
      double nPtsInDataSet
        = static_cast<double>(totalNumPts);
      double fractionOfPtsInBlock = nPtsNotBlanked/nPtsInDataSet;
      double nPtsVisibleOverBlock = nPtsVisibleOverAll*fractionOfPtsInBlock;
      nPtsVisibleOverBlock 
        = nPtsVisibleOverBlock<1.0 ? 1.0 : nPtsVisibleOverBlock;
      nPtsVisibleOverBlock = (  (nPtsVisibleOverBlock > nPtsNotBlanked)
                              ? nPtsNotBlanked : nPtsVisibleOverBlock );
      double stride = nPtsNotBlanked/nPtsVisibleOverBlock;
      if (this->UseMaskPoints)
        {
        this->BlockSampleStride=static_cast<vtkIdType>(stride+0.5);
        }
      else
        {
        this->BlockSampleStride=1;
        }
      // We will glyph this many points.
      this->BlockMaxNumPts = static_cast<vtkIdType>(nPtsVisibleOverBlock);
      //
      this->BlockPointCounter = 0;
      this->BlockNumGlyphedPts = 0;
      // Identify the first point to glyph.
      if (this->MaskPoints->GetRandomMode())
        {
        double r
          = vtkMath::Random(0.0,this->BlockSampleStride-1.0);
        this->BlockNextPoint=static_cast<vtkIdType>(r+0.5);
        }
      else
        {
        this->BlockNextPoint=0;
        }

      // We have set all ofthe parameters that will be used in 
      // our overloaded IsPoitVisible. Now let the glypher take over.
      newInInfo->Set(vtkDataObject::DATA_OBJECT(), ds);
      retVal =
        this->Superclass::RequestData(request, inputVs, outputVector);
      // Accumulate the results.
      tmpOut->ShallowCopy(output);
      append->AddInput(tmpOut);

      // Call FastDelete() instead of Delete() to avoid garbage
      // collection checks. This improves the preformance significantly
      tmpOut->FastDelete();

      // Glypher failed, so we skip the rest and fail as well.
      if (!retVal)
        {
        vtkErrorMacro("vtkGlyph3D failed.");
        iter->Delete();
        inputVs[0]->Delete();
        append->Delete();
        return 0;
        }
      numInputs++;
      }
    iter->GoToNextItem();
    }

  // copy the accumulated results to the output.
  if (numInputs > 0)
    {
    append->Update();
    output->ShallowCopy(append->GetOutput());
    }

  // clean up
  iter->Delete();
  inputVs[0]->Delete();
  append->Delete();

  return retVal;
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->MaskPoints, "MaskPoints");
}

//-----------------------------------------------------------------------------
void vtkPVGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "MaximumNumberOfPoints: " << this->GetMaximumNumberOfPoints()
     << endl;

  os << indent << "UseMaskPoints: " << (this->UseMaskPoints?"on":"off") << endl;

  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
