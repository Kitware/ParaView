// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMMaterialLibraryProxy.h"

#include "vtkClientServerStream.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkPVSession.h"
#include "vtkPVTrivialProducer.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include <numeric>

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

vtkStandardNewMacro(vtkSMMaterialLibraryProxy);

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::LoadMaterials(const char* filename)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ReadFile" << filename
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER_ROOT);

  // Synchronize from server to client
  this->Synchronize();
#else
  (void)filename;
#endif
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::LoadDefaultMaterials()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (this->DefaultMaterialsLoaded)
  {
    return;
  }

  vtkLogF(WARNING, "Loading default OSPRay materials. This operation can take a few seconds.");
  this->DefaultMaterialsLoaded = true;

  // todo: this should be relative to binary or in prefs/settings, see pq
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ReadRelativeFile"
         << "ospray_mats.json" << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER_ROOT);
  this->Synchronize();
#endif
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::Synchronize(
  vtkPVSession::ServerFlags from, vtkPVSession::ServerFlags to)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  bool builtinMode = false;
  if (!this->GetSession() || (this->GetSession()->GetProcessRoles() & vtkPVSession::SERVERS) != 0)
  {
    // avoid serialization in serial since uneccessary and can be slow
    builtinMode = true;
  }
  if (!builtinMode)
  {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "WriteBuffer"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream, false, from);

    vtkClientServerStream res = this->GetLastResult(from);
    std::string resbuf = "";
    res.GetArgument(0, 0, &resbuf);
    if (!resbuf.empty())
    {
      vtkClientServerStream stream2;
      stream2 << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ReadBuffer" << resbuf
              << vtkClientServerStream::End;
      this->ExecuteStream(stream2, false, to);
    }
  }

  vtkOSPRayMaterialLibrary* ml = vtkOSPRayMaterialLibrary::SafeDownCast(
    vtkPVMaterialLibrary::SafeDownCast(this->GetClientSideObject())->GetMaterialLibrary());
  ml->Fire();
#else
  (void)from;
  (void)to;
#endif
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::UpdateVTKObjects()
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find("LoadMaterials");
  if (it->second.ModifiedFlag)
  {
    const char* filename = vtkSMPropertyHelper(this, "LoadMaterials").GetAsString();
    this->LoadMaterials(filename);
  }
  this->Superclass::UpdateVTKObjects();
}
