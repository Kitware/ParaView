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
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkPVExtractVOI, "1.3");
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
const char* vtkPVExtractVOI::DetermineOutputType()
{
  vtkDataSet* input = this->GetInput();

  if (!input)
    {
    return NULL;
    }

  vtkImageData* imInput = vtkImageData::SafeDownCast(input);
  if (imInput)
    {
    return "vtkImageData";
    }
  else
    {
    vtkStructuredGrid* sgInput = vtkStructuredGrid::SafeDownCast(input);
    if (sgInput)
      {
      return "vtkStructuredGrid";
      }
    else
      {
      vtkRectilinearGrid* rgInput = vtkRectilinearGrid::SafeDownCast(input);
      if (rgInput)
        {
        return "vtkRectilinearGrid";
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
template <class FilterType, class InputType>
void vtkPVExtractVOIComputeInputUpdateExtents(
  FilterType* filter, InputType* input, vtkDataObject* output, vtkPVExtractVOI* self)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->SetInput(input);
  filter->PropagateUpdateExtent(output);
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::ComputeInputUpdateExtents(vtkDataObject *output)
{
  const char* outputType = this->DetermineOutputType();
  if (!outputType)
    {
    return;
    }
  vtkDataSet* input = this->GetInput();
  if (strcmp(outputType, "vtkImageData") == 0)
    {
    this->ExtractVOI->GetOutput()->ShallowCopy(output);
    this->ExtractVOI->GetOutput()->SetUpdateExtent(output->GetUpdateExtent());
    vtkPVExtractVOIComputeInputUpdateExtents(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      output,
      this);
    }
  else if (strcmp(outputType, "vtkStructuredGrid") == 0)
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
  else if (strcmp(outputType, "vtkRectilinearGrid") == 0)
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

  int extent[6];
  input->GetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
template <class FilterType, class InputType>
void vtkPVExtractVOIExecuteInformation(
  FilterType* filter, InputType* input, vtkPVExtractVOI* self)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->SetInput(input);
  filter->UpdateInformation();
  self->GetOutput()->ShallowCopy(filter->GetOutput());
}

//----------------------------------------------------------------------------
void vtkPVExtractVOI::ExecuteInformation()
{
  this->Superclass::ExecuteInformation();

  const char* outputType = this->DetermineOutputType();
  if (!outputType)
    {
    return;
    }
  vtkDataSet* input = this->GetInput();
  if (strcmp(outputType, "vtkImageData") == 0)
    {
    vtkPVExtractVOIExecuteInformation(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      this);
    }
  else if (strcmp(outputType, "vtkStructuredGrid") == 0)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteInformation(
      this->ExtractGrid, 
      vtkStructuredGrid::SafeDownCast(input),
      this);
    }
  else if (strcmp(outputType, "vtkRectilinearGrid") == 0)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteInformation(
      this->ExtractRG, 
      vtkRectilinearGrid::SafeDownCast(input),
      this);
    }
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
void vtkPVExtractVOI::ExecuteData(vtkDataObject*)
{
  const char* outputType = this->DetermineOutputType();
  if (!outputType)
    {
    return;
    }
  vtkDataSet* input = this->GetInput();
  if (strcmp(outputType, "vtkImageData") == 0)
    {
    vtkPVExtractVOIExecuteData(
      this->ExtractVOI, 
      vtkImageData::SafeDownCast(input),
      this);
    }
  else if (strcmp(outputType, "vtkStructuredGrid") == 0)
    {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteData(
      this->ExtractGrid, 
      vtkStructuredGrid::SafeDownCast(input),
      this);
    }
  else if (strcmp(outputType, "vtkRectilinearGrid") == 0)
    {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIExecuteData(
      this->ExtractRG, 
      vtkRectilinearGrid::SafeDownCast(input),
      this);
    }
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
  collector->ReportReference(this->ExtractVOI, "ExtractVOI");
  collector->ReportReference(this->ExtractGrid, "ExtractGrid");
  collector->ReportReference(this->ExtractRG, "ExtractRG");
}

//-----------------------------------------------------------------------------
void vtkPVExtractVOI::RemoveReferences()
{
  if(this->ExtractVOI)
    {
    this->ExtractVOI->Delete();
    this->ExtractVOI = 0;
    }
  if(this->ExtractGrid)
    {
    this->ExtractGrid->Delete();
    this->ExtractGrid = 0;
    }
  if(this->ExtractRG)
    {
    this->ExtractRG->Delete();
    this->ExtractRG = 0;
    }
  this->Superclass::RemoveReferences();
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
