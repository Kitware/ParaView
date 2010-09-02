/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSession.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionCore.h"

vtkStandardNewMacro(vtkSMSession);
//----------------------------------------------------------------------------
vtkSMSession::vtkSMSession()
{
  this->Core = vtkSMSessionCore::New();
  this->ProxyManager = vtkSMProxyManager::New();
  this->ProxyManager->SetSession(this);
  this->ProxyManager->SetProxyDefinitionManager(
    this->Core->GetProxyDefinitionManager());
  this->LastGUID = NULL;
}

//----------------------------------------------------------------------------
vtkSMSession::~vtkSMSession()
{
  this->Core->Delete();
  this->Core = NULL;
  this->ProxyManager->Delete();
  this->ProxyManager = NULL;
}

//----------------------------------------------------------------------------
void vtkSMSession::PushState(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PushState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::PullState(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->PullState(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::Invoke(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->Invoke(msg);
}

//----------------------------------------------------------------------------
void vtkSMSession::DeletePMObject(vtkSMMessage* msg)
{
  // This class does not handle remote sessions, so all messages are directly
  // processes locally.
  this->Core->DeletePMObject(msg);
}

//----------------------------------------------------------------------------
vtkSMProxyManager* vtkSMSession::GetProxyManager()
{
  return this->ProxyManager;
}

//----------------------------------------------------------------------------
bool vtkSMSession::GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  return this->Core->GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
void vtkSMSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
