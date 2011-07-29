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

#include "vtkCommandOptions.h"

class vtkPVOptionsInternal;

class VTK_EXPORT vtkPVOptions : public vtkCommandOptions
{
protected:
//BTX
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
  friend class vtkPVOptionsXMLParser;
//ETX
public:

  static vtkPVOptions* New();
  vtkTypeMacro(vtkPVOptions,vtkCommandOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetMacro(ServerMode, int);
  vtkGetMacro(RenderServerMode, int);
  vtkGetMacro(ConnectID, int);
  vtkGetMacro(UseOffscreenRendering, int);
  vtkGetMacro(UseStereoRendering, int);
  vtkGetStringMacro(StereoType);

  vtkGetMacro(ClientMode, int);
  // Description:
  // Get Various ports.
  vtkGetMacro(ServerPort, int);
  vtkGetMacro(DataServerPort, int);
  vtkGetMacro(RenderServerPort, int);
  vtkGetMacro(RenderNodePort, int);

  vtkGetMacro(DisableComposite, int);
  vtkGetMacro(UseSoftwareRendering, int);
  vtkGetMacro(UseSatelliteSoftwareRendering, int);
  vtkGetMacro(ReverseConnection, int);
  vtkGetMacro(UseRenderingGroup, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileMullions, int);
  vtkGetStringMacro(RenderModuleName);
  vtkGetStringMacro(CaveConfigurationFileName);
  vtkGetStringMacro(MachinesFileName);
  vtkGetStringMacro(GroupFileName);
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
  // Get the various types of host names.
  vtkGetStringMacro(ServerHostName);
  vtkGetStringMacro(DataServerHostName);
  vtkGetStringMacro(RenderServerHostName);
  vtkGetStringMacro(ClientHostName);

  // Description:
  // vtkProcessModule needs to set the render module name
  vtkSetStringMacro(RenderModuleName);

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
  // Is this in render server mode.
  vtkGetMacro(ClientRenderServer, int);

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
  vtkSetStringMacro(ParaViewDataName);
  char* ParaViewDataName;

  vtkSetStringMacro(RenderServerHostName);
  char* RenderServerHostName;

  vtkSetStringMacro(ClientHostName);
  char* ClientHostName;

  vtkSetStringMacro(DataServerHostName);
  char* DataServerHostName;

  vtkSetStringMacro(ServerHostName);
  char* ServerHostName;

  // Port information
  int ServerPort;
  int DataServerPort;
  int RenderServerPort;
  int RenderNodePort;

  int ServerMode;
  int ClientMode;
  int RenderServerMode;

  int SymmetricMPIMode;

  // Command Option for loading state file(Bug #5711)
  vtkSetStringMacro(StateFileName);
  char* StateFileName;

private:
  // Options:
  int ClientRenderServer;
  int ConnectRenderToData;
  int ConnectDataToRender;
  int ConnectID;
  int UseOffscreenRendering;
  int UseStereoRendering;
  int DisableComposite;
  int UseSoftwareRendering;
  int UseSatelliteSoftwareRendering;
  int ReverseConnection;
  int TileDimensions[2];
  int TileMullions[2];
  int UseRenderingGroup;
  int Timeout;


  char* RenderModuleName;

  vtkSetStringMacro(CaveConfigurationFileName);
  char* CaveConfigurationFileName;

  vtkSetStringMacro(MachinesFileName);
  char* MachinesFileName;

  vtkSetStringMacro(GroupFileName);
  char* GroupFileName;

  char* LogFileName;
  int TellVersion;

  vtkSetStringMacro(StereoType);
  char* StereoType;

//ETX
private:
  vtkPVOptions(const vtkPVOptions&); // Not implemented
  void operator=(const vtkPVOptions&); // Not implemented
};

#endif

