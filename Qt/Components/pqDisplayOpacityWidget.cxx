// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDisplayOpacityWidget.h"

#include "pqArraySelectorPropertyWidget.h"
#include "pqDataRepresentation.h"
#include "pqIntVectorPropertyWidget.h"
#include "pqPropertiesPanel.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPointer>

class pqDisplayOpacityWidget::pqInternals
{
public:
  QPointer<pqDataRepresentation> Representation;
  QPointer<QWidget> Variables;
  QPointer<QWidget> Components;

  // Locks in the opacity array name on the representation.
  void FinalizeOpacityArrayName(vtkSMRepresentationProxy* reprProxy)
  {
    vtkSMPropertyHelper helper(reprProxy, "OpacityArrayName");
    helper.SetUseUnchecked(true);
    const char* name = helper.GetInputArrayNameToProcess();
    const int assoc = helper.GetInputArrayAssociation();
    helper.SetUseUnchecked(false);
    helper.SetInputArrayToProcess(assoc, name);
  }
  // Locks in the opacity array component on the representation.
  void FinalizeOpacityComponent(vtkSMRepresentationProxy* reprProxy)
  {
    vtkSMPropertyHelper helper(reprProxy, "OpacityComponent");
    helper.SetUseUnchecked(true);
    const int component = helper.GetAsInt();
    helper.SetUseUnchecked(false);
    helper.Set(component);
  }
};

pqDisplayOpacityWidget::pqDisplayOpacityWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(std::unique_ptr<pqInternals>(new pqInternals()))
{
  auto hbox = new QHBoxLayout();
  hbox->setContentsMargins(0, 0, 0, 0);
  hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  this->setLayout(hbox);
  // default, show empty comboboxes. populated when a valid representation is available.
  this->Internals->Variables = new QComboBox(this);
  this->Internals->Variables->setMinimumSize(QSize(150, 0));
  this->Internals->Components = new QComboBox(this);
  hbox->addWidget(this->Internals->Variables);
  hbox->addWidget(this->Internals->Components);
}

pqDisplayOpacityWidget::~pqDisplayOpacityWidget() = default;

void pqDisplayOpacityWidget::setRepresentation(pqDataRepresentation* display)
{
  if (this->Internals->Representation != nullptr)
  {
    this->Internals->Representation = nullptr;
  }
  // gets rid of stale widgets from previous representation (if any)
  if (this->Internals->Variables != nullptr)
  {
    this->layout()->removeWidget(this->Internals->Variables);
    delete this->Internals->Variables;
    this->Internals->Variables = nullptr;
  }
  if (this->Internals->Components != nullptr)
  {
    this->layout()->removeWidget(this->Internals->Components);
    delete this->Internals->Components;
    this->Internals->Components = nullptr;
  }

  vtkSMProxy* reprProxy = nullptr;
  vtkSMProperty* varProp = nullptr;
  // valid representation? create array selector widget and an accompanying
  // int vector property widget for the array components.
  if (display && (reprProxy = display->getProxy()) &&
    (varProp = reprProxy->GetProperty("OpacityArrayName")))
  {
    this->Internals->Variables = new pqArraySelectorPropertyWidget(varProp, reprProxy, this);
    this->Internals->Variables->setObjectName("Variables");
    this->Internals->Variables->setMinimumSize(QSize(150, 0));
    vtkSMProperty* componentProp = reprProxy->GetProperty("OpacityComponent");
    this->Internals->Components = new pqIntVectorPropertyWidget(componentProp, reprProxy, this);
    this->Internals->Components->setObjectName("Components");

    this->Internals->Representation = display;

    QObject::connect(display, &pqDataRepresentation::useSeparateOpacityArrayModified, this,
      &pqDisplayOpacityWidget::onArrayModified);
    QObject::connect(qobject_cast<pqArraySelectorPropertyWidget*>(this->Internals->Variables),
      &pqArraySelectorPropertyWidget::arrayChanged, this, &pqDisplayOpacityWidget::onArrayModified);
    QObject::connect(qobject_cast<pqIntVectorPropertyWidget*>(this->Internals->Components),
      &pqIntVectorPropertyWidget::changeAvailable, this, &pqDisplayOpacityWidget::onArrayModified);
  }
  else
  {
    // dummy combo boxes.
    this->Internals->Variables = new QComboBox(this);
    this->Internals->Variables->setMinimumSize(QSize(150, 0));
    this->Internals->Components = new QComboBox(this);
  }
  this->layout()->addWidget(this->Internals->Variables);
  this->layout()->addWidget(this->Internals->Components);
}

void pqDisplayOpacityWidget::onArrayModified()
{
  if (this->Internals->Representation == nullptr)
  {
    return;
  }
  vtkSMRepresentationProxy* reprProxy =
    vtkSMRepresentationProxy::SafeDownCast(this->Internals->Representation->getProxy());
  if (reprProxy == nullptr)
  {
    return;
  }
  // need to lock in the array name and component before VTK objects are updated.
  this->Internals->FinalizeOpacityArrayName(reprProxy);
  this->Internals->FinalizeOpacityComponent(reprProxy);
  reprProxy->UpdateVTKObjects();
  this->Internals->Representation->renderViewEventually();
}
