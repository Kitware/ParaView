// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProxySelectionWidget.h"
#include "ui_pqProxySelectionWidget.h"

#include "pqComboBoxDomain.h"
#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include "Private/pqComboBoxStyle.h"

#include <QPointer>

#include <cassert>

//-----------------------------------------------------------------------------
class pqProxySelectionWidget::pqInternal
{
public:
  vtkWeakPointer<vtkSMProxy> ChosenProxy;
  vtkSmartPointer<vtkSMProxyListDomain> Domain;
  Ui::ProxySelectionWidget Ui;
  QPointer<pqProxyWidget> ProxyWidget;
  bool ShowingAdvancedProperties;
  bool HideProxyWidgetsInDefaultView;
  bool HideProxyWidgets;
  pqInternal(pqProxySelectionWidget* self)
    : ShowingAdvancedProperties(false)
    , HideProxyWidgetsInDefaultView(false)
    , HideProxyWidgets(false)
  {
    this->Ui.setupUi(self);
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.verticalLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.horizontalLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.horizontalLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.frameLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  }

  void setChosenProxy(vtkSMProxy* proxy, pqProxySelectionWidget* self)
  {
    delete this->ProxyWidget;
    this->ChosenProxy = proxy;
    if (proxy)
    {
      this->ProxyWidget = new pqProxyWidget(proxy, this->Ui.frame);
      this->ProxyWidget->setObjectName("ChosenProxyWidget");
      this->ProxyWidget->setApplyChangesImmediately(false);
      this->Ui.frameLayout->insertWidget(0, this->ProxyWidget);
      this->updateWidget(this->ShowingAdvancedProperties);
      this->ProxyWidget->setView(self->view());

      // Propagate signals from the internal ProxyWidget out from `self`.
      self->connect(this->ProxyWidget, SIGNAL(changeAvailable()), SIGNAL(changeAvailable()));
      self->connect(this->ProxyWidget, SIGNAL(changeFinished()), SIGNAL(changeFinished()));
    }
  }

  void updateWidget(bool showing_advanced_properties)
  {
    this->ShowingAdvancedProperties = showing_advanced_properties;
    if (this->ProxyWidget)
    {
      if (this->HideProxyWidgets ||
        (!showing_advanced_properties && this->HideProxyWidgetsInDefaultView))
      {
        this->ProxyWidget->hide();
      }
      else
      {
        this->ProxyWidget->show();
        this->ProxyWidget->filterWidgets(showing_advanced_properties);
      }
    }
  }
};

//-----------------------------------------------------------------------------
pqProxySelectionWidget::pqProxySelectionWidget(
  vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internal(new pqProxySelectionWidget::pqInternal(this))
{
  this->Internal->Ui.label->setText(
    QCoreApplication::translate("ServerManagerXML", smproperty->GetXMLLabel()));
  this->Internal->Domain = smproperty->FindDomain<vtkSMProxyListDomain>();

  // This widget is intended to be used for properties with ProxyListDomains
  // alone.
  assert(this->Internal->Domain);
  this->connect(
    this->Internal->Ui.comboBox, SIGNAL(currentIndexChanged(int)), SLOT(currentIndexChanged(int)));
  this->Internal->Ui.comboBox->setStyle(new pqComboBoxStyle(/*showPopup=*/false));
  this->Internal->Ui.comboBox->setMaxVisibleItems(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smproperty->GetHints()));
  new pqComboBoxDomain(this->Internal->Ui.comboBox, smproperty, this->Internal->Domain);
  this->addPropertyLink(this, "chosenProxy", SIGNAL(chosenProxyChanged()), smproperty);

  // If selected_proxy_panel_visibility="advanced" hint is specified, we
  // only show the widgets for the selected proxy in advanced mode.
  vtkPVXMLElement* hints = smproperty->GetHints()
    ? smproperty->GetHints()->FindNestedElementByName("ProxyPropertyWidget")
    : nullptr;
  this->Internal->HideProxyWidgetsInDefaultView = (hints &&
    strcmp(hints->GetAttributeOrDefault("selected_proxy_panel_visibility", ""), "advanced") == 0);
  this->Internal->HideProxyWidgets = (hints &&
    strcmp(hints->GetAttributeOrDefault("selected_proxy_panel_visibility", ""), "never") == 0);

  // If "ProxySelectionWidget" hint is provided, it can control the enabled
  // state for the combo-box.
  vtkPVXMLElement* hints2 = smproperty->GetHints()
    ? smproperty->GetHints()->FindNestedElementByName("ProxySelectionWidget")
    : nullptr;
  if (hints2)
  {
    int enabled = 1;
    if (hints2->GetScalarAttribute("enabled", &enabled))
    {
      this->Internal->Ui.comboBox->setEnabled(enabled != 0);
    }
    int visibility = 1;
    if (hints2->GetScalarAttribute("visibility", &visibility))
    {
      this->Internal->Ui.comboBox->setVisible(visibility != 0);
      this->Internal->Ui.label->setVisible(visibility != 0);
    }
  }
}

//-----------------------------------------------------------------------------
pqProxySelectionWidget::~pqProxySelectionWidget() = default;

//-----------------------------------------------------------------------------
vtkSMProxy* pqProxySelectionWidget::chosenProxy() const
{
  return this->Internal->ChosenProxy;
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::setChosenProxy(vtkSMProxy* cproxy)
{
  if (this->Internal->ChosenProxy != cproxy)
  {
    this->Internal->setChosenProxy(cproxy, this);

    // Update the QComboBox.
    for (unsigned int cc = 0; cc < this->Internal->Domain->GetNumberOfProxies(); ++cc)
    {
      if (this->Internal->Domain->GetProxy(cc) == cproxy)
      {
        this->Internal->Ui.comboBox->setCurrentIndex(cc);
      }
    }
    Q_EMIT this->chosenProxyChanged();
  }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::currentIndexChanged(int idx)
{
  this->setChosenProxy(this->Internal->Domain->GetProxy(idx));
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::apply()
{
  this->Superclass::apply();
  if (auto nestedWidget = this->Internal->ProxyWidget)
  {
    // we need to block tracing since the "nested proxy" will indeed get traced
    // by the parent proxy. See #18127.
    SM_SCOPED_TRACE(BlockTraceItems);
    nestedWidget->apply();
  }
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::reset()
{
  if (this->Internal->ProxyWidget)
  {
    this->Internal->ProxyWidget->reset();
  }
  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::select()
{
  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::deselect()
{
  this->Superclass::deselect();
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::updateWidget(bool showing_advanced_properties)
{
  this->Internal->updateWidget(showing_advanced_properties);
  this->Superclass::updateWidget(showing_advanced_properties);
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::setPanelVisibility(const char* vis)
{
  this->Superclass::setPanelVisibility(vis);
}

//-----------------------------------------------------------------------------
void pqProxySelectionWidget::setView(pqView* aview)
{
  if (this->Internal->ProxyWidget)
  {
    this->Internal->ProxyWidget->setView(aview);
  }
  this->Superclass::setView(aview);
}
