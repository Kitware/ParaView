/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMCameraConfigurationReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraConfigurationReader.h"
#include "vtkSMCameraConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMCameraConfigurationReader);

//-----------------------------------------------------------------------------
vtkSMCameraConfigurationReader::vtkSMCameraConfigurationReader()
{
  // Valid camera configuration can come from a various
  // proxy types, eg RenderView,IceTRenderView and so on.
  this->SetValidateProxyType(0);

  vtkSMCameraConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSMCameraConfigurationReader::~vtkSMCameraConfigurationReader()
{
}

//-----------------------------------------------------------------------------
void vtkSMCameraConfigurationReader::SetRenderViewProxy(vtkSMProxy* rvProxy)
{
  this->vtkSMProxyConfigurationReader::SetProxy(rvProxy);
}

//-----------------------------------------------------------------------------
int vtkSMCameraConfigurationReader::ReadConfiguration(const char* filename)
{
  int ok = this->Superclass::ReadConfiguration(filename);
  if (!ok)
  {
    return 0;
  }

  this->GetProxy()->UpdateVTKObjects();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSMCameraConfigurationReader::ReadConfiguration(vtkPVXMLElement* x)
{
  int ok = this->Superclass::ReadConfiguration(x);
  if (!ok)
  {
    return 0;
  }

  this->GetProxy()->UpdateVTKObjects();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMCameraConfigurationReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
