/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloCountFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloCountFilter.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCosmoHaloCountFilter.h"

#include "vtkStringArray.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTrivialProducer.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkCosmoHaloClassFilter.h"

vtkCxxRevisionMacro(vtkCosmoHaloCountFilter, "1.2");
vtkStandardNewMacro(vtkCosmoHaloCountFilter);

//----------------------------------------------------------------------------
void vtkCosmoHaloCountFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkCosmoHaloCountFilter::vtkCosmoHaloCountFilter() : Superclass()
{
  this->CurrentTimeIndex = 0;
  this->NumberOfClasses = 0;
  this->NumberOfTimeSteps = 0;

  this->nameArray = vtkStringArray::New();
}

//----------------------------------------------------------------------------
vtkCosmoHaloCountFilter::~vtkCosmoHaloCountFilter()
{
  this->nameArray->Delete();
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::ProcessRequest(vtkInformation* request,
                                            vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)
{
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::RequestInformation(vtkInformation* ,
                                                vtkInformationVector** inputVector,
                                                vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps = 
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 0;
    }

  if ( inInfo->Has(vtkCosmoHaloClassFilter::OUTPUT_NUMBER_OF_CLASSES()) )
    {
    this->NumberOfClasses = inInfo->Get(vtkCosmoHaloClassFilter::OUTPUT_NUMBER_OF_CLASSES());
    }
  else
    {
    this->NumberOfClasses = 0;
    }

  // The output of this filter does not contain a specific time, rather 
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::RequestUpdateExtent(vtkInformation*,
                                                 vtkInformationVector** inputVector,
                                                 vtkInformationVector*)
{
  // get the requested update extent
  double *inTimes = inputVector[0]->GetInformationObject(0)
    ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
    {
    double timeReq[1];
    timeReq[0] = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), timeReq, 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoHaloCountFilter::RequestData(vtkInformation* request,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps == 0) 
    {
    vtkErrorMacro("No time steps in input data!");
    return 0;
    }
  if (this->NumberOfClasses < 1)
    {
    vtkErrorMacro("No classes in input data");
    return 0;
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *input= vtkUnstructuredGrid::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkRectilinearGrid *output= vtkRectilinearGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIntArray *iHID = vtkIntArray::SafeDownCast(input->GetPointData()->GetArray("hID"));
  if (iHID == NULL)
    {
    vtkErrorMacro("The input data set doesn't have hID!");
    return 0;
    }

  vtkIntArray *haloClass = vtkIntArray::SafeDownCast(input->GetPointData()->GetArray("haloClass"));
  if (haloClass == NULL)
    {
    vtkErrorMacro("The input data set doesn't have haloClass!");
    return 0;
    }

  // is this the first request
  if (!this->CurrentTimeIndex)
    {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

    // allocate and initialize output data
    if( !this->AllocateOutputData(inInfo, outInfo))
      {
      return 0;
      }
    }

 
  //
  // Read in particles
  //
  int npart = input->GetNumberOfPoints();
  vtkDebugMacro(<< npart << " input particles\n");

  int *counts = new int[this->NumberOfClasses];
  memset(counts, 0, sizeof(int)*this->NumberOfClasses);

  double hid_range[2];
  iHID->GetRange(hid_range);
  int maxhid = static_cast<int> (hid_range[1]);
  int *halos = new int[maxhid+1];
  for (int i = 0; i <= maxhid; i++)
    {
    halos[i] = -1;
    }

  for (int i = 0; i < npart; i++)
    {
    int halo_id = iHID->GetValue(i);
    if (halo_id >= 0) 
      {
      halos[halo_id] = haloClass->GetValue(i);
      }
    }

  for (int i = 0; i <= maxhid; i++)
    {
    int hc = halos[i];
    if (hc >= 0)
      {
      counts[hc]++;
      }
    }

  for (int i = 0; i < this->NumberOfClasses; i++)
    {
    vtkIntArray *dataArray = vtkIntArray::SafeDownCast(
      output->GetPointData()->GetArray(this->nameArray->GetValue(i)));
    if (dataArray == NULL)
      {
      vtkErrorMacro(<< "Data array doesn't exist for halo class " << i << endl);
      return 0;
      }
    dataArray->SetValue(this->CurrentTimeIndex, counts[i]);
    }

  delete[] counts;
  delete[] halos;


  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
    {
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
    }

  return 1;
}

int vtkCosmoHaloCountFilter::AllocateOutputData(vtkInformation* inInfo, vtkInformation* outInfo)
{
  double* inTimes = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (inTimes == NULL)
    {
    vtkErrorMacro("No time steps in the input data!");
    return 0;
    }

  vtkDoubleArray* outCoords = vtkDoubleArray::New();
  outCoords->SetNumberOfTuples(this->NumberOfTimeSteps);
  for (int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    outCoords->SetValue(i, inTimes[i]);
    }

  vtkRectilinearGrid* output = vtkRectilinearGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointData* outPD=output->GetPointData();

  output->SetDimensions(this->NumberOfTimeSteps, 1, 1);
  output->SetXCoordinates(outCoords);
  outCoords->SetName("timesteps");
  outPD->AddArray(outCoords);
  outCoords->Delete();
  outCoords = vtkDoubleArray::New();
  outCoords->InsertNextValue(0);
  output->SetYCoordinates(outCoords);
  output->SetZCoordinates(outCoords);
  outCoords->Delete();

  char buffer[64];
  this->nameArray->Reset();
  for (int i = 0; i < this->NumberOfClasses; i++)
    {
    sprintf(buffer, "halo_class%d", i);
    this->nameArray->InsertNextValue(buffer);
    vtkIntArray* dataArray = vtkIntArray::New();
    dataArray->SetName(buffer);
    dataArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    outPD->AddArray(dataArray);
    dataArray->Delete();
    }

  return 1;
}
