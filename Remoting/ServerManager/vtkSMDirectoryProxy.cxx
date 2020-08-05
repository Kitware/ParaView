/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDirectoryProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

vtkStandardNewMacro(vtkSMDirectoryProxy);
//----------------------------------------------------------------------------
vtkSMDirectoryProxy::vtkSMDirectoryProxy()
{
}

//----------------------------------------------------------------------------
vtkSMDirectoryProxy::~vtkSMDirectoryProxy()
{
}

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
  if (secondaryPath != NULL)
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
