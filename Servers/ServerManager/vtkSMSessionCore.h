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
// .NAME vtkSMSessionCore
// .SECTION Description
// vtkSMSessionCore is used by vtkSMSession. It encapsulates the
// vtkClientServerInterpreter as well as the vtkPMObject map.

#ifndef __vtkSMSessionCore_h
#define __vtkSMSessionCore_h

#include "vtkObject.h"
#include "vtkSMMessage.h"

class vtkClientServerInterpreter;
class vtkMultiProcessController;
class vtkPMObject;
class vtkSMProxyDefinitionManager;
class vtkPVInformation;
class vtkSMRemoteObject;
class vtkCollection;

class VTK_EXPORT vtkSMSessionCore : public vtkObject
{
public:

  // Description:
  // Enable or Disable logging into file the message communications
  static void SetDebugLogging(bool enable)
    {
    vtkSMSessionCore::WriteDebugLog = enable;
    }

  static vtkSMSessionCore* New();
  vtkTypeMacro(vtkSMSessionCore, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Provides access to the interpreter.
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);

  // Description:
  // Provides access to the proxy definition manager.
  vtkGetObjectMacro(ProxyDefinitionManager, vtkSMProxyDefinitionManager);

  // Description:
  // Push the state message.
  virtual void PushState(vtkSMMessage* message);

  // Description:
  // Pull the state message.
  virtual void PullState(vtkSMMessage* message);

  // Description:
  // Invoke a method remotely
  virtual void Invoke(vtkSMMessage* message);

  // Description:
  // Invoke a method remotely
  virtual void DeletePMObject(vtkSMMessage* message);
  //ETX

  // Description:
  // Returns a vtkPMObject or subclass given its global id, if any.
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalid);

  // Description:
  // Return a vtkSMRemoteObject given its global id if any otherwise return NULL;
  vtkSMRemoteObject* GetRemoteObject(vtkTypeUInt32 globalid);

  // Description:
  // Register a remote object
  void RegisterRemoteObject(vtkSMRemoteObject* obj);

  // Description:
  // Unregister a remote object
  void UnRegisterRemoteObject(vtkSMRemoteObject* obj);

  // Description:
  // Gather information about an object referred by the \c globalid.
  // \c location identifies the processes to gather the information from.
  virtual bool GatherInformation(vtkTypeUInt32 location,
    vtkPVInformation* information, vtkTypeUInt32 globalid);

  // Description:
  // Returns the number of processes. This simply calls the
  // GetNumberOfProcesses() on this->ParallelController
  int GetNumberOfProcesses();

//BTX
  enum MessageTypes
    {
    PUSH_STATE   = 12,
    PULL_STATE   = 13,
    INVOKE_STATE = 14,
    GATHER_INFORMATION = 15
    };
  void PushStateSatelliteCallback();
  void InvokeSatelliteCallback();
  void GatherInformationStatelliteCallback();

  // Description:
  // Allow the user to fill its vtkCollection with all RemoteObject
  // This could be usefull when you want to hold a reference to them to
  // prevent any deletion across several method call.
  virtual void GetAllRemoteObjects(vtkCollection* collection);

protected:
  vtkSMSessionCore();
  ~vtkSMSessionCore();

  // Description:
  virtual void PushStateInternal(vtkSMMessage*);
  virtual void InvokeInternal(vtkSMMessage*);
  bool GatherInformationInternal(
    vtkPVInformation* information, vtkTypeUInt32 globalid);
  bool CollectInformation(vtkPVInformation*);


  // Description:
  // Callback for reporting interpreter errors.
  void OnInterpreterError(vtkObject*, unsigned long, void* calldata);

  enum
    {
    ROOT_SATELLITE_RMI_TAG =  887822,
    ROOT_SATELLITE_INFO_TAG = 887823
    };

  vtkSMProxyDefinitionManager* ProxyDefinitionManager;
  vtkMultiProcessController* ParallelController;
  vtkClientServerInterpreter* Interpreter;
private:
  vtkSMSessionCore(const vtkSMSessionCore&); // Not implemented
  void operator=(const vtkSMSessionCore&);   // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

  static bool WriteDebugLog;
//ETX
};

#endif
