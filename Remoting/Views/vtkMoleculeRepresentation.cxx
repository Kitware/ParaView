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

#include "vtkActor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMolecule.h"
#include "vtkMoleculeMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkView.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkMoleculeRepresentation)

  //------------------------------------------------------------------------------
  vtkMoleculeRepresentation::vtkMoleculeRepresentation()
  : MoleculeRenderMode(0)
  , UseCustomRadii(false)
{
  // setup mapper
  this->Mapper = vtkMoleculeMapper::New();
  this->Mapper->UseBallAndStickSettings();

  // setup actor
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);

  static const char* defaultRadiiArrayName = "radii";
  this->SetAtomicRadiusArray(defaultRadiiArrayName);

  vtkMath::UninitializeBounds(this->DataBounds);
}

//------------------------------------------------------------------------------
vtkMoleculeRepresentation::~vtkMoleculeRepresentation()
{
  this->Actor->Delete();
  this->Mapper->Delete();
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetMoleculeRenderMode(int mode)
{
  if (mode != this->MoleculeRenderMode)
  {
    this->MoleculeRenderMode = mode;
    this->SyncMapper();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetUseCustomRadii(bool val)
{
  if (val != this->UseCustomRadii)
  {
    this->UseCustomRadii = val;
    this->SyncMapper();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  this->Mapper->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkMoleculeRepresentation::GetRenderedDataObject(int)
{
  // Bounds are only valid when we have valid input. Test for them before
  // returning cached object.
  if (vtkMath::AreBoundsInitialized(this->DataBounds))
  {
    return this->Molecule;
  }
  return NULL;
}

//------------------------------------------------------------------------------
int vtkMoleculeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds, this->Actor->GetMatrix());
    vtkPVRenderView::SetPiece(inInfo, this, this->Molecule);
    vtkPVRenderView::SetDeliverToClientAndRenderingProcesses(inInfo, this, true, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    this->Mapper->SetInputConnection(producerPort);
    this->UpdateColoringParameters();
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetVisibility(bool value)
{
  this->Superclass::SetVisibility(value);
  this->Actor->SetVisibility(value);
}

//------------------------------------------------------------------------------
int vtkMoleculeRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMolecule");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkMoleculeRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkMolecule* input = vtkMolecule::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    this->Molecule->ShallowCopy(input);
    this->Molecule->GetBounds(this->DataBounds);
  }
  else
  {
    this->Molecule->Initialize();
  }
  this->Molecule->Modified();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
bool vtkMoleculeRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(view);
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkMoleculeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SyncMapper()
{
  switch (this->MoleculeRenderMode)
  {
    case 0:
      this->Mapper->UseBallAndStickSettings();
      break;

    case 1:
      this->Mapper->UseVDWSpheresSettings();
      break;

    case 2:
      this->Mapper->UseLiquoriceStickSettings();
      break;

    default:
      vtkWarningMacro("Unknown MoleculeRenderMode: " << this->MoleculeRenderMode);
      break;
  }

  // Changing the render mode can clobber the custom array setting. Set it
  // back to what the user requested:
  if (this->UseCustomRadii)
  {
    this->Mapper->SetAtomicRadiusType(vtkMoleculeMapper::CustomArrayRadius);
  }
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetAtomicRadiusType(int type)
{
  this->Mapper->SetAtomicRadiusType(type);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetAtomicRadiusScaleFactor(double factor)
{
  this->Mapper->SetAtomicRadiusScaleFactor(factor);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetBondRadius(double radius)
{
  this->Mapper->SetBondRadius(radius);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetUseMultiCylindersForBonds(bool use)
{
  this->Mapper->SetUseMultiCylindersForBonds(use);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetRenderAtoms(bool render)
{
  this->Mapper->SetRenderAtoms(render);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetRenderBonds(bool render)
{
  this->Mapper->SetRenderBonds(render);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetAtomicRadiusArray(const char* name)
{
  this->Mapper->SetAtomicRadiusArrayName(name);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetBondColorMode(int mode)
{
  this->Mapper->SetBondColorMode(mode);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetBondColor(double r, double g, double b)
{
  this->Mapper->SetBondColor(r * 255, g * 255, b * 255);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::SetBondColor(double color[3])
{
  this->SetBondColor(color[0], color[1], color[2]);
}

//------------------------------------------------------------------------------
void vtkMoleculeRepresentation::UpdateColoringParameters()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  // this ensures that mapper's Mtime is updated when the array selection
  // changes.
  this->Mapper->SetInputArrayToProcess(
    0, 0, 0, info->Get(vtkDataObject::FIELD_ASSOCIATION()), info->Get(vtkDataObject::FIELD_NAME()));
}

//----------------------------------------------------------------------------
#define vtkForwardPropertyCallMacro(propertyMethod, arg, arg_type)                                 \
  void vtkMoleculeRepresentation::propertyMethod(arg_type arg)                                     \
  {                                                                                                \
    this->Actor->GetProperty()->propertyMethod(arg);                                               \
  }

vtkForwardPropertyCallMacro(SetOpacity, value, double);

void vtkMoleculeRepresentation::SetMapScalars(bool map)
{
  this->Mapper->SetMapScalars(map);
}
