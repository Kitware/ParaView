/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerRenderManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerRenderManager.h"

#include "vtkObjectFactory.h"
#include "vtkServerConnection.h"
#include "vtkWeakPointer.h"
#include "vtkSocketController.h"
#include "vtkProcessModule.h"

#include <vtkstd/vector>

class vtkPVClientServerRenderManager::vtkInternal
{
public:
  typedef vtkstd::vector<vtkWeakPointer<vtkRemoteConnection> >
    ConnectionsType;
  ConnectionsType Connections;
  unsigned int Find(vtkRemoteConnection* conn)
    {
    unsigned int index=0;
    ConnectionsType::iterator iter;
    for (iter = this->Connections.begin(); iter != this->Connections.end();
      ++iter, ++index)
      {
      if (iter->GetPointer() == conn)
        {
        return index;
        }
      }
    return VTK_UNSIGNED_INT_MAX;
    }
};

static void RenderRMI(void *arg, void *, int, int)
{
  vtkPVClientServerRenderManager *self = 
    reinterpret_cast<vtkPVClientServerRenderManager*>(arg);
  self->RenderRMI();
}


vtkCxxRevisionMacro(vtkPVClientServerRenderManager, "1.1");
//----------------------------------------------------------------------------
vtkPVClientServerRenderManager::vtkPVClientServerRenderManager()
{
  this->Internal = new vtkPVClientServerRenderManager::vtkInternal();
}

//----------------------------------------------------------------------------
vtkPVClientServerRenderManager::~vtkPVClientServerRenderManager()
{
  delete this->Internal;
  this->Internal = 0;
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::Initialize(vtkRemoteConnection* conn)
{
  if (!conn || this->Internal->Find(conn) != VTK_UNSIGNED_INT_MAX)
    {
    // Already initialized
    return;
    }

  vtkSocketController* soc = conn->GetSocketController();

  vtkServerConnection* sconn = vtkServerConnection::SafeDownCast(conn);
  if (sconn && sconn->GetRenderServerSocketController())
    {
    soc = sconn->GetRenderServerSocketController();
    }

  // Register for RenderRMI. As far as ParaView's concerned, we only need the
  // RenderRMI, hence I am manually setting that up.

  soc->AddRMI(::RenderRMI, this, vtkParallelRenderManager::RENDER_RMI_TAG);
}


//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::InitializeRMIs()
{
  //vtkWarningMacro(
  //  "Please use Initialize(vtkRemoteConnection*) instead.");
  this->Superclass::InitializeRMIs();
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::SetController(
  vtkMultiProcessController* controller)
{
  if (controller && (controller->GetNumberOfProcesses() != 2))
    {
    vtkErrorMacro("Client-Server needs controller with exactly 2 processes.");
    return;
    }
  // vtkWarningMacro(
  //   "Please use Initialize(vtkRemoteConnection*) instead.");
  this->Superclass::SetController(controller);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::Activate()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSocketController* soc = pm->GetActiveRenderServerSocketController();
  if (!soc)
    {
    abort();
    }
  this->SetController(soc);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::DeActivate()
{
  this->SetController(0);
}

//----------------------------------------------------------------------------
void vtkPVClientServerRenderManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


