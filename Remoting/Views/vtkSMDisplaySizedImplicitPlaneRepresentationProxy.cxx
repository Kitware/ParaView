// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
