/*=========================================================================

  Program:   ParaView
  Module:    vtkPVOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVOptions
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
*/

#ifndef vtkPVOptions_h
#define vtkPVOptions_h

#include "vtkCommandOptions.h"
#include "vtkRemotingCoreModule.h" //needed for exports

#include <string>  // used for ivar
#include <utility> // needed for pair
#include <vector>  // needed for vector

class vtkPVOptionsInternal;

class VTKREMOTINGCORE_EXPORT vtkPVOptions : public vtkCommandOptions
{
protected:
  friend class vtkPVOptionsXMLParser;

public:
  static vtkPVOptions* New();
  vtkTypeMacro(vtkPVOptions, vtkCommandOptions);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Convenience method to get the local process's host name.
   */
  vtkGetStringMacro(HostName);
  //@}

  vtkGetMacro(ConnectID, int);
  vtkGetMacro(UseStereoRendering, int);
  vtkGetStringMacro(StereoType);

  vtkGetMacro(ReverseConnection, int);
  vtkGetMacro(UseRenderingGroup, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileMullions, int);

  /**
   * Returns true if the tile display configuration is requested.
   */
  virtual bool GetIsInTileDisplay() const;

  /**
   * Returns true of CAVE configuration is requested.
   */
  virtual bool GetIsInCave() const;

  /**
   * Returns the egl device index. -1 indicates that no value was specified.
   */
  vtkGetMacro(EGLDeviceIndex, int);

  //@{
  /**
   * This is the argument specified by --data on the command line. Additionally,
   * this can also correspond to the last argument specified on the command
   * line if the argument is unknown.
   */
  vtkGetStringMacro(ParaViewDataName);
  //@}

  //@{
  /**
   * Servers file to load on startup.
   */
  vtkGetStringMacro(ServersFileName);
  //@}

  //@{
  /**
   * Valid on PVSERVER and PVDATA_SERVER only. It denotes the time (in minutes)
   * since the time that the connection was established with the server that the
   * server may timeout. timeout <= 0 means no timeout.
   */
  vtkGetMacro(Timeout, int);
  //@}

  //@{
  /**
   * Clients need to set the ConnectID so they can handle server connections
   * after the client has started.
   */
  vtkSetMacro(ConnectID, int);
  //@}

  //@{
  /**
   * Log filename.
   */
  vtkSetStringMacro(LogFileName);
  vtkGetStringMacro(LogFileName);
  //@}

  /**
   * Is this server was started for collaboration meaning that it allow
   * several clients to connect to the same server and share the same
   * pipeline and visualization.
   */
  virtual int GetMultiClientMode()
  {
    return (this->MultiClientMode || this->MultiClientModeWithErrorMacro) ? 1 : 0;
  }
  virtual int IsMultiClientModeDebug() { return this->MultiClientModeWithErrorMacro; }

  //@{
  /**
   * Returns if this server does not allow connection after the first client.
   */
  vtkGetMacro(DisableFurtherConnections, int);
  //@}

  //@{
  /**
   * Is this client allow multiple server connection in parallel
   */
  vtkGetMacro(MultiServerMode, int);
  //@}

  //@{
  /**
   * Indicates if the application is in symmetric mpi mode.
   * This is applicable only to PVBATCH type of processes.
   * Typically, when set to true, the python script is run on satellites as
   * well, otherwise only the root node processes the python script. Disabled by
   * default.
   */
  vtkGetMacro(SymmetricMPIMode, int);
  vtkSetMacro(SymmetricMPIMode, int);
  //@}

  //@{
  /**
   * Should this run print the version numbers and exit.
   */
  vtkGetMacro(TellVersion, int);
  //@}

  /// Provides access to server-url if specified on the command line.
  vtkGetStringMacro(ServerURL);

  /// Provides access to the Catalyst Live port if specified on the command line.
  /// A value of -1 indicates that no value was set.
  vtkGetMacro(CatalystLivePort, int);

  //@{
  /**
   * This is used when user want to open a file at startup
   */
  vtkSetStringMacro(ParaViewDataName);
  //@}

  //@{
  /**
   * Until streaming becomes mainstream, we enable streaming support by passing
   * a command line argument to all processes.
   */
  vtkSetMacro(EnableStreaming, int);
  vtkGetMacro(EnableStreaming, int);
  //@}

  //@{
  /**
   * Include originating process id text into server to client messages.
   */
  vtkSetMacro(SatelliteMessageIds, int);
  vtkGetMacro(SatelliteMessageIds, int);
  //@}

  //@{
  /**
   * Should this process just print monitor information and exit?
   */
  vtkGetMacro(PrintMonitors, int);
  //@}

  //@{
  /**
   * Adding ability to test plugins by loading them at command line
   */
  vtkGetStringMacro(TestPlugins);
  vtkGetStringMacro(TestPluginPaths);
  //@}

  //@{
  /**
   * Flag for controlling auto generation of stack trace on POSIX
   * systems after crash.
   */
  vtkGetMacro(EnableStackTrace, int);
  vtkSetMacro(EnableStackTrace, int);
  //@}

  //@{
  /**
   * Flag for disabling loading of options and settings stored by the
   * application. Often used for testing.
   */
  vtkGetMacro(DisableRegistry, int);
  //@}

  //@{
  /**
   * XDisplay test on server processes during initialization sometimes happens
   * too early and may result in remote rendering prematurely disabled. When
   * this flag is set, ParaView will skip such X-display tests. Note, if the
   * display is truly inaccessible when ParaView tries to connect to the server,
   * we will indeed get runtimes errors, including segfaults.
   */
  vtkGetMacro(DisableXDisplayTests, int);
  //@}

  /**
   * When set to true, ParaView will create headless only render windows on the
   * current process.
   */
  vtkGetMacro(ForceOffscreenRendering, int);

  /**
   * When set to true, ParaView will create on-screen render windows.
   */
  vtkGetMacro(ForceOnscreenRendering, int);

  //@{
  /**
   * Get/Set the ForceNoMPIInitOnClient flag.
   */
  vtkGetMacro(ForceNoMPIInitOnClient, int);
  vtkSetMacro(ForceNoMPIInitOnClient, int);
  vtkBooleanMacro(ForceNoMPIInitOnClient, int);
  //@}

  //@{
  /**
   * Get/Set the ForceMPIInitOnClient flag.
   */
  vtkGetMacro(ForceMPIInitOnClient, int);
  vtkSetMacro(ForceMPIInitOnClient, int);
  vtkBooleanMacro(ForceMPIInitOnClient, int);
  //@}

  //@{
  /**
   * Returns the verbosity level for stderr output chosen.
   * Is set to vtkLogger::VERBOSITY_INVALID if not specified.
   */
  vtkGetMacro(LogStdErrVerbosity, int);
  //@}

  //@{
  /**
   * Provides access to display selection. These can be interpreted as EGL
   * device indices or DISPLAY selection. When not specified, this returns an
   * empty string.
  */
  const std::string& GetDisplay(int myrank = 0, int num_ranks = 1);
  //@}

  enum ProcessTypeEnum
  {
    PARAVIEW = 0x2,
    PVCLIENT = 0x4,
    PVSERVER = 0x8,
    PVRENDER_SERVER = 0x10,
    PVDATA_SERVER = 0x20,
    PVBATCH = 0x40,
    ALLPROCESS = PARAVIEW | PVCLIENT | PVSERVER | PVRENDER_SERVER | PVDATA_SERVER | PVBATCH
  };

protected:
  /**
   * Default constructor.
   */
  vtkPVOptions();

  /**
   * Destructor.
   */
  ~vtkPVOptions() override;

  /**
   * Initialize arguments.
   */
  void Initialize() override;

  /**
   * After parsing, process extra option dependencies.
   */
  int PostProcess(int argc, const char* const* argv) override;

  /**
   * This method is called when wrong argument is found. If it returns 0, then
   * the parsing will fail.
   */
  int WrongArgument(const char* argument) override;

  /**
   * This method is called when a deprecated argument is found. If it returns 0, then
   * the parsing will fail.
   */
  int DeprecatedArgument(const char* argument) override;

  //@{
  /**
   * Subclasses may need to access these
   */
  char* ParaViewDataName;
  char* ServerURL; // server URL information
  int ServerMode;
  int ClientMode;
  int RenderServerMode;
  int MultiClientMode;
  int DisableFurtherConnections;
  int MultiClientModeWithErrorMacro;
  int MultiServerMode;
  int SymmetricMPIMode;
  char* ServersFileName;
  char* TestPlugins; // to load plugins from command line for tests
  char* TestPluginPaths;
  int DisableXDisplayTests;
  int
    CatalystLivePort; // currently only set through the GUI but may eventually be set in any client
  //@}

  // inline setters
  vtkSetStringMacro(ServerURL);
  vtkSetStringMacro(ServersFileName);
  vtkSetStringMacro(TestPlugins);
  vtkSetStringMacro(TestPluginPaths);

private:
  int ConnectID;
  int UseOffscreenRendering;
  int EGLDeviceIndex;
  int UseStereoRendering;
  int ReverseConnection;
  int TileDimensions[2];
  int TileMullions[2];
  int UseRenderingGroup;
  int Timeout;
  char* LogFileName;
  int TellVersion;
  char* StereoType;
  int EnableStreaming;
  int SatelliteMessageIds;
  int PrintMonitors;
  int EnableStackTrace;
  int DisableRegistry;
  int ForceMPIInitOnClient;
  int ForceNoMPIInitOnClient;
  int DummyMesaFlag;
  int ForceOffscreenRendering;
  int ForceOnscreenRendering;

  // inline setters
  vtkSetStringMacro(StereoType);

private:
  vtkPVOptions(const vtkPVOptions&) = delete;
  void operator=(const vtkPVOptions&) = delete;

  vtkSetStringMacro(HostName);
  char* HostName;
  int LogStdErrVerbosity;

  std::vector<std::pair<std::string, int> > LogFiles;
  std::vector<std::string> Displays;
  int DisplaysAssignmentMode;

  enum DisplaysAssignmentModeEnum
  {
    CONTIGUOUS,
    ROUNDROBIN
  };

  static int VerbosityArgumentHandler(const char* argument, const char* value, void* call_data);
  static int LogArgumentHandler(const char* argument, const char* value, void* call_data);
  static int DisplaysArgumentHandler(const char* argument, const char* value, void* call_data);
  static int DisplaysAssignmentModeArgumentHandler(
    const char* argument, const char* value, void* call_data);
};

#endif
