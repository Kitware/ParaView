/*=========================================================================

   Program:   ParaView
   Module:    pqProxySelectionWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqProxySelectionWidget.h"
#include "ui_pqProxySelectionWidget.h"

#include "pqComboBoxDomain.h"
#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QPointer>

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
  {
    this->Ui.setupUi(self);
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.horizontalLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.horizontalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
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
  this->Internal->Ui.label->setText(smproperty->GetXMLLabel());
  this->Internal->Domain =
    vtkSMProxyListDomain::SafeDownCast(smproperty->FindDomain("vtkSMProxyListDomain"));

  // This widget is intended to be used for properties with ProxyListDomains
  // alone.
  Q_ASSERT(this->Internal->Domain);
  this->connect(
    this->Internal->Ui.comboBox, SIGNAL(currentIndexChanged(int)), SLOT(currentIndexChanged(int)));
  new pqComboBoxDomain(this->Internal->Ui.comboBox, smproperty, "proxy_list");
  this->addPropertyLink(this, "chosenProxy", SIGNAL(chosenProxyChanged()), smproperty);

  // If selected_proxy_panel_visibility="advanced" hint is specified, we
  // only show the widgets for the selected proxy in advanced mode.
  vtkPVXMLElement* hints = smproperty->GetHints()
    ? smproperty->GetHints()->FindNestedElementByName("ProxyPropertyWidget")
    : NULL;
  this->Internal->HideProxyWidgetsInDefaultView = (hints &&
    strcmp(hints->GetAttributeOrDefault("selected_proxy_panel_visibility", ""), "advanced") == 0);
  this->Internal->HideProxyWidgets = (hints &&
    strcmp(hints->GetAttributeOrDefault("selected_proxy_panel_visibility", ""), "never") == 0);
}

//-----------------------------------------------------------------------------
pqProxySelectionWidget::~pqProxySelectionWidget()
{
}

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
    emit this->chosenProxyChanged();
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
  if (this->Internal->ProxyWidget)
  {
    this->Internal->ProxyWidget->apply();
  }
  this->Superclass::apply();
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
