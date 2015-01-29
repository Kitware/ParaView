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
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkPVXMLParser.h"

#include <string>
#include <vtksys/SystemTools.hxx>
#include <vtksys/Directory.hxx>
#include <vtksys/ios/sstream>

#include <cstdlib>

#define vtkPVPluginLoaderDebugMacro(x)\
{ if (this->DebugPlugin) {\
  vtksys_ios::ostringstream vtkerror;\
  vtkerror << x;\
  vtkOutputWindowDisplayText(vtkerror.str().c_str());} }

#define vtkPVPluginLoaderErrorMacro(x)\
  if (!no_errors) {vtkErrorMacro(<< x);} this->SetErrorString(x);

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP=';';
#else
const char ENV_PATH_SEP=':';
#endif

namespace
{
  // This is an helper class used for plugins constructed from XMLs.
  class vtkPVXMLOnlyPlugin : public vtkPVPlugin,
                           public vtkPVServerManagerPluginInterface
  {
  std::string PluginName;
  std::string XML;
  vtkPVXMLOnlyPlugin(){};
  vtkPVXMLOnlyPlugin(const vtkPVXMLOnlyPlugin& other);
  void operator=(const vtkPVXMLOnlyPlugin& other);
public:
  static vtkPVXMLOnlyPlugin* Create(const char* xmlfile)
    {
    vtkNew<vtkPVXMLParser> parser;
    parser->SetFileName(xmlfile);
    if (!parser->Parse())
      {
      return NULL;
      }

    vtkPVXMLOnlyPlugin* instance = new vtkPVXMLOnlyPlugin();
    instance->PluginName  =
      vtksys::SystemTools::GetFilenameWithoutExtension(xmlfile);

    ifstream is;
    is.open(xmlfile, ios::binary);
    // get length of file:
    is.seekg (0, ios::end);
    size_t length = is.tellg();
    is.seekg (0, ios::beg);

    // allocate memory:
    char* buffer = new char [length + 1];

    // read data as a block:
    is.read (buffer,length);
    is.close();
    buffer[length] = 0;
    instance->XML = buffer;
    delete [] buffer;
    return instance;
    }

  // Description:
  // Returns the name for this plugin.
  virtual const char* GetPluginName()
    { return this->PluginName.c_str(); }

  // Description:
  // Returns the version for this plugin.
  virtual const char* GetPluginVersionString()
    { return "1.0"; }

  // Description:
  // Returns true if this plugin is required on the server.
  virtual bool GetRequiredOnServer()
    { return true; }

  // Description:
  // Returns true if this plugin is required on the client.
  virtual bool GetRequiredOnClient()
    { return false; }

  // Description:
  // Returns a ';' separated list of plugin names required by this plugin.
  virtual const char* GetRequiredPlugins()
    { return ""; }

  // Description:
  // Obtain the server-manager configuration xmls, if any.
  virtual void GetXMLs(std::vector<std::string>& xmls)
    {
    xmls.push_back(this->XML);
    }

  // Description:
  // Returns the callback function to call to initialize the interpretor for the
  // new vtk/server-manager classes added by this plugin. Returning NULL is
  // perfectly valid.
  virtual vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
    GetInitializeInterpreterCallback()
      { return NULL; }
  };

  // Cleans successfully opened libs when the application quits.
  // BUG # 10293
  class vtkPVPluginLoaderCleaner
    {
    std::vector<vtkLibHandle> Handles;
    std::vector<vtkPVXMLOnlyPlugin*> XMLPlugins;
  public:
    void Register(vtkLibHandle &handle)
      {
      this->Handles.push_back(handle);
      }
    void Register(vtkPVXMLOnlyPlugin* plugin)
      {
      this->XMLPlugins.push_back(plugin);
      }

    ~vtkPVPluginLoaderCleaner()
      {
      for (std::vector<vtkLibHandle>::iterator iter = this->Handles.begin();
        iter != this->Handles.end(); ++iter)
        {
        vtkDynamicLoader::CloseLibrary(*iter);
        }

      for (std::vector<vtkPVXMLOnlyPlugin*>::iterator iter =
        this->XMLPlugins.begin();
        iter != this->XMLPlugins.end(); ++iter)
        {
        delete *iter;
        }
      }
    static vtkPVPluginLoaderCleaner* GetInstance()
      {
      if (!vtkPVPluginLoaderCleaner::LibCleaner)
        {
        vtkPVPluginLoaderCleaner::LibCleaner = new vtkPVPluginLoaderCleaner();
        }
      return vtkPVPluginLoaderCleaner::LibCleaner;
      }
    static void FinalizeInstance()
      {
      if (vtkPVPluginLoaderCleaner::LibCleaner)
        {
        delete vtkPVPluginLoaderCleaner::LibCleaner;
        vtkPVPluginLoaderCleaner::LibCleaner = NULL;
        }
      }
  private:
    static vtkPVPluginLoaderCleaner *LibCleaner;
    };
  vtkPVPluginLoaderCleaner* vtkPVPluginLoaderCleaner::LibCleaner = NULL;
}


//=============================================================================
static int nifty_counter = 0;
vtkPVPluginLoaderCleanerInitializer::vtkPVPluginLoaderCleanerInitializer()
{
  nifty_counter++;
}
vtkPVPluginLoaderCleanerInitializer::~vtkPVPluginLoaderCleanerInitializer()
{
  nifty_counter--;
  if (nifty_counter == 0)
    {
    vtkPVPluginLoaderCleaner::FinalizeInstance();
    }
}
//=============================================================================


vtkPluginLoadFunction vtkPVPluginLoader::StaticPluginLoadFunction = 0;

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
        paths += ENV_PATH_SEP;
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
void vtkPVPluginLoader::LoadPluginsFromPluginSearchPath()
{
#ifdef BUILD_SHARED_LIBS
  vtkPVPluginLoaderDebugMacro(
    "Loading Plugins from standard PLUGIN_PATHS \n"
    << this->SearchPaths);

  std::vector<std::string> paths;
  vtksys::SystemTools::Split(this->SearchPaths, paths, ENV_PATH_SEP);
  for (size_t cc = 0; cc < paths.size(); cc++)
    {
    std::vector<std::string> subpaths;
    vtksys::SystemTools::Split(paths[cc].c_str(), subpaths, ';');
    for (size_t scc = 0; scc < subpaths.size(); scc++)
      {
      this->LoadPluginsFromPath(subpaths[scc].c_str());
      }
    }
#else
  vtkPVPluginLoaderDebugMacro(
    "Static build. Skipping PLUGIN_PATHS.");
#endif
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginsFromPath(const char* path)
{
  vtkPVPluginLoaderDebugMacro("Loading plugins in Path: " << path);
  vtksys::Directory dir;
  if (dir.Load(path) == false)
    {
    vtkPVPluginLoaderDebugMacro("Invalid directory: " << path);
    return;
    }

  for (unsigned int cc=0; cc < dir.GetNumberOfFiles(); cc++)
    {
    std::string ext =
      vtksys::SystemTools::GetFilenameLastExtension(dir.GetFile(cc));
    if (ext == ".so" || ext == ".dll" || ext == ".xml" || ext == ".dylib" ||
      ext == ".xml" || ext == ".sl")
      {
      std::string file = dir.GetPath();
      file += "/";
      file += dir.GetFile(cc);
      this->LoadPluginSilently(file.c_str());
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPluginInternal(const char* file, bool no_errors)
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
  std::string defaultname = vtksys::SystemTools::GetFilenameWithoutExtension(file);
  this->SetPluginName(defaultname.c_str());

  if (vtksys::SystemTools::GetFilenameLastExtension(file) == ".xml")
    {
    vtkPVPluginLoaderDebugMacro("Loading XML plugin");
    vtkPVXMLOnlyPlugin* plugin = vtkPVXMLOnlyPlugin::Create(file);
    if (plugin)
      {
      vtkPVPluginLoaderCleaner::GetInstance()->Register(plugin);
      return this->LoadPlugin(file, plugin);
      }
    vtkPVPluginLoaderErrorMacro(
      "Failed to load XML plugin. Not a valid XML or file could not be read.");
    return false;
    }
#ifndef BUILD_SHARED_LIBS
  if (StaticPluginLoadFunction &&
      StaticPluginLoadFunction(file))
    {
    this->Loaded = true;
    return true;
    }
  vtkPVPluginLoaderErrorMacro(
    "Could not find the plugin statically linked in, and "
    "cannot load dynamic plugins  in static builds.");
  return false;
#else // ifndef BUILD_SHARED_LIBS
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(file);
  if (!lib)
    {
    vtkPVPluginLoaderErrorMacro(vtkDynamicLoader::LastError());
    vtkPVPluginLoaderDebugMacro("Failed to load the shared library.");
    vtkPVPluginLoaderDebugMacro(this->ErrorString);
    return false;
    }

  // So that the lib is closed when the application quits.
  // BUG #10293.
  vtkPVPluginLoaderCleaner::GetInstance()->Register(lib);

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

  std::string pv_verfication_data = pv_plugin_query_verification_data();

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
  std::string ldLibPath;
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
  std::string thisPluginsPath(file);
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
  return this->LoadPlugin(file, plugin);
#endif // ifndef BUILD_SHARED_LIBS else
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPlugin(const char* file, vtkPVPlugin* plugin)
{
#ifndef BUILD_SHARED_LIBS
  if (StaticPluginLoadFunction &&
      StaticPluginLoadFunction(plugin->GetPluginName()))
    {
    this->Loaded = true;
    return true;
    }
  else
    {
    this->SetErrorString("Failed to load static plugin.");
    }
#endif

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

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::SetStaticPluginLoadFunction(vtkPluginLoadFunction function)
{
  if (!StaticPluginLoadFunction)
    {
    StaticPluginLoadFunction = function;
    }
}
