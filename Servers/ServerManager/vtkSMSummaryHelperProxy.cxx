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
vtkCxxRevisionMacro(vtkSMSummaryHelperProxy, "1.1");
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
void vtkSMSummaryHelperProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);

  vtkProcessModule* pm =vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (int cc=0; cc < numObjects; cc++)
    {
    stream << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetController" << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
      << this->GetID(cc) << "SetController" << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMSummaryHelperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
