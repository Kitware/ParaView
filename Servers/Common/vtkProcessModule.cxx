/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkProcessModule.h"

#include "vtkAlgorithm.h"
#include "vtkCacheSizeKeeper.h"
#include "vtkCallbackCommand.h"
#include "vtkClientServerID.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkConnectionIterator.h"
#include "vtkDataObject.h"
#include "vtkInstantiator.h"
#include "vtkKWProcessStatistics.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVPaths.h"
#include "vtkPVProgressHandler.h"
#include "vtkPVServerInformation.h"
#include "vtkPVServerOptions.h"
#include "vtkServerConnection.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkStdString.h"
#include "vtkStringList.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h" // For VTK_USE_MPI

#ifdef VTK_USE_MPI
# include "vtkMPIController.h"
#endif

#include <vtkstd/map>
#include <vtkstd/new>
#include <vtkstd/vector>

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>
#include <vtksys/ios/sstream>

vtkProcessModule* vtkProcessModule::ProcessModule = 0;
ostream* vtkProcessModule::DebugLogStream = 0;

//*****************************************************************************
class vtkProcessModuleInternals
{
public:
  typedef 
  vtkstd::map<vtkStdString, vtkSmartPointer<vtkDataObject> > DataTypesType;
  typedef vtkstd::map<vtkStdString, vtkStdString> MapStringToString;
  
  DataTypesType DataTypes;
  MapStringToString Paths;
  
  // Flag indicating on which servers the SendPrepareProgress
  // was sent. This is used to determine where to send the
  // CleanupPendingProgress request.
  vtkTypeUInt32 ProgressServersFlag;
};

//*****************************************************************************
class vtkProcessModule::vtkInterpreterInitializationCallbackVector : 
  public vtkstd::vector<vtkProcessModule::InterpreterInitializationCallback>
{
};

//*****************************************************************************
// Used to clean up the vtkProcessModule::InitializationCallbacks vector.
class vtkProcessModuleCleanup
{
public:
  inline void Use()
    {
    }
  ~vtkProcessModuleCleanup()
    {
    delete vtkProcessModule::InitializationCallbacks;
    vtkProcessModule::InitializationCallbacks = 0;
    }

};

vtkProcessModule::vtkInterpreterInitializationCallbackVector* vtkProcessModule::InitializationCallbacks = 0;
static vtkProcessModuleCleanup vtkProcessModuleCleanupGlobal;

//*****************************************************************************
class vtkProcessModuleObserver : public vtkCommand
{
public:
  static vtkProcessModuleObserver* New()
    { return new vtkProcessModuleObserver; }

  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
    if (this->ProcessModule)
      {
      this->ProcessModule->ExecuteEvent(wdg, event, calldata);
      }
    this->AbortFlagOn();
    }

  void SetProcessModule(vtkProcessModule* pm)
    {
    this->ProcessModule = pm;
    }

protected:
  vtkProcessModuleObserver()
    {
    this->ProcessModule = 0;
    }
  vtkProcessModule* ProcessModule;
};
//*****************************************************************************


vtkStandardNewMacro(vtkProcessModule);
vtkCxxSetObjectMacro(vtkProcessModule, ActiveRemoteConnection, vtkRemoteConnection);
vtkCxxSetObjectMacro(vtkProcessModule, GUIHelper, vtkProcessModuleGUIHelper);

//-----------------------------------------------------------------------------
vtkProcessModule::vtkProcessModule()
{
  vtkProcessModuleCleanupGlobal.Use();

  this->Internals = new vtkProcessModuleInternals;
  this->ConnectionManager = 0;
  this->Interpreter = 0;
  
  this->Observer = vtkProcessModuleObserver::New();
  this->Observer->SetProcessModule(this);

  this->InterpreterObserver = 0;
  this->ReportInterpreterErrors = 1;

  this->UniqueID.ID = 3;

  this->ProgressRequests = 0;

  this->Options = 0;
  this->GUIHelper = 0;

  this->LogFile = 0;
  this->LogThreshold = 0;
  this->Timer = vtkTimerLog::New();

  this->ActiveRemoteConnection = 0 ;

  this->SupportMultipleConnections = 0;
  this->DisableNewConnections = false;
  this->ExceptionRaised = 0;
  
  this->MemoryInformation = vtkKWProcessStatistics::New();

  this->ServerInformation = vtkPVServerInformation::New();

  this->UseMPI = 1;
  this->SendStreamToClientOnly = 0;

  this->CacheSizeKeeper = vtkCacheSizeKeeper::New();

  this->LastConnectionID = -1;

  this->LastProgress = -1;
  this->LastProgressName = 0;

#ifdef VTK_USE_MPI
# ifdef PARAVIEW_USE_MPI_SSEND
  // ParaView uses Ssend for all Trigger RMI calls. This helps in overcoming
  // bufferring issues with send on some implementations.
  vtkMPIController::SetUseSsendForRMI(1);
# endif
#endif

  vtkMapper::SetResolveCoincidentTopologyToShiftZBuffer();
  vtkMapper::SetResolveCoincidentTopologyZShift(2.0e-3);
}

//-----------------------------------------------------------------------------
vtkProcessModule::~vtkProcessModule()
{
  this->SetActiveRemoteConnection(0);
  this->Observer->SetProcessModule(0);
  this->Observer->Delete();

  if (this->ConnectionManager)
    {
    this->ConnectionManager->Delete();
    this->ConnectionManager = 0;
    }
  this->FinalizeInterpreter();
  delete this->Internals;

  if (this->InterpreterObserver)
    {
    this->InterpreterObserver->Delete();
    this->InterpreterObserver = 0;
    }

  this->SetOptions(0);
  this->SetGUIHelper(0);

  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    this->LogFile = 0;
    }

  this->Timer->Delete();
  this->MemoryInformation->Delete();
  this->ServerInformation->Delete();

  this->CacheSizeKeeper->Delete();
  this->SetLastProgressName(0);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetOptions(vtkPVOptions* op)
{
  this->Options = op;
}

//-----------------------------------------------------------------------------
vtkDataObject* vtkProcessModule::GetDataObjectOfType(const char* classname)
{
  if (!classname)
    {
    return 0;
    }

  // Since we can not instantiate these classes, we'll replace
  // them with a subclass
  if (strcmp(classname, "vtkDataSet") == 0)
    {
    classname = "vtkImageData";
    }
  else if (strcmp(classname, "vtkPointSet") == 0)
    {
    classname = "vtkPolyData";
    }
  else if (strcmp(classname, "vtkCompositeDataSet") == 0)
    {
    classname = "vtkHierarchicalDataSet";
    }

  vtkProcessModuleInternals::DataTypesType::iterator it =
    this->Internals->DataTypes.find(classname);
  if (it != this->Internals->DataTypes.end())
    {
    return it->second.GetPointer();
    }

  vtkObject* object = vtkInstantiator::CreateInstance(classname);
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);
  if (!dobj)
    {
    if (object)
      {
      object->Delete();
      }
    return 0;
    }

  this->Internals->DataTypes[classname] = dobj;
  dobj->Delete();
  return dobj;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::GatherInformation(vtkIdType connectionID,
  vtkTypeUInt32 serverFlags, vtkPVInformation* info, vtkClientServerID id)
{
  vtkIdType rootId = 
    vtkProcessModuleConnectionManager::GetRootConnection(connectionID);
  this->ConnectionManager->GatherInformation(rootId, serverFlags, info, id);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::Start(int argc, char** argv)
{
  // This is the place where we set up the ConnectionManager.
  if (this->ConnectionManager)
    {
    vtkErrorMacro("Duplicate call to Start.");
    return 1;
    }

  this->ConnectionManager = vtkProcessModuleConnectionManager::New();
  this->ConnectionManager->AddObserver(vtkCommand::AbortCheckEvent,
    this->Observer);
  this->ConnectionManager->AddObserver(vtkCommand::ConnectionCreatedEvent,
    this->Observer);
  this->ConnectionManager->AddObserver(vtkCommand::ConnectionClosedEvent,
    this->Observer);

  int myId;
  // This call blocks on the Satellite nodes (never on root node).
  if (this->ConnectionManager->Initialize(argc, argv, 
      this->Options->GetClientMode(), &myId) != 0)
    {
    return 1;
    }


  if (myId == 0)
    {
    // Should only be called on root nodes.
    int ret = this->InitializeConnections();
    if (!ret)
      {
      // Failed.
      return 1;
      }
    }

  if (this->Options->GetClientMode() || 
    (!this->Options->GetServerMode() && !this->Options->GetRenderServerMode()))
    {
    // Starts the client event loop.
    return this->StartClient(argc, argv);
    }

  // if (this->GetPartitionId()==0)
  //  {
  //  ofstream *fp = new ofstream("/tmp/pvdebug.log");
  //  this->InitializeDebugLog(*fp);
  //  }

  // Running in server mode.
  // StartServer() needs to be called only on the root node.
  return (this->GetPartitionId() == 0)? this->StartServer(0) : 0;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::StartClient(int argc, char** argv)
{
  if (!this->GUIHelper)
    {
    vtkErrorMacro("GUIHelper must be set on the client.");
    return 1;
    }  

  if (!this->SupportMultipleConnections)
    {
    // Before the Client event loop is started, ParaView needs a server connection.
    // Note that this method is called in Client mode or when the paraview
    // executable in run. We don't need a server connection in the latter case.
    if (this->Options->GetClientMode())
      {
      if (this->ShouldWaitForConnection())
        {
        // Wait for the server(s) to connect. (Ofcouse servers means 1
        // data server and 1 render server if running in render client mode.).
        if (!this->ClientWaitForConnection())
          {
          vtkErrorMacro("Could not connect to server(s). Exiting.");
          return 1;
          }
        // Now, we dont want the client to receive any more connections,
        // so close the socket that accepts new connections.
        this->ConnectionManager->StopAcceptingAllConnections();
        }
      else
        {
        // Connect to the server(s). (Ofcouse servers means 1
        // data server and 1 render server if running in render client mode.).
        if (!this->ConnectToRemote())
          {
          // failed!
          return 1;
          }
        }
      }

    // Since we don't support mutiple connections, the connection must have been
    // established by now. Disable any new connection requests.
    this->DisableNewConnections=true;
    }

  // fill up ServerInformation with local information. ServerInformation is only used
  // when there's no remote server.
  this->ServerInformation->CopyFromObject(this);

  // if the PM supports multiple connections, its the responsibility of the GUI
  // to connect to the server.
  return this->GUIHelper->RunGUIStart(argc, argv,
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses(),
    vtkMultiProcessController::GetGlobalController()->GetLocalProcessId());
}

//-----------------------------------------------------------------------------
int vtkProcessModule::StartServer(unsigned long msec)
{
  // Observe errors on the server side so that they can be processed to
  // detect out of memory errors.
  vtkOutputWindow::GetInstance()->AddObserver(vtkCommand::ErrorEvent,
    this->Observer);
  // Running in server mode.
  int ret = 0;
  int support_multiple_connections = this->SupportMultipleConnections;
  if (this->ShouldWaitForConnection())
    {
    cout << "Waiting for client..." << endl;
    }
  else
    {
    // Server in reverse connect mode, connect to the client.
    // When in reverse connect mode, the server can connect to 
    // one and only one client. 
    // When the client disconnects, the server terminates.
    if (!this->ConnectToRemote())
      {
      // failed -- in reverse connect mode, the client must be 
      // up and running before the server tries to connect.
      return 1;
      }
    support_multiple_connections = 0;
    }

  while (!this->ExceptionRaised && 
    (ret = this->ConnectionManager->MonitorConnections(msec)) >= 0)
    {
    if (ret == 2)
      {
      cout << "Client connected." << endl;
      if (!support_multiple_connections)
        {
        // Don't accept any more connections.
        this->ConnectionManager->StopAcceptingAllConnections();
        }
      }

    else if (ret == 3)
      {
      // Connection dropped.
      cout << "Client connection closed." << endl;
      if (!support_multiple_connections)
        {
        // since supporting only one connection, server can do nothing but exit.
        ret = 0;
        break;
        }
      }
    }

  return (ret==-1)? 1 : 0;
}


//-----------------------------------------------------------------------------
int vtkProcessModule::InitializeConnections()
{
  // Detemine if this process supports connections.
  switch (this->Options->GetProcessType())
    {
  case vtkPVOptions::PARAVIEW:
  case vtkPVOptions::PVBATCH:
  case vtkPVOptions::XMLONLY:
  case vtkPVOptions::ALLPROCESS:
    return 1; // nothing to do here.
    }
  
  if (this->ShouldWaitForConnection())
    {
    return this->SetupWaitForConnection();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::ShouldWaitForConnection()
{
  // if client mode then return reverse connection
  if(this->Options->GetClientMode())
    {
    // if in client mode, it should not wait for a connection
    // unless reverse is 1, so just return reverse connection value
    return this->Options->GetReverseConnection();
    }
  // if server mode, then by default wait for the connection
  // so return not getreverseconnection
  return !this->Options->GetReverseConnection();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::SetupWaitForConnection()
{
  int port = 0;
  switch (this->Options->GetProcessType())
    {
  case vtkPVOptions::PVCLIENT:
    // Check if we wait 2 separate connections (only client in render server mode
    // waits for 2 connections).
    if (this->Options->GetRenderServerMode())
      {
      int ret = this->ConnectionManager->AcceptConnectionsOnPort(
        this->Options->GetDataServerPort(),
        vtkProcessModuleConnectionManager::DATA_SERVER);
      if (ret == -1)
        {
        return 0;
        }
      ret = this->ConnectionManager->AcceptConnectionsOnPort(
        this->Options->GetRenderServerPort(),
        vtkProcessModuleConnectionManager::RENDER_SERVER);
      if (ret == -1)
        {
        return 0;
        }
      cout << "Listen on render server port:" << 
        this->Options->GetRenderServerPort() << endl;
      cout << "Listen on data server port:" <<
        this->Options->GetDataServerPort() << endl;
      return 1; // success.
      }
    else
      {
      port = this->Options->GetServerPort();
      }
    break;
    
  case vtkPVOptions::PVSERVER:
    port = this->Options->GetServerPort();
    break;

  case vtkPVOptions::PVRENDER_SERVER:
    port = this->Options->GetRenderServerPort();
    break;

  case vtkPVOptions::PVDATA_SERVER:
    port = this->Options->GetDataServerPort();
    break;

  default:
    return 0;
    }
  
  cout << "Listen on port: " << port << endl;
  int ret = this->ConnectionManager->AcceptConnectionsOnPort(
    port, vtkProcessModuleConnectionManager::RENDER_AND_DATA_SERVER);
  if (this->Options->GetRenderServerMode())
    {
    cout << "RenderServer: ";
    }
  return (ret == -1)? 0 : 1;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::AcceptConnectionsOnPort(int port)
{
  return this->ConnectionManager->AcceptConnectionsOnPort(
    port, vtkProcessModuleConnectionManager::RENDER_AND_DATA_SERVER);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::AcceptConnectionsOnPort(int data_server_port, 
  int render_server_port, int &ds_id, int &rs_id)
{
  ds_id = this->ConnectionManager->AcceptConnectionsOnPort(
    data_server_port, vtkProcessModuleConnectionManager::DATA_SERVER);
  rs_id = this->ConnectionManager->AcceptConnectionsOnPort(
    render_server_port, vtkProcessModuleConnectionManager::RENDER_SERVER);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::StopAcceptingAllConnections()
{
  this->ConnectionManager->StopAcceptingAllConnections();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::StopAcceptingConnections(int id)
{
  this->ConnectionManager->StopAcceptingConnections(id);
}

//-----------------------------------------------------------------------------
bool vtkProcessModule::IsAcceptingConnections()
{
  return this->ConnectionManager->IsAcceptingConnections();
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModule::ConnectToRemote(const char* servername, int port)
{
  if (this->DisableNewConnections)
    {
    vtkErrorMacro("Cannot create new connections.");
    return 0;
    }
  return this->ConnectionManager->OpenConnection(servername, port);
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModule::ConnectToRemote(const char* dataserver_host, 
  int dataserver_port, const char* renderserver_host, int renderserver_port)
{
  if (this->DisableNewConnections)
    {
    vtkErrorMacro("Cannot create new connections.");
    return 0;
    }
  return this->ConnectionManager->OpenConnection(
    dataserver_host, dataserver_port, renderserver_host, renderserver_port);
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModule::ConnectToSelf()
{
  if (this->DisableNewConnections)
    {
    vtkErrorMacro("Cannot create new connections.");
    return 0;
    }
  return this->ConnectionManager->OpenSelfConnection();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Disconnect(vtkIdType id)
{
  this->ConnectionManager->CloseConnection(id);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::AddManagedSocket(vtkSocket* soc,
  vtkProcessModuleConnection* conn)
{
  this->ConnectionManager->AddManagedSocket(soc, conn);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::RemoveManagedSocket(vtkSocket* soc)
{
  this->ConnectionManager->RemoveManagedSocket(soc);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::ConnectToRemote()
{
  const char* message = "client";
  while (1)
    {
    vtkIdType id = 0;

    switch (this->Options->GetProcessType())
      {
    case vtkPVOptions::PVCLIENT:
      if (this->Options->GetRenderServerMode())
        {
        id = this->ConnectionManager->OpenConnection(
          this->Options->GetDataServerHostName(),
          this->Options->GetDataServerPort(),
          this->Options->GetRenderServerHostName(),
          this->Options->GetRenderServerPort());
        message = "servers";
        }
      else
        {
        id = this->ConnectionManager->OpenConnection(
          this->Options->GetServerHostName(), this->Options->GetServerPort());
        message = "server";
        }
      break;

    case vtkPVOptions::PVSERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetServerPort());
      break;

    case vtkPVOptions::PVRENDER_SERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetRenderServerPort());
      cout << "RenderServer: ";
      break;

    case vtkPVOptions::PVDATA_SERVER:
      id = this->ConnectionManager->OpenConnection(
        this->Options->GetClientHostName(),
        this->Options->GetDataServerPort());
      break;

    default:
      vtkErrorMacro("Invalid mode!");
      return 0;
      }
    
    if (id)
      {
      // connection successful.
      cout << "Connected to " << message << endl;
      return 1;
      }
    
    if (!this->GUIHelper)
      {
      // Probably a server. Just flag error and exit.
      vtkErrorMacro("Server Error: Could not connect to client.");
      return 0;
      }
    int start = 0;
    if (!this->GUIHelper->OpenConnectionDialog(&start))
      {
      vtkErrorMacro("Client error: Could not connect to the server. If you are trying "
        "to connect a client to data and render servers, you must use "
        "the --client-render-server (-crs) argument.");
      this->GUIHelper->ExitApplication();
      return 0;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::IsRemote(vtkIdType id)
{
  vtkRemoteConnection* rc = vtkRemoteConnection::SafeDownCast(
    this->ConnectionManager->GetConnectionFromID(id));
  if (rc)
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModule::MonitorConnections(unsigned long msec)
{
  switch(this->ConnectionManager->MonitorConnections(msec))
    {
    case -1:
      return -1;
    case 2:
      vtkIdType connid = this->LastConnectionID;
      this->LastConnectionID = -1;
      return connid;
    }
    
  return 0;
}

//-----------------------------------------------------------------------------
// Called on client in reverse connect mode.
// Will establish server connections.
int vtkProcessModule::ClientWaitForConnection()
{
  
  cout << "Waiting for server..." << endl;
  this->GUIHelper->PopupDialog("Waiting for server",
    "Waiting for server to connect to this client via the reverse connection.");

  int not_abort_connection = 1;
  int res;
  while (not_abort_connection)
    {
    // Wait for 1/100 th of a second.
    res = this->ConnectionManager->MonitorConnections(10);
    if (res != 0 && res != 1)
      {
      this->GUIHelper->ClosePopup();
      }
    
    if (res < 0)
      {
      // Error !
      return 0;
      }
    if (res == 2)
      {
      // Connection created successfully.
      cout << "Server connected." << endl;
      return 1;
      }
    if (res == 1)
      {
      // Processed 1 of the 2 required connections....wait on...
      continue;
      }
    // Timeout.
    not_abort_connection = this->GUIHelper->UpdatePopup();
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::NewStreamObject(
  const char* type, vtkClientServerStream& stream)
{
  vtkClientServerID id = this->GetUniqueID();
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;
  return id;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::NewStreamObject(
  const char* type, vtkClientServerStream& stream, vtkClientServerID id)
{
  if (this->UniqueID.ID <= id.ID)
    {
    this->UniqueID.ID = (id.ID+1);
    }
  stream << vtkClientServerStream::New << type
         << id <<  vtkClientServerStream::End;

  return id;
}

//-----------------------------------------------------------------------------
vtkObjectBase* vtkProcessModule::GetObjectFromID(vtkClientServerID id,
  bool silent)
{
  return this->Interpreter->GetObjectFromID(id, silent);
}
//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetIDFromObject(vtkObjectBase *obj)
{
  return this->Interpreter->GetIDFromObject(obj);
}

//----------------------------------------------------------------------------
void vtkProcessModule::DeleteStreamObject(
  vtkClientServerID id, vtkClientServerStream& stream)
{
  stream << vtkClientServerStream::Delete << id
         <<  vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
const vtkClientServerStream& vtkProcessModule::GetLastResult(
  vtkIdType connectionID, vtkTypeUInt32 server)
{
  return this->ConnectionManager->GetLastResult(connectionID, server);
}

//-----------------------------------------------------------------------------
vtkIdType vtkProcessModule::GetConnectionID(vtkProcessModuleConnection* conn)
{
  return this->ConnectionManager->GetConnectionID(conn);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::SendStream(vtkIdType connectionID, 
  vtkTypeUInt32 server, vtkClientServerStream& stream, int resetStream/*=1*/)
{
  if (stream.GetNumberOfMessages() < 1)
    {
    return 0;
    }
  
  if (this->SendStreamToClientOnly)
    {
    server &= vtkProcessModule::CLIENT;
    if (!server)
      {
      /*
      vtkWarningMacro("The process module is in a ClientOnly mode, "
        "and a SendStream was requested to send something only to the servers."
        "This call will be ignored.");
        */
      }
    }

  int ret = this->ConnectionManager->SendStream(connectionID,
    server, stream, resetStream);
 
  // If send failed on a Client, it means that the server connection was closed.
  // So currently, we exit the client.
 
  if (ret != 0 && this->GUIHelper)
    {
    cout << "Connection Error: Server connection closed!" << endl;
    }
  return ret;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Initialize()
{
  this->InitializeInterpreter();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::Finalize()
{
  this->SetGUIHelper(0);
  if (this->ConnectionManager)
    {
    // Tell the connection manager to close all connections.
    // This will clean up the communicators.
    this->ConnectionManager->Finalize();
    }
  this->FinalizeInterpreter();
  this->InvokeEvent(vtkCommand::ExitEvent);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InitializeInterpreter(InterpreterInitializationCallback callback)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm && pm->GetInterpreter())
    {
    (*callback)(pm->GetInterpreter());
    }
  else
    {
    if (!vtkProcessModule::InitializationCallbacks)
      {
      vtkProcessModule::InitializationCallbacks = new vtkInterpreterInitializationCallbackVector();
      }
    vtkProcessModule::InitializationCallbacks->push_back(callback);
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InitializeInterpreter()
{
  if (this->Interpreter)
    {
    return;
    }
  vtkMultiThreader::SetGlobalMaximumNumberOfThreads(1);

  // Create the interpreter and supporting stream.
  this->Interpreter = vtkClientServerInterpreter::New();

  // Setup a callback for the interpreter to report errors.
  this->InterpreterObserver = vtkCallbackCommand::New();
  this->InterpreterObserver->SetCallback(
    &vtkProcessModule::InterpreterCallbackFunction);
  this->InterpreterObserver->SetClientData(this);
  this->Interpreter->AddObserver(vtkCommand::UserEvent,
    this->InterpreterObserver);

  if (!this->Options)
    {
    vtkErrorMacro("Options must be set before calling "
      "InitializeInterpreter().");
    }

  if (getenv("VTK_CLIENT_SERVER_LOG") ||
    this->Options->GetLogFileName())
    {
    const char* logfilename = this->Options->GetLogFileName();
    if (!logfilename && this->Options->GetClientMode())
      {
      logfilename = "paraviewClient.log";
      }
    if (!logfilename && this->Options->GetServerMode())
      {
      logfilename = "paraviewServer.log";
      }
    if (!logfilename && this->Options->GetRenderServerMode())
      {
      logfilename = "paraviewRenderServer.log";
      }
    if (!logfilename)
      {
      logfilename = "paraview.log";
      }

    // TODO: would be nice if each process could write the log to a separate
    // file, however InitializeInterpreter() is called before the multi process
    // controller is initialized hence we cannot really get the information
    // about number of processes etc.
    this->Interpreter->SetLogFile(logfilename);
    } 

  // Assign standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Assign
    << this->GetProcessModuleID() << this
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css); 

  // If any initialization callbacks were registered, call them.
  if (this->InitializationCallbacks)
    {
    vtkInterpreterInitializationCallbackVector& callbacks = 
      (*this->InitializationCallbacks);
   vtkInterpreterInitializationCallbackVector::iterator iter;
   for (iter = callbacks.begin(); iter != callbacks.end(); ++iter)
     {
     if (*iter)
       {
       (*(*iter))(this->GetInterpreter());
       }
     }
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::FinalizeInterpreter()
{
  if (!this->Interpreter)
    {
    return;
    }

  // Delete the standard IDs.
  vtkClientServerStream css;
  css << vtkClientServerStream::Delete
    << this->GetProcessModuleID()
    << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(css);

  // Free the interpreter and supporting stream.
  this->Interpreter->RemoveObserver(this->InterpreterObserver);
  this->InterpreterObserver->Delete();
  this->InterpreterObserver = 0;
  this->Interpreter->Delete();
  this->Interpreter = 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallbackFunction(vtkObject*,
  unsigned long eid, void* cd, void* d)
{
  reinterpret_cast<vtkProcessModule*>(cd)->InterpreterCallback(eid, d);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InterpreterCallback(unsigned long, void* pinfo)
{
  if (!this->ReportInterpreterErrors)
    {
    return;
    }
  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(pinfo);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
    (last.GetCommand(0) == vtkClientServerStream::Error) &&
    last.GetArgument(0, 0, &errorMessage))
    {
    vtksys_ios::ostringstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str().c_str());
    vtkErrorMacro("Aborting execution for debugging purposes.");
    abort();
    }
}

//-----------------------------------------------------------------------------
vtkMultiProcessController* vtkProcessModule::GetController()
{
  return vtkMultiProcessController::GetGlobalController();
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetPartitionId()
{
  if (this->Options && this->Options->GetClientMode())
    {
    return 0;
    }
  if (vtkMultiProcessController::GetGlobalController())
    {
    return vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
    }
  return 0;
}


//-----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfLocalPartitions()
{
  if (vtkMultiProcessController::GetGlobalController())
    {
    return vtkMultiProcessController::GetGlobalController()->
      GetNumberOfProcesses();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfPartitions(vtkIdType id)
{
  if (this->Options && this->Options->GetClientMode() && 
    id != vtkProcessModuleConnectionManager::GetSelfConnectionID())
    {
    return this->ConnectionManager->GetNumberOfPartitions(id);
    }
  if (vtkMultiProcessController::GetGlobalController())
    {
    return vtkMultiProcessController::GetGlobalController()->
      GetNumberOfProcesses();
    }
  return 1;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetUniqueID()
{
  this->UniqueID.ID ++;
  return this->UniqueID;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::ReserveID(vtkClientServerID id)
{
  if (this->UniqueID < id)
    {
    this->UniqueID = id;
    }
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetProcessModuleID()
{
  vtkClientServerID id(2);
  return id;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetProcessModule(vtkProcessModule* pm)
{
  vtkProcessModule::ProcessModule = pm;
}

//-----------------------------------------------------------------------------
vtkProcessModule* vtkProcessModule::GetProcessModule()
{
  return vtkProcessModule::ProcessModule;
}

//----------------------------------------------------------------------------
void vtkProcessModule::RegisterProgressEvent(vtkObject* po, int id)
{
  // We do this check to avoid registering progress events for
  // objects that don't report any progress at all.
  //if (po->IsA("vtkAlgorithm") || po->IsA("vtkKdTree"))
  //  {
  //  //po->AddObserver(vtkCommand::ProgressEvent, this->Observer);
  //  //this->ProgressHandler->RegisterProgressEvent(po, id);
  //  }
  if (this->ActiveRemoteConnection)
    {
    this->ActiveRemoteConnection->GetProgressHandler()->
      RegisterProgressEvent(po, id);
    }
  else
    {
    this->ConnectionManager->GetConnectionFromID(
      vtkProcessModuleConnectionManager::GetSelfConnectionID())->
      GetProgressHandler()->RegisterProgressEvent(po, id);
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendPrepareProgress(vtkIdType connectionId,
  vtkTypeUInt32 servers/*=CLIENT|DATA_SERVER*/)
{
  if (!this->GUIHelper)
    {
    // vtkErrorMacro("GUIHelper must be set for SendPrepareProgress.");
    return;
    }

  if (this->ProgressRequests == 0)
    {
    this->Internals->ProgressServersFlag = servers;
    this->GUIHelper->SendPrepareProgress();
    this->InvokeEvent(vtkCommand::StartEvent);
    }
  else
    {
    // we need to send the progress request to those servers to which
    // we haven't already sent the request.
    servers = servers & (~this->Internals->ProgressServersFlag);
    this->Internals->ProgressServersFlag |= servers;
    }

  if (servers)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->GetProcessModuleID()
           << "PrepareProgress" 
           << vtkClientServerStream::End;
    this->SendStream(connectionId, servers, stream);
    }

  this->ProgressRequests ++;
}

//----------------------------------------------------------------------------
void vtkProcessModule::SendCleanupPendingProgress(vtkIdType connectionId)
{
  if (!this->GUIHelper)
    {
    // vtkErrorMacro("GUIHelper must be set for SendCleanupPendingProgress.");
    return;
    }

  if ( this->ProgressRequests < 0 )
    {
    vtkErrorMacro("Internal ParaView Error: Progress requests went below zero");
    abort();
    }
  this->ProgressRequests --;
  if ( this->ProgressRequests > 0 )
    {
    return;
    }
  vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke 
           << this->GetProcessModuleID()
           << "CleanupPendingProgress" 
           << vtkClientServerStream::End;
  this->SendStream(connectionId, this->Internals->ProgressServersFlag, stream);
  this->Internals->ProgressServersFlag = 0;
  
  this->GUIHelper->SendCleanupPendingProgress();

  if (this->LastProgress < 100 && this->LastProgressName)
    {
    this->LastProgress = 100;
    float fprog = 1.0;
    this->InvokeEvent(vtkCommand::ProgressEvent, &fprog);
    this->SetLastProgressName(0);
    }
  this->InvokeEvent(vtkCommand::EndEvent);
}

//-----------------------------------------------------------------------------
vtkPVProgressHandler* vtkProcessModule::GetActiveProgressHandler()
{
  if (this->ActiveRemoteConnection)
    {
    return this->ActiveRemoteConnection->GetProgressHandler();
    }
  return this->ConnectionManager->GetConnectionFromID(
    vtkProcessModuleConnectionManager::GetSelfConnectionID())->GetProgressHandler();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::PrepareProgress()
{
  if (this->ActiveRemoteConnection)
    {
    this->ActiveRemoteConnection->GetProgressHandler()->PrepareProgress();
    }
  else
    {
    this->ConnectionManager->GetConnectionFromID(
      vtkProcessModuleConnectionManager::GetSelfConnectionID())->
      GetProgressHandler()->PrepareProgress();
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::CleanupPendingProgress()
{
  if (this->ActiveRemoteConnection)
    {
    this->ActiveRemoteConnection->GetProgressHandler()->CleanupPendingProgress();
    }
  else
    {
    this->ConnectionManager->GetConnectionFromID(
      vtkProcessModuleConnectionManager::GetSelfConnectionID())->
      GetProgressHandler()->CleanupPendingProgress();
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::ExceptionEvent(const char* message)
{
  vtkErrorMacro("Received exception from server: " << message);
}
//-----------------------------------------------------------------------------
void vtkProcessModule::ExceptionEvent(int type)
{
  this->ExceptionRaised = 1;
  const char* msg = 0;
  switch (type)
    {
  case vtkProcessModule::EXCEPTION_BAD_ALLOC:
    msg = "Insufficient memory exception.";
    break;
  case vtkProcessModule::EXCEPTION_UNKNOWN:
    msg = "Exception.";
    break;
    }
  vtkErrorMacro("Exception: " << msg);
  // Now send every client the message, for now,
  // we send to only the active client, as only the active client
  // is listening for messages from the server.
  if (this->GetActiveSocketController())
    {
    this->GetActiveSocketController()->Send((char*)msg, strlen(msg)+1, 1,
      vtkProcessModule::EXCEPTION_EVENT_TAG);
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::ExecuteEvent(
  vtkObject* o, unsigned long event, void* calldata)
{
  switch (event)
    {
  case vtkCommand::AbortCheckEvent:
    this->InvokeEvent(vtkCommand::AbortCheckEvent);
    break;

  case vtkCommand::ConnectionCreatedEvent:
    this->InvokeEvent(event, calldata);
    this->LastConnectionID = *(reinterpret_cast<vtkIdType*>(calldata));
    break;

  case vtkCommand::ConnectionClosedEvent:
    this->InvokeEvent(event, calldata);
    break;

  case vtkCommand::ErrorEvent:
    if (o == vtkOutputWindow::GetInstance())
      {
      vtksys::RegularExpression re("Unable to allocate");
      const char* data = reinterpret_cast<const char*>(calldata);
      if (data && re.find(data))
        {
        // We throw an exception instead of calling 
        // this->ExceptionEvent() directly so that the the calls
        // unwind. This makes it possible for the server to exit gracefully
        // (although there might be some leaks). Otherwise, the server most
        // likely will segfault or we will have to force exit (using exit()).
        throw vtkstd::bad_alloc();
        }
      }
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::SetLocalProgress(const char* filter, int progress)
{
  if (!this->GUIHelper)
    {
    //vtkErrorMacro("GUIHelper must be set for SetLocalProgress " << filter
    //  << " " << progress);
    return;
    }
  this->LastProgress = progress;
  this->SetLastProgressName(filter);
  float fprog = (float)progress/100.0;
  this->InvokeEvent(vtkCommand::ProgressEvent, &fprog);
  this->GUIHelper->SetLocalProgress(filter, progress);
}

//-----------------------------------------------------------------------------
const char* vtkProcessModule::DetermineLogFilePrefix()
{
  if (this->Options)
    {
    switch (this->Options->GetProcessType())
      {
    case vtkPVOptions::PVCLIENT:
      return NULL; // don't need a log for client.
    case vtkPVOptions::PVSERVER:
      return "ServerNodeLog";
    case vtkPVOptions::PVRENDER_SERVER:
      return "RenderServerNodeLog";
    case vtkPVOptions::PVDATA_SERVER:
      return "DataServerNodeLog";
      }
    }
  return "NodeLog";
}

//-----------------------------------------------------------------------------
ofstream* vtkProcessModule::GetLogFile()
{
  return this->LogFile;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::CreateLogFile()
{
  const char *prefix = this->DetermineLogFilePrefix();
  if (!prefix)
    {
    return;
    }
  
  vtksys_ios::ostringstream fileName;
  fileName << prefix << this->GetPartitionId() << ".txt"
    << ends;
  if (this->LogFile)
    {
    this->LogFile->close();
    delete this->LogFile;
    }
  this->LogFile = new ofstream(fileName.str().c_str(), ios::out);
  if (this->LogFile->fail())
    {
    delete this->LogFile;
    this->LogFile = 0;
    }
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetDirectoryListing(vtkIdType connectionID,
  const char* dir, vtkStringList* dirs, vtkStringList* files, int save)
{
  // Get the listing from the server.
  vtkClientServerStream stream;
  vtkClientServerID lid = 
    this->NewStreamObject("vtkPVServerFileListing", stream);
  stream << vtkClientServerStream::Invoke
    << lid << "GetFileListing" << dir << save
    << vtkClientServerStream::End;
  this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);
  
  vtkClientServerStream result;
  if(!this->GetLastResult(connectionID, 
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &result))
    {
    vtkErrorMacro("Error getting file list result from server.");
    this->DeleteStreamObject(lid, stream);
    this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);
    return 0;
    }
  this->DeleteStreamObject(lid, stream);
  this->SendStream(connectionID, vtkProcessModule::DATA_SERVER_ROOT, stream);

  // Parse the listing.
  if ( dirs )
    {
    dirs->RemoveAllItems();
    }
  if ( files )
    {
    files->RemoveAllItems();
    }
  if(result.GetNumberOfMessages() == 2)
    {
    int i;
    // The first message lists directories.
    if ( dirs )
      {
      for(i=0; i < result.GetNumberOfArguments(0); ++i)
        {
        const char* d;
        if(result.GetArgument(0, i, &d))
          {
          dirs->AddString(d);
          }
        else
          {
          vtkErrorMacro("Error getting directory name from listing.");
          }
        }
      }

    // The second message lists files.
    if ( files )
      {
      for(i=0; i < result.GetNumberOfArguments(1); ++i)
        {
        const char* f;
        if(result.GetArgument(1, i, &f))
          {
          files->AddString(f);
          }
        else
          {
          vtkErrorMacro("Error getting file name from listing.");
          }
        }
      }
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
int vtkProcessModule::LoadModule(vtkIdType connectionID,
  vtkTypeUInt32 serverFlags, const char* name, const char* directory)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "LoadModuleInternal" << name << directory
    << vtkClientServerStream::End;
  
  this->SendStream(connectionID, serverFlags, stream);
  
  int result = 0;
  if(!this->GetLastResult(connectionID,
      this->GetRootId(serverFlags)).GetArgument(0, 0, &result))
    {
    vtkErrorMacro("LoadModule could not get result from server.");
    return 0;
    }

  return result;
}

//-----------------------------------------------------------------------------
// This method (similar to GatherInformationInternal, simply forwards the
// call to the SelfConnection.
int vtkProcessModule::LoadModuleInternal(const char* name, const char* dir)
{
  return this->ConnectionManager->LoadModule(
    vtkProcessModuleConnectionManager::GetSelfConnectionID(), name, dir);
}

//-----------------------------------------------------------------------------
void vtkProcessModule::LogStartEvent(const char* str)
{
  vtkTimerLog::MarkStartEvent(str);
  this->Timer->StartTimer();
}
  
//-----------------------------------------------------------------------------
void vtkProcessModule::LogEndEvent(const char* str)
{
  this->Timer->StopTimer();
  vtkTimerLog::MarkEndEvent(str);
  if (strstr(str, "id:") && this->LogFile)
    {
    *this->LogFile << str << ", " << this->Timer->GetElapsedTime()
      << " seconds" << endl;
    *this->LogFile << "--- Virtual memory available: "
      << this->MemoryInformation->GetAvailableVirtualMemory()
      << " KB" << endl;
    *this->LogFile << "--- Physical memory available: "
      << this->MemoryInformation->GetAvailablePhysicalMemory()
      << " KB" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetLogBufferLength(vtkIdType connectionID,
                                          vtkTypeUInt32 servers,
                                          int length)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "SetLogBufferLength"
         << length
         << vtkClientServerStream::End;
  this->SendStream(connectionID, servers, stream);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetLogBufferLength(int length)
{
  vtkTimerLog::SetMaxEntries(length);
}

//----------------------------------------------------------------------------
void vtkProcessModule::ResetLog(vtkIdType connectionID,
                                vtkTypeUInt32 servers)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "ResetLog"
         << vtkClientServerStream::End;
  this->SendStream(connectionID, servers, stream);
}

//----------------------------------------------------------------------------
void vtkProcessModule::ResetLog()
{
  vtkTimerLog::ResetLog();
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetEnableLog(vtkIdType connectionID,
                                    vtkTypeUInt32 servers,
                                    int flag)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "SetEnableLog"
         << flag
         << vtkClientServerStream::End;
  this->SendStream(connectionID, servers, stream);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetEnableLog(int flag)
{
  vtkTimerLog::SetLogging(flag);
}

//----------------------------------------------------------------------------
void vtkProcessModule::SetLogThreshold(vtkIdType connectionID, 
                                       vtkTypeUInt32 servers,
                                       double threshold)
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetProcessModuleID()
         << "SetLogThreshold"
         << threshold
         << vtkClientServerStream::End;
  this->SendStream(connectionID, servers, stream);
}

//============================================================================
// Stuff that is a part of render-process module.
//-----------------------------------------------------------------------------
const char* vtkProcessModule::GetPath(const char* tag, 
  const char* relativePath, const char* file)
{
  if ( !tag || !relativePath || !file )
    {
    return 0;
    }
  int found=0;

  if(this->Options)
    {
    vtksys_stl::string selfPath, errorMsg;
    vtksys_stl::string oldSelfPath;
    if (vtksys::SystemTools::FindProgramPath(
        this->Options->GetArgv0(), selfPath, errorMsg))
      {
      oldSelfPath = selfPath;
      selfPath = vtksys::SystemTools::GetFilenamePath(selfPath);
      selfPath += "/../share/paraview-" PARAVIEW_VERSION;
      vtkstd::string str = selfPath;
      str += "/";
      str += relativePath;
      str += "/";
      str += file;
      if(vtksys::SystemTools::FileExists(str.c_str()))
        {
        this->Internals->Paths[tag] = selfPath.c_str();
        found = 1;
        }
      }
    if ( !found )
      {
      selfPath = oldSelfPath;
      selfPath = vtksys::SystemTools::GetFilenamePath(selfPath);
      selfPath += "/../../share/paraview-" PARAVIEW_VERSION;
      vtkstd::string str = selfPath;
      str += "/";
      str += relativePath;
      str += "/";
      str += file;
      if(vtksys::SystemTools::FileExists(str.c_str()))
        {
        this->Internals->Paths[tag] = selfPath.c_str();
        found = 1;
        }
      }
    }

  if (!found)
    {
    // Look in binary and installation directories
    const char** dir;
    for(dir=PARAVIEW_PATHS; !found && *dir; ++dir)
      {
      vtkstd::string fullFile = *dir;
      fullFile += "/";
      fullFile += relativePath;
      fullFile += "/";
      fullFile += file;
      if(vtksys::SystemTools::FileExists(fullFile.c_str()))
        {
        this->Internals->Paths[tag] = *dir;
        found = 1;
        }
      }
    }
  if ( this->Internals->Paths.find(tag) == this->Internals->Paths.end() )
    {
    return 0;
    }

  return this->Internals->Paths[tag].c_str();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetRenderNodePort()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetRenderNodePort();
}

//----------------------------------------------------------------------------
char* vtkProcessModule::GetMachinesFileName()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetMachinesFileName();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetClientMode()
{
  if ( !this->Options )
    {
    return 0;
    }
  return this->Options->GetClientMode();
}

//----------------------------------------------------------------------------
int vtkProcessModule::GetRenderClientMode(vtkIdType cid)
{
  return this->ConnectionManager->GetRenderClientMode(cid);
}

//----------------------------------------------------------------------------
unsigned int vtkProcessModule::GetNumberOfMachines()
{
  vtkPVServerOptions *opt = vtkPVServerOptions::SafeDownCast(this->Options);
  if (!opt)
    {
    return 0;
    }
  return opt->GetNumberOfMachines();
}

//----------------------------------------------------------------------------
const char* vtkProcessModule::GetMachineName(unsigned int idx)
{
  vtkPVServerOptions *opt = vtkPVServerOptions::SafeDownCast(this->Options);
  if (!opt)
    {
    return NULL;
    }
  return opt->GetMachineName(idx);
}

//----------------------------------------------------------------------------
vtkPVServerInformation* vtkProcessModule::GetServerInformation(
  vtkIdType id)
{
  vtkPVServerInformation* info = 
    this->ConnectionManager->GetServerInformation(id);
  return (info)? info : this->ServerInformation;
}

//-----------------------------------------------------------------------------
vtkClientServerID vtkProcessModule::GetMPIMToNSocketConnectionID(
  vtkIdType id)
{
  return this->ConnectionManager->GetMPIMToNSocketConnectionID(id);
}


//----------------------------------------------------------------------------
// This method leaks memory.  It is a quick and dirty way to set different 
// DISPLAY environment variables on the render server.  I think the string 
// cannot be deleted until paraview exits.  The var should have the form:
// "DISPLAY=amber1"
void vtkProcessModule::SetProcessEnvironmentVariable(int processId,
                                                     const char* var)
{
  if (this->GetPartitionId() == processId)
    {
    char* envstr = vtksys::SystemTools::DuplicateString(var);
    putenv(envstr);
    }
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkProcessModule::GetActiveSocketController()
{
  if (!this->ActiveRemoteConnection)
    {
    return 0;
    }
  return this->ActiveRemoteConnection->GetSocketController();
}

//-----------------------------------------------------------------------------
vtkSocketController* vtkProcessModule::GetActiveRenderServerSocketController()
{
  if (!this->ActiveRemoteConnection)
    {
    return 0;
    }
  if (vtkServerConnection::SafeDownCast(this->ActiveRemoteConnection))
    {
    vtkSocketController* c = vtkServerConnection::SafeDownCast(
      this->ActiveRemoteConnection)->GetRenderServerSocketController();
    if (c)
      {
      return c;
      }
    }
  return this->GetActiveSocketController();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::PushUndo(vtkIdType id, const char* label, 
  vtkPVXMLElement* root)
{
  this->ConnectionManager->PushUndo(id, label, root); 
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkProcessModule::NewNextUndo(vtkIdType id)
{
  return this->ConnectionManager->NewNextUndo(id);
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkProcessModule::NewNextRedo(vtkIdType id)
{
  return this->ConnectionManager->NewNextRedo(id);
}

//-----------------------------------------------------------------------------
int vtkProcessModule::GetNumberOfConnections()
{
  if (!this->ConnectionManager)
    {
    return 0;
    }
  return this->ConnectionManager->GetNumberOfConnections();
}

//-----------------------------------------------------------------------------
void vtkProcessModule::InitializeDebugLog(ostream& ref)
{
  if (vtkProcessModule::DebugLogStream)
    {
    vtkGenericWarningMacro("Debug log already initialized.");
    }
  vtkProcessModule::DebugLogStream = &ref;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::DebugLog(const char* msg)
{
  if (vtkProcessModule::DebugLogStream)
    {
    *vtkProcessModule::DebugLogStream << msg << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkProcessModule::FinalizeDebugLog()
{
  vtkProcessModule::DebugLogStream = 0;
}

//-----------------------------------------------------------------------------
void vtkProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LogThreshold: " << this->LogThreshold << endl;
  os << indent << "ProgressRequests: " << this->ProgressRequests << endl;
  os << indent << "ReportInterpreterErrors: " << this->ReportInterpreterErrors
    << endl;
  os << indent << "SupportMultipleConnections: " << this->SupportMultipleConnections
    << endl;
  os << indent << "UseMPI: " << this->UseMPI << endl;
  os << indent << "SendStreamToClientOnly: " 
    << this->SendStreamToClientOnly << endl;
  os << indent 
    << (this->LastProgressName? this->LastProgressName : "(none)") << endl;
 
  os << indent << "Interpreter: " ;
  if (this->Interpreter)
    {
    this->Interpreter->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ActiveRemoteConnection: " ;
  if (this->ActiveRemoteConnection)
    {
    this->ActiveRemoteConnection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  
  os << indent << "Options: ";
  if (this->Options)
    {
    this->Options->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "GUIHelper: ";
  if (this->GUIHelper)
    {
    this->GUIHelper->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "CacheSizeKeeper: ";
  if (this->CacheSizeKeeper)
    {
    this->CacheSizeKeeper->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
