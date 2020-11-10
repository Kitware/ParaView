/*=========================================================================

  Program:   ParaView
  Module:    vtkInSituPipelineIO.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInSituPipelineIO.h"

#include "vtkInSituInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMWriterFactory.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <cassert>

namespace detail
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

    this->Writer = detail::CreateWriter(this, producer);
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

  // clang-format off
  vtksys::RegularExpression regex(R"=((%[.0-9]*)((ts)|(t)))=");
  // clang-format on

  std::string name{ fname };
  while (regex.find(name))
  {
    if (regex.match(2) == "ts")
    {
      char buffer[256];
      std::snprintf(buffer, 256, (regex.match(1) + "d").c_str(), timestep);
      name.replace(regex.start(), regex.end() - regex.start(), buffer);
    }
    else if (regex.match(2) == "t")
    {
      char buffer[256];
      std::snprintf(buffer, 256, (regex.match(1) + "f").c_str(), time);
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

  vtkLogF(TRACE, "%s --> %s", fname, name.c_str());
  return name;
}

//----------------------------------------------------------------------------
void vtkInSituPipelineIO::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(nullptr)") << endl;
  os << indent << "ChannelName: " << (this->ChannelName ? this->ChannelName : "(nullptr)") << endl;
}
