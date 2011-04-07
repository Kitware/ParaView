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

#include "vtkDynamicLoader.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVServerManagerPluginInterface.h"

#include <vtkstd/string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#include <cstdlib>

#define vtkPVPluginLoaderDebugMacro(x)\
{ if (this->DebugPlugin) {\
  vtksys_ios::ostringstream vtkerror;\
  vtkerror << x;\
  vtkOutputWindowDisplayText(vtkerror.str().c_str());} }

#define vtkPVPluginLoaderErrorMacro(x)\
  vtkErrorMacro(<< x); this->SetErrorString(x);

namespace
{
  // Cleans successfully opened libs when the application quits.
  // BUG # 10293
  class vtkPVPluginLoaderCleaner
    {
    vtkstd::vector<vtkLibHandle> Handles;

  public:
    void Register(vtkLibHandle &handle)
      {
      this->Handles.push_back(handle);
      }

    ~vtkPVPluginLoaderCleaner()
      {
      for (vtkstd::vector<vtkLibHandle>::iterator iter = this->Handles.begin();
        iter != this->Handles.end(); ++iter)
        {
        vtkDynamicLoader::CloseLibrary(*iter);
        }
      }
    };
  static vtkPVPluginLoaderCleaner LibCleaner;
};


vtkStandardNewMacro(vtkPVPluginLoader);
//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->DebugPlugin = vtksys::SystemTools::GetEnv("PV_PLUGIN_DEBUG") != NULL;
  this->ErrorString = NULL;
  this->PluginName = NULL;
  this->PluginVersion = NULL;
  this->FileName = NULL;
  this->SearchPaths = NULL;
  this->Loaded = false;
  this->SetErrorString("No plugin loaded yet.");

  vtksys::String paths;
  const char* env = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if(env)
    {
    paths += env;
    vtkPVPluginLoaderDebugMacro("PV_PLUGIN_PATH: " << env);
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

    // pqPluginManager::pluginPaths() used to automatically load plugins a host
    // of locations. We no longer support that. It becomes less useful since we
    // now list plugins in the plugin manager dialog.
    }

  this->SetSearchPaths(paths.c_str());
}

//-----------------------------------------------------------------------------
vtkPVPluginLoader::~vtkPVPluginLoader()
{
  this->SetErrorString(0);
  this->SetPluginName(0);
  this->SetPluginVersion(0);
  this->SetFileName(0);
  this->SetSearchPaths(0);
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPlugin(const char* file)
{
  this->Loaded = false;
  vtkPVPluginLoaderDebugMacro(
    "\n***************************************************\n"
    "Attempting to load " << file);
  if (!file || file[0] == '\0')
    {
    vtkPVPluginLoaderErrorMacro("Invalid filename");
    return false;
    }

  this->SetFileName(file);
  vtkstd::string defaultname = vtksys::SystemTools::GetFilenameWithoutExtension(file);
  this->SetPluginName(defaultname.c_str());
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(file);
  if (!lib)
    {
    vtkPVPluginLoaderDebugMacro("Failed to load the shared library.");
    vtkPVPluginLoaderErrorMacro(vtkDynamicLoader::LastError());
    return false;
    }

  // So that the lib is closed when the application quits.
  // BUG #10293.
  ::LibCleaner.Register(lib);

  vtkPVPluginLoaderDebugMacro("Loaded shared library successfully. "
    "Now trying to validate that it's a ParaView plugin.");

  // A plugin shared library has two global functions:
  // * pv_plugin_query_verification_data -- to obtain version
  // * pv_plugin_instance -- to obtain the plugin instance.

  pv_plugin_query_verification_data_fptr pv_plugin_query_verification_data =
    (pv_plugin_query_verification_data_fptr)(
      vtkDynamicLoader::GetSymbolAddress(lib,
        "pv_plugin_query_verification_data"));
  if (!pv_plugin_query_verification_data)
    {
    vtkPVPluginLoaderDebugMacro("Failed to locate the global function "
      "\"pv_plugin_query_verification_data\" which is required to test the "
      "plugin signature. This may not be a ParaView plugin dll or maybe "
      "from a older version of ParaView when this function was not required.");
    vtkPVPluginLoaderErrorMacro(
      "Not a ParaView Plugin since could not locate the plugin-verification function");
    vtkDynamicLoader::CloseLibrary(lib);
    return false;
    }

  vtkstd::string pv_verfication_data = pv_plugin_query_verification_data();

  vtkPVPluginLoaderDebugMacro("Plugin's signature: " <<
    pv_verfication_data.c_str());

  // Validate the signature. If the signature is invalid, then this plugin is
  // totally bogus (even for the GUI layer).
  if (pv_verfication_data != __PV_PLUGIN_VERIFICATION_STRING__)
    {
    vtksys_ios::ostringstream error;
    error << "Mismatch in versions: \n" <<
      "ParaView Signature: " << __PV_PLUGIN_VERIFICATION_STRING__ << "\n"
      "Plugin Signature: " << pv_verfication_data.c_str();
    vtkPVPluginLoaderErrorMacro(error.str().c_str());
    vtkDynamicLoader::CloseLibrary(lib);
    vtkPVPluginLoaderDebugMacro(
      "Mismatch in versions signifies that the plugin was built for "
      "a different version of ParaView or with a different compilter. "
      "Look at the signatures to determine what caused the mismatch.");
    return false;
    }

  // If we succeeded so far, then obtain the instace of vtkPVPlugin for this
  // plugin and load it.

  pv_plugin_query_instance_fptr pv_plugin_query_instance =
    (pv_plugin_query_instance_fptr)(
      vtkDynamicLoader::GetSymbolAddress(lib,
        "pv_plugin_instance"));
  if (!pv_plugin_query_instance)
    {
    vtkPVPluginLoaderDebugMacro("We've encountered an error locating the other "
      "global function \"pv_plugin_instance\" which is required to locate the "
      "instance of the vtkPVPlugin class. Possibly the plugin shared library was "
      "not compiled properly.");
    vtkPVPluginLoaderErrorMacro(
      "Not a ParaView Plugin since could not locate the plugin-instance "
      "function.");
    vtkDynamicLoader::CloseLibrary(lib);
    return false;
    }

  vtkPVPluginLoaderDebugMacro("Plugin signature verification successful. "
    "This is definitely a ParaView plugin compiled with correct compiler for "
    "correct ParaView version.");

  // BUG # 0008673
  // Tell the platform to look in the plugin's directory for
  // its dependencies. This isn't the right thing to do. A better
  // solution would be to let the plugin tell us where to look so
  // that a list of locations could be added.
  vtkstd::string ldLibPath;
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
  vtkstd::string thisPluginsPath(file);
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
    vtkPVPluginLoaderDebugMacro("Updating Shared Library Paths: " <<
      ldLibPath.c_str());
    }

  vtkPVPlugin* plugin = pv_plugin_query_instance();
  this->SetPluginName(plugin->GetPluginName());
  this->SetPluginVersion(plugin->GetPluginVersionString());

  // Print some debug information about the plugin, if needed.
  vtkPVPluginLoaderDebugMacro("Plugin instance located successfully. "
    "Now loading components from the plugin instance based on the interfaces it "
    "implements.");
  vtkPVPluginLoaderDebugMacro(
    "----------------------------------------------------------------\n"
    "Plugin Information: \n"
    "  Name        : " << plugin->GetPluginName() << "\n"
    "  Version     : " << plugin->GetPluginVersionString() << "\n"
    "  ReqOnServer : " << plugin->GetRequiredOnServer() << "\n"
    "  ReqOnClient : " << plugin->GetRequiredOnClient() << "\n"
    "  ReqPlugins  : " << plugin->GetRequiredPlugins());
  vtkPVServerManagerPluginInterface* smplugin =
    dynamic_cast<vtkPVServerManagerPluginInterface*>(plugin);
  if (smplugin)
    {
    vtkPVPluginLoaderDebugMacro("  ServerManager Plugin : Yes");
    }
  else
    {
    vtkPVPluginLoaderDebugMacro("  ServerManager Plugin : No");
    }

  vtkPVPythonPluginInterface *pyplugin =
    dynamic_cast<vtkPVPythonPluginInterface*>(plugin);
  if (pyplugin)
    {
    vtkPVPluginLoaderDebugMacro("  Python Plugin : Yes");
    }
  else
    {
    vtkPVPluginLoaderDebugMacro("  Python Plugin : No");
    }

  // Set the filename so the vtkPVPluginTracker knows what file this plugin was
  // loaded from.
  plugin->SetFileName(file);

  // From this point onwards the vtkPVPlugin travels the same path as a
  // statically imported plugin.
  vtkPVPlugin::ImportPlugin(plugin);
  this->Loaded = true;
  return true;
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginConfigurationXMLFromString(const char*
  xmlcontents)
{
  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLFromString(
    xmlcontents);
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DebugPlugin: " << this->DebugPlugin << endl;
  os << indent << "PluginName: " <<
    (this->PluginName? this->PluginName : "(none)") << endl;
  os << indent << "PluginVersion: " <<
    (this->PluginVersion? this->PluginVersion : "(none)") << endl;
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "SearchPaths: " <<
    (this->SearchPaths ? this->SearchPaths : "(none)") << endl;
}
