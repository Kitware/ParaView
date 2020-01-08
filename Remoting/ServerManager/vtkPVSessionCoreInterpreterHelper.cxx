/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionCoreInterpreterHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSessionCoreInterpreterHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVSession.h"
#include "vtkPVSessionCore.h"
#include "vtkProcessModule.h"
#include "vtkSIProxy.h"

vtkStandardNewMacro(vtkPVSessionCoreInterpreterHelper);
//----------------------------------------------------------------------------
vtkPVSessionCoreInterpreterHelper::vtkPVSessionCoreInterpreterHelper()
{
  this->LogLevel = 0;
}

//----------------------------------------------------------------------------
vtkPVSessionCoreInterpreterHelper::~vtkPVSessionCoreInterpreterHelper()
{
}

//----------------------------------------------------------------------------
void vtkPVSessionCoreInterpreterHelper::SetCore(vtkPVSessionCore* core)
{
  this->Core = core;
}

//----------------------------------------------------------------------------
vtkSIObject* vtkPVSessionCoreInterpreterHelper::GetSIObject(vtkTypeUInt32 gid)
{
  return this->Core->GetSIObject(gid);
}
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVSessionCoreInterpreterHelper::GetNextGlobalIdChunk(vtkTypeUInt32 chunkSize)
{
  return this->Core->GetNextChunkGlobalUniqueIdentifier(chunkSize);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkPVSessionCoreInterpreterHelper::GetVTKObject(vtkTypeUInt32 gid)
{
  vtkSIProxy* siProxy = vtkSIProxy::SafeDownCast(this->Core->GetSIObject(gid));
  if (!siProxy)
  {
    switch (this->LogLevel)
    {
      case 0:
        vtkErrorMacro("No vtkSIProxy for id : " << gid);
        break;
      default:
        vtkWarningMacro("No vtkSIProxy for id : " << gid);
    }
    return NULL;
  }
  return siProxy->GetVTKObject();
}

//----------------------------------------------------------------------------
vtkProcessModule* vtkPVSessionCoreInterpreterHelper::GetProcessModule()
{
  return vtkProcessModule::GetProcessModule();
}

//----------------------------------------------------------------------------
vtkPVProgressHandler* vtkPVSessionCoreInterpreterHelper::GetActiveProgressHandler()
{
  vtkPVSession* session =
    vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetActiveSession());
  if (!session)
  {
    session = vtkPVSession::SafeDownCast(vtkProcessModule::GetProcessModule()->GetSession());
  }

  return session ? session->GetProgressHandler() : NULL;
}

//----------------------------------------------------------------------------
void vtkPVSessionCoreInterpreterHelper::SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection* m2n)
{
  this->Core->SetMPIMToNSocketConnection(m2n);
}

//----------------------------------------------------------------------------
void vtkPVSessionCoreInterpreterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
