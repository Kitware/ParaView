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
vtkCxxRevisionMacro(vtkSMPWriterProxy, "1.5");
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

  int isXMLPWriter = 0;
  int isPVDWriter = 0;

  str << vtkClientServerStream::Invoke
    << this->GetID()
    << "IsA"
    << "vtkXMLPDataWriter"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 vtkProcessModule::GetRootId(this->Servers), str);
  pm->GetLastResult(this->ConnectionID,
                    vtkProcessModule::GetRootId(this->Servers)).GetArgument(0, 0, &isXMLPWriter);

  str << vtkClientServerStream::Invoke
    << this->GetID()
    << "IsA"
    << "vtkXMLPVDWriter"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::GetRootId(this->Servers), str);
  pm->GetLastResult(this->ConnectionID,
    vtkProcessModule::GetRootId(this->Servers)).GetArgument(0, 0, &isPVDWriter);

  if (isXMLPWriter)
    {
    str << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID()
        << "GetNumberOfLocalPartitions"
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
        << this->GetID()
        << "SetNumberOfPieces"
        << vtkClientServerStream::LastResult 
        << vtkClientServerStream::End;
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
    }
  else if (isPVDWriter)
    {
    str << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID()
        << "GetNumberOfLocalPartitions"
        << vtkClientServerStream::End;
    str << vtkClientServerStream::Invoke
        << this->GetID()
        << "SetNumberOfPieces"
        << vtkClientServerStream::LastResult 
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
    }

  pm->SendStream(this->ConnectionID, this->Servers, str);
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::AddInput(vtkSMSourceProxy* input, 
                                 const char* method,
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

    stream << vtkClientServerStream::Invoke
           << sumHelper->GetID() << "SetWriter" << this->GetID()
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "GetController"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << sumHelper->GetID() << "SetController"
           << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
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

    stream << vtkClientServerStream::Invoke
           << sumHelper->GetID() 
           << "SynchronizeSummaryFiles"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  this->Superclass::UpdatePipeline();
}

//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::UpdatePipeline(double time)
{
  vtkSMProxy* sumHelper = this->GetSubProxy("SummaryHelper");
  if (sumHelper)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;

    stream << vtkClientServerStream::Invoke
           << sumHelper->GetID() 
           << "SynchronizeSummaryFiles"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  this->Superclass::UpdatePipeline(time);
}


//-----------------------------------------------------------------------------
void vtkSMPWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
