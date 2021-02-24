/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractWriterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractWriterProxy.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkSMExtractsController.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <cstdio>
#include <string>

//----------------------------------------------------------------------------
vtkSMExtractWriterProxy::vtkSMExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMExtractWriterProxy::~vtkSMExtractWriterProxy() = default;

//----------------------------------------------------------------------------
std::string vtkSMExtractWriterProxy::GenerateDataExtractsFileName(
  const std::string& fname, vtkSMExtractsController* extractor)
{
  return vtkSMExtractWriterProxy::GenerateExtractsFileName(
    fname, extractor, extractor->GetRealExtractsOutputDirectory());
}

//----------------------------------------------------------------------------
std::string vtkSMExtractWriterProxy::GenerateImageExtractsFileName(
  const std::string& fname, vtkSMExtractsController* extractor)
{
  return vtkSMExtractWriterProxy::GenerateImageExtractsFileName(fname, std::string(), extractor);
}

//----------------------------------------------------------------------------
std::string vtkSMExtractWriterProxy::GenerateImageExtractsFileName(
  const std::string& fname, const std::string& cameraparams, vtkSMExtractsController* extractor)
{
  auto str = vtkSMExtractWriterProxy::GenerateExtractsFileName(
    fname, extractor, extractor->GetRealExtractsOutputDirectory());
  size_t pos = 0;
  while ((pos = str.find("%cm", pos)) != std::string::npos)
  {
    str.replace(pos, 3, cameraparams);
    pos += cameraparams.size();
  }
  return str;
}

//----------------------------------------------------------------------------
std::string vtkSMExtractWriterProxy::GenerateExtractsFileName(
  const std::string& fname, vtkSMExtractsController* extractor, const char* rootdir)
{
  // clang-format off
  vtksys::RegularExpression regex(R"=((%[.0-9]*)((ts)|(t)))=");
  // clang-format on

  // `FileIsFullPath` check helps us support absolute paths provided on
  // individual extractor.
  std::string name = vtksys::SystemTools::FileIsFullPath(fname)
    ? fname
    : vtksys::SystemTools::JoinPath({ rootdir, "/" + fname });
  while (regex.find(name))
  {
    if (regex.match(2) == "ts")
    {
      char buffer[256];
      std::snprintf(buffer, 256, (regex.match(1) + "d").c_str(), extractor->GetTimeStep());
      name.replace(regex.start(), regex.end() - regex.start(), buffer);
    }
    else if (regex.match(2) == "t")
    {
      char buffer[256];
      std::snprintf(buffer, 256, (regex.match(1) + "f").c_str(), extractor->GetTime());
      name.replace(regex.start(), regex.end() - regex.start(), buffer);
    }
    else
    {
      vtkLogF(ERROR,
        "Unknown conversion specified '%s' in '%s'. Filename may not be picked correctly.",
        regex.match(2).c_str(), regex.match(0).c_str());
      break;
    }
  }

  vtkLogF(TRACE, "%s --> %s", fname.c_str(), name.c_str());
  return name;
}

//----------------------------------------------------------------------------
void vtkSMExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
