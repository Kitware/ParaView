// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

//-----------------------------------------------------------------------------
vtkSMUndoElement::vtkSMUndoElement()
{
  this->Session = nullptr;
}

//-----------------------------------------------------------------------------
vtkSMUndoElement::~vtkSMUndoElement()
{
  this->SetSession(nullptr);
}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMUndoElement::SetSession(vtkSMSession* session)
{
  if (this->Session != session)
  {
    this->Session = session;
    this->Modified();
  }
}
//----------------------------------------------------------------------------
vtkSMSession* vtkSMUndoElement::GetSession()
{
  return this->Session.GetPointer();
}
//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMUndoElement::GetSessionProxyManager()
{
  return vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(this->Session);
}
