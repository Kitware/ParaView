// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMCameraConfigurationReader.h"
#include "vtkSMCameraConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

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
vtkSMCameraConfigurationReader::~vtkSMCameraConfigurationReader() = default;

//-----------------------------------------------------------------------------
void vtkSMCameraConfigurationReader::SetRenderViewProxy(vtkSMProxy* rvProxy)
{
  this->vtkSMProxyConfigurationReader::SetProxy(rvProxy);
}

//-----------------------------------------------------------------------------
int vtkSMCameraConfigurationReader::ReadConfiguration(const char* filename)
{
  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", this->GetProxy())
    .arg("comment", "Load a camera configuration");
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
  SM_SCOPED_TRACE(PropertiesModified).arg("proxy", this->GetProxy());
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
