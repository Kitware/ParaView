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

#include "vtkPVDataInformation.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QPointer>
#include <QStyle>

#include <algorithm>

//=============================================================================
class pqMultiBlockInspectorWidget::pqInternals : public QObject
{
  QPointer<pqDataRepresentation> Representation;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqProxyWidget> HelperProxyWidget;

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

  static bool isCompositeDataSet(pqDataRepresentation* repr)
  {
    auto reprProxy = repr ? repr->getProxy() : nullptr;
    auto inputProxy = reprProxy
      ? vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(reprProxy, "Input").GetAsProxy())
      : nullptr;
    auto dataInfo = inputProxy ? inputProxy->GetDataInformation() : nullptr;
    return dataInfo && dataInfo->IsCompositeDataSet();
  }

public:
  Ui::MultiBlockInspectorWidget Ui;

  static void resizeLabelPixmap(QLabel* iconLabel, int iconSize)
  {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QPixmap* pixmap = iconLabel->pixmap();
    iconLabel->setPixmap(
      pixmap->scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

#else

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) && QT_VERSION < QT_VERSION_CHECK(5, 16, 0)
    const QPixmap pixmap = iconLabel->pixmap(Qt::ReturnByValue);
#else
    const QPixmap pixmap = iconLabel->pixmap();
#endif

    iconLabel->setPixmap(
      pixmap.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
#endif
  }

  pqInternals(pqMultiBlockInspectorWidget* self)
  {
    this->Ui.setupUi(self);
    if (auto settings = pqApplicationCore::instance()->settings())
    {
      const bool checked = settings->value("pqMultiBlockInspectorWidget/ShowHints", true).toBool();
      this->Ui.showHints->setChecked(checked);
    }
    const int iconSize = std::max(self->style()->pixelMetric(QStyle::PM_SmallIconSize), 20);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateDisabled, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateRepresentationInherited, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateBlockInherited, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateMixedInherited, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateSet, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateSetAndRepresentationInherited, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateSetAndBlockInherited, iconSize);
    pqInternals::resizeLabelPixmap(this->Ui.iconStateSetAndMixedInherited, iconSize);
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
    this->Ui.extractBlocks->setEnabled(pqInternals::isCompositeDataSet(repr));
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
    const std::vector<std::string> selectors(selectorsProperty->GetUncheckedElements());
    // get assembly
    const auto repr = this->representation();
    const std::string assemblyName =
      repr && repr->getProxy() && repr->getProxy()->GetProperty("Assembly")
      ? vtkSMPropertyHelper(repr->getProxy(), "Assembly").GetAsString()
      : "Hierarchy";
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

  delete this->HelperProxyWidget;
  if (!port || !repr)
  {
    return;
  }

  if (pqInternals::hasAppearanceProperties(repr))
  {
    // Here we create a widget using the representation proxy and only expose the properties/groups
    // that have the panel visibility set to "multiblock_inspector". This way we instantiate a
    // pqDataAssemblyPropertyWidget which we later add to the container.
    this->HelperProxyWidget = new pqProxyWidget(repr->getProxy(), { "multiblock_inspector" }, {});
    QObject::connect(this->HelperProxyWidget.data(), &pqProxyWidget::changeFinished,
      [this, repr]()
      {
        this->Ui.extractBlocks->setEnabled(pqInternals::isCompositeDataSet(repr));
        repr->renderViewEventually();
      });
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
