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
  enum ProcessTypeEnum
  {
    PARAVIEW = 0x1,
    PVCLIENT = 0x2,
    PVSERVER = 0x4,
    PVRENDER_SERVER = 0x8,
    PVDATA_SERVER = 0x10,
    PVBATCH = 0x20,
    XMLONLY = 0x40,
    ALLPROCESS = PARAVIEW | PVCLIENT | PVSERVER | PVRENDER_SERVER | PVDATA_SERVER
  };
  
  static vtkPVOptions* New();
  vtkTypeRevisionMacro(vtkPVOptions,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  int Parse(int argc, const char* const argv[]);
  void GetRemainingArguments(int* argc, char** argv[]);
  const char* GetHelp();

  vtkGetMacro(ServerMode, int);
  vtkGetMacro(RenderServerMode, int);
  vtkGetMacro(ConnectID, int);
  vtkGetMacro(UseOffscreenRendering, int);
  vtkGetMacro(UseStereoRendering, int);
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
  vtkGetMacro(HelpSelected, int);
  vtkGetMacro(ReverseConnection, int);
  vtkGetMacro(UseRenderingGroup, int);
  vtkGetVector2Macro(TileDimensions, int);
  vtkGetStringMacro(RenderModuleName);
  vtkGetStringMacro(CaveConfigurationFileName);
  vtkGetStringMacro(MachinesFileName);
  vtkGetStringMacro(GroupFileName);
  vtkGetStringMacro(ParaViewDataName);

  // Description:
  // Get the various types of host names. 
  vtkGetStringMacro(ServerHostName);
  vtkGetStringMacro(DataServerHostName);
  vtkGetStringMacro(RenderServerHostName);
  vtkGetStringMacro(ClientHostName);
  // Description:
  // Set/Get the type of the process for this set of options.
  // data-server, render-server, combined-server or client.
  ProcessTypeEnum GetProcessType() { return this->ProcessType;}
  void SetProcessType(ProcessTypeEnum p) {this->ProcessType = p;}

  // Description:
  // vtkProcessModule needs to set the render module name
  vtkSetStringMacro(RenderModuleName);

  // Description:
  // vtkPVProcessModule needs to set this.
  vtkSetVector2Macro(TileDimensions, int);
  vtkSetMacro(UseOffscreenRendering, int);

  // Description:
  // In case of unknown argument, set this variable with the unknown argument.
  vtkGetStringMacro(UnknownArgument);

  // Description:
  // Get the error message if Parse returned 0.
  vtkGetStringMacro(ErrorMessage);

  // Description:
  // Get argv[0]
  const char* GetArgv0();

  // Description:
  // Get the index of the last argument parsed.
  int GetLastArgument();

  // Description:
  // Pass in the name and the attributes for all tags that are not Options.
  // If it returns 1, then it is successful, and 0 if it failed.
  virtual int ParseExtraXMLTag(const char* , const char** ) {return 1;}
  // Description:
  // Is this in render server mode.
  vtkGetMacro(ClientRenderServer, int);

protected:
  // Description:
  // Default constructor.
  vtkPVOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVOptions();

  // Description:
  // Add a command line option.  For each argument added there is a long
  // version --long and a short version -l, a help string, and a variable
  // that is set to the value of the option.  The types can be int, char*, or
  // boolean (set to 1 of option is present).  Also deprecated arguments can
  // be added with only a help string.  The help string should say that the
  // argument is deprecated and suggest the alternative argument to use.
  // Each option can specify in a bit flag int the processes that the option
  // is valid for, the default is to be valid for all paraview processes.
  void AddBooleanArgument(const char* longarg, const char* shortarg,
                          int* var, const char* help, int type=ALLPROCESS);
  void AddDeprecatedArgument(const char* longarg, const char* shortarg,
                             const char* help, int type=ALLPROCESS);
  void AddArgument(const char* longarg, const char* shortarg,
                   int* var, const char* help, int type=ALLPROCESS);
  void AddArgument(const char* longarg, const char* shortarg,
                   char** var, const char* help, int type=ALLPROCESS);
  
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
  // This method loads the paraview config file.  The command line
  // will override any of the values in this file, but all options can
  // be in the file.
  int LoadXMLConfigFile(const char*);

  vtkSetStringMacro(UnknownArgument);
  char* UnknownArgument;

  vtkSetStringMacro(ErrorMessage);
  char* ErrorMessage;

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
  int HelpSelected;

private:
  ProcessTypeEnum ProcessType; // data-server, render-server, combined-server, client
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
  int UseRenderingGroup;

  
  char* RenderModuleName;

  vtkSetStringMacro(CaveConfigurationFileName);
  char* CaveConfigurationFileName;

  vtkSetStringMacro(MachinesFileName);
  char* MachinesFileName;

  vtkSetStringMacro(GroupFileName);
  char* GroupFileName;

  vtkSetStringMacro(XMLConfigFile);
  char* XMLConfigFile;

private:
  vtkPVOptions(const vtkPVOptions&); // Not implemented
  void operator=(const vtkPVOptions&); // Not implemented

  vtkPVOptionsInternal* Internals;
  static int UnknownArgumentHandler(const char* argument, void* call_data);
  static int DeprecatedArgumentHandler(const char* argument, const char* value, void* call_data);

  void CleanArgcArgv();
  int Argc;
  char** Argv;
};

#endif // #ifndef __vtkPVOptions_h

