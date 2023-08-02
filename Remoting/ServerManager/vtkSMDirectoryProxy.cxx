// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDirectoryProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

vtkStandardNewMacro(vtkSMDirectoryProxy);
//----------------------------------------------------------------------------
vtkSMDirectoryProxy::vtkSMDirectoryProxy() = default;

//----------------------------------------------------------------------------
vtkSMDirectoryProxy::~vtkSMDirectoryProxy() = default;

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::List(const char* dir)
{
  return this->CallDirectoryMethod("OpenDirectory", dir);
}

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::MakeDirectory(const char* dir)
{
  return this->CallDirectoryMethod("MakeDirectory", dir);
}

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::DeleteDirectory(const char* dir)
{
  return this->CallDirectoryMethod("DeleteDirectory", dir);
}

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::Rename(const char* oldname, const char* newname)
{
  return this->CallDirectoryMethod("RenameDirectory", oldname, newname);
}

bool vtkSMDirectoryProxy::CallDirectoryMethod(
  const char* method, const char* path, const char* secondaryPath)
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return false;
  }

  // create a helper for calling a method on vtk objects
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(this->GetSession());
  vtkSmartPointer<vtkSMProxy> helper;
  helper.TakeReference(pxm->NewProxy("misc", "FilePathEncodingHelper"));
  helper->SetLocation(this->GetLocation());
  vtkSMPropertyHelper(helper->GetProperty("ActiveFileName")).Set(path);
  if (secondaryPath != nullptr)
  {
    vtkSMPropertyHelper(helper->GetProperty("SecondaryFileName")).Set(secondaryPath);
  }
  vtkSMPropertyHelper(helper->GetProperty("ActiveGlobalId"))
    .Set(static_cast<vtkIdType>(this->GetGlobalID()));
  helper->UpdateVTKObjects();
  helper->UpdatePropertyInformation(helper->GetProperty(method));

  int ret = vtkSMPropertyHelper(helper->GetProperty(method)).GetAsInt();
  return (ret != 0);
}

//----------------------------------------------------------------------------
void vtkSMDirectoryProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
