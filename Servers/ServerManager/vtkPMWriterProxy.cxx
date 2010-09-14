/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMWriterProxy.h"

#include "vtkParallelSerialWriter.h"
#include "vtkFileSeriesWriter.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPMInputProperty.h"
#include "vtkProcessModule2.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkPMWriterProxy);
//----------------------------------------------------------------------------
vtkPMWriterProxy::vtkPMWriterProxy()
{
  this->FileNameMethod = NULL;
}

//----------------------------------------------------------------------------
vtkPMWriterProxy::~vtkPMWriterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkPMWriterProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(element))
    {
    return false;
    }

  const char* setFileNameMethod = element->GetAttribute("file_name_method");
  if (setFileNameMethod)
    {
    this->SetFileNameMethod(setFileNameMethod);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPMWriterProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkParallelSerialWriter* psw =
    vtkParallelSerialWriter::SafeDownCast(this->GetVTKObject());
  if (psw)
    {
    psw->SetInterpreter(this->Interpreter);
    }

  vtkFileSeriesWriter* fsw =
    vtkFileSeriesWriter::SafeDownCast(this->GetVTKObject());
  if (fsw)
    {
    fsw->SetInterpreter(this->Interpreter);
    }

  vtkPMProxy* writerProxy = this->GetSubProxyHelper("Writer");
  if (writerProxy)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->GetVTKObjectID()
      << "SetWriter"
      << writerProxy->GetVTKObjectID()
      << vtkClientServerStream::End;
    if (this->FileNameMethod)
      {
      stream << vtkClientServerStream::Invoke
        << this->GetVTKObjectID()
        << "SetFileNameMethod"
        << this->FileNameMethod
        << vtkClientServerStream::End;
      }
    this->Interpreter->ProcessStream(stream);
    }

  vtkPMProxy* helper = this->GetSubProxyHelper("PreGatherHelper");
  if (helper)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->GetVTKObjectID()
      << "SetPreGatherHelper" << helper->GetVTKObjectID()
      << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  helper = this->GetSubProxyHelper("PostGatherHelper");
  if (helper)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->GetVTKObjectID()
      << "SetPostGatherHelper" << helper->GetVTKObjectID()
      << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  // Pass piece/process information to the writer if it needs it.
  vtkProcessModule2::GetProcessModule()->ReportInterpreterErrorsOff();
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  int numProcs = controller->GetNumberOfProcesses();
  int procId = controller->GetLocalProcessId();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->VTKObjectID
    << "SetNumberOfPieces"
    << numProcs
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  stream.Reset();

  // ALTERNATIVE: 1
  stream << vtkClientServerStream::Invoke
    << this->VTKObjectID
    << "SetStartPiece"
    << procId
    << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
    << this->VTKObjectID
    << "SetEndPiece"
    << procId
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  stream.Reset();

  // ALTERNATIVE: 2
  stream << vtkClientServerStream::Invoke
    << this->VTKObjectID
    << "SetPiece"
    << procId
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  vtkProcessModule2::GetProcessModule()->ReportInterpreterErrorsOn();
  stream.Reset();

  return true;
}

#ifdef FIXME
//----------------------------------------------------------------------------
vtkClientServerID vtkPMWriterProxy::GetVTKObjectID(
  vtkPMProperty* property_helper)
{
  if (vtkPMInputProperty::SafeDownCast(property_helper) &&
    !this->CompleteArraysID.IsNull())
    {
    return this->CompleteArraysID;
    }

  if (!this->WriterObjectID.IsNull())
    {
    if (property_helper->GetCommand() &&
      strcmp(property_helper->GetCommand(), "SetFileName") == 0)
      {
      return this->GetVTKObjectID();
      }
    return this->WriterObjectID;
    }

  return this->Superclass::GetVTKObjectID(property_helper);
}
#endif

//----------------------------------------------------------------------------
void vtkPMWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileNameMethod: " << this->FileNameMethod << endl;
}
