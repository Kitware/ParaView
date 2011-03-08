/*=========================================================================

  Program:   ParaView
  Module:    vtkSITextSourceRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSITextSourceRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSITextSourceRepresentationProxy);
//----------------------------------------------------------------------------
vtkSITextSourceRepresentationProxy::vtkSITextSourceRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSITextSourceRepresentationProxy::~vtkSITextSourceRepresentationProxy()
{
}


//----------------------------------------------------------------------------
bool vtkSITextSourceRepresentationProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObject() << "SetTextWidgetRepresentation"
         << this->GetSubSIProxy("TextWidgetRepresentation")->GetVTKObject()
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  return true;
}

//----------------------------------------------------------------------------
void vtkSITextSourceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
