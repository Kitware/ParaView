/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMaterialLibrary.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMaterialLibrary.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkProcessModule.h"
#include "vtkResourceFileLocator.h"
#include "vtkVersion.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayMaterialLibrary.h"
#endif

#include <cassert>
#include <string>
#include <vtksys/SystemTools.hxx>

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP = ';';
#else
const char ENV_PATH_SEP = ':';
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMaterialLibrary);

//-----------------------------------------------------------------------------
vtkPVMaterialLibrary::vtkPVMaterialLibrary()
{
// initialize the class that will act as library of materials
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->MaterialLibrary = vtkOSPRayMaterialLibrary::New();
#else
  this->MaterialLibrary = nullptr;
#endif

  // now look for any materials that we want preloaded into the application
  this->SearchPaths = nullptr;
  std::string paths;

  // user can define a path via environment variables
  const char* env = vtksys::SystemTools::GetEnv("PV_MATERIALS_PATH");
  if (env)
  {
    paths += env;
  }

  // and we look in some default locations relative to the exe
  std::string vtklibs = vtkGetLibraryPathForSymbol(GetVTKVersion);
  if (vtklibs.empty())
  {
    // this can happen for static builds (see paraview/paraview#19775),
    // in which case we just we the executable path to locate the materials.
    auto pm = vtkProcessModule::GetProcessModule();
    vtklibs = pm ? std::string(pm->GetSelfDir()) : std::string();
  }

  // where materials might be in relation to that
  std::vector<std::string> prefixes = {
#if defined(_WIN32) || defined(__APPLE__)
    "materials"
#else
    "share/paraview-" PARAVIEW_VERSION "/materials"
#endif
  };
  // search
  vtkNew<vtkResourceFileLocator> locator;
  auto resource_dir = locator->Locate(vtklibs, prefixes, "ospray_mats.json");
  if (!resource_dir.empty())
  {
    // append results to search path
    if (paths.size())
    {
      paths += ENV_PATH_SEP;
    }
    paths += resource_dir.c_str();
  }

  // now we are ready
  this->SetSearchPaths(paths.c_str());
}

//-----------------------------------------------------------------------------
vtkPVMaterialLibrary::~vtkPVMaterialLibrary()
{
  this->SetSearchPaths(nullptr);
  if (this->MaterialLibrary)
  {
    this->MaterialLibrary->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SearchPaths: " << (this->SearchPaths ? this->SearchPaths : "(none)") << endl;
}

//-----------------------------------------------------------------------------
void vtkPVMaterialLibrary::ReadRelativeFile(const char* FileName)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  std::vector<std::string> paths;
  vtksys::SystemTools::Split(this->SearchPaths, paths, ENV_PATH_SEP);
  for (size_t cc = 0; cc < paths.size(); cc++)
  {
    std::vector<std::string> subpaths;
    vtksys::SystemTools::Split(paths[cc].c_str(), subpaths, ';');
    for (size_t scc = 0; scc < subpaths.size(); scc++)
    {
      std::string fullpath = subpaths[scc] + "/" + FileName;
      if (this->ReadFile(fullpath.c_str()))
      {
        return;
      }
    }
  }
#else
  (void)FileName;
#endif
}

//-----------------------------------------------------------------------------
vtkObject* vtkPVMaterialLibrary::GetMaterialLibrary()
{
  return this->MaterialLibrary;
}

//-----------------------------------------------------------------------------
bool vtkPVMaterialLibrary::ReadFile(const char* FileName)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->ReadFile(FileName);
#else
  (void)FileName;
  return false;
#endif
}

//-----------------------------------------------------------------------------
bool vtkPVMaterialLibrary::ReadBuffer(const char* FileName)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->ReadBuffer(FileName);
#else
  (void)FileName;
  return false;
#endif
}

//-----------------------------------------------------------------------------
const char* vtkPVMaterialLibrary::WriteBuffer()
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->WriteBuffer();
#else
  return nullptr;
#endif
}
