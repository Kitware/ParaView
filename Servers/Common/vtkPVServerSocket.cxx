/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerSocket.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVServerSocket.h"

#include "vtkProcessModuleConnectionManager.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVServerSocket);
//-----------------------------------------------------------------------------
vtkPVServerSocket::vtkPVServerSocket()
{
  this->Type = vtkProcessModuleConnectionManager::RENDER_AND_DATA_SERVER;
}

//-----------------------------------------------------------------------------
vtkPVServerSocket::~vtkPVServerSocket()
{
}

//-----------------------------------------------------------------------------
void vtkPVServerSocket::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Type: " ;
  switch (this->Type)
    {
  case vtkProcessModuleConnectionManager::RENDER_SERVER:
    os << "RENDER_SERVER";
    break;
  case vtkProcessModuleConnectionManager::DATA_SERVER:
    os << "DATA_SERVER";
    break;
  case vtkProcessModuleConnectionManager::RENDER_AND_DATA_SERVER:
    os << "RENDER_AND_DATA_SERVER";
    break;
  default:
    os << "Invalid";
    }
  os << endl;
}
