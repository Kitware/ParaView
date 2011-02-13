/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialProducer.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrivialProducer.h"

#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVTrivialExtentTranslator.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPVTrivialProducer);
//----------------------------------------------------------------------------
vtkPVTrivialProducer::vtkPVTrivialProducer()
{
  this->PVExtentTranslator = vtkPVTrivialExtentTranslator::New();
  vtkStreamingDemandDrivenPipeline::SafeDownCast(
    this->GetExecutive())->SetExtentTranslator(0, this->PVExtentTranslator);

  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
}

//----------------------------------------------------------------------------
vtkPVTrivialProducer::~vtkPVTrivialProducer()
{
  if (this->PVExtentTranslator)
    {
    this->PVExtentTranslator->SetDataSet(0);
    this->PVExtentTranslator->Delete();
    this->PVExtentTranslator = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::SetOutput(vtkDataObject* output)
{
  this->Superclass::SetOutput(output);

  if (this->PVExtentTranslator)
    {
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      this->GetExecutive())->SetExtentTranslator(0, this->PVExtentTranslator);
    this->PVExtentTranslator->SetDataSet(vtkDataSet::SafeDownCast(output));
    }
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::GatherExtents()
{
  if(this->PVExtentTranslator)
    {
    this->PVExtentTranslator->GatherExtents();
    }
}

//----------------------------------------------------------------------------
int
vtkPVTrivialProducer::ProcessRequest(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  if (!this->Superclass::ProcessRequest(request, inputVector, outputVector))
    {
    return 0;
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()) &&
    this->Output)
    {
    vtkInformation* outputInfo = outputVector->GetInformationObject(0);
    vtkInformation* dataInfo = this->Output->GetInformation();
    if(dataInfo->Get(vtkDataObject::DATA_EXTENT_TYPE()) == VTK_3D_EXTENT)
      {
      if (this->WholeExtent[0] <= this->WholeExtent[1] &&
        this->WholeExtent[2] <= this->WholeExtent[3] &&
        this->WholeExtent[4] <= this->WholeExtent[5])
        {
        outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
          this->WholeExtent, 6);
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->PVExtentTranslator,
    "PVExtentTranslator");
}

//----------------------------------------------------------------------------
void vtkPVTrivialProducer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
