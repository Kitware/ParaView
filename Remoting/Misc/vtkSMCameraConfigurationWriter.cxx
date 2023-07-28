// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMCameraConfigurationWriter.h"
#include "vtkSMCameraConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkStringList.h"

vtkStandardNewMacro(vtkSMCameraConfigurationWriter);

//-----------------------------------------------------------------------------
vtkSMCameraConfigurationWriter::vtkSMCameraConfigurationWriter()
{
  vtkStringList* propNames = vtkStringList::New();
  propNames->AddString("CameraPosition");
  propNames->AddString("CameraFocalPoint");
  propNames->AddString("CameraViewUp");
  propNames->AddString("CenterOfRotation");
  propNames->AddString("RotationFactor");
  propNames->AddString("CameraViewAngle");
  propNames->AddString("CameraParallelScale");
  propNames->AddString("CameraParallelProjection");
  vtkSMNamedPropertyIterator* propIt = vtkSMNamedPropertyIterator::New();
  propIt->SetPropertyNames(propNames);
  propNames->Delete();
  this->SetPropertyIterator(propIt);
  propIt->Delete();

  vtkSMCameraConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSMCameraConfigurationWriter::~vtkSMCameraConfigurationWriter() = default;

//-----------------------------------------------------------------------------
void vtkSMCameraConfigurationWriter::SetRenderViewProxy(vtkSMProxy* rvProxy)
{
  this->vtkSMProxyConfigurationWriter::SetProxy(rvProxy);
  this->GetPropertyIterator()->SetProxy(rvProxy);
}

//-----------------------------------------------------------------------------
void vtkSMCameraConfigurationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
