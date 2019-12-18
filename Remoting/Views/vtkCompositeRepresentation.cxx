/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeRepresentation.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVView.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>

#include <assert.h>

class vtkCompositeRepresentation::vtkInternals
{
public:
  typedef std::map<std::string, vtkSmartPointer<vtkPVDataRepresentation> > RepresentationMap;
  RepresentationMap Representations;

  std::string ActiveRepresentationKey;

  vtkSmartPointer<vtkStringArray> RepresentationTypes;
};

vtkStandardNewMacro(vtkCompositeRepresentation);
//----------------------------------------------------------------------------
vtkCompositeRepresentation::vtkCompositeRepresentation()
{
  this->Internals = new vtkInternals();
  this->Internals->RepresentationTypes = vtkSmartPointer<vtkStringArray>::New();
  this->Internals->RepresentationTypes->SetNumberOfComponents(1);
  this->Observer =
    vtkMakeMemberFunctionCommand(*this, &vtkCompositeRepresentation::TriggerUpdateDataEvent);
}

//----------------------------------------------------------------------------
vtkCompositeRepresentation::~vtkCompositeRepresentation()
{
  delete this->Internals;
  this->Internals = 0;
  this->Observer->Delete();
  this->Observer = 0;
}

//----------------------------------------------------------------------------
int vtkCompositeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (request == vtkPVView::REQUEST_UPDATE())
  {
    // skip update requests since this representation doesn't really do anything
    // by itself.
    return 0;
  }

  return this->Superclass::ProcessViewRequest(request, inInfo, outInfo);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetVisibility(bool visible)
{
  this->Superclass::SetVisibility(visible);
  vtkPVDataRepresentation* repr = this->GetActiveRepresentation();
  if (repr)
  {
    repr->SetVisibility(visible);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::AddRepresentation(const char* key, vtkPVDataRepresentation* repr)
{
  assert(repr != NULL && key != NULL);

  // Make sure the representation that we register is already initialized
  repr->Initialize(1, 0); // Should abort if no initialized as 1 > 0

  if (this->Internals->Representations.find(key) != this->Internals->Representations.end())
  {
    vtkWarningMacro("Replacing existing representation for key: " << key);
    this->Internals->Representations[key]->RemoveObserver(this->Observer);
  }

  this->Internals->Representations[key] = repr;
  repr->SetVisibility(false);
  repr->AddObserver(vtkCommand::UpdateDataEvent, this->Observer);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::RemoveRepresentation(const char* key)
{
  assert(key != NULL);

  vtkInternals::RepresentationMap::iterator iter = this->Internals->Representations.find(key);
  if (iter != this->Internals->Representations.end())
  {
    iter->second.GetPointer()->RemoveObserver(this->Observer);
    this->Internals->Representations.erase(iter);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::RemoveRepresentation(vtkPVDataRepresentation* repr)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); ++iter)
  {
    if (iter->second.GetPointer() == repr)
    {
      iter->second.GetPointer()->RemoveObserver(this->Observer);
      this->Internals->Representations.erase(iter);
      break;
    }
  }
}

//----------------------------------------------------------------------------
vtkStringArray* vtkCompositeRepresentation::GetRepresentationTypes()
{
  this->Internals->RepresentationTypes->SetNumberOfTuples(
    static_cast<vtkIdType>(this->Internals->Representations.size()));
  vtkIdType cc = 0;
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); ++iter, ++cc)
  {
    this->Internals->RepresentationTypes->SetValue(cc, iter->first.c_str());
  }

  return this->Internals->RepresentationTypes;
}

//----------------------------------------------------------------------------
const char* vtkCompositeRepresentation::GetActiveRepresentationKey()
{
  vtkInternals::RepresentationMap::iterator iter =
    this->Internals->Representations.find(this->Internals->ActiveRepresentationKey);
  if (iter != this->Internals->Representations.end())
  {
    return this->Internals->ActiveRepresentationKey.c_str();
  }

  return NULL;
}

//----------------------------------------------------------------------------
vtkPVDataRepresentation* vtkCompositeRepresentation::GetActiveRepresentation()
{
  vtkInternals::RepresentationMap::iterator iter =
    this->Internals->Representations.find(this->Internals->ActiveRepresentationKey);
  if (iter != this->Internals->Representations.end())
  {
    return iter->second;
  }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkCompositeRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->SetInputConnection(port, input);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetInputConnection(vtkAlgorithmOutput* input)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->SetInputConnection(input);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::AddInputConnection(int port, vtkAlgorithmOutput* input)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->AddInputConnection(port, input);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::AddInputConnection(vtkAlgorithmOutput* input)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->AddInputConnection(input);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->RemoveInputConnection(port, input);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::RemoveInputConnection(int port, int idx)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->RemoveInputConnection(port, idx);
  }
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::MarkModified()
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->MarkModified();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetUpdateTime(double time)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->SetUpdateTime(time);
  }
  this->Superclass::SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetForceUseCache(bool val)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->SetForceUseCache(val);
  }
  this->Superclass::SetForceUseCache(val);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetForcedCacheKey(double val)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    iter->second.GetPointer()->SetForcedCacheKey(val);
  }
  this->Superclass::SetForcedCacheKey(val);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCompositeRepresentation::GetRenderedDataObject(int port)
{
  vtkPVDataRepresentation* activeRepr = this->GetActiveRepresentation();
  if (activeRepr)
  {
    return activeRepr->GetRenderedDataObject(port);
  }

  return this->Superclass::GetRenderedDataObject(port);
}

//----------------------------------------------------------------------------
bool vtkCompositeRepresentation::AddToView(vtkView* view)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    view->AddRepresentation(iter->second.GetPointer());
  }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkCompositeRepresentation::RemoveFromView(vtkView* view)
{
  vtkInternals::RepresentationMap::iterator iter;
  for (iter = this->Internals->Representations.begin();
       iter != this->Internals->Representations.end(); iter++)
  {
    view->RemoveRepresentation(iter->second.GetPointer());
  }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::TriggerUpdateDataEvent()
{
  // We fire UpdateDataEvent to notify the representation proxy that the
  // representation was updated. The representation proxty will then call
  // PostUpdateData(). We do this since now representations are not updated at
  // the proxy level.
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::SetActiveRepresentation(const char* key)
{
  assert(key != NULL);

  vtkPVDataRepresentation* curActive = this->GetActiveRepresentation();
  this->Internals->ActiveRepresentationKey = key;
  vtkPVDataRepresentation* newActive = this->GetActiveRepresentation();
  if (curActive != newActive)
  {
    if (curActive)
    {
      curActive->SetVisibility(false);
    }

    if (newActive)
    {
      newActive->SetVisibility(this->GetVisibility());
    }
  }

  // Get some feedback if the Representation Key is invalid
  // this might occur with char* keys...
  if (!newActive && key && strlen(key))
  {
    vtkErrorMacro(<< "No representation was found with Name: " << key);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCompositeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
