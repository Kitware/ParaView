/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCommunicationModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCommunicationModule.h"

#include "vtkClientServerStream.h"

#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMCommunicationModule, "1.1");

//----------------------------------------------------------------------------
vtkSMCommunicationModule::vtkSMCommunicationModule()
{
  this->UniqueID.ID = 3;
}

//----------------------------------------------------------------------------
vtkSMCommunicationModule::~vtkSMCommunicationModule()
{
}

//----------------------------------------------------------------------------
vtkClientServerID vtkSMCommunicationModule::GetUniqueID()
{
  return vtkProcessModule::GetProcessModule()->GetUniqueID();

  // TODO revise this
  //this->UniqueID.ID++;
  //return this->UniqueID;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkSMCommunicationModule::NewStreamObject(
  const char* type, vtkClientServerStream& stream)
{
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

//----------------------------------------------------------------------------
void vtkSMCommunicationModule::DeleteStreamObject(
  vtkClientServerID id, vtkClientServerStream& stream)
{
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}

//---------------------------------------------------------------------------
void vtkSMCommunicationModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

