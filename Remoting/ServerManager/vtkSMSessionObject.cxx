// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSessionObject.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

//---------------------------------------------------------------------------
vtkSMSessionObject::vtkScopedMonitorProgress::vtkScopedMonitorProgress(vtkSMSessionObject* parent)
  : Parent(parent)
{
  if (this->Parent && this->Parent->GetSession())
  {
    this->Parent->GetSession()->PrepareProgress();
  }
}

//---------------------------------------------------------------------------
vtkSMSessionObject::vtkScopedMonitorProgress::~vtkScopedMonitorProgress()
{
  if (this->Parent && this->Parent->GetSession())
  {
    this->Parent->GetSession()->CleanupPendingProgress();
  }
}

vtkStandardNewMacro(vtkSMSessionObject);
//---------------------------------------------------------------------------
vtkSMSessionObject::vtkSMSessionObject() = default;

//---------------------------------------------------------------------------
vtkSMSessionObject::~vtkSMSessionObject() = default;

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMSessionObject::GetSessionProxyManager()
{
  return vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(this->Session);
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMSessionObject::GetSession()
{
  return this->Session;
}

//----------------------------------------------------------------------------
void vtkSMSessionObject::SetSession(vtkSMSession* session)
{
  if (this->Session != session)
  {
    this->Session = session;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMSessionObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Session: " << this->Session << endl;
}
