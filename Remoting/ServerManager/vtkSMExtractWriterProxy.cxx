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
#include "vtkPVStringFormatter.h"
#include "vtkSMExtractsController.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <string>

//----------------------------------------------------------------------------
vtkSMExtractWriterProxy::vtkSMExtractWriterProxy() = default;

//----------------------------------------------------------------------------
vtkSMExtractWriterProxy::~vtkSMExtractWriterProxy() = default;

//----------------------------------------------------------------------------
std::string vtkSMExtractWriterProxy::GenerateExtractsFileName(
  const std::string& filename, const char* outDir)
{
  // `FileIsFullPath` check helps us support absolute paths provided on
  // individual extractor.
  std::string name = vtksys::SystemTools::FileIsFullPath(filename)
    ? filename
    : vtksys::SystemTools::JoinPath({ outDir, "/" + filename });

  // check for old format for ts and t
  bool oldFormatUsed = false;
  vtksys::RegularExpression regex(R"=((%[.0-9]*)((ts)|(t)))=");
  std::string possibleOldFormatString1 = name;
  while (regex.find(name))
  {
    if (regex.match(2) == "ts")
    {
      oldFormatUsed = true;
      // assume that precision information will always have the format, eg %.[0-9]*
      std::string formatString = "0" + regex.match(1).substr(2) + "d";
      std::string replacement = "{timestep:" + formatString + "}";
      name.replace(regex.start(), regex.end() - regex.start(), replacement);
    }
    else if (regex.match(2) == "t")
    {
      oldFormatUsed = true;
      // remove the % symbol from the formatString
      std::string formatString = regex.match(1).substr(1) + "f";
      std::string replacement = "{time:" + formatString + "}";
      name.replace(regex.start(), regex.end() - regex.start(), replacement);
    }
    else
    {
      vtkLogF(ERROR,
        "Unknown conversion specified '%s' in '%s'. Filename may not be picked correctly.",
        regex.match(2).c_str(), regex.match(0).c_str());
      break;
    }
  }

  // check for old format for cm
  std::string possibleOldFormatString2 = name;
  vtksys::SystemTools::ReplaceString(name, "%cm", "{camera}");
  if (possibleOldFormatString2 != name || oldFormatUsed)
  {
    vtkLogF(WARNING, "Legacy formatting pattern detected. Please replace '%s' with '%s'.",
      possibleOldFormatString1.c_str(), name.c_str());
  }

  return vtkPVStringFormatter::Format(name);
}

//----------------------------------------------------------------------------
void vtkSMExtractWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
