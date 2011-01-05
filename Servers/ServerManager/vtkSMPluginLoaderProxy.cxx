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

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMPluginLoaderProxy);
//----------------------------------------------------------------------------
vtkSMPluginLoaderProxy::vtkSMPluginLoaderProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPluginLoaderProxy::~vtkSMPluginLoaderProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMPluginLoaderProxy::LoadPlugin(const char* filename)
{
  this->CreateVTKObjects();
  vtkSMMessage message;
  message << pvstream::InvokeRequest() << "LoadPlugin" << filename;
  this->Invoke(&message);
  this->UpdatePropertyInformation();
  return vtkSMPropertyHelper(this, "Loaded").GetAsInt(0) != 0;
}

//----------------------------------------------------------------------------
void vtkSMPluginLoaderProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
