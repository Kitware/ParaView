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
#include <vector>
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

  void processStateFile(pugi::xml_node node)
  {
    if (strcmp(node.name(), "ServerManagerState") == 0)
    {
      for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
      {
        if (strcmp(it->name(), "Proxy") == 0)
        {
          this->processProxy(*it);
        }
        else if (strcmp(it->name(), "ProxyCollection") == 0)
        {
          this->processProxyCollection(*it);
        }
      }
    }
    else
    {
      for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
      {
        this->processStateFile(*it);
      }
    }
  }

  void processProxy(pugi::xml_node node)
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

    std::set<std::string> filenameProperties = this->locateFileNameProperties(prototype);
    if (filenameProperties.size() == 0)
    {
      return;
    }

    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
    {
      if (strcmp(it->name(), "Property") == 0)
      {
        if (filenameProperties.find(it->attribute("name").value()) != filenameProperties.end())
        {
          PropertyInfo info;
          info.XMLElement = *it;
          for (pugi::xml_node_iterator it2 = it->begin(); it2 != it->end(); ++it2)
          {
            info.FilePaths.push_back(it->child("Element").attribute("value").value());
          }
          this->PropertiesMap[it->attribute("id").value()] = info;
        }
      }
    }
  }

  void processProxyCollection(pugi::xml_node node)
  {
    pugi::xml_attribute name = node.attribute("name");
    if (name.empty())
    {
      vtkGenericWarningMacro(
        "Possibly invalid state file. Proxy Collection doesn't have a name attribute.");
      return;
    }

    if (strcmp(name.value(), "sources") != 0)
    {
      return;
    }

    // iterate over all property xmls in the proxyXML and add those xmls which
    // are in the filenameProperties set.
    for (pugi::xml_node_iterator it = node.begin(); it != node.end(); ++it)
    {
      if (strcmp(it->name(), "Item") == 0)
      {
        int itemId = it->attribute("id").as_int();
        this->CollectionsMap[itemId] = *it;
      }
    }
  }

  std::set<std::string> locateFileNameProperties(vtkSMProxy* proxy)
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
  this->SequenceParser = vtkFileSequenceParser::New();
}

//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::~vtkSMLoadStateOptionsProxy()
{
  delete this->Internals;
  this->SetStateFileName(0);
  this->SequenceParser->Delete();
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

  this->Internals->processStateFile(this->Internals->StateXML);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::HasDataFiles()
{
  // This will always return false until PrepareToLoad has been called
  return (this->Internals->PropertiesMap.size() > 0);
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
      vtkSMPropertyHelper propertyHelper(this, "DataDirectory");

      // Default to state file directory
      if (strlen(propertyHelper.GetAsString()) == 0)
      {
        propertyHelper.Set(vtksys::SystemTools::GetParentDirectory(this->StateFileName).c_str());
        this->UpdateVTKObjects();
      }

      vtkInternals::PropertiesMapType::iterator iter;
      for (iter = this->Internals->PropertiesMap.begin();
           iter != this->Internals->PropertiesMap.end(); ++iter)
      {
        vtkInternals::PropertyInfo& info = iter->second;

        if (iter->first.find("FilePattern") == std::string::npos)
        {
          std::vector<std::string>::iterator fIter;
          for (fIter = info.FilePaths.begin(); fIter != info.FilePaths.end(); ++fIter)
          {
            // TODO: Inefficient - need vtkPVInfomation class to bundle file paths
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
              info.Modified = true;
            }
            else
            {
              return false;
            }
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
      info.XMLElement.child("Element").attribute("value").set_value(fIter->c_str());
    }

    // Also fix up sources proxy collection
    int id = std::atoi(iter->first.substr(0, iter->first.find(".")).c_str());

    // Get sequence basename if needed
    std::string filename = vtksys::SystemTools::GetFilenameName(info.FilePaths[0]);
    char* cstr = new char[filename.length() + 1];
    strcpy(cstr, filename.c_str());
    if (this->SequenceParser->ParseFileSequence(cstr))
    {
      filename = this->SequenceParser->GetSequenceName();
    }
    delete[] cstr;

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
