/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileSeriesReaderProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFileSeriesReaderProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMFileSeriesReaderProxy);

//-----------------------------------------------------------------------------
vtkSMFileSeriesReaderProxy::vtkSMFileSeriesReaderProxy()
{
  this->FileNameMethod = 0;
}

//-----------------------------------------------------------------------------
vtkSMFileSeriesReaderProxy::~vtkSMFileSeriesReaderProxy()
{
  this->SetFileNameMethod(0);
}

//-----------------------------------------------------------------------------
void vtkSMFileSeriesReaderProxy::CreateVTKObjects()
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

  vtkSMSourceProxy* reader = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Reader"));
  if (!reader)
    {
    vtkErrorMacro("Missing subproxy: Reader");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << this->GetID() << "SetReader" << reader->GetID() 
         << vtkClientServerStream::End;
  if (this->GetFileNameMethod())
    {
    stream << vtkClientServerStream::Invoke
           << this->GetID() 
           << "SetFileNameMethod" 
           << this->GetFileNameMethod()
           << vtkClientServerStream::End;
    }
  pm->SendStream(this->ConnectionID, this->Servers, stream);
} 

//---------------------------------------------------------------------------
int vtkSMFileSeriesReaderProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  const char* setFileNameMethod = element->GetAttribute("file_name_method");
  if(setFileNameMethod)
    {
    this->SetFileNameMethod(setFileNameMethod);
    }

  return this->Superclass::ReadXMLAttributes(pm, element);
}

//-----------------------------------------------------------------------------
void vtkSMFileSeriesReaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
