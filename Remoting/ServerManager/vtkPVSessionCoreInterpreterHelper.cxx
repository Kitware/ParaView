// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
vtkPVSessionCoreInterpreterHelper::~vtkPVSessionCoreInterpreterHelper() = default;

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
    return nullptr;
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

  return session ? session->GetProgressHandler() : nullptr;
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
