/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyDefinitionManager
 *
 * vtkSMProxyDefinitionManager is a remote-object that represents the
 * vtkSIProxyDefinitionManager instance on all the processes. ParaView clients
 * should use API on this class to add/update xml definitions to ensure that
 * the xmls are processed/updated correctly on all the processes.
*/

#ifndef vtkSMProxyDefinitionManager_h
#define vtkSMProxyDefinitionManager_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSIProxyDefinitionManager.h"  // needed for enums
#include "vtkSMMessageMinimal.h"          // needed
#include "vtkSMRemoteObject.h"
#include "vtkWeakPointer.h" // needed for weak pointer.

class vtkSMProxyLocator;
class vtkEventForwarderCommand;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyDefinitionManager : public vtkSMRemoteObject
{
public:
  static vtkSMProxyDefinitionManager* New();
  vtkTypeMacro(vtkSMProxyDefinitionManager, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Synchronizes the client-side definitions using the server-side definitions,
   * if applicable. Call this method after any code that could result in
   * changing of the XML definitions on the server e.g. loading of plugins.
   */
  void SynchronizeDefinitions();

  /**
   * Overridden call SynchronizeDefinitions() when the session changes. Also
   * ensures that the internal references to vtkSIProxyDefinitionManager are
   * updated correctly.
   */
  void SetSession(vtkSMSession*) override;

  //***************************************************************************
  // enums re-defined from vtkSIProxyDefinitionManager for convenience.
  enum Events
  {
    ProxyDefinitionsUpdated = vtkSIProxyDefinitionManager::ProxyDefinitionsUpdated,
    CompoundProxyDefinitionsUpdated = vtkSIProxyDefinitionManager::CompoundProxyDefinitionsUpdated
  };

  enum
  {
    ALL_DEFINITIONS = vtkSIProxyDefinitionManager::ALL_DEFINITIONS,
    CORE_DEFINITIONS = vtkSIProxyDefinitionManager::CORE_DEFINITIONS,
    CUSTOM_DEFINITIONS = vtkSIProxyDefinitionManager::CUSTOM_DEFINITIONS
  };

  //***************************************************************************
  // Methods simply forwarded to the client-side vtkSIProxyDefinitionManager
  // instance.

  //@{
  /**
   * Returns a registered proxy definition or return a NULL otherwise.
   * Moreover, error can be throw if the definition was not found if the
   * flag throwError is true.
   */
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name, bool throwError)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->GetProxyDefinition(group, name, throwError)
      : NULL;
  }
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->GetProxyDefinition(group, name)
      : NULL;
  }
  //@}

  /**
   * Returns the same thing as GetProxyDefinition in a flatten manner.
   * By flatten, we mean that the class hierarchy has been walked and merged
   * into a single vtkPVXMLElement definition.
   */
  vtkPVXMLElement* GetCollapsedProxyDefinition(
    const char* group, const char* name, const char* subProxyName, bool throwError)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->GetCollapsedProxyDefinition(
          group, name, subProxyName, throwError)
      : NULL;
  }

  /**
   * Return true if the XML Definition was found
   */
  bool HasDefinition(const char* groupName, const char* proxyName)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->HasDefinition(groupName, proxyName)
      : false;
  }

  /**
   * Save registered custom proxy definitions. The caller must release the
   * reference to the returned vtkPVXMLElement.
   */
  void SaveCustomProxyDefinitions(vtkPVXMLElement* root)
  {
    if (this->ProxyDefinitionManager)
    {
      this->ProxyDefinitionManager->SaveCustomProxyDefinitions(root);
    }
  }

  //@{
  /**
   * Return a NEW instance of vtkPVProxyDefinitionIterator configured to
   * get through all the definition available for the requested scope.
   * Possible scope defined as enum inside vtkSIProxyDefinitionManager:
   * ALL_DEFINITIONS=0 / CORE_DEFINITIONS=1 / CUSTOM_DEFINITIONS=2
   * Some extra restriction can be set directly on the iterator itself
   * by setting a set of GroupName...
   */
  VTK_NEWINSTANCE
  vtkPVProxyDefinitionIterator* NewIterator()
  {
    return this->ProxyDefinitionManager ? this->ProxyDefinitionManager->NewIterator() : NULL;
  }
  VTK_NEWINSTANCE
  vtkPVProxyDefinitionIterator* NewIterator(int scope)
  {
    return this->ProxyDefinitionManager ? this->ProxyDefinitionManager->NewIterator(scope) : NULL;
  }
  //@}

  //@{
  /**
   * Return a new configured iterator for traversing a set of proxy definition
   * for only one GroupName.
   * Possible scope defined as enum inside vtkSIProxyDefinitionManager:
   * ALL_DEFINITIONS=0 / CORE_DEFINITIONS=1 / CUSTOM_DEFINITIONS=2
   */
  vtkPVProxyDefinitionIterator* NewSingleGroupIterator(const char* groupName)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->NewSingleGroupIterator(groupName)
      : NULL;
  }
  vtkPVProxyDefinitionIterator* NewSingleGroupIterator(const char* groupName, int scope)
  {
    return this->ProxyDefinitionManager
      ? this->ProxyDefinitionManager->NewSingleGroupIterator(groupName, scope)
      : NULL;
  }
  //@}

  //***************************************************************************
  // Methods that ensure that the command takes effect on all instances of
  // vtkSIProxyDefinitionManager across all processes.

  //@{
  /**
   * Add/Remove/Clear custom proxy definitions.
   */
  void AddCustomProxyDefinition(const char* group, const char* name, vtkPVXMLElement* top);
  void RemoveCustomProxyDefinition(const char* group, const char* name);
  void ClearCustomProxyDefinitions();
  //@}

  //@{
  /**
   * Load custom proxy definitions and register them.
   */
  void LoadCustomProxyDefinitions(vtkPVXMLElement* root);
  void LoadCustomProxyDefinitionsFromString(const char* xmlContent);
  //@}

  //@{
  /**
   * Loads server-manager configuration xml.
   */
  bool LoadConfigurationXML(vtkPVXMLElement* root);
  bool LoadConfigurationXMLFromString(const char* xmlContent);
  //@}

  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

protected:
  vtkSMProxyDefinitionManager();
  ~vtkSMProxyDefinitionManager() override;

  vtkEventForwarderCommand* Forwarder;
  vtkWeakPointer<vtkSIProxyDefinitionManager> ProxyDefinitionManager;

private:
  vtkSMProxyDefinitionManager(const vtkSMProxyDefinitionManager&) = delete;
  void operator=(const vtkSMProxyDefinitionManager&) = delete;
};

#endif
