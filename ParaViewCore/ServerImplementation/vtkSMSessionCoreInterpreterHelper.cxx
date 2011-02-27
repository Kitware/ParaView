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
#include "vtkSMSessionCoreInterpreterHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSIProxy.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSMSessionCore.h"

vtkStandardNewMacro(vtkSMSessionCoreInterpreterHelper);
//----------------------------------------------------------------------------
vtkSMSessionCoreInterpreterHelper::vtkSMSessionCoreInterpreterHelper()
{
}

//----------------------------------------------------------------------------
vtkSMSessionCoreInterpreterHelper::~vtkSMSessionCoreInterpreterHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMSessionCoreInterpreterHelper::SetCore(vtkSMSessionCore* core)
{
  this->Core = core;
}

//----------------------------------------------------------------------------
vtkSIObject* vtkSMSessionCoreInterpreterHelper::GetPMObject(vtkTypeUInt32 gid)
{
  return this->Core->GetPMObject(gid);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSMSessionCoreInterpreterHelper::GetVTKObject(vtkTypeUInt32 gid)
{
  vtkSIProxy* pmproxy = vtkSIProxy::SafeDownCast(
    this->Core->GetPMObject(gid));
  if (!pmproxy)
    {
    vtkErrorMacro("No vtkSIProxy for id : " << gid);
    return NULL;
    }
  return pmproxy->GetVTKObject();
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkSMSessionCoreInterpreterHelper::GetProcessModule()
{
  return vtkProcessModule::GetProcessModule();
}

//----------------------------------------------------------------------------
vtkPVProgressHandler* vtkSMSessionCoreInterpreterHelper::GetActiveProgressHandler()
{
  vtkPVSession* session = vtkPVSession::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetActiveSession());
  if (!session)
    {
    session = vtkPVSession::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetSession());
    }

  return session? session->GetProgressHandler() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMSessionCoreInterpreterHelper::SetMPIMToNSocketConnection(
  vtkMPIMToNSocketConnection* m2n)
{
  this->Core->SetMPIMToNSocketConnection(m2n);
}

//----------------------------------------------------------------------------
void vtkSMSessionCoreInterpreterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
