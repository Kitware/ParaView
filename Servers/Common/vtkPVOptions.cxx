/*=========================================================================

  Module:    vtkPVOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOptions.h"

#include "vtkObjectFactory.h"

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>


//----------------------------------------------------------------------------
//****************************************************************************
class vtkPVOptionsInternal
{
public:
  kwsys::CommandLineArguments CMD;
};
//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVOptions );
vtkCxxRevisionMacro(vtkPVOptions, "1.2");

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  // Initialize kwsys::CommandLineArguments
  this->Internals = new vtkPVOptionsInternal;
  this->Internals->CMD.SetUnknownArgumentCallback(vtkPVOptions::UnknownArgumentHandler);
  this->Internals->CMD.SetClientData(this);
  this->UnknownArgument = 0;

  this->CaveConfigurationFileName = 0;
  this->MachinesFileName = 0;
  this->CrashOnErrors = 0;
  this->AlwaysSSH = 0;
  this->RenderModuleName = NULL;
  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;

  this->UseTiledDisplay = 0;
  this->TileDimensions[0] = 0;
  this->TileDimensions[1] = 0;
  this->ClientMode = 0;
  this->ServerMode = 0;
  this->RenderServerMode = 0;
  this->HostName = NULL;
  this->RenderServerHostName = 0;
  this->SetRenderServerHostName("localhost");
  this->SetHostName("localhost");
  this->Port = 11111;
  this->RenderServerPort = 22221;
  this->RenderNodePort = 0;
  this->ReverseConnection = 0;
  this->Username = 0;
  this->UseSoftwareRendering = 0;
  this->UseSatelliteSoftwareRendering = 0;
  this->UseStereoRendering = 0;
  this->UseOffscreenRendering = 0;
  this->StartEmpty = 0;
  this->DisableComposite = 0;
  this->PlayDemoFlag = 0;
  this->DisableRegistry = 0;
  this->ClientRenderServer = 0;
  this->ConnectRenderToData = 0;
  this->ConnectDataToRender = 0;
  this->HelpSelected = 0;
  this->ConnectID = 0;
  this->BatchScriptName = 0;
}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions()
{
  this->SetRenderModuleName(0);
  this->SetCaveConfigurationFileName(NULL);
  this->SetGroupFileName(0);
  this->SetRenderServerHostName(NULL);
  this->SetHostName(NULL);
  this->SetUsername(0);
  this->SetMachinesFileName(0);
  this->SetBatchScriptName(0);
  
  // Remove internals
  this->SetUnknownArgument(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
const char* vtkPVOptions::GetHelp()
{
  int width = kwsys::SystemTools::GetTerminalWidth();
  if ( width < 9 )
    {
    width = 80;
    }

  this->Internals->CMD.SetLineLength(width);

  return this->Internals->CMD.GetHelp();
}

//----------------------------------------------------------------------------
void vtkPVOptions::Initialize()
{
  typedef kwsys::CommandLineArguments argT;
  this->Internals->CMD.AddBooleanArgument("--server", &this->ServerMode,
    "Start ParaView as a server (use MPI run).");
  this->Internals->CMD.AddBooleanArgument("-v", &this->ServerMode,
    "--server");
  this->Internals->CMD.AddBooleanArgument("--render-server", &this->RenderServerMode,
    "Start ParaView as a render server (use MPI run).");
  this->Internals->CMD.AddBooleanArgument("-rs", &this->RenderServerMode,
    "--render-server");
  this->Internals->CMD.AddArgument("--connect-id", argT::EQUAL_ARGUMENT, &this->ConnectID,
    "Set the ID of the server and client to make sure they match.");
  this->Internals->CMD.AddArgument("--render-module", argT::EQUAL_ARGUMENT, &this->RenderModuleName,
    "User specified rendering module.");
  this->Internals->CMD.AddArgument("--cave-configuration", argT::EQUAL_ARGUMENT, &this->CaveConfigurationFileName,
    "Specify the file that defines the displays for a cave. It is used only with CaveRenderModule.");
  this->Internals->CMD.AddArgument("-cc", argT::EQUAL_ARGUMENT, &this->CaveConfigurationFileName,
    "--cave-configuration");
  this->Internals->CMD.AddArgument("--batch", argT::EQUAL_ARGUMENT, &this->BatchScriptName,
    "Load and run the batch script specified.");
  this->Internals->CMD.AddArgument("-b", argT::EQUAL_ARGUMENT, &this->BatchScriptName,
    "--batch");
  this->Internals->CMD.AddBooleanArgument("--use-offscreen-rendering", &this->UseOffscreenRendering,
    "Render offscreen on the satellite processes. This option only works with software rendering or mangled mesa on Unix.");
  this->Internals->CMD.AddBooleanArgument("--play-demo", &this->PlayDemoFlag,
    "Run the ParaView demo.");
  this->Internals->CMD.AddBooleanArgument("-pd", &this->PlayDemoFlag,
    "--play-demo");
  this->Internals->CMD.AddBooleanArgument("--disable-registry", &this->DisableRegistry,
    "Do not use registry when running ParaView (for testing).");
  this->Internals->CMD.AddBooleanArgument("-dr", &this->DisableRegistry,
    "--disable-registry");
  this->Internals->CMD.AddBooleanArgument("--stereo", &this->UseStereoRendering,
    "Tell the application to enable stereo rendering (only when running on a single process).");
  this->Internals->CMD.AddBooleanArgument("--client", &this->ClientMode,
    "Run ParaView as client (MPI run, 1 process) (ParaView Server must be started first).");
  this->Internals->CMD.AddBooleanArgument("-c", &this->ClientMode,
    "--client");
  this->Internals->CMD.AddBooleanArgument("--client-render-server", &this->ClientRenderServer,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->Internals->CMD.AddBooleanArgument("-crs", &this->ClientRenderServer,
    "--client-render-server");
  this->Internals->CMD.AddBooleanArgument("--connect-render-to-data", &this->ConnectRenderToData,
    "Run ParaView as a client to a data and render server. The data server will wait for the render server.");
  this->Internals->CMD.AddBooleanArgument("-r2d", &this->ConnectRenderToData,
    "--connect-render-to-data");
  this->Internals->CMD.AddBooleanArgument("--connect-data-to-render", &this->ConnectDataToRender,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->Internals->CMD.AddBooleanArgument("-r2d", &this->ConnectDataToRender,
    "--connect-data-to-render");
  this->Internals->CMD.AddArgument("--user", argT::EQUAL_ARGUMENT, &this->Username, 
    "Tell the client what username to send to server when establishing SSH connection.");
  this->Internals->CMD.AddArgument("--host", argT::EQUAL_ARGUMENT, &this->HostName, 
    "Tell the client where to look for the server (default: localhost). Used with --client option or --server -rc options.");
  this->Internals->CMD.AddArgument("--h", argT::EQUAL_ARGUMENT, &this->HostName, 
    "--host");
  this->Internals->CMD.AddArgument("--render-server-host", argT::EQUAL_ARGUMENT, &this->RenderServerHostName, 
    "Tell the client where to look for the render server (default: localhost). Used with --client option.");
  this->Internals->CMD.AddArgument("-rsh", argT::EQUAL_ARGUMENT, &this->RenderServerHostName, 
    "--render-server-host");
  this->Internals->CMD.AddArgument("--port", argT::EQUAL_ARGUMENT, &this->Port, 
    "Specify the port client and server will use (--port=11111).  Client and servers ports must match.");
  this->Internals->CMD.AddArgument("--machines", argT::EQUAL_ARGUMENT, &this->MachinesFileName, 
    "Specify the network configurations file for the render server.");
  this->Internals->CMD.AddArgument("-m", argT::EQUAL_ARGUMENT, &this->MachinesFileName, 
    "--machines");
  this->Internals->CMD.AddArgument("--render-node-port", argT::EQUAL_ARGUMENT, &this->RenderNodePort, 
    "Specify the port to be used by each render node (--render-node-port=22222).  Client and render servers ports must match.");
  this->Internals->CMD.AddArgument("--render-port", argT::EQUAL_ARGUMENT, &this->RenderServerPort, 
    "Specify the port client and render server will use (--port=22222).  Client and render servers ports must match.");
  this->Internals->CMD.AddBooleanArgument("--crash-on-errors", &this->CrashOnErrors, 
    "For debugging purposes. This will make ParaView abort on errors.");
  this->Internals->CMD.AddBooleanArgument("--disable-composite", &this->DisableComposite, 
    "Use this option when rendering resources are not available on the server.");
  this->Internals->CMD.AddBooleanArgument("-dc", &this->DisableComposite, 
    "--disable-composite");

  this->Internals->CMD.AddBooleanArgument("--use-software-rendering", &this->UseSoftwareRendering, 
    "Use software (Mesa) rendering (supports off-screen rendering).");
  this->Internals->CMD.AddBooleanArgument("-r", &this->UseSoftwareRendering, 
    "--use-software-rendering");
  this->Internals->CMD.AddBooleanArgument("--use-satellite-rendering", &this->UseSatelliteSoftwareRendering, 
    "Use software (Mesa) rendering (supports off-screen rendering) only on satellite processes.");
  this->Internals->CMD.AddBooleanArgument("-s", &this->UseSatelliteSoftwareRendering, 
    "--use-satellite-rendering");
  this->Internals->CMD.AddBooleanArgument("--help", &this->HelpSelected, 
    "Displays available command line arguments.");
  this->Internals->CMD.AddBooleanArgument("--reverse-connection", &this->ReverseConnection, 
    "Have the server connect to the client.");
  this->Internals->CMD.AddBooleanArgument("-rc", &this->ReverseConnection,
    "--reverse-connection");
  this->Internals->CMD.AddBooleanArgument("--always-ssh", &this->AlwaysSSH, 
    "Always use SSH.");
  this->Internals->CMD.AddBooleanArgument("--use-tiled-display", &this->UseTiledDisplay, 
    "Duplicate the final data to all nodes and tile node displays 1-N into one large display.");
  this->Internals->CMD.AddBooleanArgument("-td", &this->UseTiledDisplay, 
    "--use-tiled-display");
  this->Internals->CMD.AddBooleanArgument("--start-empty", &this->StartEmpty, 
    "Start ParaView without any default modules.");
  this->Internals->CMD.AddBooleanArgument("-e", &this->StartEmpty, 
    "--start-empty");
  this->Internals->CMD.AddArgument("--tile-dimensions-x", argT::EQUAL_ARGUMENT, this->TileDimensions, 
    "Size of tile display in the number of displays in each row of the display.");
  this->Internals->CMD.AddArgument("-tdx", argT::EQUAL_ARGUMENT, this->TileDimensions, 
    "--tile-dimensions-x");
  this->Internals->CMD.AddArgument("--tile-dimensions-y", argT::EQUAL_ARGUMENT, this->TileDimensions+1, 
    "Size of tile display in the number of displays in each column of the display.");
  this->Internals->CMD.AddArgument("-tdy", argT::EQUAL_ARGUMENT, this->TileDimensions+1, 
    "--tile-dimensions-y");

  // Temporarily removing this (for the release - it has bugs)
  /*
  this->Internals->CMD.AddBooleanArgument("--use-rendering-group", &this->UseRenderingGroup, 
    "Use a subset of processes to render.");
  this->Internals->CMD.AddBooleanArgument("-p", &this->UseRenderingGroup, 
    "--use-rendering-group");
  this->Internals->CMD.AddArgument("--group-file", arg%::EQUAL_ARGUMENT, &this->GroupFileName, 
    "Group file is the name of the input file listing number of processors to render on.");
  this->Internals->CMD.AddArgument("--group-file", arg%::EQUAL_ARGUMENT, &this->GroupFileName, 
    "--group-file");
    */
}

//----------------------------------------------------------------------------
int vtkPVOptions::PostProcess()
{
  if ( this->CaveConfigurationFileName )
    {
    this->SetRenderModuleName("CaveRenderModule");
    }
  if ( this->BatchScriptName && 
    kwsys::SystemTools::GetFilenameLastExtension(this->BatchScriptName) != ".pvb")
    {
    ostrstream str;
    str << "Wrong batch script name: " << this->BatchScriptName << ends;
    this->SetErrorMessage(str.str());
    str.rdbuf()->freeze(0);
    return 0;
    }
  if ( this->ClientRenderServer )
    {
    this->ClientMode = 1;
    this->RenderServerMode = 1;
    }
  if ( this->ConnectDataToRender )
    {
    this->ClientMode = 1;
    this->RenderServerMode = 1;
    }
  if ( this->ConnectRenderToData )
    {
    this->ClientMode = 1;
    this->RenderServerMode = 2;
    }
  if ( this->UseSatelliteSoftwareRendering )
    {
    this->UseSoftwareRendering = 1;
    }
  if ( getenv("PV_SOFTWARE_RENDERING") )
    {
    this->UseSoftwareRendering = 1;
    this->UseSatelliteSoftwareRendering = 1;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::WrongArgument(const char* argument)
{
  if ( kwsys::SystemTools::FileExists(argument) &&
    kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvs")
    {
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVOptions::Parse(int argc, const char* const argv[])
{
  this->Internals->CMD.Initialize(argc, argv);
  this->Initialize();
  int res1 = this->Internals->CMD.Parse();
  int res2 = this->PostProcess();
  cout << "Res1: " << res1 << " Res2: " << res2 << endl;
  return res1 && res2;
}

//----------------------------------------------------------------------------
int vtkPVOptions::UnknownArgumentHandler(const char* argument, void* call_data)
{
  cout << "UnknownArgumentHandler: " << argument << endl;
  vtkPVOptions* self = static_cast<vtkPVOptions*>(call_data);
  if ( self )
    {
    self->SetUnknownArgument(argument);
    return self->WrongArgument(argument);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PlayDemoFlag: " << this->PlayDemoFlag << endl;
  os << indent << "DisableRegistry: " << this->DisableRegistry << endl;
  os << indent << "BatchScriptName: " << (this->BatchScriptName?this->BatchScriptName:"(none)") << endl;
  os << indent << "CrashOnErrors: " << this->CrashOnErrors << endl;
  os << indent << "StartEmpty: " << this->StartEmpty << endl;
  os << indent << "HelpSelected: " << this->HelpSelected << endl;
  os << indent << "UnknownArgument: " << (this->UnknownArgument?this->UnknownArgument:"(none)") << endl;
  os << indent << "GroupFileName: " << (this->GroupFileName?this->UnknownArgument:"(none)") << endl;
  os << indent << "ErrorMessage: " << (this->ErrorMessage?this->ErrorMessage:"(none)") << endl;

  // Separate runtime informations so that it can be displayed in the about dialog
  this->AboutPrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVOptions::AboutPrintSelf(ostream& os, vtkIndent indent)
{
  if (this->ClientMode)
    {
    os << indent << "Running as a client\n";
    }

  if (this->ServerMode)
    {
    os << indent << "Running as a server\n";
    }

  if (this->ClientRenderServer)
    {
    os << indent << "Running as a client connected to a render server\n";
    }

  if (this->RenderServerMode)
    {
    os << indent << "Running as a render server\n";
    }

  if (this->ConnectDataToRender)
    {
    os << indent << "Running as a client to a data and render server\n";
    }

  if (this->ConnectDataToRender)
    {
    os << indent << "Running as a client to a data and render server\n";
    }

  if (this->ClientMode || this->ServerMode || this->RenderServerMode 
   || this->RenderServerMode || this->ConnectDataToRender || this->ConnectDataToRender)
    {
    os << indent << "ConnectID is: " << this->ConnectID << endl;
    os << indent << "Port: " << this->Port << endl;
    os << indent << "Render Node Port: " << this->RenderNodePort << endl;
    os << indent << "Render Server Port: " << this->RenderServerPort << endl;
    os << indent << "Reverse Connection: " << (this->ReverseConnection?"on":"off") << endl;
    os << indent << "Host: " << (this->HostName?this->HostName:"(none)") << endl;
    os << indent << "Render Host: " << (this->RenderServerHostName?this->RenderServerHostName:"(none)") << endl;
    os << indent << "Username: " 
       << (this->Username?this->Username:"(none)") << endl;
    if(this->AlwaysSSH)
      {
      os << indent << "Always using SSH";
      }
    }

  if (this->UseSoftwareRendering)
    {
    os << indent << "Using Software Rendering\n";
    }
  if (this->UseSatelliteSoftwareRendering)
    {
    os << indent << "Using SatelliteSoftwareRendering\n";
    }
  if(this->UseStereoRendering)
    {
    os << indent << "Using Stereo Rendering\n";
    }
  if(this->UseOffscreenRendering)
    {
    os << indent << "Using Offscreen Rendering\n"; 
    }
  if (this->UseTiledDisplay)
    { 
    os << indent << "Using Tiled Display" << endl;
    os << indent << "With Tile Dimensions: " << this->TileDimensions[0]
       << ", " << this->TileDimensions[1] << endl;
    }
  if(this->UseRenderingGroup)
    {
    os << indent << "Using RenderingGroup: Enabled\n";
    }
  if( this->RenderModuleName )
    {
    os << indent << "Render Module: " << this->RenderModuleName << endl;
    }
  if(this->MachinesFileName)
    {
    os << indent << "Network configuration: " << this->MachinesFileName << endl;
    }
  if(this->CaveConfigurationFileName)
    {
    os << indent << "Specify cave configuration: " << this->CaveConfigurationFileName << endl;
    }
  os << indent << "Composite is: " << (this->DisableComposite?"Disabled":"Enabled") << endl;

}
