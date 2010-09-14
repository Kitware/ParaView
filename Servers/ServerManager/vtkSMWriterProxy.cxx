/*=========================================================================

Program:   ParaView
Module:    vtkSMWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMWriterProxy);
//-----------------------------------------------------------------------------
vtkSMWriterProxy::vtkSMWriterProxy()
{
  this->SetKernelClassName("vtkPMWriterProxy");
  this->SupportsParallel = 0;
  this->ParallelOnly = 0;
  this->FileNameMethod = 0;
}

//-----------------------------------------------------------------------------
vtkSMWriterProxy::~vtkSMWriterProxy()
{
  this->SetFileNameMethod(0);
}

//-----------------------------------------------------------------------------
int vtkSMWriterProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
  vtkPVXMLElement* element)
{
  if (element->GetAttribute("supports_parallel"))
    {
    element->GetScalarAttribute("supports_parallel", &this->SupportsParallel);
    }

  if (element->GetAttribute("parallel_only"))
    {
    element->GetScalarAttribute("parallel_only", &this->ParallelOnly);
    }

  if (this->ParallelOnly)
    {
    this->SetSupportsParallel(1);
      // if ParallelOnly, then we must support Parallel.
    }

  const char* setFileNameMethod = element->GetAttribute("file_name_method");
  if (setFileNameMethod)
    {
    this->SetFileNameMethod(setFileNameMethod);
    }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline()
{
  //pm->SendPrepareProgress(this->ConnectionID);
  vtkSMMessage message;
  message << pvstream::InvokeRequest() << "Write";
  this->Invoke(&message);
  // pm->SendCleanupPendingProgress(this->ConnectionID);

  this->Superclass::UpdatePipeline();
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::UpdatePipeline(double time)
{
  //pm->SendPrepareProgress(this->ConnectionID);
  vtkSMMessage message;
  message << pvstream::InvokeRequest() << "Write";
  this->Invoke(&message);
  // pm->SendCleanupPendingProgress(this->ConnectionID);
  this->Superclass::UpdatePipeline(time);
}

//-----------------------------------------------------------------------------
void vtkSMWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SupportsParallel: "
     << this->SupportsParallel << endl;
  os << indent << "ParallelOnly: "
     << this->ParallelOnly << endl;
}
