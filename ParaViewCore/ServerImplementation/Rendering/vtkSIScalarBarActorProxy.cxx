/*=========================================================================

  Program:   ParaView
  Module:    vtkSIScalarBarActorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIScalarBarActorProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <assert.h>

vtkStandardNewMacro(vtkSIScalarBarActorProxy);
//----------------------------------------------------------------------------
vtkSIScalarBarActorProxy::vtkSIScalarBarActorProxy()
{
}
//----------------------------------------------------------------------------
vtkSIScalarBarActorProxy::~vtkSIScalarBarActorProxy()
{
}
//----------------------------------------------------------------------------
bool vtkSIScalarBarActorProxy::CreateVTKObjects(vtkSMMessage* message)
{
  bool return_value = this->Superclass::CreateVTKObjects(message);

  if(return_value)
    {
    vtkClientServerStream stream;

    // Label Text property
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetLabelTextProperty"
           << this->GetSubSIProxy("LabelTextProperty")->GetVTKObject()
           << vtkClientServerStream::End;

    // Title text property
    stream << vtkClientServerStream::Invoke
           << this->GetVTKObject()
           << "SetTitleTextProperty"
           << this->GetSubSIProxy("TitleTextProperty")->GetVTKObject()
           << vtkClientServerStream::End;

    // Execute
    return (this->Interpreter->ProcessStream(stream) != 0);
    }
  return return_value;
}

//----------------------------------------------------------------------------
void vtkSIScalarBarActorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
