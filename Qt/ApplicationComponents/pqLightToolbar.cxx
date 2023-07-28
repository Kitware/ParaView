// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLightToolbar.h"
#include "ui_pqLightToolbar.h"

#include "pqActiveObjects.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "vtkPVRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMTrace.h"

#include <QIcon>
#include <cmath>

class pqLightToolbar::pqInternals : public Ui::pqLightToolbar
{
public:
  pqRenderView* View = nullptr;
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
pqLightToolbar::pqLightToolbar(const QString& title, QWidget* parentObject)
  : Superclass(title, parentObject)
  , Internals(new pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
pqLightToolbar::pqLightToolbar(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  this->constructor();
}

//-----------------------------------------------------------------------------
void pqLightToolbar::constructor()
{
  this->Internals->setupUi(this);

  QIcon LightIcon;
  LightIcon.addFile(":/pqWidgets/Icons/pqLightOn.svg", QSize(), QIcon::Normal, QIcon::On);
  LightIcon.addFile(":/pqWidgets/Icons/pqLightOff.svg", QSize(), QIcon::Normal, QIcon::Off);
  this->Internals->actionActiveLightKit->setIcon(LightIcon);

  this->Internals->Links.setUseUncheckedProperties(false);
  this->Internals->Links.setAutoUpdateVTKObjects(true);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));

  QObject::connect(
    this->Internals->actionActiveLightKit, SIGNAL(triggered()), this, SLOT(toggleLightKit()));
}

//-----------------------------------------------------------------------------
pqLightToolbar::~pqLightToolbar() = default;

//-----------------------------------------------------------------------------
void pqLightToolbar::setView(pqView* view)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(view);
  if (this->Internals->View == view)
  {
    return;
  }

  this->Internals->View = renderView;
  this->Internals->Links.removeAllPropertyLinks();

  bool enabled =
    this->Internals->View && this->Internals->View->getProxy()->GetProperty("UseLight");
  this->Internals->actionActiveLightKit->setEnabled(enabled);

  if (enabled)
  {
    this->Internals->Links.addPropertyLink(this->Internals->actionActiveLightKit, "checked",
      SIGNAL(toggled(bool)), view->getProxy(), view->getProxy()->GetProperty("UseLight"));
  }
}

//-----------------------------------------------------------------------------
void pqLightToolbar::toggleLightKit()
{
  if (!this->Internals->View)
  {
    return;
  }

  bool useLight = vtkSMPropertyHelper(this->Internals->View->getProxy(), "UseLight").GetAsInt();

  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", this->Internals->View->getProxy())
    .arg("comment", QString(" %1 light kit").arg(useLight ? "enable" : "disable").toUtf8().data());
  vtkSMPropertyHelper(this->Internals->View->getProxy(), "UseLight").Set(useLight);

  this->Internals->View->render();
}
