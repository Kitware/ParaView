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
/**
 * @class   vtkProcessModule
 * @brief   process initialization and management core for
 * ParaView processes.
 * vtkProcessModule is the process initialization and session management core
 * for ParaView processes.
*/

#ifndef vtkProcessModule_h
#define vtkProcessModule_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"                 // needed for vtkSmartPointer.

#include <string> // for std::string

class vtkInformation;
class vtkMultiProcessController;
class vtkNetworkAccessManager;
class vtkProcessModuleInternals;
class vtkPVOptions;
class vtkSession;
class vtkSessionIterator;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkProcessModule : public vtkObject
{
public:
  static vtkProcessModule* New();
  vtkTypeMacro(vtkProcessModule, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ProcessTypes
  {
    PROCESS_CLIENT,        /* Capable of connecting to remote server or self.
                      Cannot run in Parallel */
    PROCESS_SERVER,        /* data-server+render-server */
    PROCESS_DATA_SERVER,   /* data-server */
    PROCESS_RENDER_SERVER, /* render-server */
    PROCESS_BATCH,         /* Capable of running in parallel with root node acting as client.
                      Cannot connect to remote server */
    PROCESS_INVALID = -1,
  };

  /**
   * These flags are used to specify destination servers for the
   * SendStream function.
   * FIXME: These flags should simply move to PVSession. They don't make sense
   * in non-PVSessions since the processes may have different roles in that
   * case.
   */
  enum ServerFlags
  {
    DATA_SERVER = 0x01,
    DATA_SERVER_ROOT = 0x02,
    RENDER_SERVER = 0x04,
    RENDER_SERVER_ROOT = 0x08,
    SERVERS = DATA_SERVER | RENDER_SERVER,
    CLIENT = 0x10,
    CLIENT_AND_SERVERS = DATA_SERVER | CLIENT | RENDER_SERVER
  };

  static ProcessTypes GetProcessType();
  static unsigned int GetProcessTypeAsInt()
  {
    return static_cast<int>(vtkProcessModule::GetProcessType());
  }

  /**
   * This method has been added to support migration from one type to another
   * but this method call if NOT RECOMMENDED.
   * -> We use it to handle the Animation saving at disconnection time on the
   * server side. We create a new session and migrate the sever process to
   * a batch process.
   */
  void UpdateProcessType(ProcessTypes newType, bool dontKnowWhatImDoing = true);

  //********** PROCESS INITIALIZATION/CLEANUP API *****************************

  /**
   * Initializes the process and the ProcessModule.
   * The function is to initialize MPI if applicable
   * for the process and setup some environment e.g. DISPLAY.
   * Initializes the ProcessModule.
   * for the process and setup some environment e.g. DISPLAY.
   */
  static bool Initialize(ProcessTypes type, int& argc, char**& argv);

  /**
   * Finalizes and cleans up the process.
   */
  static bool Finalize();

  //********** SESSION MANAGEMENT API *****************************

  /**
   * Registers a new session. A new ID is assigned for the session and
   * that ID is returned. The ID can be used in future to access this
   * session.
   * Fires vtkCommand::ConnectionCreatedEvent every time a session is
   * registered. The event-data for this event is a vtkIdType whose value is the
   * session id.
   */
  vtkIdType RegisterSession(vtkSession*);

  //@{
  /**
   * Unregister a session given its ID. This is the same ID that is returned
   * when the session was registered. Returns true is the session was
   * unregistered. Unregistering a session implies that the ProcessModule
   * will no longer monitor communication on the sockets, if any, in the
   * session.
   * Fires vtkCommand::ConnectionClosedEvent every time a session is
   * unregistered. The event-data for this event is a vtkIdType whose value is the
   * session id.
   */
  bool UnRegisterSession(vtkIdType sessionID);
  bool UnRegisterSession(vtkSession* session);
  //@}

  //@{
  /**
   * RegisterSession and UnRegisterSession fire events with SessionID in
   * calldata. To provide access to that in Python, we have this method. The
   * value is valid only in vtkCommand::ConnectionCreatedEvent and
   * vtkCommand::ConnectionClosedEvent callbacks and is set to 0 at other times.
   */
  vtkGetMacro(EventCallDataSessionId, vtkIdType);
  //@}

  /**
   * Returns the session associated with a given ID.
   */
  vtkSession* GetSession(vtkIdType);

  /**
   * Returns the session id for the session, if any. Return 0 is the session has
   * not been registered with the process module.
   */
  vtkIdType GetSessionID(vtkSession*);

  /**
   * Returns a new session iterator that can be used to iterate over the
   * registered sessions.
   */
  vtkSessionIterator* NewSessionIterator();

  /**
   * Whenever any session is processing some message, it typically marks itself
   * active with the process module. The active session can be accessed using
   * this method.
   */
  vtkSession* GetActiveSession();

  /**
   * This is a convenience method that either returns the active session, if
   * present, otherwise the first session. Don't use this for new API. This is
   * provided for some old api.
   */
  vtkSession* GetSession();

  //@{
  /**
   * Return true, if multiple sessions can be used simultanuously.
   * We set the default to be FALSE.
   */
  vtkGetMacro(MultipleSessionsSupport, bool);
  vtkSetMacro(MultipleSessionsSupport, bool);
  vtkBooleanMacro(MultipleSessionsSupport, bool);
  //@}

  //********** ACCESSORS FOR VARIOUS HELPERS *****************************

  /**
   * Provides access to the global ProcessModule. This method can only be called
   * after Initialize().
   */
  static vtkProcessModule* GetProcessModule();

  //@{
  /**
   * Set/Get the application command line options object.
   * Note that this has to be explicitly set. vtkProcessModule::Initialize()
   * does not initialize the vtkPVOptions.
   */
  vtkGetObjectMacro(Options, vtkPVOptions);
  void SetOptions(vtkPVOptions* op);
  //@}

  //********** ACCESSORS FOR VARIOUS HELPERS *****************************

  //@{
  /**
   * Get/Set the network access manager. vtkNetworkAccessManager encapsulates
   * the setup of interprocess communication channels. By default a
   * vtkTCPNetworkAccessManager is setup. If you want to change the network
   * access manager, it should be done only when no sessions are present.
   * Ideally, you want to do that during the initialization of the process
   * itself.
   */
  vtkGetObjectMacro(NetworkAccessManager, vtkNetworkAccessManager);
  void SetNetworkAccessManager(vtkNetworkAccessManager*);
  //@}

  /**
   * Provides access to the global MPI controller, if any. Same can be obtained
   * using vtkMultiProcessController::GetGlobalController();
   */
  vtkMultiProcessController* GetGlobalController();

  /**
   * Returns the number of processes in this process group.
   */
  int GetNumberOfLocalPartitions();

  /**
   * Returns the local process id.
   */
  int GetPartitionId();

  /**
   * Return whether MPI is initialized in this process group.
   */
  bool IsMPIInitialized();

  //@{
  /**
   * Set/Get whether to report errors from the Interpreter.
   */
  vtkGetMacro(ReportInterpreterErrors, bool);
  vtkSetMacro(ReportInterpreterErrors, bool);
  vtkBooleanMacro(ReportInterpreterErrors, bool);
  //@}

  //@{
  /**
   * Returns true if ParaView is to be run in symmetric mode. Symmetric mode
   * implies that satellites process same code as the root node. This is
   * applicable only for PROCESS_BATCH.
   */
  vtkGetMacro(SymmetricMPIMode, bool);
  //@}

  /**
   * The full path to the current executable that is running (or empty if unknown).
   */
  std::string GetProgramPath() const { return this->ProgramPath; }
  /**
   * The directory containing the current executable (or empty if unknown).
   */
  std::string GetSelfDir() const { return this->SelfDir; }

  //@{
  /**
   * This is temporary approach to control the number of ghost-levels to request
   * by default for data pipelines. Currently, change in ghost level
   * request causes the pipeline to re-execute which can be expensive. In an
   * ideal world, additional ghost levels can be automatically provided by
   * data-exchange between ranks. Until we do that, this is only mechanism
   * available to override the number of ghost levels requested by default.
   *
   * The default is 0 for structured pipelines, and 1 for unstructured
   * pipelines. When not running in parallel, however, these ghost-level
   * requests don't make sense and hence are generally ignored.
   *
   * Note, this is expected to change in the future, so use this with
   * caution.
   */
  static void SetDefaultMinimumGhostLevelsToRequestForStructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForStructuredPipelines();
  static void SetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines(int);
  static int GetDefaultMinimumGhostLevelsToRequestForUnstructuredPipelines();
  //@}

  /**
   * This returns number of ghost level to request based on characteristics of
   * the pipelines.
   */
  static int GetNumberOfGhostLevelsToRequest(vtkInformation* outInfo);

protected:
  vtkProcessModule();
  ~vtkProcessModule() override;

  vtkSetMacro(SymmetricMPIMode, bool);

  //@{
  /**
   * Push/Pop the active session.
   */
  void PushActiveSession(vtkSession*);
  void PopActiveSession(vtkSession*);
  //@}

  /**
   * Marking vtkSession as friend since it needs access to
   * PushActiveSession/PopActiveSession.
   */
  friend class vtkSession;

  vtkNetworkAccessManager* NetworkAccessManager;
  vtkPVOptions* Options;

  /**
   * Used to keep track of maximum session used. Only used to ensure that no
   * session id is ever repeated.
   */
  vtkIdType MaxSessionId;

  /**
   * Sets the executable path of the process so that ParaView can, e.g., set up
   * paths for Python properly.
   */
  void SetExecutablePath(const std::string& path);

protected:
  vtkProcessModuleInternals* Internals;

  // vtkSessionIterator needs access to vtkInternals to be able to iterate over
  // the sessions efficiently.
  friend class vtkSessionIterator;

  bool ReportInterpreterErrors;

private:
  vtkProcessModule(const vtkProcessModule&) = delete;
  void operator=(const vtkProcessModule&) = delete;

  void DetermineExecutablePath(int argc, char** argv);

  // Helper to initialize Python environment. This doesn't initialize Python
  // but simply sets up the environment so when Python is initialized, it can
  // find ParaView modules. This does nothing if not build with Python support.
  bool InitializePythonEnvironment();

  static ProcessTypes ProcessType;

  // Set to true in Initialize if Finalize() should cleanup MPI.
  static bool FinalizeMPI;

  // Set to true in InitializePythonEnvironment() if Finalize() should cleanup
  // Python.
  static bool FinalizePython;

  static vtkSmartPointer<vtkProcessModule> Singleton;
  static vtkSmartPointer<vtkMultiProcessController> GlobalController;

  bool SymmetricMPIMode;

  bool MultipleSessionsSupport;

  vtkIdType EventCallDataSessionId;

  std::string ProgramPath;
  std::string SelfDir;

  static int DefaultMinimumGhostLevelsToRequestForStructuredPipelines;
  static int DefaultMinimumGhostLevelsToRequestForUnstructuredPipelines;
};

#endif // vtkProcessModule_h
