/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPipelineState.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPipelineState.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStateLocator.h"

#include "vtkPVSession.h"

#include <assert.h>
#include <sstream>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMPipelineState);
//----------------------------------------------------------------------------
vtkSMPipelineState::vtkSMPipelineState()
{
  this->SetGlobalID(vtkSMSessionProxyManager::GetReservedGlobalID());
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMPipelineState::~vtkSMPipelineState()
{
}

//----------------------------------------------------------------------------
const vtkSMMessage* vtkSMPipelineState::GetFullState()
{
  assert("Session should be valid" && this->Session);
  return this->GetSessionProxyManager()->GetFullState();
}
//----------------------------------------------------------------------------
void vtkSMPipelineState::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  assert("Session should be valid" && this->Session);
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  if (this->ClientOnlyLocationFlag)
  {
    pxm->DisableStateUpdateNotification();
    pxm->LoadState(msg, locator);
    pxm->EnableStateUpdateNotification();
  }
  else
  {
    pxm->LoadState(msg, locator);
  }
}
//----------------------------------------------------------------------------
void vtkSMPipelineState::ValidateState()
{
  if (this->Session)
  {
    vtkSMMessage msg;
    msg.CopyFrom(*this->GetFullState());
    //    cout << "~~~~~~~~~~~~~ PUSH pxm state to server ~~~~~~~~~~~~~~~~" << endl;
    //    msg.PrintDebugString();
    //    cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    this->PushState(&msg);
  }
}
//----------------------------------------------------------------------------
void vtkSMPipelineState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
