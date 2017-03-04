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

#include "vtkNew.h"
#include "vtkObjectFactory.h"
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
#include "vtkSMVectorProperty.h"
#include "vtksys/SystemTools.hxx"
#include <algorithm>
#include <set>
#include <sstream>
#include <vector>

//---------------------------------------------------------------------------
class vtkSMLoadStateOptionsProxy::vtkInternals
{
public:
  struct PropertyInfo
  {
    vtkPVXMLElement* XMLElement;
    bool IsDirectory;
    bool AcceptAnyFile;
    bool SupportsMultiple;
    std::vector<std::string> Values; // TODO: why does this need to be a vector?
    bool Modified;
    vtkSmartPointer<vtkSMProxy> Prototype;
    PropertyInfo()
      : XMLElement(0)
      , IsDirectory(false)
      , AcceptAnyFile(false)
      , SupportsMultiple(false)
      , Modified(false)
    {
    }
  };
  typedef std::map<int, std::map<std::string, PropertyInfo> > PropertiesMapType;
  PropertiesMapType PropertiesMap;
  std::map<int, vtkPVXMLElement*> CollectionsMap;
  std::map<int, std::string> ProxyLabels;

  vtkSmartPointer<vtkPVXMLElement> XMLRoot;

  void processStateFile(vtkPVXMLElement* xml)
  {
    if (xml == NULL)
    {
      return;
    }

    if (strcmp(xml->GetName(), "ServerManagerState") == 0)
    {
      for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
      {
        vtkPVXMLElement* child = xml->GetNestedElement(cc);
        if (child && strcmp(child->GetName(), "Proxy") == 0)
        {
          this->processProxy(child);
        }
        else if (child && strcmp(child->GetName(), "ProxyCollection") == 0)
        {
          this->processProxyCollection(child);
        }
      }
    }
    else
    {
      for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
      {
        this->processStateFile(xml->GetNestedElement(cc));
      }
    }
  }

  void processProxy(vtkPVXMLElement* xml)
  {
    const char* group = xml->GetAttribute("group");
    const char* type = xml->GetAttribute("type");
    if (group == NULL || type == NULL)
    {
      vtkGenericWarningMacro("Possibly invalid state file.");
      return;
    }
    vtkSMProxy* prototype =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetPrototypeProxy(
        group, type);
    if (!prototype)
    {
      return;
    }

    std::set<std::string> filenameProperties = this->locateFileNameProperties(prototype);
    if (filenameProperties.size() == 0)
    {
      return;
    }

    vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();

    vtkSMProxy* tempClone = pxm->NewProxy(group, type);
    tempClone->SetLocation(0);
    tempClone->SetSession(NULL);

    // makes it easier to determine current values for filenames.
    tempClone->LoadXMLState(xml, NULL);

    int proxyid = std::atoi(xml->GetAttribute("id"));
    // iterate over all property xmls in the xml and add those xmls which
    // are in the filenameProperties set.
    for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* propXML = xml->GetNestedElement(cc);
      if (propXML && propXML->GetName() && strcmp(propXML->GetName(), "Property") == 0)
      {
        std::string propName = propXML->GetAttribute("name");
        if (filenameProperties.find(propName) != filenameProperties.end())
        {
          vtkSMProperty* smproperty = tempClone->GetProperty(propName.c_str());
          PropertyInfo info;
          info.XMLElement = propXML;
          // Is the property has repeat_command, then it's repeatable.
          info.SupportsMultiple = (smproperty->GetRepeatable() != 0);
          // Is this a directory?
          info.IsDirectory = (smproperty->GetHints() &&
            smproperty->GetHints()->FindNestedElementByName("UseDirectoryName"));
          // Does this accept any file?
          info.AcceptAnyFile = (smproperty->GetHints() &&
            smproperty->GetHints()->FindNestedElementByName("AcceptAnyFile"));

          info.Values = this->getValues(smproperty);
          info.Prototype = tempClone;
          this->PropertiesMap[proxyid][propName] = info;
        }
      }
    }
    tempClone->Delete();
    this->ProxyLabels[proxyid] = xml->GetAttribute("type");
  }

  void processProxyCollection(vtkPVXMLElement* xml)
  {
    const char* name = xml->GetAttribute("name");
    if (name == NULL)
    {
      vtkGenericWarningMacro(
        "Possibly invalid state file. Proxy Collection doesn't have a name attribute.");
      return;
    }

    if (strcmp(name, "sources") != 0)
    {
      return;
    }

    // iterate over all property xmls in the proxyXML and add those xmls which
    // are in the filenameProperties set.
    for (unsigned int cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* itemXML = xml->GetNestedElement(cc);
      if (itemXML && itemXML->GetName() && strcmp(itemXML->GetName(), "Item") == 0)
      {
        int itemId = std::atoi(itemXML->GetAttribute("id"));
        // this->CollectionsMap[itemId] = xml;
      }
    }
  }

  std::vector<std::string> getValues(vtkSMProperty* property)
  {
    std::vector<std::string> props;

    vtkSMVectorProperty* VectorProperty;
    VectorProperty = vtkSMVectorProperty::SafeDownCast(property);
    if (!VectorProperty)
    {
      return props;
    }

    // TODO: use smart pointer
    vtkSMPropertyHelper* helper = new vtkSMPropertyHelper(property);

    if (VectorProperty->IsA("vtkSMDoubleVectorProperty"))
    {
      std::vector<double> vals = helper->GetArray<double>();
      for (std::vector<double>::iterator it = vals.begin(); it != vals.end(); it++)
      {
        std::ostringstream sstream(*it);
        props.push_back(sstream.str());
      }
    }
    else if (VectorProperty->IsA("vtkSMIntVectorProperty"))
    {
      std::vector<int> vals = helper->GetArray<int>();
      for (std::vector<int>::iterator it = vals.begin(); it != vals.end(); it++)
      {
        std::ostringstream sstream(*it);
        props.push_back(sstream.str());
      }
    }
    else if (VectorProperty->IsA("vtkSMIdTypeVectorProperty"))
    {
      std::vector<vtkIdType> vals = helper->GetArray<vtkIdType>();
      for (std::vector<vtkIdType>::iterator it = vals.begin(); it != vals.end(); it++)
      {
        std::ostringstream sstream(*it);
        props.push_back(sstream.str());
      }
    }
    else if (VectorProperty->IsA("vtkSMStringVectorProperty"))
    {
      unsigned int count = helper->GetNumberOfElements();
      for (unsigned int cc = 0; cc < count; cc++)
      {
        props.push_back(helper->GetAsString(cc));
      }
    }
    delete helper;
    return props;
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

  void replaceDataFilePaths(vtkPVXMLElement* xml)
  {
    vtkInternals::PropertiesMapType::iterator iter;
    for (iter = this->PropertiesMap.begin(); iter != this->PropertiesMap.end(); ++iter)
    {
      std::map<std::string, PropertyInfo>::iterator iter2 = iter->second.begin();
      for (; iter2 != iter->second.end(); ++iter2)
      {
        vtkInternals::PropertyInfo& info = iter2->second;
        if (!info.Modified)
        {
          continue;
        }

        // Update XML Element using new values.
        std::ostringstream sstream(info.Values.size());
        info.XMLElement->SetAttribute("number_of_elements", sstream.str().c_str());
        for (int cc = info.XMLElement->GetNumberOfNestedElements() - 1; cc >= 0; cc--)
        {
          // remove old "Element" elements.
          vtkPVXMLElement* child = info.XMLElement->GetNestedElement(cc);
          if (strcmp(child->GetName(), "Element") == 0)
          {
            info.XMLElement->RemoveNestedElement(child);
          }
        }
        int index = 0;
        for (std::vector<std::string>::iterator it = info.Values.begin(); it != info.Values.end();
             it++)
        {
          vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
          elementElement->SetName("Element");
          elementElement->AddAttribute("index", index++);
          elementElement->AddAttribute("value", it->c_str());
          info.XMLElement->AddNestedElement(elementElement);
          elementElement->Delete();
        }

        // Also fix up sources proxy collection
        int id = iter->first;
        std::map<int, vtkPVXMLElement*>::iterator proxyCollectionIter =
          this->CollectionsMap.begin();
        vtkPVXMLElement* proxyCollectionXML = proxyCollectionIter->second;
        for (unsigned int cc = 0; cc < proxyCollectionXML->GetNumberOfNestedElements(); cc++)
        {
          // locate and remove old element.
          vtkPVXMLElement* itemXML = proxyCollectionXML->GetNestedElement(cc);
          int itemId = std::atoi(itemXML->GetAttribute("id"));
          if (id == itemId)
          {
            proxyCollectionXML->RemoveNestedElement(itemXML);

            // create a new element with new source name
            vtkPVXMLElement* newItemXML = vtkPVXMLElement::New();
            newItemXML->SetName("Item");
            newItemXML->AddAttribute("id", itemId);

            vtkGenericWarningMacro("info.Values = " << info.Values[0]);
            // TODO: Need to work out how to port from Qt code
            // newItemXML->AddAttribute(
            //   "name", this->ConstructPipelineName(info.Values).toLocal8Bit().data());
            // proxyCollectionXML->AddNestedElement(newItemXML);
            newItemXML->Delete();
            break;
          }
        }
      }
    }
  }
};

vtkStandardNewMacro(vtkSMLoadStateOptionsProxy);
//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::vtkSMLoadStateOptionsProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::~vtkSMLoadStateOptionsProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::PrepareToLoad(const char* statefilename)
{
  // TODO: ....
  if (vtksys::SystemTools::StringEndsWith(statefilename, ".pvsm"))
  {
    vtkNew<vtkPVXMLParser> xmlParser;
    xmlParser->SetFileName(statefilename);
    xmlParser->Parse();

    this->StateXML = xmlParser->GetRootElement();

    this->Internals->processStateFile(this->StateXML);
  }
  else
  { // python file
#ifdef PARAVIEW_ENABLE_PYTHON
// TODO: ....
#else
    vtkWarningMacro("ParaView was not built with Python support so it cannot open a python file");
    return false;
#endif
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
bool vtkSMLoadStateOptionsProxy::Load()
{
  SM_SCOPED_TRACE(LoadState).arg("filename", "todo: the filename").arg("options", this);

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
      // TODO: Deal with data directory
      break;
    }
    case 2:
    {
      // TODO: Replace individual files
      std::string dateFilePath = vtkSMPropertyHelper(this, "DataFilePaths").GetAsString();
      std::vector<std::string> filenames;
      filenames.push_back(dateFilePath);

      // Hard coded for Disk.pvsm for now
      int key1 = 8817;
      std::string key2 = "FileName";
      vtkInternals::PropertyInfo& info = this->Internals->PropertiesMap[key1][key2];

      if (info.Values != filenames)
      {
        info.Values = filenames;
        info.Modified = true;
      }
      break;
    }
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  pxm->LoadXMLState(this->StateXML);

  return true;
}

//----------------------------------------------------------------------------
void vtkSMLoadStateOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
