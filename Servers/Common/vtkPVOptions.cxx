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

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>


//----------------------------------------------------------------------------
//****************************************************************************
class vtkPVOptionsInternal
{
public:
  vtkPVOptionsInternal(vtkPVOptions* p)
    {
      this->XMLParser = vtkPVOptionsXMLParser::New();
      this->XMLParser->SetPVOptions(p);
    }
  ~vtkPVOptionsInternal()
    {
      this->XMLParser->Delete();
    }
  vtkPVOptionsXMLParser* XMLParser;
  kwsys::CommandLineArguments CMD;
};
//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptions);
vtkCxxRevisionMacro(vtkPVOptions, "1.23");

//----------------------------------------------------------------------------
vtkPVOptions::vtkPVOptions()
{
  this->ProcessType = ALLPROCESS;
  // Initialize kwsys::CommandLineArguments
  this->Internals = new vtkPVOptionsInternal(this);
  this->Internals->CMD.SetUnknownArgumentCallback(vtkPVOptions::UnknownArgumentHandler);
  this->Internals->CMD.SetClientData(this);
  this->UnknownArgument = 0;
  this->ErrorMessage = 0;
  this->Argc = 0;
  this->Argv = 0;
  this->CaveConfigurationFileName = 0;
  this->MachinesFileName = 0;
  this->RenderModuleName = NULL;
  this->UseRenderingGroup = 0;
  this->GroupFileName = 0;
  this->XMLConfigFile = 0;
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
  this->HelpSelected = 0;
  this->ConnectID = 0;
}

//----------------------------------------------------------------------------
vtkPVOptions::~vtkPVOptions()
{
  this->SetXMLConfigFile(0);
  this->SetRenderModuleName(0);
  this->SetCaveConfigurationFileName(NULL);
  this->SetGroupFileName(0);
  this->SetServerHostName(0);
  this->SetDataServerHostName(0);
  this->SetRenderServerHostName(0);
  this->SetClientHostName(0);
  this->SetMachinesFileName(0);
  
  // Remove internals
  this->SetUnknownArgument(0);
  this->SetErrorMessage(0);
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
  this->Internals->CMD.SetLineLength(300);

  return this->Internals->CMD.GetHelp();
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
                    vtkPVOptions::PVCLIENT);
  this->AddArgument("--render-module", 0, &this->RenderModuleName,
                    "User specified rendering module.",
                    vtkPVOptions::PVRENDER_SERVER| vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--use-offscreen-rendering", 0, &this->UseOffscreenRendering,
                           "Render offscreen on the satellite processes."
                           " This option only works with software rendering or mangled mesa on Unix.",
                           vtkPVOptions::PVRENDER_SERVER| vtkPVOptions::PVSERVER);
  this->AddBooleanArgument("--stereo", 0, &this->UseStereoRendering,
                           "Tell the application to enable stereo rendering"
                           " (only when running on a single process).",
                           vtkPVOptions::PVCLIENT | vtkPVOptions::PARAVIEW);
  this->AddArgument("--server-host", "-dsh", &this->ServerHostName,
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
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVDATA_SERVER|vtkPVOptions::PVSERVER);
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
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  this->AddArgument("--tile-dimensions-y", "-tdy", this->TileDimensions+1, 
                    "Size of tile display in the number of displays in each column of the display.",
                    vtkPVOptions::PVRENDER_SERVER|vtkPVOptions::PVSERVER);
  
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
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::WrongArgument(const char* argument)
{
  // if the unknown file is a config file then it is OK
  if(this->XMLConfigFile && strcmp(argument, this->XMLConfigFile) == 0)
    {
    // if the UnknownArgument is the XMLConfigFile then set the 
    // UnknownArgument to null as it really is not Unknown anymore.
    if(this->UnknownArgument && 
       (strcmp(this->UnknownArgument, this->XMLConfigFile) == 0))
      {
      this->SetUnknownArgument(0);
      }
    return 1;
    }
  if(kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetErrorMessage("Batch file argument to ParaView executable is deprecated. Please use \"pvbatch\".");
    return 0;
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkPVOptions::GetArgv0()
{
  return this->Internals->CMD.GetArgv0();
}

//----------------------------------------------------------------------------
int vtkPVOptions::LoadXMLConfigFile(const char* fname)
{
  this->Internals->XMLParser->SetFileName(fname);
  this->Internals->XMLParser->Parse();
  this->SetXMLConfigFile(fname);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVOptions::Parse(int argc, const char* const argv[])
{
  this->Internals->CMD.Initialize(argc, argv);
  this->Initialize();
  this->AddBooleanArgument("--help", "/?", &this->HelpSelected, 
                           "Displays available command line arguments.",
                           ALLPROCESS);

  // First get options from the xml file
  for(int i =0; i < argc; ++i)
    {
    vtkstd::string arg = argv[i];
    if(arg.size() > 4 && arg.find(".pvx") == (arg.size() -4))
      {
      if(!this->LoadXMLConfigFile(arg.c_str()))
        {
        return 0;
        }
      }
    }
  // now get options from the command line
  int res1 = this->Internals->CMD.Parse();
  int res2 = this->PostProcess(argc, argv);
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
void vtkPVOptions::AddDeprecatedArgument(const char* longarg, const char* shortarg,
                                         const char* help, int type)
{
  // if it is for xml or not for the current process do nothing
  if((type == XMLONLY) || !(type & this->ProcessType))
    {
    return;
    }
  // Add a callback for the deprecated argument handling
  this->Internals->CMD.AddCallback(longarg, kwsys::CommandLineArguments::NO_ARGUMENT,
                                   vtkPVOptions::DeprecatedArgumentHandler, this, help);
  if(shortarg)
    {
    this->Internals->CMD.AddCallback(shortarg, kwsys::CommandLineArguments::NO_ARGUMENT,
                                     vtkPVOptions::DeprecatedArgumentHandler, this, help);
    }
}

//----------------------------------------------------------------------------
int vtkPVOptions::DeprecatedArgument(const char* argument)
{
  ostrstream str;
  str << "  " << this->Internals->CMD.GetHelp(argument);
  str << ends;
  this->SetErrorMessage(str.str());
  delete [] str.str();
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVOptions::AddBooleanArgument(const char* longarg, const char* shortarg,
                                      int* var, const char* help, int type)
{
  // add the argument to the XML parser
  this->Internals->XMLParser->AddBooleanArgument(longarg, var, type);
  if(type == XMLONLY)
    {
    return;
    }
  // if the process type matches then add the argument to the command line
  if(type & this->ProcessType)
    {
    this->Internals->CMD.AddBooleanArgument(longarg, var, help);
    if ( shortarg )
      {
      this->Internals->CMD.AddBooleanArgument(shortarg, var, longarg);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVOptions::AddArgument(const char* longarg, const char* shortarg, int* var, const char* help, int type)
{
  this->Internals->XMLParser->AddArgument(longarg, var, type);
  if(type == XMLONLY)
    {
    return;
    }
  if(type & this->ProcessType)
    {
    typedef kwsys::CommandLineArguments argT;
    this->Internals->CMD.AddArgument(longarg, argT::EQUAL_ARGUMENT, var, help);
    if ( shortarg )
      {
      this->Internals->CMD.AddArgument(shortarg, argT::EQUAL_ARGUMENT, var, longarg);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVOptions::AddArgument(const char* longarg, const char* shortarg, char** var, const char* help, int type)
{
  this->Internals->XMLParser->AddArgument(longarg, var, type);
  if(type == XMLONLY)
    {
    return;
    }
  if(type & this->ProcessType)
    {
    typedef kwsys::CommandLineArguments argT;
    this->Internals->CMD.AddArgument(longarg, argT::EQUAL_ARGUMENT, var, help);
    if ( shortarg )
      {
      this->Internals->CMD.AddArgument(shortarg, argT::EQUAL_ARGUMENT, var, longarg);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVOptions::UnknownArgumentHandler(const char* argument, void* call_data)
{
  vtkPVOptions* self = static_cast<vtkPVOptions*>(call_data);
  if ( self )
    {
    self->SetUnknownArgument(argument);
    return self->WrongArgument(argument);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVOptions::DeprecatedArgumentHandler(const char* argument, 
                                            const char* , void* call_data)
{
  //cout << "UnknownArgumentHandler: " << argument << endl;
  vtkPVOptions* self = static_cast<vtkPVOptions*>(call_data);
  if ( self )
    {
    return self->DeprecatedArgument(argument);
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
int vtkPVOptions::GetLastArgument()
{
  return this->Internals->CMD.GetLastArgument();
}

//----------------------------------------------------------------------------
void vtkPVOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ParaViewDataName: " << (this->ParaViewDataName?this->ParaViewDataName:"(none)") << endl;
  os << indent << "XMLConfigFile: " << (this->XMLConfigFile?this->XMLConfigFile:"(none)") << endl;
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
  if (this->ConnectDataToRender)
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
