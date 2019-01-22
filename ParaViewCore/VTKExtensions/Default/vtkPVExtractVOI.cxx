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

#include "vtkCompositeDataPipeline.h"
#include "vtkExtentTranslator.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"

#include "vtkPVConfig.h"
#if VTK_MODULE_ENABLE_VTK_FiltersParallelMPI
#include "vtkPExtractGrid.h"
#include "vtkPExtractRectilinearGrid.h"
#include "vtkPExtractVOI.h"
#else
#include "vtkExtractGrid.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkExtractVOI.h"
#endif

vtkStandardNewMacro(vtkPVExtractVOI);

//----------------------------------------------------------------------------
// Construct object to extract all of the input data.
vtkPVExtractVOI::vtkPVExtractVOI()
{
  this->VOI[0] = this->VOI[2] = this->VOI[4] = 0;
  this->VOI[1] = this->VOI[3] = this->VOI[5] = VTK_INT_MAX;

  this->SampleRate[0] = this->SampleRate[1] = this->SampleRate[2] = 1;

  this->IncludeBoundary = 0;

#if VTK_MODULE_ENABLE_VTK_FiltersParallelMPI
  this->ExtractGrid = vtkPExtractGrid::New();
  this->ExtractVOI = vtkPExtractVOI::New();
  this->ExtractRG = vtkPExtractRectilinearGrid::New();
#else
  this->ExtractGrid = vtkExtractGrid::New();
  this->ExtractVOI = vtkExtractVOI::New();
  this->ExtractRG = vtkExtractRectilinearGrid::New();
#endif
}

//----------------------------------------------------------------------------
vtkPVExtractVOI::~vtkPVExtractVOI()
{
  if (this->ExtractVOI)
  {
    this->ExtractVOI->Delete();
  }
  if (this->ExtractGrid)
  {
    this->ExtractGrid->Delete();
  }
  if (this->ExtractRG)
  {
    this->ExtractRG->Delete();
  }
}

//----------------------------------------------------------------------------
template <class FilterType>
void vtkPVExtractVOIProcessRequest(FilterType* filter, vtkPVExtractVOI* self,
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  filter->SetVOI(self->GetVOI());
  filter->SetSampleRate(self->GetSampleRate());
  filter->ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
  {
    vtkPVExtractVOIProcessRequest(this->ExtractVOI, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
  {
    vtkPVExtractVOIProcessRequest(this->ExtractGrid, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
  {
    vtkPVExtractVOIProcessRequest(this->ExtractRG, this, request, inputVector, outputVector);
  }

  // We can handle anything.
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
  {
    vtkPVExtractVOIProcessRequest(this->ExtractVOI, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
  {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIProcessRequest(this->ExtractGrid, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
  {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIProcessRequest(this->ExtractRG, this, request, inputVector, outputVector);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractVOI::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // This filter does not support subsampling composite datasets. Detect and
  // fail gracefully when this happens:
  bool isSubSampling =
    this->SampleRate[0] != 1 || this->SampleRate[1] != 1 || this->SampleRate[2] != 1;
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (isSubSampling && inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    vtkErrorMacro("The Extract Subset filter does not support subsampling on "
                  "composite datasets.");
    return 1;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (output->GetDataObjectType() == VTK_IMAGE_DATA)
  {
    vtkPVExtractVOIProcessRequest(this->ExtractVOI, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_STRUCTURED_GRID)
  {
    this->ExtractGrid->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIProcessRequest(this->ExtractGrid, this, request, inputVector, outputVector);
  }
  else if (output->GetDataObjectType() == VTK_RECTILINEAR_GRID)
  {
    this->ExtractRG->SetIncludeBoundary(this->IncludeBoundary);
    vtkPVExtractVOIProcessRequest(this->ExtractRG, this, request, inputVector, outputVector);
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
  this->Superclass::PrintSelf(os, indent);

  os << indent << "VOI: \n";
  os << indent << "  Imin,Imax: (" << this->VOI[0] << ", " << this->VOI[1] << ")\n";
  os << indent << "  Jmin,Jmax: (" << this->VOI[2] << ", " << this->VOI[3] << ")\n";
  os << indent << "  Kmin,Kmax: (" << this->VOI[4] << ", " << this->VOI[5] << ")\n";

  os << indent << "Sample Rate: (" << this->SampleRate[0] << ", " << this->SampleRate[1] << ", "
     << this->SampleRate[2] << ")\n";

  os << indent << "Include Boundary: " << (this->IncludeBoundary ? "On\n" : "Off\n");
}
