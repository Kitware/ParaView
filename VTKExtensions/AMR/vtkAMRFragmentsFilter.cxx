/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRFragmentsFilter.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2014 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkAMRFragmentsFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include "vtkDataObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNonOverlappingAMR.h"

#include "vtkAMRConnectivity.h"
#include "vtkExtractCTHPart.h"
#include "vtkPVAMRDualContour.h"
#include "vtkPVAMRFragmentIntegration.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkAMRFragmentsFilter);

vtkAMRFragmentsFilter::vtkAMRFragmentsFilter()
{
  this->SetNumberOfOutputPorts(2);

  this->ExtractSurface = false;
  this->UseWatertightSurface = false;
  this->IntegrateFragments = true;
  this->VolumeFractionSurfaceValue = 0.5;

  this->Producer = vtkTrivialProducer::New();
  this->Extract = vtkExtractCTHPart::New();
  this->Contour = vtkPVAMRDualContour::New();
  this->Connectivity = vtkAMRConnectivity::New();
  this->Integration = vtkPVAMRFragmentIntegration::New();

  this->Connectivity->SetInputConnection(0, this->Producer->GetOutputPort(0));
  this->Extract->SetInputConnection(0, this->Connectivity->GetOutputPort(0));
  this->Contour->SetInputConnection(0, this->Connectivity->GetOutputPort(0));
  this->Integration->SetInputConnection(0, this->Connectivity->GetOutputPort(0));
}

vtkAMRFragmentsFilter::~vtkAMRFragmentsFilter()
{
  if (this->Producer != 0)
  {
    this->Producer->Delete();
  }
  if (this->Extract != 0)
  {
    this->Extract->Delete();
  }
  if (this->Contour != 0)
  {
    this->Contour->Delete();
  }
  if (this->Connectivity != 0)
  {
    this->Connectivity->Delete();
  }
  if (this->Integration != 0)
  {
    this->Integration->Delete();
  }
}

void vtkAMRFragmentsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkAMRFragmentsFilter::AddInputVolumeArrayToProcess(const char* name)
{
  this->Connectivity->AddInputVolumeArrayToProcess(name);
  this->Integration->AddInputVolumeArrayToProcess(name);
  this->Contour->AddInputCellArrayToProcess(name);
  this->Extract->AddVolumeArrayName(name);
}

void vtkAMRFragmentsFilter::ClearInputVolumeArrayToProcess()
{
  this->Connectivity->ClearInputVolumeArrayToProcess();
  this->Integration->ClearInputVolumeArrayToProcess();
  this->Contour->ClearInputCellArrayToProcess();
  this->Extract->RemoveVolumeArrayNames();
}

void vtkAMRFragmentsFilter::AddInputMassArrayToProcess(const char* name)
{
  this->Integration->AddInputMassArrayToProcess(name);
  this->Modified();
}

void vtkAMRFragmentsFilter::ClearInputMassArrayToProcess()
{
  this->Integration->ClearInputMassArrayToProcess();
  this->Modified();
}

void vtkAMRFragmentsFilter::AddInputVolumeWeightedArrayToProcess(const char* name)
{
  this->Integration->AddInputVolumeWeightedArrayToProcess(name);
  this->Modified();
}

void vtkAMRFragmentsFilter::ClearInputVolumeWeightedArrayToProcess()
{
  this->Integration->ClearInputVolumeWeightedArrayToProcess();
  this->Modified();
}

void vtkAMRFragmentsFilter::AddInputMassWeightedArrayToProcess(const char* name)
{
  this->Integration->AddInputMassWeightedArrayToProcess(name);
  this->Modified();
}

void vtkAMRFragmentsFilter::ClearInputMassWeightedArrayToProcess()
{
  this->Integration->ClearInputMassWeightedArrayToProcess();
  this->Modified();
}

int vtkAMRFragmentsFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
      break;
    default:
      return 0;
  }

  return 1;
}

int vtkAMRFragmentsFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0: // the integrated fragments
    case 1: // the extracted surface
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      return 0;
  }

  return 1;
}

int vtkAMRFragmentsFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* amrInput =
    vtkNonOverlappingAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Producer->SetOutput(amrInput);

  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo = outputVector->GetInformationObject(1);
  vtkMultiBlockDataSet* mbdsOutput1 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  this->Connectivity->SetResolveBlocks(true);
  this->Connectivity->SetVolumeFractionSurfaceValue(this->VolumeFractionSurfaceValue);

  this->Contour->SetVolumeFractionSurfaceValue(this->VolumeFractionSurfaceValue);
  this->Extract->SetVolumeFractionSurfaceValue(this->VolumeFractionSurfaceValue);

  if (this->ExtractSurface)
  {
    this->Connectivity->SetPropagateGhosts(true);
    if (UseWatertightSurface)
    {
      this->Contour->Update();
      mbdsOutput1->ShallowCopy(this->Contour->GetOutput());
    }
    else
    {
      this->Extract->Update();
      mbdsOutput1->ShallowCopy(this->Extract->GetOutput());
    }
  }
  else
  {
    this->Connectivity->SetPropagateGhosts(false);
  }

  if (this->IntegrateFragments)
  {
    this->Integration->Update();
    mbdsOutput0->ShallowCopy(this->Integration->GetOutput());
  }

  return 1;
}
