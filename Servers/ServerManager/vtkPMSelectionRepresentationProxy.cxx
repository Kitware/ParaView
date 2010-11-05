/*=========================================================================

  Program:   ParaView
  Module:    vtkPMSelectionRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMSelectionRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerInterpreter.h"

vtkStandardNewMacro(vtkPMSelectionRepresentationProxy);
//----------------------------------------------------------------------------
vtkPMSelectionRepresentationProxy::vtkPMSelectionRepresentationProxy()
{

}

//----------------------------------------------------------------------------
vtkPMSelectionRepresentationProxy::~vtkPMSelectionRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkPMSelectionRepresentationProxy::CreateVTKObjects(vtkSMMessage* message)
{
  if (this->ObjectsCreated)
    {
    return true;
    }

  if (!this->Superclass::CreateVTKObjects(message))
    {
    return false;
    }

  vtkPMProxy* label_repr = this->GetSubProxyHelper("LabelRepresentation");

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetVTKObjectID()
    << "SetLabelRepresentation"
    << label_repr->GetVTKObjectID()
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  return true;
}

//----------------------------------------------------------------------------
void vtkPMSelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
