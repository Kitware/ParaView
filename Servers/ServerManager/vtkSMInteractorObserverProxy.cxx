/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInteractorObserverProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInteractorObserverProxy.h"

#include "vtkInteractorObserver.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVGenericRenderWindowInteractor.h"
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMInteractorObserverProxy, "1.8.2.2");

//===========================================================================
//***************************************************************************
class vtkSMInteractorObserverProxyObserver : public vtkCommand
{
public:
  static vtkSMInteractorObserverProxyObserver *New() 
    {return new vtkSMInteractorObserverProxyObserver;};

  vtkSMInteractorObserverProxyObserver()
    {
      this->SMInteractorObserverProxy = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
      if ( this->SMInteractorObserverProxy )
        {
        this->SMInteractorObserverProxy->ExecuteEvent(wdg, event, calldata);
        }
    }
  vtkSMInteractorObserverProxy* SMInteractorObserverProxy;
};
//***************************************************************************

//----------------------------------------------------------------------------
vtkSMInteractorObserverProxy::vtkSMInteractorObserverProxy()
{
  this->Observer = vtkSMInteractorObserverProxyObserver::New();
  this->Observer->SMInteractorObserverProxy = this;
  this->Enabled = 0;
  this->CurrentRenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
vtkSMInteractorObserverProxy::~vtkSMInteractorObserverProxy()
{
  this->Observer->Delete();
}


//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::InitializeObservers(vtkInteractorObserver* wdg)
{
  if(wdg)
    {
    wdg->AddObserver(vtkCommand::InteractionEvent, this->Observer);
    wdg->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    wdg->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
    }
}


//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::SetEnabled(int e)
{ 
  this->Enabled = e;

  if (!this->CurrentRenderModuleProxy)
    {
    return; // widgets are not enabled till rendermodule is set.
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  for(cc=0;cc < numObjects; cc++)
    {
    str << vtkClientServerStream::Invoke << this->GetID(cc)
      << "SetEnabled" << this->Enabled << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    } 
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::ExecuteEvent(vtkObject*, unsigned long event, void*)
{
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent);
  vtkPVGenericRenderWindowInteractor* iren;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  if ( event == vtkCommand::StartInteractionEvent )
    {
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      pm->GetObjectFromID( 
      this->CurrentRenderModuleProxy->GetInteractorProxy()->GetID(0)));
    iren->InteractiveRenderEnabledOn();
    }
  else if ( event == vtkCommand::EndInteractionEvent )
    {
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      pm->GetObjectFromID( 
      this->CurrentRenderModuleProxy->GetInteractorProxy()->GetID(0)));
    iren->InteractiveRenderEnabledOff();
    }
  else if ( event == vtkCommand::PlaceWidgetEvent )
    {
    this->InvokeEvent(vtkCommand::PlaceWidgetEvent);
    }
  else
    {
    // So the the client object changes are sent over to the Servers
    this->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  //Chage the SMProxy default proxy creation location
  this->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
  //Superclass creates the actual VTK objects
  this->Superclass::CreateVTKObjects(numObjects);

   //additional initialization 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtkInteractorObserver* widget = vtkInteractorObserver::SafeDownCast(
      pm->GetObjectFromID(this->GetID(cc)));
    this->InitializeObservers(widget);
    } 
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::SetCurrentRenderer(vtkSMProxy *renderer)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID null = {0 };
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetCurrentRenderer" 
           << ( (renderer)? renderer->GetID(0) : null )
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    }
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::SetInteractor(vtkSMProxy* interactor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID null = {0 };
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetInteractor" 
           << ((interactor)? interactor->GetID(0) : null)
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    } 
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::SetCurrentRenderModuleProxy(
  vtkSMRenderModuleProxy* rm)
{
  if (this->CurrentRenderModuleProxy && rm != this->CurrentRenderModuleProxy
    && rm)
    {
    vtkErrorMacro("CurrentRenderModuleProxy already set.");
    return;
    }
  this->CurrentRenderModuleProxy = rm;
  this->SetEnabled(this->Enabled);
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CurrentRenderModuleProxy: " 
    << this->CurrentRenderModuleProxy << endl;
}

