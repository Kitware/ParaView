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
    CLIENT = 0x2,
    RENDER_SERVER = 0x4,
    DATA_SERVER_ROOT = 0x8,
    RENDER_SERVER_ROOT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
  };
//ETX
  
  static vtkProcessModule* New();
  vtkTypeRevisionMacro(vtkProcessModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
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

  
  // Description:
  // Send the current ClientServerStream data to different places and 
  // combinations of places.  Possible places are the Client, the 
  // Server (data server), or the RenderServer.  Also the stream
  // can be sent to the root of the render and data servers.
  // Most combinations are possible.
  virtual void SendStreamToClient();
  virtual void SendStreamToServer();
  virtual void SendStreamToRenderServer();
  virtual void SendStreamToServerRoot();
  virtual void SendStreamToRenderServerRoot(); 
  virtual void SendStreamToClientAndServerRoot();
  virtual void SendStreamToRenderServerAndServerRoot();
  virtual void SendStreamToClientAndRenderServerRoot(); 
  virtual void SendStreamToClientAndServer();
  virtual void SendStreamToClientAndRenderServer();
  virtual void SendStreamToRenderServerAndServer();
  virtual void SendStreamToRenderServerClientAndServer();
//BTX
  virtual void SendStreamToServerTemp(vtkClientServerStream* stream);
  virtual void SendStreamToServerRootTemp(vtkClientServerStream* stream);
//ETX

  // Description:
  // Send the stream represented by the given string to the client,
  // server, or both.  This should not be called by C++ code and is
  // provided only for debugging and testing purposes.  Returns 1 if
  // the string is successfully parsed and 0 otherwise.
  virtual int SendStringToClient(const char*);
  virtual int SendStringToClientAndServer(const char*);
  virtual int SendStringToClientAndServerRoot(const char*);
  virtual int SendStringToServer(const char*);
  virtual int SendStringToServerRoot(const char*);

  // Description:
  // Get a result stream represented by a string.  This should not be
  // called by C++ code and is provided only for debugging and testing
  // purposes.
  virtual const char* GetStringFromServer();
  virtual const char* GetStringFromClient();


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

  vtkClientServerID GetUniqueID();
  vtkClientServerID GetProcessModuleID();

  static vtkProcessModule* GetProcessModule();
  static void SetProcessModule(vtkProcessModule* pm);

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

private:
  vtkProcessModule(const vtkProcessModule&); // Not implemented
  void operator=(const vtkProcessModule&); // Not implemented
};

#endif
