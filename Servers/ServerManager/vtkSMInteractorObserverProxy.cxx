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
#include "vtkPVRenderModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMDisplayWindowProxy.h"
#include "vtkKWEvent.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMInteractorObserverProxy, "1.7");

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
  this->RendererInitialized = 0;
  this->InteractorInitialized = 0;
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
void vtkSMInteractorObserverProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  
  if ( !this->RendererInitialized || !this->InteractorInitialized)
    {
    return;
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
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
  if ( event == vtkCommand::StartInteractionEvent )
    {
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    }
  else if ( event == vtkCommand::EndInteractionEvent )
    {
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
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
void vtkSMInteractorObserverProxy::SetCurrentRenderer(vtkClientServerID rendererID)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetCurrentRenderer" 
           << rendererID
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    }
  this->RendererInitialized = (rendererID.ID) ? 1 : 0;    
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::SetInteractor(vtkClientServerID interactorID)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetInteractor" 
           << interactorID
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    } 
  this->InteractorInitialized = (interactorID.ID)? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::AddToDisplayWindow(vtkSMDisplayWindowProxy* dw)
{
  vtkSMProxy* rendererProxy = dw->GetRendererProxy();
  if (!rendererProxy && rendererProxy->GetNumberOfIDs() > 0)
    {
    vtkErrorMacro("No renderer available.");
    return;
    }
  //TODO: verify this
  vtkClientServerID id = rendererProxy->GetID(0);
  this->SetCurrentRenderer(id);

  vtkSMProxy* interactorProxy = dw->GetInteractorProxy();
  if (!interactorProxy && interactorProxy->GetNumberOfIDs() > 0)
    {
    vtkErrorMacro("No interactor available. Needed for 3DWidgets");
    return;
    }
  id = interactorProxy->GetID(0);
  this->SetInteractor(id);
  this->SetEnabled(this->Enabled);
  this->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "RendererInitialized: " << this->RendererInitialized << endl;
  os << indent << "InteractorInitialized: " << this->InteractorInitialized << endl;
}

