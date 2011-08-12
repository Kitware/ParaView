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
#include "vtkPVConfig.h" //For PARAVIEW_ALWAYS_SECURE_CONNECTION option
#include "vtkPVOptionsXMLParser.h"
#include "vtkParallelRenderManager.h"
#include "vtkProcessModule.h"

#include <vtksys/CommandLineArguments.hxx>
#include <vtksys/SystemTools.hxx>



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->SetProcessType(ALLPROCESS);

  // Initialize vtksys::CommandLineArguments
  this->CaveConfigurationFileName = 0;
  this->MachinesFileName = 0;
  this->RenderModuleName = NULL;
  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;
  this->ParaViewDataName = 0;
  this->StateFileName = 0;

  this->ClientRenderServer = 0;
  this->ConnectRenderToData = 0;
  this->ConnectDataToRender = 0;


  this->TileDimensions[0] = 0;
  this->TileDimensions[1] = 0;
  this->TileMullions[0] = 0;
  this->TileMullions[1] = 0;
  this->ClientMode = 0;
  this->ServerMode = 0;
  this->RenderServerMode = 0;
  this->SymmetricMPIMode = 0;

  this->TellVersion = 0;

  // initialize host names
  this->ServerHostName = 0;
  this->SetServerHostName("localhost");
  this->DataServerHostName = 0;
  this->SetDataServerHostName("localhost");
  this->RenderServerHostName = 0;
  this->SetRenderServerHostName("localhost");
  this->ClientHostName = 0;
  this->SetClientHostName("localhost");
  // initialize ports to defaults
  this->ServerPort = 11111;
  this->DataServerPort = 11111;
  this->RenderServerPort = 22221;
  this->RenderNodePort = 0;  // this means pick a random port

  this->ReverseConnection = 0;
  this->UseSoftwareRendering = 0;
  this->UseSatelliteSoftwareRendering = 0;
  this->UseStereoRendering = 0;
  this->UseOffscreenRendering = 0;
  this->DisableComposite = 0;
  this->ConnectID = 0;
  this->LogFileName = 0;
  this->StereoType = 0;
  this->SetStereoType("Anaglyph");

  this->Timeout = 0;

  if (this->XMLParser)
    {
    this->XMLParser->Delete();
    this->XMLParser = 0;
    }
  this->XMLParser = vtkPVOptionsXMLParser::New();
  this->XMLParser->SetPVOptions(this);

}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions()
{
  this->SetRenderModuleName(0);
  this->SetCaveConfigurationFileName(NULL);
  this->SetGroupFileName(0);
  this->SetServerHostName(0);
  this->SetDataServerHostName(0);
  this->SetRenderServerHostName(0);
  this->SetClientHostName(0);
  this->SetMachinesFileName(0);
  this->SetStateFileName(0);
  this->SetLogFileName(0);
  this->SetStereoType(0);
  this->SetParaViewDataName(0);
}

//----------------------------------------------------------------------------
void vtkPVOptions::Initialize()
{
  switch (vtkProcessModule::GetProcessType())
    {
  case vtkProcessModule::PROCESS_CLIENT:
    this->SetProcessType(PVCLIENT);
    break;

  case vtkProcessModule::PROCESS_SERVER:
    this->SetProcessType(PVSERVER);
    break;

  case vtkProcessModule::PROCESS_DATA_SERVER:
    this->SetProcessType(PVDATA_SERVER);
    break;

  case vtkProcessModule::PROCESS_RENDER_SERVER:
    this->SetProcessType(PVRENDER_SERVER);
    break;

  case vtkProcessModule::PROCESS_BATCH:
    this->SetProcessType(PVBATCH);
    break;

  default:
    break;
    }

  this->AddArgument("--cslog", 0, &this->LogFileName,
                    "ClientServerStream log file.",
                    vtkPVOptions::ALLPROCESS);

  this->AddArgument("--data", 0, &this->ParaViewDataName,
                    "Load the specified data. "
                    "To specify file series replace the numeral with a '.' eg. "
                    "my0.vtk, my1.vtk...myN.vtk becomes my..vtk",
                    vtkPVOptions::PVCLIENT|vtkPVOptions::PARAVIEW);
  /*
  this->AddBooleanArgument("--client-render-server", "-crs", &this->ClientRenderServer,
                           "Run ParaView as a client to a data and render server."
                           " The render server will wait for the data server.",
                           vtkPVOptions::PVCLIENT);
  this->AddBooleanArgument("--connect-render-to-data", "-r2d", &this->ConnectRenderToData,
                           "Run ParaView as a client to a data and render server."
                           " The data server will wait for the render server.",
                           vtkPVOptions::PVCLIENT);
  this->AddBooleanArgument("--connect-data-to-render", "-d2r", &this->ConnectDataToRender,
                           "Run ParaView as a client to a data and render server."
                           " The render server will wait for the data server.",
                           vtkPVOptions::PVCLIENT);
  this->AddArgument("--render-server-host", "-rsh", &this->RenderServerHostName,
                    "Tell the client the host name of the render server (default: localhost).",
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--render-module", 0, &this->RenderModuleName,
                    "User specified rendering module.",
                    vtkPVOptions::PVCLIENT| vtkPVOptions::PVRENDER_SERVER
                    | vtkPVOptions::PVSERVER | vtkPVOptions::PARAVIEW);
  */

  this->AddArgument("--connect-id", 0, &this->ConnectID,
                    "Set the ID of the server and client to make sure they match.",
                    vtkPVOptions::PVCLIENT | vtkPVOptions::PVSERVER |
                    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER);
  this->AddBooleanArgument("--use-offscreen-rendering", 0, &this->UseOffscreenRendering,
                           "Render offscreen on the satellite processes."
                           " This option only works with software rendering or mangled mesa on Unix.",
                           vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER|vtkPVOptions::PVBATCH);

  this->AddBooleanArgument("--stereo", 0, &this->UseStereoRendering,
                           "Tell the application to enable stereo rendering",
                           vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);
  this->AddArgument("--stereo-type", 0, &this->StereoType,
                           "Specify the stereo type. This valid only when "
                           "--stereo is specified. Possible values are "
                           "\"Crystal Eyes\", \"Red-Blue\", \"Interlaced\", "
                           "\"Dresden\", \"Anaglyph\", \"Checkerboard\"",
                           vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);

  /*
  this->AddArgument("--server-host", "-sh", &this->ServerHostName,
                    "Tell the client the host name of the data server.",
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--data-server-host", "-dsh", &this->DataServerHostName,
                    "Tell the client the host name of the data server.",
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--render-server-host", "-rsh", &this->RenderServerHostName,
                    "Tell the client the host name of the render server.",
                    vtkPVOptions::PVCLIENT);
  */
  this->AddArgument("--client-host", "-ch", &this->ClientHostName,
                    "Tell the data|render server the host name of the client, use with -rc.",
                    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER |
                    vtkPVOptions::PVSERVER);
  this->AddArgument("--data-server-port", "-dsp", &this->DataServerPort,
                    "What port data server use to connect to the client. (default 11111).",
                    /*vtkPVOptions::PVCLIENT | */vtkPVOptions::PVDATA_SERVER);
  this->AddArgument("--render-server-port", "-rsp", &this->RenderServerPort,
                    "What port should the render server use to connect to the client. (default 22221).",
                    /*vtkPVOptions::PVCLIENT |*/ vtkPVOptions::PVRENDER_SERVER);
  this->AddArgument("--server-port", "-sp", &this->ServerPort,
                    "What port should the combined server use to connect to the client. (default 11111).",
                    /*vtkPVOptions::PVCLIENT |*/ vtkPVOptions::PVSERVER);

  this->AddArgument("--render-node-port", 0, &this->RenderNodePort,
                    "Specify the port to be used by each render node (--render-node-port=22222)."
                    "  Client and render servers ports must match.",
                    vtkPVOptions::XMLONLY);
  this->AddBooleanArgument("--disable-composite", "-dc", &this->DisableComposite,
                           "Use this option when rendering resources are not available on the server.",
                           vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--reverse-connection", "-rc", &this->ReverseConnection,
                           "Have the server connect to the client.",
                           vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER |
                           vtkPVOptions::PVSERVER);

  this->AddArgument("--tile-dimensions-x", "-tdx", this->TileDimensions,
                    "Size of tile display in the number of displays in each row of the display.",
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-dimensions-y", "-tdy", this->TileDimensions+1,
                    "Size of tile display in the number of displays in each column of the display.",
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-mullion-x", "-tmx", this->TileMullions,
                    "Size of the gap between columns in the tile display, in Pixels.",
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-mullion-y", "-tmy", this->TileMullions+1,
                    "Size of the gap between rows in the tile display, in Pixels.",
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);

  this->AddArgument("--timeout", 0, &this->Timeout,
                    "Time (in minutes) since connecting with a client "
                    "after which the server may timeout. The client typically shows warning "
                    "messages before the server times out.",
                    vtkPVOptions::PVDATA_SERVER|vtkPVOptions::PVSERVER);

  // Disabling for now since we don't support Cave anymore.
  // this->AddArgument("--cave-configuration", "-cc", &this->CaveConfigurationFileName,
  // "Specify the file that defines the displays for a cave. It is used only with CaveRenderModule.");

  this->AddArgument("--machines", "-m", &this->MachinesFileName,
                    "Specify the network configurations file for the render server.");

  this->AddBooleanArgument("--version", "-V", &this->TellVersion,
                           "Give the version number and exit.");

  // add new Command Option for loading StateFile (Bug #5711)
  this->AddArgument("--state", 0, &this->StateFileName,
    "Load the specified statefile (.pvsm).",
    vtkPVOptions::PVCLIENT|vtkPVOptions::PARAVIEW);

  this->AddBooleanArgument("--symmetric", "-sym",
    &this->SymmetricMPIMode,
    "When specified, the python script is processed symmetrically on all processes.",
    vtkPVOptions::PVBATCH);
}

//----------------------------------------------------------------------------
int vtkPVOptions::PostProcess(int, const char* const*)
{
  switch(this->GetProcessType())
    {
    case vtkPVOptions::PVCLIENT:
      this->ClientMode = 1;
      break;
    case vtkPVOptions::PARAVIEW:
      break;
    case vtkPVOptions::PVRENDER_SERVER:
      this->RenderServerMode = 1;
    case vtkPVOptions::PVDATA_SERVER:
    case vtkPVOptions::PVSERVER:
      this->ServerMode = 1;
      break;
    case vtkPVOptions::PVBATCH:
    case vtkPVOptions::XMLONLY:
    case vtkPVOptions::ALLPROCESS:
      break;
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
  if ( this->TileDimensions[0] > 0 || this->TileDimensions[1] > 0 )
    {
    if ( this->TileDimensions[0] <= 0 )
      {
      this->TileDimensions[0] = 1;
      }
    if ( this->TileDimensions[1] <= 0 )
      {
      this->TileDimensions[1] = 1;
      }
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
  if ( this->CaveConfigurationFileName )
    {
    this->SetRenderModuleName("CaveRenderModule");
    }
#ifdef PARAVIEW_ALWAYS_SECURE_CONNECTION
  if ( (this->ClientMode || this->ServerMode) && !this->ConnectID)
    {
    this->SetErrorMessage("You need to specify a connect ID (--connect-id).");
    return 0;
    }
#endif //PARAVIEW_ALWAYS_SECURE_CONNECTION

  if (this->GetSymmetricMPIMode())
    {
    // Disable render event propagation since satellites are no longer doing
    // ProcessRMIs() since symmetric script processing is enabled.
    vtkParallelRenderManager::SetDefaultRenderEventPropagation(false);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::WrongArgument(const char* argument)
{
  if(vtksys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetErrorMessage("Batch file argument to ParaView executable is deprecated. Please use \"pvbatch\".");
    return 0;
    }

  if (this->Superclass::WrongArgument(argument))
    {
    return 1;
    }

  if (this->ParaViewDataName == NULL && this->GetProcessType() == PVCLIENT)
    {
    // BUG #11199. Assume it's a data file.
    this->SetParaViewDataName(argument);
    if (this->GetUnknownArgument() &&
      strcmp(this->GetUnknownArgument(), argument) == 0)
      {
      this->SetUnknownArgument(0);
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVOptions::DeprecatedArgument(const char* argument)
{
  return this->Superclass::DeprecatedArgument(argument);
}

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ParaViewDataName: " << (this->ParaViewDataName?this->ParaViewDataName:"(none)") << endl;
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
  if (this->ConnectRenderToData)
    {
    os << indent << "Running as a client to a data and render server\n";
    }

  if (this->ConnectDataToRender)
    {
    os << indent << "Running as a client to a data and render server\n";
    }

  if (this->ClientRenderServer)
    {
    os << indent << "Running as a client connected to a render server\n";
    }



  if (this->RenderServerMode)
    {
    os << indent << "Running as a render server\n";
    }

  if (this->ClientMode || this->ServerMode || this->RenderServerMode )
    {
    os << indent << "ConnectID is: " << this->ConnectID << endl;
    os << indent << "Reverse Connection: " << (this->ReverseConnection?"on":"off") << endl;
    if (this->RenderServerMode)
      {
      os << indent << "DataServerPort: " << this->DataServerPort << endl;
      os << indent << "Render Node Port: " << this->RenderNodePort << endl;
      os << indent << "Render Server Port: " << this->RenderServerPort << endl;
      os << indent << "Connect Render Server to Data Server: "
         << (this->ConnectRenderToData?"on":"off") << endl;
      os << indent << "Connect Data Server to Render Server: "
         << (this->ConnectDataToRender?"on":"off") << endl;
      os << indent << "DataServerHostName: " << (this->DataServerHostName?this->DataServerHostName:"(none)") << endl;
      os << indent << "RenderServerHostName: " << (this->RenderServerHostName?this->RenderServerHostName:"(none)") << endl;
      }
    else
      {
      os << indent << "ServerPort: " << this->ServerPort << endl;
      os << indent << "ServerHostName: " << (this->ServerHostName?this->ServerHostName:"(none)") << endl;
      }
    os << indent << "ClientHostName: " << (this->ClientHostName?this->ClientHostName:"(none)") << endl;
    }

  os << indent << "Timeout: " << this->Timeout << endl;
  os << indent << "Software Rendering: " << (this->UseSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Satellite Software Rendering: " << (this->UseSatelliteSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Stereo Rendering: " << (this->UseStereoRendering?"Enabled":"Disabled") << endl;

  os << indent << "Offscreen Rendering: " << (this->UseOffscreenRendering?"Enabled":"Disabled") << endl;

  os << indent << "Tiled Display: " << (this->TileDimensions[0]?"Enabled":"Disabled") << endl;
  if (this->TileDimensions[0])
    {
    os << indent << "With Tile Dimensions: " << this->TileDimensions[0]
       << ", " << this->TileDimensions[1] << endl;
    os << indent << "And Tile Mullions: " << this->TileMullions[0]
       << ", " << this->TileMullions[1] << endl;
    }

  os << indent << "Using RenderingGroup: " << (this->UseRenderingGroup?"Enabled":"Disabled") << endl;

  os << indent << "Render Module Used: " << (this->RenderModuleName?this->RenderModuleName:"(none)") << endl;

  os << indent << "Network Configuration: " << (this->MachinesFileName?this->MachinesFileName:"(none)") << endl;

  os << indent << "Cave Configuration: " << (this->CaveConfigurationFileName?this->CaveConfigurationFileName:"(none)") << endl;

  os << indent << "Compositing: " << (this->DisableComposite?"Disabled":"Enabled") << endl;

  if (this->TellVersion)
    {
    os << indent << "Running to display software version.\n";
    }

  os << indent << "StateFileName: "
    << (this->StateFileName?this->StateFileName:"(none)") << endl;
  os << indent << "LogFileName: "
    << (this->LogFileName? this->LogFileName : "(none)") << endl;
  os << indent << "SymmetricMPIMode: " << this->SymmetricMPIMode << endl;
}
