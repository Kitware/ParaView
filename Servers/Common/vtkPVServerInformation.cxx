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
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVServerInformation);
vtkCxxRevisionMacro(vtkPVServerInformation, "1.3");

//----------------------------------------------------------------------------
vtkPVServerInformation::vtkPVServerInformation()
{
  this->RemoteRendering = 1;
  this->TileDimensions[0] = this->TileDimensions[1] = 1;
  this->UseOffscreenRendering = 0;
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
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::DeepCopy(vtkPVServerInformation *info)
{
  this->RemoteRendering = info->GetRemoteRendering();
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
}

//----------------------------------------------------------------------------
// Since information is only from the root, we do not have to worry
// about adding information.
void vtkPVServerInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVServerInformation* serverInfo;
  serverInfo = vtkPVServerInformation::SafeDownCast(info);
  if (serverInfo && serverInfo->GetRemoteRendering() == 0)
    {
    this->RemoteRendering = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVServerInformation::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->RemoteRendering;
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
}
