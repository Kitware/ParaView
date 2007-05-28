/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSummaryHelperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMSummaryHelperProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"

vtkStandardNewMacro(vtkSMSummaryHelperProxy);
vtkCxxRevisionMacro(vtkSMSummaryHelperProxy, "1.3");
//-----------------------------------------------------------------------------
vtkSMSummaryHelperProxy::vtkSMSummaryHelperProxy()
{
  this->SetServers(vtkProcessModule::DATA_SERVER);
}

//-----------------------------------------------------------------------------
vtkSMSummaryHelperProxy::~vtkSMSummaryHelperProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMSummaryHelperProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();

  vtkProcessModule* pm =vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetController" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID() 
         << "SetController" 
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//-----------------------------------------------------------------------------
void vtkSMSummaryHelperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
