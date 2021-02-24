/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginLoaderProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMPluginLoaderProxy);
//----------------------------------------------------------------------------
vtkSMPluginLoaderProxy::vtkSMPluginLoaderProxy() = default;

//----------------------------------------------------------------------------
vtkSMPluginLoaderProxy::~vtkSMPluginLoaderProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMPluginLoaderProxy::LoadPlugin(const char* filename)
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LoadPlugin" << filename
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->UpdatePropertyInformation();
  return vtkSMPropertyHelper(this, "Loaded").GetAsInt(0) != 0;
}

//----------------------------------------------------------------------------
void vtkSMPluginLoaderProxy::LoadPluginConfigurationXMLFromString(const char* xmlcontents)
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this)
         << "LoadPluginConfigurationXMLFromString" << xmlcontents << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMPluginLoaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
