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

void vtkSMPVDWriterProxy::UpdatePipeline()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int idx;

  for (idx = 0; idx < this->GetNumberOfIDs(); idx++)
    {
    str << vtkClientServerStream::Invoke
        << this->GetID(idx)
        << "SetNumberOfPieces"
        << pm->GetNumberOfPartitions()
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID()
        << "GetPartitionId"
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
        << this->GetID(idx)
        << "SetPiece"
        << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str);
    }

  this->Superclass::UpdatePipeline();

  for (idx = 0; idx < this->GetNumberOfIDs(); idx++)
    {
    str << vtkClientServerStream::Invoke
        << this->GetID(idx)
        << "Write"
        << vtkClientServerStream::End;
    }

  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str);
    }
}

void vtkSMPVDWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
