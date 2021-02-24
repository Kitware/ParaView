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
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
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
#include <vtk_pugixml.h>

using namespace vtksys;

#include <algorithm>
#include <set>
#include <sstream>

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
  typedef std::map<int, std::map<std::string, PropertyInfo> > PropertiesMapType;
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

    // Skip the filename domains from non-sources groups
    if (strcmp(group.value(), "sources") != 0)
    {
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

    int proxyId = node.attribute("id").as_int();

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
        this->PropertiesMap[proxyId][pIter->node().attribute("name").value()] = info;
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
      if (property->FindDomain<vtkSMFileListDomain>() != nullptr)
      {
        fileNameProperties.insert(piter->GetKey());
      }
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
  this->StateFileName = nullptr;
  this->PathMatchingThreshold = 3;
}

//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::~vtkSMLoadStateOptionsProxy()
{
  delete this->Internals;
  this->SetStateFileName(nullptr);
}

//----------------------------------------------------------------------------
namespace
{

/**
 * Converts pugixml to vtkPVXMLElement
 */
vtkPVXMLElement* ConvertXML(vtkPVXMLParser* parser, pugi::xml_node& node)
{
  std::stringstream ss;
  node.print(ss);

  parser->Parse(ss.str().c_str());
  return parser->GetRootElement();
}
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

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

  std::map<std::string, int> namesUsed;

  // Setup proxies for for explicit file change dialog
  for (auto idIter = this->Internals->PropertiesMap.begin();
       idIter != this->Internals->PropertiesMap.end(); idIter++)
  {
    pugi::xml_node proxyXML = idIter->second.begin()->second.XMLElement.parent();
    vtkSmartPointer<vtkSMProxy> newReaderProxy;
    newReaderProxy.TakeReference(
      pxm->NewProxy(proxyXML.attribute("group").value(), proxyXML.attribute("type").value()));
    newReaderProxy->PrototypeOn();
    newReaderProxy->SetLocation(0);

    // Property group to group properties by source
    pugi::xml_document propertyGroup;
    pugi::xml_node propertyGroupElement = propertyGroup.append_child("PropertyGroup");
    propertyGroupElement.append_attribute("label").set_value(proxyXML.attribute("type").value());
    propertyGroupElement.append_attribute("panel_widget").set_value("LoadStateOptionsDialog");

    vtkNew<vtkPVXMLParser> XMLParser;
    newReaderProxy->LoadXMLState(ConvertXML(XMLParser.Get(), proxyXML), nullptr);
    std::string newProxyName = std::to_string(idIter->first);
    this->AddSubProxy(newProxyName.c_str(), newReaderProxy.GetPointer());

    std::string baseName =
      this->Internals->CollectionsMap[idIter->first].attribute("name").as_string();

    // Remove all '.'s in the name to avoid possible name collisions with the appended index
    // If baseName is not unique for each proxy then the proxies are linked and their properties
    // cannot
    // be set separately if the property names are the same.
    baseName.erase(std::remove(baseName.begin(), baseName.end(), '.'), baseName.end());

    auto nameUseCount = namesUsed.find(baseName);
    if (nameUseCount != namesUsed.end())
    {
      baseName.append(".").append(std::to_string(nameUseCount->second));
      ++nameUseCount->second;
    }
    else
    {
      namesUsed[baseName] = 1;
    }

    for (auto pIter = idIter->second.begin(); pIter != idIter->second.end(); pIter++)
    {
      vtkSmartPointer<vtkSMProperty> property = newReaderProxy->GetProperty(pIter->first.c_str());
      property->SetPanelVisibility("default");

      // Hints to insure explicit file property only show up for that mode
      pugi::xml_document hints;
      pugi::xml_node widgetDecorator =
        hints.append_child("Hints").append_child("PropertyWidgetDecorator");
      widgetDecorator.append_attribute("type").set_value("GenericDecorator");
      widgetDecorator.append_attribute("mode").set_value("visibility");
      widgetDecorator.append_attribute("property").set_value("LoadStateDataFileOptions");
      widgetDecorator.append_attribute("value").set_value("2");

      property->SetHints(ConvertXML(XMLParser.Get(), hints));

      pugi::xml_node propertyXML = pIter->second.XMLElement;
      std::string exposedName = baseName;

      exposedName.append(".").append(propertyXML.attribute("name").value());
      this->ExposeSubProxyProperty(newProxyName.c_str(), propertyXML.attribute("name").value(),
        exposedName.c_str(), 1 /*override*/);
      propertyGroupElement.append_child("Property")
        .append_attribute("name")
        .set_value(exposedName.c_str());
    }

    this->NewPropertyGroup(ConvertXML(XMLParser.Get(), propertyGroup));
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::HasDataFiles()
{
  // This will always return false until PrepareToLoad has been called
  return (this->Internals->PropertiesMap.size() > 0);
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::LocateFilesInDirectory(
  std::vector<std::string>& filepaths, int path, bool clearFilenameIfNotFound)
{
  std::string lastLocatedPath = "";
  int numOfPathMatches = 0;
  std::vector<std::string>::iterator fIter;
  for (fIter = filepaths.begin(); fIter != filepaths.end(); ++fIter)
  {
    if (fIter->empty())
    {
      // don't attempt to fix empty strings (see paraview/paraview#19137).
      continue;
    }
    // TODO: Inefficient - need vtkPVInfomation class to bundle file paths
    if (numOfPathMatches < this->PathMatchingThreshold)
    {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LocateFileInDirectory"
             << *fIter << path << vtkClientServerStream::End;
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
        if (SystemTools::GetParentDirectory(locatedPath) ==
          SystemTools::GetParentDirectory(lastLocatedPath))
        {
          numOfPathMatches++;
        }
        else
        {
          numOfPathMatches = 0;
        }
        lastLocatedPath = locatedPath;
      }
      else if (clearFilenameIfNotFound)
      {
        vtkErrorMacro(<< "Could not find file " << *fIter << " in data directory.");
        *fIter = "";
      }
      else
      {
        return false;
      }
    }
    else
    {
      std::vector<std::string> directoryPathComponents;
      SystemTools::SplitPath(
        SystemTools::GetParentDirectory(lastLocatedPath), directoryPathComponents);
      directoryPathComponents.push_back(SystemTools::GetFilenameName(*fIter));
      *fIter = SystemTools::JoinPath(directoryPathComponents);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::Load()
{
  SM_SCOPED_TRACE(LoadState).arg("filename", this->StateFileName).arg("options", this);
  this->DataFileOptions = vtkSMPropertyHelper(this, "LoadStateDataFileOptions").GetAsInt();
  this->OnlyUseFilesInDataDirectory =
    (vtkSMPropertyHelper(this, "OnlyUseFilesInDataDirectory").GetAsInt() == 1);
  switch (this->DataFileOptions)
  {
    case USE_FILES_FROM_STATE:
    {
      // Nothing to do
      break;
    }
    case USE_DATA_DIRECTORY:
    {
      for (auto idIter = this->Internals->PropertiesMap.begin();
           idIter != this->Internals->PropertiesMap.end(); idIter++)
      {
        for (auto pIter = idIter->second.begin(); pIter != idIter->second.end(); pIter++)
        {
          vtkInternals::PropertyInfo& info = pIter->second;

          if (pIter->first.find("FilePattern") == std::string::npos)
          {
            bool path = pIter->first.compare("FilePrefix") == 0;
            if (this->LocateFilesInDirectory(
                  info.FilePaths, path, this->OnlyUseFilesInDataDirectory))
            {
              info.Modified = true;
            }
          }
        }
      }

      for (auto idIter = this->Internals->PropertiesMap.begin();
           idIter != this->Internals->PropertiesMap.end(); idIter++)
      {
        std::string primaryFilename = "";
        for (auto pIter = idIter->second.begin(); pIter != idIter->second.end(); pIter++)
        {
          vtkInternals::PropertyInfo& info = pIter->second;
          if (!info.Modified)
          {
            continue;
          }

          for (auto fIter = info.FilePaths.begin(); fIter != info.FilePaths.end(); ++fIter)
          {
            std::string idx = std::to_string(std::distance(info.FilePaths.begin(), fIter));
            info.XMLElement.find_child_by_attribute("Element", "index", idx.c_str())
              .attribute("value")
              .set_value(fIter->c_str());
            if (primaryFilename.empty() && fIter->compare(0, 3, "XML") != 0)
            {
              primaryFilename = fIter->c_str();
            }
          }

          // Also fix up sources proxy collection. Get file sequence basename if needed.
          if (!primaryFilename.empty() && info.FilePaths.size() > 1)
          {
            std::string filename = SystemTools::GetFilenameName(primaryFilename);
            vtkNew<vtkFileSequenceParser> sequenceParser;
            if (sequenceParser->ParseFileSequence(filename.c_str()))
            {
              filename = sequenceParser->GetSequenceName();
            }
            this->Internals->CollectionsMap[idIter->first].attribute("name").set_value(
              filename.c_str());
          }
        }
      }

      break;
    }
    case CHOOSE_FILES_EXPLICITLY:
    {
      for (auto idIter = this->Internals->PropertiesMap.begin();
           idIter != this->Internals->PropertiesMap.end(); idIter++)
      {
        std::string primaryFilename = "";
        vtkSMProxy* subProxy = this->GetSubProxy(std::to_string(idIter->first).c_str());
        for (auto pIter = idIter->second.begin(); pIter != idIter->second.end(); pIter++)
        {
          std::string propertyValue =
            vtkSMPropertyHelper(subProxy, pIter->first.c_str()).GetAsString();
          vtkInternals::PropertyInfo& info = pIter->second;

          // First check if environment variable shows up in user specified path
          if (propertyValue.compare(0, 1, "$") == 0)
          {
            std::vector<std::string> pathComponents;
            SystemTools::SplitPath(propertyValue, pathComponents);
            std::string variablePath;
            if (SystemTools::GetEnv(pathComponents[1].erase(0, 1), variablePath))
            {
              pathComponents.erase(pathComponents.begin(), pathComponents.begin() + 2);
              std::vector<std::string> variablePathComponents;
              SystemTools::SplitPath(variablePath, variablePathComponents);
              pathComponents.insert(pathComponents.begin(), variablePathComponents.begin(),
                variablePathComponents.end());
              propertyValue = SystemTools::JoinPath(pathComponents);
            }
            else
            {
              vtkWarningMacro("Environment variable " << pathComponents[1] << " is not set.");
              continue;
            }
          }

          // Clear out existing file names
          while (info.XMLElement.remove_child("Element"))
          {
          }

          // Add the new file names
          vtkSMPropertyHelper filenamePropHelper(subProxy, pIter->first.c_str());
          for (unsigned int i = 0; i < filenamePropHelper.GetNumberOfElements(); ++i)
          {
            // Build up file name list from the current property value
            pugi::xml_node newNode = info.XMLElement.append_child("Element");
            newNode.append_attribute("index").set_value(std::to_string(i).c_str());
            std::string filename = filenamePropHelper.GetAsString(i);
            newNode.append_attribute("value").set_value(filename.c_str());
            if (primaryFilename.empty())
            {
              primaryFilename = filename;
            }
          }
        }

        // Also fix up sources proxy collection
        std::string filename = SystemTools::GetFilenameName(primaryFilename);
        vtkNew<vtkFileSequenceParser> sequenceParser;
        if (sequenceParser->ParseFileSequence(filename.c_str()))
        {
          filename = sequenceParser->GetSequenceName();
        }

        this->Internals->CollectionsMap[idIter->first].attribute("name").set_value(
          filename.c_str());
      }
      break;
    }
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkNew<vtkPVXMLParser> XMLParser;
  pxm->LoadXMLState(ConvertXML(XMLParser.Get(), this->Internals->StateXML));

  return true;
}

//----------------------------------------------------------------------------
void vtkSMLoadStateOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
