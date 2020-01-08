/*=========================================================================

Program:   ParaView
Module:    vtkSMExportProxyDepot.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExportProxyDepot.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMWriterFactory.h"
#include "vtkSMWriterProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

class vtkSMExportProxyDepot::Internal
{
public:
  Internal() {}
  ~Internal() {}
  vtkSmartPointer<vtkCollection> WriterCollection = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollectionIterator> WriterIterator =
    vtkSmartPointer<vtkCollectionIterator>::New();
  vtkSmartPointer<vtkCollection> ScreenshotCollection = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollectionIterator> ScreenshotIterator =
    vtkSmartPointer<vtkCollectionIterator>::New();
};

vtkStandardNewMacro(vtkSMExportProxyDepot);

//-----------------------------------------------------------------------------
vtkSMExportProxyDepot::vtkSMExportProxyDepot()
{
  this->Session = nullptr;
  this->Internals = new Internal;
}

//-----------------------------------------------------------------------------
vtkSMExportProxyDepot::~vtkSMExportProxyDepot()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMExportProxyDepot::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMExportProxyDepot::GetGlobalOptions()
{
  if (!this->Session)
  {
    return nullptr;
  }
  // todo: need to refactor to make this hacky search for the last
  // proxy uneccessary.
  // do that by making that the session delete old export proxies
  // to ensure that 'There can only be one.'
  vtkCollection* coll = vtkCollection::New();
  this->Session->GetProxies("export_global", "catalyst", coll);
  coll->InitTraversal();
  vtkObject* obj = coll->GetNextItemAsObject();
  vtkSMProxy* globalProxy = nullptr;
  while (obj != nullptr)
  {
    globalProxy = vtkSMProxy::SafeDownCast(obj);
    obj = coll->GetNextItemAsObject();
  }
  coll->Delete();
  if (!globalProxy)
  {
    globalProxy = this->Session->NewProxy("coprocessing", "CatalystGlobalOptions");
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->PreInitializeProxy(globalProxy);
    controller->PostInitializeProxy(globalProxy);
    this->Session->RegisterProxy("export_global", "catalyst", globalProxy);
    globalProxy->FastDelete();
  }
  return globalProxy;
}

//-----------------------------------------------------------------------------
bool vtkSMExportProxyDepot::HasWriterProxy(const char* filtername, const char* format)
{
  if (!this->Session)
  {
    return false;
  }
  std::string filterhash = filtername;
  filterhash += "|";
  filterhash += format;

  vtkSMSourceProxy* writerProxy =
    vtkSMSourceProxy::SafeDownCast(this->Session->GetProxy("export_writers", filterhash.c_str()));
  if (!writerProxy)
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMExportProxyDepot::GetWriterProxy(
  vtkSMSourceProxy* filter, const char* filtername, const char* format)
{
  if (!this->Session)
  {
    return nullptr;
  }
  std::string filterhash = filtername;
  filterhash += "|";
  filterhash += format;

  vtkSMSourceProxy* writerProxy =
    vtkSMSourceProxy::SafeDownCast(this->Session->GetProxy("export_writers", filterhash.c_str()));
  if (!writerProxy)
  {
    // first time accessed, make it
    vtkSmartPointer<vtkSMWriterFactory> wf = vtkSmartPointer<vtkSMWriterFactory>::New();
    auto sl = vtkSmartPointer<vtkStringList>::New();
    wf->GetGroups(sl);
    // we want our particular approved writers
    for (int i = 0; i < sl->GetNumberOfStrings(); i++)
    {
      wf->RemoveGroup(sl->GetString(i));
    }
    wf->AddGroup("insitu2_writer_parameters");
    wf->UpdateAvailableWriters();

    writerProxy = vtkSMWriterProxy::SafeDownCast(wf->CreateWriter(format, filter, 0, true));

    vtkPVXMLElement* hint = writerProxy->GetHints();
    std::string extension = "";
    // use first entry in the hint to choose a default file extension
    if (hint != nullptr)
    {
      vtkPVXMLElement* factory = hint->FindNestedElementByName("WriterFactory");
      if (factory != nullptr)
      {
        const char* exts = factory->GetAttribute("extensions");
        if (exts)
        {
          std::string asstr = exts;
          extension = asstr;
          size_t pos = asstr.find(" ");
          if (pos != std::string::npos)
          {
            extension = asstr.substr(0, pos);
          }
        }
      }
    }

    // swap the generic ".ext" extension from XML with a better one
    std::string fname = vtkSMPropertyHelper(writerProxy, "CatalystFilePattern").GetAsString();
    if (fname.substr(fname.length() - 3, fname.length()) == "ext")
    {
      fname = fname.substr(0, fname.length() - 3) + extension;
      vtkSMPropertyHelper(writerProxy, "CatalystFilePattern").Set(fname.c_str());
    }

    this->Session->RegisterProxy("export_writers", filterhash.c_str(), writerProxy);
    writerProxy->FastDelete();
  }
  return writerProxy;
}

//-----------------------------------------------------------------------------
void vtkSMExportProxyDepot::InitNextWriterProxy()
{
  this->Session->GetProxies("export_writers", this->Internals->WriterCollection);
  this->Internals->WriterIterator.TakeReference(this->Internals->WriterCollection->NewIterator());
};

//-----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMExportProxyDepot::GetNextWriterProxy()
{
  vtkSMSourceProxy* ret =
    vtkSMSourceProxy::SafeDownCast(this->Internals->WriterIterator->GetCurrentObject());
  this->Internals->WriterIterator->GoToNextItem();
  return ret;
};

//-----------------------------------------------------------------------------
bool vtkSMExportProxyDepot::HasScreenshotProxy(const char* viewname, const char* format)
{
  if (!this->Session)
  {
    return false;
  }

  std::string viewhash = viewname;
  viewhash += "|";
  viewhash += format;

  vtkSMProxy* ssProxy = this->Session->GetProxy("export_screenshots", viewhash.c_str());
  if (!ssProxy)
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMExportProxyDepot::GetScreenshotProxy(
  vtkSMProxy* view, const char* viewname, const char* format)
{
  if (!this->Session)
  {
    return nullptr;
  }
  std::string viewhash = viewname;
  viewhash += "|";
  viewhash += format;

  vtkSMProxy* ssProxy = this->Session->GetProxy("export_screenshots", viewhash.c_str());
  if (!ssProxy)
  {
    // first time accessed, make it
    ssProxy = this->Session->NewProxy("insitu2_writer_parameters", "SaveScreenshot");
    if (!ssProxy)
    {
      return nullptr;
    }

    std::string formatS = format;
    size_t dotP = formatS.find_first_of(".") + 1;
    size_t rparenP = formatS.find_last_of(")");
    std::string extension = formatS.substr(dotP, rparenP - dotP);

    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->PreInitializeProxy(ssProxy);
    vtkSMPropertyHelper(ssProxy, "View").Set(view);

    // swap the generic ".ext" extension from XML with a better one
    std::string fname = vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").GetAsString();
    fname = fname.substr(0, fname.length() - 3) + extension;
    vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").Set(fname.c_str());

    controller->PostInitializeProxy(ssProxy);

    this->Session->RegisterProxy("export_screenshots", viewhash.c_str(), ssProxy);
    ssProxy->FastDelete();
  }
  return ssProxy;
}

//-----------------------------------------------------------------------------
void vtkSMExportProxyDepot::InitNextScreenshotProxy()
{
  this->Session->GetProxies("export_screenshots", this->Internals->ScreenshotCollection);
  this->Internals->ScreenshotIterator.TakeReference(
    this->Internals->ScreenshotCollection->NewIterator());
};

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMExportProxyDepot::GetNextScreenshotProxy()
{
  vtkSMProxy* ret =
    vtkSMProxy::SafeDownCast(this->Internals->ScreenshotIterator->GetCurrentObject());
  this->Internals->ScreenshotIterator->GoToNextItem();
  return ret;
};
