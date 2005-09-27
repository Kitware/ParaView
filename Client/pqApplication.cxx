// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqApplication.h"
#include "pqServer.h"

#include <vtkClientServerStream.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkPVGenericRenderWindowInteractor.h>
#include <vtkPVOptions.h>
#include <vtkPVProcessModule.h>
#include <vtkPVRenderViewProxy.h>
#include <vtkPVServerInformation.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSMApplication.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

vtkCxxRevisionMacro(pqApplication, "1.1");
vtkStandardNewMacro(pqApplication);

// The ParaView version of vtkPVApplication also registers the render
// module proxy with the proxy manager.  I don't see the point, so I'm not
// doing it.  If it's needed, someone can add it later.
vtkCxxSetObjectMacro(pqApplication, RenderModuleProxy, vtkSMRenderModuleProxy);

//-----------------------------------------------------------------------------

pqApplication::pqApplication()
{
  this->ProcessModule = NULL;
  this->RenderModuleProxy = NULL;
}

pqApplication::~pqApplication()
{
  this->SetRenderModuleProxy(NULL);
  if (this->ProcessModule)
    {
    this->ProcessModule->UnRegister(this);
    this->ProcessModule = NULL;
    }
}

void pqApplication::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ProcessModule: " << this->ProcessModule << endl;
}

//-----------------------------------------------------------------------------

void pqApplication::Initialize(vtkProcessModule *pm)
{
  if (this->ProcessModule != NULL)
    {
    vtkErrorMacro(<< "Initialize already called.");
    return;
    }

  this->ProcessModule = pm;
  this->ProcessModule->Register(this);

  // By default, the server manager checks every value assigned to
  // a property against the it's domains. If the domain check fails,
  // error is returned and the value is rejected. This sometimes
  // creates problems (specially when the domains are not fully
  // specified). Therefore, it is usually easier to turn domain
  // checking off.
  vtkSMProperty::SetCheckDomains(0);
}

//-----------------------------------------------------------------------------

void pqApplication::Finalize()
{
//  this->SMApplication->Finalize();
  this->SetRenderModuleProxy(NULL);

  // After this is called, there should be no communication with servers.
  this->ProcessModule->Exit();
}

//-----------------------------------------------------------------------------

int pqApplication::SetupRenderModule(const char *renderModuleName)
{
  vtkSMProxyManager* const pxm = GetServer()->GetProxyManager();
  vtkPVProcessModule *pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  pm->SynchronizeServerClientOptions();

  if (!renderModuleName)
    {
    renderModuleName = pm->GetOptions()->GetRenderModuleName();
    }
  if (!renderModuleName)
    {
    // User didn't specify the render module to use.
    if (pm->GetOptions()->GetTileDimensions()[0])
      {
      // Server/client says we are rendering for a Tile Display.
      // Now decide if we must use IceT or not.
      if (pm->GetServerInformation()->GetUseIceT())
        {
        renderModuleName = "IceTRenderModule";
        }
      else
        {
        renderModuleName = "MultiDisplayRenderModule";
        }
      }
    else if (pm->GetOptions()->GetClientMode())
      {
      // Client server configuration without Tiles.
      if (pm->GetServerInformation()->GetUseIceT())
        {
        renderModuleName = "IceTDesktopRenderModule";
        }
      else
        {
        renderModuleName = "MPIRenderModule"; 
        // TODO: if I separated the MPI and ClientServer
        // render modules, this is where I will use
        // the ClientServerRenderModule.
        }
      }
    else
      {
      // Not running in Client Server Mode.
      // Use local information to choose render module.
#ifdef VTK_USE_MPI
      renderModuleName = "MPIRenderModule";
#else
      renderModuleName = "LODRenderModule";
#endif
      }
    }
  
  vtkSMProxy *p = pxm->NewProxy("rendermodules", renderModuleName);
  if (!p)
    {
    return 0;
    }
  
  vtkSMRenderModuleProxy* rm = vtkSMRenderModuleProxy::SafeDownCast(p);
  if (!rm)
    {
    vtkErrorMacro("Render Module must be a subclass of vtkSMRenderModuleProxy.");
    p->Delete();
    return 0;
    }
  rm->UpdateVTKObjects();
  this->SetRenderModuleProxy(rm);
  pm->GetOptions()->SetRenderModuleName(renderModuleName);
  rm->Delete();
  return 1;
}

//-----------------------------------------------------------------------------

vtkSMProxyManager *pqApplication::GetProxyManager()
{
  return GetServer()->GetProxyManager();
}

//-----------------------------------------------------------------------------

vtkSMDisplayProxy *pqApplication::AddPart(vtkSMSourceProxy *part)
{
  // without this, you will get runtime errors from the part display
  // (connected below). this should be fixed
  part->CreateParts();

  // Create part display.
  vtkSMRenderModuleProxy *rm = this->GetRenderModuleProxy();
  vtkSMDisplayProxy *partdisplay = rm->CreateDisplayProxy();

  // Set the part as input to the part display.
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(partdisplay->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(part);

  vtkSMDataObjectDisplayProxy *dod
    = vtkSMDataObjectDisplayProxy::SafeDownCast(partdisplay);
  if (dod)
    {
    dod->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
    }

  partdisplay->UpdateVTKObjects();

  // Add the part display to the render module.
  pp = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->AddProxy(partdisplay);
  rm->UpdateVTKObjects();

  // Allow the render module proxy to maintain the part display.
  partdisplay->Delete();

  return partdisplay;
}

void pqApplication::RemovePart(vtkSMDisplayProxy *part)
{
  vtkSMRenderModuleProxy *rm = this->GetRenderModuleProxy();
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->RemoveProxy(part);
  rm->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------

void pqApplication::ResetCamera()
{
  double bounds[6];
  vtkSMRenderModuleProxy *rendermodule = this->GetRenderModuleProxy();
  rendermodule->ComputeVisiblePropBounds(bounds);
  if (   (bounds[0] <= bounds[1])
      && (bounds[2] <= bounds[3])
      && (bounds[4] <= bounds[5]) )
    {
    rendermodule->ResetCamera(bounds);
    }
}

//-----------------------------------------------------------------------------

void pqApplication::StillRender()
{
  this->GetRenderModuleProxy()->StillRender();
}

void pqApplication::InteractiveRender()
{
  this->GetRenderModuleProxy()->InteractiveRender();
}

//-----------------------------------------------------------------------------

vtkPVGenericRenderWindowInteractor *pqApplication::GetInteractor()
{
  return vtkPVGenericRenderWindowInteractor::SafeDownCast(
                                      this->RenderModuleProxy->GetInteractor());
}
