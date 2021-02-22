/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFileUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFileUtilities.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSMDirectoryProxy.h"
#include "vtkSMSessionProxyManager.h"
namespace
{
vtkTypeUInt32 ConvertLocation(vtkTypeUInt32 location)
{
  vtkTypeUInt32 proxyLocation = 0;
  if ((location & (vtkPVSession::DATA_SERVER_ROOT | vtkPVSession::DATA_SERVER)) != 0)
  {
    proxyLocation |= vtkPVSession::DATA_SERVER_ROOT;
  }
  if ((location & (vtkPVSession::RENDER_SERVER_ROOT | vtkPVSession::RENDER_SERVER)) != 0)
  {
    proxyLocation |= vtkPVSession::RENDER_SERVER_ROOT;
  }
  if ((location & vtkPVSession::CLIENT) != 0)
  {
    proxyLocation |= vtkPVSession::CLIENT;
  }
  return proxyLocation;
}

template <typename F>
bool Call(vtkSMSessionProxyManager* pxm, vtkTypeUInt32 location, F&& f)
{
  auto pm = vtkProcessModule::GetProcessModule();
  auto controller = pm->GetGlobalController();
  const auto isSymmetric = pm->GetSymmetricMPIMode();
  bool status = false;
  if (!isSymmetric || controller->GetLocalProcessId() == 0)
  {
    auto dirProxy = vtkSMDirectoryProxy::SafeDownCast(pxm->NewProxy("misc", "Directory"));
    dirProxy->SetLocation(::ConvertLocation(location));
    status = f(dirProxy);
    dirProxy->Delete();
  }

  if (isSymmetric)
  {
    // in symmetric MPI mode, while the actual attempt to make the directory
    // only happens on the root node, we need  to report the consistent status
    // on all ranks.
    int i_status = status ? 1 : 0;
    controller->Broadcast(&i_status, 1, 0);
    status = (i_status == 1);
  }

  return status;
}
}

vtkStandardNewMacro(vtkSMFileUtilities);
//----------------------------------------------------------------------------
vtkSMFileUtilities::vtkSMFileUtilities() = default;

//----------------------------------------------------------------------------
vtkSMFileUtilities::~vtkSMFileUtilities() = default;

//----------------------------------------------------------------------------
bool vtkSMFileUtilities::MakeDirectory(const std::string& name, vtkTypeUInt32 location)
{
  auto pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    vtkErrorMacro("Missing proxy manager. Make sure the session was set correctly using "
                  "vtkSMFileUtilities::SetSession(...).");
    return false;
  }
  return ::Call(pxm, location,
    [&name](vtkSMDirectoryProxy* dirProxy) { return dirProxy->MakeDirectory(name.c_str()); });
}

//----------------------------------------------------------------------------
bool vtkSMFileUtilities::DeleteDirectory(const std::string& name, vtkTypeUInt32 location)
{
  auto pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    vtkErrorMacro("Missing proxy manager. Make sure the session was set correctly using "
                  "vtkSMFileUtilities::SetSession(...).");
    return false;
  }
  return ::Call(pxm, location,
    [&name](vtkSMDirectoryProxy* dirProxy) { return dirProxy->DeleteDirectory(name.c_str()); });
}

//----------------------------------------------------------------------------
bool vtkSMFileUtilities::RenameDirectory(
  const std::string& name, const std::string& newname, vtkTypeUInt32 location)
{
  auto pxm = this->GetSessionProxyManager();
  if (!pxm)
  {
    vtkErrorMacro("Missing proxy manager. Make sure the session was set correctly using "
                  "vtkSMFileUtilities::SetSession(...).");
    return false;
  }
  return ::Call(pxm, location, [&name, &newname](vtkSMDirectoryProxy* dirProxy) {
    return dirProxy->Rename(name.c_str(), newname.c_str());
  });
}

//----------------------------------------------------------------------------
void vtkSMFileUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
