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
#include "vtkPVEnvironmentInformation.h"
#include "vtkPVPluginLoader.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include <vtkstd/map>

#include "vtkPVPluginInformation.h"
#include "vtkPVPythonModule.h"
#include "vtkSMPluginProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLParser.h"
#include <vtkstd/set>

class vtkSMPluginManager::vtkSMPluginManagerInternals
{
public:
  vtkSMPluginManagerInternals(){}
  ~vtkSMPluginManagerInternals()
  {
    for(ServerPluginsMap::iterator it = this->Server2PluginsMap.begin();
      it != this->Server2PluginsMap.end(); it++)
      {
      for(int i=0; i<(int)(it->second.size()) ; i++)
        {
        if(it->second[i])
          {
          it->second[i]->Delete();
          }
        }
      }
  }
  typedef vtkstd::map<vtkstd::string, vtkstd::vector<vtkPVPluginInformation* > >ServerPluginsMap;
  typedef vtkstd::map<vtkstd::string, vtkstd::string>ServerSearchPathsMap;
  ServerPluginsMap Server2PluginsMap;
  ServerSearchPathsMap Server2SearchPathsMap;
  vtkstd::set<vtkstd::string> LoadedServerManagerXMLs;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSMPluginManager);
vtkCxxRevisionMacro(vtkSMPluginManager, "1.1");
//---------------------------------------------------------------------------
vtkSMPluginManager::vtkSMPluginManager()
{
  this->Internal = new vtkSMPluginManagerInternals();
}

//---------------------------------------------------------------------------
vtkSMPluginManager::~vtkSMPluginManager()
{ 
  delete this->Internal;
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
    pxy->UnRegister(NULL);
    this->InvokeEvent(vtkSMPluginManager::LoadPluginInvoked, localInfo);
    pluginInfo = localInfo;
    }

  return pluginInfo;
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
  this->ProcessPluginSMXML(pluginProxy->GetServerManagerXML());  
  
  this->Internal->LoadedServerManagerXMLs.insert(loadedxml);  
  
#ifdef VTK_WRAP_PYTHON
  this->ProcessPluginPythonInfo(pluginProxy->GetPythonModuleNames(),
                                pluginProxy->GetPythonModuleSources(),
                                pluginProxy->GetPythonPackageFlags());
#endif //VTK_WRAP_PYTHON
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
