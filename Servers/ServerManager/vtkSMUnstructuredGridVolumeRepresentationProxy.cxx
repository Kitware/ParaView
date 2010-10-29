/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridVolumeRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridVolumeRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"


vtkStandardNewMacro(vtkSMUnstructuredGridVolumeRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::vtkSMUnstructuredGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridVolumeRepresentationProxy::~vtkSMUnstructuredGridVolumeRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::CreateVTKObjects()
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

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "Projected tetra"
         << this->GetSubProxy("VolumePTMapper")->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "HAVS"
         << this->GetSubProxy("VolumeHAVSMapper")->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "Z sweep"
         << this->GetSubProxy("VolumeZSweepMapper")->GetID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "AddVolumeMapper"
         << "Bunyk ray cast"
         << this->GetSubProxy("VolumeBunykMapper")->GetID()
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->Servers, stream);
}

#ifdef FIXME
//-----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::UpdateRenderViewExtensions(
  vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* rvp = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!rvp)
    {
    return;
    }
  vtkPVOpenGLExtensionsInformation* glinfo =
    rvp->GetOpenGLExtensionsInformation();
  if (glinfo)
    {
    // These are extensions needed for HAVS. It would be nice
    // if these was some way of asking the HAVS mapper the extensions
    // it needs rather than hardcoding it here.
    int supports_GL_EXT_texture3D =
      glinfo->ExtensionSupported( "GL_EXT_texture3D");
    int supports_GL_EXT_framebuffer_object =
      glinfo->ExtensionSupported( "GL_EXT_framebuffer_object");
    int supports_GL_ARB_fragment_program =
      glinfo->ExtensionSupported( "GL_ARB_fragment_program" );
    int supports_GL_ARB_vertex_program =
      glinfo->ExtensionSupported( "GL_ARB_vertex_program" );
    int supports_GL_ARB_texture_float =
      glinfo->ExtensionSupported( "GL_ARB_texture_float" );
    int supports_GL_ATI_texture_float =
      glinfo->ExtensionSupported( "GL_ATI_texture_float" );

    if ( !supports_GL_EXT_texture3D ||
      !supports_GL_EXT_framebuffer_object ||
      !supports_GL_ARB_fragment_program ||
      !supports_GL_ARB_vertex_program ||
      !(supports_GL_ARB_texture_float || supports_GL_ATI_texture_float))
      {
      this->SupportsHAVSMapper = 0;
      }
    else
      {
      this->SupportsHAVSMapper = 1;
      }
    }
  this->RenderViewExtensionsTested = 1;
}
#endif


//----------------------------------------------------------------------------
void vtkSMUnstructuredGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


