/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSessionBase
// .SECTION Description
// Abstract class used to provide the main implementation of the ParaView
// session methods for the following classes: vtkSMSession,
//                                            vtkSMSessionClient,
//                                            vtkSMSessionServer

#ifndef __vtkPVSessionBase_h
#define __vtkPVSessionBase_h

#include "vtkPVSession.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage

class vtkClientServerStream;
class vtkCollection;
class vtkSIObject;
class vtkPVInformation;
class vtkPVServerInformation;
class vtkSIProxyDefinitionManager;
class vtkPVSessionCore;

class VTK_EXPORT vtkPVSessionBase : public vtkPVSession
{
public:
  vtkTypeMacro(vtkPVSessionBase, vtkPVSession);
  void PrintSelf(ostream& os, vtkIndent indent);

  enum EventIds
    {
    RegisterRemoteObjectEvent   = 1234,
    UnRegisterRemoteObjectEvent = 4321,
    ProcessingRemoteEnd         = 2143
    };

  //---------------------------------------------------------------------------
  // Superclass Implementations
  //---------------------------------------------------------------------------

  // Description:
  // Returns a ServerFlags indicate the nature of the current processes. e.g. if
  // the current processes acts as a data-server and a render-server, it returns
  // DATA_SERVER | RENDER_SERVER. The implementation provided is suitable for
  // server processes such as pvserver, pvdataserver (both root and satellites).
  virtual ServerFlags GetProcessRoles();

  // Description:
  // vtkPVServerInformation is an information-object that provides information
  // about the server processes. These include server-side capabilities as well
  // as server-side command line arguments e.g. tile-display parameters. Use
  // this method to obtain the server-side information.
  // Overridden to provide support for non-remote-server case. We simply read
  // the local process information and return it.
  virtual vtkPVServerInformation* GetServerInformation();

  // Description:
  // This is socket connection, if any to communicate between the data-server
  // and render-server nodes. Forwarded for vtkPVSessionCore.
  virtual vtkMPIMToNSocketConnection* GetMPIMToNSocketConnection();

  //---------------------------------------------------------------------------
  // Remote communication API.
  //---------------------------------------------------------------------------

//BTX
  // Description:
  // Push the state message.
  virtual void PushState(vtkSMMessage* msg);

  // Description:
  // Pull the state message.
  virtual void PullState(vtkSMMessage* msg);

  // Description:
  // Execute a command on the given processes. Use GetLastResult() to obtain the
  // last result after the command stream is evaluated. Once can set
  // \c ignore_errors to true, to ignore any interpreting errors.
  virtual void ExecuteStream( vtkTypeUInt32 location,
                              const vtkClientServerStream& stream,
                              bool ignore_errors = false );

  // Description:
  // Returns the response of the ExecuteStream() call from the location. Note if
  // location refers to multiple processes, then the reply is only fetched from
  // the "closest" process.
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 location);
//ETX

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  virtual bool GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid);

  //---------------------------------------------------------------------------
  // API dealing with/forwarded to vtkPVSessionCore dealing with SIObjects and
  // SMObjects.
  //---------------------------------------------------------------------------

  // Description:
  // Provides access to the session core.
  vtkGetObjectMacro(SessionCore, vtkPVSessionCore);

  // Description:
  // Get the ProxyDefinitionManager.
  vtkSIProxyDefinitionManager* GetProxyDefinitionManager();

  // Description:
  // Returns a vtkSIObject or subclass given its global id, if any.
  vtkSIObject* GetSIObject(vtkTypeUInt32 globalid);

//BTX
  // Description:
  // Unregister server side object. (SIObject)
  virtual void UnRegisterSIObject(vtkSMMessage* msg);

  // Description:
  // Register server side object. (SIObject)
  virtual void RegisterSIObject(vtkSMMessage* msg);
//ETX

  // Description:
  // Return a vtkSMRemoteObject given its global id if any otherwise return NULL;
  vtkObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Allow the user to fill its vtkCollection with all RemoteObject
  // This could be usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);

  //---------------------------------------------------------------------------
  // API for GlobalId management
  //---------------------------------------------------------------------------

  // Description:
  // Provides the next available identifier. This implementation works locally.
  // without any code distribution. To support the distributed architecture
  // the vtkSMSessionClient overide those method to call them on the DATA_SERVER
  // vtkPVSessionBase instance.
  virtual vtkTypeUInt32 GetNextGlobalUniqueIdentifier();

  // Description:
  // Return the first Id of the requested chunk.
  // 1 = ReverveNextIdChunk(10); | Reserved ids [1,2,3,4,5,6,7,8,9,10]
  // 11 = ReverveNextIdChunk(10);| Reserved ids [11,12,13,14,15,16,17,18,19,20]
  // b = a + 10;
  virtual vtkTypeUInt32 GetNextChunkGlobalUniqueIdentifier(vtkTypeUInt32 chunkSize);

  // Description:
  // This propertie is used to discard ignore_synchronization proxy property
  // when we load protobuf states.
  // Therefore, if we load any camera state while that property is true, this
  // won't affect the proxy/property state at all. It will simply remain the same.
  virtual bool IsProcessingRemoteNotification();

  // Description:
  // Update internal session core in order to use the one used in another session
  virtual void UseSessionCoreOf(vtkPVSessionBase* other);

//BTX
protected:
  vtkPVSessionBase();
  vtkPVSessionBase(vtkPVSessionCore* coreToUse);
  ~vtkPVSessionBase();

  // Description:
  // Method used to migrate from one Session type to another by keeping the same
  // vtkPVSessionCore
  vtkPVSessionCore* GetSessionCore() const;
  void SetSessionCore(vtkPVSessionCore*);

  // Description:
  // Should be called to begin/end receiving progresses on this session.
  // Overridden to relay to the server(s).
  virtual void PrepareProgressInternal();
  virtual void CleanupPendingProgressInternal();

  friend class vtkSMRemoteObject;
  friend class vtkSMSessionProxyManager;

  // Description:
  // Methods used to monitor if we are currently processing a server notification
  // Only vtkSMSessionClient use the flag to disable ignore_synchronization
  // properties from beeing updated.
  virtual bool StartProcessingRemoteNotification();
  virtual void StopProcessingRemoteNotification(bool previousValue);
  bool ProcessingRemoteNotification;

  // Description:
  // Register a remote object
  void RegisterRemoteObject(vtkTypeUInt32 globalid, vtkTypeUInt32 location,
                            vtkObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkTypeUInt32 globalid, vtkTypeUInt32 location);

  vtkPVSessionCore* SessionCore;

private:
  vtkPVSessionBase(const vtkPVSessionBase&); // Not implemented
  void operator=(const vtkPVSessionBase&); // Not implemented

  // Shared constructor method
  void InitSessionBase(vtkPVSessionCore* coreToUse);

  vtkPVServerInformation* LocalServerInformation;
  unsigned long ActivateObserverTag;
  unsigned long DesactivateObserverTag;
//ETX
};

#endif
