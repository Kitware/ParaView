/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// This super class assumes the application is running all in one process
// with no MPI.

#ifndef __vtkProcessModule_h
#define __vtkProcessModule_h

#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed for UniqueID ...

class vtkMultiProcessController;
class vtkPVInformation;
class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkDataObject;
class vtkPVProgressHandler;
class vtkProcessObject;
//BTX
struct vtkProcessModuleInternals;
//ETX

class vtkProcessModuleObserver;

class VTK_EXPORT vtkProcessModule : public vtkObject
{
public:
//BTX
  // Description: 
  // These flags are used to specify destination servers for the
  // SendStream function. 
  enum ServerFlags
  {
    DATA_SERVER = 0x1,
    DATA_SERVER_ROOT = 0x2,
    RENDER_SERVER = 0x4,
    RENDER_SERVER_ROOT = 0x8,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
  };

  enum ProgressEventEnum
    {
    PROGRESS_EVENT_TAG = 31415
    };

  static inline ServerFlags GetRootId(ServerFlags serverId)
    {
      if (serverId > RENDER_SERVER)
        {
        vtkGenericWarningMacro("Server ID correspond to either data or "
                               "render server");
        return  static_cast<ServerFlags>(0);
        }
      return static_cast<ServerFlags>(serverId << 1);
    }
//ETX
  
  vtkTypeRevisionMacro(vtkProcessModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Returns a data object of the given type. This is a utility
  // method used to increase performance. The first time the
  // data object of a given type is requested, it is instantiated
  // and put in map. The following calls do not cause instantiation.
  // Used while comparing data types for input matching.
  vtkDataObject* GetDataObjectOfType(const char* classname);

  // Description:
  // This is going to be a generic method of getting/gathering 
  // information form the server.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  // Description:
  // Same as GatherInformation but use render server.
  virtual void GatherInformationRenderServer(vtkPVInformation* info,
                                             vtkClientServerID id);
  //ETX
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);
  
//BTX  
  // Description:
  // Return the client server stream
  vtkClientServerStream& GetStream() 
    {
      return *this->ClientServerStream;
    }

  // Description:
  // Return the client server stream
  vtkClientServerStream* GetStreamPointer() 
    {
      return this->ClientServerStream;
    }
  
  // Description: 
  // These methods construct/delete a vtk object in the vtkClientServerStream
  // owned by the process module.  The type of the object is specified by
  // string name, and the object to delete is specified by the object id
  // passed in.  To send the stream to the server call SendStreamToServer or
  // SendStreamToClientAndServer.  For construction, the unique id for the
  // new object is returned.
  vtkClientServerID NewStreamObject(const char*);
  vtkClientServerID NewStreamObject(const char*, vtkClientServerStream& stream);
  void DeleteStreamObject(vtkClientServerID);
  void DeleteStreamObject(vtkClientServerID, vtkClientServerStream& stream);
  
  // Description:
  // Return the vtk object associated with the given id for the client.
  // If the id is for an object on the server then 0 is returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);
  
  // Description:
  // Return a message containing the result of the last SendMessages call.
  // In client/server mode this causes a round trip to the server.
  virtual const vtkClientServerStream& GetLastServerResult();

  // Description:
  // Return a message containing the result of the last call made on
  // the client.
  virtual const vtkClientServerStream& GetLastClientResult();
  
   // Description:
  // Send a vtkClientServerStream to the specified servers.
  // Servers are specified with a bit vector.   To send to more
  // than one server use the bitwise or operator to combine servers.
  // The stream can either be passed in or the current stream will
  // be used.  If the current stream is used, then the stream is cleared
  // after the call.  If a stream is passed the resetStream flag determines
  // if Reset is called on the stream after it is sent.
  int SendStream(vtkTypeUInt32 server);
  int SendStream(vtkTypeUInt32 server, vtkClientServerStream&, int resetStream=1);
//ETX

  
//BTX
  virtual void SendStreamToServerTemp(vtkClientServerStream* stream);
  virtual void SendStreamToServerRootTemp(vtkClientServerStream* stream);
//ETX

  //BTX
  // Description:
  // Get the interpreter used on the local process.
  virtual vtkClientServerInterpreter* GetInterpreter();

  // Description:
  // Initialize/Finalize the process module's
  // vtkClientServerInterpreter.
  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();
  //ETX

  // Description:
  // Set/Get whether to report errors from the Interpreter.
  vtkGetMacro(ReportInterpreterErrors, int);
  vtkSetMacro(ReportInterpreterErrors, int);
  vtkBooleanMacro(ReportInterpreterErrors, int);

  // Description:
  // The controller is needed for filter that communicate internally.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId() { return 0;} ;

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions() { return 1;} ;
  
  vtkClientServerID GetUniqueID();
  vtkClientServerID GetProcessModuleID();

  static vtkProcessModule* GetProcessModule();
  static void SetProcessModule(vtkProcessModule* pm);

  // Description:
  // Register object with progress handler.
  void RegisterProgressEvent(vtkProcessObject* po, int id);

  // Description:
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();

  // Description:
  // This method is called before progress reports start comming.
  void PrepareProgress();

  // Description:
  // This method is called after force update to clenaup all the pending
  // progresses.
  void CleanupPendingProgress();

  // Description:
  // Execute event on callback
  void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);

  // Description:
  // Get the observer.
  vtkCommand* GetObserver();

  // Description:
  // Set the local progress. Subclass should overwrite it.
  virtual void SetLocalProgress(const char* filter, int progress) = 0;
  vtkGetMacro(ProgressRequests, int);
  vtkSetMacro(ProgressRequests, int);
  vtkGetObjectMacro(ProgressHandler, vtkPVProgressHandler);
protected:
  vtkProcessModule();
  ~vtkProcessModule();

  // Description:
  // Given the servers that need to receive the stream, create a flag
  // that will send it to the correct places for this process module and
  // make sure it only gets sent to each server once.
  virtual vtkTypeUInt32 CreateSendFlag(vtkTypeUInt32 servers);
  // send a stream to the client
  virtual int SendStreamToClient(vtkClientServerStream&);
  // send a stream to the data server
  virtual int SendStreamToDataServer(vtkClientServerStream&);
  // send a stream to the data server root mpi process
  virtual int SendStreamToDataServerRoot(vtkClientServerStream&);
  // send a stream to the render server
  virtual int SendStreamToRenderServer(vtkClientServerStream&);
  // send a stream to the render server root mpi process
  virtual int SendStreamToRenderServerRoot(vtkClientServerStream&);

  
  
  static void InterpreterCallbackFunction(vtkObject* caller,
                                          unsigned long eid,
                                          void* cd, void* d);
  virtual void InterpreterCallback(unsigned long eid, void*);

  vtkMultiProcessController *Controller;
  vtkPVInformation *TemporaryInformation;

  vtkClientServerInterpreter* Interpreter;
  vtkClientServerStream* ClientServerStream;
  vtkClientServerID UniqueID;
  vtkCallbackCommand* InterpreterObserver;
  int ReportInterpreterErrors;

  static vtkProcessModule* ProcessModule;

  vtkProcessModuleInternals* Internals;

  void ProgressEvent(vtkObject *o, int val, const char* filter);

  vtkPVProgressHandler* ProgressHandler;
  int ProgressRequests;

  vtkProcessModuleObserver* Observer;

private:
  vtkProcessModule(const vtkProcessModule&); // Not implemented
  void operator=(const vtkProcessModule&); // Not implemented
};

#endif
