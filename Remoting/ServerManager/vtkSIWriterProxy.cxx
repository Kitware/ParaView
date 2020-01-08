/*=========================================================================

  Program:   ParaView
  Module:    vtkSIWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIWriterProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSIInputProperty.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkSIWriterProxy);
//----------------------------------------------------------------------------
vtkSIWriterProxy::vtkSIWriterProxy()
{
  this->FileNameMethod = NULL;
}

//----------------------------------------------------------------------------
vtkSIWriterProxy::~vtkSIWriterProxy()
{
  this->SetFileNameMethod(NULL);
}

//----------------------------------------------------------------------------
bool vtkSIWriterProxy::ReadXMLAttributes(vtkPVXMLElement* element)
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
void vtkSIWriterProxy::OnCreateVTKObjects()
{
  this->Superclass::OnCreateVTKObjects();

  vtkObjectBase* object = this->GetVTKObject();
  vtkSIProxy* writerProxy = this->GetSubSIProxy("Writer");
  if (writerProxy)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << object << "SetWriter" << writerProxy->GetVTKObject()
           << vtkClientServerStream::End;
    if (this->FileNameMethod)
    {
      stream << vtkClientServerStream::Invoke << object << "SetFileNameMethod"
             << this->FileNameMethod << vtkClientServerStream::End;
    }
    this->Interpreter->ProcessStream(stream);
  }

  vtkSIProxy* helper = this->GetSubSIProxy("PreGatherHelper");
  if (helper)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << object << "SetPreGatherHelper"
           << helper->GetVTKObject() << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }

  helper = this->GetSubSIProxy("PostGatherHelper");
  if (helper)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << object << "SetPostGatherHelper"
           << helper->GetVTKObject() << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }

  // Pass piece/process information to the writer if it needs it.
  vtkProcessModule::GetProcessModule()->ReportInterpreterErrorsOff();
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int numProcs = controller->GetNumberOfProcesses();
  int procId = controller->GetLocalProcessId();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << object << "SetNumberOfPieces" << numProcs
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  stream.Reset();

  // ALTERNATIVE: 1
  stream << vtkClientServerStream::Invoke << object << "SetStartPiece" << procId
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << object << "SetEndPiece" << procId
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  stream.Reset();

  // ALTERNATIVE: 2
  stream << vtkClientServerStream::Invoke << object << "SetPiece" << procId
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  vtkProcessModule::GetProcessModule()->ReportInterpreterErrorsOn();
  stream.Reset();
}

//----------------------------------------------------------------------------
void vtkSIWriterProxy::AddInput(int input_port, vtkAlgorithmOutput* connection, const char* method)
{
  std::vector<vtkAlgorithm*> pipeline;
  if (auto passArrays = this->GetSubSIProxy("PassArrays"))
  {
    pipeline.push_back(vtkAlgorithm::SafeDownCast(passArrays->GetVTKObject()));
  }
  if (auto completeArrays = this->GetSubSIProxy("CompleteArrays"))
  {
    pipeline.push_back(vtkAlgorithm::SafeDownCast(completeArrays->GetVTKObject()));
  }

  if (pipeline.size() == 2)
  {
    pipeline.back()->SetInputConnection(pipeline.front()->GetOutputPort());
  }

  if (pipeline.size() > 0)
  {
    pipeline.front()->SetInputConnection(connection);
    this->Superclass::AddInput(input_port, pipeline.back()->GetOutputPort(), method);
  }
  else
  {
    this->Superclass::AddInput(input_port, connection, method);
  }
}

//----------------------------------------------------------------------------
void vtkSIWriterProxy::CleanInputs(const char* method)
{
  std::vector<vtkAlgorithm*> pipeline;
  if (auto passArrays = this->GetSubSIProxy("PassArrays"))
  {
    pipeline.push_back(vtkAlgorithm::SafeDownCast(passArrays->GetVTKObject()));
  }
  if (auto completeArrays = this->GetSubSIProxy("CompleteArrays"))
  {
    pipeline.push_back(vtkAlgorithm::SafeDownCast(completeArrays->GetVTKObject()));
  }

  if (pipeline.size() > 0)
  {
    pipeline.front()->SetInputConnection(nullptr);
    pipeline.back()->SetInputConnection(nullptr);
  }

  this->Superclass::CleanInputs(method);
}

//----------------------------------------------------------------------------
void vtkSIWriterProxy::UpdatePipelineTime(double time)
{
  vtkAlgorithm* writer = vtkAlgorithm::SafeDownCast(this->GetVTKObject());
  if (!writer)
  {
    vtkErrorMacro("Unexpected VTK object "
      << (this->GetVTKObject() ? this->GetVTKObject()->GetClassName() : "(NULL"));
    return;
  }
  for (int port = 0; port < writer->GetNumberOfInputPorts(); port++)
  {
    for (int c = 0; c < writer->GetNumberOfInputConnections(port); c++)
    {
      vtkInformation* info = writer->GetInputInformation(port, c);
      info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), time);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSIWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileNameMethod: " << (this->FileNameMethod ? this->FileNameMethod : "(none)")
     << endl;
}
