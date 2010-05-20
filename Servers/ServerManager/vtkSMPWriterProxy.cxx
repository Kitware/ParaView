/*=========================================================================

Program:   ParaView
Module:    vtkSMPWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPWriterProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMPWriterProxy);
//-----------------------------------------------------------------------------
vtkSMPWriterProxy::vtkSMPWriterProxy()
{
  this->SupportsParallel = 1;
}

//-----------------------------------------------------------------------------
vtkSMPWriterProxy::~vtkSMPWriterProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;

  // Some writers used SetStartPiece/SetEndPiece API while others use SetPiece
  // API. To handle both cases, we simply call both while telling the process
  // module to ignore any interpretor errors.
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "SetReportInterpreterErrors" << 0
      << vtkClientServerStream::End;

  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetNumberOfLocalPartitions"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetNumberOfPieces"
      << vtkClientServerStream::LastResult 
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);

  // ALTERNATIVE: 1
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetPartitionId"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetStartPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "GetPartitionId"
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetEndPiece"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, str);

  // ALTERNATIVE: 2
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

  // Restore ReportInterpreterErrors flag.
  str << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID()
      << "SetReportInterpreterErrors" << 1
      << vtkClientServerStream::End;

  pm->SendStream(this->ConnectionID, this->Servers, str);
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
