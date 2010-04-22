/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIsoVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIsoVolume.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkGenericClip.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPVClipDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkIsoVolume, "1.1");
vtkStandardNewMacro(vtkIsoVolume);

//----------------------------------------------------------------------------
// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper
vtkIsoVolume::vtkIsoVolume()
{
  this->LowerThreshold         = 0.0;
  this->UpperThreshold         = 1.0;
  this->UsingPointScalars      = 0.0;

  this->LowerBoundClipDS = 0;
  this->UpperBoundClipDS = 0;
  this->Threshold        = 0;

  this->SetInputArrayToProcess(
    0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkIsoVolume::~vtkIsoVolume()
{
  // Release memory.
  if(this->LowerBoundClipDS)
    {
    this->LowerBoundClipDS->Delete();
    }
  if(this->UpperBoundClipDS)
    {
    this->UpperBoundClipDS->Delete();
    }
  if(this->Threshold)
    {
    this->Threshold->Delete();
    }
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkIsoVolume::ThresholdBetween(double lower, double upper)
{
  if(this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

int vtkIsoVolume::RequestData(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector)
{
  int retVal = 1;

  // Get the info objects.
  vtkInformation* inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataArray* inScalars = this->GetInputArrayToProcess(0,inputVector);
  vtkDataObject* outObj = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inScalars)
    {
    vtkDebugMacro(<<"No scalar data to threshold");
    return !retVal;
    }

  vtkDebugMacro(<<"Executing vtkIsoVolume filter");

  // Are we using pointScalars?
  vtkDataSet* input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numPts = input->GetNumberOfPoints();
  this->UsingPointScalars = (inScalars->GetNumberOfTuples() == numPts);

  // If not using point scalar we would like to extract volume using
  // vtkThreshold filter.
  if(!this->UsingPointScalars)
    {
    if(this->Threshold)
      {
      this->Threshold->Delete();
      }

    this->Threshold = vtkThreshold::New();
    this->Threshold->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, inScalars->GetName());
    this->Threshold->ThresholdBetween(
        this->LowerThreshold, this->UpperThreshold);
    retVal =
      this->Threshold->ProcessRequest(request, inputVector, outputVector);

    return retVal;
    }

  double* range = input->GetScalarRange();

  // Use the lower bound clip filter only if the data contained
  // has values lower or equal then the threshold.
  bool usingLowerBoundClipDS = false;
  if(range[0] <= this->LowerThreshold)
    {
    usingLowerBoundClipDS = true;

    if(this->LowerBoundClipDS)
      {
      this->LowerBoundClipDS->Delete();
      }

    this->LowerBoundClipDS = vtkPVClipDataSet::New();
    this->LowerBoundClipDS->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_POINTS, inScalars->GetName());
    this->LowerBoundClipDS->SetValue(this->LowerThreshold);
    this->LowerBoundClipDS->GenerateClipScalarsOff();
    retVal = this->LowerBoundClipDS->ProcessRequest(request, inputVector,
                                                    outputVector);
    }

  // Its useless to create upper bound clip filter
  // if range[1] is lower than upper threshold.
  if(range[1] > this->UpperThreshold)
    {
    if(this->UpperBoundClipDS)
      {
      this->UpperBoundClipDS->Delete();
      }

    this->UpperBoundClipDS = vtkPVClipDataSet::New();
    this->UpperBoundClipDS->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_POINTS, inScalars->GetName());
    this->UpperBoundClipDS->SetValue(this->UpperThreshold);
    this->UpperBoundClipDS->GenerateClipScalarsOff();
    this->UpperBoundClipDS->InsideOutOn();

    vtkInformation*       lbcdsInfo(0);
    vtkInformationVector* outputInfoVec(0);
    vtkUnstructuredGrid*  output(0);
    vtkInformation*       outInfo2(0);

    if(usingLowerBoundClipDS)
      {
      lbcdsInfo = outputVector->GetInformationObject(0);

      // If there is a valid output create new output
      // information and information vector object.
      if(lbcdsInfo->Get(vtkDataObject::DATA_OBJECT()))
        {
        output = vtkUnstructuredGrid::New();
        outInfo2 = vtkInformation::New();
        outInfo2->Copy(outInfo);
        outInfo2->Set(vtkDataObject::DATA_OBJECT(), output);
        outputInfoVec = vtkInformationVector::New();
        outputInfoVec->SetNumberOfInformationObjects(1);
        outputInfoVec->SetInformationObject(0, outInfo2);
        }
      else
        {
        // If not a valid output data object then set
        // the flag to false.
        usingLowerBoundClipDS = false;
        }
      }


    // If not using the previous clip filter use original args.
    if(!usingLowerBoundClipDS && this->UpperBoundClipDS)
      {
      retVal =  this->UpperBoundClipDS->ProcessRequest(request, inputVector,
                                                       outputVector);
      }
    else if(usingLowerBoundClipDS && this->UpperBoundClipDS)
      {
      // If using the previous clip filter then use the newly created
      // information vector object and once done free up the memory.
      this->UpperBoundClipDS->ProcessRequest(request, &outputVector,
                                             outputInfoVec);
      vtkUnstructuredGrid::SafeDownCast(outObj)->ShallowCopy(output);
      output->Delete();
      outInfo2->Delete();
      outputInfoVec->Delete();
      }
    else
      {
      // Do nothing.
      }
    }

  return retVal;
}

//----------------------------------------------------------------------------
int vtkIsoVolume::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkIsoVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "UsingPointScalars: " << this->UsingPointScalars << "\n";

  (this->LowerBoundClipDS) ?
      os << indent << "LowerBoundClipDS: " << this->LowerBoundClipDS << "\n" :
      os << indent << "LowerBoundClipDS: " << "NULL" << "\n" ;

  (this->UpperBoundClipDS) ?
      os << indent << "UpperBoundClipDS: " << this->UpperBoundClipDS << "\n" :
      os << indent << "UpperBoundClipDS: " << "NULL" << "\n" ;

  (this->Threshold) ?
      os << indent << "Threshold: " << this->Threshold << "\n" :
      os << indent << "Threshold: " << "NULL" << "\n" ;
}

//----------------------------------------------------------------------------
int vtkIsoVolume::ProcessRequest(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  if(request->Has(
    vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    int retValLCF = 1;
    int retValUCF = 1;

    if(this->LowerBoundClipDS)
      {
      retValLCF = this->LowerBoundClipDS->ProcessRequest(
        request, inputVector, outputVector);
      }

    if(this->UpperBoundClipDS)
      {
      retValUCF = this->LowerBoundClipDS->ProcessRequest(
        request, inputVector, outputVector);
      }

    if(retValLCF != 1 && retValUCF != 1)
      {
      return 0;
      }
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
