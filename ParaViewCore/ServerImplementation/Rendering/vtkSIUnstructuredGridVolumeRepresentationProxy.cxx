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
#ifndef VTKGL2
  stream << vtkClientServerStream::Invoke << self << "AddVolumeMapper"
         << "HAVS" << this->GetSubSIProxy("VolumeHAVSMapper")->GetVTKObject()
         << vtkClientServerStream::End;
#endif
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
#ifndef VTKGL2
  vtkSIRenderViewProxy* rvp = vtkSIRenderViewProxy::SafeDownCast(view);
  if (!rvp)
  {
    return;
  }
  vtkPVOpenGLExtensionsInformation* glinfo = rvp->GetOpenGLExtensionsInformation();
  if (glinfo)
  {
    // These are extensions needed for HAVS. It would be nice
    // if these was some way of asking the HAVS mapper the extensions
    // it needs rather than hardcoding it here.
    int supports_GL_EXT_texture3D = glinfo->ExtensionSupported("GL_EXT_texture3D");
    int supports_GL_EXT_framebuffer_object =
      glinfo->ExtensionSupported("GL_EXT_framebuffer_object");
    int supports_GL_ARB_fragment_program = glinfo->ExtensionSupported("GL_ARB_fragment_program");
    int supports_GL_ARB_vertex_program = glinfo->ExtensionSupported("GL_ARB_vertex_program");
    int supports_GL_ARB_texture_float = glinfo->ExtensionSupported("GL_ARB_texture_float");
    int supports_GL_ATI_texture_float = glinfo->ExtensionSupported("GL_ATI_texture_float");

    if (!supports_GL_EXT_texture3D || !supports_GL_EXT_framebuffer_object ||
      !supports_GL_ARB_fragment_program || !supports_GL_ARB_vertex_program ||
      !(supports_GL_ARB_texture_float || supports_GL_ATI_texture_float))
    {
      this->SupportsHAVSMapper = 0;
    }
    else
    {
      this->SupportsHAVSMapper = 1;
    }
  }
#endif
  this->RenderViewExtensionsTested = 1;
}
#endif

//----------------------------------------------------------------------------
void vtkSIUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
