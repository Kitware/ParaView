/*=========================================================================

  Module:    vtkPVBatchOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBatchOptions.h"

#include "vtkObjectFactory.h"

#include <kwsys/CommandLineArguments.hxx>
#include <kwsys/SystemTools.hxx>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVBatchOptions);
vtkCxxRevisionMacro(vtkPVBatchOptions, "1.3");

//----------------------------------------------------------------------------
vtkPVBatchOptions::vtkPVBatchOptions()
{
  this->RequireBatchScript = 1;
  this->BatchScriptName = 0;
  this->ClientRenderServer = 0;
  this->ConnectRenderToData = 0;
  this->ConnectDataToRender = 0;

  // When running from client, default is different.
  this->ServerMode = 0;
}

//----------------------------------------------------------------------------
vtkPVBatchOptions::~vtkPVBatchOptions()
{
  this->SetBatchScriptName(0);
}

//----------------------------------------------------------------------------
void vtkPVBatchOptions::Initialize()
{
  this->Superclass::Initialize();

  this->AddBooleanArgument("--client-render-server", "-crs", &this->ClientRenderServer,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->AddBooleanArgument("--connect-render-to-data", "-r2d", &this->ConnectRenderToData,
    "Run ParaView as a client to a data and render server. The data server will wait for the render server.");
  this->AddBooleanArgument("--connect-data-to-render", "-d2r", &this->ConnectDataToRender,
    "Run ParaView as a client to a data and render server. The render server will wait for the data server.");
  this->AddArgument("--render-port", 0, &this->RenderServerPort, 
    "Specify the port client and render server will use (--port=22222).  Client and render servers ports must match.");
  this->AddArgument("--render-server-host", "-rsh", &this->RenderServerHostName, 
    "Tell the client where to look for the render server (default: localhost). Used with --client option.");
}

//----------------------------------------------------------------------------
int vtkPVBatchOptions::PostProcess(int argc, const char* const* argv)
{
  if ( this->RequireBatchScript && !this->BatchScriptName )
    {
    this->SetErrorMessage("Batch script not specified");
    return 0;
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
  return this->Superclass::PostProcess(argc, argv);
}

int vtkPVBatchOptions::WrongArgument(const char* argument)
{
  if ( kwsys::SystemTools::FileExists(argument) &&
    kwsys::SystemTools::GetFilenameLastExtension(argument) == ".pvb")
    {
    this->SetBatchScriptName(argument);
    return 1;
    }

  return this->Superclass::WrongArgument(argument);
}

//----------------------------------------------------------------------------
void vtkPVBatchOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BatchScriptName: " << (this->BatchScriptName?this->BatchScriptName:"(none)") << endl;

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


}

