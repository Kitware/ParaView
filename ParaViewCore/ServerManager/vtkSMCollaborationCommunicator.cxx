/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCollaborationCommunicator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCollaborationCommunicator.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMSession.h"
#include "vtkReservedRemoteObjectIds.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMCollaborationCommunicator);
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMCollaborationCommunicator::GetReservedGlobalID()
{
  return vtkReservedRemoteObjectIds::RESERVED_COLLABORATION_COMMUNICATOR_ID;
}
//----------------------------------------------------------------------------
vtkSMCollaborationCommunicator::vtkSMCollaborationCommunicator()
{
}

//----------------------------------------------------------------------------
vtkSMCollaborationCommunicator::~vtkSMCollaborationCommunicator()
{
}

//----------------------------------------------------------------------------
void vtkSMCollaborationCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
void vtkSMCollaborationCommunicator::LoadState( const vtkSMMessage* msg,
                                                vtkSMStateLocator* locator,
                                                bool definitionOnly)
{

}
