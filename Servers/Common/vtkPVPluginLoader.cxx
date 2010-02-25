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

#include "vtkClientServerInterpreter.h"
#include "vtkDynamicLoader.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginInformation.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"

#include <vtkstd/string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

#include <cstdlib>

#define vtkPVPluginLoaderDebugMacro(x)\
{ if (this->DebugPlugin) {\
  vtksys_ios::ostringstream vtkerror;\
  vtkerror << x;\
  vtkOutputWindowDisplayText(vtkerror.str().c_str());} }

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
vtkCxxRevisionMacro(vtkPVPluginLoader, "1.21");
//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->PluginInfo = vtkPVPluginInformation::New();
  this->ServerManagerXML = vtkStringArray::New();
  this->PythonModuleNames = vtkStringArray::New();
  this->PythonModuleSources = vtkStringArray::New();
  this->PythonPackageFlags = vtkIntArray::New();
  this->DebugPlugin = vtksys::SystemTools::GetEnv("PV_PLUGIN_DEBUG") != NULL;

  vtksys::String paths;
  const char* env = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if(env)
    {
    paths += env;
    }
  vtkPVPluginLoaderDebugMacro("PV_PLUGIN_PATH: " << env);

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
  vtkPVPluginLoaderDebugMacro(
    "\n***************************************************\n"
    "Attempting to load " << file);
  if (this->PluginInfo->GetLoaded())
    {
    vtkPVPluginLoaderDebugMacro("Already loaded! Nothing to do.");
    return;
    }

  if (!file || file[0] == '\0')
    {
    vtkErrorMacro("Invalid filename");
    return;
    }

  this->PluginInfo->SetFileName(file);
  vtkstd::string defaultname = vtksys::SystemTools::GetFilenameWithoutExtension(file);
  this->PluginInfo->SetPluginName(defaultname.c_str());
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(file);
  if (!lib)
    {
    vtkPVPluginLoaderDebugMacro("Failed to load the shared library.");
    this->PluginInfo->SetError(vtkDynamicLoader::LastError());
    vtkPVPluginLoaderDebugMacro(this->PluginInfo->GetError());
    return;
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
    this->PluginInfo->SetError(
      "Not a ParaView Plugin since could not locate the plugin-verification function");
    //vtkErrorMacro(
    //  "Not a ParaView Plugin since could not locate the plugin-verification "
    //  "function");
    vtkDynamicLoader::CloseLibrary(lib);
    return;
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
    vtkErrorMacro(<< error.str().c_str());
    this->PluginInfo->SetError(error.str().c_str());
    vtkDynamicLoader::CloseLibrary(lib);
    vtkPVPluginLoaderDebugMacro(
      "Mismatch in versions signifies that the plugin was built for "
      "a different version of ParaView or with a different compilter. "
      "Look at the signatures to determine what caused the mismatch.");
    return;
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
    this->PluginInfo->SetError(
      "Not a ParaView Plugin since could not locate the plugin-instance "
      "function.");
    vtkErrorMacro(
      "Not a ParaView Plugin since could not locate the plugin-instance "
      "function.");
    vtkDynamicLoader::CloseLibrary(lib);
    return;
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
  this->Load(plugin);
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::Load(vtkPVPlugin* plugin)
{
  if (!plugin)
    {
    vtkErrorMacro("Cannot load NULL plugin.");
    this->PluginInfo->SetError("Cannot load NULL plugin");
    return;
    }

  vtkPVPluginLoaderDebugMacro("Plugin instance located successfully. "
    "Now loading components from the plugin instance based on the interfaces it "
    "implements.");

  // Populate some basic plugin information.
  this->PluginInfo->SetPluginName(plugin->GetPluginName());
  this->PluginInfo->SetPluginVersion(plugin->GetPluginVersionString());
  this->PluginInfo->SetLoaded(1);

  // plugin RequiredOnServer flag
  bool serverRequired = plugin->GetRequiredOnServer();
  bool clientRequired = plugin->GetRequiredOnClient();
  this->PluginInfo->SetRequiredOnServer(serverRequired? 1 : 0);
  this->PluginInfo->SetRequiredOnClient(clientRequired? 1 : 0);

  // plugin required-plugins
  this->PluginInfo->SetRequiredPlugins(plugin->GetRequiredPlugins());


  vtkPVPluginLoaderDebugMacro(
    "----------------------------------------------------------------\n"
    "Plugin Information: \n"
    "  Name        : " << this->PluginInfo->GetPluginName() << "\n"
    "  Version     : " << this->PluginInfo->GetPluginVersion() << "\n"
    "  ReqOnServer : " << this->PluginInfo->GetRequiredOnServer() << "\n"
    "  ReqOnClient : " << this->PluginInfo->GetRequiredOnClient() << "\n"
    "  ReqPlugins  : " << this->PluginInfo->GetRequiredPlugins());

  // Now, if this is a server manager plugin, get the xmls.
  vtkPVServerManagerPluginInterface* smplugin =
    dynamic_cast<vtkPVServerManagerPluginInterface*>(plugin);
  if (smplugin)
    {
    vtkstd::vector<vtkstd::string> xmls;
    smplugin->GetXMLs(xmls);

    this->ServerManagerXML->SetNumberOfTuples(static_cast<int>(xmls.size()));
    for (int i=0; i<static_cast<int>(xmls.size()); i++)
      {
      this->ServerManagerXML->SetValue(i, xmls[i]);
      }

    if (smplugin->GetInitializeInterpreterCallback())
      {
      vtkProcessModule::InitializeInterpreter(
        smplugin->GetInitializeInterpreterCallback());
      }
    vtkPVPluginLoaderDebugMacro(
      "  ServerManager Plugin : Yes");
    }
  else
    {
    vtkPVPluginLoaderDebugMacro(
      "  ServerManager Plugin : No");
    }

  // Now, if this is a python-module plugin, get the python source list.
  vtkPVPythonPluginInterface *pyplugin =
    dynamic_cast<vtkPVPythonPluginInterface*>(plugin);
  if (pyplugin)
    {
    vtkstd::vector<vtkstd::string> names;
    vtkstd::vector<vtkstd::string> sources;
    vtkstd::vector<int> package_flags;
    pyplugin->GetPythonSourceList(names, sources, package_flags);
    this->PythonModuleNames->SetNumberOfTuples(static_cast<int>(names.size()));
    this->PythonModuleSources->SetNumberOfTuples(static_cast<int>(sources.size()));
    this->PythonPackageFlags->SetNumberOfTuples(static_cast<int>(package_flags.size()));
    for (int cc=0; cc < static_cast<int>(names.size()); cc++)
      {
      this->PythonModuleNames->SetValue(cc, names[cc]);
      this->PythonModuleSources->SetValue(cc, sources[cc]);
      this->PythonPackageFlags->SetValue(cc, package_flags[cc]);
      }
    vtkPVPluginLoaderDebugMacro("  Python Plugin : Yes");
    }
  else
    {
    vtkPVPluginLoaderDebugMacro("  Python Plugin : No");
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
const char* vtkPVPluginLoader::GetFileName()
{
  return this->GetPluginInfo()->GetFileName();
}

//-----------------------------------------------------------------------------
const char* vtkPVPluginLoader::GetPluginName()
{
  return this->GetPluginInfo()->GetPluginName();
}

//-----------------------------------------------------------------------------
const char* vtkPVPluginLoader::GetPluginVersion()
{
  return this->GetPluginInfo()->GetPluginVersion();
}

//-----------------------------------------------------------------------------
int vtkPVPluginLoader::GetLoaded()
{
  return this->GetPluginInfo()->GetLoaded();
}

//-----------------------------------------------------------------------------
const char* vtkPVPluginLoader::GetError()
{
  return this->GetPluginInfo()->GetError();
}

//-----------------------------------------------------------------------------
const char* vtkPVPluginLoader::GetSearchPaths()
{
  return this->GetPluginInfo()->GetSearchPaths();
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

