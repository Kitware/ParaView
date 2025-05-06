// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInSituPipelineIO.h"

#include "vtkInSituInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVStringFormatter.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMWriterFactory.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <cassert>

namespace
{
vtkSmartPointer<vtkSMSourceProxy> CreateWriter(
  vtkInSituPipelineIO* self, vtkSMSourceProxy* producer)
{
  auto factory = vtkSMProxyManager::GetProxyManager()->GetWriterFactory();
  if (factory->GetNumberOfRegisteredPrototypes() == 0)
  {
    // setup writer factory, if not setup already.
    factory->UpdateAvailableWriters();
  }
  auto writer =
    vtkSMSourceProxy::SafeDownCast(factory->CreateWriter(self->GetFileName(), producer, 0));
  return vtkSmartPointer<vtkSMSourceProxy>::Take(writer);
}
}

vtkStandardNewMacro(vtkInSituPipelineIO);
//----------------------------------------------------------------------------
vtkInSituPipelineIO::vtkInSituPipelineIO()
  : FileName(nullptr)
  , ChannelName(nullptr)
{
}

//----------------------------------------------------------------------------
vtkInSituPipelineIO::~vtkInSituPipelineIO()
{
  this->SetFileName(nullptr);
  this->SetChannelName(nullptr);
}

//----------------------------------------------------------------------------
bool vtkInSituPipelineIO::Initialize()
{
  // let's just ensure there's no writer setup already.
  this->Writer = nullptr;

  if (this->FileName == nullptr || this->FileName[0] == '\0')
  {
    vtkErrorMacro("Invalid filename");
    return false;
  }

  if (!this->ChannelName)
  {
    vtkErrorMacro("Missing channel name");
    return false;
  }

  if (vtkInSituInitializationHelper::GetProducer(this->ChannelName) == nullptr)
  {
    vtkErrorMacro("No producer named '" << this->ChannelName << "'.");
    return false;
  }

  return this->Superclass::Initialize();
  ;
}

//----------------------------------------------------------------------------
bool vtkInSituPipelineIO::Execute(int timestep, double time)
{
  if (this->Writer == nullptr)
  {
    auto producer = vtkInSituInitializationHelper::GetProducer(this->ChannelName);
    assert(producer != nullptr);

    this->Writer = ::CreateWriter(this, producer);
    if (!this->Writer)
    {
      vtkErrorMacro("Failed to create writer on channel '" << this->ChannelName << "' for file '"
                                                           << this->FileName << "'.");
      return false;
    }
  }

  auto fname = this->GetCurrentFileName(this->FileName, timestep, time);
  auto pname = vtkSMCoreUtilities::GetFileNameProperty(this->Writer);
  pname = pname ? pname : "FileName"; // some writers don't use FileListDomain.

  vtkSMPropertyHelper(this->Writer, pname).Set(fname.c_str());
  this->Writer->UpdateVTKObjects();
  this->Writer->UpdatePipeline(time);
  return true;
}

//----------------------------------------------------------------------------
bool vtkInSituPipelineIO::Finalize()
{
  this->Writer = nullptr;
  return this->Superclass::Finalize();
}

//----------------------------------------------------------------------------
std::string vtkInSituPipelineIO::GetCurrentFileName(const char* fname, int timestep, double time)
{
  assert(fname != nullptr);
  // define scope of arguments
  PV_STRING_FORMATTER_SCOPE(fmt::arg("timestep", timestep), fmt::arg("time", time));

  std::string name{ fname };
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
void vtkInSituPipelineIO::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "ChannelName: " << (this->ChannelName ? this->ChannelName : "(nullptr)") << endl;
}
