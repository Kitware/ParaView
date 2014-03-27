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

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"
#include "vtkSmartPointer.h" // Needed for the singleton

class vtkPVXMLElement;
class vtkSMPluginManager;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMProxySelectionModel;
class vtkSMReaderFactory;
class vtkSMSession;
class vtkSMSessionProxyManager;
class vtkSMStateLoader;
class vtkSMUndoStackBuilder;
class vtkSMWriterFactory;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyManager : public vtkSMObject
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
  // Returns the current active session. If no active session is set, and
  // there's only one registered vtkSMSession with vtkProcessModule, then that
  // session is automatically treated as the active session.
  vtkSMSession* GetActiveSession();

  // Description:
  // Set the active session. It's acceptable to set the active session as NULL
  // (or 0 in case of sessionId), however GetActiveSession() may automatically
  // pick an active session if none is provided.
  void SetActiveSession(vtkSMSession* session);
  void SetActiveSession(vtkIdType sessionId);

  // Description:
  // Convenient method to get the active vtkSMSessionProxyManager. If no
  vtkSMSessionProxyManager* GetActiveSessionProxyManager();

  // Description:
  // Return the corresponding vtkSMSessionProxyManager and if any,
  // then create a new one.
  vtkSMSessionProxyManager* GetSessionProxyManager(vtkSMSession* session);

  // Description:
  // Calls forwarded to the active vtkSMSessionProxyManager, if any. Raises
  // errors if no active session manager can be determined (using
  // GetActiveSessionProxyManager()).
  vtkSMProxy* NewProxy(const char* groupName,
    const char* proxyName, const char* subProxyName = NULL);
  void RegisterProxy(const char* groupname, const char* name, vtkSMProxy* proxy);
  vtkSMProxy* GetProxy(const char* groupname, const char* name);
  void UnRegisterProxy(const char* groupname, const char* name, vtkSMProxy*);
  const char* GetProxyName(const char* groupname, unsigned int idx);
  const char* GetProxyName(const char* groupname, vtkSMProxy* proxy);

  // Description:
  // Get/Set the undo-stack builder if the application is using undo-redo
  // mechanism to track changes.
  void SetUndoStackBuilder(vtkSMUndoStackBuilder* builder);
  vtkGetObjectMacro(UndoStackBuilder, vtkSMUndoStackBuilder);

  // Description:
  // PluginManager keeps track of plugins loaded on various sessions.
  // This provides access to the application-wide plugin manager.
  vtkGetObjectMacro(PluginManager, vtkSMPluginManager);

  // Description:
  // Provides access to the reader factory. Before using the reader factory, it
  // is essential that it's configured correctly.
  vtkGetObjectMacro(ReaderFactory, vtkSMReaderFactory);

  // Description:
  // Provides access to the writer factory. Before using the reader factory, it
  // is essential that it's configured correctly.
  vtkGetObjectMacro(WriterFactory, vtkSMWriterFactory);


  enum eventId
    {
    ActiveSessionChanged = 9753
    };

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

//BTX
protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();
#ifndef __WRAP__
  static vtkSMProxyManager* New();
#endif

  friend class vtkSMSessionProxyManager;

  // Description:
  // Connections updated. Update the active session accordingly.
  void ConnectionsUpdated(vtkObject*, unsigned long, void*);

  vtkSMUndoStackBuilder* UndoStackBuilder;
  vtkSMPluginManager* PluginManager;
  vtkSMReaderFactory* ReaderFactory;
  vtkSMWriterFactory* WriterFactory;

private:
  class vtkPXMInternal;
  vtkPXMInternal* PXMStorage;

  static vtkSmartPointer<vtkSMProxyManager> Singleton;

  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
//ETX
};

#endif
