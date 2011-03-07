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
// .NAME vtkPVSessionCore
// .SECTION Description
// vtkPVSessionCore is used by vtkSMSession. In handles processing of
// communication locally as well as with satellites (in case on MPI processes). 

#ifndef __vtkPVSessionCore_h
#define __vtkPVSessionCore_h

#include "vtkObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.

class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkCollection;
class vtkMPIMToNSocketConnection;
class vtkMultiProcessController;
class vtkSIObject;
class vtkPVInformation;
class vtkPVProxyDefinitionManager;

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
  vtkGetObjectMacro(ProxyDefinitionManager, vtkPVProxyDefinitionManager);

  // Description:
  // Push the state message.
  virtual void PushState(vtkSMMessage* message);

  // Description:
  // Pull the state message.
  virtual void PullState(vtkSMMessage* message);

  // Description:
  // Execute a command on the given processes. Use GetLastResult() to obtain the
  // last result after the command stream is evaluated. Once can set
  // \c ignore_errors to true, to ignore any interpreting errors.
  virtual void ExecuteStream(
    vtkTypeUInt32 location, const vtkClientServerStream& stream,
    bool ignore_errors=false);

  // Description:
  // Returns the response of the ExecuteStream() call from the location. Note if
  // location refers to multiple processes, then the reply is only fetched from
  // the "closest" process.
  virtual const vtkClientServerStream& GetLastResult();

  // Description:
  // Invoke a method remotely
  virtual void DeleteSIObject(vtkSMMessage* message);
  //ETX

  // Description:
  // Returns a vtkSIObject or subclass given its global id, if any.
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
  virtual bool GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid);

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
    PUSH_STATE   = 12,
    PULL_STATE   = 13,
    EXECUTE_STREAM = 14,
    GATHER_INFORMATION = 15,
    DELETE_SI=16
    };
  void PushStateSatelliteCallback();
  void ExecuteStreamSatelliteCallback();
  void GatherInformationStatelliteCallback();
  void DeleteSIObjectSatelliteCallback();

  // Description:
  // Allow the user to fill its vtkCollection with all RemoteObject
  // This could be usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);

protected:
  vtkPVSessionCore();
  ~vtkPVSessionCore();

  // Description:
  virtual void PushStateInternal(vtkSMMessage*);
  virtual void ExecuteStreamInternal(
    const vtkClientServerStream& stream, bool ignore_errors);
  bool GatherInformationInternal(
    vtkPVInformation* information, vtkTypeUInt32 globalid);
  bool CollectInformation(vtkPVInformation*);

  virtual void DeleteSIObjectInternal(vtkSMMessage* message);

  // Description:
  // Callback for reporting interpreter errors.
  void OnInterpreterError(vtkObject*, unsigned long, void* calldata);

  enum
    {
    ROOT_SATELLITE_RMI_TAG =  887822,
    ROOT_SATELLITE_INFO_TAG = 887823
    };

  vtkPVProxyDefinitionManager* ProxyDefinitionManager;
  vtkMultiProcessController* ParallelController;
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
