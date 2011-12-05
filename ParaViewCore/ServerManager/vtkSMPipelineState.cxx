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
#include "vtkSMSession.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStateLocator.h"
#include "vtkSMProxyLocator.h"

#include "vtkPVSession.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMPipelineState);
//----------------------------------------------------------------------------
vtkSMPipelineState::vtkSMPipelineState()
{
  this->SetGlobalID(vtkSMProxyManager::GetReservedGlobalID());
  this->SetLocation(vtkPVSession::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMPipelineState::~vtkSMPipelineState()
{
}

//----------------------------------------------------------------------------
const vtkSMMessage* vtkSMPipelineState::GetFullState()
{
  return vtkSMObject::GetProxyManager()->GetFullState();
}
//----------------------------------------------------------------------------
void vtkSMPipelineState::LoadState( const vtkSMMessage* msg,
                                    vtkSMProxyLocator* locator)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  if(this->ClientOnlyLocationFlag)
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
  if(this->Session)
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
