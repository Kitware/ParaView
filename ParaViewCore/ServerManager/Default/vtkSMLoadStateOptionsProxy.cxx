/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLoadStateOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLoadStateOptionsProxy.h"

#include "vtkClientServerStream.h"
#include "vtkFileSequenceParser.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPVXMLParser.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtksys/SystemTools.hxx"
#include <set>
#include <sstream>
#include <vtk_pugixml.h>

//---------------------------------------------------------------------------
class vtkSMLoadStateOptionsProxy::vtkInternals
{
public:
  struct PropertyInfo
  {
    pugi::xml_node XMLElement;
    std::vector<std::string> FilePaths;
    bool Modified;
    PropertyInfo()
      : Modified(false)
    {
    }
  };
  typedef std::map<std::string, PropertyInfo> PropertiesMapType;
  PropertiesMapType PropertiesMap;
  std::map<int, pugi::xml_node> CollectionsMap;
  pugi::xml_document StateXML;

  void ProcessStateFile(pugi::xml_node node)
  {
    pugi::xpath_node_set proxies =
      node.select_nodes("//ServerManagerState/Proxy[@group and @type]");
    for (auto iter = proxies.begin(); iter != proxies.end(); ++iter)
    {
      this->ProcessProxy(iter->node());
    }
    pugi::xpath_node_set collections =
      node.select_nodes("//ServerManagerState/ProxyCollection[@name='sources']");
    for (auto iter = collections.begin(); iter != collections.end(); ++iter)
    {
      this->ProcessProxyCollection(iter->node());
    }
  }

  void ProcessProxy(pugi::xml_node node)
  {
    pugi::xml_attribute group = node.attribute("group");
    pugi::xml_attribute type = node.attribute("type");

    if (group.empty() || type.empty())
    {
      vtkGenericWarningMacro("Possibly invalid state file.");
      return;
    }

    vtkSMProxy* prototype =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetPrototypeProxy(
        group.value(), type.value());
    if (!prototype)
    {
      return;
    }

    std::set<std::string> filenameProperties = this->LocateFileNameProperties(prototype);
    if (filenameProperties.size() == 0)
    {
      return;
    }

    pugi::xpath_variable_set vars;
    vars.add("propertyname", pugi::xpath_type_string);
    for (auto iter = filenameProperties.begin(); iter != filenameProperties.end(); ++iter)
    {
      vars.set("propertyname", iter->c_str());
      pugi::xpath_node_set properties =
        node.select_nodes("./Property[@name = string($propertyname)]", &vars);
      for (auto pIter = properties.begin(); pIter != properties.end(); ++pIter)
      {
        PropertyInfo info;
        info.XMLElement = pIter->node();
        pugi::xpath_node_set elements = info.XMLElement.select_nodes("./Element");
        for (auto eIter = elements.begin(); eIter != elements.end(); ++eIter)
        {
          info.FilePaths.push_back(eIter->node().attribute("value").value());
        }
        this->PropertiesMap[pIter->node().attribute("id").value()] = info;
      }
    }
  }

  void ProcessProxyCollection(pugi::xml_node node)
  {
    pugi::xml_attribute name = node.attribute("name");
    if (name.empty())
    {
      vtkGenericWarningMacro(
        "Possibly invalid state file. Proxy Collection doesn't have a name attribute.");
      return;
    }

    pugi::xpath_node_set items = node.select_nodes("./Item");
    for (auto iter = items.begin(); iter != items.end(); ++iter)
    {
      int itemId = iter->node().attribute("id").as_int();
      this->CollectionsMap[itemId] = iter->node();
    }
  }

  std::set<std::string> LocateFileNameProperties(vtkSMProxy* proxy)
  {
    std::set<std::string> fileNameProperties;
    vtkSMPropertyIterator* piter = proxy->NewPropertyIterator();
    for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
      vtkSMProperty* property = piter->GetProperty();
      vtkSMDomainIterator* diter = property->NewDomainIterator();
      for (diter->Begin(); !diter->IsAtEnd(); diter->Next())
      {
        if (vtkSMFileListDomain::SafeDownCast(diter->GetDomain()))
        {
          fileNameProperties.insert(piter->GetKey());
        }
      }
      diter->Delete();
    }
    piter->Delete();
    return fileNameProperties;
  }
};

vtkStandardNewMacro(vtkSMLoadStateOptionsProxy);
//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::vtkSMLoadStateOptionsProxy()
{
  this->Internals = new vtkInternals();
  this->StateFileName = 0;
  this->PathMatchingThreshold = 3;
}

//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::~vtkSMLoadStateOptionsProxy()
{
  delete this->Internals;
  this->SetStateFileName(0);
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::PrepareToLoad(const char* statefilename)
{
  this->SetStateFileName(statefilename);
  pugi::xml_parse_result result = this->Internals->StateXML.load_file(statefilename);

  if (!result)
  {
    vtkErrorMacro(
      "Error parsing state file XML from " << statefilename << ": " << result.description());
    return false;
  }

  this->Internals->ProcessStateFile(this->Internals->StateXML);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::HasDataFiles()
{
  // This will always return false until PrepareToLoad has been called
  return (this->Internals->PropertiesMap.size() > 0);
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::LocateFilesInDirectory(std::vector<std::string>& filepaths)
{
  std::string lastLocatedPath = "";
  int numOfPathMatches = 0;
  std::vector<std::string>::iterator fIter;
  for (fIter = filepaths.begin(); fIter != filepaths.end(); ++fIter)
  {
    // TODO: Inefficient - need vtkPVInfomation class to bundle file paths
    if (numOfPathMatches < this->PathMatchingThreshold)
    {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LocateFileInDirectory"
             << *fIter << vtkClientServerStream::End;
      this->ExecuteStream(stream, false, vtkPVSession::DATA_SERVER_ROOT);
      vtkClientServerStream result = this->GetLastResult();
      std::string locatedPath = "";
      if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1)
      {
        result.GetArgument(0, 0, &locatedPath);
      }

      if (!locatedPath.empty())
      {
        *fIter = locatedPath;
        if (vtksys::SystemTools::GetParentDirectory(locatedPath) ==
          vtksys::SystemTools::GetParentDirectory(lastLocatedPath))
        {
          numOfPathMatches++;
        }
        else
        {
          numOfPathMatches = 0;
        }
        lastLocatedPath = locatedPath;
      }
      else
      {
        return false;
      }
    }
    else
    {
      std::vector<std::string> directoryPathComponents;
      vtksys::SystemTools::SplitPath(
        vtksys::SystemTools::GetParentDirectory(lastLocatedPath), directoryPathComponents);
      directoryPathComponents.push_back(vtksys::SystemTools::GetFilenameName(*fIter));
      *fIter = vtksys::SystemTools::JoinPath(directoryPathComponents);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::Load()
{
  SM_SCOPED_TRACE(LoadState).arg("filename", this->StateFileName).arg("options", this);

  int dataFileOptions = vtkSMPropertyHelper(this, "LoadStateDateFileOptions").GetAsInt();
  switch (dataFileOptions)
  {
    case 0:
    {
      // Nothing to do
      break;
    }
    case 1:
    {
      vtkInternals::PropertiesMapType::iterator iter;
      for (iter = this->Internals->PropertiesMap.begin();
           iter != this->Internals->PropertiesMap.end(); ++iter)
      {
        vtkInternals::PropertyInfo& info = iter->second;

        if (iter->first.find("FilePattern") == std::string::npos)
        {
          if (this->LocateFilesInDirectory(info.FilePaths))
          {
            info.Modified = true;
          }
          else
          {
            return false;
          }
        }
      }

      break;
    }
    case 2:
    {
      // TODO: Find a way to get Proxies to reproduce old Qt dialog
      vtkWarningMacro("Explicitly specifying file paths not implemented yet.");
      return false;
      break;
    }
  }

  vtkInternals::PropertiesMapType::iterator iter;
  for (iter = this->Internals->PropertiesMap.begin(); iter != this->Internals->PropertiesMap.end();
       ++iter)
  {
    vtkInternals::PropertyInfo& info = iter->second;
    if (!info.Modified)
    {
      continue;
    }

    std::vector<std::string>::iterator fIter;
    for (fIter = info.FilePaths.begin(); fIter != info.FilePaths.end(); ++fIter)
    {
      std::stringstream idx;
      idx << std::distance(info.FilePaths.begin(), fIter);
      info.XMLElement.find_child_by_attribute("Element", "index", idx.str().c_str())
        .attribute("value")
        .set_value(fIter->c_str());
    }

    // Also fix up sources proxy collection
    int id = std::atoi(iter->first.substr(0, iter->first.find(".")).c_str());

    // Get sequence basename if needed
    std::string filename = vtksys::SystemTools::GetFilenameName(info.FilePaths.at(0));
    if (this->SequenceParser->ParseFileSequence(filename.c_str()))
    {
      filename = this->SequenceParser->GetSequenceName();
    }

    this->Internals->CollectionsMap[id].attribute("name").set_value(filename.c_str());
  }

  // Convert back to vtkPVXMLElement for now
  std::ostringstream sstream;
  this->Internals->StateXML.save(sstream, "  ");
  vtkNew<vtkPVXMLParser> parser;
  parser->Parse(sstream.str().c_str());

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  pxm->LoadXMLState(parser->GetRootElement());

  return true;
}

//----------------------------------------------------------------------------
void vtkSMLoadStateOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
