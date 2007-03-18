/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiViewRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiViewRenderModuleProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientServerRenderModuleProxy.h"
#include "vtkSMIceTDesktopRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include <vtkstd/vector>

class vtkSMMultiViewRenderModuleProxyVector : 
  public vtkstd::vector<vtkSmartPointer<vtkSMProxy> > {};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMultiViewRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMMultiViewRenderModuleProxy, "1.7");

//----------------------------------------------------------------------------
vtkSMMultiViewRenderModuleProxy::vtkSMMultiViewRenderModuleProxy()
{
  this->RenderModuleName = 0;
  this->RenderModuleId = 0;
  this->RenderModules = new vtkSMMultiViewRenderModuleProxyVector;
}

//----------------------------------------------------------------------------
vtkSMMultiViewRenderModuleProxy::~vtkSMMultiViewRenderModuleProxy()
{
  delete this->RenderModules;
  this->SetRenderModuleName(0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMMultiViewRenderModuleProxy::NewRenderModule()
{
  this->CreateVTKObjects(1);

  if (!this->RenderModuleName)
    {
    vtkErrorMacro("Cannot create render module with a RenderModuleName");
    return 0;
    }

  vtkSMProxy* renderModule = this->GetProxyManager()->NewProxy(
    "rendermodules", this->RenderModuleName);
  renderModule->SetConnectionID(this->ConnectionID);

  /*
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->vtkSMProxy::GetProperty("RenderModules"));
  pp->AddProxy(renderModule);
  this->UpdateProperty("RenderModules");
  */

  return renderModule;
}

//-----------------------------------------------------------------------------
unsigned int vtkSMMultiViewRenderModuleProxy::GetNumberOfRenderModules()
{
  return this->RenderModules->size();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMMultiViewRenderModuleProxy::GetRenderModule(unsigned int index)
{
  if (index >= this->RenderModules->size())
    {
    vtkErrorMacro("Invalid index " << index);
    return 0;
    }
  return (*this->RenderModules)[index].GetPointer();
}

//-----------------------------------------------------------------------------
void vtkSMMultiViewRenderModuleProxy::AddRenderModule(
  vtkSMProxy* renderModule)
{
  vtkSMClientServerRenderModuleProxy *clientServerModule =
    vtkSMClientServerRenderModuleProxy::SafeDownCast(renderModule);
  if (clientServerModule)
    {
    clientServerModule->SetServerRenderWindowProxy(
      this->GetSubProxy("RenderWindow"));
    clientServerModule->SetServerRenderSyncManagerProxy(
      this->GetSubProxy("RenderSyncManager"));
    clientServerModule->SetRenderModuleId(this->RenderModuleId);
    }

  vtkSMIceTDesktopRenderModuleProxy* iceT = 
    vtkSMIceTDesktopRenderModuleProxy::SafeDownCast(renderModule);
  if (iceT)
    {
    iceT->SetServerDisplayManagerProxy(this->GetSubProxy("DisplayManager"));
    }

  this->RenderModuleId++;

  ostrstream name;
  name << "RenderModule." << renderModule->GetSelfIDAsString() << endl;
  this->AddSubProxy(name.str(), renderModule);

  this->RenderModules->push_back(renderModule);
  delete[] name.str();
}

//-----------------------------------------------------------------------------
void vtkSMMultiViewRenderModuleProxy::RemoveRenderModule(
  vtkSMProxy* renderModule)
{
  const char* name = this->GetSubProxyName(renderModule);
  if (name)
    {
    this->RemoveSubProxy(name);
    this->RenderModuleId--; // this will only work when the
                            // render modules are removed in reverse
                            // order that they were added.
    }
  vtkSMMultiViewRenderModuleProxyVector::iterator iter = 
    this->RenderModules->begin();
  for (; iter != this->RenderModules->end(); ++iter)
    {
    if ( (*iter).GetPointer() == renderModule)
      {
      this->RenderModules->erase(iter);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMMultiViewRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  if (!this->RenderModuleName)
    {
    vtkErrorMacro("A render module name has to be set before "
                  "vtkSMMultiViewRenderModuleProxyProxy can create "
                  "vtk objects");
    return;
    }
  if (   (strcmp(this->RenderModuleName, "IceTDesktopRenderModule") == 0)
      || (strcmp(this->RenderModuleName, "IceTRenderModule") == 0)
      || (strcmp(this->RenderModuleName, "ClientServerRenderModule") == 0) )
    {
    vtkSMProxy* renWin = this->GetProxyManager()->NewProxy(
      "renderwindow", "RenderWindow");
    renWin->SetConnectionID(this->ConnectionID);
    renWin->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("RenderWindow", renWin);
    renWin->Delete();

    vtkSMProxy* renSyncMan = this->GetProxyManager()->NewProxy(
      "composite_managers", "DesktopDeliveryServer");
    renSyncMan->SetConnectionID(this->ConnectionID);
    renSyncMan->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("RenderSyncManager", renSyncMan);
    renSyncMan->Delete();
    }

  if (   (strcmp(this->RenderModuleName, "IceTDesktopRenderModule") == 0)
      || (strcmp(this->RenderModuleName, "IceTRenderModule") == 0) )
    {
    vtkSMProxy* displayMan = this->GetProxyManager()->NewProxy(
      "composite_managers", "IceTRenderManager");
    displayMan->SetConnectionID(this->ConnectionID);
    displayMan->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("DisplayManager", displayMan);
    displayMan->Delete();
    }

  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy* vtkSMMultiViewRenderModuleProxy::CreateDisplayProxy()
{
  if (!this->RenderModuleName)
    {
    vtkErrorMacro("A render module name has to be set before "
                  "vtkSMMultiViewRenderModuleProxyProxy can create "
                  "display proxies.");
    }
  unsigned int numMax = this->GetNumberOfRenderModules();
  for (unsigned int cc=0;  cc <numMax; cc++)
    {
    vtkSMRenderModuleProxy* renModule = vtkSMRenderModuleProxy::SafeDownCast(
      this->GetRenderModule(cc));
    if (renModule)
      {
      return renModule->CreateDisplayProxy();
      }
    }

  vtkSMProxy* renderModule = this->GetProxyManager()->NewProxy(
    "rendermodules", this->RenderModuleName); 
  
  vtkSMAbstractDisplayProxy* display = 0;
  if (renderModule && vtkSMRenderModuleProxy::SafeDownCast(renderModule))
    {
    display = vtkSMRenderModuleProxy::SafeDownCast(renderModule)->CreateDisplayProxy();
    renderModule->Delete();
    }
  return display;
}

//----------------------------------------------------------------------------
void vtkSMMultiViewRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os <<  "RenderModuleName: " 
     << (this->RenderModuleName?this->RenderModuleName:"(none)")
     << endl;
}




