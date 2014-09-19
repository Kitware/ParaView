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
// .NAME vtkPVOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
//
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkPVOptions_h
#define __vtkPVOptions_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkCommandOptions.h"

class vtkPVOptionsInternal;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVOptions : public vtkCommandOptions
{
protected:
//BTX
  friend class vtkPVOptionsXMLParser;
//ETX
public:
  static vtkPVOptions* New();
  vtkTypeMacro(vtkPVOptions,vtkCommandOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to get the local process's host name.
  vtkGetStringMacro(HostName);

  vtkGetMacro(ConnectID, int);
  vtkGetMacro(UseOffscreenRendering, int);
  vtkGetMacro(UseStereoRendering, int);
  vtkGetStringMacro(StereoType);

  vtkGetMacro(ReverseConnection, int);
  vtkGetMacro(UseRenderingGroup, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileMullions, int);

  // Description:
  // This is the argument specified by --data on the command line. Additionally,
  // this can also correspond to the last argument specified on the command
  // line if the argument is unknown.
  vtkGetStringMacro(ParaViewDataName);

  // Description:
  // State file to load on startup.
  vtkGetStringMacro(StateFileName); // Bug #5711

  // Description:
  // Valid on PVSERVER and PVDATA_SERVER only. It denotes the time (in minutes)
  // since the time that the connection was established with the server that the
  // server may timeout. timeout <= 0 means no timeout.
  vtkGetMacro(Timeout, int);

  // Description:
  // Clients need to set the ConnectID so they can handle server connections
  // after the client has started.
  vtkSetMacro(ConnectID, int);

  // Description:
  // Log filename.
  vtkSetStringMacro(LogFileName);
  vtkGetStringMacro(LogFileName);

  // Description:
  // vtkPVProcessModule needs to set this.
  vtkSetVector2Macro(TileDimensions, int);
  vtkSetVector2Macro(TileMullions, int);
  vtkSetMacro(UseOffscreenRendering, int);

  // Description:
  // Is this server was started for collaboration meaning that it allow
  // several clients to connect to the same server and share the same
  // pipeline and visualization.
  virtual int GetMultiClientMode()
  { return (this->MultiClientMode || this->MultiClientModeWithErrorMacro) ?1:0; }
  virtual int IsMultiClientModeDebug() { return this->MultiClientModeWithErrorMacro; }

  // Description:
  // Is this client allow multiple server connection in parallel
  vtkGetMacro(MultiServerMode, int);

  // Description:
  // Indicates if the application is in symmetric mpi mode.
  // This is applicable only to PVBATCH type of processes.
  // Typically, when set to true, the python script is run on satellites as
  // well, otherwise only the root node processes the python script. Disabled by
  // default.
  vtkGetMacro(SymmetricMPIMode, int);
  vtkSetMacro(SymmetricMPIMode, int);

  // Description:
  // Should this run print the version numbers and exit.
  vtkGetMacro(TellVersion, int);

  /// Provides access to server-url if specified on the command line.
  vtkGetStringMacro(ServerURL);

  // Description:
  // This is used when user want to open a file at startup
  vtkSetStringMacro(ParaViewDataName);

  // Description:
  // Until streaming becomes mainstream, we enable streaming support by passing
  // a command line argument to all processes.
  vtkGetMacro(EnableStreaming, int);

  // Description:
  // When set, use cuda interop feature
  vtkGetMacro(UseCudaInterop, int);

  // Description:
  // Include originating process id text into server to client messages.
  vtkSetMacro(SatelliteMessageIds, int);
  vtkGetMacro(SatelliteMessageIds, int );

  // Description:
  // Should this process just print monitor information and exit?
  vtkGetMacro(PrintMonitors, int);

  // Description:
  // Adding ability to test plugins by loading them at command line
  vtkGetStringMacro(TestPlugin);
  vtkGetStringMacro(TestPluginPath);

  // Description:
  // Flag for controlling auto generation of stack trace on POSIX
  // systems after crash.
  vtkGetMacro(EnableStackTrace, int);
  vtkSetMacro(EnableStackTrace, int);

  // Description:
  // Flag for disabling loading of options and settings stored by the
  // application. Often used for testing.
  vtkGetMacro(DisableRegistry, int);

  // Description:
  // XDisplay test on server processes during initialization sometimes happens
  // too early and may result in remote rendering prematurely disabled. When
  // this flag is set, ParaView will skip such X-display tests. Note, if the
  // display is truly inaccessible when ParaView tries to connect to the server,
  // we will indeed get runtimes errors, including segfaults.
  vtkGetMacro(DisableXDisplayTests, int);

  enum ProcessTypeEnum
    {
    PARAVIEW = 0x2,
    PVCLIENT = 0x4,
    PVSERVER = 0x8,
    PVRENDER_SERVER = 0x10,
    PVDATA_SERVER = 0x20,
    PVBATCH = 0x40,
    ALLPROCESS = PARAVIEW | PVCLIENT | PVSERVER | PVRENDER_SERVER |
      PVDATA_SERVER | PVBATCH
    };

protected:
//BTX
  // Description:
  // Default constructor.
  vtkPVOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVOptions();

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess(int argc, const char* const* argv);

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  // Description:
  // This method is called when a deprecated argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int DeprecatedArgument(const char* argument);

  // Description:
  // Subclasses may need to access these
  char* ParaViewDataName;
  char* ServerURL; // server URL information
  int ServerMode;
  int ClientMode;
  int RenderServerMode;
  int MultiClientMode;
  int MultiClientModeWithErrorMacro;
  int MultiServerMode;
  int SymmetricMPIMode;
  char* StateFileName;  // loading state file(Bug #5711)
  char* TestPlugin; // to load plugins from command line for tests
  char* TestPluginPath;
  int DisableXDisplayTests;

  // inline setters
  vtkSetStringMacro(ServerURL);
  vtkSetStringMacro(StateFileName);
  vtkSetStringMacro(TestPlugin);
  vtkSetStringMacro(TestPluginPath);

private:
  int ConnectID;
  int UseOffscreenRendering;
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
  int UseCudaInterop;
  int SatelliteMessageIds;
  int PrintMonitors;
  int EnableStackTrace;
  int DisableRegistry;
  int ForceMPIInitOnClient;
  int ForceNoMPIInitOnClient;

  // inline setters
  vtkSetStringMacro(StereoType);

//ETX
private:
  vtkPVOptions(const vtkPVOptions&); // Not implemented
  void operator=(const vtkPVOptions&); // Not implemented

  vtkSetStringMacro(HostName);
  char* HostName;
};

#endif

