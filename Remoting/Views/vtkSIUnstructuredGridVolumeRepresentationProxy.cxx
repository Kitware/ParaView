/*=========================================================================

  Program:   ParaView
  Module:    vtkSIUnstructuredGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIUnstructuredGridVolumeRepresentationProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIUnstructuredGridVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIUnstructuredGridVolumeRepresentationProxy::vtkSIUnstructuredGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSIUnstructuredGridVolumeRepresentationProxy::~vtkSIUnstructuredGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSIUnstructuredGridVolumeRepresentationProxy::CreateVTKObjects()
{
  if (!this->Superclass::CreateVTKObjects())
  {
    return false;
  }

  vtkObjectBase* self = this->GetVTKObject();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << self << "AddVolumeMapper"
         << "Projected tetra" << this->GetSubSIProxy("VolumePTMapper")->GetVTKObject()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << self << "AddVolumeMapper"
         << "Z sweep" << this->GetSubSIProxy("VolumeZSweepMapper")->GetVTKObject()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << self << "AddVolumeMapper"
         << "Bunyk ray cast" << this->GetSubSIProxy("VolumeBunykMapper")->GetVTKObject()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke << self << "AddVolumeMapper"
         << "Resample To Image"
         << this->GetSubSIProxy("VolumeResampleToImageMapper")->GetVTKObject()
         << vtkClientServerStream::End;
  return this->Interpreter->ProcessStream(stream) ? true : false;
}

#ifdef FIXME_COLLABORATION
// This FIXME is from view-restructuring days.
//-----------------------------------------------------------------------------
void vtkSIUnstructuredGridVolumeRepresentationProxy::UpdateRenderViewExtensions(
  vtkSIViewProxy* view)
{
  this->RenderViewExtensionsTested = 1;
}
#endif

//----------------------------------------------------------------------------
void vtkSIUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
