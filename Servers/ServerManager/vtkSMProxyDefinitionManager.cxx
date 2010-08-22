/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyDefinitionManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyDefinitionManager.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMGeneratedModules.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStringList.h"

// #include "vtkSMProxyManager.h" // FIXME <===========================================================================
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/DateStamp.h> // For date stamp
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#include <assert.h>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
typedef vtkSmartPointer<vtkPVXMLElement>        XMLElement;
typedef vtkstd::map<vtkStdString, XMLElement>   StrToXmlMap;
typedef vtkstd::map<vtkStdString, StrToXmlMap>  StrToStrToXmlMap;

class vtkSMProxyDefinitionManager::vtkInternals
{
public:
  // Keep track of ServerManager definition
  StrToStrToXmlMap CoreDefinitions;
  // Keep track of custom definition
  StrToStrToXmlMap CustomsDefinitions;
  //-------------------------------------------------------------------------
  bool HasCoreDefinition( const char* groupName, const char* proxyName)
  {
    return GetProxyElement(CoreDefinitions, groupName, proxyName);
  }
  //-------------------------------------------------------------------------
  bool HasCustomDefinition( const char* groupName, const char* proxyName)
  {
    return GetProxyElement(CustomsDefinitions, groupName, proxyName);
  }
  //-------------------------------------------------------------------------
  unsigned int GetNumberOfProxy(const char* groupName)
  {
    unsigned int nbProxy = 0;
    if(groupName)
    {
      nbProxy += this->CoreDefinitions[groupName].size();
      nbProxy += this->CustomsDefinitions[groupName].size();
    }
    return nbProxy;
  }
  //-------------------------------------------------------------------------
  const char* GetXMLProxyName(const char* groupName, unsigned int n)
  {
    StrToXmlMap::iterator it;
    StrToXmlMap map = this->CoreDefinitions[groupName];
    if(map.size() > n)
      {
      for (it = map.begin(); it != map.end(); it++)
        {
        if(n-- == 0)
          {
          return it->first.c_str();
          }
        }
      }
    map = this->CustomsDefinitions[groupName];
    if(map.size() > n)
      {
      for (it = map.begin(); it != map.end(); it++)
        {
        if(n-- == 0)
          {
          return it->first.c_str();
          }
        }
      }
    return 0; // OutOfBounds
  }
  //-------------------------------------------------------------------------
  vtkPVXMLElement* GetProxyElement(const char* groupName,
                                   const char* proxyName)
  {
    vtkPVXMLElement* elementToReturn = 0;

    // Search in ServerManager definitions
    elementToReturn = GetProxyElement(this->CoreDefinitions,
                                      groupName, proxyName);

    // If not found yet, search in customs ones...
    if(!elementToReturn)
      {
      elementToReturn = GetProxyElement(this->CustomsDefinitions,
                                        groupName, proxyName);
      }

    return elementToReturn;
  }
  //-------------------------------------------------------------------------
  vtkPVXMLElement* GetProxyElement(const StrToStrToXmlMap map,
                                   const char* firstStr,
                                   const char* secondStr)
  {
    vtkPVXMLElement* elementToReturn = 0;

    // Test if parameters are valid
    if (firstStr && secondStr)
      {
      // Find the value based on both keys
      StrToStrToXmlMap::const_iterator it = map.find(firstStr);
      if (it != map.end())
        {
        // We found a match for the first key
        StrToXmlMap::const_iterator it2 = it->second.find(secondStr);
        if (it2 != it->second.end())
          {
          // We found a match for the second key
          elementToReturn = it2->second.GetPointer();
          }
        }
      }

    // The result might be NULL if the value was not found
    return elementToReturn;
  }
};
//****************************************************************************/
class vtkInternalDefinitionIterator : public vtkSMProxyDefinitionIterator
{
public:

  static vtkInternalDefinitionIterator* New();
  vtkTypeMacro(vtkInternalDefinitionIterator, vtkSMProxyDefinitionIterator);

  //-------------------------------------------------------------------------
  void GoToFirstItem()
  {
    this->Reset();
  }
  //-------------------------------------------------------------------------
  void GoToNextItem()
  {
    if(!IsDoneWithCoreTraversal())
      {
      this->NextCoreDefinition();
      }
    else if(!IsDoneWithCustomTraversal())
      {
      NextCustomDefinition();
      }
    else
      {
      while(IsDoneWithCoreTraversal() && IsDoneWithCustomTraversal() && !IsDoneWithGroupTraversal())
        {
        this->NextGroup();
        }
      }
  }
  //-------------------------------------------------------------------------
  bool IsDoneWithTraversal()
  {
    if(!Initialized)
    {
      GoToFirstItem();
    }
    if( IsDoneWithCoreTraversal() && IsDoneWithCustomTraversal())
      {
      if(IsDoneWithGroupTraversal())
        {
        return true;
        }
      else
        {
        NextGroup();
        return IsDoneWithTraversal();
        }
      }
    return false;
  }
  //-------------------------------------------------------------------------
  virtual const char* GetGroupName()
  {
    return this->CurrentGroupName.c_str();
  }
  //-------------------------------------------------------------------------
  virtual const char* GetProxyName()
  {
    if(this->IsCustom())
      {
      return this->CustomProxyIterator->first.c_str();
      }
    else
      {
      return this->CoreProxyIterator->first.c_str();
      }
  }
  //-------------------------------------------------------------------------
  virtual bool IsCustom()
  {
    return this->IsDoneWithCoreTraversal();
  }
  //-------------------------------------------------------------------------
  virtual vtkPVXMLElement* GetProxyDefinition()
  {
    if(this->IsCustom())
      {
      return this->CustomProxyIterator->second.GetPointer();
      }
    else
      {
      return this->CoreProxyIterator->second.GetPointer();
      }
  }
  //-------------------------------------------------------------------------
  void AddTraversalGroupName(const char* groupName)
  {
    this->GroupNames.insert(vtkStdString(groupName));
  }
  //-------------------------------------------------------------------------
  void RegisterCoreDefinitionMap(StrToStrToXmlMap* map)
  {
    this->CoreDefinitionMap = map;
  }
  //-------------------------------------------------------------------------
  void RegisterCustomDefinitionMap(StrToStrToXmlMap* map)
  {
    this->CustomDefinitionMap = map;
  }

 //-------------------------------------------------------------------------
  void GoToNextGroup()
  {
    this->NextGroup();
  }

protected:
  vtkInternalDefinitionIterator()
  {
    Initialized = false;
    CoreDefinitionMap = 0;
    CustomDefinitionMap = 0;
  }
  ~vtkInternalDefinitionIterator(){}

  //-------------------------------------------------------------------------
  void Reset()
  {
    Initialized = true;
    if(this->GroupNames.size() == 0)
      {
      // Look for all name available
      if(this->CoreDefinitionMap)
        {
        StrToStrToXmlMap::iterator it = this->CoreDefinitionMap->begin();
        while(it != this->CoreDefinitionMap->end())
          {
          this->AddTraversalGroupName(it->first.c_str());
          it++;
          }
        }
      if(this->CustomDefinitionMap)
        {
        StrToStrToXmlMap::iterator it = this->CustomDefinitionMap->begin();
        while(it != this->CustomDefinitionMap->end())
          {
          this->AddTraversalGroupName(it->first.c_str());
          it++;
          }
        }

      if(this->GroupNames.size() == 0)
        {
        // Throw an error FIXME +++++++++++++++++++++++++++++++++++++++++++++++
        return;
        }
      Reset();
      }
    else
      {
      this->GroupNameIterator = this->GroupNames.begin();
      }
  }
  //-------------------------------------------------------------------------
  bool IsDoneWithGroupTraversal()
  {
    return this->GroupNameIterator == this->GroupNames.end();
  }
  //-------------------------------------------------------------------------
  bool IsDoneWithCoreTraversal()
  {
    if(this->CoreDefinitionMap)
      {
      return this->CoreProxyIterator == this->CoreProxyIteratorEnd;
      }
    return true;
  }
  //-------------------------------------------------------------------------
  bool IsDoneWithCustomTraversal()
  {
    if(this->CustomDefinitionMap)
      {
      return this->CustomProxyIterator == this->CustomProxyIteratorEnd;
      }
    return true;
  }
  //-------------------------------------------------------------------------
  void NextGroup()
  {
    this->CurrentGroupName = *GroupNameIterator;
    this->GroupNameIterator++;
    if(this->CoreDefinitionMap)
      {
      this->CoreProxyIterator    = (*CoreDefinitionMap)[this->CurrentGroupName].begin();
      this->CoreProxyIteratorEnd = (*CoreDefinitionMap)[this->CurrentGroupName].end();
      }
    if(this->CustomDefinitionMap)
      {
      this->CustomProxyIterator    = (*CustomDefinitionMap)[this->CurrentGroupName].begin();
      this->CustomProxyIteratorEnd = (*CustomDefinitionMap)[this->CurrentGroupName].end();
      }
  }
  //-------------------------------------------------------------------------
  void NextCoreDefinition()
  {
    this->CoreProxyIterator++;
  }
  //-------------------------------------------------------------------------
  void NextCustomDefinition()
  {
    this->CustomProxyIterator++;
  }

private:
  bool Initialized;
  vtkStdString CurrentGroupName;
  StrToXmlMap::iterator CoreProxyIterator;
  StrToXmlMap::iterator CoreProxyIteratorEnd;
  StrToXmlMap::iterator CustomProxyIterator;
  StrToXmlMap::iterator CustomProxyIteratorEnd;
  StrToStrToXmlMap* CoreDefinitionMap;
  StrToStrToXmlMap* CustomDefinitionMap;
  vtkstd::set<vtkStdString> GroupNames;
  vtkstd::set<vtkStdString>::iterator GroupNameIterator;
};
//****************************************************************************/
vtkStandardNewMacro(vtkSMProxyDefinitionManager)
vtkStandardNewMacro(vtkInternalDefinitionIterator)
//---------------------------------------------------------------------------
vtkSMProxyDefinitionManager::vtkSMProxyDefinitionManager()
{
  this->Internals = new vtkInternals;
  this->InternalsFlatten = new vtkInternals;

  // Load the generated modules
# include "vtkParaViewIncludeModulesToSMApplication.h"

}

//---------------------------------------------------------------------------
vtkSMProxyDefinitionManager::~vtkSMProxyDefinitionManager()
{
  delete this->Internals;
  delete this->InternalsFlatten;
}

//----------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::AddElement(const char* groupName,
                                             const char* proxyName,
                                             vtkPVXMLElement* element)
{
  if (element->GetName() && strcmp(element->GetName(), "Extension") == 0)
    {
    // This is an extension for an existing definition.
    vtkPVXMLElement* coreElem = this->Internals->GetProxyElement(
        this->Internals->CoreDefinitions, groupName, proxyName);
    if(coreElem)
      {
      // We found it, so we can extend it
      for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
        {
        coreElem->AddNestedElement(element->GetNestedElement(cc));
        }
      }
    else
      {
      vtkWarningMacro("Extension for (" << groupName << ", " << proxyName
                      << ") ignored since could not find core definition.");
      }
    }
  else
    {
    // Just referenced it
    this->Internals->CoreDefinitions[groupName][proxyName] = element;
    }
}

//---------------------------------------------------------------------------
bool vtkSMProxyDefinitionManager::ProxyElementExists(const char* groupName,
                                                     const char* proxyName)
{
  return (false || this->Internals->GetProxyElement(groupName, proxyName));
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyDefinitionManager::GetProxyDefinition(
                 const char* groupName, const char* proxyName,
                 const bool throwError)
{
  vtkPVXMLElement* element = this->Internals->GetProxyElement(groupName, proxyName);
  if (!throwError || element)
    {
    return element;
    }
  else
    {
    vtkErrorMacro( << "No proxy that matches: group=" << groupName
                   << " and proxy=" << proxyName << " were found.");
    }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxyDefinitionManager::GetNumberOfXMLProxies(const char* groupName)
{
  return this->Internals->GetNumberOfProxy(groupName);
}
//---------------------------------------------------------------------------
const char* vtkSMProxyDefinitionManager::GetXMLProxyName(const char* groupName, unsigned int n)
{
  return this->Internals->GetXMLProxyName(groupName, n);
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::ClearCustomProxyDefinition()
{
  this->Internals->CustomsDefinitions.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::RemoveCustomProxyDefinition(
  const char* groupName, const char* proxyName)
{
  if(ProxyElementExists(groupName, proxyName))
    {
    // FIXME see what should be done for event management  // FIXME <===========================================================================
//    vtkSMProxyManager::RegisteredProxyInformation info;
//    info.Proxy = 0;
//    info.GroupName = groupName;
//    info.ProxyName = proxyName;
//    info.Type = vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION;
//    this->InvokeEvent(vtkCommand::UnRegisterEvent, &info);

    this->Internals->CustomsDefinitions[groupName].erase(proxyName);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::AddCustomProxyDefinition(
  const char* groupName, const char* proxyName, vtkPVXMLElement* top)
{
  if (!top)
    {
    return;
    }

  vtkPVXMLElement* currentCustomElement = this->Internals->GetProxyElement(
      this->Internals->CustomsDefinitions, groupName, proxyName);

  // There's a possibility that the custom proxy definition is the
  // state has already been loaded (or another proxy definition with
  // the same name exists). If that existing definition matches what
  // the state is requesting, we don't need to raise any errors,
  // simply skip it.
  if(currentCustomElement && !currentCustomElement->Equals(top))
    {
    // A definition already exist with not the same content
    vtkErrorMacro("Proxy definition has already been registered with name \""
                  << proxyName << "\" under group \"" << groupName <<"\".");
    }
  else
    {
    this->Internals->CustomsDefinitions[groupName][proxyName] = top;

    // FIXME see what should be done for event management
//    vtkSMProxyManager::RegisteredProxyInformation info; // FIXME <===========================================================================
//    info.Proxy = 0;
//    info.GroupName = groupName;
//    info.ProxyName = proxyName;
//    info.Type = vtkSMProxyManager::RegisteredProxyInformation::COMPOUND_PROXY_DEFINITION;
//    this->GetProxyManager()->InvokeEvent(vtkCommand::RegisterEvent, &info);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadCustomProxyDefinitions(vtkPVXMLElement* root)
{
  if (!root)
    {
    return;
    }

  vtksys::RegularExpression proxyDefRe(".*Proxy$");
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "CustomProxyDefinition") == 0)
      {
      const char* group = currentElement->GetAttribute("group");
      const char* name = currentElement->GetAttribute("name");
      if (name && group)
        {
        if (currentElement->GetNumberOfNestedElements() == 1)
          {
          vtkPVXMLElement* defElement = currentElement->GetNestedElement(0);
          const char* tagName = defElement->GetName();
          if (tagName && proxyDefRe.find(tagName))
            {
            // Register custom proxy definitions for all elements ending with
            // "Proxy".
            this->AddCustomProxyDefinition(group, name, defElement);
            }
          }
        }
      else
        {
        vtkErrorMacro("Missing name or group");
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadCustomProxyDefinitions(const char* filename)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  parser->Parse();

  this->LoadCustomProxyDefinitions(parser->GetRootElement());
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::SaveCustomProxyDefinitions(
  vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    vtkErrorMacro("root element must be specified.");
    return;
    }

  vtkSMProxyDefinitionIterator* iter = NewIterator(2); // 2: Custom only
  while(!iter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* elem = iter->GetProxyDefinition();
    if (elem)
      {
      vtkPVXMLElement* defElement = vtkPVXMLElement::New();
      defElement->SetName("CustomProxyDefinition");
      defElement->AddAttribute("name", iter->GetProxyName());
      defElement->AddAttribute("group", iter->GetGroupName());
      defElement->AddNestedElement(elem, 0);
      rootElement->AddNestedElement(defElement);
      defElement->Delete();
      }
    iter->GoToNextItem();
    }
  iter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::SaveCustomProxyDefinitions(const char* filename)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  root->SetName("CustomProxyDefinitions");
  this->SaveCustomProxyDefinitions(root);

  ofstream os(filename, ios::out);
  root->PrintXML(os, vtkIndent());
  root->Delete();
}

//---------------------------------------------------------------------------
bool vtkSMProxyDefinitionManager::LoadConfigurationXML(const char* filename)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  parser->SetFileName(filename);
  if(parser->Parse())
    {
    this->LoadConfigurationXML(parser->GetRootElement());
    parser->Delete();
    return true;
    }
  else
    {
    parser->Delete();
    return false;
    }
}
//---------------------------------------------------------------------------
bool vtkSMProxyDefinitionManager::LoadConfigurationXML(vtkPVXMLElement* root)
{
    if(!root)
      {
      vtkErrorMacro("Must parse a configuration before storing it.");
      return false;
      }

    // Loop over the top-level elements.
    unsigned int i;
    for(i=0; i < root->GetNumberOfNestedElements(); ++i)
      {
      vtkPVXMLElement* group = root->GetNestedElement(i);
      const char* groupName = group->GetAttribute("name");

      // Loop over the top-level elements.
      for(unsigned int i=0; i < group->GetNumberOfNestedElements(); ++i)
        {
        vtkPVXMLElement* proxy = group->GetNestedElement(i);
        const char* proxyName = proxy->GetAttribute("name");
        if(proxyName)
          {
          AddElement(groupName, proxyName, proxy);
          }
        }
      }
    return true;
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
vtkSMProxyDefinitionIterator* vtkSMProxyDefinitionManager::NewSingleGroupIterator(char const* groupName, int scope)
{
  vtkInternalDefinitionIterator* iterator = (vtkInternalDefinitionIterator*) NewIterator(scope);
  iterator->AddTraversalGroupName(groupName);
  return iterator;
}
//---------------------------------------------------------------------------
vtkSMProxyDefinitionIterator* vtkSMProxyDefinitionManager::NewIterator(int scope)
{
  vtkInternalDefinitionIterator* iterator = vtkInternalDefinitionIterator::New();
  switch(scope)
    {
    case 1: // Core only
      iterator->RegisterCoreDefinitionMap( & this->Internals->CoreDefinitions);
      break;
    case 2: // Custom only
      iterator->RegisterCustomDefinitionMap( & this->Internals->CustomsDefinitions);
      break;
    default: // Both
      iterator->RegisterCoreDefinitionMap( & this->Internals->CoreDefinitions);
      iterator->RegisterCustomDefinitionMap( & this->Internals->CustomsDefinitions);
      break;
    }
  return iterator;
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::InvalidateCollapsedDefinition()
{
  this->InternalsFlatten->CoreDefinitions.clear();
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyDefinitionManager::GetCollapsedProxyDefinition(const char* group, const char* name, bool throwError)
{
  // Look in the cache
  vtkPVXMLElement* flattenDefinition = this->InternalsFlatten->GetProxyElement(group,name);
  if (flattenDefinition)
    {
    // Found it, so return it...
    return flattenDefinition;
    }

  // Not found in the cache, look if the definition exists
  vtkPVXMLElement* originalDefinition = this->GetProxyDefinition(group,name,throwError);

  // Look for parent hierarchy
  if(originalDefinition)
    {
    const char* base_group = originalDefinition->GetAttribute("base_proxygroup");
    const char* base_name  = originalDefinition->GetAttribute("base_proxyname");

    if( base_group && base_name)
      {
      vtkstd::vector<vtkPVXMLElement*> classHierarchy;
      while(originalDefinition)
        {
        std::cout << "Found : " << originalDefinition->GetAttribute("name") << std::endl;
        classHierarchy.push_back(originalDefinition);
        if(base_group && base_name)
          {
          originalDefinition = this->GetProxyDefinition(base_group,base_name,throwError);
          base_group = originalDefinition->GetAttribute("base_proxygroup");
          base_name  = originalDefinition->GetAttribute("base_proxyname");
          }
        else
          {
          originalDefinition = 0;
          }
        }

      // Build the flattened version of it
      vtkSmartPointer<vtkPVXMLElement> newElement = vtkSmartPointer<vtkPVXMLElement>::New();
      while(classHierarchy.size() > 0)
        {
        vtkPVXMLElement* currentElement = classHierarchy.back();
        classHierarchy.pop_back();

        newElement->SetName(currentElement->GetName());
        newElement->SetAttribute("name",currentElement->GetAttribute("name"));
        MergeProxyDefinition(currentElement, newElement);
        }

      // Remove parent declaration
      newElement->RemoveAttribute("base_proxygroup");
      newElement->RemoveAttribute("base_proxyname");

      // Register it in the cache
      this->InternalsFlatten->CoreDefinitions[group][name] = newElement.GetPointer();

      return newElement.GetPointer();
      }
    }

  // Could be either the original definition or a NULL pointer if not found
  return originalDefinition;
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::MergeProxyDefinition(vtkPVXMLElement* element,
                                                       vtkPVXMLElement* elementToFill)
{
  // Local vars
  vtkstd::set<vtkStdString> exposedPropertyNames;
  vtkstd::set<vtkStdString> subProxyNames;
  vtkCollection* exposedProperties = vtkCollection::New();
  vtkCollection* subProxies = vtkCollection::New();
  vtkstd::set<vtkStdString> subProxiesToOverride;
  vtkstd::set<vtkStdString> exposedPropertiesToOverride;

  // Fill existing nested elements
  elementToFill->GetElementsByName("SubProxy", subProxies);
  elementToFill->GetElementsByName("DoubleVectorProperty", exposedProperties);
  elementToFill->GetElementsByName("IntVectorProperty", exposedProperties);
  elementToFill->GetElementsByName("ProxyProperty", exposedProperties);
  elementToFill->GetElementsByName("Property", exposedProperties);

  // Keep only one documentation node
  if(element->FindNestedElementByName("Documentation") &&
     elementToFill->FindNestedElementByName("Documentation"))
    {
    elementToFill->RemoveNestedElement(
        elementToFill->FindNestedElementByName("Documentation"));
    }

  // Extract properties names
  vtkCollectionIterator* xmlPropertyIter = exposedProperties->NewIterator();
  xmlPropertyIter->InitTraversal();
  while(!xmlPropertyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* propertyElement =
        vtkPVXMLElement::SafeDownCast(xmlPropertyIter->GetCurrentObject());

    const char* propName = propertyElement->GetAttribute("exposed_name");
    if(!propName)
      {
      propName = propertyElement->GetAttribute("name");
      }
    exposedPropertyNames.insert(propName);
    //
    xmlPropertyIter->GoToNextItem();
    }
  xmlPropertyIter->Delete();

  // Extract subProxy names
  vtkCollectionIterator* xmlSubProxyIter = subProxies->NewIterator();
  xmlSubProxyIter->InitTraversal();
  while(!xmlSubProxyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* subProxyElement =
        vtkPVXMLElement::SafeDownCast(xmlSubProxyIter->GetCurrentObject());
    vtkPVXMLElement* subProxy = subProxyElement->FindNestedElementByName("Proxy");
    subProxyNames.insert(subProxy->GetAttribute("name"));
    //
    xmlSubProxyIter->GoToNextItem();
    }
  xmlSubProxyIter->Delete();

  // Look for the same thing but on the input side
  exposedProperties->RemoveAllItems();
  subProxies->RemoveAllItems();

  // Fill nested elements that are available in the source
  element->GetElementsByName("SubProxy", subProxies);
  element->GetElementsByName("DoubleVectorProperty", exposedProperties);
  element->GetElementsByName("IntVectorProperty", exposedProperties);
  element->GetElementsByName("ProxyProperty", exposedProperties);
  element->GetElementsByName("Property", exposedProperties);

  // Look for conflict in Property name
  xmlPropertyIter = exposedProperties->NewIterator();
  xmlPropertyIter->InitTraversal();
  while(!xmlPropertyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* propertyElement =
        vtkPVXMLElement::SafeDownCast(xmlPropertyIter->GetCurrentObject());

    const char* propName = propertyElement->GetAttribute("exposed_name");
    if(!propName)
      {
      propName = propertyElement->GetAttribute("name");
      }
    if(exposedPropertyNames.find(propName) != exposedPropertyNames.end())
      {
      // Possible conflict
      if(!propertyElement->GetAttribute("override"))
        {
        // Conflict detected !!!!
        vtkErrorMacro("An exposed property conflict has been found during "
                      << "merging. (" << propName << ")");
        return;
        }
      else
        {
        // Need to override a property name
        exposedPropertiesToOverride.insert(propName);
        }
      }
    //
    xmlPropertyIter->GoToNextItem();
    }
  xmlPropertyIter->Delete();

  // Look for conflict in SubProxyName
  xmlSubProxyIter = subProxies->NewIterator();
  xmlSubProxyIter->InitTraversal();
  while(!xmlSubProxyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* subProxyElement =
        vtkPVXMLElement::SafeDownCast(xmlSubProxyIter->GetCurrentObject());

    vtkPVXMLElement* subProxy = subProxyElement->FindNestedElementByName("Proxy");
    if(subProxyNames.find(subProxy->GetAttribute("name")) != subProxyNames.end())
      {
      // Possible conflict
      if(!subProxy->GetAttribute("override"))
        {
        // Conflict detected !!!!
        vtkErrorMacro("A conflict has been found on subproxy during "
                      << "merging. (" << subProxy->GetAttribute("name") << ")");
        return;
        }
      else
        {
        // Need to override a subproxy
        subProxiesToOverride.insert(subProxy->GetAttribute("name"));
        }
      }
    //
    xmlSubProxyIter->GoToNextItem();
    }
  xmlSubProxyIter->Delete();

  // Remove overriden destination elements...

  // Remove overriden properties
  exposedProperties->RemoveAllItems();
  elementToFill->GetElementsByName("DoubleVectorProperty", exposedProperties);
  elementToFill->GetElementsByName("IntVectorProperty", exposedProperties);
  elementToFill->GetElementsByName("ProxyProperty", exposedProperties);
  elementToFill->GetElementsByName("Property", exposedProperties);
  xmlPropertyIter = exposedProperties->NewIterator();
  xmlPropertyIter->InitTraversal();
  while(!xmlPropertyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* propertyElement =
        vtkPVXMLElement::SafeDownCast(xmlPropertyIter->GetCurrentObject());

    const char* propName = propertyElement->GetAttribute("exposed_name");
    if(!propName)
      {
      propName = propertyElement->GetAttribute("name");
      }
    if(exposedPropertiesToOverride.find(propName)
       != exposedPropertiesToOverride.end())
      {
      propertyElement->GetParent()->RemoveNestedElement(propertyElement);
      }
    //
    xmlPropertyIter->GoToNextItem();
    }
  xmlPropertyIter->Delete();

  // Remove overriden sub-proxy
  subProxies->RemoveAllItems();
  elementToFill->GetElementsByName("SubProxy", subProxies);
  xmlSubProxyIter = subProxies->NewIterator();
  xmlSubProxyIter->InitTraversal();
  while(!xmlSubProxyIter->IsDoneWithTraversal())
    {
    vtkPVXMLElement* subProxyElement =
        vtkPVXMLElement::SafeDownCast(xmlSubProxyIter->GetCurrentObject());

    vtkPVXMLElement* subProxyProxy = subProxyElement->FindNestedElementByName("Proxy");
    if(subProxiesToOverride.find(subProxyProxy->GetAttribute("name"))
       != subProxiesToOverride.end())
      {
      // Remove the current subproxy
      elementToFill->RemoveNestedElement(subProxyElement);
      }
    //
    xmlSubProxyIter->GoToNextItem();
    }
  xmlSubProxyIter->Delete();


  // Fill the output with all the input elements
  unsigned int numChildren = element->GetNumberOfNestedElements();
  unsigned int cc;
  for (cc=0; cc < numChildren; cc++)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    vtkSmartPointer<vtkPVXMLElement> newElement =
        vtkSmartPointer<vtkPVXMLElement>::New();
    child->Copy(newElement);
    elementToFill->AddNestedElement(newElement);
    }

  // Free memory
  exposedProperties->Delete();
  subProxies->Delete();
}
