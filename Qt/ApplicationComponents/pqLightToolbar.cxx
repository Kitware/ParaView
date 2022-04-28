/*=========================================================================

   Program: ParaView
   Module:    pqLightToolbar.cxx

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
