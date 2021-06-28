/*=========================================================================

  Program:   ParaView
  Module:    vtkRemotingCoreConfiguration.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemotingCoreConfiguration
 * @brief runtime configuration options for vtkRemotingCore module.
 *
 * vtkRemotingCoreConfiguration is a singleton that maintains runtime
 * configuration for the process. These can be thought of as options settable by
 * command line arguments and hence are initialized when the process launches
 * and are not changed during the lifetime of the process.
 *
 * vtkRemotingCoreConfiguration can be setup via command line arguments. For
 * that, create a `vtkCLIOptions` object and populate it using `PopulateOptions`
 * method. `vtkCLIOptions` can then pass command line arguments for parsing.
 */

#ifndef vtkRemotingCoreConfiguration_h
#define vtkRemotingCoreConfiguration_h

#include "vtkObject.h"
#include "vtkProcessModule.h"      // for vtkProcessModule::ProcessTypes
#include "vtkRemotingCoreModule.h" //needed for exports
#include <string>                  // for std::string
#include <utility>                 // for std::pair
#include <vector>                  // for std::vector

class vtkCLIOptions;
class vtkDisplayConfiguration;

class VTKREMOTINGCORE_EXPORT vtkRemotingCoreConfiguration : public vtkObject
{
public:
  vtkTypeMacro(vtkRemotingCoreConfiguration, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provides access to the singleton.
   */
  static vtkRemotingCoreConfiguration* GetInstance();

  //---------------------------------------------------------------------------
  // Options added using `PopulateGlobalOptions`
  //---------------------------------------------------------------------------

  /**
   * Flag set when user expects the executable to simply print out the version
   * number and exit.
   */
  vtkGetMacro(TellVersion, bool);

  /**
   * Get whether to load user-specific session/configuration files from
   * previous session or start the application using default configuration.
   */
  vtkGetMacro(DisableRegistry, bool);

  /**
   * Print monitor information and exit. This is currently only supported on
   * Windows.
   */
  vtkGetMacro(PrintMonitors, bool);

  //---------------------------------------------------------------------------
  // Options added using `PopulateConnectionOptions`.
  //---------------------------------------------------------------------------

  /**
   * Get the hostname to use. By default, this is initialized to the hostname
   * determined automatically using system calls (specifically,
   * vtksys::SystemInformation).
   */
  const std::string& GetHostName() const { return this->HostName; }

  /**
   * Get the hostname for the "client" process. This is used by server processes
   * when in reverse-connection mode to determine where the client can be
   * reached. Default is "localhost".
   */
  const std::string& GetClientHostName() const { return this->ClientHostName; }

  /**
   * Get the server-port number to use. This is the port number that a
   * server-process is expected to use as the 'listening' port. For
   * reverse-connection, this is the port number that the client process is
   * listening on.
   */
  vtkGetMacro(ServerPort, int);

  /**
   * Get if the process is acting in reverse connection mode. This flag is only
   * read on the server processes since client is capable of supported reverse
   * and non-reverse connections in the same process.
   */
  vtkGetMacro(ReverseConnection, bool);

  /**
   * Returns the connection identifier used to validate client-server
   * connections.
   */
  vtkGetMacro(ConnectID, int);

  /**
   * @note This API may change in future. This is left as is for now to keep
   * changes in this MR to a minimal.
   */
  vtkSetMacro(ConnectID, int);

  /**
   * On server processes, this denotes the time (in minutes)
   * since the time that the connection was established with the server that the
   * server may timeout. timeout <= 0 means no timeout.
   */
  vtkGetMacro(Timeout, int);

  /**
   * On client processes, this returns the server connection url to use to
   * connect to the server process(es) on startup, if any.
   *
   * @sa GetServerResourceName
   */
  const std::string& GetServerURL() const { return this->ServerURL; }

  /**
   * On client processes, this returns the server connection resource name to
   * use to connect to the server process(es) on startup, if any.
   */
  const std::string& GetServerResourceName() const { return this->ServerResourceName; }

  /**
   * On client processes, this provides list of server configurations files to
   * use instead of the default user-specific server configurations file.
   */
  const std::vector<std::string>& GetServerConfigurationsFiles() const
  {
    return this->ServerConfigurationsFiles;
  }
  ///@}

  //---------------------------------------------------------------------------
  // Options added using `PopulatePluginOptions`.
  //---------------------------------------------------------------------------

  /**
   * Get a list of paths to add to plugin search paths. Any plugin requested by
   * name will be searched under these paths.
   */
  const std::vector<std::string>& GetPluginSearchPaths() const { return this->PluginSearchPaths; }

  /**
   * Get a list of names for plugins to load.
   */
  const std::vector<std::string>& GetPlugins() const { return this->Plugins; }
  ///@}

  //---------------------------------------------------------------------------
  // Options added using `PopulateRenderingOptions`.
  //---------------------------------------------------------------------------

  /**
   * Get whether stereo rendering is enabled. This is only valid on rendering
   * processes.
   */
  vtkGetMacro(UseStereoRendering, bool);

  /**
   * Get stereo type requested. Returned values are `VTK_STEREO_*` defined in
   * vtkRenderWindow.h.
   */
  vtkGetMacro(StereoType, int);
  const char* GetStereoTypeAsString() const;

  /**
   * Eye separation to use when using stereo rendering.
   */
  double GetEyeSeparation() const;

  /**
   * Get the gap in pixel between tiles in a tile display./
   */
  vtkGetVector2Macro(TileMullions, int);

  /**
   * Get the dimensions of the tile display.
   */
  void GetTileDimensions(int dims[2]);
  const int* GetTileDimensions();

  /**
   * Returns true if in tile display mode.
   */
  bool GetIsInTileDisplay() const;

  /**
   * Returns true of in CAVE mode.
   */
  bool GetIsInCave() const;

  /**
   * When in CAVE mode, returns the display configurations.
   */
  vtkDisplayConfiguration* GetDisplayConfiguration() const { return this->DisplayConfiguration; }

  /**
   * XDisplay test on server processes during initialization sometimes happens
   * too early and may result in remote rendering prematurely disabled. When
   * this flag is set, ParaView will skip such X-display tests. Note, if the
   * display is truly inaccessible when ParaView tries to connect to the server,
   * we will indeed get runtimes errors, including segfaults.
   */
  vtkGetMacro(DisableXDisplayTests, bool);

  /**
   * When set to true, ParaView will create headless only render windows on the
   * current process.
   */
  vtkGetMacro(ForceOffscreenRendering, bool);

  /**
   * When set to true, ParaView will create on-screen render windows.
   */
  vtkGetMacro(ForceOnscreenRendering, bool);

  /**
   * Returns the EGL device index for the current process. This uses
   * `vtkMultiProcessController::GetLocalProcessId` to determine the rank of the process.
   * -1 indicates no index was specified.
   */
  int GetEGLDeviceIndex();

  /**
   * Returns the display setting, if any, for the current process.
   */
  std::string GetDisplay();

  //---------------------------------------------------------------------------
  // Options added using `PopulateMiscellaneousOptions`.
  //---------------------------------------------------------------------------

  /**
   * Returns true on client process if it should enable connecting to multiple
   * servers at the same time.
   */
  vtkGetMacro(MultiServerMode, bool);

  /**
   * Returns true data server process if the server should allow connecting to
   * multiple clients in a collaboration mode.
   */
  vtkGetMacro(MultiClientMode, bool);

  /**
   * Returns if this server does not allow connection after the first client.
   * This requires MultiClientMode.
   */
  vtkGetMacro(DisableFurtherConnections, bool);

  //---------------------------------------------------------------------------
  /**
   * Populates vtkCLIOptions with available command line options.
   * `processType` indicates which type of ParaView process the options are
   * being setup for. That may affect available options.
   */
  bool PopulateOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);
  bool PopulateGlobalOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);
  bool PopulateConnectionOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);
  bool PopulatePluginOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);
  bool PopulateRenderingOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);
  bool PopulateMiscellaneousOptions(vtkCLIOptions* app, vtkProcessModule::ProcessTypes processType);

  /**
   * Method to setup environment variables for display.
   */
  void HandleDisplayEnvironment();

protected:
  vtkRemotingCoreConfiguration();
  ~vtkRemotingCoreConfiguration();

private:
  static vtkRemotingCoreConfiguration* New();
  vtkRemotingCoreConfiguration(const vtkRemotingCoreConfiguration&) = delete;
  void operator=(const vtkRemotingCoreConfiguration&) = delete;

  enum DisplaysAssignmentModeEnum
  {
    CONTIGUOUS,
    ROUNDROBIN
  };

  bool TellVersion = false;
  bool DisableRegistry = false;
  std::string HostName = "localhost";
  std::string ClientHostName = "localhost";
  int ServerPort = 11111;
  bool ReverseConnection = false;
  int ConnectID = 0;
  std::string ServerURL;
  std::string ServerResourceName;
  int Timeout = 0;
  bool UseStereoRendering = false;
  int StereoType = 0;
  double EyeSeparation = 0.06;
  bool DisableXDisplayTests = false;
  bool ForceOnscreenRendering = false;
  bool ForceOffscreenRendering = false;
  int EGLDeviceIndex = -1;
  DisplaysAssignmentModeEnum DisplaysAssignmentMode = ROUNDROBIN;
  bool MultiServerMode = false;
  bool MultiClientMode = false;
  bool DisableFurtherConnections = false;
  bool PrintMonitors = false;

  std::vector<std::string> Displays;
  std::vector<std::string> PluginSearchPaths;
  std::vector<std::string> Plugins;
  std::vector<std::string> ServerConfigurationsFiles;
  int TileDimensions[2] = { 0, 0 };
  int TileMullions[2] = { 0, 0 };

  vtkDisplayConfiguration* DisplayConfiguration = nullptr;
};

#endif
