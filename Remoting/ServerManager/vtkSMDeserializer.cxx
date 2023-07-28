// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDeserializer.h"

#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

//----------------------------------------------------------------------------
vtkSMDeserializer::vtkSMDeserializer() = default;

//----------------------------------------------------------------------------
vtkSMDeserializer::~vtkSMDeserializer() = default;

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMDeserializer::CreateProxy(
  const char* xmlgroup, const char* xmlname, const char* subname)
{
  vtkSMSessionProxyManager* pxm = this->SessionProxyManager;
  return pxm ? pxm->NewProxy(xmlgroup, xmlname, subname) : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::SetSession(vtkSMSession* session)
{
  this->SetSessionProxyManager(session ? session->GetSessionProxyManager() : nullptr);
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::SetSessionProxyManager(vtkSMSessionProxyManager* pxm)
{
  this->SessionProxyManager = pxm;
}

//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMDeserializer::GetSessionProxyManager()
{
  return this->SessionProxyManager;
}

//----------------------------------------------------------------------------
vtkSMSession* vtkSMDeserializer::GetSession()
{
  return this->SessionProxyManager ? this->SessionProxyManager->GetSession() : nullptr;
}

//----------------------------------------------------------------------------
void vtkSMDeserializer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SessionProxyManager: " << this->SessionProxyManager.GetPointer() << endl;
}
