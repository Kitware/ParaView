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
#include "pqUndoStack.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

//-----------------------------------------------------------------------------
pqLoadPaletteReaction::pqLoadPaletteReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QMenu* menu = new QMenu();
  menu->setObjectName("LoadPaletteMenu");
  parentObject->setMenu(menu);
  this->connect(menu, SIGNAL(aboutToShow()), SLOT(populateMenu()));
  this->connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)),
    SLOT(updateEnableState()));
  this->connect(menu, SIGNAL(triggered(QAction*)), SLOT(actionTriggered(QAction*)));
}

//-----------------------------------------------------------------------------
pqLoadPaletteReaction::~pqLoadPaletteReaction()
{
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::updateEnableState()
{
  this->parentAction()->setEnabled(
    pqActiveObjects::instance().activeServer() != NULL);
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::populateMenu()
{
  QMenu* menu = qobject_cast<QMenu*>(this->sender());
  menu->clear();

  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  Q_ASSERT(pxm);

  vtkSMProxyDefinitionManager* pdmgr = pxm->GetProxyDefinitionManager();
  Q_ASSERT(pdmgr);

  vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
  iter.TakeReference(pdmgr->NewSingleGroupIterator("palettes"));
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    if (vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName()))
      {
      QAction* actn = menu->addAction(prototype->GetXMLLabel());
      actn->setProperty("PV_XML_GROUP", "palettes");
      actn->setProperty("PV_XML_NAME", iter->GetProxyName());
      }
    }
}

//-----------------------------------------------------------------------------
void pqLoadPaletteReaction::actionTriggered(QAction* actn)
{
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  Q_ASSERT(pxm);

  vtkSMProxy* paletteProxy = pxm->GetProxy("global_properties", "ColorPalette");

  vtkSMProxy* palettePrototype = pxm->GetPrototypeProxy("palettes",
    actn->property("PV_XML_NAME").toString().toLatin1().data());
  Q_ASSERT(palettePrototype);

  BEGIN_UNDO_SET("Load color palette");
  paletteProxy->Copy(palettePrototype);
  paletteProxy->UpdateVTKObjects();
  END_UNDO_SET();

  pqApplicationCore::instance()->render();
}
