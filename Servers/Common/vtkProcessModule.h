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

class vtkAlgorithm;
class vtkMultiProcessController;
class vtkPVInformation;
class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkDataObject;
class vtkPVProgressHandler;
class vtkProcessObject;
class vtkProcessModuleGUIHelper;
class vtkPVOptions;
class vtkKWProcessStatistics;

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

  enum ExceptionEventEnum
    {
    EXCEPTION_EVENT_TAG = 31416,
    EXCEPTION_BAD_ALLOC = 31417,
    EXCEPTION_UNKNOWN   = 31418
    };

  static inline int GetRootId(int serverId)
    {
      if (serverId == ( DATA_SERVER | CLIENT) || serverId == ( RENDER_SERVER | CLIENT) || serverId == CLIENT_AND_SERVERS)
        {
        return CLIENT;
        }
      if (serverId > CLIENT)
        {
        vtkGenericWarningMacro("Server ID correspond to either data or "
                               "render server");
        return 0;
        }
      if (serverId == DATA_SERVER_ROOT || serverId == RENDER_SERVER_ROOT)
        {
        return serverId;
        }
      if (serverId == CLIENT)
        {
        return CLIENT;
        }
      return serverId << 1;
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
  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.
  virtual int Start(int, char **) = 0;
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit() = 0;

  // Description:
  // These methods append commands to the given vtkClientServerStream
  // to construct or delete a vtk object.  For construction, the type
  // of the object is specified by string name and the new unique
  // object id is returned.  For deletion the object is specified by
  // its id.  These methods do not send the stream anywhere, so the
  // caller must use SendStream() to actually perform the operation.
  vtkClientServerID NewStreamObject(const char*, vtkClientServerStream& stream);
  void DeleteStreamObject(vtkClientServerID, vtkClientServerStream& stream);

  // Description:
  // Return the vtk object associated with the given id for the
  // client.  If the id is for an object on another node then 0 is
  // returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);

  // Description:
  // Return the last result for the specified server.  In this case,
  // the server should be exactly one of the ServerFlags, and not a
  // combination of servers.  For an MPI server the result from the
  // root node is returned.  There is no connection to the individual
  // nodes of a server.
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 server);

  // Description:
  // Send a vtkClientServerStream to the specified servers.  Servers
  // are specified with a bit vector.  To send to more than one server
  // use the bitwise or operator to combine servers.  The resetStream
  // flag determines if Reset is called to clear the stream after it
  // is sent.
  int SendStream(vtkTypeUInt32 server, vtkClientServerStream& stream,
                 int resetStream=1);

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
  // Initialize and finalize process module.
  void Initialize();
  void Finalize();

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
  void RegisterProgressEvent(vtkObject* po, int id);

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
  virtual void ExecuteEvent(vtkObject *o, unsigned long event, void* calldata);

  //BTX
  // Description:
  // Get the observer.
  vtkCommand* GetObserver();
  //ETX

  // Description:
  // Set the local progress. Subclass should overwrite it.
  virtual void SetLocalProgress(const char* filter, int progress) = 0;
  vtkGetMacro(ProgressRequests, int);
  vtkSetMacro(ProgressRequests, int);
  vtkGetObjectMacro(ProgressHandler, vtkPVProgressHandler);

  // Description:
  vtkSetMacro(ProgressEnabled, int);
  vtkGetMacro(ProgressEnabled, int);
  
  // Description:
  // Set and get the application options
  vtkGetObjectMacro(Options, vtkPVOptions);
  virtual void SetOptions(vtkPVOptions* op);

  // Description:
  // Set the gui helper
  void SetGUIHelper(vtkProcessModuleGUIHelper*);

  // Description:
  // Get a pointer to the log file.
  ofstream* GetLogFile();

  virtual void CreateLogFile();

//BTX
  enum CommunicationIds
  {
    MultiDisplayDummy=948346,
    MultiDisplayRootRender,
    MultiDisplaySatelliteRender,
    MultiDisplayInfo,
    PickBestProc,
    PickBestDist2,
    IceTWinInfo,
    IceTNumTilesX,
    IceTNumTilesY,
    IceTTileRanks,
    IceTRenInfo,
    GlyphNPointsGather,
    GlyphNPointsScatter,
    TreeCompositeDataFlag,
    TreeCompositeStatus,
    DuplicatePDNProcs,
    DuplicatePDNRecLen,
    DuplicatePDNAllBuffers,
    IntegrateAttrInfo,
    IntegrateAttrData,
    PickMakeGIDs,
    TemporalPickHasData,
    TemporalPicksData
  };
//ETX

  // Description:
  // Internal method- called when an exception Tag is received 
  // from the server.
  void ExceptionEvent(const char* message);

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

  // Description:
  // Get the last result from the DataServer, RenderServer or Client.
  // If these are MPI processes, only the root last result is returned.
  virtual const vtkClientServerStream& GetLastDataServerResult();
  virtual const vtkClientServerStream& GetLastRenderServerResult();
  virtual const vtkClientServerStream& GetLastClientResult();
  
  
  static void InterpreterCallbackFunction(vtkObject* caller,
                                          unsigned long eid,
                                          void* cd, void* d);
  virtual void InterpreterCallback(unsigned long eid, void*);

  virtual const char* DetermineLogFilePrefix() { return "NodeLog"; }

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
  int ProgressEnabled;
  int AbortCommunication;

  vtkProcessModuleObserver* Observer;
  vtkPVOptions* Options;
  vtkProcessModuleGUIHelper* GUIHelper;
  ofstream *LogFile;

  vtkKWProcessStatistics *MemoryInformation;

private:
  vtkProcessModule(const vtkProcessModule&); // Not implemented
  void operator=(const vtkProcessModule&); // Not implemented
};

#endif
