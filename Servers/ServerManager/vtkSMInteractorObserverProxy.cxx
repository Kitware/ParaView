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
#include "vtkPVXMLElement.h"
#include "vtkPVOptions.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMInteractorObserverProxy, "1.8.2.4");

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
  this->ClientSideVTKClassName = 0;
}

//----------------------------------------------------------------------------
vtkSMInteractorObserverProxy::~vtkSMInteractorObserverProxy()
{
  this->Observer->Delete();
  this->SetClientSideVTKClassName(0);
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
  vtkPVGenericRenderWindowInteractor* iren = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->CurrentRenderModuleProxy)
    {
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      pm->GetObjectFromID( 
        this->CurrentRenderModuleProxy->GetInteractorProxy()->GetID(0)));
    }
  if ( event == vtkCommand::StartInteractionEvent && iren)
    {
    iren->InteractiveRenderEnabledOn();
    }
  else if ( event == vtkCommand::EndInteractionEvent && iren)
    {
    this->UpdateVTKObjects();
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

  if (iren)
    {
    iren->Render();
    }
}

//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  
  unsigned int cc;
  vtkTypeUInt32 old_Servers = this->Servers;
  vtkTypeUInt32 new_Servers = this->Servers;
  int create_on_client = 0;
  
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
 
  // This crookedness is required because, vtkPickPointWidget/ vtkPickLineWiget
  // need render module, and hence should be created only on the client side.
  // They cannot be created on the ServerSide at all (since they are 
  // are ServerManager). So, we create vtkPointWidget / vtkLineWidget on the
  // serverside. However, if not running in client server mode,
  // we create vtkPickPointWidget / vtkPickLineWiget directly.

  if (this->ClientSideVTKClassName && 
    !pm->GetOptions()->GetClientMode() && !pm->GetOptions()->GetServerMode())
    {
    // paraview run as a single process or MPI,
    this->SetVTKClassName(this->ClientSideVTKClassName); 
    }
  else if (this->ClientSideVTKClassName)
    {
    // check if the proxy is being created on the Client at all.
    create_on_client = ( (this->Servers & vtkProcessModule::CLIENT) != 0)? 1 : 0;
    new_Servers = (create_on_client)? ( old_Servers & (~vtkProcessModule::CLIENT)) :
      old_Servers;
    this->SetServers(new_Servers);
    }

  //Superclass creates the actual VTK objects
  this->Superclass::CreateVTKObjects(numObjects);

  
  if (create_on_client)
    {
    vtkClientServerStream stream;
    for (cc=0; cc < this->GetNumberOfIDs(); cc++)
      {
      stream << vtkClientServerStream::New
        << this->ClientSideVTKClassName << this->GetID(cc)
        << vtkClientServerStream::End;
      }
    if (stream.GetNumberOfMessages() > 0)
      {
      pm->SendStream(vtkProcessModule::CLIENT, stream);
      }
    this->SetServers(old_Servers);
    }

   //additional initialization 
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
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
int vtkSMInteractorObserverProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }
  const char* client_class = element->GetAttribute("client_class");
  if (client_class)
    {
    this->SetClientSideVTKClassName(client_class);
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkSMInteractorObserverProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CurrentRenderModuleProxy: " 
    << this->CurrentRenderModuleProxy << endl;
}

