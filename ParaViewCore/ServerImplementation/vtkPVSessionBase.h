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
  // Delete server side object. (SIObject)
  virtual void DeleteSIObject(vtkSMMessage* msg);
//ETX

  // Description:
  // Return a vtkSMRemoteObject given its global id if any otherwise return NULL;
  vtkObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Allow the user to fill its vtkCollection with all RemoteObject
  // This could be usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);


//BTX
protected:
  vtkPVSessionBase();
  ~vtkPVSessionBase();

  // Description:
  // Should be called to begin/end receiving progresses on this session.
  // Overridden to relay to the server(s).
  virtual void PrepareProgressInternal();
  virtual void CleanupPendingProgressInternal();

  friend class vtkSMRemoteObject;

  // Description:
  // Register a remote object
  void RegisterRemoteObject(vtkTypeUInt32 globalid,vtkObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkTypeUInt32 globalid, vtkTypeUInt32 location);

  vtkPVSessionCore* SessionCore;

private:
  vtkPVSessionBase(const vtkPVSessionBase&); // Not implemented
  void operator=(const vtkPVSessionBase&); // Not implemented

  vtkPVServerInformation* LocalServerInformation;
//ETX
};

#endif
