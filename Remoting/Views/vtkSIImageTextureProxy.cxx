// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIImageTextureProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIImageTextureProxy);
//----------------------------------------------------------------------------
vtkSIImageTextureProxy::vtkSIImageTextureProxy() = default;

//----------------------------------------------------------------------------
vtkSIImageTextureProxy::~vtkSIImageTextureProxy() = default;

//----------------------------------------------------------------------------
void vtkSIImageTextureProxy::OnCreateVTKObjects()
{
  this->Superclass::OnCreateVTKObjects();

  // Do the binding between the SubProxy source to the local input
  vtkSIProxy* reader = this->GetSubSIProxy("Source");
  if (reader)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << reader->GetVTKObject() << "GetOutputPort"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << this->GetVTKObject() << "SetInputConnection"
           << vtkClientServerStream::LastResult << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
  }
}

//----------------------------------------------------------------------------
void vtkSIImageTextureProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
