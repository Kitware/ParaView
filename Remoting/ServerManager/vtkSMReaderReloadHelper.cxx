/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderReloadHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMReaderReloadHelper.h"

#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <vtksys/SystemTools.hxx>

namespace
{
vtkPVFileInformation* vtkFindFileGroupFor(vtkPVFileInformation* info, const std::string& unixFname)
{
  if (info == nullptr)
  {
    return nullptr;
  }

  if (!info->IsGroup() && !info->IsDirectory())
  {
    return nullptr;
  }

  for (int cc = 0, max = info->GetContents()->GetNumberOfItems(); cc < max; ++cc)
  {
    vtkPVFileInformation* item =
      vtkPVFileInformation::SafeDownCast(info->GetContents()->GetItemAsObject(cc));
    if (item == nullptr || item->GetFullPath() == nullptr)
    {
      continue;
    }

    if (vtkPVFileInformation::IsGroup(item->GetType()))
    {
      if (vtkPVFileInformation* found = vtkFindFileGroupFor(item, unixFname))
      {
        return found;
      }
    }
    else if ((item->GetType() == vtkPVFileInformation::SINGLE_FILE &&
               info->GetType() == vtkPVFileInformation::FILE_GROUP) ||
      (item->IsDirectory() && info->GetType() == vtkPVFileInformation::DIRECTORY_GROUP))
    {
      std::string unixFullPath(item->GetFullPath());
      vtksys::SystemTools::ConvertToUnixSlashes(unixFullPath);
      if (unixFname == unixFullPath)
      {
        return info;
      }
    }
  }
  return nullptr;
}

// Returns empty to indicate nothing found or don't bother updating.
std::vector<std::string> vtkGetFilesInSeries(vtkSMSessionProxyManager* pxm, const char* fname)
{
  std::string dir = vtksys::SystemTools::GetFilenamePath(fname);

  // We use unix-slashes so we can consistently compare filenames in
  // vtkFindFileGroupFor().
  std::string unixFname = fname;
  vtksys::SystemTools::ConvertToUnixSlashes(unixFname);

  vtkSmartPointer<vtkSMProxy> helper;
  helper.TakeReference(pxm->NewProxy("misc", "FileInformationHelper"));
  vtkSMPropertyHelper(helper, "WorkingDirectory").Set(dir.c_str());
  vtkSMPropertyHelper(helper, "Path").Set(dir.c_str());
  vtkSMPropertyHelper(helper, "SpecialDirectories").Set(0);
  vtkSMPropertyHelper(helper, "DirectoryListing").Set(1);
  helper->UpdateVTKObjects();

  vtkNew<vtkPVFileInformation> info;
  helper->GatherInformation(info.Get());

  if (vtkPVFileInformation* group = vtkFindFileGroupFor(info.Get(), unixFname))
  {
    std::vector<std::string> retval(group->GetContents()->GetNumberOfItems());
    for (int cc = 0, max = group->GetContents()->GetNumberOfItems(); cc < max; ++cc)
    {
      vtkPVFileInformation* item =
        vtkPVFileInformation::SafeDownCast(group->GetContents()->GetItemAsObject(cc));
      retval[cc] = item->GetFullPath();
    }
    return retval;
  }

  return std::vector<std::string>();
}
}

vtkStandardNewMacro(vtkSMReaderReloadHelper);
//----------------------------------------------------------------------------
vtkSMReaderReloadHelper::vtkSMReaderReloadHelper() = default;

//----------------------------------------------------------------------------
vtkSMReaderReloadHelper::~vtkSMReaderReloadHelper() = default;

//----------------------------------------------------------------------------
bool vtkSMReaderReloadHelper::SupportsReload(vtkSMSourceProxy* proxy)
{
  if (vtkPVXMLElement* hints = proxy ? proxy->GetHints() : nullptr)
  {
    if (vtkPVXMLElement* rfhints = hints->FindNestedElementByName("ReloadFiles"))
    {
      if (proxy->GetProperty(rfhints->GetAttributeOrEmpty("property")))
      {
        return true;
      }
    }
    return (hints->FindNestedElementByName("ReaderFactory") != nullptr &&
      vtkSMCoreUtilities::GetFileNameProperty(proxy) != nullptr);
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderReloadHelper::SupportsFileSeries(vtkSMSourceProxy* proxy)
{
  if (this->SupportsReload(proxy))
  {
    const char* pname = vtkSMCoreUtilities::GetFileNameProperty(proxy);
    vtkSMStringVectorProperty* svp =
      vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(pname));
    return svp && svp->GetRepeatable();
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderReloadHelper::ReloadFiles(vtkSMSourceProxy* proxy)
{
  if (!this->SupportsReload(proxy))
  {
    return false;
  }

  assert(proxy);

  SM_SCOPED_TRACE(CallFunction).arg("ReloadFiles").arg(proxy);

  vtkPVXMLElement* hints = proxy->GetHints();
  vtkPVXMLElement* rfhints = hints ? hints->FindNestedElementByName("ReloadFiles") : nullptr;
  if (rfhints && proxy->GetProperty(rfhints->GetAttributeOrEmpty("property")))
  {
    proxy->InvokeCommand(rfhints->GetAttributeOrEmpty("property"));
  }
  else
  {
    proxy->RecreateVTKObjects();
  }
  proxy->UpdatePipelineInformation();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMReaderReloadHelper::ExtendFileSeries(vtkSMSourceProxy* proxy)
{
  if (!this->SupportsFileSeries(proxy))
  {
    return false;
  }

  SM_SCOPED_TRACE(CallFunction).arg("ExtendFileSeries").arg(proxy);

  const char* pname = vtkSMCoreUtilities::GetFileNameProperty(proxy);
  assert(pname);

  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty(pname));
  assert(svp && svp->GetRepeatable());
  if (svp->GetNumberOfElements() < 1)
  {
    vtkErrorMacro("Reader has not files specified. At least 1 file must be specified "
                  "to be able to add more to the series.");
    return false;
  }

  std::vector<std::string> files =
    vtkGetFilesInSeries(proxy->GetSessionProxyManager(), svp->GetElement(0));
  if (files.size() > 0)
  {
    svp->SetElements(files);
    proxy->UpdateVTKObjects();
    proxy->UpdatePipelineInformation();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMReaderReloadHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
