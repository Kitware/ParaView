/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginManager.h"

#include "vtkCommand.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginInformation.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPythonModule.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMPluginProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLParser.h"
#include "vtkStringArray.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>

#define vtkPVPluginLoaderDebugMacro(x)\
{ if (debug_plugin) {\
  vtksys_ios::ostringstream vtkerror;\
  vtkerror << x;\
  vtkOutputWindowDisplayText(vtkerror.str().c_str());} }

class vtkSMPluginManager::vtkSMPluginManagerInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkPVPluginInformation> >
    VectorOfPluginInformation;
  typedef vtkstd::map<vtkstd::string, VectorOfPluginInformation> ServerPluginsMap;
  typedef vtkstd::map<vtkstd::string, vtkstd::string>ServerSearchPathsMap;
  ServerPluginsMap Server2PluginsMap;
  ServerSearchPathsMap Server2SearchPathsMap;
  vtkstd::set<vtkstd::string> LoadedServerManagerXMLs;
};

//*****************************************************************************
static void vtkSMPluginManagerImportPlugin(vtkPVPlugin* plugin, void* calldata)
{
  vtkPVPluginLoader* loader = vtkPVPluginLoader::New();
  loader->Load(plugin);

  vtkSMPluginManager* mgr = reinterpret_cast<vtkSMPluginManager*>(calldata);
  mgr->ProcessPluginInfo(loader);
  mgr->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked,
    loader->GetPluginInfo());
  loader->Delete();
}

//*****************************************************************************
vtkStandardNewMacro(vtkSMPluginManager);
vtkCxxRevisionMacro(vtkSMPluginManager, "1.11");
//---------------------------------------------------------------------------
vtkSMPluginManager::vtkSMPluginManager()
{
  this->Internal = new vtkSMPluginManagerInternals();
  vtkPVPlugin::RegisterPluginManagerCallback(vtkSMPluginManagerImportPlugin,
    this);
}

//---------------------------------------------------------------------------
vtkSMPluginManager::~vtkSMPluginManager()
{ 
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMPluginManager::LoadPluginConfigurationXML(const char* filename)
{
  bool debug_plugin = vtksys::SystemTools::GetEnv("PV_PLUGIN_DEBUG") != NULL;
  vtkPVPluginLoaderDebugMacro("Loading plugin configuration xml: " << filename);
  if (!vtksys::SystemTools::FileExists(filename, true))
    {
    vtkPVPluginLoaderDebugMacro("Failed to located configuration xml. "
      "Could not populate the list of plugins distributed with application.");
    return;
    }

  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(filename);
  parser->SuppressErrorMessagesOn();
  if (!parser->Parse())
    {
    vtkPVPluginLoaderDebugMacro("Configuration file not a valid xml.");
    return;
    }

  vtkPVXMLElement* root = parser->GetRootElement();
  if (strcmp(root->GetName(), "Plugins") != 0)
    {
    vtkPVPluginLoaderDebugMacro("Root element in the xml must be <Plugins/>. "
      "Got " << root->GetName());
    return;
    }

  for (unsigned int cc=0; cc < root->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Plugin") == 0)
      {
      const char* name=child->GetAttribute("name");
      int auto_load;
      if (!name || !child->GetScalarAttribute("auto_load", &auto_load))
        {
        vtkPVPluginLoaderDebugMacro(
          "Missing required attribute name or auto_load. Skipping element.");
        continue;
        }
      vtkPVPluginLoaderDebugMacro("Trying to locate plugin with name: "
        << name);
      vtkstd::string plugin_filename = this->LocatePlugin(name);
      if (plugin_filename == "")
        {
        int required = 0;
        child->GetScalarAttribute("required", &required);
        if (required)
          {
          vtkErrorMacro(
            "Failed to locate required plugin: " << name << "\n"
            "Application may not work exactly as expected.");
          }
        vtkPVPluginLoaderDebugMacro("Failed to locate file plugin: "
          << name);
        continue;
        }
      vtkPVPluginLoaderDebugMacro("--- Found " << plugin_filename);
      vtkPVPluginInformation* info = vtkPVPluginInformation::New();
      info->SetPluginName(name);
      info->SetFileName(plugin_filename.c_str());
      info->SetLoaded(0);
      info->SetAutoLoad(auto_load);
      info->SetServerURI("builtin:");
      this->UpdatePluginMap("builtin:", info);
      this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, info);
      info->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkSMPluginManager::LocatePlugin(const char* plugin)
{
  bool debug_plugin = vtksys::SystemTools::GetEnv("PV_PLUGIN_DEBUG") != NULL;
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  vtkstd::string app_dir = options->GetApplicationPath();
  app_dir = vtksys::SystemTools::GetProgramPath(app_dir.c_str());


  vtkstd::vector<vtkstd::string> paths_to_search;
  paths_to_search.push_back(app_dir);
  paths_to_search.push_back(app_dir + "/plugins/" + plugin);
#if defined(__APPLE__)
  paths_to_search.push_back(app_dir + "/../Plugins");
  paths_to_search.push_back(app_dir + "/../../..");
#endif

  vtkstd::string name = plugin;
  vtkstd::string filename;
#if defined(_WIN32) && !defined(__CYGWIN__)
  filename = name + ".dll";
#elif defined(__APPLE__)
  filename = "lib" + name + ".dylib";
#else
  filename = "lib" + name + ".so";
#endif
  for (size_t cc=0; cc < paths_to_search.size(); cc++)
    {
    vtkstd::string path = paths_to_search[cc];
    if (vtksys::SystemTools::FileExists(
        (path + "/" + filename).c_str(), true))
      {
      return (path + "/" + filename);
      }
    vtkPVPluginLoaderDebugMacro(
      (path + "/" + filename).c_str() << "-- not found");
    }
  return vtkStdString();
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* vtkSMPluginManager::LoadLocalPlugin(const char* filename)
{
  if(!filename || !(*filename))
    {
    return NULL;
    }

  const char* serverURI = "builtin:";
  vtkPVPluginInformation* pluginInfo = this->FindPluginByFileName(
    serverURI, filename);
  if(pluginInfo && pluginInfo->GetLoaded())
    {
    this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, pluginInfo);
    return pluginInfo;
    }
    
  vtkSmartPointer<vtkPVPluginLoader> loader = vtkSmartPointer<vtkPVPluginLoader>::New();
  loader->SetFileName(filename);
  pluginInfo = loader->GetPluginInfo();
  vtkPVPluginInformation* localInfo = vtkPVPluginInformation::New();
  localInfo->DeepCopy(pluginInfo);
  localInfo->SetServerURI(serverURI);
  if(localInfo->GetLoaded())
    {
    this->ProcessPluginInfo(loader);
    }
  else if(!localInfo->GetError())
    {
    vtkstd::string loadError = filename;
    loadError.append(", is not a Paraview server manager plugin!");
    localInfo->SetError(loadError.c_str());
    }
    
  this->UpdatePluginMap(serverURI, localInfo);   
  this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, localInfo);
  localInfo->Delete();

  return localInfo;
}

//-----------------------------------------------------------------------------
vtkPVPluginInformation* vtkSMPluginManager::LoadPlugin(
  const char* filename, vtkIdType connectionId, const char* serverURI,
  bool loadRemote)
{ 
  if(!serverURI || !(*serverURI) || !filename || !(*filename))
    {
    return NULL;
    }
    
  vtkPVPluginInformation* pluginInfo = this->FindPluginByFileName(
    serverURI, filename);
  if(pluginInfo && pluginInfo->GetLoaded())
    {
    this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, pluginInfo);
    return pluginInfo;
    }
    
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMPluginProxy* pxy = vtkSMPluginProxy::SafeDownCast(
    pxm->NewProxy("misc", "PluginLoader"));
  if(pxy && filename && filename[0] != '\0')
    {
    pxy->SetConnectionID(connectionId);
    // data & render servers
    if(loadRemote)
      {
      pxy->SetServers(vtkProcessModule::SERVERS);
      }
    else
      {
      pxy->SetServers(vtkProcessModule::CLIENT);
      }
    pluginInfo = pxy->Load(filename);
    vtkPVPluginInformation* localInfo = vtkPVPluginInformation::New();
    localInfo->DeepCopy(pluginInfo);
    localInfo->SetServerURI(serverURI);
    if(localInfo->GetLoaded())
      {
      this->ProcessPluginInfo(pxy);
      }
    else if(!localInfo->GetError())
      {
      vtkstd::string loadError = filename;
      loadError.append(", is not a Paraview server manager plugin!");
      localInfo->SetError(loadError.c_str());
      }
    this->UpdatePluginMap(serverURI, localInfo);
    localInfo->Delete();
    pxy->UnRegister(NULL);
    this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, localInfo);
    pluginInfo = localInfo;
    }

  return pluginInfo;
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::RemovePlugin(
  const char* serverURI, const char* filename)
{
  if(!serverURI || !(*serverURI) || !filename || !(*filename))
    {
    return;
    }

  vtkSMPluginManagerInternals::ServerPluginsMap::iterator it = 
     this->Internal->Server2PluginsMap.find(serverURI);
  if( it != this->Internal->Server2PluginsMap.end())
    {
    if(filename && *filename)
      {
      bool found = false;
      vtkSMPluginManagerInternals::VectorOfPluginInformation::iterator infoIt = 
        it->second.begin();
      for(; infoIt != it->second.end(); infoIt++)
        {
        if((*infoIt)->GetFileName() && !strcmp(filename, (*infoIt)->GetFileName()))
          {
          found = true;
          break;
          }
        }

      if(found)
        {
        it->second.erase(infoIt);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::UpdatePluginMap(
  const char* serverURI, vtkPVPluginInformation* localInfo)
{
  vtkSMPluginManagerInternals::ServerPluginsMap::iterator it = 
    this->Internal->Server2PluginsMap.find(serverURI);
  if(it != this->Internal->Server2PluginsMap.end())
    {
    it->second.push_back(localInfo);
    }
  else
    {
    this->Internal->Server2PluginsMap[serverURI].push_back(localInfo);
    } 
}

//---------------------------------------------------------------------------
const char* vtkSMPluginManager::GetPluginPath(
  vtkIdType connectionId, const char* serverURI)
{
  vtkSMPluginManagerInternals::ServerSearchPathsMap::iterator it = 
    this->Internal->Server2SearchPathsMap.find(serverURI);
  if(it != this->Internal->Server2SearchPathsMap.end())
    {
    return it->second.c_str();
    }
  else
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMProxy* pxy = pxm->NewProxy("misc", "PluginLoader");
    pxy->SetConnectionID(connectionId);
    pxy->UpdateVTKObjects();
    pxy->UpdatePropertyInformation();
    vtkSMStringVectorProperty* svp = NULL;
    if(pxy->GetProperty("SearchPaths"))
      {
      svp = vtkSMStringVectorProperty::SafeDownCast(
        pxy->GetProperty("SearchPaths"));
      if(svp)
        {
        this->Internal->Server2SearchPathsMap[serverURI] = svp->GetElement(0);
        }
      }
    pxy->UnRegister(NULL);
    return  svp ? this->Internal->Server2SearchPathsMap[serverURI].c_str() : NULL;
    } 
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::ProcessPluginInfo(vtkSMPluginProxy* pluginProxy)
{
  if(!pluginProxy)
    {
    return;
    }
  vtkstd::string loadedxml = pluginProxy->GetPluginInfo()->GetPluginName();
  if(this->Internal->LoadedServerManagerXMLs.find(loadedxml) != 
    this->Internal->LoadedServerManagerXMLs.end())
    {
    // already processed;
    return;
    }

  vtkStringArray *array = vtkStringArray::New();
  vtkSMPropertyHelper helper(pluginProxy, "ServerManagerXML");
  array->SetNumberOfTuples(helper.GetNumberOfElements());
  for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
    {
    array->SetValue(cc, helper.GetAsString(cc));
    }
  this->ProcessPluginSMXML(array);
  array->Delete();
  
  this->Internal->LoadedServerManagerXMLs.insert(loadedxml);  
  
#ifdef PARAVIEW_ENABLE_PYTHON
  vtkStringArray* modules = vtkStringArray::New();
  vtkSMPropertyHelper helperModules(pluginProxy, "PythonModuleNames");
  modules->SetNumberOfTuples(helperModules.GetNumberOfElements());
  for (unsigned int cc=0; cc < helperModules.GetNumberOfElements(); cc++)
    {
    modules->SetValue(cc, helperModules.GetAsString(cc));
    }

  vtkStringArray* sources = vtkStringArray::New();
  vtkSMPropertyHelper helperSources(pluginProxy, "PythonModuleSources");
  sources->SetNumberOfTuples(helperSources.GetNumberOfElements());
  for (unsigned int cc=0; cc < helperSources.GetNumberOfElements(); cc++)
    {
    sources->SetValue(cc, helperSources.GetAsString(cc));
    }

  vtkIntArray* flags = vtkIntArray::New();
  vtkSMPropertyHelper helperFlags(pluginProxy, "PythonPackageFlags");
  flags->SetNumberOfTuples(helperFlags.GetNumberOfElements());
  for (unsigned int cc=0; cc < helperFlags.GetNumberOfElements(); cc++)
    {
    flags->SetValue(cc, helperFlags.GetAsInt(cc));
    }

  this->ProcessPluginPythonInfo(modules, sources, flags);
  modules->Delete();
  sources->Delete();
  flags->Delete();
#endif //PARAVIEW_ENABLE_PYTHON
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::ProcessPluginInfo(vtkPVPluginLoader* pluginLoader)
  {
  if(!pluginLoader)
    {
    return;
    }
  vtkstd::string loadedxml = pluginLoader->GetPluginInfo()->GetPluginName();
  if(this->Internal->LoadedServerManagerXMLs.find(loadedxml) != 
    this->Internal->LoadedServerManagerXMLs.end())
    {
    // already processed;
    return;
    }
    
  this->ProcessPluginSMXML(pluginLoader->GetServerManagerXML());  

  this->Internal->LoadedServerManagerXMLs.insert(loadedxml);  

#ifdef PARAVIEW_ENABLE_PYTHON
  this->ProcessPluginPythonInfo(pluginLoader->GetPythonModuleNames(),
    pluginLoader->GetPythonModuleSources(),
    pluginLoader->GetPythonPackageFlags());
#endif //PARAVIEW_ENABLE_PYTHON
  }

//---------------------------------------------------------------------------
void vtkSMPluginManager::ProcessPluginSMXML(vtkStringArray* smXMLArray)
{
  if(!smXMLArray)
    {
    return;
    }
   
  for(vtkIdType i = 0; i < smXMLArray->GetNumberOfTuples(); i++)
    {
    vtkSMObject::GetProxyManager()->LoadConfigurationXML(smXMLArray->GetValue(i).c_str());
    }
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::ProcessPluginPythonInfo(vtkStringArray* pyNameArray,
  vtkStringArray* pySourceArray, vtkIntArray* pyFlagArray)
{
  if(!pyNameArray || !pySourceArray || !pyFlagArray)
    {
    return;
    }
  
  if(pyNameArray->GetNumberOfTuples() == pySourceArray->GetNumberOfTuples() 
  && pyNameArray->GetNumberOfTuples() == pyFlagArray->GetNumberOfTuples())
    {
    for (vtkIdType i = 0; i < pyNameArray->GetNumberOfTuples(); i++)
      {
      vtkSmartPointer<vtkPVPythonModule> module = vtkSmartPointer<vtkPVPythonModule>::New();
      module->SetFullName(pyNameArray->GetValue(i).c_str());
      module->SetSource(pySourceArray->GetValue(i).c_str());
      module->SetIsPackage(pyFlagArray->GetValue(i));
      vtkPVPythonModule::RegisterModule(module);
      }
    }
}

//---------------------------------------------------------------------------
vtkPVPluginInformation* vtkSMPluginManager::FindPluginByFileName(
  const char* serverURI, const char* filename)
{
  vtkSMPluginManagerInternals::ServerPluginsMap::iterator it = 
     this->Internal->Server2PluginsMap.find(serverURI);
  if( it != this->Internal->Server2PluginsMap.end())
    {
    if(filename && *filename)
      {
      for(int i=0; i<(int)(it->second.size()) ; i++)
        {
        if(it->second[i]->GetFileName() &&
        !strcmp(filename, it->second[i]->GetFileName()))
          {
          return it->second[i];
          }
        }
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::UpdatePluginLoadInfo(
  const char* filename, const char* serverURI, int loaded)
{
  vtkPVPluginInformation* pluginInfo = this->FindPluginByFileName(
    serverURI, filename);
  if(pluginInfo)
    {
    pluginInfo->SetLoaded(loaded);
    }  
}

//---------------------------------------------------------------------------
void vtkSMPluginManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
