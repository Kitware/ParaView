/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerInformation.h"

#include "vtkClientServerStream.h"
#include "vtkPVProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkToolkits.h"

vtkStandardNewMacro(vtkPVServerInformation);
vtkCxxRevisionMacro(vtkPVServerInformation, "1.4");

//----------------------------------------------------------------------------
vtkPVServerInformation::vtkPVServerInformation()
{
  this->RemoteRendering = 1;
  this->TileDimensions[0] = this->TileDimensions[1] = 0;
  this->UseOffscreenRendering = 0;
#if defined(PARAVIEW_USE_ICE_T) && defined(VTK_USE_MPI)
  this->UseIceT = 1;
#else
  this->UseIceT = 0;
#endif
}

//----------------------------------------------------------------------------
vtkPVServerInformation::~vtkPVServerInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteRendering: " << this->RemoteRendering << endl;
  os << indent << "UseOffscreenRendering: " << this->UseOffscreenRendering << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0]
     << ", " << this->TileDimensions[1] << endl;
  os << indent << "UseIceT: " << this->UseIceT << endl;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation *info)
{
  this->RemoteRendering = info->GetRemoteRendering();
  info->GetTileDimensions(this->TileDimensions);
  this->UseOffscreenRendering = info->GetUseOffscreenRendering();
  this->UseIceT = info->GetUseIceT();
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromObject(vtkObject* obj)
{
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(obj);
  if(!pm)
    {
    vtkErrorMacro("Cannot downcast to vtkPVProcessModule.");
    return;
    }
    
  this->DeepCopy(pm->GetServerInformation());

  vtkPVOptions *options = pm->GetOptions();
  options->GetTileDimensions(this->TileDimensions);
  this->UseOffscreenRendering = options->GetUseOffscreenRendering();
}

//----------------------------------------------------------------------------
// Consider an option added if it is a non-default option that the user
// has probably selected.
void vtkPVServerInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVServerInformation* serverInfo;
  serverInfo = vtkPVServerInformation::SafeDownCast(info);
  if (serverInfo)
    {
    if (!serverInfo->GetRemoteRendering())
      {
      this->RemoteRendering = 0;
      }
    if (serverInfo->GetTileDimensions()[0])
      {
      serverInfo->GetTileDimensions(this->TileDimensions);
      }
    if (serverInfo->GetUseOffscreenRendering())
      {
      this->UseOffscreenRendering = 1;
      }
    // ICE-T either is there or is not.
    this->UseIceT = serverInfo->GetUseIceT();
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->RemoteRendering;
  *css << this->TileDimensions[0] << this->TileDimensions[1];
  *css << this->UseOffscreenRendering;
  *css << this->UseIceT;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->RemoteRendering))
    {
    vtkErrorMacro("Error parsing RemoteRendering from message.");
    return;
    }
  if(   !css->GetArgument(0, 1, &this->TileDimensions[0])
     || !css->GetArgument(0, 2, &this->TileDimensions[1]) )
    {
    vtkErrorMacro("Error parsing TileDimensions from message.");
    return;
    }
  if(!css->GetArgument(0, 3, &this->UseOffscreenRendering))
    {
    vtkErrorMacro("Error parsing UseOffscreenRendering from message.");
    return;
    }
  if (!css->GetArgument(0, 4, &this->UseIceT))
    {
    vtkErrorMacro("Error parsing ICE-T flag from message.");
    return;
    }
}
