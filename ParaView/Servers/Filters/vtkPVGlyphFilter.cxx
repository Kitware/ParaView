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
#include "vtkCompositeDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMaskPoints.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkPVGlyphFilter, "1.24");
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
  this->BlockNumPts = 0;
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
  if ( mode == this->MaskPoints->GetRandomMode() )
    {
    return;
    }
  this->MaskPoints->SetRandomMode(mode);
  // Store random mode to so that we don't have to call
  // MaskPoints->GetRandomMode() in tight loop.
  this->RandomMode = mode;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkPVGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
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
  info->Set(vtkCompositeDataPipeline::INPUT_REQUIRED_COMPOSITE_DATA_TYPE(), 
            "vtkCompositeDataSet");
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

  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(
      vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (hdInput) 
    {
    return this->RequestCompositeData(request, inputVector, outputVector);
    }

  if (!this->UseMaskPoints)
    {
    return this->Superclass::RequestData(request, inputVector, outputVector);
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkIdType maxNumPts = this->MaximumNumberOfPoints;
  vtkIdType numPts = input->GetNumberOfPoints();

  vtkIdType totalNumPts = this->GatherTotalNumberOfPoints(numPts);

  // What fraction of the points will this processes get allocated?
  maxNumPts = (vtkIdType)(
    (double)(maxNumPts)*(double)(numPts)/(double)(totalNumPts));
  
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
  
  int retVal = this->MaskAndExecute(numPts, maxNumPts, input,
                                    request, inputVs, outputVector);
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
int vtkPVGlyphFilter::IsPointVisible(vtkDataSet* ds, vtkIdType ptId)
{
  if (this->BlockOnRatio == 0)
    {
    return 1;
    }

  if (this->InputIsUniformGrid)
    {
    vtkUniformGrid* ug = static_cast<vtkUniformGrid*>(ds);
    if(!ug->IsPointVisible(ptId))
      {
      return 0;
      }
    }

  if (this->BlockNumPts < this->BlockMaxNumPts &&
      this->BlockPointCounter++ == this->BlockNextPoint)
    {
    this->BlockNumPts++;
    if (this->RandomMode)
      {
      this->BlockNextPoint += static_cast<vtkIdType>(
        1+2*vtkMath::Random()*this->BlockOnRatio);
      }
    else
      {
      this->BlockNextPoint += static_cast<vtkIdType>(this->BlockOnRatio);
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGlyphFilter::RequestCompositeData(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkMultiGroupDataSet *hdInput = vtkMultiGroupDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  vtkMultiGroupDataInformation* hdInfo = 
    hdInput->GetMultiGroupDataInformation();

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  vtkIdType maxNumPts = this->MaximumNumberOfPoints;
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

  unsigned int numGroups = hdInput->GetNumberOfGroups();

  for (unsigned int level=0; level<numGroups; level++)
    {
    unsigned int numDataSets = hdInput->GetNumberOfDataSets(level);
    for (unsigned int dataIdx=0; dataIdx<numDataSets; dataIdx++)
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(
        hdInput->GetDataSet(level, dataIdx));
      if (ds)
        {
        vtkPolyData* tmpOut = vtkPolyData::New();
        
        if (ds->IsA("vtkUniformGrid"))
          {
          this->InputIsUniformGrid = 1;
          }
        else
          {
          this->InputIsUniformGrid = 0;
          }

        vtkIdType numBlankedPts = 0;
        vtkInformation* blockInfo = hdInfo->GetInformation(level, dataIdx);
        if (blockInfo)
          {
          if (blockInfo->Has(
                vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS()))
            {
            numBlankedPts = blockInfo->Get(
              vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS());
            }
          }
        vtkIdType blockNumPts = ds->GetNumberOfPoints() - numBlankedPts;
        // What fraction of the points will this processes get allocated?
        vtkIdType blockMaxNumPts = (vtkIdType)(
          (double)(maxNumPts)*(double)(blockNumPts)/(double)(totalNumPts));
        this->BlockMaxNumPts = (blockMaxNumPts < 1) ? 1 : blockMaxNumPts;
        if (this->UseMaskPoints)
          {
          this->BlockOnRatio = blockNumPts/this->BlockMaxNumPts;
          }
        this->BlockPointCounter = 0;
        this->BlockNumPts = 0;
        if (this->MaskPoints->GetRandomMode())
          {
          this->BlockNextPoint = static_cast<vtkIdType>(
            1+vtkMath::Random()*this->BlockOnRatio);
          }
        else
          {
          this->BlockNextPoint = static_cast<vtkIdType>(1+this->BlockOnRatio);
          }
        
        //retVal = this->MaskAndExecute(blockNumPts, blockMaxNumPts, ds,
        //request, inputVs, outputVector);
        newInInfo->Set(vtkDataObject::DATA_OBJECT(), ds);
        retVal = 
          this->Superclass::RequestData(request, inputVs, outputVector);

        tmpOut->ShallowCopy(output);
        
        append->AddInput(tmpOut);
        
        // Call FastDelete() instead of Delete() to avoid garbage
        // collection checks. This improves the preformance significantly
        tmpOut->FastDelete();
        if (!retVal)
          {
          break;
          }
        numInputs++;
        }
      }
    }
  inputVs[0]->Delete();

  if (retVal)
    {
    if (numInputs > 0)
      {
      append->Update();
      }
    
    output->ShallowCopy(append->GetOutput());
    
    append->Delete();
    }

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
