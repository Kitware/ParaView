/*=========================================================================

   Program: ParaView
   Module:  pqMultiBlockInspectorWidget.cxx

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
#include "pqMultiBlockInspectorWidget.h"
#include "ui_pqMultiBlockInspectorWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqProxyWidget.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkPVLogger.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <QPointer>

#include <cassert>

//=============================================================================
class pqMultiBlockInspectorWidget::pqInternals : public QObject
{
  QPointer<pqDataRepresentation> Representation;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqProxyWidget> HelperProxyWidget;

  void* LastOutputPort = nullptr;
  void* LastRepresentation = nullptr;

  vtkSmartPointer<vtkSMProxy> HelperProxy;

  vtkSMProxy* helperProxy()
  {
    if (this->HelperProxy == nullptr)
    {
      auto pxm = pqActiveObjects::instance().proxyManager();
      Q_ASSERT(pxm != nullptr);

      this->HelperProxy.TakeReference(pxm->NewProxy("misc", "MultiBlockInspectorHelper"));
      this->HelperProxy->SetLocation(0);
      this->HelperProxy->SetPrototype(true);
    }
    return this->HelperProxy;
  };

  void update();
  void representationAdded(pqOutputPort*, pqDataRepresentation*) { this->update(); }

  static bool hasAppearanceProperties(pqDataRepresentation* repr)
  {
    auto proxy = repr ? repr->getProxy() : nullptr;
    if (proxy == nullptr)
    {
      return false;
    }

    for (size_t cc = 0, max = proxy->GetNumberOfPropertyGroups(); cc < max; ++cc)
    {
      auto group = proxy->GetPropertyGroup(cc);
      if (group && group->GetPanelVisibility() != nullptr &&
        strcmp(group->GetPanelVisibility(), "multiblock_inspector") == 0)
      {
        return true;
      }
    }

    return false;
  }

public:
  Ui::MultiBlockInspectorWidget Ui;

  pqInternals(pqMultiBlockInspectorWidget* self)
    : LastOutputPort(nullptr)
    , LastRepresentation(nullptr)
  {
    this->Ui.setupUi(self);
    if (auto settings = pqApplicationCore::instance()->settings())
    {
      bool checked = settings->value("pqMultiBlockInspectorWidget/ShowHints", true).toBool();
      this->Ui.showHints->setChecked(checked);
    }
  }
  ~pqInternals() override
  {
    if (auto settings = pqApplicationCore::instance()->settings())
    {
      settings->setValue("pqMultiBlockInspectorWidget/ShowHints", this->Ui.showHints->isChecked());
    }
  }

  pqOutputPort* outputPort() const { return this->OutputPort; }
  void setOutputPort(pqOutputPort* port)
  {
    if (this->OutputPort != port)
    {
      this->OutputPort = port;
      this->update();
    }
  }

  pqDataRepresentation* representation() const { return this->Representation; }
  void setRepresentation(pqDataRepresentation* repr)
  {
    if (this->Representation != repr)
    {
      this->Representation = repr;
      this->update();
    }
  }
};

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::pqInternals::update()
{
  auto port = this->outputPort();
  auto repr = this->representation();

  if (port == this->LastOutputPort && repr == this->LastRepresentation)
  {
    // nothing has changed.
    return;
  }

  delete this->HelperProxyWidget;
  this->LastOutputPort = port;
  this->LastRepresentation = repr;
  if (!port)
  {
    return;
  }

  if (pqInternals::hasAppearanceProperties(repr))
  {
    this->HelperProxyWidget = new pqProxyWidget(repr->getProxy(), { "multiblock_inspector" }, {});
    QObject::connect(this->HelperProxyWidget, &pqProxyWidget::changeFinished,
      [repr]() { repr->renderViewEventually(); });
  }
  else
  {
    auto panelProxy = this->helperProxy();
    vtkSMPropertyHelper(panelProxy, "Input").Set(port->getSourceProxy(), port->getPortNumber());
    this->HelperProxyWidget = new pqProxyWidget(panelProxy);
  }
  this->HelperProxyWidget->setApplyChangesImmediately(true);
  this->HelperProxyWidget->updatePanel();
  this->Ui.container->layout()->addWidget(this->HelperProxyWidget);
}

//-----------------------------------------------------------------------------
pqMultiBlockInspectorWidget::pqMultiBlockInspectorWidget(
  QWidget* parentObject, Qt::WindowFlags f, bool arg_autotracking)
  : Superclass(parentObject, f)
  , Internals(new pqMultiBlockInspectorWidget::pqInternals(this))
  , AutoTracking(arg_autotracking)
{
  auto& internals = (*this->Internals);

  // hookups for auto-tracking.
  if (this->AutoTracking)
  {
    auto& activeObjects = pqActiveObjects::instance();
    QObject::connect(
      &activeObjects, &pqActiveObjects::portChanged, &internals, &pqInternals::setOutputPort);
    QObject::connect(&activeObjects,
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), &internals,
      &pqInternals::setRepresentation);

    internals.setOutputPort(activeObjects.activePort());
    internals.setRepresentation(activeObjects.activeRepresentation());
  }
}

//-----------------------------------------------------------------------------
pqMultiBlockInspectorWidget::~pqMultiBlockInspectorWidget() = default;

//-----------------------------------------------------------------------------
pqOutputPort* pqMultiBlockInspectorWidget::outputPort() const
{
  const auto& internals = (*this->Internals);
  return internals.outputPort();
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqMultiBlockInspectorWidget::representation() const
{
  const auto& internals = (*this->Internals);
  return internals.representation();
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setOutputPort(pqOutputPort* port)
{
  if (this->isAutoTrackingEnabled())
  {
    qDebug("`setOutputPort` called when auto-tracking is enabled. Ignored.");
  }
  else
  {
    auto& internals = (*this->Internals);
    internals.setOutputPort(port);
  }
}

//-----------------------------------------------------------------------------
void pqMultiBlockInspectorWidget::setRepresentation(pqDataRepresentation* repr)
{
  if (this->isAutoTrackingEnabled())
  {
    qDebug("`setRepresentation` called when auto-tracking is enabled. Ignored.");
  }
  else
  {
    auto& internals = (*this->Internals);
    internals.setRepresentation(repr);
  }
}
