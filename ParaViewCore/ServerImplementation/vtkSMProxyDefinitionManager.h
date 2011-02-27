/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyDefinitionManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyDefinitionManager - object responsible for managing XML proxies definitions
// .SECTION Description
// vtkSMProxyDefinitionManager is a class that manages XML proxies definition.
// It maintains a map of XML elements (populated by the XML parser) from
// which it can extract Hint, Documentation, Properties, Domains definition.
//
// Whenever the proxy definitions are updated, this class fires
// vtkSMProxyDefinitionManager::ProxyDefinitionsUpdated,
// vtkSMProxyDefinitionManager::CompoundProxyDefinitionsUpdated events. Note
// when a compound proxy is registered, on CompoundProxyDefinitionsUpdated event
// is fired.
// .SECTION See Also
// vtkSMXMLParser

#ifndef __vtkSMProxyDefinitionManager_h
#define __vtkSMProxyDefinitionManager_h

#include "vtkObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class vtkPVPlugin;
class vtkPVXMLElement;
class vtkSMProxyDefinitionIterator;

// Description:
// Information object used in Event notification
struct RegisteredDefinitionInformation
  {
    const char* GroupName;
    const char* ProxyName;
    bool CustomDefinition;

    RegisteredDefinitionInformation(const char* groupName,
                                    const char* proxyName,
                                    bool isCustom=false)
      {
      this->GroupName = groupName;
      this->ProxyName = proxyName;
      this->CustomDefinition = isCustom;
      }
  };

class VTK_EXPORT vtkSMProxyDefinitionManager : public vtkObject
{
public:
  // FIXME COLLABORATION : For now we dynamically convert InformationHelper
  // into the correct si_class and attribute sets.
  // THIS CODE SHOULD BE REMOVED once InformationHelper have been removed from
  // legacy XML
  static void PatchXMLProperty(vtkPVXMLElement* propElement);

  static vtkSMProxyDefinitionManager* New();
  vtkTypeMacro(vtkSMProxyDefinitionManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a registered proxy definition and return a NULL pointer if not found.
  // Moreover, error can be throw if the definition was not found if the flag
  // throwError is true.
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name, bool throwError);
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name)
    {
    return this->GetProxyDefinition(group, name, true);
    }


  // Description:
  // Returns the same thing as GetProxyDefinition in a flatten manner.
  // By flatten, we mean that the class hierarchy has been walked and merged
  // into a single vtkPVXMLElement definition.
  vtkPVXMLElement* GetCollapsedProxyDefinition(const char* group,
                                               const char* name,
                                               const char* subProxyName,
                                               bool throwError);
  vtkPVXMLElement* GetCollapsedProxyDefinition(const char* group,
                                               const char* name,
                                               const char* subProxyName)
    {
    return this->GetCollapsedProxyDefinition(group, name, subProxyName, true);
    }


  // Description:
  // Add a custom proxy definition. Custom definitions are NOT ALLOWED to
  // overrive or overlap any ProxyDefinition that has been defined by parsing
  // server manager proxy configuration files.
  // This can be a compound proxy definition (look at
  // vtkSMCompoundSourceProxy.h) or a regular proxy definition.
  // For all practical purposes, there's no difference between a proxy
  // definition added using this method or by parsing a server manager
  // configuration file.
  void AddCustomProxyDefinition(
      const char* group, const char* name, vtkPVXMLElement* top);

  // Description:
  // Given its name, remove a custom proxy definition.
  // Note that this can only be used to remove definitions added using
  // AddCustomProxyDefinition(), cannot be used to remove definitions
  // loaded using vtkSMXMLParser.
  void RemoveCustomProxyDefinition(const char* group, const char* name);

  // Description:
  // Remove all registered custom proxy definitions.
  // Note that this can only be used to remove definitions added using
  // AddCustomProxyDefinition(), cannot be used to remove definitions
  // loaded using vtkSMXMLParser.
  void ClearCustomProxyDefinition();

  // Description:
  // Load custom proxy definitions and register them.
  void LoadCustomProxyDefinitions(const char* filename);
  void LoadCustomProxyDefinitions(vtkPVXMLElement* root);
//BTX
  void LoadCustomProxyDefinitions(vtkSMMessage* msg);
//ETX

  // Description:
  // Save registered custom proxy definitions.
  void SaveCustomProxyDefinitions(const char* filename);
  void SaveCustomProxyDefinitions(vtkPVXMLElement* root);
//BTX
  void SaveCustomProxyDefinitions(vtkSMMessage* msg);
//ETX


  // Description:
  // Returns the number of proxies under the group with \c groupName for which
  // proxies can be created.
  unsigned int GetNumberOfXMLProxies(const char* groupName);

  // Description:
  // Returns the name for the nth XML proxy element under the
  // group with name \c groupName.
  const char* GetXMLProxyName(const char* groupName, unsigned int n);

  // Description:
  // Returns true if a proxy definition do exist for that given group name and
  // proxy name, false otherwise.
  bool ProxyElementExists(const char* groupName,  const char* proxyName);

  // Description:
  // Loads server-manager configuration xml.
  bool LoadConfigurationXML(const char* filename);
  bool LoadConfigurationXML(vtkPVXMLElement* root);
  bool LoadConfigurationXMLFromString(const char* xmlContent);

  enum Events
    {
    ProxyDefinitionsUpdated=2000,
    CompoundProxyDefinitionsUpdated=2001
    };

  // Description
  // Return a new configured iterator for traversing a set of proxy definition
  // for all the available groups.
  // Scope values:
  // 0 : ALL (default in case)
  // 1 : CORE_DEFINITIONS
  // 2 : CUSTOM_DEFINITIONS

  enum
    {
    ALL_DEFINITIONS=0,
    CORE_DEFINITIONS=1,
    CUSTOM_DEFINITIONS=2
    };

  vtkSMProxyDefinitionIterator* NewIterator(int scope=ALL_DEFINITIONS);

  // Description
  // Return a new configured iterator for traversing a set of proxy definition
  // for only one group name
  // Scope values:
  // 0 : ALL (default in case)
  // 1 : CORE_DEFINITIONS
  // 2 : CUSTOM_DEFINITIONS
  vtkSMProxyDefinitionIterator* NewSingleGroupIterator(const char* groupName,
    int scope=ALL_DEFINITIONS);

//BTX

  void GetXMLDefinitionState(vtkSMMessage* msg);
  void LoadXMLDefinitionState(vtkSMMessage* msg);

protected:
  vtkSMProxyDefinitionManager();
  ~vtkSMProxyDefinitionManager();

  // Description:
  // Callback called when a plugin is loaded.
  void OnPluginLoaded(vtkObject* caller, unsigned long event, void* calldata);
  void HandlePlugin(vtkPVPlugin*);

  // Description:
  // Called by the XML parser to add an element from which a proxy
  // can be created. Called during parsing.
  void AddElement(const char* groupName,
                  const char* proxyName, vtkPVXMLElement* element);

  // Description
  // Integrate a ProxyDefinition into another ProxyDefinition by merging them.
  // If properties are overriden is the last property that will last. So when we build
  // a merged definition hierarchy, we should start from the root and go down.
  void MergeProxyDefinition(vtkPVXMLElement* element, vtkPVXMLElement* elementToFill);

  void InvalidateCollapsedDefinition();

  // Description:
  // Given the proxy name and group name, returns the XML element for
  // the proxy.
  vtkPVXMLElement* GetProxyElement(const char* groupName,
                                   const char* proxyName);

  // Description:
  // Convenient method used to extract sub-proxy definition inside a proxy
  // definition. If (subProxyName == NULL) return proxyDefinition;
  vtkPVXMLElement* ExtractSubProxy(vtkPVXMLElement* proxyDefinition,
                                   const char* subProxyName);

private:
  vtkSMProxyDefinitionManager(const vtkSMProxyDefinitionManager&); // Not implemented
  void operator=(const vtkSMProxyDefinitionManager&); // Not implemented

  bool TriggerNotificationEvent;

  class vtkInternals;
  vtkInternals* Internals;
  vtkInternals* InternalsFlatten;
//ETX
};

#endif
