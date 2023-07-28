// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMServerStateLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMServerStateLocator);
//---------------------------------------------------------------------------
vtkSMServerStateLocator::vtkSMServerStateLocator()
{
  this->Session = nullptr;
}

//---------------------------------------------------------------------------
vtkSMServerStateLocator::~vtkSMServerStateLocator()
{
  this->SetSession(nullptr);
}

//---------------------------------------------------------------------------
void vtkSMServerStateLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkSMServerStateLocator::FindState(
  vtkTypeUInt32 globalID, vtkSMMessage* stateToFill, bool vtkNotUsed(useParent))
{
  bool foundInCache = true;
  if (!(foundInCache = this->Superclass::FindState(globalID, stateToFill, false)) &&
    this->Session && stateToFill)
  {
    vtkSMMessage newState;
    newState.set_global_id(globalID);
    newState.set_location(vtkPVSession::DATA_SERVER_ROOT);
    newState.set_req_def(true);
    this->Session->PullState(&newState);
    stateToFill->Clear();
    stateToFill->CopyFrom(newState);
    // We only rely on XML definition to figure out the SM classname
    if (!newState.HasExtension(ProxyState::xml_group))
    {
      //      cout << "--------- Skipped server State -------------" << endl;
      //      newState.PrintDebugString();
      //      cout << "---------------------------------------------------" << endl;
      return false;
    }
    //    cout << "--------- State fetch from the server -------------" << endl;
    //    stateToFill->PrintDebugString();
    //    cout << "---------------------------------------------------" << endl;
    // this->RegisterState(&newState);
    return true;
  }

  // Found in the cache ?
  return foundInCache;
}
//---------------------------------------------------------------------------
vtkSMSession* vtkSMServerStateLocator::GetSession()
{
  return this->Session.GetPointer();
}
//---------------------------------------------------------------------------
void vtkSMServerStateLocator::SetSession(vtkSMSession* session)
{
  this->Session = session;
}
