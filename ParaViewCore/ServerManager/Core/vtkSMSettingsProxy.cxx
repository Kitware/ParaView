/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSettingsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSettingsProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSettings.h"
#include "vtkSmartPointer.h"

class vtkSMSettingsObserver : public vtkCommand
{
public:
  static vtkSMSettingsObserver* New() { return new vtkSMSettingsObserver; }

  virtual void Execute(vtkObject*, unsigned long eventId, void*)
  {
    if (this->Proxy)
    {
      this->Proxy->ExecuteEvent(eventId);
    }
  }

  vtkSMSettingsObserver()
    : Proxy(NULL)
  {
  }

  vtkSMSettingsProxy* Proxy;
};

vtkStandardNewMacro(vtkSMSettingsProxy);

//----------------------------------------------------------------------------
vtkSMSettingsProxy::vtkSMSettingsProxy()
{
  this->SetLocation(vtkPVSession::CLIENT);
  this->Observer = vtkSMSettingsObserver::New();
  this->Observer->Proxy = this;
}

//----------------------------------------------------------------------------
vtkSMSettingsProxy::~vtkSMSettingsProxy()
{
  if (this->ObjectsCreated)
  {
    vtkObject* object = vtkObject::SafeDownCast(this->GetClientSideObject());
    if (object)
    {
      object->RemoveObserver(this->Observer);
    }
  }

  this->Observer->Proxy = NULL;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
int vtkSMSettingsProxy::ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
  {
    return 0;
  }

  // Now link information properties that provide the current value of the VTK
  // object with the corresponding proxy property
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(this->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMProperty* property = iter->GetProperty();
    if (property)
    {
      vtkSMProperty* infoProperty = property->GetInformationProperty();
      if (infoProperty)
      {
        this->LinkProperty(infoProperty, property);
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }

  this->Superclass::CreateVTKObjects();

  vtkObject* object = vtkObject::SafeDownCast(this->GetClientSideObject());
  if (object)
  {
    object->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
  }
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::ExecuteEvent(unsigned long eventId)
{
  this->UpdatePropertyInformation();

  this->InvokeEvent(eventId);

  vtkSMSettings* settings = vtkSMSettings::GetInstance();
  settings->SetProxySettings(this);
}

//----------------------------------------------------------------------------
void vtkSMSettingsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
