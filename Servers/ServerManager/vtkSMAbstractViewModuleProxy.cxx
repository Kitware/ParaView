/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAbstractViewModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMAbstractViewModuleProxy.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImageWriter.h"
#include "vtkInstantiator.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"
#include "vtkWindowToImageFilter.h"

vtkCxxRevisionMacro(vtkSMAbstractViewModuleProxy, "1.10");

//-----------------------------------------------------------------------------
vtkSMAbstractViewModuleProxy::vtkSMAbstractViewModuleProxy()
{
  // All the subproxies are created on Client and Render Server.
  this->Displays = vtkCollection::New();
  this->DisplayXMLName = 0;

  this->GUISize[0] = this->GUISize[1] = 300;
  this->WindowPosition[0] = this->WindowPosition[1] = 0;

  this->ViewTimeLinks = vtkSMPropertyLink::New();
}

//-----------------------------------------------------------------------------
vtkSMAbstractViewModuleProxy::~vtkSMAbstractViewModuleProxy()
{
  this->ViewTimeLinks->Delete();
  this->ViewTimeLinks = 0;

  this->Displays->Delete();
  this->SetDisplayXMLName(0);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  // I don't directly use this->SetServers() to set the servers of the
  // subproxies, as the subclasses may have special subproxies that have
  // specific servers on which they want those to be created.
  this->SetServersSelf(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects(numObjects);

  this->ViewTimeLinks->AddLinkedProperty(
    this->GetProperty("ViewTime"), vtkSMLink::INPUT);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::InteractiveRender()
{
  this->UpdateAllDisplays();

  this->BeginInteractiveRender();
  this->PerformRender();
  this->EndInteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::BeginInteractiveRender()
{
  int interactive=1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  vtkTimerLog::MarkStartEvent("Interactive Render");
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::EndInteractiveRender()
{
  vtkTimerLog::MarkEndEvent("Interactive Render");
  
  int interactive=1;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::StillRender()
{
  this->UpdateAllDisplays();

  this->BeginStillRender();
  this->PerformRender();
  this->EndStillRender();
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::BeginStillRender()
{
  int interactive=0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);
  vtkTimerLog::MarkStartEvent("Still Render");
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::EndStillRender()
{
  vtkTimerLog::MarkEndEvent("Still Render");
  int interactive=0;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::AddDisplay(
  vtkSMAbstractDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }

  // Link UpdateTime on DisplayProxy with ViewTime.
  vtkSMProperty* prop = disp->GetProperty("UpdateTime");
  if (prop)
    {
    this->ViewTimeLinks->AddLinkedProperty(prop, vtkSMLink::OUTPUT);
    disp->UpdateProperty("UpdateTime");
    }
  
  this->Displays->AddItem(disp);

  disp->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::RemoveDisplay(
  vtkSMAbstractDisplayProxy* disp)
{
  if (!disp)
    {
    return;
    }
  vtkSMProperty* prop = disp->GetProperty("UpdateTime");
  if (prop)
    {
    this->ViewTimeLinks->RemoveLinkedProperty(prop);
    }
  this->Displays->RemoveItem(disp);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::RemoveAllDisplays()
{
  while ( this->Displays->GetNumberOfItems() )
    {
    vtkSMAbstractDisplayProxy* disp = 
      vtkSMAbstractDisplayProxy::SafeDownCast(
        this->Displays->GetItemAsObject(0));
    this->RemoveDisplay(disp);
    }
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMAbstractViewModuleProxy::GetRenderingProgressServers()
{
  return vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT;
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::UpdateAllDisplays()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkCollectionIterator* iter = this->Displays->NewIterator();

  // Check if this update is going to result in updating of any pipeline,
  // if so we must enable progresses, otherwise progresses are not necessary.
  bool enable_progress = false;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMAbstractDisplayProxy* disp = 
      vtkSMAbstractDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !disp->GetVisibilityCM())
      {
      // Some displays don't need updating.
      continue;
      }
    if (disp->UpdateRequired())
      {
      enable_progress = true;
      break;
      }
    }


  if (enable_progress)
    {
    pm->SendPrepareProgress(this->ConnectionID,
      vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
    }
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMAbstractDisplayProxy* disp = 
      vtkSMAbstractDisplayProxy::SafeDownCast(iter->GetCurrentObject());
    if (!disp || !disp->GetVisibilityCM())
      {
      // Some displays don't need updating.
      continue;
      }
    // In case of ordered compositing, make sure any distributed geometry is up
    // to date.
    disp->UpdateDistributedGeometry(this);
    // We don;t use properties here since it tends to slow things down.
    }
  iter->Delete();
  if (enable_progress)
    {
    pm->SendCleanupPendingProgress(this->ConnectionID);
    }
}

//-----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy* vtkSMAbstractViewModuleProxy::CreateDisplayProxy()
{
  if (!this->DisplayXMLName)
    {
    vtkErrorMacro("DisplayXMLName must be set to create Display proxies.");
    return NULL;
    }
  
  vtkSMProxy* p = vtkSMObject::GetProxyManager()->NewProxy(
    "displays", this->DisplayXMLName);
  if (!p)
    {
    return NULL;
    }
  p->SetConnectionID(this->ConnectionID);
  vtkSMAbstractDisplayProxy *pDisp = vtkSMAbstractDisplayProxy::SafeDownCast(p);
  if (!pDisp)
    {
    vtkErrorMacro(<< "'displays' ," <<  this->DisplayXMLName 
                  << " must be a subclass of vtkSMAbstractDisplayProxy.");
    p->Delete();
    return NULL;
    }
  return pDisp;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMAbstractViewModuleProxy::SaveState(vtkPVXMLElement* root)
{
  return this->Superclass::SaveState(root);
}

//-----------------------------------------------------------------------------
int vtkSMAbstractViewModuleProxy::ReadXMLAttributes(vtkSMProxyManager* pm,
  vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  const char* display_name = element->GetAttribute("display_name");
  if (display_name)
    {
    this->SetDisplayXMLName(display_name);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GUISize: " 
    << this->GUISize[0] << ", " << this->GUISize[1] << endl;
  os << indent << "WindowPosition: " 
    << this->WindowPosition[0] << ", " << this->WindowPosition[1] << endl;
  os << indent << "Displays: " << this->Displays << endl;
}

