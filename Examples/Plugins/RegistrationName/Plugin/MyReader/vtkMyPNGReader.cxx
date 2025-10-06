// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMyPNGReader.h"

#include <vtkObjectFactory.h>

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkMyPNGReader);

//----------------------------------------------------------------------------
vtkMyPNGReader::vtkMyPNGReader() = default;

//----------------------------------------------------------------------------
vtkMyPNGReader::~vtkMyPNGReader() = default;

//----------------------------------------------------------------------------
const char* vtkMyPNGReader::GetRegistrationName()
{
  const std::string& fileStem = this->GetFileStem(this->FileName);

  this->RegistrationName = "MyPNGReaderFile-" + fileStem;
  return this->RegistrationName.c_str();
}

//----------------------------------------------------------------------------
std::string vtkMyPNGReader::GetFileStem(const std::string& filePath)
{
  std::vector<std::string> components;
  vtksys::SystemTools::SplitPath(filePath, components);
  const std::string& fileName = components.back();

  std::vector<std::string> splittedFileName = vtksys::SystemTools::SplitString(fileName, '.');
  const std::string& fileStem = splittedFileName.front();

  return fileStem;
}

//----------------------------------------------------------------------------
void vtkMyPNGReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
