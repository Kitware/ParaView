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
#include "vtkSMIceTDesktopRenderModuleProxy.h"
#include "vtkSMProxyManager.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMMultiViewRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMMultiViewRenderModuleProxy, "1.2");

//----------------------------------------------------------------------------
vtkSMMultiViewRenderModuleProxy::vtkSMMultiViewRenderModuleProxy()
{
  this->RenderModuleName = 0;
  this->RenderModuleId = 0;
}

//----------------------------------------------------------------------------
vtkSMMultiViewRenderModuleProxy::~vtkSMMultiViewRenderModuleProxy()
{
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

  vtkSMIceTDesktopRenderModuleProxy* iceT = 
    vtkSMIceTDesktopRenderModuleProxy::SafeDownCast(renderModule);
  if (iceT)
    {
    iceT->SetServerRenderWindowProxy(this->GetSubProxy("RenderWindow"));
    iceT->SetServerCompositeManagerProxy(this->GetSubProxy("CompositeManager"));
    iceT->SetServerDisplayManagerProxy(this->GetSubProxy("DisplayManager"));
    iceT->SetRenderModuleId(this->RenderModuleId);
    }

  ostrstream name;
  name << "RenderModule" << this->RenderModuleId << ends;
  this->AddProxy(name.str(), renderModule);
  delete[] name.str();
  this->RenderModuleId++;

  return renderModule;
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
  if (strcmp(this->RenderModuleName, "IceTDesktopRenderModule") == 0||
      strcmp(this->RenderModuleName, "IceTRenderModule") == 0)
    {
    vtkSMProxy* renWin = this->GetProxyManager()->NewProxy(
      "renderwindow", "RenderWindow");
    renWin->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("RenderWindow", renWin);
    renWin->Delete();

    vtkSMProxy* comMan = this->GetProxyManager()->NewProxy(
      "composite_managers", "DesktopDeliveryServer");
    comMan->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("CompositeManager", comMan);
    comMan->Delete();

    vtkSMProxy* displayMan = this->GetProxyManager()->NewProxy(
      "composite_managers", "IceTRenderManager");
    displayMan->SetServers(vtkProcessModule::RENDER_SERVER);
    this->AddSubProxy("DisplayManager", displayMan);
    displayMan->Delete();
    }

  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMMultiViewRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os <<  "RenderModuleName: " 
     << (this->RenderModuleName?this->RenderModuleName:"(none)")
     << endl;
}




