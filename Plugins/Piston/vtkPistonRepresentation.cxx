/*=========================================================================

  Program:   ParaView
  Module:    vtkPistonRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPistonRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPistonDataObject.h"
#include "vtkPistonMapper.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

//*****************************************************************************

vtkStandardNewMacro(vtkPistonRepresentation);

//----------------------------------------------------------------------------
vtkPistonRepresentation::vtkPistonRepresentation()
{
  this->Mapper = vtkPistonMapper::New();
  this->Actor = vtkActor::New();

  this->DebugString = 0;
  this->SetDebugString(this->GetClassName());

  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkPistonRepresentation::~vtkPistonRepresentation()
{
  this->Mapper->Delete();
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkPistonRepresentation::SetupDefaults()
{
  vtkPistonDataObject* input = vtkPistonDataObject::SafeDownCast(this->GetInputDataObject(0, 0));
  if (input)
  {
    this->Mapper->SetInputConnection(this->GetInternalOutputPort());
  }

  this->Actor->SetMapper(this->Mapper);

  // Not insanely thrilled about this API on vtkProp about properties, but oh
  // well. We have to live with it.
  // vtkInformation* keys = vtkInformation::New();
  // this->Actor->SetPropertyKeys(keys);
  // keys->Delete();
}

//----------------------------------------------------------------------------
int vtkPistonRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPistonDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->GetVisibility())
  {
    return false;
  }

  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}

//----------------------------------------------------------------------------
int vtkPistonRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ensure that the ghost-level information is setup correctly to avoid
  // internal faces for unstructured grids.
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      vtkInformation* inInfo = inputVector[cc]->GetInformationObject(kk);

      vtkStreamingDemandDrivenPipeline* sddp =
        vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
      int ghostLevels = sddp->GetUpdateGhostLevel(inInfo);
      sddp->SetUpdateGhostLevel(inInfo, ghostLevels);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPistonRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // mark delivery filters modified.
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      vtkAlgorithmOutput* aout = this->GetInternalOutputPort();
      vtkPVTrivialProducer* prod = vtkPVTrivialProducer::SafeDownCast(aout->GetProducer());
      if (prod)
      {
        prod->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
      }
    }
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkPistonDataObject* input = vtkPistonDataObject::SafeDownCast(this->GetInputDataObject(0, 0));
  if (input)
  {
    this->Mapper->SetInputConnection(this->GetInternalOutputPort());
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkPistonRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPistonRepresentation::RemoveFromView(vtkView* view)
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
void vtkPistonRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkPistonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
