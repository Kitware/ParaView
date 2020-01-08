/*=========================================================================

  Program:   ParaView
  Module:    vtkSIImageTextureProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIImageTextureProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIImageTextureProxy);
//----------------------------------------------------------------------------
vtkSIImageTextureProxy::vtkSIImageTextureProxy()
{
}

//----------------------------------------------------------------------------
vtkSIImageTextureProxy::~vtkSIImageTextureProxy()
{
}

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
