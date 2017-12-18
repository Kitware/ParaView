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

#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#ifdef PARAVIEW_USE_OSPRAY
#include "vtkOSPRayMaterialLibrary.h"
#endif
#include "vtkProcessModule.h"

#include <string>
#include <vtksys/SystemTools.hxx>

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP = ';';
#else
const char ENV_PATH_SEP = ':';
#endif

vtkStandardNewMacro(vtkPVMaterialLibrary);
//-----------------------------------------------------------------------------
vtkPVMaterialLibrary::vtkPVMaterialLibrary()
{
  this->SearchPaths = nullptr;
  vtksys::String paths;

  const char* env = vtksys::SystemTools::GetEnv("PV_MATERIALS_PATH");
  if (env)
  {
    paths += env;
  }

  /*
  //todo: compile time defined paths like so, see vtkPVPluginLoader
  #ifdef PARAVIEW_PLUGIN_LOADER_PATHS
    if (!paths.empty())
    {
      paths += ENV_PATH_SEP;
    }
    paths += PARAVIEW_PLUGIN_LOADER_PATHS;
  #endif
  */

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* opt = pm ? pm->GetOptions() : NULL;
  if (opt)
  {
    std::string appDir = vtkProcessModule::GetProcessModule()->GetSelfDir();
    if (appDir.size())
    {
      appDir += "/materials";
      if (paths.size())
      {
        paths += ENV_PATH_SEP;
      }
      paths += appDir;
    }
  }

  this->SetSearchPaths(paths.c_str());
#ifdef PARAVIEW_USE_OSPRAY
  this->MaterialLibrary = vtkOSPRayMaterialLibrary::New();
#else
  this->MaterialLibrary = nullptr;
#endif
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
#ifdef PARAVIEW_USE_OSPRAY
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
#ifdef PARAVIEW_USE_OSPRAY
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->ReadFile(FileName);
#else
  (void)FileName;
  return false;
#endif
}

//-----------------------------------------------------------------------------
bool vtkPVMaterialLibrary::ReadBuffer(const char* FileName)
{
#ifdef PARAVIEW_USE_OSPRAY
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->ReadBuffer(FileName);
#else
  (void)FileName;
  return false;
#endif
}

//-----------------------------------------------------------------------------
const char* vtkPVMaterialLibrary::WriteBuffer()
{
#ifdef PARAVIEW_USE_OSPRAY
  return vtkOSPRayMaterialLibrary::SafeDownCast(this->GetMaterialLibrary())->WriteBuffer();
#else
  return nullptr;
#endif
}
