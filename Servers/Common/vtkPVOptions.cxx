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
vtkStandardNewMacro(vtkPVOptions);
vtkCxxRevisionMacro(vtkPVOptions, "1.12");

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  // Initialize kwsys::CommandLineArguments
  this->Internals = new vtkPVOptionsInternal;
  this->Internals->CMD.SetUnknownArgumentCallback(vtkPVOptions::UnknownArgumentHandler);
  this->Internals->CMD.SetClientData(this);
  this->UnknownArgument = 0;
  this->ErrorMessage = 0;
  this->Argc = 0;
  this->Argv = 0;

  this->CaveConfigurationFileName = 0;
  this->MachinesFileName = 0;
  this->AlwaysSSH = 0;
  this->RenderModuleName = NULL;
  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;

  this->UseTiledDisplay = 0;
  this->TileDimensions[0] = 0;
  this->TileDimensions[1] = 0;
  this->ClientMode = 0;
  this->ServerMode = 1;
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
  this->DisableComposite = 0;
  this->ClientRenderServer = 0;
  this->ConnectRenderToData = 0;
  this->ConnectDataToRender = 0;
  this->HelpSelected = 0;
  this->ConnectID = 0;
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
  
  // Remove internals
  this->SetUnknownArgument(0);
  this->CleanArgcArgv();
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
  this->AddBooleanArgument("--render-server", "-rs", &this->RenderServerMode,
    "Start ParaView as a render server (use MPI run).");
  this->AddArgument("--connect-id", 0, &this->ConnectID,
    "Set the ID of the server and client to make sure they match.");
  this->AddArgument("--render-module", 0, &this->RenderModuleName,
    "User specified rendering module.");
  this->AddArgument("--cave-configuration", "-cc", &this->CaveConfigurationFileName,
    "Specify the file that defines the displays for a cave. It is used only with CaveRenderModule.");
  this->AddBooleanArgument("--use-offscreen-rendering", 0, &this->UseOffscreenRendering,
    "Render offscreen on the satellite processes. This option only works with software rendering or mangled mesa on Unix.");
  this->AddBooleanArgument("--stereo", 0, &this->UseStereoRendering,
    "Tell the application to enable stereo rendering (only when running on a single process).");
  this->AddBooleanArgument("--client-render-server", "-crs", &this->ClientRenderServer,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->AddBooleanArgument("--connect-render-to-data", "-r2d", &this->ConnectRenderToData,
    "Run ParaView as a client to a data and render server. The data server will wait for the render server.");
  this->AddBooleanArgument("--connect-data-to-render", "-d2r", &this->ConnectDataToRender,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->AddArgument("--user", 0, &this->Username, 
    "Tell the client what username to send to server when establishing SSH connection.");
  this->AddArgument("--host", "-h", &this->HostName, 
    "Tell the client where to look for the server (default: localhost). Used with --client option or --server -rc options.");
  this->AddArgument("--render-server-host", "-rsh", &this->RenderServerHostName, 
    "Tell the client where to look for the render server (default: localhost). Used with --client option.");
  this->AddArgument("--port", 0, &this->Port, 
    "Specify the port client and server will use (--port=11111).  Client and servers ports must match.");
  this->AddArgument("--machines", "-m", &this->MachinesFileName, 
    "Specify the network configurations file for the render server.");
  this->AddArgument("--render-node-port", 0, &this->RenderNodePort, 
    "Specify the port to be used by each render node (--render-node-port=22222).  Client and render servers ports must match.");
  this->AddArgument("--render-port", 0, &this->RenderServerPort, 
    "Specify the port client and render server will use (--port=22222).  Client and render servers ports must match.");
  this->AddBooleanArgument("--disable-composite", "-dc", &this->DisableComposite, 
    "Use this option when rendering resources are not available on the server.");

  this->AddBooleanArgument("--use-software-rendering", "-r", &this->UseSoftwareRendering, 
    "Use software (Mesa) rendering (supports off-screen rendering).");
  this->AddBooleanArgument("--use-satellite-rendering", "-s", &this->UseSatelliteSoftwareRendering, 
    "Use software (Mesa) rendering (supports off-screen rendering) only on satellite processes.");
  this->AddBooleanArgument("--reverse-connection", "-rc", &this->ReverseConnection, 
    "Have the server connect to the client.");
  this->AddBooleanArgument("--always-ssh", 0, &this->AlwaysSSH, 
    "Always use SSH.");
  this->AddBooleanArgument("--use-tiled-display", "-td", &this->UseTiledDisplay, 
    "Duplicate the final data to all nodes and tile node displays 1-N into one large display.");
  this->AddArgument("--tile-dimensions-x", "-tdx", this->TileDimensions, 
    "Size of tile display in the number of displays in each row of the display.");
  this->AddArgument("--tile-dimensions-y", "-tdy", this->TileDimensions+1, 
    "Size of tile display in the number of displays in each column of the display.");

  // Temporarily removing this (for the release - it has bugs)
  /*
  this->AddBooleanArgument("--use-rendering-group", "-p", &this->UseRenderingGroup, 
    "Use a subset of processes to render.");
  this->AddArgument("--group-file", "-gf", &this->GroupFileName, 
    "Group file is the name of the input file listing number of processors to render on.");
    */
}

//----------------------------------------------------------------------------
int vtkPVOptions::PostProcess()
{
  if ( this->CaveConfigurationFileName )
    {
    this->SetRenderModuleName("CaveRenderModule");
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
int vtkPVOptions::WrongArgument(const char*)
{
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVOptions::Parse(int argc, const char* const argv[])
{
  this->Internals->CMD.Initialize(argc, argv);
  this->Initialize();
  this->AddBooleanArgument("--help", "/?", &this->HelpSelected, 
    "Displays available command line arguments.");
  int res1 = this->Internals->CMD.Parse();
  int res2 = this->PostProcess();
  //cout << "Res1: " << res1 << " Res2: " << res2 << endl;
  this->CleanArgcArgv();
  this->Internals->CMD.GetRemainingArguments(&this->Argc, &this->Argv);
  return res1 && res2;
}

//----------------------------------------------------------------------------
void vtkPVOptions::CleanArgcArgv()
{
  int cc;
  if ( this->Argv )
    {
    for ( cc = 0; cc < this->Argc; cc ++ )
      {
      delete [] this->Argv[cc];
      }
    delete [] this->Argv;
    this->Argv = 0;
    }
}
//----------------------------------------------------------------------------
void vtkPVOptions::AddBooleanArgument(const char* longarg, const char* shortarg, int* var, const char* help)
{
  this->Internals->CMD.AddBooleanArgument(longarg, var, help);
  if ( shortarg )
    {
    this->Internals->CMD.AddBooleanArgument(shortarg, var, longarg);
    }
}

//----------------------------------------------------------------------------
void vtkPVOptions::AddArgument(const char* longarg, const char* shortarg, int* var, const char* help)
{
  typedef kwsys::CommandLineArguments argT;
  this->Internals->CMD.AddArgument(longarg, argT::EQUAL_ARGUMENT, var, help);
  if ( shortarg )
    {
    this->Internals->CMD.AddArgument(shortarg, argT::EQUAL_ARGUMENT, var, longarg);
    }
}

//----------------------------------------------------------------------------
void vtkPVOptions::AddArgument(const char* longarg, const char* shortarg, char** var, const char* help)
{
  typedef kwsys::CommandLineArguments argT;
  this->Internals->CMD.AddArgument(longarg, argT::EQUAL_ARGUMENT, var, help);
  if ( shortarg )
    {
    this->Internals->CMD.AddArgument(shortarg, argT::EQUAL_ARGUMENT, var, longarg);
    }
}

//----------------------------------------------------------------------------
int vtkPVOptions::UnknownArgumentHandler(const char* argument, void* call_data)
{
  //cout << "UnknownArgumentHandler: " << argument << endl;
  vtkPVOptions* self = static_cast<vtkPVOptions*>(call_data);
  if ( self )
    {
    self->SetUnknownArgument(argument);
    return self->WrongArgument(argument);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVOptions::GetRemainingArguments(int* argc, char*** argv)
{
  *argc = this->Argc;
  *argv = this->Argv;
}

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UnknownArgument: " << (this->UnknownArgument?this->UnknownArgument:"(none)") << endl;
  os << indent << "ErrorMessage: " << (this->ErrorMessage?this->ErrorMessage:"(none)") << endl;
  os << indent << "HelpSelected: " << this->HelpSelected << endl;
  os << indent << "GroupFileName: " << (this->GroupFileName?this->GroupFileName:"(none)") << endl;

  // Everything after this line will be showned in Help/About dialog
  os << indent << "Runtime information:" << endl; //important please leave it here, for more info: vtkPVApplication::AddAboutText

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

  os << indent << "Software Rendering: " << (this->UseSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Satellite Software Rendering: " << (this->UseSatelliteSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Stereo Rendering: " << (this->UseStereoRendering?"Enabled":"Disabled") << endl;

  os << indent << "Offscreen Rendering: " << (this->UseOffscreenRendering?"Enabled":"Disabled") << endl;

  os << indent << "Tiled Display: " << (this->UseTiledDisplay?"Enabled":"Disabled") << endl;
  if (this->UseTiledDisplay)
    { 
    os << indent << "With Tile Dimensions: " << this->TileDimensions[0]
       << ", " << this->TileDimensions[1] << endl;
    }

  os << indent << "Using RenderingGroup: " << (this->UseRenderingGroup?"Enabled":"Disabled") << endl;

  os << indent << "Render Module Used: " << (this->RenderModuleName?this->RenderModuleName:"(none)") << endl;

  os << indent << "Network Configuration: " << (this->MachinesFileName?this->MachinesFileName:"(none)") << endl;

  os << indent << "Cave Configuration: " << (this->CaveConfigurationFileName?this->CaveConfigurationFileName:"(none)") << endl;

  os << indent << "Compositing: " << (this->DisableComposite?"Disabled":"Enabled") << endl;

}
