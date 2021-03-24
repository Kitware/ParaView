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
#include "vtkPDirectory.h"
#include "vtkPVConfig.h"
#include "vtkPVLogger.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdlib>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define vtkPVPluginLoaderErrorMacro(x)                                                             \
  do                                                                                               \
  {                                                                                                \
    const char* errstring = x;                                                                     \
    if (!no_errors)                                                                                \
    {                                                                                              \
      vtkErrorMacro(<< errstring << endl);                                                         \
    }                                                                                              \
    this->SetErrorString(errstring);                                                               \
  } while (false)

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP = ';';
#else
const char ENV_PATH_SEP = ':';
#endif

namespace
{
// This is an helper class used for plugins constructed from XMLs.
class vtkPVXMLOnlyPlugin : public vtkPVPlugin, public vtkPVServerManagerPluginInterface
{
  std::string PluginName;
  std::string XML;
  vtkPVXMLOnlyPlugin() = default;
  vtkPVXMLOnlyPlugin(const vtkPVXMLOnlyPlugin& other);
  void operator=(const vtkPVXMLOnlyPlugin& other);

public:
  static vtkPVXMLOnlyPlugin* Create(const char* xmlfile)
  {
    vtkNew<vtkPVXMLParser> parser;
    parser->SetFileName(xmlfile);
    if (!parser->Parse())
    {
      return nullptr;
    }

    vtkPVXMLOnlyPlugin* instance = new vtkPVXMLOnlyPlugin();
    instance->PluginName = vtksys::SystemTools::GetFilenameWithoutExtension(xmlfile);

    vtksys::ifstream is;
    is.open(xmlfile, ios::binary);
    // get length of file:
    is.seekg(0, ios::end);
    size_t length = is.tellg();
    is.seekg(0, ios::beg);

    // allocate memory:
    char* buffer = new char[length + 1];

    // read data as a block:
    is.read(buffer, length);
    is.close();
    buffer[length] = 0;
    instance->XML = buffer;
    delete[] buffer;
    return instance;
  }

  /**
   * Returns the name for this plugin.
   */
  const char* GetPluginName() override { return this->PluginName.c_str(); }

  /**
   * Returns the version for this plugin.
   */
  const char* GetPluginVersionString() override { return "1.0"; }

  /**
   * Returns true if this plugin is required on the server.
   */
  bool GetRequiredOnServer() override { return true; }

  /**
   * Returns true if this plugin is required on the client.
   */
  bool GetRequiredOnClient() override { return false; }

  /**
   * Returns a ';' separated list of plugin names required by this plugin.
   */
  const char* GetRequiredPlugins() override { return ""; }

  /**
   * Returns a description of this plugin.
   */
  const char* GetDescription() override { return ""; }

  /**
   * Obtain the server-manager configuration xmls, if any.
   */
  void GetXMLs(std::vector<std::string>& xmls) override { xmls.push_back(this->XML); }

  /**
   * Returns the callback function to call to initialize the interpretor for the
   * new vtk/server-manager classes added by this plugin. Returning nullptr is
   * perfectly valid.
   */
  vtkClientServerInterpreterInitializer::InterpreterInitializationCallback
  GetInitializeInterpreterCallback() override
  {
    return nullptr;
  }

  /**
   * Returns EULA for the plugin, if any. If none, this will return nullptr.
   */
  const char* GetEULA() override { return nullptr; }
};

// Cleans successfully opened libs when the application quits.
// BUG # 10293
class vtkPVPluginLoaderCleaner
{
  typedef std::map<std::string, vtkLibHandle> HandlesType;
  HandlesType Handles;
  std::vector<vtkPVXMLOnlyPlugin*> XMLPlugins;

public:
  void Register(const char* pname, vtkLibHandle& handle) { this->Handles[pname] = handle; }
  void Register(vtkPVXMLOnlyPlugin* plugin) { this->XMLPlugins.push_back(plugin); }

  ~vtkPVPluginLoaderCleaner()
  {
    for (HandlesType::const_iterator iter = this->Handles.begin(); iter != this->Handles.end();
         ++iter)
    {
      vtkDynamicLoader::CloseLibrary(iter->second);
    }
    for (std::vector<vtkPVXMLOnlyPlugin*>::iterator iter = this->XMLPlugins.begin();
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
      vtkPVPluginLoaderCleaner* cleaner = vtkPVPluginLoaderCleaner::LibCleaner;
      vtkPVPluginLoaderCleaner::LibCleaner = nullptr;
      delete cleaner;
    }
  }
  static void PluginLibraryUnloaded(const char* pname)
  {
    if (vtkPVPluginLoaderCleaner::LibCleaner && pname)
    {
      vtkPVPluginLoaderCleaner::LibCleaner->Handles.erase(pname);
    }
  }

private:
  static vtkPVPluginLoaderCleaner* LibCleaner;
};
vtkPVPluginLoaderCleaner* vtkPVPluginLoaderCleaner::LibCleaner = nullptr;
};

//=============================================================================
using VectorOfCallbacks = std::vector<vtkPVPluginLoader::PluginLoaderCallback>;
// variables used by the Initializer should be only primitive types
// cannot be objects or their constructor will interfere with the Initializer
static VectorOfCallbacks* RegisteredPluginLoaderCallbacks = nullptr;
static int nifty_counter = 0;
vtkPVPluginLoaderCleanerInitializer::vtkPVPluginLoaderCleanerInitializer()
{
  if (nifty_counter++ == 0)
  {
    ::RegisteredPluginLoaderCallbacks = new VectorOfCallbacks();
  }
}

vtkPVPluginLoaderCleanerInitializer::~vtkPVPluginLoaderCleanerInitializer()
{
  if (--nifty_counter == 0)
  {
    vtkPVPluginLoaderCleaner::FinalizeInstance();
    delete ::RegisteredPluginLoaderCallbacks;
    ::RegisteredPluginLoaderCallbacks = nullptr;
  }
}

vtkStandardNewMacro(vtkPVPluginLoader);
//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->ErrorString = nullptr;
  this->PluginName = nullptr;
  this->PluginVersion = nullptr;
  this->FileName = nullptr;
  this->SearchPaths = nullptr;
  this->Loaded = false;
  this->SetErrorString("No plugin loaded yet.");

  std::string paths;
  const char* env = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if (env)
  {
    paths += env;
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "PV_PLUGIN_PATH: %s", env);
  }

#ifdef PARAVIEW_PLUGIN_LOADER_PATHS
  if (!paths.empty())
  {
    paths += ENV_PATH_SEP;
  }
  paths += PARAVIEW_PLUGIN_LOADER_PATHS;
  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "PARAVIEW_PLUGIN_LOADER_PATHS: %s",
    PARAVIEW_PLUGIN_LOADER_PATHS);
#endif

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* opt = pm ? pm->GetOptions() : nullptr;
  if (opt)
  {
    std::string appDir = vtkProcessModule::GetProcessModule()->GetSelfDir();
    if (appDir.size())
    {
      appDir += "/plugins";
      if (paths.size())
      {
        paths += ENV_PATH_SEP;
      }
      paths += appDir;
      vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "appDir: %s", appDir.c_str());
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
  this->SetErrorString(nullptr);
  this->SetPluginName(nullptr);
  this->SetPluginVersion(nullptr);
  this->SetFileName(nullptr);
  this->SetSearchPaths(nullptr);
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginsFromPluginSearchPath()
{
#if BUILD_SHARED_LIBS
  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Loading Plugins from standard PLUGIN_PATHS\n%s",
    (this->SearchPaths ? this->SearchPaths : "(nullptr)"));

  std::vector<std::string> paths;
  vtksys::SystemTools::Split(this->SearchPaths, paths, ENV_PATH_SEP);
  for (size_t cc = 0; cc < paths.size(); cc++)
  {
    std::vector<std::string> subpaths;
    vtksys::SystemTools::Split(paths[cc], subpaths, ';');
    for (size_t scc = 0; scc < subpaths.size(); scc++)
    {
      this->LoadPluginsFromPath(subpaths[scc].c_str());
    }
  }
#else
  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Static build. Skipping PLUGIN_PATHS.");
#endif
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginsFromPluginConfigFile()
{
#if BUILD_SHARED_LIBS
  const char* configFiles = vtksys::SystemTools::GetEnv("PV_PLUGIN_CONFIG_FILE");
  if (configFiles != nullptr)
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Loading Plugins from standard PV_PLUGIN_CONFIG_FILE: %s", configFiles);
    std::vector<std::string> paths;
    vtksys::SystemTools::Split(configFiles, paths, ENV_PATH_SEP);
    for (size_t cc = 0; cc < paths.size(); cc++)
    {
      std::vector<std::string> subpaths;
      vtksys::SystemTools::Split(paths[cc], subpaths, ';');
      for (size_t scc = 0; scc < subpaths.size(); scc++)
      {
        vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXML(subpaths[scc].c_str(), true);
      }
    }
  }
#else
  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Static build. Skipping PV_PLUGIN_CONFIG_FILE.");
#endif
}
//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginsFromPath(const char* path)
{
  vtkVLogIfF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), path != nullptr, "Loading plugins in Path: %s", path);

  vtkNew<vtkPDirectory> dir;
  if (dir->Load(path) == false)
  {
    vtkVLogIfF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), path != nullptr, "Invalid directory: %s", path);
    return;
  }

#ifdef _WIN32
  const char* compiled_extension = ".dll";
#else
  const char* compiled_extension = ".so";
#endif

  for (vtkIdType cc = 0; cc < dir->GetNumberOfFiles(); cc++)
  {
    const char* file = dir->GetFile(cc);
    std::string rel_path;
    bool has_valid_extension;
    bool assume_exists = false;

    // If we have a directory, search it for a plugin of the same name.
    if (dir->FileIsDirectory(file))
    {
      rel_path = file;
      rel_path += '/';
      rel_path += file;
      rel_path += compiled_extension;
      has_valid_extension = true;
    }
    else
    {
      // We have a file, check to see if its extension is acceptable.
      rel_path = file;
      std::string ext = vtksys::SystemTools::GetFilenameLastExtension(rel_path);
      has_valid_extension =
        (ext == compiled_extension || ext == ".xml" || ext == ".sl" || ext == ".py");
      assume_exists = true;
    }

    // No extension, not a plugin.
    if (!has_valid_extension)
    {
      continue;
    }

    // Calculate the full path to the plugin.
    std::string full_file = dir->GetPath();
    full_file += '/';
    full_file += rel_path;

    // Check if it exists and is a file.
    if (!assume_exists && !vtksys::SystemTools::FileExists(full_file, true))
    {
      continue;
    }

    // Load the plugin.
    this->LoadPluginSilently(full_file.c_str());
  }
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPluginByName(const char* name)
{
  if (vtkPVPluginLoader::CallPluginLoaderCallbacks(name))
  {
    return true;
  }

  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  unsigned int nplugins = tracker->GetNumberOfPlugins();

  for (unsigned int i = 0; i < nplugins; ++i)
  {
    const char* plugin_name = tracker->GetPluginName(i);
    if (!plugin_name)
    {
      continue;
    }

    if (!strcmp(name, plugin_name))
    {
      const char* filename = tracker->GetPluginFileName(i);
      if (!filename)
      {
        vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
          "Found the %s plugin, but no file associated with it?", name);
        return false;
      }

      return this->LoadPlugin(filename);
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPluginInternal(const char* file, bool no_errors)
{
  this->Loaded = false;
  if (!file || file[0] == '\0')
  {
    vtkPVPluginLoaderErrorMacro("Invalid filename");
    return false;
  }

  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Attempting to load: %s", file);

  this->SetFileName(file);
  std::string defaultname = vtksys::SystemTools::GetFilenameWithoutExtension(file);
  this->SetPluginName(defaultname.c_str());

  // Avoid duplicate loading of the same plugin
  {
    vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
    unsigned int nplugins = tracker->GetNumberOfPlugins();

    for (unsigned int i = 0; i < nplugins; ++i)
    {
      const char* filename = tracker->GetPluginFileName(i);
      if (!filename)
      {
        continue;
      }

      if (strcmp(file, filename))
      {
        continue;
      }

      bool already_loaded = tracker->GetPluginLoaded(i);
      if (already_loaded)
      {
        return true;
      }
      else
      {
        break;
      }
    }
  }

  // first, try the callbacks.
  if (vtkPVPluginLoader::CallPluginLoaderCallbacks(file))
  {
    this->Loaded = true;
    return true;
  }

  if (vtksys::SystemTools::GetFilenameLastExtension(file) == ".xml")
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Loading XML plugin.");
    vtkPVXMLOnlyPlugin* plugin = vtkPVXMLOnlyPlugin::Create(file);
    if (plugin)
    {
      vtkPVPluginLoaderCleaner::GetInstance()->Register(plugin);
      plugin->SetFileName(file);
      return this->LoadPluginInternal(plugin);
    }
    vtkPVPluginLoaderErrorMacro(
      "Failed to load XML plugin. Not a valid XML or file could not be read.");
    return false;
  }

#if !BUILD_SHARED_LIBS
  vtkPVPluginLoaderErrorMacro("Could not find the plugin statically linked in, and "
                              "cannot load dynamic plugins  in static builds.");
  return false;
#else // ifndef BUILD_SHARED_LIBS
  int flags = 0;
#ifdef _WIN32
  // Windows doesn't have rpath or other mechanisms for specifying where
  // dependent libraries live. Assume those not provided by ParaView live next
  // to the plugin.
  flags |= vtksys::DynamicLoader::SearchBesideLibrary;
#endif
  vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(file, flags);
  if (!lib)
  {
    std::stringstream ostr;
    ostr << file << ": " << vtkDynamicLoader::LastError();
    vtkPVPluginLoaderErrorMacro(ostr.str().c_str());
    vtkVLogIfF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), this->ErrorString != nullptr,
      "Failed to load the shared library.\n%s", this->ErrorString);
    return false;
  }

  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
    "Loaded shared library successfully. Now trying to validate that it's a ParaView plugin.");

  // A plugin shared library has two global functions:
  // * pv_plugin_instance -- to obtain the plugin instance.

  // If we succeeded so far, then obtain the instance of vtkPVPlugin for this
  // plugin and load it.

  pv_plugin_query_instance_fptr pv_plugin_query_instance =
    (pv_plugin_query_instance_fptr)(vtkDynamicLoader::GetSymbolAddress(lib, "pv_plugin_instance"));
  if (!pv_plugin_query_instance)
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "We've encountered an error locating the other "
      "global function \"pv_plugin_instance\" which is required to locate the "
      "instance of the vtkPVPlugin class. Possibly the plugin shared library was "
      "not compiled properly.");
    vtkPVPluginLoaderErrorMacro("Not a ParaView Plugin since could not locate the plugin-instance "
                                "function.");
    vtkDynamicLoader::CloseLibrary(lib);
    return false;
  }

  // BUG # 0008673
  // Tell the platform to look in the plugin's directory for
  // its dependencies. This isn't the right thing to do. A better
  // solution would be to let the plugin tell us where to look so
  // that a list of locations could be added.
  std::string ldLibPath;
#if defined(_WIN32) && !defined(__CYGWIN__)
  const char LIB_PATH_SEP = ';';
  const char PATH_SEP = '\\';
  const char* LIB_PATH_NAME = "PATH";
  ldLibPath = LIB_PATH_NAME;
  ldLibPath += '=';
#elif defined(__APPLE__)
  const char LIB_PATH_SEP = ':';
  const char PATH_SEP = '/';
  const char* LIB_PATH_NAME = "DYLD_LIBRARY_PATH";
#else
  const char LIB_PATH_SEP = ':';
  const char PATH_SEP = '/';
  const char* LIB_PATH_NAME = "LD_LIBRARY_PATH";
#endif
  // Trim the plugin name from the end of its path.
  std::string thisPluginsPath(file);
  size_t eop = thisPluginsPath.rfind(PATH_SEP);
  thisPluginsPath = thisPluginsPath.substr(0, eop);
  // Load the shared library search path.
  const char* pLdLibPath = vtksys::SystemTools::GetEnv(LIB_PATH_NAME);
  bool pluginPathPresent =
    pLdLibPath == nullptr ? false : strstr(pLdLibPath, thisPluginsPath.c_str()) != nullptr;
  // Update it.
  if (!pluginPathPresent)
  {
    // Make sure we are only adding it once, because there can
    // be multiple plugins in the same folder.
    if (pLdLibPath)
    {
      ldLibPath += pLdLibPath;
      ldLibPath += LIB_PATH_SEP;
    }
    ldLibPath += thisPluginsPath;

    vtksys::SystemTools::PutEnv(ldLibPath);
    vtkVLogF(
      PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Updating Shared Library Paths: %s", ldLibPath.c_str());
  }

  if (vtkPVPlugin* plugin = pv_plugin_query_instance())
  {
    plugin->SetFileName(file);
    //  if (plugin->UnloadOnExit())
    {
      // So that the lib is closed when the application quits.
      // BUGS #10293, #15608.
      vtkPVPluginLoaderCleaner::GetInstance()->Register(plugin->GetPluginName(), lib);
    }
    return this->LoadPluginInternal(plugin);
  }
#endif // ifndef BUILD_SHARED_LIBS else
  return false;
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::LoadPluginInternal(vtkPVPlugin* plugin)
{
  this->SetPluginName(plugin->GetPluginName());
  this->SetPluginVersion(plugin->GetPluginVersionString());

  // From this point onwards the vtkPVPlugin travels the same path as a
  // statically imported plugin.
  vtkPVPlugin::ImportPlugin(plugin);
  this->Loaded = true;
  return true;
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::LoadPluginConfigurationXMLFromString(const char* xmlcontents)
{
  vtkPVPluginTracker::GetInstance()->LoadPluginConfigurationXMLFromString(xmlcontents);
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PluginName: " << (this->PluginName ? this->PluginName : "(none)") << endl;
  os << indent << "PluginVersion: " << (this->PluginVersion ? this->PluginVersion : "(none)")
     << endl;
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "SearchPaths: " << (this->SearchPaths ? this->SearchPaths : "(none)") << endl;
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::PluginLibraryUnloaded(const char* pluginname)
{
  vtkPVPluginLoaderCleaner::PluginLibraryUnloaded(pluginname);
}

//-----------------------------------------------------------------------------
int vtkPVPluginLoader::RegisterLoadPluginCallback(PluginLoaderCallback callback)
{
  if (::RegisteredPluginLoaderCallbacks)
  {
    size_t index = ::RegisteredPluginLoaderCallbacks->size();
    ::RegisteredPluginLoaderCallbacks->push_back(callback);
    return static_cast<int>(index);
  }
  return -1;
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::UnregisterLoadPluginCallback(int index)
{
  if (::RegisteredPluginLoaderCallbacks != nullptr && index >= 0 &&
    index < static_cast<int>(::RegisteredPluginLoaderCallbacks->size()))
  {
    auto iter = ::RegisteredPluginLoaderCallbacks->begin();
    std::advance(iter, index);
    ::RegisteredPluginLoaderCallbacks->erase(iter);
  }
}

//-----------------------------------------------------------------------------
bool vtkPVPluginLoader::CallPluginLoaderCallbacks(const char* nameOrFile)
{
  if (::RegisteredPluginLoaderCallbacks)
  {
    for (auto iter = ::RegisteredPluginLoaderCallbacks->rbegin();
         iter != ::RegisteredPluginLoaderCallbacks->rend(); ++iter)
    {
      if ((*iter)(nameOrFile))
      {
        return true;
      }
    }
  }
  return false;
}
