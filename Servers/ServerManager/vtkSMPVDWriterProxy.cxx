/*=========================================================================

Program:   ParaView
Module:    vtkSMPVDWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVDWriterProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMPVDWriterProxy);
vtkCxxRevisionMacro(vtkSMPVDWriterProxy, "Revision: 1.1 $");
//-----------------------------------------------------------------------------
void vtkSMPVDWriterProxy::UpdatePipeline()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetNumberOfPieces"
      << pm->GetNumberOfPartitions(this->ConnectionID)
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetPartitionId"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);

  this->Superclass::UpdatePipeline();

  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);
}

//-----------------------------------------------------------------------------
void vtkSMPVDWriterProxy::UpdatePipeline(double time)
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetNumberOfPieces"
      << pm->GetNumberOfPartitions(this->ConnectionID)
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetPartitionId"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);

  this->Superclass::UpdatePipeline(time);

  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "Write"
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);
}

void vtkSMPVDWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
