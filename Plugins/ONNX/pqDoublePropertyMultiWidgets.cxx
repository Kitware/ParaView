// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDoublePropertyMultiWidgets.h"

#include "pqDoubleRangeWidget.h"
#include "pqNumericParameter.h"

#include "vtkSMDoubleVectorProperty.h"

#include <QGridLayout>
#include <QLabel>

//-----------------------------------------------------------------------------
pqDoublePropertyMultiWidgets::pqDoublePropertyMultiWidgets(
  QWidget* parent, vtkSMDoubleVectorProperty* valuesProperty)
  : Superclass(valuesProperty->GetParent(), parent)
{
  auto layout = new QGridLayout(this);
  this->setLayout(layout);
  layout->setContentsMargins(0, 0, 0, 0);

  this->setChangeAvailableAsChangeFinished(true);

  this->addPropertyLink(this, "values", SIGNAL(valuesChanged()), valuesProperty);
}

//-----------------------------------------------------------------------------
void pqDoublePropertyMultiWidgets::setValues(const QVariantList& values)
{
  const auto nbOfParameters = this->DynamicWidgets.size();
  if (values.size() < nbOfParameters)
  {
    qWarning() << "Cannot initialize widgets: to few values.";
    return;
  }
  for (int idx = 0; idx < nbOfParameters; idx++)
  {
    pqDoubleRangeWidget* range = this->DynamicWidgets[idx];
    if (range)
    {
      range->setValue(values[idx].toDouble());
    }
  }
}

//-----------------------------------------------------------------------------
QVariantList pqDoublePropertyMultiWidgets::values() const
{
  QVariantList values;
  values.clear();
  values.reserve(this->DynamicWidgets.size());
  for (int idx = 0; idx < this->DynamicWidgets.size(); idx++)
  {
    pqDoubleRangeWidget* range = this->DynamicWidgets[idx];
    if (range)
    {
      values.push_back(range->value());
    }
  }

  return values;
}

//-----------------------------------------------------------------------------
void pqDoublePropertyMultiWidgets::clear()
{
  for (auto paramWidget : this->DynamicWidgets)
  {
    delete paramWidget;
  }
  this->DynamicWidgets.clear();

  for (auto paramLabel : this->DynamicLabels)
  {
    delete paramLabel;
  }
  this->DynamicLabels.clear();
}

//-----------------------------------------------------------------------------
void pqDoublePropertyMultiWidgets::addWidget(pqNumericParameter* param)
{
  auto widget =
    new pqDoubleRangeWidget(this, param->getMin(), param->getMax(), param->getDefault());
  widget->setObjectName(param->getName());
  widget->setToolTip(param->getName());
  this->DynamicWidgets.append(widget);

  QLabel* label = new QLabel(param->getName(), this);
  this->DynamicLabels.append(label);

  QGridLayout* boxLayout = dynamic_cast<QGridLayout*>(this->layout());
  int row = boxLayout->rowCount();
  boxLayout->addWidget(label, row, 0);
  boxLayout->addWidget(widget, row, 1);

  connect(widget, &pqDoubleRangeWidget::valueChanged, this,
    &pqDoublePropertyMultiWidgets::changeAvailable);
  connect(
    widget, &pqDoubleRangeWidget::valueChanged, this, &pqDoublePropertyMultiWidgets::valuesChanged);
}
