// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMultiBlockInspectorWidget.h"
#include "ui_pqMultiBlockInspectorWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqProxyWidget.h"
#include "pqSettings.h"

#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QPointer>

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
    this->Ui.extractBlocks->setEnabled(repr != nullptr);
  }

  void extract()
  {
    // first save selectors, because if you create the filter first
    // the port/representation of the inspector will change and the selectors will be reset.
    auto helperProxy = this->HelperProxyWidget->proxy();
    // we need to check both BlockSelectors and Selectors because different properties are available
    // if hasAppearanceProperties is true, see update()
    auto selectorsProperty = vtkSMStringVectorProperty::SafeDownCast(
      helperProxy->GetProperty("BlockSelectors") ? helperProxy->GetProperty("BlockSelectors")
                                                 : helperProxy->GetProperty("Selectors"));
    Q_ASSERT(selectorsProperty != nullptr);
    std::vector<std::string> selectors(selectorsProperty->GetUncheckedElements());
    // get assembly
    const auto repr = this->representation();
    std::string assemblyName = "Hierarchy";
    if (repr && repr->getProxy() && repr->getProxy()->GetProperty("Assembly"))
    {
      assemblyName = vtkSMPropertyHelper(repr->getProxy(), "Assembly").GetAsString();
    }
    // create extract block filter
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    auto extractBlockFilter = builder->createFilter(
      "filters", "ExtractBlock", this->OutputPort->getSource(), this->OutputPort->getPortNumber());
    auto extractBlockProxy = extractBlockFilter->getProxy();
    // set assembly
    vtkSMPropertyHelper(extractBlockProxy, "Assembly").Set(assemblyName.c_str());
    // copy selectors
    unsigned int idx = 0;
    for (const auto& selector : selectors)
    {
      vtkSMPropertyHelper(extractBlockProxy, "Selectors").Set(idx++, selector.c_str());
    }
    extractBlockProxy->UpdateVTKObjects();
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
    QObject::connect(this->HelperProxyWidget.data(), &pqProxyWidget::changeFinished,
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
    QObject::connect(
      internals.Ui.extractBlocks, &QPushButton::clicked, &internals, &pqInternals::extract);

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
