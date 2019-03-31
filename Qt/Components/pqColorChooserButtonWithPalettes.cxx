/*=========================================================================

   Program: ParaView
   Module:  pqColorChooserButtonWithPalettes.cxx

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
#include "pqColorChooserButtonWithPalettes.h"

#include "pqActiveObjects.h"
#include "pqServer.h"
#include "pqSetName.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMSessionProxyManager.h"

// Qt Includes.
#include <QAction>
#include <QActionGroup>
#include <QColorDialog>
#include <QIcon>
#include <QMenu>

#include <cassert>

//-----------------------------------------------------------------------------
pqColorChooserButtonWithPalettes::pqColorChooserButtonWithPalettes(QWidget* parentObject)
  : Superclass(parentObject)
{
  // Setup a popup menu.
  QMenu* popupMenu = new QMenu(this);
  popupMenu << pqSetName("ColorPaletteMenu");
  this->setMenu(popupMenu);
  this->connect(popupMenu, SIGNAL(aboutToShow()), SLOT(updateMenu()));
  this->connect(popupMenu, SIGNAL(triggered(QAction*)), SLOT(actionTriggered(QAction*)));

  this->setPopupMode(QToolButton::MenuButtonPopup);

  // although unnecessary, fillup the menu here so that test playback
  // can work correctly.
  this->updateMenu();
}

//-----------------------------------------------------------------------------
pqColorChooserButtonWithPalettes::~pqColorChooserButtonWithPalettes()
{
}

//-----------------------------------------------------------------------------
vtkSMGlobalPropertiesProxy* pqColorChooserButtonWithPalettes::colorPalette() const
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkSMSessionProxyManager* pxm = server ? server->proxyManager() : NULL;
  return (pxm
      ? vtkSMGlobalPropertiesProxy::SafeDownCast(pxm->GetProxy("global_properties", "ColorPalette"))
      : NULL);
}

//-----------------------------------------------------------------------------
void pqColorChooserButtonWithPalettes::updateMenu()
{
  QMenu* popupMenu = this->menu();
  assert(popupMenu);

  popupMenu->clear();

  delete this->ActionGroup;
  this->ActionGroup = new QActionGroup(popupMenu);

  // Add palettes colors
  vtkSMProxy* cp = this->colorPalette();
  if (!cp)
  {
    return;
  }

  QString paletteColorName;

  pqColorPaletteLinkHelper* helper = this->findChild<pqColorPaletteLinkHelper*>();
  if (helper)
  {
    paletteColorName = helper->selectedPaletteColor();
  }

  vtkSMPropertyIterator* iter = cp->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(iter->GetProperty());

    if (dvp && dvp->GetNumberOfElements() == 3)
    {
      QColor qcolor;
      qcolor.setRgbF(dvp->GetElement(0), dvp->GetElement(1), dvp->GetElement(2));

      QAction* action = popupMenu->addAction(this->renderColorSwatch(qcolor), dvp->GetXMLLabel());
      action << pqSetName(iter->GetKey());
      action->setData(QVariant(iter->GetKey()));
      action->setCheckable(true);
      if (paletteColorName == iter->GetKey())
      {
        action->setChecked(true);
      }
      this->ActionGroup->addAction(action);
    }
  }

  // Add B&W stock colors
  popupMenu->addSeparator();
  QColor black(Qt::black);
  popupMenu->addAction(this->renderColorSwatch(black), tr("Black"))->setData(black);
  QColor white(Qt::white);
  popupMenu->addAction(this->renderColorSwatch(white), tr("White"))->setData(white);

  iter->Delete();
}

//-----------------------------------------------------------------------------
void pqColorChooserButtonWithPalettes::actionTriggered(QAction* action)
{
  QColor color;
  if (action->data().type() == QVariant::String)
  {
    QString prop_name = action->data().toString();
    vtkSMProxy* globalProps = this->colorPalette();
    assert(globalProps);

    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      globalProps->GetProperty(prop_name.toLocal8Bit().data()));
    color.setRgbF(dvp->GetElement(0), dvp->GetElement(1), dvp->GetElement(2));
    pqColorPaletteLinkHelper* helper = this->findChild<pqColorPaletteLinkHelper*>();
    if (helper)
    {
      helper->setSelectedPaletteColor(prop_name);
    }
  }
  else if (action->data().type() == QVariant::Color)
  {
    color = action->data().value<QColor>();
  }

  this->setChosenColor(color);
}

//============================================================================
//  CLASS: pqColorPaletteLinkHelper
//============================================================================

//-----------------------------------------------------------------------------
pqColorPaletteLinkHelper::pqColorPaletteLinkHelper(
  pqColorChooserButtonWithPalettes* button, vtkSMProxy* smproxy, const char* smproperty)
  : Superclass(button)
  , SMProxy(smproxy)
  , SMPropertyName(smproperty)
{
  assert(button && smproperty && smproxy);
}

//-----------------------------------------------------------------------------
pqColorPaletteLinkHelper::~pqColorPaletteLinkHelper()
{
}

//-----------------------------------------------------------------------------
void pqColorPaletteLinkHelper::setSelectedPaletteColor(const QString& colorName)
{
  pqColorChooserButtonWithPalettes* button =
    qobject_cast<pqColorChooserButtonWithPalettes*>(this->parent());
  assert(button);

  vtkSMGlobalPropertiesProxy* palette = button->colorPalette();
  if (palette && this->SMProxy)
  {
    palette->Link(
      colorName.toLocal8Bit().data(), this->SMProxy, this->SMPropertyName.toLocal8Bit().data());
  }
}

//-----------------------------------------------------------------------------
QString pqColorPaletteLinkHelper::selectedPaletteColor() const
{
  pqColorChooserButtonWithPalettes* button =
    qobject_cast<pqColorChooserButtonWithPalettes*>(this->parent());
  assert(button);

  vtkSMGlobalPropertiesProxy* palette = button->colorPalette();
  if (palette && this->SMProxy)
  {
    return palette->GetLinkedPropertyName(this->SMProxy, this->SMPropertyName.toLocal8Bit().data());
  }

  return QString();
}
