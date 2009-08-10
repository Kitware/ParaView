/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginLoader.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerInterpreter.h"
#include "vtkDynamicLoader.h"
#include "vtkIntArray.h"
#include "vtkPVOptions.h"
#include "vtkPVPluginInformation.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>
using vtkstd::string;
#include <cstdlib>

vtkStandardNewMacro(vtkPVPluginLoader);
vtkCxxRevisionMacro(vtkPVPluginLoader, "1.14");

#ifdef _WIN32
// __cdecl gives an unmangled name
#define C_DECL __cdecl
#else
#define C_DECL
#endif

typedef const char* (C_DECL *PluginNameFunc)();
typedef const char* (C_DECL *PluginVersionFunc)();
typedef int (C_DECL *PluginRequiredOnServerFunc)();
typedef int (C_DECL *PluginRequiredOnClientFunc)();
typedef const char* (C_DECL *PluginRequiredPluginsFunc)();
typedef const char* (C_DECL *PluginXML1)();
typedef void (C_DECL *PluginXML2)(int&, char**&);
typedef void (C_DECL *PluginPython)(int&, const char **&, const char **&,
                                    const int *&);
typedef void (C_DECL *PluginInit)(vtkClientServerInterpreter*);

//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->PluginInfo = vtkPVPluginInformation::New();
  this->ServerManagerXML = vtkStringArray::New();
  this->PythonModuleNames = vtkStringArray::New();
  this->PythonModuleSources = vtkStringArray::New();
  this->PythonPackageFlags = vtkIntArray::New();

  vtksys::String paths;
  const char* env = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if(env)
    {
    paths += env;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* opt = pm ? pm->GetOptions() : NULL;
  if(opt)
    {
    const char* path = opt->GetApplicationPath();
    vtksys::String appDir = vtksys::SystemTools::GetProgramPath(path);
    if(appDir.size())
      {
      appDir += "/plugins";
      if(paths.size())
        {
        paths += ";";
        }
      paths += appDir;
      }
    }

  this->PluginInfo->SetSearchPaths(paths.c_str());
}

//-----------------------------------------------------------------------------
vtkPVPluginLoader::~vtkPVPluginLoader()
{
  if(this->ServerManagerXML)
    {
    this->ServerManagerXML->Delete();
    }
  if(this->PythonModuleNames)
    {
    this->PythonModuleNames->Delete();
    }
  if(this->PythonModuleSources)
    {
    this->PythonModuleSources->Delete();
    }
  if(this->PythonPackageFlags)
    {
    this->PythonPackageFlags->Delete();
    }

  this->PluginInfo->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::SetFileName(const char* file)
{
  if(this->PluginInfo->GetLoaded())
    {
    return;
    }

  if(file && file[0] != '\0')
    {   
    this->PluginInfo->SetFileName(file);
    vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(file);
    if(lib)
      {
      // plugin name
      PluginNameFunc pluginName = 
        (PluginNameFunc)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginName");
      // plugin version
      PluginVersionFunc pluginVersion = 
        (PluginVersionFunc)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginVersion");
      if(pluginName && pluginVersion)
        {
        vtkstd::string pluginNameString = (*pluginName)();
        this->PluginInfo->SetPluginName(pluginNameString.c_str());

        vtkstd::string pluginVersionString = (*pluginVersion)();
        this->PluginInfo->SetPluginVersion(pluginVersionString.c_str());
        }
      // plugin RequiredOnServer flag
      PluginRequiredOnServerFunc pluginServerRequired = 
        (PluginRequiredOnServerFunc)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginRequiredOnServer");
      // plugin RequiredOnClient flag
      PluginRequiredOnClientFunc pluginClientRequired = 
        (PluginRequiredOnClientFunc)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginRequiredOnClient");
      if(pluginServerRequired && pluginClientRequired)
        {
        int serverRequired = pluginServerRequired();
        int clientRequired = pluginClientRequired();
        this->PluginInfo->SetRequiredOnServer(serverRequired);
        this->PluginInfo->SetRequiredOnClient(clientRequired);
        }
        
      // plugin required-plugins
      PluginRequiredPluginsFunc pluginRequiredPlugins = 
        (PluginRequiredPluginsFunc)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginRequiredPlugins");
      if(pluginRequiredPlugins)
        {
        const char* requiredPlugins = (*pluginRequiredPlugins)();
        if(requiredPlugins)
          {
          this->PluginInfo->SetRequiredPlugins(requiredPlugins);
          }
        }
        
      // BUG # 0008673
      // Tell the platform to look in the plugin's directory for 
      // its dependencies. This isn't the right thing to do. A better
      // solution would be to let the plugin tell us where to look so
      // that a list of locations could be added.
      string ldLibPath;
#if defined(_WIN32) && !defined(__CYGWIN__)
      const char LIB_PATH_SEP=';';
      const char PATH_SEP='\\';
      const char *LIB_PATH_NAME="PATH";
      ldLibPath=LIB_PATH_NAME;
      ldLibPath+='=';
#elif defined (__APPLE__)
      const char LIB_PATH_SEP=':';
      const char PATH_SEP='/';
      const char *LIB_PATH_NAME="DYLD_LIBRARY_PATH";
#else
      const char LIB_PATH_SEP=':';
      const char PATH_SEP='/';
      const char *LIB_PATH_NAME="LD_LIBRARY_PATH";
#endif
      // Trim the plugin name from the end of its path.
      string thisPluginsPath(file);
      size_t eop=thisPluginsPath.rfind(PATH_SEP);
      thisPluginsPath=thisPluginsPath.substr(0,eop);
      // Load the shared library search path.
      const char *pLdLibPath=getenv(LIB_PATH_NAME);
      bool pluginPathPresent
        = pLdLibPath==NULL?false:strstr(pLdLibPath,thisPluginsPath.c_str())!=NULL;
      // Update it.
      if (!pluginPathPresent)
        {
        // Make sure we are only adding it once, because there can
        // be multiple plugins in the same folder.
        if (pLdLibPath)
          {
          ldLibPath+=pLdLibPath;
          ldLibPath+=LIB_PATH_SEP;
          }
        ldLibPath+=thisPluginsPath;

        vtksys::SystemTools::PutEnv(ldLibPath.c_str());
        }

      // old SM xml, new plugins don't have this method
      PluginXML1 xml1 = 
        (PluginXML1)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginXML");

      // new SM xml
      PluginXML2 xml2 = 
        (PluginXML2)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginXMLList");

      PluginPython python =
        (PluginPython)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginPythonSourceList");

      PluginInit init = 
        (PluginInit)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginInit");
      if(xml1 || xml2 || python || init)
        {
        this->PluginInfo->SetLoaded(1);
        if(init)
          {
          (*init)(vtkProcessModule::GetProcessModule()->GetInterpreter());
          }
        if(xml1)
          {
          const char* xmlString = (*xml1)();
          if(xmlString)
            {
            this->ServerManagerXML->SetNumberOfTuples(1);
            this->ServerManagerXML->SetValue(0, vtkStdString(xmlString));
            }
          }
        if(xml2)
          {
          int num;
          char** xml;
          (*xml2)(num, xml);
          this->ServerManagerXML->SetNumberOfTuples(num);
          for(int i=0; i<num; i++)
            {
            this->ServerManagerXML->SetValue(i, vtkStdString(xml[i]));
            }
          }
        if (python)
          {
          int num;
          const char **name;
          const char **source;
          const int *packages;
          (*python)(num, name, source, packages);
          this->PythonModuleNames->SetNumberOfTuples(num);
          this->PythonModuleSources->SetNumberOfTuples(num);
          this->PythonPackageFlags->SetNumberOfTuples(num);
          for (int i = 0; i < num; i++)
            {
            this->PythonModuleNames->SetValue(i, vtkStdString(name[i]));
            this->PythonModuleSources->SetValue(i, vtkStdString(source[i]));
            this->PythonPackageFlags->SetValue(i, packages[i]);
            }
          }
        this->Modified();
        }
      else
        {
        // toss it out if it isn't a server manager plugin
        vtkDynamicLoader::CloseLibrary(lib);
        if(this->PluginInfo->GetRequiredOnServer() && 
          !this->PluginInfo->GetRequiredOnClient())
          {
          this->PluginInfo->SetError("There are no server manager components in this plugin.");
          }
        }
      }
    else
      {
      this->PluginInfo->SetError(vtkDynamicLoader::LastError());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ServerManagerXML: " 
    << (this->ServerManagerXML ? "(exists)" : "(none)") << endl;
  os << indent << "PythonModuleNames: " 
    << (this->PythonModuleNames ? "(exists)" : "(none)") << endl;
  os << indent << "PythonModuleSources: " 
    << (this->PythonModuleSources ? "(exists)" : "(none)") << endl;
  os << indent << "PythonPackageFlags: " 
    << (this->PythonPackageFlags ? "(exists)" : "(none)") << endl;
  os << indent << "PluginInfo: "  << endl;
  this->PluginInfo->PrintSelf(os, i2);
}

