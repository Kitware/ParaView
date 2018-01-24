/*=========================================================================

   Program: ParaView
   Module:    pqCatalystExportNowReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqCatalystExportNowReaction.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVConfig.h"
#include "vtkSMExportProxyDepot.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include <iostream>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
pqCatalystExportNowReaction::pqCatalystExportNowReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqCatalystExportNowReaction::~pqCatalystExportNowReaction()
{
}

//-----------------------------------------------------------------------------
void pqCatalystExportNowReaction::onTriggered()
{
// todo: think about getting by without any python
#ifdef PARAVIEW_ENABLE_PYTHON
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMExportProxyDepot* ed = pxm->GetExportDepot();

  vtkSMProxy* globaloptions = ed->GetGlobalOptions();
  if (!globaloptions)
  {
    // todo: warn about this unexpected condition?
    cerr << "THAT's ODD" << endl;
  }

  vtkSmartPointer<vtkCollection> configs = vtkSmartPointer<vtkCollection>::New();
  pxm->GetProxies("export_configurations", configs);
  vtkCollectionIterator* it = configs->NewIterator();
  int i = 0;
  bool exported_any = false;
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    i++;
    vtkSMProxy* pxy = vtkSMProxy::SafeDownCast(it->GetCurrentObject());
    const char* proxyname = pxm->GetProxyName("export_configurations", pxy);
    std::string gotten = proxyname;
    std::stringstream tosep(gotten);
    std::string typestr, inputname, format;
    getline(tosep, typestr, ',');
    getline(tosep, inputname, ',');
    getline(tosep, format, ',');

    if (typestr == "screenshot")
    {
      vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(pxy);
      if (!shProxy)
      {
        // todo: warn about this unexpected condition?
        continue;
      }
      // todo: get extension better, and manufacture actual filename from state
      int dotP = format.find_first_of(".") + 1;
      int rparenP = format.find_last_of(")");
      std::string filename = "foo_" + std::to_string(i) + "." + format.substr(dotP, rparenP - dotP);
      shProxy->WriteImage(filename.c_str());
      exported_any = true;
    }
    else if (typestr == "filter")
    {
      vtkSMSourceProxy* writerProxy = vtkSMSourceProxy::SafeDownCast(pxy);
      if (!writerProxy)
      {
        // todo: warn about this unexpected condition?
        continue;
      }
      // todo: get extension somehow, and manufacture actual filename from state
      vtkSMPropertyHelper(writerProxy, "FileName").Set("foo.pvd");
      writerProxy->UpdateVTKObjects();
      writerProxy->UpdatePipeline();
      exported_any = true;
    }
    else
    {
      // todo: warn about this unexpected condition?
      continue;
    }
  }
  it->Delete();
  if (!exported_any)
  {
    qWarning(
      "Nothing to export, use Catalyst Export Inspector to configure what you want to write.");
  }
#endif
}
