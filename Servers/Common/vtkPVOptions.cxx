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
#include "vtkPVOptionsXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h" //For PARAVIEW_ALWAYS_SECURE_CONNECTION option

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);
vtkCxxRevisionMacro(vtkPVOptions, "1.30");

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->SetProcessType(ALLPROCESS);
  // Initialize kwsys::CommandLineArguments
  this->CaveConfigurationFileName = 0;
  this->MachinesFileName = 0;
  this->RenderModuleName = NULL;
  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;
  this->ParaViewDataName = 0;
  
  this->ClientRenderServer = 0;
  this->ConnectRenderToData = 0;
  this->ConnectDataToRender = 0;


  this->TileDimensions[0] = 0;
  this->TileDimensions[1] = 0;
  this->ClientMode = 0;
  this->ServerMode = 0;
  this->RenderServerMode = 0;

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
}

//----------------------------------------------------------------------------
void vtkPVOptions::Initialize()
{
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
  this->AddArgument("--data", 0, &this->ParaViewDataName,
                    "Load the specified data.", 
                    vtkPVOptions::PVCLIENT|vtkPVOptions::PARAVIEW);
  this->AddArgument("--connect-id", 0, &this->ConnectID,
                    "Set the ID of the server and client to make sure they match.",
                    vtkPVOptions::PVCLIENT | vtkPVOptions::PVSERVER |
                    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER);
  this->AddArgument("--render-module", 0, &this->RenderModuleName,
                    "User specified rendering module.",
                    vtkPVOptions::PVCLIENT| vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--use-offscreen-rendering", 0, &this->UseOffscreenRendering,
                           "Render offscreen on the satellite processes."
                           " This option only works with software rendering or mangled mesa on Unix.",
                           vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--stereo", 0, &this->UseStereoRendering,
                           "Tell the application to enable stereo rendering"
                           " (only when running on a single process).",
                           vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);
  this->AddArgument("--server-host", "-sh", &this->ServerHostName,
                    "Tell the client the host name of the data server.",
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--data-server-host", "-dsh", &this->DataServerHostName,
                    "Tell the client the host name of the data server.", 
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--render-server-host", "-rsh", &this->RenderServerHostName,
                    "Tell the client the host name of the render server.", 
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--client-host", "-ch", &this->ClientHostName,
                    "Tell the data|render server the host name of the client, use with -rc.",
                    vtkPVOptions::PVRENDER_SERVER | vtkPVOptions::PVDATA_SERVER |
                    vtkPVOptions::PVSERVER);
  this->AddArgument("--data-server-port", "-dsp", &this->DataServerPort,
                    "What port data server use to connect to the client. (default 11111).", 
                    vtkPVOptions::PVCLIENT | vtkPVOptions::PVDATA_SERVER);
  this->AddArgument("--render-server-port", "-rsp", &this->RenderServerPort,
                    "What port should the render server use to connect to the client. (default 22221).", 
                    vtkPVOptions::PVCLIENT | vtkPVOptions::PVRENDER_SERVER);
  this->AddArgument("--server-port", "-sp", &this->ServerPort,
                    "What port should the combined server use to connect to the client. (default 11111).", 
                    vtkPVOptions::PVCLIENT | vtkPVOptions::PVSERVER);

  this->AddArgument("--render-node-port", 0, &this->RenderNodePort, 
                    "Specify the port to be used by each render node (--render-node-port=22222)."
                    "  Client and render servers ports must match.",
                    vtkPVOptions::XMLONLY);
  this->AddBooleanArgument("--disable-composite", "-dc", &this->DisableComposite, 
                           "Use this option when rendering resources are not available on the server.", 
                           vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--reverse-connection", "-rc", &this->ReverseConnection, 
                           "Have the server connect to the client.");
  this->AddArgument("--tile-dimensions-x", "-tdx", this->TileDimensions, 
                    "Size of tile display in the number of displays in each row of the display.",
                    vtkPVOptions::PVCLIENT|vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-dimensions-y", "-tdy", this->TileDimensions+1, 
                    "Size of tile display in the number of displays in each column of the display.",
                    vtkPVOptions::PVCLIENT|vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  
  // This should be deprecated when I get the time 
  this->AddArgument("--cave-configuration", "-cc", &this->CaveConfigurationFileName,
    "Specify the file that defines the displays for a cave. It is used only with CaveRenderModule.");
  this->AddArgument("--machines", "-m", &this->MachinesFileName, 
                    "Specify the network configurations file for the render server.");
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
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::WrongArgument(const char* argument)
{
  if(kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetErrorMessage("Batch file argument to ParaView executable is deprecated. Please use \"pvbatch\".");
    return 0;
    }

  return this->Superclass::WrongArgument(argument);
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
    os << indent << "DataServerPort: " << this->DataServerPort << endl;
    os << indent << "ServerPort: " << this->ServerPort << endl;
    os << indent << "Render Node Port: " << this->RenderNodePort << endl;
    os << indent << "Render Server Port: " << this->RenderServerPort << endl;
    os << indent << "Reverse Connection: " << (this->ReverseConnection?"on":"off") << endl;
    os << indent << "Connect Render Server to Data Server: "
       << (this->ConnectRenderToData?"on":"off") << endl;
    os << indent << "Connect Data Server to Render Server: "
       << (this->ConnectDataToRender?"on":"off") << endl;
    os << indent << "ServerHostName: " << (this->ServerHostName?this->ServerHostName:"(none)") << endl;
    os << indent << "DataServerHostName: " << (this->DataServerHostName?this->DataServerHostName:"(none)") << endl;
    os << indent << "RenderServerHostName: " << (this->RenderServerHostName?this->RenderServerHostName:"(none)") << endl;
    os << indent << "ClientHostName: " << (this->ClientHostName?this->ClientHostName:"(none)") << endl;
    }

  os << indent << "Software Rendering: " << (this->UseSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Satellite Software Rendering: " << (this->UseSatelliteSoftwareRendering?"Enabled":"Disabled") << endl;

  os << indent << "Stereo Rendering: " << (this->UseStereoRendering?"Enabled":"Disabled") << endl;

  os << indent << "Offscreen Rendering: " << (this->UseOffscreenRendering?"Enabled":"Disabled") << endl;

  os << indent << "Tiled Display: " << (this->TileDimensions[0]?"Enabled":"Disabled") << endl;
  if (this->TileDimensions[0])
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
