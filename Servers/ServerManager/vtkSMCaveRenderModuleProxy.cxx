/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCaveRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCaveRenderModuleProxy.h"

#include <sys/stat.h>
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMProxyProperty.h"
#include "vtkCaveRenderManager.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkRenderWindow.h"
#include "vtkSMProxyManager.h"
#include "vtkPVServerInformation.h"

vtkStandardNewMacro(vtkSMCaveRenderModuleProxy);
//-----------------------------------------------------------------------------
vtkSMCaveRenderModuleProxy::vtkSMCaveRenderModuleProxy()
{
  this->SetDisplayXMLName("MultiDisplay");
}

//-----------------------------------------------------------------------------
vtkSMCaveRenderModuleProxy::~vtkSMCaveRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::CreateRenderSyncManager()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  vtkSMProxy* cm = pxm->NewProxy("composite_managers", "CaveRenderManager" );
  if (!cm)
    {
    vtkErrorMacro("Failed to create RenderSyncManagerProxy.");
    return;
    }
  cm->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  cm->SetConnectionID(this->ConnectionID);
  this->AddSubProxy("RenderSyncManager", cm);
  cm->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::InitializeCompositingPipeline()
{
  if (!this->RenderSyncManagerProxy)
    {
    vtkErrorMacro("RenderSyncManagerProxy not set.");
    return;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;

  // We had trouble with SGI/aliasing with compositing.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions(this->ConnectionID ) > 1))
    {
    stream << vtkClientServerStream::Invoke
           << this->RenderWindowProxy->GetID() 
           << "SetMultiSamples" << 0
           << vtkClientServerStream::End;
    pm->SendStream(this->RenderWindowProxy->GetConnectionID(),
      this->RenderWindowProxy->GetServers(), stream);
    }

  if (pm->GetOptions()->GetClientMode())
    {
    // using vtkClientRenderSyncManager. 
    // Clean up this mess !!!!!!!!!!!!!
    // Even a cast to vtkPVClientServerModule would be better than this.
    // How can we syncronize the process modules and render modules?
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
           << "GetClientMode" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID() 
           << "SetClientFlag"
           << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    
    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
           << "GetRenderServerSocketController" 
           << pm->GetConnectionClientServerID(this->ConnectionID)
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke 
           << this->RenderSyncManagerProxy->GetID()
           << "SetSocketController" << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    pm->SendStream(this->RenderSyncManagerProxy->GetConnectionID(),
      this->RenderSyncManagerProxy->GetServers(), stream);
    }

  this->Superclass::InitializeCompositingPipeline();
  //TODO: I am almost certain that I have messed up the "Timing is 
  //critical" stuff in vtkPVCaveRenderModule. Must verify that.

  if (pm->GetOptions()->GetClientMode())
    {
    int numDisplays;
    numDisplays = pm->GetNumberOfPartitions(this->ConnectionID);
    // Setup the tiles.
    // We need a better way to retreive the number of processes
    vtkMPIMToNSocketConnection* m2n = NULL;
    if (pm->GetMPIMToNSocketConnectionID(this->ConnectionID).ID)
      {
      m2n = vtkMPIMToNSocketConnection::SafeDownCast(
        pm->GetObjectFromID(pm->GetMPIMToNSocketConnectionID(this->ConnectionID)));
      }   
    if (m2n)
      {
      numDisplays = m2n->GetNumberOfConnections();
      }    
    this->LoadConfigurationFile(numDisplays);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::LoadConfigurationFile(int numDisplays)
{
  int idx;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  const char* fileName = pm->GetOptions()->GetCaveConfigurationFileName();
  ifstream *File = 0;
  if(!fileName)
    {
    this->ConfigureFromServerInformation();
    return;
    }

  vtkWarningMacro("Cave parameters should be specified in the XML "
                  "configuration file. The --cave-configuration (and -cc) "
                  "command-line arguments have been deprecated and will be "
                  "removed in the next ParaView release.");
  
  // Open the new file
  struct stat fs;
  if ( !stat( fileName, &fs) )
    {
#ifdef _WIN32
    File = new ifstream(fileName, ios::in | ios::binary);
#else
    File = new ifstream(fileName, ios::in);
#endif
    }
  if (! File)
    {
    vtkErrorMacro(<< "Initialize: Could not open file " << fileName);
    return;
    }

  if (File->fail())
    {
    File->close();
    delete File;
    vtkErrorMacro(<< "Initialize: Could not open file " << fileName);
    return;
    }

  vtkCaveRenderManager* crm = 
    vtkCaveRenderManager::SafeDownCast(pm->GetObjectFromID(
        this->RenderSyncManagerProxy->GetID()));

  for (idx = 0; idx < numDisplays; ++idx)
    { // Just a test case.  Configuration file later.
    char displayName[256];
    double o[3];
    double x[3];
    double y[3];

    File->getline(displayName,256);
    if (File->fail())
      {
      File->close();
      delete File;
      vtkErrorMacro(<< "Could not read display " << idx);
      return;
      }
    pm->SetProcessEnvironmentVariable(idx, displayName); 

    *File >> o[0];
    *File >> o[1];
    *File >> o[2];

    *File >> x[0];
    *File >> x[1];
    *File >> x[2];

    *File >> y[0];
    *File >> y[1];
    *File >> y[2];

    if (File->fail())
      {
      File->close();
      delete File;
      vtkErrorMacro("Unexpected end of configuration file.");
      return;
      }

    crm->DefineDisplay(idx, o, x, y);
    }
  File->close();
  delete File;
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::ConfigureFromServerInformation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVServerInformation* serverInfo = pm->GetServerInformation(
    this->ConnectionID);
  vtkCaveRenderManager* crm = 
    vtkCaveRenderManager::SafeDownCast(pm->GetObjectFromID(
        this->RenderSyncManagerProxy->GetID()));

  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  for (idx = 0; idx < numMachines; idx++)
    {
    crm->DefineDisplay(idx, serverInfo->GetLowerLeft(idx),
                       serverInfo->GetLowerRight(idx),
                       serverInfo->GetUpperLeft(idx));
    }
    
}

//-----------------------------------------------------------------------------
int vtkSMCaveRenderModuleProxy::GetLocalRenderDecision(unsigned long,
   int vtkNotUsed(stillRender))
{
  return 1; // for Cave, always 1.
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
