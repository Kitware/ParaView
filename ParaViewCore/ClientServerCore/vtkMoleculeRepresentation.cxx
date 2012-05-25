/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMoleculeRepresentation.h"

#include "vtkView.h"
#include "vtkActor.h"
#include "vtkMolecule.h"
#include "vtkRenderer.h"
#include "vtkPVRenderView.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkMoleculeMapper.h"
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkMoleculeRepresentation)

vtkMoleculeRepresentation::vtkMoleculeRepresentation()
{
  // setup mapper
  this->Mapper = vtkMoleculeMapper::New();
  this->Mapper->UseBallAndStickSettings();

  // setup actor
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
}

vtkMoleculeRepresentation::~vtkMoleculeRepresentation()
{
  this->Actor->Delete();
  this->Mapper->Delete();
}

void vtkMoleculeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkMoleculeRepresentation::SetMoleculeRenderMode(int mode)
{
  if(mode == 0)
    {
    this->Mapper->UseBallAndStickSettings();
    }
  else if(mode == 1)
    {
    this->Mapper->UseVDWSpheresSettings();
    }
  else if(mode == 2)
    {
    this->Mapper->UseLiquoriceStickSettings();
    }
}

int vtkMoleculeRepresentation::ProcessViewRequest(vtkInformationRequestKey *request_type,
                                                  vtkInformation *input_info,
                                                  vtkInformation *output_info)
{
  return this->Superclass::ProcessViewRequest(request_type, input_info, output_info);
}

void vtkMoleculeRepresentation::SetVisibility(bool value)
{
  this->Superclass::SetVisibility(value);
  this->Actor->SetVisibility(value);
}

int vtkMoleculeRepresentation::FillInputPortInformation(int port,
                                                        vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");
  return 1;
}

int vtkMoleculeRepresentation::RequestData(vtkInformation *request,
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
    {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkMolecule* input =
      vtkMolecule::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    this->Mapper->SetInputData(input);
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

bool vtkMoleculeRepresentation::AddToView(vtkView *view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
    }
  return false;
}

bool vtkMoleculeRepresentation::RemoveFromView(vtkView *view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return true;
    }
  return false;
}
