// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLoadPaletteReaction.h"

#include <QAction>
#include <QMenu>

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqApplicationSettingsReaction.h"
#include "pqUndoStack.h"
#include "vtkLogger.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QCoreApplication>
#include <array>
#include <cassert>
#include <string>

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

  // Palette ordering / ban list can be found in issue #20707
  std::array<std::string, 8> mainPalettes = { "BlueGrayBackground", "WarmGrayBackground",
    "DarkGrayBackground", "NeutralGrayBackground", "LightGrayBackground", "WhiteBackground",
    "BlackBackground", "GradientBackground" };

  for (const std::string& str : mainPalettes)
  {
    const char* name = str.c_str();
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", name))
    {
      QAction* actn =
        menu->addAction(QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()));
      actn->setProperty("PV_XML_GROUP", "palettes");
      actn->setProperty("PV_XML_NAME", name);
    }
    else
    {
      vtkLog(WARNING, "Missing palette: " << name);
    }
  }

  // If there were any other available palettes, we append them in alphabetical order.
  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewSingleGroupIterator("palettes"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName()))
    {
      if (std::find(mainPalettes.cbegin(), mainPalettes.cend(),
            std::string(iter->GetProxyName())) != mainPalettes.cend())
      {
        // skip main palettes since already added.
        continue;
      }
      QAction* actn =
        menu->addAction(QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()));
      actn->setProperty("PV_XML_GROUP", "palettes");
      actn->setProperty("PV_XML_NAME", iter->GetProxyName());
    }
  }
  menu->addAction(tr("Edit Current Palette ..."));
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::actionTriggered(QAction* actn)
{
  if (actn->property("PV_XML_NAME").isValid())
  {
    vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
    assert(pxm);

    vtkSMProxy* paletteProxy = pxm->GetProxy("settings", "ColorPalette");

    vtkSMProxy* palettePrototype =
      pxm->GetPrototypeProxy("palettes", actn->property("PV_XML_NAME").toString().toUtf8().data());
    assert(palettePrototype);

    BEGIN_UNDO_SET(tr("Load color palette"));
    SM_SCOPED_TRACE(CallFunction)
      .arg("LoadPalette")
      .arg("paletteName", actn->property("PV_XML_NAME").toString().toUtf8().data());
    paletteProxy->Copy(palettePrototype);
    paletteProxy->UpdateVTKObjects();
    END_UNDO_SET();

    // Also save to settings to ensure the palette loaded is preserved on
    // restart.
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    settings->SetProxySettings(paletteProxy, nullptr);
    pqApplicationCore::instance()->render();
  }
  else
  {
    pqApplicationSettingsReaction::showApplicationSettingsDialog(tr("Color Palette"));
  }
}
