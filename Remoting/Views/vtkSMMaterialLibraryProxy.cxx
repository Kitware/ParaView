/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialLibraryProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMaterialLibraryProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVMaterialLibrary.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

#include "vtkSMParaViewPipelineController.h"

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

  this->Synchronize();
#else
  (void)filename;
#endif
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::LoadDefaultMaterials()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  // todo: this should be relative to binary or in prefs/settings, see pq
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ReadRelativeFile"
         << "ospray_mats.json" << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER_ROOT);
#endif
}

void vtkSMMaterialLibraryProxy::Synchronize()
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
    this->ExecuteStream(stream, false, vtkPVSession::RENDER_SERVER_ROOT);

    vtkClientServerStream res = this->GetLastResult(vtkPVSession::RENDER_SERVER_ROOT);
    std::string resbuf = "";
    res.GetArgument(0, 0, &resbuf);
    if (!resbuf.empty())
    {
      vtkClientServerStream stream2;
      stream2 << vtkClientServerStream::Invoke << VTKOBJECT(this) << "ReadBuffer" << resbuf
              << vtkClientServerStream::End;
      this->ExecuteStream(stream2, false, vtkProcessModule::CLIENT);
    }
  }

  vtkOSPRayMaterialLibrary* ml = vtkOSPRayMaterialLibrary::SafeDownCast(
    vtkPVMaterialLibrary::SafeDownCast(this->GetClientSideObject())->GetMaterialLibrary());
  ml->Fire();

  // materials are loaded, create the proxies accordingly
  this->ResetPropertiesToXMLDefaults();

  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();
  std::set<std::string> matNames = ml->GetMaterialNames();

  for (auto& matName : matNames)
  {
    std::string type = ml->LookupImplName(matName);
    vtkSmartPointer<vtkSMProxy> matProxy;
    matProxy.TakeReference(pxm->NewProxy("materials", "Material"));

    vtkSMPropertyHelper(matProxy, "Name").Set(matName.c_str());
    vtkSMPropertyHelper(matProxy, "Type").Set(type.c_str());

    // we need to update name and type before adding attributes
    matProxy->UpdateVTKObjects();

    auto varList = ml->GetDoubleShaderVariableList(matName);
    auto texList = ml->GetTextureList(matName);

    // This property is in rendering.xml
    // under the Material proxy
    vtkSMPropertyHelper(matProxy, "DoubleVariables")
      .SetNumberOfElements((varList.size() + texList.size()) * 2);

    vtkSMPropertyHelper(this, "Materials").Add(matProxy);

    // register have to be done after the proxy is added to the library in order to not
    // invoke register event too early
    pxm->RegisterProxy("materials", matProxy);

    int currentIndex = 0;
    // Add all the double variables
    for (auto& varName : varList)
    {
      vtkSMPropertyHelper(matProxy, "DoubleVariables").Set(currentIndex++, varName.c_str());

      auto varValue = ml->GetDoubleShaderVariable(matName, varName);

      std::string str =
        std::accumulate(varValue.begin() + 1, varValue.end(), std::to_string(varValue[0]),
          [](const std::string& a, double b) { return a + ' ' + std::to_string(b); });

      vtkSMPropertyHelper(matProxy, "DoubleVariables").Set(currentIndex++, str.c_str());
    }
    // Add all the texture
    for (auto& varName : texList)
    {
      vtkSMPropertyHelper(matProxy, "DoubleVariables").Set(currentIndex++, varName.c_str());

      // vtkTexture* texture = ml->GetTexture(matName, varName);
      // Todo get the name of the texture / load it

      // We need to get the filename to register the proxy
      // std::string filename = "/home/lfxpaul/dev/materials/textures/brushed.jpg";

      // //vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
      // vtkSMProxy* texture = pxm->NewProxy("textures", "ImageTexture");

      // pxm->RegisterProxy("textures", texture);

      // vtkSMPropertyHelper(texture, "FileName").Set(filename.c_str());

      // vtkNew<vtkSMParaViewPipelineController> controller;
      // controller->RegisterTextureProxy(texture, filename.c_str());

      // Could we register / create proxy ImageTexture here ?

      // vtkSMPropertyHelper(matProxy, "DoubleVariables").Set(currentIndex++, filename.c_str());
      vtkSMPropertyHelper(matProxy, "DoubleVariables").Set(currentIndex++, "None");
    }

    matProxy->UpdateVTKObjects();
  }

  this->UpdateVTKObjects();
#endif
}

//-----------------------------------------------------------------------------
void vtkSMMaterialLibraryProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
