/*=========================================================================

  Program:   ParaView
  Module:    vtkSISelectionRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSISelectionRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerInterpreter.h"

vtkStandardNewMacro(vtkSISelectionRepresentationProxy);
//----------------------------------------------------------------------------
vtkSISelectionRepresentationProxy::vtkSISelectionRepresentationProxy()
{

}

//----------------------------------------------------------------------------
vtkSISelectionRepresentationProxy::~vtkSISelectionRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSISelectionRepresentationProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkSIProxy* label_repr = this->GetSubSIProxy("LabelRepresentation");

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetVTKObject()
         << "SetLabelRepresentation"
         << label_repr->GetVTKObject()
         << vtkClientServerStream::End;

  return (this->Interpreter->ProcessStream(stream) != 0);
}

//----------------------------------------------------------------------------
void vtkSISelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
