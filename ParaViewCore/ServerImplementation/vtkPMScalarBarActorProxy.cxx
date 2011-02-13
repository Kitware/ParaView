/*=========================================================================

  Program:   ParaView
  Module:    vtkPMScalarBarActorProxy

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMScalarBarActorProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <assert.h>

vtkStandardNewMacro(vtkPMScalarBarActorProxy);
//----------------------------------------------------------------------------
vtkPMScalarBarActorProxy::vtkPMScalarBarActorProxy()
{
}
//----------------------------------------------------------------------------
vtkPMScalarBarActorProxy::~vtkPMScalarBarActorProxy()
{
}
//----------------------------------------------------------------------------
bool vtkPMScalarBarActorProxy::CreateVTKObjects(vtkSMMessage* message)
{
  bool return_value = this->Superclass::CreateVTKObjects(message);

  if(return_value)
    {
    vtkClientServerStream stream;

    // Label Text property
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetLabelTextProperty"
           << this->GetSubProxyHelper("LabelTextProperty")->GetVTKObject()
           << vtkClientServerStream::End;

    // Title text property
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetTitleTextProperty"
           << this->GetSubProxyHelper("TitleTextProperty")->GetVTKObject()
           << vtkClientServerStream::End;

    // Execute
    return this->Interpreter->ProcessStream(stream);
    }
  return return_value;
}

//----------------------------------------------------------------------------
void vtkPMScalarBarActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
