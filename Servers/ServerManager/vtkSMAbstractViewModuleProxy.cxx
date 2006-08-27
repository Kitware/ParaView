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
#include "vtkProcessModule.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderModuleHelper.h"
#include "vtkSMAbstractDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkTimerLog.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWindowToImageFilter.h"

#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkProcessModuleConnectionManager.h"

vtkCxxRevisionMacro(vtkSMAbstractViewModuleProxy, "1.3");

//-----------------------------------------------------------------------------
vtkSMAbstractViewModuleProxy::vtkSMAbstractViewModuleProxy()
{
  // All the subproxies are created on Client and Render Server.
  this->Displays = vtkCollection::New();
  this->DisplayXMLName = 0;
}

//-----------------------------------------------------------------------------
vtkSMAbstractViewModuleProxy::~vtkSMAbstractViewModuleProxy()
{
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
  vtkTimerLog::MarkStartEvent("Interactive Render");
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::EndInteractiveRender()
{
  vtkTimerLog::MarkEndEvent("Interactive Render");
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
  vtkTimerLog::MarkStartEvent("Still Render");
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::EndStillRender()
{
  vtkTimerLog::MarkEndEvent("Still Render");
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::AddDisplay(
  vtkSMAbstractDisplayProxy* disp)
{
  if (!disp)
    {
    return;
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
    disp->UpdateDistributedGeometry();
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
void vtkSMAbstractViewModuleProxy::SaveInBatchScript(ofstream* file)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Render module not created yet!");
    return;
    }

  *file << "set pvTemp" << this->GetSelfIDAsString() 
        << " [$proxyManager NewProxy "
        << this->GetXMLGroup() << " " << this->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy " << this->GetXMLGroup()
        << " pvTemp" << this->GetSelfIDAsString() << " $pvTemp" 
        << this->GetSelfIDAsString() << endl;
  *file << "  $pvTemp" << this->GetSelfIDAsString() << " UnRegister {}" << endl;

  // Now, we save all the properties that are not Input.
  // Also note that only exposed properties are getting saved.
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* p = iter->GetProperty();
    if (vtkSMInputProperty::SafeDownCast(p))
      {
      continue;
      }

    if (p->GetIsInternal() || p->GetInformationOnly())
      {
      *file << "  # skipping proxy property " << iter->GetKey() << endl;
      continue;
      }

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
    vtkSMDoubleVectorProperty* dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(p);
    vtkSMStringVectorProperty* svp = 
      vtkSMStringVectorProperty::SafeDownCast(p);
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(p);
    if (ivp)
      {
      for (unsigned int i=0; i < ivp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty {"
          << iter->GetKey() << "}] SetElement "
          << i << " " << ivp->GetElement(i) 
          << endl;
        }
      }
    else if (dvp)
      {
      for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty {"
          << iter->GetKey() << "}] SetElement "
          << i << " " << dvp->GetElement(i) 
          << endl;
        }
      }
    else if (svp)
      {
      for (unsigned int i=0; i < svp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty {"
          << iter->GetKey() << "}] SetElement "
          << i << " {" << svp->GetElement(i) << "}"
          << endl;
        }
      }
    else if (pp)
      {
      // the only proxy property the RenderModule exposes is
      // Displays.
      for (unsigned int i=0; i < pp->GetNumberOfProxies(); i++)
        {
        vtkSMProxy* proxy = pp->GetProxy(i);
        // Some displays get saved in batch other don't,
        // instead of mirroring that logic to determine if
        // the display got saved in batch, we just catch the
        // exception.
        *file << "  catch { [$pvTemp" << this->GetSelfIDAsString() 
              << " GetProperty {"
              << iter->GetKey() << "}] AddProxy $pvTemp"
              << proxy->GetSelfIDAsString()
              << " } ;#--- " << proxy->GetXMLName() << endl;
        }
      }
    else
      {
      *file << "  # skipping property " << iter->GetKey() << endl;
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMAbstractViewModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Displays: " << this->Displays << endl;
}

