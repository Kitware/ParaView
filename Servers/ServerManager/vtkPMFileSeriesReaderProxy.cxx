/*=========================================================================

  Program:   ParaView
  Module:    vtkPMFileSeriesReaderProxy

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMFileSeriesReaderProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVXMLElement.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <assert.h>

vtkStandardNewMacro(vtkPMFileSeriesReaderProxy);
//----------------------------------------------------------------------------
vtkPMFileSeriesReaderProxy::vtkPMFileSeriesReaderProxy()
{
  this->FileNameMethod = 0;
}

//----------------------------------------------------------------------------
vtkPMFileSeriesReaderProxy::~vtkPMFileSeriesReaderProxy()
{
  this->SetFileNameMethod(0);
}

//----------------------------------------------------------------------------
bool vtkPMFileSeriesReaderProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if(!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  // Connect reader and set filename method
  vtkObjectBase *reader = this->GetSubProxyHelper("Reader")->GetVTKObject();
  if (!reader)
    {
    vtkErrorMacro("Missing subproxy: Reader");
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObjectID() << "SetReader" << reader
         << vtkClientServerStream::End;
  if (this->GetFileNameMethod())
    {
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObjectID()
           << "SetFileNameMethod"
           << this->GetFileNameMethod()
           << vtkClientServerStream::End;
    }
  if (!this->Interpreter->ProcessStream(stream))
    {
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPMFileSeriesReaderProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  bool ret = this->Superclass::ReadXMLAttributes(element);
  const char* fileNameMethod = element->GetAttribute("file_name_method");
  if(fileNameMethod && ret)
    {
    this->SetFileNameMethod(fileNameMethod);
    }
  return ret;
}

//----------------------------------------------------------------------------
void vtkPMFileSeriesReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
