/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMSelectionRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::vtkSMSelectionRepresentationProxy()
{

}

//----------------------------------------------------------------------------
vtkSMSelectionRepresentationProxy::~vtkSMSelectionRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::CreateVTKObjects()
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

  vtkSMProxy* label_repr = this->GetSubProxy("LabelRepresentation");

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetID()
    << "SetLabelRepresentation"
    << label_repr->GetID()
    << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMSelectionRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


