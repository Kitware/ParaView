/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqOptions.h"
#include "pqServer.h"

#include <vtkObjectFactory.h>
#include <vtkProcessModuleGUIHelper.h>
#include <vtkPVClientServerModule.h>
#include <vtkPVOptions.h>
#include <vtkPVProcessModule.h>
#include <vtkPVServerInformation.h>
#include <vtkSMApplication.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>

// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkVolumeRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkWidgetsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVServerCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);

namespace
{

vtkSMApplication* pqGetSMApplication()
{
  static vtkSMApplication* sm_application = 0;
  if(!sm_application)
    {
    sm_application = vtkSMApplication::New();
    sm_application->Initialize();
    vtkSMProperty::SetCheckDomains(0);
    }
    
   return sm_application;
}

class pqProcessModuleGUIHelper :
  public vtkProcessModuleGUIHelper
{
public:
  static pqProcessModuleGUIHelper* New();

  virtual int OpenConnectionDialog(int *)
  {
    return 0;
  }

  virtual void SendPrepareProgress()
  {
  }
  
  virtual void SetLocalProgress(const char *filter, int progress)
  {
  }
  
  virtual void SendCleanupPendingProgress()
  {
  }

  virtual void ExitApplication()
  {
  }

  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
  {
    return 0;
  }

protected:
  pqProcessModuleGUIHelper() {}

private:
  pqProcessModuleGUIHelper(const pqProcessModuleGUIHelper&);
  void operator=(const pqProcessModuleGUIHelper&);
};

vtkStandardNewMacro(pqProcessModuleGUIHelper);

vtkProcessModule* pqGetProcessModule()
{
  if(!vtkProcessModule::GetProcessModule())
    {
    pqOptions* const options = pqOptions::New();
    
    vtkProcessModule* process_module = vtkPVProcessModule::New();
//      vtkProcessModule* process_module = vtkPVClientServerModule::New();
//      options->SetProcessType(vtkPVOptions::PVCLIENT);
      
/*
    if(op->GetClientMode() || op->GetServerMode() || op->GetRenderServerMode()) 
      {
      pm = vtkPVClientServerModule::New();
      }
    else
      {
  #ifdef VTK_USE_MPI
      pm = vtkPVMPIProcessModule::New();
  #else 
      pm = vtkPVProcessModule::New();
  #endif
      }
*/

    process_module->Initialize();
    process_module->SetOptions(options);
    process_module->SetGUIHelper(pqProcessModuleGUIHelper::New());
    
    // Initialize built-in wrapper modules.
    vtkCommonCS_Initialize(process_module->GetInterpreter());
    vtkFilteringCS_Initialize(process_module->GetInterpreter());
    vtkGenericFilteringCS_Initialize(process_module->GetInterpreter());
    vtkImagingCS_Initialize(process_module->GetInterpreter());
    vtkGraphicsCS_Initialize(process_module->GetInterpreter());
    vtkIOCS_Initialize(process_module->GetInterpreter());
    vtkRenderingCS_Initialize(process_module->GetInterpreter());
    vtkVolumeRenderingCS_Initialize(process_module->GetInterpreter());
    vtkHybridCS_Initialize(process_module->GetInterpreter());
    vtkWidgetsCS_Initialize(process_module->GetInterpreter());
    vtkParallelCS_Initialize(process_module->GetInterpreter());
    vtkPVServerCommonCS_Initialize(process_module->GetInterpreter());
    vtkPVFiltersCS_Initialize(process_module->GetInterpreter());
    vtkXdmfCS_Initialize(process_module->GetInterpreter());
    
    vtkProcessModule::SetProcessModule(process_module);
    
//    process_module->Start(0, 0);
  }
  
  return vtkProcessModule::GetProcessModule();
}

vtkSMProxyManager* pqGetProxyManager()
{
  return pqGetSMApplication()->GetProxyManager();
}

vtkSMRenderModuleProxy* pqGetRenderModule()
{
  static vtkSMRenderModuleProxy* render_module_proxy = 0;
  if(!render_module_proxy)
    {
      vtkSMProxyManager* const pxm = pqGetSMApplication()->GetProxyManager();
      vtkPVProcessModule *pm = vtkPVProcessModule::SafeDownCast(pqGetProcessModule());
      pm->SynchronizeServerClientOptions();

      const char* renderModuleName = 0;

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
      
      render_module_proxy = vtkSMRenderModuleProxy::SafeDownCast(p);
      render_module_proxy->UpdateVTKObjects();

      pm->GetOptions()->SetRenderModuleName(renderModuleName);
    }
  
  return render_module_proxy;
}

} // namespace


class pqServer::implementation
{
};

pqServer::pqServer() :
  Implementation(new implementation())
{
}

pqServer::~pqServer()
{
  delete Implementation;
}

vtkProcessModule* pqServer::GetProcessModule()
{
  return pqGetProcessModule();
}

vtkSMProxyManager* pqServer::GetProxyManager()
{
  return pqGetProxyManager();
}

vtkSMRenderModuleProxy* pqServer::GetRenderModule()
{
  return pqGetRenderModule();
}
  
