/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// This super class assumes the application is running all in one process
// with no MPI.

#ifndef __vtkPVProcessModule_h
#define __vtkPVProcessModule_h

#include "vtkKWObject.h"

#include "vtkClientServerID.h" // Needed for UniqueID ...

class vtkPolyData;
class vtkKWLoadSaveDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVInformation;
class vtkPVPart;
class vtkPVPartDisplay;
class vtkSource;
class vtkStringList;
class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;

class VTK_EXPORT vtkPVProcessModule : public vtkKWObject
{
public:
  static vtkPVProcessModule* New();
  vtkTypeRevisionMacro(vtkPVProcessModule,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.
  virtual int Start(int argc, char **argv);
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // Access to the subclass PVApplication store in the superclass
  // as a generic vtkKWApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // The controller is needed for filter that communicate internally.
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  //BTX
  // Description:
  // This is going to be a generic method of getting/gathering 
  // information form the server.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  //ETX
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);
  
  // Description:
  // Get the partition piece.  -1 means no assigned piece.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // Set the application instance for this class.
  virtual void SetApplication (vtkKWApplication* arg);
  
  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, int save);
  
  // Description:
  // Get a file selection dialog instance.
  virtual vtkKWLoadSaveDialog* NewLoadSaveDialog();
  // Description:
  // Get an object from an int id.  This is only useful in
  // when in client mode and calling this from tcl where vtkClientServerID
  // is not wrapped.
  virtual vtkObjectBase* GetObjectFromIntID(unsigned int);
//BTX  
  // Description:
  // Return the client server stream
  vtkClientServerStream& GetStream() 
    {
      return *this->ClientServerStream;
    }
  
  // Description: 
  // These methods construct/delete a vtk object in the vtkClientServerStream
  // owned by the process module.  The type of the object is specified by
  // string name, and the object to delete is specified by the object id
  // passed in.  To send the stream to the server call SendStreamToServer or
  // SendStreamToClientAndServer.  For construction, the unique id for the
  // new object is returned.
  vtkClientServerID NewStreamObject(const char*);
  void DeleteStreamObject(vtkClientServerID);
  
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
//ETX
  // Description:

  // Send the current vtkClientServerStream contents to the client only.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClient();

  // Send the current vtkClientServerStream contents to the server.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToServer();

  // Send the current vtkClientServerStream contents to the server
  // root node.  Also reset the vtkClientServerStream object.
  virtual void SendStreamToServerRoot();

  // Description:
  // Send current ClientServerStream data to the server and the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClientAndServer();

  // Description:
  // Send current ClientServerStream data to the server root and the client.
  // Also reset the vtkClientServerStream object.
  virtual void SendStreamToClientAndServerRoot();

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
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise.
  virtual int LoadModule(const char* name);

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name);

  vtkClientServerID GetUniqueID();
  vtkClientServerID GetApplicationID();
  vtkClientServerID GetProcessModuleID();
protected:
  vtkPVProcessModule();
  ~vtkPVProcessModule();

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
private:
  vtkPVProcessModule(const vtkPVProcessModule&); // Not implemented
  void operator=(const vtkPVProcessModule&); // Not implemented
};

#endif
