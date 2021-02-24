/*=========================================================================

   Program: ParaView
   Module:  pqLoadPaletteReaction.cxx

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
#include "pqLoadPaletteReaction.h"

#include <QAction>
#include <QMenu>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqApplicationSettingsReaction.h"
#include "pqUndoStack.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqLoadPaletteReaction::pqLoadPaletteReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QMenu* menu = new QMenu();
  this->Menu = menu;
  menu->setObjectName("LoadPaletteMenu");
  parentObject->setMenu(menu);
  this->connect(menu, SIGNAL(aboutToShow()), SLOT(populateMenu()));
  this->connect(
    &pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)), SLOT(updateEnableState()));
  this->connect(menu, SIGNAL(triggered(QAction*)), SLOT(actionTriggered(QAction*)));
}

//-----------------------------------------------------------------------------
pqLoadPaletteReaction::~pqLoadPaletteReaction()
{
  if (QAction* pa = this->parentAction())
  {
    pa->setMenu(nullptr);
  }
  delete this->Menu;
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::updateEnableState()
{
  this->parentAction()->setEnabled(pqActiveObjects::instance().activeServer() != nullptr);
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::populateMenu()
{
  QMenu* menu = qobject_cast<QMenu*>(this->sender());
  menu->clear();

  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  assert(pxm);

  vtkSMProxyDefinitionManager* pdmgr = pxm->GetProxyDefinitionManager();
  assert(pdmgr);

  // Add "DefaultBackground" as the first entry.
  if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", "DefaultBackground"))
  {
    QAction* actn = menu->addAction(prototype->GetXMLLabel());
    actn->setProperty("PV_XML_GROUP", "palettes");
    actn->setProperty("PV_XML_NAME", "DefaultBackground");
  }

  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewSingleGroupIterator("palettes"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName()))
    {
      if (strcmp(prototype->GetXMLName(), "DefaultBackground") == 0)
      {
        // skip DefaultBackground since already added.
        continue;
      }
      QAction* actn = menu->addAction(prototype->GetXMLLabel());
      actn->setProperty("PV_XML_GROUP", "palettes");
      actn->setProperty("PV_XML_NAME", iter->GetProxyName());
    }
  }
  menu->addAction("Edit Current Palette ...");
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::actionTriggered(QAction* actn)
{
  if (actn->property("PV_XML_NAME").isValid())
  {
    vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
    assert(pxm);

    vtkSMProxy* paletteProxy = pxm->GetProxy("settings", "ColorPalette");

    vtkSMProxy* palettePrototype = pxm->GetPrototypeProxy(
      "palettes", actn->property("PV_XML_NAME").toString().toLocal8Bit().data());
    assert(palettePrototype);

    BEGIN_UNDO_SET("Load color palette");
    SM_SCOPED_TRACE(CallFunction)
      .arg("LoadPalette")
      .arg("paletteName", actn->property("PV_XML_NAME").toString().toLocal8Bit().data());
    paletteProxy->Copy(palettePrototype);
    paletteProxy->UpdateVTKObjects();
    END_UNDO_SET();

    pqApplicationCore::instance()->render();
  }
  else
  {
    pqApplicationSettingsReaction::showApplicationSettingsDialog("Color Palette");
  }
}
