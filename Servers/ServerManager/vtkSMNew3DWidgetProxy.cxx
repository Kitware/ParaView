/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNew3DWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMNew3DWidgetProxy.h"

#include "vtkAbstractWidget.h"
#include "vtkClientServerInterpreter.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWidgetRepresentation.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMNew3DWidgetProxy);
vtkCxxRevisionMacro(vtkSMNew3DWidgetProxy, "1.3");

class vtkSMNew3DWidgetObserver : public vtkCommand
{
public:
  static vtkSMNew3DWidgetObserver *New() 
    { return new vtkSMNew3DWidgetObserver; }
  virtual void Execute(vtkObject*, unsigned long event, void*)
    {
      if (this->Proxy)
        {
        this->Proxy->ExecuteEvent(event);
        }
    }
  vtkSMNew3DWidgetObserver():Proxy(0) {}
  vtkSMNew3DWidgetProxy* Proxy;
};

struct vtkSMNew3DWidgetProxyInternals
{
  typedef vtkstd::list<vtkSmartPointer<vtkSMLink> > LinksType;
  LinksType Links;
};

//-----------------------------------------------------------------------------
vtkSMNew3DWidgetProxy::vtkSMNew3DWidgetProxy()
{
  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer = vtkSMNew3DWidgetObserver::New();
  this->Observer->Proxy = this;
  this->Internal = new vtkSMNew3DWidgetProxyInternals;
}

//-----------------------------------------------------------------------------
vtkSMNew3DWidgetProxy::~vtkSMNew3DWidgetProxy()
{
  // Unless the interactor is set to 0, the widget leaks.
  // There must be a reference loop somewhere.
  if (this->WidgetProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAbstractWidget* widget = vtkAbstractWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID(0)));
    widget->SetInteractor(0);
    }

  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer->Delete();

  if (this->Internal)
    {
    delete this->Internal;
    }
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::AddToRenderModule(rm);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->WidgetProxy)
    {
    vtkAbstractWidget* widget = vtkAbstractWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID(0)));
    if (widget)
      {
      widget->SetInteractor(rm->GetInteractor());
      }
    }

  this->SetEnabled(1);
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::RemoveFromRenderModule(rm);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->WidgetProxy)
    {
    this->SetEnabled(0);

    vtkAbstractWidget* widget = vtkAbstractWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID(0)));
    if (this->Widget)
      {
      widget->SetInteractor(0);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::SetEnabled(int enable)
{
  if (this->WidgetProxy)
    {
    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Enabled"));
    enabled->SetElements1(enable);
    this->WidgetProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->RepresentationProxy = this->GetSubProxy("Prop");
  if (!this->RepresentationProxy)
    {
    vtkErrorMacro("A representation proxy must be defined as a Prop sub-proxy");
    return;
    }
  this->RepresentationProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);

  this->WidgetProxy = this->GetSubProxy("Widget");
  if (!this->WidgetProxy)
    {
    vtkErrorMacro("A widget proxy must be defined as a Widget sub-proxy");
    return;
    }
  this->WidgetProxy->SetServers(vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects(numObjects);

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Representation"));
  if (pp)
    {
    pp->AddProxy(this->RepresentationProxy);
    }
  this->WidgetProxy->UpdateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->Widget = vtkAbstractWidget::SafeDownCast(
    pm->GetObjectFromID(this->WidgetProxy->GetID(0)));
  if (this->Widget)
    {
    this->Widget->AddObserver(
      vtkCommand::StartInteractionEvent, this->Observer);
    this->Widget->AddObserver(
      vtkCommand::EndInteractionEvent, this->Observer);
    }

  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for(piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
    vtkSMProperty* prop = piter->GetProperty();
    vtkSMProperty* info = prop->GetInformationProperty();
    if (info)
      {
      vtkSMPropertyLink* link = vtkSMPropertyLink::New();
      link->AddLinkedProperty(this, 
                              piter->GetKey(), 
                              vtkSMLink::OUTPUT);
      link->AddLinkedProperty(this, 
                              this->GetPropertyName(info),
                              vtkSMLink::INPUT);
      this->Internal->Links.push_back(link);
      link->Delete();
      }
    }
  piter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::ExecuteEvent(unsigned long event)
{
  if (event == vtkCommand::StartInteractionEvent)
    {
    vtkPVGenericRenderWindowInteractor* inter =
      vtkPVGenericRenderWindowInteractor::SafeDownCast(
        this->Widget->GetInteractor());
    if (inter)
      {
      inter->InteractiveRenderEnabledOn();
      }
    }
  else if (event == vtkCommand::EndInteractionEvent)
    {
    vtkPVGenericRenderWindowInteractor* inter =
      vtkPVGenericRenderWindowInteractor::SafeDownCast(
        this->Widget->GetInteractor());
    if (inter)
      {
      inter->InteractiveRenderEnabledOff();
      }
    this->RepresentationProxy->UpdatePropertyInformation();
    vtkSMDoubleVectorProperty* value = vtkSMDoubleVectorProperty::SafeDownCast(
      this->RepresentationProxy->GetProperty("Value"));
    value = vtkSMDoubleVectorProperty::SafeDownCast(
      this->RepresentationProxy->GetProperty("ValueInfo"));
    }
}

//----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::UnRegister(vtkObjectBase* obj)
{
  if ( this->SelfID.ID != 0 )
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    // If the object is not being deleted by the interpreter and it
    // has a reference count of 2 (SelfID and the reference that is
    // being released), delete the internals so that the links
    // release their references to the proxy
    if ( pm && obj != pm->GetInterpreter() && this->Internal )
      {
      int size = this->Internal->Links.size();
      if (size > 0 && this->ReferenceCount == 2 + 2*size)
        {
        vtkSMNew3DWidgetProxyInternals* internal = this->Internal;
        this->Internal = 0;
        delete internal;
        internal = 0;
        }
      }
    }

  this->Superclass::UnRegister(obj);
}

//-----------------------------------------------------------------------------
void vtkSMNew3DWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
