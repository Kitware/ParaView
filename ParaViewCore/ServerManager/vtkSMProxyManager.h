/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyManager - singleton/facade to vtkSMSessionProxyManager
// .SECTION Description
// vtkSMProxyManager is a singleton/facade that creates and manages proxies.
// It maintains a map of vtkSMSessionProxyManager and delegate all proxy related
// call to the appropriate one based on the provided session.
// .SECTION See Also
// vtkSMSessionProxyManager, vtkSMProxyDefinitionManager

#ifndef __vtkSMProxyManager_h
#define __vtkSMProxyManager_h

#include "vtkSMObject.h"
#include "vtkSmartPointer.h" // Needed for the singleton

class vtkPVXMLElement;
class vtkSMSession;
class vtkSMStateLoader;
class vtkSMProxy;
class vtkSMSessionProxyManager;
class vtkSMProxyLocator;
class vtkSMProxySelectionModel;
class vtkSMUndoStackBuilder;
class vtkSMGlobalPropertiesManager;

class VTK_EXPORT vtkSMProxyManager : public vtkSMObject
{
public:
  vtkTypeMacro(vtkSMProxyManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the global ProxyManager.
  static vtkSMProxyManager* GetProxyManager();

  // Description:
  // Free the singleton
  static void Finalize();

  // Description:
  // Allow to check if the Singleton has been initialized and has a reference
  // to a valid ProxyManager
  static bool IsInitialized();

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the major version number eg. if version is 2.9.1
  // this method will return 2.
  static int GetVersionMajor();

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the minor version number eg. if version is 2.9.1
  // this method will return 9.
  static int GetVersionMinor();

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the patch version number eg. if version is 2.9.1
  // this method will return 1.
  static int GetVersionPatch();

  // Description:
  // Returns a string with the format "paraview version x.x.x, Date: YYYY-MM-DD"
  static const char* GetParaViewSourceVersion();

  // Description:
  // Return the current active session if any otherwise, return NULL.
  vtkSMSession* GetActiveSession();

  // Description:
  // Set a new active session and register an associated vtkSMSessionProxyManager
  // if none was attached to that given session.
  void SetActiveSession(vtkSMSession* session);

  // Description:
  // Convenient method to get the active vtkSMSessionProxyManager
  vtkSMSessionProxyManager* GetActiveSessionProxyManager();

  // Description:
  // Return the corresponding vtkSMSessionProxyManager and if any,
  // then create a new one.
  vtkSMSessionProxyManager* GetSessionProxyManager(vtkSMSession* session);

  // Description:
  // Need to be called when a session get removed so we can cleanup the
  // corresponding vtkSMSessionProxyManager
  void UnRegisterSession(vtkSMSession*);

  // Description:
  // This will update each pipeline for each session in the proper order.
  // This method is used after a undo/redo action to ensure that every changes
  // have been properly pushed.
  void UpdateAllRegisteredProxiesInOrder();

  // Description:
  // Register/UnRegister a selection model. A selection model can be typically
  // used by applications to keep track of active sources, filters, views etc.
  void RegisterSelectionModel(const char* name, vtkSMProxySelectionModel*);
  void UnRegisterSelectionModel(const char* name);

  // Description:
  // Get a registered selection model. Will return null if no such model is
  // registered.
  vtkSMProxySelectionModel* GetSelectionModel(const char* name);

  // Description:
  // ParaView has notion of "global properties". These are application wide
  // properties such as foreground color, text color etc. Changing values of
  // these properties affects all objects that are linked to these properties.
  // This class provides convenient API to setup/remove such links.
  void SetGlobalPropertiesManager(const char* name, vtkSMGlobalPropertiesManager*);
  void RemoveGlobalPropertiesManager(const char* name);

  // Description:
  // Accessors for global properties managers.
  unsigned int GetNumberOfGlobalPropertiesManagers();
  vtkSMGlobalPropertiesManager* GetGlobalPropertiesManager(unsigned int index);
  vtkSMGlobalPropertiesManager* GetGlobalPropertiesManager(const char* name);
  const char* GetGlobalPropertiesManagerName(vtkSMGlobalPropertiesManager*);


  // Description:
  // Attach an undo stack builder which will be attached to any existing session
  // or any session to come.
  void AttachUndoStackBuilder(vtkSMUndoStackBuilder* undoBuilder);

  // Description:
  // Test if any SessionProxyManager is available. If any return true.
  bool HasSessionProxyManager();

//BTX
  struct RegisteredProxyInformation
  {
    vtkSMProxy* Proxy;
    const char* GroupName;
    const char* ProxyName;
    // Set when the register/unregister event if fired for registration of
    // a compound proxy definition.
    unsigned int Type;

    enum
      {
      PROXY =0x1,
      COMPOUND_PROXY_DEFINITION = 0x2,
      LINK = 0x3,
      GLOBAL_PROPERTIES_MANAGER = 0x4
      };
  };

  struct ModifiedPropertyInformation
    {
    vtkSMProxy* Proxy;
    const char* PropertyName;
    };

  struct LoadStateInformation
    {
    vtkPVXMLElement* RootElement;
    vtkSMProxyLocator* ProxyLocator;
    };

  struct StateChangedInformation
    {
    vtkSMProxy* Proxy;
    vtkPVXMLElement* StateChangeElement;
    };

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();

  static vtkSMProxyManager* New();

  friend class vtkSMSessionProxyManager;

  // Description:
  // Save global property managers.
  void SaveGlobalPropertiesManagers(vtkPVXMLElement* root);

private:
  class vtkPXMInternal;
  vtkPXMInternal* PXMStorage;

  static vtkSmartPointer<vtkSMProxyManager> Singleton;

  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
//ETX
};

#endif
