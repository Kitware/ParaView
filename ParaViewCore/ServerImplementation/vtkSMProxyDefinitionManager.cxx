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
#include "vtkPVSession.h"
#include "vtkPVConfig.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"
#include "vtkPVServerManagerPluginInterface.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMGeneratedModules.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkStdString.h"
#include "vtkStringList.h"

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
  void Clear()
    {
    this->CoreDefinitions.clear();
    this->CustomsDefinitions.clear();
    }
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

  static void ExtractMetaInformation(vtkPVXMLElement* proxy,
                                     vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > &subProxyMap,
                                     vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > &propertyMap)
    {
    vtkstd::set<vtkstd::string> propertyTypeName;
    propertyTypeName.insert("DoubleVectorProperty");
    propertyTypeName.insert("IntVectorProperty");
    propertyTypeName.insert("ProxyProperty");
    propertyTypeName.insert("Property");

    unsigned int numChildren = proxy->GetNumberOfNestedElements();
    unsigned int cc;
    for (cc=0; cc < numChildren; cc++)
      {
      vtkPVXMLElement* child = proxy->GetNestedElement(cc);
      if (child && child->GetName())
        {
        if(strcmp(child->GetName(), "SubProxy") == 0)
          { // SubProxy
          if(child->GetAttribute("name"))
            {
            subProxyMap[child->GetAttribute("name")] = child;
            }
          vtkPVXMLElement* exposedPropElement =
              child->FindNestedElementByName("ExposedProperties");

          // exposedPropElement can be NULL if only Shared property are used...
          if(exposedPropElement)
            {
            unsigned int ccSub = 0;
            unsigned int numChildrenSub = exposedPropElement->GetNumberOfNestedElements();
            for(ccSub = 0; ccSub < numChildrenSub; ccSub++)
              {
              vtkPVXMLElement* subProxyChild = exposedPropElement->GetNestedElement(ccSub);
              if (subProxyChild && subProxyChild->GetName()
                && propertyTypeName.find(subProxyChild->GetName()) != propertyTypeName.end())
                  { // Property
                const char* propName = subProxyChild->GetAttribute("exposed_name");
                propName = propName ? propName : subProxyChild->GetAttribute("name");
                propertyMap[propName] = subProxyChild;
                } // Property end ---------------------
              }
            }
          } // SubProxy end ------------------
        else if(propertyTypeName.find(child->GetName()) != propertyTypeName.end())
          { // Property
          const char* propName = child->GetAttribute("exposed_name");
          propName = propName ? propName : child->GetAttribute("name");
          propertyMap[propName] = child;
          } // Property end ---------------------
        }
      }
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
        // TODO vtkErrorMacro("No definition available for that iterator.");
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
    return this->GroupNames.size() == 0
        || this->GroupNameIterator == this->GroupNames.end();
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

  // Now register with the plugin tracker, so that when new plugins are loaded,
  // we parse the XML if provided and automatically add it to the proxy
  // definitions.
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  tracker->AddObserver(vtkCommand::RegisterEvent, this,
    &vtkSMProxyDefinitionManager::OnPluginLoaded);
  // process any already loaded plugins.
  for (unsigned int cc=0; cc < tracker->GetNumberOfPlugins(); cc++)
    {
    this->HandlePlugin(tracker->GetPlugin(cc));
    }
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
  if (this->ProxyElementExists(groupName, proxyName))
    {
    this->Internals->CustomsDefinitions[groupName].erase(proxyName);

    // Let the world know that definitions may have changed.
    this->InvokeEvent(vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated);
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

    // Let the world know that definitions may have changed.
    this->InvokeEvent(vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated);
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
bool vtkSMProxyDefinitionManager::LoadConfigurationXMLFromString(const char* xmlContent)
{
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  if(parser->Parse(xmlContent))
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
  if (!root)
    {
    vtkErrorMacro("Must parse a configuration before storing it.");
    return false;
    }

  // Loop over the top-level elements.
  for (unsigned int i=0; i < root->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* group = root->GetNestedElement(i);
    const char* groupName = group->GetAttribute("name");

    // Loop over the top-level elements.
    for(unsigned int cc=0; cc < group->GetNumberOfNestedElements(); ++cc)
      {
      vtkPVXMLElement* proxy = group->GetNestedElement(cc);
      const char* proxyName = proxy->GetAttribute("name");
      if (proxyName)
        {
        this->AddElement(groupName, proxyName, proxy);
        }
      }
    }
  this->InvokeEvent(vtkSMProxyDefinitionManager::ProxyDefinitionsUpdated);
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
vtkPVXMLElement* vtkSMProxyDefinitionManager::ExtractSubProxy(
    vtkPVXMLElement* proxyDefinition, const char* subProxyName)
{
  if(!subProxyName)
    {
    return proxyDefinition;
    }

  // Extract just the sub-proxy in-line definition
  for(unsigned int cc=0;cc<proxyDefinition->GetNumberOfNestedElements();cc++)
    {
    if(strcmp(proxyDefinition->GetNestedElement(cc)->GetName(), "SubProxy") == 0)
      {
      vtkPVXMLElement* subProxyDef =
          proxyDefinition->GetNestedElement(cc)->FindNestedElementByName("Proxy");
      if( subProxyDef && strcmp( subProxyDef->GetAttribute("name"), subProxyName) == 0)
        {
        return subProxyDef;
        }
      }
    }

  return NULL;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyDefinitionManager::GetCollapsedProxyDefinition(
    const char* group, const char* name, const char* subProxyName, bool throwError)
{
  // Look in the cache
  vtkPVXMLElement* flattenDefinition = this->InternalsFlatten->GetProxyElement(group,name);
  if (flattenDefinition)
    {
    // Found it, so return it...
    return ExtractSubProxy(flattenDefinition, subProxyName);
    }

  // Not found in the cache, look if the definition exists
  vtkPVXMLElement* originalDefinition = this->GetProxyDefinition(group,name,throwError);

  // Look for parent hierarchy
  if(originalDefinition)
    {
    vtkPVXMLElement* realDefinition = originalDefinition;
    const char* base_group = originalDefinition->GetAttribute("base_proxygroup");
    const char* base_name  = originalDefinition->GetAttribute("base_proxyname");

    if( base_group && base_name)
      {
      vtkstd::vector<vtkPVXMLElement*> classHierarchy;
      while(originalDefinition)
        {
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
        this->MergeProxyDefinition(currentElement, newElement);
        }
      realDefinition->CopyAttributes(newElement);

      // Remove parent declaration
      newElement->RemoveAttribute("base_proxygroup");
      newElement->RemoveAttribute("base_proxyname");

      // Register it in the cache
      this->InternalsFlatten->CoreDefinitions[group][name] = newElement.GetPointer();

      return ExtractSubProxy(newElement.GetPointer(), subProxyName);
      }
    }

  // Could be either the original definition or a NULL pointer if not found
  return ExtractSubProxy(originalDefinition, subProxyName);
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::MergeProxyDefinition(vtkPVXMLElement* element,
                                                       vtkPVXMLElement* elementToFill)
{
  // Meta-data of elementToFill
  vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > subProxyToFill;
  vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > propertiesToFill;
  vtkInternals::ExtractMetaInformation( elementToFill,
                                        subProxyToFill,
                                        propertiesToFill);

  // Meta-data of element that should be merged into the other
  vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > subProxySrc;
  vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> > propertiesSrc;
  vtkInternals::ExtractMetaInformation( element,
                                        subProxySrc,
                                        propertiesSrc);

  // Look for conflicting sub-proxy name and remove their definition if override
  vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVXMLElement> >::iterator mapIter;
  mapIter = subProxyToFill.begin();
  while( mapIter != subProxyToFill.end())
    {
    vtkstd::string name = mapIter->first;
    if( subProxySrc.find(name) != subProxySrc.end() )
      {
      if (!subProxySrc[name]->GetAttribute("override"))
        {
        vtkWarningMacro("#####################################" << endl
                        << "Find conflict between 2 SubProxy name. ("
                        << name.c_str() << ")" << endl
                        << "#####################################" << endl);
        return;
        }
      else
        { // Remove the given subProxy of the Element to Fill
        vtkPVXMLElement *subProxyDefToRemove = subProxyToFill[name].GetPointer();
        subProxyDefToRemove->GetParent()->RemoveNestedElement(subProxyDefToRemove);
        }
      }
    // Move to next
    mapIter++;
    }

  // Look for conflicting property name and remove their definition if override
  mapIter = propertiesToFill.begin();
  while( mapIter != propertiesToFill.end())
    {
    vtkstd::string name = mapIter->first;
    if( propertiesSrc.find(name) != propertiesSrc.end())
      {
      if (!propertiesSrc[name]->GetAttribute("override") )
        {
        vtkWarningMacro("#####################################" << endl
                        << "Find conflict between 2 property name. ("
                        << name.c_str() << ")" << endl
                        << "#####################################" << endl);
        return;
        }
      else
        { // Remove the given property of the Element to Fill
        vtkPVXMLElement *subPropDefToRemove = propertiesToFill[name].GetPointer();
        subPropDefToRemove->GetParent()->RemoveNestedElement(subPropDefToRemove);
        }
      }
    // Move to next
    mapIter++;
    }

  // By default alway overide the documentation
  if( element->FindNestedElementByName("Documentation") &&
      elementToFill->FindNestedElementByName("Documentation"))
    {
    elementToFill->RemoveNestedElement(
        elementToFill->FindNestedElementByName("Documentation"));
    }

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
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::GetXMLDefinitionState(vtkSMMessage* msg)
{
  // Setup required message header
  msg->Clear();
  msg->set_global_id(1);
  msg->set_location(vtkPVSession::DATA_SERVER);

  // This is made in a naive way, but we are sure that at each request
  // we have the correct and latest definition available.
  // This is not the most efficient way to do it. But optimistation should come
  // after. And for now, it is the less intrusive way to deal with server
  // XML definition centralisation state.
  ProxyDefinitionState_ProxyXMLDefinition *xmlDef;
  vtkSMProxyDefinitionIterator* iter;

  // Core Definition
  iter = this->NewIterator(1);
  iter->GoToFirstItem();
  while( !iter->IsDoneWithTraversal() )
    {
    vtkstd::ostringstream xmlContent;
    iter->GetProxyDefinition()->PrintXML(xmlContent, vtkIndent());

    xmlDef = msg->AddExtension(ProxyDefinitionState::xml_definition_proxy);
    xmlDef->set_group(iter->GetGroupName());
    xmlDef->set_name(iter->GetProxyName());
    xmlDef->set_xml(xmlContent.str());

    iter->GoToNextItem();
    }
  iter->Delete();

  // Custome Definition
  iter = this->NewIterator(2);
  iter->GoToFirstItem();
  while( !iter->IsDoneWithTraversal() )
    {
    vtkstd::ostringstream xmlContent;
    iter->GetProxyDefinition()->PrintXML(xmlContent, vtkIndent());

    xmlDef = msg->AddExtension(ProxyDefinitionState::xml_custom_definition_proxy);
    xmlDef->set_group(iter->GetGroupName());
    xmlDef->set_name(iter->GetProxyName());
    xmlDef->set_xml(xmlContent.str());

    iter->GoToNextItem();
    }
  iter->Delete();
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::LoadXMLDefinitionState(vtkSMMessage* msg)
{
  // Init and local vars
  this->Internals->Clear();
  this->InternalsFlatten->Clear();
  vtkPVXMLParser *parser = vtkPVXMLParser::New();

  // Fill the definition with the content of the state
  int size = msg->ExtensionSize(ProxyDefinitionState::xml_definition_proxy);
  const ProxyDefinitionState_ProxyXMLDefinition *xmlDef;
  for(int i=0; i < size; i++)
    {
    xmlDef = &msg->GetExtension(ProxyDefinitionState::xml_definition_proxy, i);
    parser->Parse(xmlDef->xml().c_str());
    this->AddElement( xmlDef->group().c_str(), xmlDef->name().c_str(),
                      parser->GetRootElement());
    }

  // Manage custom ones
  size = msg->ExtensionSize(ProxyDefinitionState::xml_custom_definition_proxy);
  for(int i=0; i < size; i++)
    {
    xmlDef = &msg->GetExtension(ProxyDefinitionState::xml_custom_definition_proxy, i);
    parser->Parse(xmlDef->xml().c_str());
    this->AddCustomProxyDefinition( xmlDef->group().c_str(), xmlDef->name().c_str(),
                                    parser->GetRootElement());
    }
  parser->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::OnPluginLoaded(
  vtkObject*, unsigned long, void* calldata)
{
  vtkPVPlugin* plugin = reinterpret_cast<vtkPVPlugin*>(calldata);
  this->HandlePlugin(plugin);
}

//---------------------------------------------------------------------------
void vtkSMProxyDefinitionManager::HandlePlugin(vtkPVPlugin* plugin)
{
  vtkPVServerManagerPluginInterface* smplugin =
    dynamic_cast<vtkPVServerManagerPluginInterface*>(plugin);
  if (smplugin)
    {
    vtkstd::vector<vtkstd::string> xmls;
    smplugin->GetXMLs(xmls);
    for (size_t cc=0; cc < xmls.size(); cc++)
      {
      this->LoadConfigurationXMLFromString(xmls[cc].c_str());
      }
    }
}
//---------------------------------------------------------------------------
// For now we dynamically convert InformationHelper
// into the correct kernel_class and attribute sets.
// THIS CODE MUST BE REMOVED once InformationHelper have been removed from
// legacy XML
void vtkSMProxyDefinitionManager::PatchXMLProperty(vtkPVXMLElement* propElement)
{
  vtkPVXMLElement* informationHelper = NULL;

  // Search InformationHelper XML element
  for(unsigned int i=0; i < propElement->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* currentElement = propElement->GetNestedElement(i);
    if ( vtkstd::string(currentElement->GetName()).find("Helper") !=
         vtkstd::string::npos)
      {
      informationHelper = currentElement;
      break;
      }
    }

  // Process InformationHelper
  if(informationHelper)
    {
    if(strcmp(informationHelper->GetName(),"StringArrayHelper") == 0
       || strcmp(informationHelper->GetName(),"DoubleArrayInformationHelper") == 0
       || strcmp(informationHelper->GetName(),"IntArrayInformationHelper") == 0 )
      {
      propElement->SetAttribute("kernel_class", "vtkPMDataArrayProperty");
      }
    else if (strcmp(informationHelper->GetName(),"TimeStepsInformationHelper") == 0)
      {
      propElement->SetAttribute("kernel_class", "vtkPMTimeStepsProperty");
      }
    else if (strcmp(informationHelper->GetName(),"TimeRangeInformationHelper") == 0)
      {
      propElement->SetAttribute("kernel_class", "vtkPMTimeRangeProperty");
      }
    else if (strcmp(informationHelper->GetName(),"SILInformationHelper") == 0)
      {
      propElement->SetAttribute("kernel_class", "vtkPMSILProperty");
      propElement->SetAttribute("subtree", informationHelper->GetAttribute("subtree"));
      }
    else if (strcmp(informationHelper->GetName(),"ArraySelectionInformationHelper") == 0)
      {
      propElement->SetAttribute("kernel_class", "vtkPMArraySelectionProperty");
      propElement->SetAttribute("command", informationHelper->GetAttribute("attribute_name"));
      propElement->SetAttribute("number_of_elements_per_command", "2");
      }
    else if(strcmp(informationHelper->GetName(),"SimpleDoubleInformationHelper") == 0
            || strcmp(informationHelper->GetName(),"SimpleIntInformationHelper") == 0
            || strcmp(informationHelper->GetName(),"SimpleStringInformationHelper") == 0
            || strcmp(informationHelper->GetName(),"SimpleIdTypeInformationHelper") == 0 )
      {
      // Nothing to do, just remove them
      }
    else
      {
      cerr << "No PMProperty for the following information helper: "
           << informationHelper->GetName() << endl;
      }

    // Remove InformationHelper from XML
    propElement->RemoveNestedElement(informationHelper);
    }
}
