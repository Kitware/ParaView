/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionCore.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSessionCore
// .SECTION Description
// vtkPVSessionCore is used by vtkSMSession.
// vtkPVSessionCore handle the communication to MPI satellites and
// ServerImplementation code instanciation and execution.
// On the other hand, the vtkSMSession dispatch the request to the right
// process and therefore to the right vtkPVSessionCore instance.

#ifndef __vtkPVSessionCore_h
#define __vtkPVSessionCore_h

#include "vtkObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.
#include "vtkWeakPointer.h" // needed for vtkMultiProcessController

class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkCollection;
class vtkMPIMToNSocketConnection;
class vtkMultiProcessController;
class vtkPVInformation;
class vtkSIObject;
class vtkSIProxyDefinitionManager;

class VTK_EXPORT vtkPVSessionCore : public vtkObject
{
public:
  static vtkPVSessionCore* New();
  vtkTypeMacro(vtkPVSessionCore, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Provides access to the interpreter.
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);

  // Description:
  // Provides access to the proxy definition manager.
  vtkGetObjectMacro(ProxyDefinitionManager, vtkSIProxyDefinitionManager);

  // Description:
  // Push the state message.
  // This might forward the message to the MPI statellites if needed.
  virtual void PushState(vtkSMMessage* message);

  // Description:
  // Pull the state message from the local SI object instances.
  virtual void PullState(vtkSMMessage* message);

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
  virtual const vtkClientServerStream& GetLastResult();

  // Description:
  // Notify that the given SIObject is not used anymore .
  // This does not necessary delete the SIObject specially if this one is
  // used by other local SIObject. It only decrease its reference count.
  virtual void DeleteSIObject(vtkSMMessage* message);
  //ETX

  // Description:
  // Returns a vtkSIObject or subclass given its global id, if any otherwise
  // return NULL;
  vtkSIObject* GetSIObject(vtkTypeUInt32 globalid);

  // Description:
  // Return a vtkObject given its global id if any otherwise return NULL;
  vtkObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Register a remote object
  void RegisterRemoteObject(vtkTypeUInt32 globalid, vtkObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  virtual bool GatherInformation( vtkTypeUInt32 location,
                                  vtkPVInformation* information,
                                  vtkTypeUInt32 globalid );

  // Description:
  // Returns the number of processes. This simply calls the
  // GetNumberOfProcesses() on this->ParallelController
  int GetNumberOfProcesses();

  // Description:
  // Get/Set the socket connection used to communicate betweeen data=server and
  // render-server processes. This is valid only on data-server and
  // render-server processes.
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);
  vtkGetObjectMacro(MPIMToNSocketConnection, vtkMPIMToNSocketConnection);

//BTX
  enum MessageTypes
    {
    PUSH_STATE         = 12,
    PULL_STATE         = 13,
    EXECUTE_STREAM     = 14,
    GATHER_INFORMATION = 15,
    DELETE_SI          = 16
    };
  // Methods used to managed MPI satellite
  void PushStateSatelliteCallback();
  void ExecuteStreamSatelliteCallback();
  void GatherInformationStatelliteCallback();
  void DeleteSIObjectSatelliteCallback();

  // Description:
  // Allow the user to fill a vtkCollection with all RemoteObjects
  // This is usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);

protected:
  vtkPVSessionCore();
  ~vtkPVSessionCore();

  // Description:
  // This will create a vtkSIObject and/or execute some update on the
  // vtkObject that it own.
  virtual void PushStateInternal(vtkSMMessage*);

  // Description:
  // This will execute localy the given vtkClientServerStream either by
  // calling method on the vtkSIObject or its internal vtkObject.
  virtual void ExecuteStreamInternal( const vtkClientServerStream& stream,
                                      bool ignore_errors );

  // Description:
  // This will gather informations on the local vtkObjects
  // through the local vtkSIObjects.
  bool GatherInformationInternal( vtkPVInformation* information,
                                  vtkTypeUInt32 globalid );

  // Description:
  // Gather informations across MPI satellites.
  bool CollectInformation(vtkPVInformation*);

  // Description:
  // Decrement reference count of a local vtkSIObject. This might not result
  // in an actual deletion of the object if this one is used by another
  // SIObject.
  virtual void DeleteSIObjectInternal(vtkSMMessage* message);

  // Description:
  // Callback for reporting interpreter errors.
  void OnInterpreterError(vtkObject*, unsigned long, void* calldata);

  enum
    {
    ROOT_SATELLITE_RMI_TAG =  887822,
    ROOT_SATELLITE_INFO_TAG = 887823
    };

  vtkSIProxyDefinitionManager* ProxyDefinitionManager;
  vtkWeakPointer<vtkMultiProcessController> ParallelController;
  vtkClientServerInterpreter* Interpreter;
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

private:
  vtkPVSessionCore(const vtkPVSessionCore&); // Not implemented
  void operator=(const vtkPVSessionCore&);   // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
  bool SymmetricMPIMode;

  ostream *LogStream;
//ETX
};

#endif
