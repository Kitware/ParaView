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
// 
// .SECTION See Also
// kwsys::CommandLineArguments

#ifndef __vtkPVOptions_h
#define __vtkPVOptions_h

#include "vtkObject.h"

class vtkPVOptionsInternal;

class VTK_EXPORT vtkPVOptions : public vtkObject
{
public:
  static vtkPVOptions* New();
  vtkTypeRevisionMacro(vtkPVOptions,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  int Parse(int argc, const char* const argv[]);
  const char* GetHelp();

  vtkGetMacro(ServerMode, int);
  vtkGetMacro(RenderServerMode, int);
  vtkGetMacro(ConnectID, int);
  vtkGetMacro(UseOffscreenRendering, int);
  vtkGetMacro(UseStereoRendering, int);
  vtkGetMacro(ClientMode, int);
  vtkGetMacro(ClientRenderServer, int);
  vtkGetMacro(Port, int);
  vtkGetMacro(RenderNodePort, int);
  vtkGetMacro(RenderServerPort, int);
  vtkGetMacro(DisableComposite, int);
  vtkGetMacro(UseSoftwareRendering, int);
  vtkGetMacro(UseSatelliteSoftwareRendering, int);
  vtkGetMacro(HelpSelected, int);
  vtkGetMacro(ReverseConnection, int);
  vtkGetMacro(AlwaysSSH, int);
  vtkGetMacro(UseTiledDisplay, int);
  vtkGetMacro(UseRenderingGroup, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkGetStringMacro(RenderModuleName);
  vtkGetStringMacro(CaveConfigurationFileName);
  vtkGetStringMacro(Username);
  vtkGetStringMacro(HostName);
  vtkGetStringMacro(RenderServerHostName);
  vtkGetStringMacro(MachinesFileName);
  vtkGetStringMacro(GroupFileName);

  // Description:
  // In case of unknown argument, set this variable with the unknown argument.
  vtkGetStringMacro(UnknownArgument);

  // Description:
  // Get the error message if Parse returned 0.
  vtkGetStringMacro(ErrorMessage);

protected:
  // Description:
  // Default constructor.
  vtkPVOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVOptions();

  // Description:
  // Add option.
  void AddBooleanArgument(const char* longarg, const char* shortarg, int* var, const char* help);
  void AddArgument(const char* longarg, const char* shortarg, int* var, const char* help);
  void AddArgument(const char* longarg, const char* shortarg, char** var, const char* help);

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess();

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  vtkSetStringMacro(UnknownArgument);
  char* UnknownArgument;

  vtkSetStringMacro(ErrorMessage);
  char* ErrorMessage;

  // Description:
  // Subclasses may need to access these
  vtkSetStringMacro(Username);
  char* Username;

  vtkSetStringMacro(HostName);
  char* HostName;

  int Port;

  int ServerMode;
  int ClientMode;

private:
  // Options:
  int RenderServerMode;
  int ConnectID;
  int UseOffscreenRendering;
  int UseStereoRendering;
  int ClientRenderServer;
  int ConnectRenderToData;
  int ConnectDataToRender;
  int RenderNodePort;
  int RenderServerPort;
  int DisableComposite;
  int UseSoftwareRendering;
  int UseSatelliteSoftwareRendering;
  int HelpSelected;
  int ReverseConnection;
  int AlwaysSSH;
  int UseTiledDisplay;
  int TileDimensions[2];
  int UseRenderingGroup;

  vtkSetStringMacro(RenderModuleName);
  char* RenderModuleName;

  vtkSetStringMacro(CaveConfigurationFileName);
  char* CaveConfigurationFileName;

  vtkSetStringMacro(RenderServerHostName);
  char* RenderServerHostName;

  vtkSetStringMacro(MachinesFileName);
  char* MachinesFileName;

  vtkSetStringMacro(GroupFileName);
  char* GroupFileName;

private:
  vtkPVOptions(const vtkPVOptions&); // Not implemented
  void operator=(const vtkPVOptions&); // Not implemented

  vtkPVOptionsInternal* Internals;
  static int UnknownArgumentHandler(const char* argument, void* call_data);
};

#endif // #ifndef __vtkPVOptions_h

