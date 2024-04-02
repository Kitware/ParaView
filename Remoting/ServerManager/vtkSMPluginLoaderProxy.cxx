// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
bool vtkSMPluginLoaderProxy::LoadPluginByName(const char* name, bool acceptDelayed)
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LoadPluginByName" << name
         << acceptDelayed << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->UpdatePropertyInformation();
  return vtkSMPropertyHelper(this, "Loaded").GetAsInt(0) != 0;
}

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
void vtkSMPluginLoaderProxy::LoadPluginConfigurationXML(const char* configurationFile)
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LoadPluginConfigurationXML"
         << configurationFile << vtkClientServerStream::End;
  this->ExecuteStream(stream);
  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMPluginLoaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
