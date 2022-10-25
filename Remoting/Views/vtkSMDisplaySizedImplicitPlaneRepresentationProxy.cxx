/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplaySizedImplicitPlaneRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplaySizedImplicitPlaneRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDisplaySizedImplicitPlaneRepresentation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMDisplaySizedImplicitPlaneRepresentationProxy);

//---------------------------------------------------------------------------
vtkSMDisplaySizedImplicitPlaneRepresentationProxy::
  vtkSMDisplaySizedImplicitPlaneRepresentationProxy() = default;

//---------------------------------------------------------------------------
vtkSMDisplaySizedImplicitPlaneRepresentationProxy::
  ~vtkSMDisplaySizedImplicitPlaneRepresentationProxy() = default;

//---------------------------------------------------------------------------
void vtkSMDisplaySizedImplicitPlaneRepresentationProxy::SendRepresentation()
{
  vtkDisplaySizedImplicitPlaneRepresentation* rep =
    vtkDisplaySizedImplicitPlaneRepresentation::SafeDownCast(this->GetClientSideObject());

  int repState = rep->GetRepresentationState();
  // Don't bother to send to server if representation is the same.
  if (repState == this->RepresentationState)
  {
    return;
  }
  this->RepresentationState = repState;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "SetRepresentationState" << repState
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//---------------------------------------------------------------------------
void vtkSMDisplaySizedImplicitPlaneRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
