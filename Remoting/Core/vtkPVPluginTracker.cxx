/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginTracker.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginTracker.h"

#include "vtkClientServerInterpreterInitializer.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPResourceFileLocator.h"
#include "vtkPSystemTools.h"
#include "vtkPVConfig.h"
#include "vtkPVLogger.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPythonModule.h"
#include "vtkPVPythonPluginInterface.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkVersion.h"

#include "vtksys/FStream.hxx"
#include "vtksys/String.hxx"
#include "vtksys/SystemTools.hxx"

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(__CYGWIN__)
/* String comparison routine. */
#define VTKSTRNCASECMP _strnicmp
#else
#include "strings.h"
#define VTKSTRNCASECMP strncasecmp
#endif

namespace
{

class vtkItem
{
public:
  std::string FileName;
  std::string PluginName;
  vtkPVPlugin* Plugin;
  bool AutoLoad;
  vtkItem()
  {
    this->Plugin = nullptr;
    this->AutoLoad = false;
  }
};

/**
 * Convert a plugin name to its library name i.e. add platform specific
 * library prefix and suffix.
 */
std::string vtkGetPluginFileNameFromName(const std::string& pluginname)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  return pluginname + ".dll";
#else
  // starting with ParaView 5.7, we are building .so's even on macOS
  // since they are built as "add_library(.. MODULE)" which by default generates
  // `.so`s which seems to be the convention.
  return pluginname + ".so";
#endif
}

using VectorOfSearchFunctions = std::vector<vtkPluginSearchFunction>;
static VectorOfSearchFunctions RegisteredPluginSearchFunctions;

using VectorOfListFunctions = std::vector<vtkPluginListFunction>;
static VectorOfListFunctions RegisteredPluginListFunctions;

std::vector<std::string> tokenize(const std::string& input, char delimiter)
{
  std::vector<std::string> tokens;
  std::stringstream ss(input);
  std::string item;
  while (std::getline(ss, item, delimiter))
  {
    tokens.push_back(std::move(item));
  }
  return tokens;
}

/**
 * Locate a plugin library or a config file anchored at standard locations
 * for locating plugins.
 */
std::string vtkLocatePluginOrConfigFile(const char* plugin, const char* hint, bool isPlugin)
{
  vtkVLogScopeF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "looking for plugin '%s'", plugin);

  auto pm = vtkProcessModule::GetProcessModule();
  // Make sure we can get the options before going further
  if (pm == nullptr)
  {
    vtkLogF(ERROR, "vtkProcessModule does not exist!");
    return std::string();
  }

  // First search in the static lookup tables.
  if (isPlugin)
  {
    for (auto searchFunction : RegisteredPluginSearchFunctions)
    {
      if (searchFunction && searchFunction(plugin))
      {
        vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "found plugin linked in statically!");
        return plugin;
      }
    }

#if !BUILD_SHARED_LIBS
    // in static builds, we don't attempt to locate plugin so's/dll's since we
    // can't load them anyways. This has a design flaw that we are also skipping *.py
    // or *.xml plugins. Since this has been the case historically, we'll leave
    // that as is for now. When we refactor the plugin infrastructure we should
    // fix that.
    return std::string();
#endif
  }

  const std::string exe_dir = pm->GetSelfDir();

  std::vector<std::string> prefixes = {
    std::string(PARAVIEW_RELATIVE_LIBPATH "/" PARAVIEW_SUBDIR "/") + plugin,
    std::string(PARAVIEW_RELATIVE_LIBPATH "/" PARAVIEW_SUBDIR "/"),

// .app bundles
#if defined(__APPLE__)
    std::string("../Plugins/") + plugin,
    std::string("../Plugins/"),
#endif
    std::string()
  };

  const std::string landmark = isPlugin ? vtkGetPluginFileNameFromName(plugin) : plugin;

  vtkNew<vtkPResourceFileLocator> locator;
  locator->SetLogVerbosity(PARAVIEW_LOG_PLUGIN_VERBOSITY());

  if (hint && *hint)
  {
    const std::string hintdir = vtksys::SystemTools::GetFilenamePath(hint);
    auto path = locator->Locate(hintdir + "/" + plugin, landmark);
    if (!path.empty())
    {
      return path + "/" + landmark;
    }
  }

  // First try the test plugin path, if it exists.
  vtkPVOptions* options = pm->GetOptions();
  if (options && options->GetTestPluginPaths() && strlen(options->GetTestPluginPaths()) > 0)
  {
    std::vector<std::string> testPluginPaths = tokenize(options->GetTestPluginPaths(), ',');
    for (const auto& testPluginPath : testPluginPaths)
    {
      vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "check `test-plugin-path` first.");
      auto path = locator->Locate(testPluginPath, landmark);
      if (!path.empty())
      {
        return path + "/" + landmark;
      }
    }
  }

  if (!exe_dir.empty())
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "check various prefixes relative to executable location: `%s`", exe_dir.c_str());
    auto pluginpath = locator->Locate(exe_dir, prefixes, landmark);
    if (!pluginpath.empty())
    {
      return pluginpath + "/" + landmark;
    }
  }

  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "failed!!!");
  return std::string();
}

/**
 * Converts a filename for a plugin to it's name i.e. removes the library
 * prefix and suffix, if any.
 */
std::string vtkGetPluginNameFromFileName(const std::string& filename)
{
  std::string defaultname = vtksys::SystemTools::GetFilenameWithoutExtension(filename);
  if (defaultname.size() > 3 && VTKSTRNCASECMP(defaultname.c_str(), "lib", 3) == 0)
  {
    defaultname.erase(0, 3);
  }
  return defaultname;
}
}

class vtkPVPluginTracker::vtkPluginsList : public std::vector<vtkItem>
{
public:
  iterator LocateUsingPluginName(const char* pluginname)
  {
    for (iterator iter = this->begin(); iter != this->end(); ++iter)
    {
      if (iter->PluginName == pluginname)
      {
        return iter;
      }
    }
    return this->end();
  }

  iterator LocateUsingFileName(const char* filename)
  {
    for (iterator iter = this->begin(); iter != this->end(); ++iter)
    {
      if (iter->FileName == filename)
      {
        return iter;
      }
    }
    return this->end();
  }
};

vtkStandardNewMacro(vtkPVPluginTracker);
//----------------------------------------------------------------------------
vtkPVPluginTracker::vtkPVPluginTracker()
{
  this->PluginsList = new vtkPluginsList();
  if (vtksys::SystemTools::GetEnv("PV_PLUGIN_DEBUG") != nullptr)
  {
    vtkWarningMacro("`PV_PLUGIN_DEBUG` environment variable has been deprecated. "
                    "Please use `PARAVIEW_LOG_PLUGIN_VERBOSITY=INFO` instead.");
    vtkPVLogger::SetPluginVerbosity(vtkLogger::VERBOSITY_INFO);
  }
}

//----------------------------------------------------------------------------
vtkPVPluginTracker::~vtkPVPluginTracker()
{
  delete this->PluginsList;
  this->PluginsList = nullptr;
}

//----------------------------------------------------------------------------
vtkPVPluginTracker* vtkPVPluginTracker::GetInstance()
{
  static vtkSmartPointer<vtkPVPluginTracker> Instance;
  if (Instance.GetPointer() == nullptr)
  {
    vtkPVPluginTracker* mgr = vtkPVPluginTracker::New();
    Instance = mgr;
    mgr->FastDelete();
  }
  return Instance;
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXMLs(const char* appname)
{
  if (!appname || !*appname)
  {
    return;
  }

  auto pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    return;
  }

#if BUILD_SHARED_LIBS
  // it makes sense to look for plugin configuration xmls and such only for
  // shared-builds. for static builds, it's not only not necessary, but also not
  // correct as we're missing all statically linked plugins that are available.
  const std::string exe_dir = pm->GetSelfDir();
  if (!exe_dir.empty())
  {
#if defined(__APPLE__)
    // Try it as a bundle.
    {
      auto conf = exe_dir + "/../Resources/" + appname + ".conf";
      if (vtksys::SystemTools::FileExists(conf))
      {
        this->LoadPluginConfigurationXMLConf(exe_dir, conf);
        return;
      }
    }
#endif // defined(__APPLE__)
    // Load it from beside the executable.
    {
      auto conf = exe_dir + "/" + appname + ".conf";
      if (vtksys::SystemTools::FileExists(conf))
      {
        this->LoadPluginConfigurationXMLConf(exe_dir, conf);
        return;
      }
    }
  }
#endif // BUILD_SHARED_LIBS

  std::vector<std::string> names;
  for (auto& listFunction : RegisteredPluginListFunctions)
  {
    if (listFunction)
    {
      listFunction(appname, names);
    }
  }

  vtkNew<vtkPVXMLElement> root;
  root->SetName("Plugins");
  for (const auto& name : names)
  {
    vtkNew<vtkPVXMLElement> child;
    child->SetName("Plugin");
    child->AddAttribute("name", name.c_str());
    root->AddNestedElement(child);
  }
  this->LoadPluginConfigurationXML(root, /*forceLoad=*/false);
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXMLConf(
  std::string const& exe_dir, std::string const& conf)
{
  vtksys::ifstream fin(conf.c_str());
  std::string line;
  // TODO: Replace with a JSON parser.
  while (std::getline(fin, line))
  {
    if (!vtksys::SystemTools::FileIsFullPath(line))
    {
      line = exe_dir + "/" + line;
    }

    this->LoadPluginConfigurationXML(line.c_str(), false);
  }
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXML(const char* filename, bool forceLoad)
{
  if (!vtkPSystemTools::FileExists(filename, true))
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Loading plugin configuration xml `%s` -- failed, not found!", filename);
    return;
  }

  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(filename);
  parser->SuppressErrorMessagesOn();
  if (!parser->Parse())
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Loading plugin configuration xml `%s` -- failed, invalid XML!", filename);
    return;
  }

  vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Loading plugin configuration xml `%s`.", filename);
  this->LoadPluginConfigurationXMLHinted(parser->GetRootElement(), filename, forceLoad);
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXMLFromString(
  const char* xmlcontents, bool forceLoad)
{
  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SuppressErrorMessagesOn();
  if (!parser->Parse(xmlcontents))
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "string not a valid xml -- failed!");
    return;
  }

  this->LoadPluginConfigurationXML(parser->GetRootElement(), forceLoad);
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXML(vtkPVXMLElement* root, bool forceLoad)
{
  this->LoadPluginConfigurationXMLHinted(root, nullptr, forceLoad);
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::LoadPluginConfigurationXMLHinted(
  vtkPVXMLElement* root, char const* hint, bool forceLoad)
{
  if (root == nullptr)
  {
    return;
  }

  if (strcmp(root->GetName(), "Plugins") != 0)
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "Root element in the xml must be `<Plugins/>`, got `%s` -- failed!", root->GetName());
    return;
  }

  for (unsigned int cc = 0; cc < root->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Plugin") == 0)
    {
      std::string name = child->GetAttributeOrEmpty("name");
      int auto_load = 0;
      child->GetScalarAttribute("auto_load", &auto_load);
      if (name.empty())
      {
        vtkVLogF(
          PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Missing required attribute name. Skipping element.");
        continue;
      }
      vtkVLogF(
        PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Trying to locate plugin with name `%s`", name.c_str());
      std::string plugin_filename;
      if (child->GetAttribute("filename"))
      {
        std::string filename = child->GetAttribute("filename");
        if (hint && !vtksys::SystemTools::FileIsFullPath(filename))
        {
          std::string basedir = vtksys::SystemTools::GetFilenamePath(hint);
          plugin_filename = vtksys::SystemTools::CollapseFullPath(filename, basedir);

          // Ensure the path is under the base directory given.
          if (!vtksys::SystemTools::IsSubDirectory(plugin_filename, basedir))
          {
            vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
              "Invalid `filename=` setting for %s; must be underneath the XML directory.",
              name.c_str());
            continue;
          }
        }
        else
        {
          plugin_filename = filename;
        }

        if (!vtkPSystemTools::FileExists(plugin_filename, true))
        {
          vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
            "Invalid `filename=` setting for %s; the given path (%s) must exist.", name.c_str(),
            plugin_filename.c_str());
          // TODO: Warn that the plugin was not found.
          plugin_filename.clear();
        }
      }
      else
      {
        plugin_filename = vtkLocatePluginOrConfigFile(name.c_str(), hint, true);
      }
      if (plugin_filename.empty())
      {
        int required = 0;
        child->GetScalarAttribute("required", &required);
        if (required)
        {
          vtkErrorMacro("Failed to locate required plugin: "
            << name << "\nApplication may not work exactly as expected.");
        }
        vtkVLogF(
          PARAVIEW_LOG_PLUGIN_VERBOSITY(), "Failed to locate file plugin `%s`", name.c_str());
        continue;
      }
      vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "found `%s`", plugin_filename.c_str());
      unsigned int index = this->RegisterAvailablePlugin(plugin_filename.c_str());
      if ((auto_load || forceLoad) && !this->GetPluginLoaded(index))
      {
        // load the plugin.
        vtkPVPluginLoader* loader = vtkPVPluginLoader::New();
        loader->LoadPlugin(plugin_filename.c_str());
        loader->Delete();
      }
      (*this->PluginsList)[index].AutoLoad = (auto_load != 0);
    }
  }
}

//----------------------------------------------------------------------------
unsigned int vtkPVPluginTracker::GetNumberOfPlugins()
{
  return static_cast<unsigned int>(this->PluginsList->size());
}

//----------------------------------------------------------------------------
unsigned int vtkPVPluginTracker::RegisterAvailablePlugin(const char* filename)
{
  std::string defaultname = vtkGetPluginNameFromFileName(filename);
  vtkPluginsList::iterator iter = this->PluginsList->LocateUsingFileName(filename);
  if (iter == this->PluginsList->end())
  {
    iter = this->PluginsList->LocateUsingPluginName(defaultname.c_str());
  }
  if (iter == this->PluginsList->end())
  {
    vtkItem item;
    item.FileName = filename;
    item.PluginName = defaultname;
    this->PluginsList->push_back(item);
    this->InvokeEvent(vtkPVPluginTracker::RegisterAvailablePluginEvent);
    return static_cast<unsigned int>(this->PluginsList->size() - 1);
  }
  else
  {
    // don't update the filename here. This avoids clobbering of paths for
    // distributed plugins between servers that are named the same (as far as
    // the client goes).
    // iter->FileName = filename;
    return static_cast<unsigned int>(iter - this->PluginsList->begin());
  }
}

//----------------------------------------------------------------------------
void vtkPVPluginTracker::RegisterPlugin(vtkPVPlugin* plugin)
{
  assert(plugin != nullptr);

  vtkPluginsList::iterator iter = this->PluginsList->LocateUsingPluginName(plugin->GetPluginName());
  if (iter == this->PluginsList->end())
  {
    vtkItem item;
    item.FileName = plugin->GetFileName() ? plugin->GetFileName() : "linked-in";
    item.PluginName = plugin->GetPluginName();
    item.Plugin = plugin;
    this->PluginsList->push_back(item);
  }
  else
  {
    iter->Plugin = plugin;
    if (plugin->GetFileName())
    {
      iter->FileName = plugin->GetFileName();
    }
  }

  // Do some basic processing of the plugin here itself.

  // If this plugin has functions for initializing the interpreter, we set them
  // up right now.
  vtkPVServerManagerPluginInterface* smplugin =
    dynamic_cast<vtkPVServerManagerPluginInterface*>(plugin);
  if (smplugin)
  {
    if (smplugin->GetInitializeInterpreterCallback())
    {
      // This also initializes any existing instances of
      // vtkClientServerInterpreter. Refer to
      // vtkClientServerInterpreterInitializer::RegisterCallback implementation
      // for details.
      vtkClientServerInterpreterInitializer::GetInitializer()->RegisterCallback(
        smplugin->GetInitializeInterpreterCallback());
    }
  }

  // If this plugin has Python modules, process those.
  vtkPVPythonPluginInterface* pythonplugin = dynamic_cast<vtkPVPythonPluginInterface*>(plugin);
  if (pythonplugin)
  {
    std::vector<std::string> modules, sources;
    std::vector<int> package_flags;
    pythonplugin->GetPythonSourceList(modules, sources, package_flags);
    assert(modules.size() == sources.size() && sources.size() == package_flags.size());
    for (size_t cc = 0; cc < modules.size(); cc++)
    {
      vtkPVPythonModule* module = vtkPVPythonModule::New();
      module->SetFullName(modules[cc].c_str());
      module->SetSource(sources[cc].c_str());
      module->SetIsPackage(package_flags[cc]);
      vtkPVPythonModule::RegisterModule(module);
      module->Delete();
    }
  }

  this->InvokeEvent(vtkCommand::RegisterEvent, plugin);
}

//----------------------------------------------------------------------------
vtkPVPlugin* vtkPVPluginTracker::GetPlugin(unsigned int index)
{
  if (index >= this->GetNumberOfPlugins())
  {
    vtkWarningMacro("Invalid index: " << index);
    return nullptr;
  }
  return (*this->PluginsList)[index].Plugin;
}

//----------------------------------------------------------------------------
const char* vtkPVPluginTracker::GetPluginName(unsigned int index)
{
  if (index >= this->GetNumberOfPlugins())
  {
    vtkWarningMacro("Invalid index: " << index);
    return nullptr;
  }
  return (*this->PluginsList)[index].PluginName.c_str();
}

//----------------------------------------------------------------------------
const char* vtkPVPluginTracker::GetPluginFileName(unsigned int index)
{
  if (index >= this->GetNumberOfPlugins())
  {
    vtkWarningMacro("Invalid index: " << index);
    return nullptr;
  }
  return (*this->PluginsList)[index].FileName.c_str();
}

//----------------------------------------------------------------------------
bool vtkPVPluginTracker::GetPluginLoaded(unsigned int index)
{
  if (index >= this->GetNumberOfPlugins())
  {
    vtkWarningMacro("Invalid index: " << index);
    return false;
  }
  return (*this->PluginsList)[index].Plugin != nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVPluginTracker::GetPluginAutoLoad(unsigned int index)
{
  if (index >= this->GetNumberOfPlugins())
  {
    vtkWarningMacro("Invalid index: " << index);
    return false;
  }
  return (*this->PluginsList)[index].AutoLoad;
}

//-----------------------------------------------------------------------------
void vtkPVPluginTracker::RegisterStaticPluginSearchFunction(vtkPluginSearchFunction function)
{
  RegisteredPluginSearchFunctions.push_back(function);
}

//-----------------------------------------------------------------------------
void vtkPVPluginTracker::RegisterStaticPluginListFunction(vtkPluginListFunction function)
{
  RegisteredPluginListFunctions.push_back(function);
}

#ifndef VTK_LEGACY_REMOVE
//-----------------------------------------------------------------------------
void vtkPVPluginTracker::SetStaticPluginSearchFunction(vtkPluginSearchFunction function)
{
  VTK_LEGACY_BODY(vtkPVPluginTracker::SetStaticPluginSearchFunction, "ParaView 5.7");
  vtkPVPluginTracker::RegisterStaticPluginSearchFunction(function);
}

#endif
