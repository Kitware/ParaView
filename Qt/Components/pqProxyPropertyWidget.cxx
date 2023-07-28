// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqProxyPropertyWidget.h"

#include <QVBoxLayout>

#include "pqProxySelectionWidget.h"
#include "pqSelectionInputWidget.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"

pqProxyPropertyWidget::pqProxyPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* parentObject)
  : pqPropertyWidget(smProxy, parentObject)
{
  QVBoxLayout* vbox = new QVBoxLayout;
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(0);

  bool selection_input =
    (smProperty->GetHints() && smProperty->GetHints()->FindNestedElementByName("SelectionInput"));
  if (selection_input)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqSelectionInputWidget`.");
    pqSelectionInputWidget* siw = new pqSelectionInputWidget(this);
    siw->setObjectName(smProxy->GetPropertyName(smProperty));
    vbox->addWidget(siw);
    this->SelectionInputWidget = siw;
    this->addPropertyLink(siw, "selection", SIGNAL(selectionChanged(pqSMProxy)), smProperty);

    this->connect(siw, SIGNAL(selectionChanged(pqSMProxy)), this, SIGNAL(changeAvailable()));
    this->connect(siw, SIGNAL(selectionChanged(pqSMProxy)), this, SIGNAL(changeFinished()));

    // don't show label for the proxy selection widget
    this->setShowLabel(false);
  }
  else if (smProperty->FindDomain<vtkSMProxyListDomain>())
  {
    vtkVLogF(
      PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqProxySelectionWidget` for proxy-list domain.");
    pqProxySelectionWidget* widget = new pqProxySelectionWidget(smProperty, smProxy, this);
    widget->setView(this->view());
    this->connect(widget, SIGNAL(changeAvailable()), SIGNAL(changeAvailable()));
    this->connect(widget, SIGNAL(changeFinished()), SIGNAL(changeFinished()));
    this->connect(widget, SIGNAL(restartRequired()), SIGNAL(restartRequired()));
    widget->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));

    vbox->addWidget(widget);

    // store the proxy selection widget so that we can call
    // its accept() method when our apply() is called
    this->ProxySelectionWidget = widget;

    // don't show label for the proxy selection widget
    this->setShowLabel(false);
  }

  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
void pqProxyPropertyWidget::apply()
{
  if (this->SelectionInputWidget)
  {
    this->SelectionInputWidget->preAccept();
  }
  this->Superclass::apply();

  // apply properties for the proxy selection widget
  if (this->ProxySelectionWidget)
  {
    this->ProxySelectionWidget->apply();
  }

  if (this->SelectionInputWidget)
  {
    this->SelectionInputWidget->postAccept();
  }
}

//-----------------------------------------------------------------------------
void pqProxyPropertyWidget::reset()
{
  if (this->ProxySelectionWidget)
  {
    this->ProxySelectionWidget->reset();
  }
  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqProxyPropertyWidget::select()
{
  if (this->ProxySelectionWidget)
  {
    this->ProxySelectionWidget->select();
  }
}

//-----------------------------------------------------------------------------
void pqProxyPropertyWidget::deselect()
{
  if (this->ProxySelectionWidget)
  {
    this->ProxySelectionWidget->deselect();
  }
}

//-----------------------------------------------------------------------------
void pqProxyPropertyWidget::updateWidget(bool showing_advanced_properties)
{
  if (this->ProxySelectionWidget)
  {
    this->ProxySelectionWidget->updateWidget(showing_advanced_properties);
  }
}

vtkSMProxy* pqProxyPropertyWidget::chosenProxy() const
{
  if (this->ProxySelectionWidget)
  {
    return this->ProxySelectionWidget->chosenProxy();
  }
  return nullptr;
}
