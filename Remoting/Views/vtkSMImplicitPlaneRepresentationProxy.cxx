// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMImplicitPlaneRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMImplicitPlaneRepresentationProxy);

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::vtkSMImplicitPlaneRepresentationProxy() = default;

//---------------------------------------------------------------------------
vtkSMImplicitPlaneRepresentationProxy::~vtkSMImplicitPlaneRepresentationProxy() = default;

//---------------------------------------------------------------------------
void vtkSMImplicitPlaneRepresentationProxy::SendRepresentation()
{
  vtkImplicitPlaneRepresentation* rep =
    vtkImplicitPlaneRepresentation::SafeDownCast(this->GetClientSideObject());

  int repState = rep->GetRepresentationState();
  // Don't bother to server if representation is the same.
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
void vtkSMImplicitPlaneRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
