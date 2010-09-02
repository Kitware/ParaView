/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataSourceProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDataSourceProxy);

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::vtkSMDataSourceProxy()
{
}

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::~vtkSMDataSourceProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMDataSourceProxy::CopyData(vtkSMSourceProxy *sourceProxy)
{
  if (!sourceProxy || this->Servers != sourceProxy->GetServers())
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << sourceProxy->GetID() << "GetOutput"
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);

  stream  << vtkClientServerStream::Invoke
          << this->GetID() << "CopyData" 
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);

  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMDataSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
