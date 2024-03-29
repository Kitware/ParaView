// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVSession.h"

#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVServerInformation.h"

//----------------------------------------------------------------------------
vtkPVSession::vtkPVSession()
{
  this->ProgressHandler = vtkPVProgressHandler::New();
  this->ProgressHandler->SetSession(this); // not reference counted.
  this->ProgressCount = 0;
  this->InCleanupPendingProgress = false;
}

//----------------------------------------------------------------------------
vtkPVSession::~vtkPVSession()
{
  this->ProgressHandler->SetSession(nullptr);
  this->ProgressHandler->Delete();
  this->ProgressHandler = nullptr;
}

//----------------------------------------------------------------------------
vtkPVSession::ServerFlags vtkPVSession::GetProcessRoles()
{
  return CLIENT_AND_SERVERS;
}

//----------------------------------------------------------------------------
vtkMultiProcessController* vtkPVSession::GetController(vtkPVSession::ServerFlags)
{
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVSession::GetPendingProgress()
{
  return (this->InCleanupPendingProgress || this->ProgressCount > 0);
}

//----------------------------------------------------------------------------
void vtkPVSession::PrepareProgress()
{
  if (this->InCleanupPendingProgress)
  {
    return;
  }

  if (this->ProgressCount++ == 0)
  {
    this->PrepareProgressInternal();
  }
}

//----------------------------------------------------------------------------
void vtkPVSession::CleanupPendingProgress()
{
  if (this->InCleanupPendingProgress)
  {
    return;
  }

  this->InCleanupPendingProgress = true;
  if (--this->ProgressCount == 0)
  {
    this->CleanupPendingProgressInternal();
  }
  if (this->ProgressCount < 0)
  {
    vtkErrorMacro("PrepareProgress and CleanupPendingProgress mismatch!");
    this->ProgressCount = 0;
  }
  this->InCleanupPendingProgress = false;
}

//----------------------------------------------------------------------------
void vtkPVSession::PrepareProgressInternal()
{
  this->ProgressHandler->PrepareProgress();
}

//----------------------------------------------------------------------------
void vtkPVSession::CleanupPendingProgressInternal()
{
  this->ProgressHandler->CleanupPendingProgress();
}

//-----------------------------------------------------------------------------
bool vtkPVSession::OnWrongTagEvent(vtkObject*, unsigned long, void* calldata)
{
  int tag = -1;
  int len = -1;
  const char* data = reinterpret_cast<const char*>(calldata);
  const char* ptr = data;
  memcpy(&tag, ptr, sizeof(tag));

  if (tag == vtkPVSession::EXCEPTION_EVENT_TAG)
  {
    ptr += sizeof(tag);
    memcpy(&len, ptr, sizeof(len));
    ptr += sizeof(len);
    vtkErrorMacro("Encountered Exception: " << ptr);
    // this->OnSocketError();
  }
  else
  {
    vtkErrorMacro("Internal ParaView Error: "
                  "Socket Communicator received wrong tag: "
      << tag);
    // Treat as a socket error.
    // this->OnSocketError();
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkPVSession::IsMultiClients()
{
  return (this->GetServerInformation()->GetMultiClientsEnable() != 0);
}
