/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractVOI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractVOI.h"

#include "vtkExtentTranslator.h"
#include "vtkExtractGrid.h"
#include "vtkExtractVOI.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkPVExtractVOI, "1.6");
vtkStandardNewMacro(vtkPVExtractVOI);

//----------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkPVExtractVOI::vtkPVExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_LARGE_INTEGER;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;
  
  this->IncludeBoundary = 0;

  this->ExtractGrid = vtkExtractGrid::New();
  this->ExtractVOI  = vtkExtractVOI::New();
  this->ExtractRG   = vtkExtractRectilinearGrid::New();
}

//----------------------------------------------------------------------------
vtkPVExtractVOI::~vtkPVExtractVOI()
{
  if(this->ExtractVOI)
    {
    this->ExtractVOI->Delete();
    }
  if(this->ExtractGrid)
    {
    this->ExtractGrid->Delete();
    }
  if(this->ExtractRG)
    {
    this->ExtractRG->Delete();
    }
}

//----------------------------------------------------------------------------
template <class FilterType, class InputType>
void vtkPVExtractVOIComputeInputUpdateExtents(
  FilterType* filter, InputType* input, vtkDataObject* output, vtkPVExtractVOI* self)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->SetInput(input);
  vtkInformation* innerInfo = filter->GetOutput()->GetPipelineInformation();
  vtkInformation* outerInfo = output->GetPipelineInformation();
  innerInfo->CopyEntry(
    outerInfo, vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  innerInfo->CopyEntry(
    outerInfo, vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED());
  filter->GetOutput()->PropagateUpdateExtent();
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestUpdateExtent(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = 
    vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = 
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    this->ExtractVOI->GetOutput()->ShallowCopy(output);
    this->ExtractVOI->GetOutput()->SetUpdateExtent(output->GetUpdateExtent());
    vtkPVExtractVOIComputeInputUpdateExtents(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      output,
      this);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    this->ExtractGrid->GetOutput()->ShallowCopy(output);
    this->ExtractGrid->GetOutput()->SetUpdateExtent(output->GetUpdateExtent());
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIComputeInputUpdateExtents(
      this->ExtractGrid, 
      vtkStructuredGrid::SafeDownCast(input),
      output,
      this);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    this->ExtractRG->GetOutput()->ShallowCopy(output);
    this->ExtractRG->GetOutput()->SetUpdateExtent(output->GetUpdateExtent());
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIComputeInputUpdateExtents(
      this->ExtractRG, 
      vtkRectilinearGrid::SafeDownCast(input),
      output,
      this);
    }

  // We can handle anything.
  input->SetRequestExactExtent(0);

  return 1;
}

//----------------------------------------------------------------------------
template <class FilterType, class InputType>
void vtkPVExtractVOIExecuteInformation(
  FilterType* filter, InputType* input, vtkPVExtractVOI* self)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->SetInput(input);
  filter->GetOutput()->UpdateInformation();
  self->GetOutput()->ShallowCopy(filter->GetOutput());
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = 
    vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = 
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));


  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    vtkPVExtractVOIExecuteInformation(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      this);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteInformation(
      this->ExtractGrid, 
      vtkStructuredGrid::SafeDownCast(input),
      this);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteInformation(
      this->ExtractRG, 
      vtkRectilinearGrid::SafeDownCast(input),
      this);
    }

  return 1;
}

//----------------------------------------------------------------------------
template <class FilterType, class InputType>
void vtkPVExtractVOIExecuteData(
  FilterType* filter, InputType* input, vtkPVExtractVOI* self)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->SetInput(input);
  filter->Update();
  self->GetOutput()->ShallowCopy(filter->GetOutput());
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = 
    vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet* input = 
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
    {
    vtkPVExtractVOIExecuteData(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      this);
    }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteData(
      this->ExtractGrid, 
      vtkStructuredGrid::SafeDownCast(input),
      this);
    }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteData(
      this->ExtractRG, 
      vtkRectilinearGrid::SafeDownCast(input),
      this);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::SetSampleRateI(int ratei)
{
  if (this->SampleRate[0] == ratei)
    {
    return;
    }
  
  this->SampleRate[0] = ratei;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::SetSampleRateJ(int ratej)
{
  if (this->SampleRate[1] == ratej)
    {
    return;
    }
  
  this->SampleRate[1] = ratej;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::SetSampleRateK(int ratek)
{
  if (this->SampleRate[2] == ratek)
    {
    return;
    }
  
  this->SampleRate[2] = ratek;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVExtractVOI::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ExtractVOI, "ExtractVOI");
  vtkGarbageCollectorReport(collector, this->ExtractGrid, "ExtractGrid");
  vtkGarbageCollectorReport(collector, this->ExtractRG, "ExtractRG");
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", " 
     << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", " 
     << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", " 
     << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", "
               << this->SampleRate[1] << ", "
               << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: " 
     << (this->IncludeBoundary ? "On\n" : "Off\n");
}
