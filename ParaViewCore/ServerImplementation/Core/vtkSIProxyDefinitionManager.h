/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProxyDefinitionManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIProxyDefinitionManager - object responsible for managing XML
// proxies definitions
// .SECTION Description
// vtkSIProxyDefinitionManager is a class that manages XML proxies definition.
// It maintains a map of vtkPVXMLElement (populated by the XML parser) from
// which it can extract Hint, Documentation, Properties, Domains definition.
//
// This class fires the following events:
// \li \c vtkSIProxyDefinitionManager::ProxyDefinitionsUpdated - Fired any time
// any definitions are updated. If a group of definitions are being updated (i.e.
// a new definition is registered, or unregistred, or modified)
// then this event gets fired after all of them are updated.
// \li \c vtkSIProxyDefinitionManager::CompoundProxyDefinitionsUpdated - Fired
// when a custom proxy definition is updated. Similar to
// ProxyDefinitionsUpdated this is fired after collective updates, if
// applicable. Note whenever CompoundProxyDefinitionsUpdated is fired,
// ProxyDefinitionsUpdated is also fired.
// \li \c vtkCommand::RegisterEvent - Fired when a new proxy definition is
// registered or an old one modified (through extensions). This is fired for
// regular proxies as well as custom proxy definitions.
// \li \c vtkCommand::UnRegisterEvent - Fired when a proxy definition is
// removed. Since this class only support removing custom proxies, this event is
// fired only when a custom proxy is removed.

#ifndef __vtkSIProxyDefinitionManager_h
#define __vtkSIProxyDefinitionManager_h

#include "vtkSIObject.h"

class vtkPVPlugin;
class vtkPVProxyDefinitionIterator;
class vtkPVXMLElement;

class VTK_EXPORT vtkSIProxyDefinitionManager : public vtkSIObject
{
public:
  static vtkSIProxyDefinitionManager* New();
  vtkTypeMacro(vtkSIProxyDefinitionManager, vtkSIObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the ID reserved for the proxy definition manager.
  static vtkTypeUInt32 GetReservedGlobalID();

  // Description:
  // For now we dynamically convert InformationHelper
  // into the correct si_class and attribute sets.
  // THIS CODE SHOULD BE REMOVED once InformationHelper have been removed
  // from legacy XML
  static void PatchXMLProperty(vtkPVXMLElement* propElement);

  // Description:
  // Returns a registered proxy definition or return a NULL otherwise.
  // Moreover, error can be throw if the definition was not found if the
  // flag throwError is true.
  vtkPVXMLElement* GetProxyDefinition( const char* group, const char* name,
                                       bool throwError );
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name)
    {
    // We do throw an error by default
    return this->GetProxyDefinition(group, name, true);
    }

  // Description:
  // Return true if the XML Definition was found
  bool HasDefinition( const char* groupName, const char* proxyName );

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
  void AddCustomProxyDefinition( const char* group, const char* name,
                                 vtkPVXMLElement* top );
  void AddCustomProxyDefinition(
    const char* groupName, const char* proxyName, const char* xmlcontents);

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
  void ClearCustomProxyDefinitions();

  // Description:
  // Load custom proxy definitions and register them.
  void LoadCustomProxyDefinitions(vtkPVXMLElement* root);
  void LoadCustomProxyDefinitionsFromString(const char* xmlContent);

  // Description:
  // Save registered custom proxy definitions. The caller must release the
  // reference to the returned vtkPVXMLElement.
  void SaveCustomProxyDefinitions(vtkPVXMLElement* root);

  // Description:
  // Loads server-manager configuration xml.
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
    ALL_DEFINITIONS    = 0,
    CORE_DEFINITIONS   = 1,
    CUSTOM_DEFINITIONS = 2
    };

  // Description:
  // Return a NEW instance of vtkPVProxyDefinitionIterator configured to
  // get through all the definition available for the requested scope.
  // Possible scope defined as enum inside vtkSIProxyDefinitionManager:
  //   ALL_DEFINITIONS=0 / CORE_DEFINITIONS=1 / CUSTOM_DEFINITIONS=2
  // Some extra restriction can be set directly on the iterator itself
  // by setting a set of GroupName...
  vtkPVProxyDefinitionIterator* NewIterator( int scope=ALL_DEFINITIONS );

  // Description
  // Return a new configured iterator for traversing a set of proxy definition
  // for only one GroupName.
  // Possible scope defined as enum inside vtkSIProxyDefinitionManager:
  //   ALL_DEFINITIONS=0 / CORE_DEFINITIONS=1 / CUSTOM_DEFINITIONS=2
  vtkPVProxyDefinitionIterator* NewSingleGroupIterator(const char* groupName,
    int scope=ALL_DEFINITIONS);

  // Description:
  // Desactivate the modification of the ProxyDefinitions for that given
  // vtkSIProxyDefinitionManager to make sure update only come from the
  // remote server and not plugin loaded on the client.
  void EnableXMLProxyDefnitionUpdate(bool);

//BTX
  // Description:
  // Push a new state to the underneath implementation
  // The provided implementation just store the message
  // and return it at the Pull one.
  virtual void Push(vtkSMMessage* msg);

  // Description:
  // Pull the current state of the underneath implementation
  // The provided implementation update the given message with the one
  // that has been previously pushed
  virtual void Pull(vtkSMMessage* msg);

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

protected:
  vtkSIProxyDefinitionManager();
  ~vtkSIProxyDefinitionManager();

  // Description:
  // Helper method that add a ShowInMenu Hint for a proxy definition.
  // This allow that given proxy to show up inside the Sources/Filters menu
  // inside the UI.
  void AttachShowInMenuHintsToProxy(vtkPVXMLElement* proxy);

  // Description:
  // Helper method that add a ShowInMenu Hint for any proxy definition that lie
  // in a sources or filters group.
  // @See method AttachShowInMenuHintsToProxy
  void AttachShowInMenuHintsToProxyFromProxyGroups(vtkPVXMLElement* root);

  // Description:
  // Loads server-manager configuration xml. Those method are protected
  // as they allow to automatically add some extra hints for those loaded
  // definition set. This is essentially used when proxy get loaded as
  // legacy proxy don't have those expected Hints.
  // FIXME: Once those pluging get updated, this extra hint attachement
  //        might be removed.
  bool LoadConfigurationXML(vtkPVXMLElement* root, bool attachShowInMenuHints);
  bool LoadConfigurationXMLFromString(const char* xmlContent, bool attachShowInMenuHints);

  // Description:
  // Callback called when a plugin is loaded.
  void OnPluginLoaded(vtkObject* caller, unsigned long event, void* calldata);
  void HandlePlugin(vtkPVPlugin*);

  // Description:
  // Called by the XML parser to add an element from which a proxy
  // can be created. Called during parsing.
  void AddElement(const char* groupName,
                  const char* proxyName, vtkPVXMLElement* element);

  // Description:
  // Implementation for add custom proxy definition.
  bool AddCustomProxyDefinitionInternal(
    const char* group, const char* name, vtkPVXMLElement* top );

  // Description
  // Integrate a ProxyDefinition into another ProxyDefinition by merging them.
  // If properties are overriden is the last property that will last. So when we build
  // a merged definition hierarchy, we should start from the root and go down.
  void MergeProxyDefinition(vtkPVXMLElement* element, vtkPVXMLElement* elementToFill);

  // Description:
  // Method used to clear the Flatten version of the definition. This will
  // force its recomputation when needed. This should be called each time
  // new definition get added. (Not the Custom one since those should NOT
  // conflict with the core definitions.)
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


  // Description:
  // Called when custom definitions are updated. Fires appropriate events.
  void InvokeCustomDefitionsUpdated();

private:
  vtkSIProxyDefinitionManager(const vtkSIProxyDefinitionManager&); // Not implemented
  void operator=(const vtkSIProxyDefinitionManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
  vtkInternals* InternalsFlatten;
//ETX
};

#endif
