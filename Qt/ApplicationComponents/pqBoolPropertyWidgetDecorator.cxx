// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBoolPropertyWidgetDecorator.h"

#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqBoolPropertyWidgetDecorator::pqBoolPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
  , Index(0)
  , ObserverId(0)
  , BoolProperty(true)
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  assert(proxy != nullptr);

  for (unsigned int cc = 0; cc < config->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = config->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name"))
    {
      const char* name = child->GetAttribute("name");
      const char* function = child->GetAttributeOrDefault("function", "boolean");
      const char* value = child->GetAttribute("value");

      int index = atoi(child->GetAttributeOrDefault("index", "0"));
      if (strcmp(function, "boolean") != 0 && strcmp(function, "boolean_invert") != 0 &&
        strcmp(function, "greaterthan") != 0 && strcmp(function, "lessthan") != 0 &&
        strcmp(function, "equals") != 0 && strcmp(function, "contains") != 0)
      {
        qDebug("pqBoolPropertyWidgetDecorator currently only "
               "supports 'boolean', 'boolean_invert', 'greaterthan', "
               "'lessthan', 'equals' and 'contains'.");
      }
      this->Property = proxy->GetProperty(name);
      this->Index = index;
      this->Function = function;
      this->Value = value;

      if (!this->Property)
      {
        qDebug() << "Property '" << (name ? name : "(null)")
                 << "' specified in pqBoolPropertyWidgetDecorator not found in proxy '"
                 << proxy->GetXMLName() << "'.";
        break;
      }

      this->ObserverId = this->Property->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent,
        this, &pqBoolPropertyWidgetDecorator::updateBoolPropertyState);
      break;
    }
  }

  this->updateBoolPropertyState();
}

//-----------------------------------------------------------------------------
pqBoolPropertyWidgetDecorator::~pqBoolPropertyWidgetDecorator()
{
  if (this->Property && this->ObserverId)
  {
    this->Property->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
  }
}

//-----------------------------------------------------------------------------
void pqBoolPropertyWidgetDecorator::updateBoolPropertyState()
{
  if (this->Property && this->Function == "boolean")
  {
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) != 0;
    this->setBoolProperty(enabled);
  }
  if (this->Property && this->Function == "boolean_invert")
  {
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) != 0;
    this->setBoolProperty(!enabled);
  }
  if (this->Property && this->Function == "greaterthan")
  {
    double number = this->Value.toDouble();
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsDouble(this->Index) > number;
    this->setBoolProperty(enabled);
  }
  if (this->Property && this->Function == "lessthan")
  {
    double number = this->Value.toDouble();
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsDouble(this->Index) < number;
    this->setBoolProperty(enabled);
  }
  if (this->Property && this->Function == "equals")
  {
    bool enabled =
      this->Value == vtkSMUncheckedPropertyHelper(this->Property).GetAsString(this->Index);
    this->setBoolProperty(enabled);
  }
  if (this->Property && this->Function == "contains")
  {
    bool enabled = QString(vtkSMUncheckedPropertyHelper(this->Property).GetAsString(this->Index))
                     .contains(this->Value);
    this->setBoolProperty(enabled);
  }
}

//-----------------------------------------------------------------------------
void pqBoolPropertyWidgetDecorator::setBoolProperty(bool val)
{
  if (this->BoolProperty != val)
  {
    this->BoolProperty = val;
    Q_EMIT this->boolPropertyChanged();
  }
}
