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
vtkCxxRevisionMacro(vtkSMPWriterProxy, "1.1");
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
void vtkSMPWriterProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
 
  if (this->VTKClassName && strstr(this->VTKClassName, "XMLP"))
    {
    unsigned int idx;
    for (idx = 0; idx < this->GetNumberOfIDs(); idx++)
      {
      str << vtkClientServerStream::Invoke
        << this->GetID(idx)
        << "SetNumberOfPieces"
        << pm->GetNumberOfPartitions(this->ConnectionID)
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
      pm->SendStream(this->ConnectionID, this->Servers, str);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::AddInput(vtkSMSourceProxy* input, const char* method,
  int hasMultipleInputs)
{

  vtkSMSourceProxy* completeArrays = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("CompleteArrays"));
  if (completeArrays)
    {

    vtkSMInputProperty* ivp  = vtkSMInputProperty::SafeDownCast(
      completeArrays->GetProperty("Input"));
    ivp->RemoveAllProxies();
    ivp->AddProxy(input);
    input = completeArrays; // change the actual input to the writer to be
      // output of complete arrays.
    completeArrays->UpdateVTKObjects();
    }

  this->Superclass::AddInput(input, method, hasMultipleInputs);

  vtkSMProxy* sumHelper = this->GetSubProxy("SummaryHelper");
  if (sumHelper)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;

    for (unsigned int cc=0; cc < sumHelper->GetNumberOfIDs(); cc++)
      {
      stream << vtkClientServerStream::Invoke
             << sumHelper->GetID(cc) << "SetWriter" << this->GetID(0)
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << pm->GetProcessModuleID() << "GetController"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke
             << sumHelper->GetID(cc) << "SetController"
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::UpdatePipeline()
{
  vtkSMProxy* sumHelper = this->GetSubProxy("SummaryHelper");
  if (sumHelper)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;

    for (unsigned int cc=0; cc < sumHelper->GetNumberOfIDs(); cc++)
      {
      stream << vtkClientServerStream::Invoke
             << sumHelper->GetID(cc) 
             << "SynchronizeSummaryFiles"
             << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  this->Superclass::UpdatePipeline();
}



//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
