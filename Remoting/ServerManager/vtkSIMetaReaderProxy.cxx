// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIMetaReaderProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>
#include <vector>

#include <cassert>

vtkStandardNewMacro(vtkSIMetaReaderProxy);
//----------------------------------------------------------------------------
vtkSIMetaReaderProxy::vtkSIMetaReaderProxy()
{
  this->FileNameMethod = nullptr;
}

//----------------------------------------------------------------------------
vtkSIMetaReaderProxy::~vtkSIMetaReaderProxy()
{
  this->SetFileNameMethod(nullptr);
}

//----------------------------------------------------------------------------
void vtkSIMetaReaderProxy::OnCreateVTKObjects()
{
  this->Superclass::OnCreateVTKObjects();

  // Connect reader and set filename method
  vtkObjectBase* reader = this->GetSubSIProxy("Reader")->GetVTKObject();
  if (!reader)
  {
    vtkErrorMacro("Missing subproxy: Reader");
    return;
  }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << this->GetVTKObject() << "SetReader" << reader
         << vtkClientServerStream::End;
  if (this->GetFileNameMethod())
  {
    stream << vtkClientServerStream::Invoke << this->GetVTKObject() << "SetFileNameMethod"
           << this->GetFileNameMethod() << vtkClientServerStream::End;
  }
  this->Interpreter->ProcessStream(stream);
}

//----------------------------------------------------------------------------
bool vtkSIMetaReaderProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  bool ret = this->Superclass::ReadXMLAttributes(element);
  const char* fileNameMethod = element->GetAttribute("file_name_method");
  if (fileNameMethod && ret)
  {
    this->SetFileNameMethod(fileNameMethod);
  }
  return ret;
}

//----------------------------------------------------------------------------
void vtkSIMetaReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
