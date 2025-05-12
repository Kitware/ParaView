// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBoolPropertyDecorator.h"

#include "vtkCommand.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <cassert>

vtkStandardNewMacro(vtkBoolPropertyDecorator);

//-----------------------------------------------------------------------------
vtkBoolPropertyDecorator::vtkBoolPropertyDecorator() = default;

//-----------------------------------------------------------------------------
void vtkBoolPropertyDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Index : " << this->Index << "\n";
  os << indent << "BoolProperty : " << this->BoolProperty << "\n";
  os << indent << "Property : " << this->Property << "\n";
  os << indent << "ObserverId : " << this->ObserverId << "\n";
  os << indent << "Function : " << this->Function << "\n";
  os << indent << "Value : " << this->Value << "\n";
}

//-----------------------------------------------------------------------------
vtkBoolPropertyDecorator::~vtkBoolPropertyDecorator()
{
  if (this->Property && this->ObserverId)
  {
    this->Property->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
  }
}

//-----------------------------------------------------------------------------
void vtkBoolPropertyDecorator::Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(config, proxy);
  this->Index = 0;
  this->ObserverId = 0;
  this->BoolProperty = true;

  assert(proxy != nullptr);

  for (unsigned int cc = 0; cc < config->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = config->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name"))
    {
      const char* name = child->GetAttribute("name");
      const char* function = child->GetAttributeOrDefault("function", "boolean");
      const char* value = child->GetAttributeOrDefault("value", "");

      int index = atoi(child->GetAttributeOrDefault("index", "0"));
      if (strcmp(function, "boolean") != 0 && strcmp(function, "boolean_invert") != 0 &&
        strcmp(function, "greaterthan") != 0 && strcmp(function, "lessthan") != 0 &&
        strcmp(function, "equals") != 0 && strcmp(function, "contains") != 0)
      {
        vtkDebugMacro("vtkBoolPropertyDecorator currently only "
                      "supports 'boolean', 'boolean_invert', 'greaterthan', "
                      "'lessthan', 'equals' and 'contains'.");
      }
      this->Property = this->Proxy()->GetProperty(name);
      this->Index = index;
      this->Function = function;
      this->Value = value;

      if (!this->Property)
      {
        vtkDebugMacro(<< "Property '" << (name ? name : "(null)")
                      << "' specified in vtkBoolPropertyDecorator not found in proxy '"
                      << this->Proxy()->GetXMLName() << "'.");
        break;
      }

      this->ObserverId = this->Property->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent,
        this, &vtkBoolPropertyDecorator::UpdateBoolPropertyState);
      break;
    }
  }

  this->UpdateBoolPropertyState();
}

//-----------------------------------------------------------------------------
void vtkBoolPropertyDecorator::UpdateBoolPropertyState()
{
  if (this->Property && this->Function == "boolean")
  {
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) != 0;
    this->SetBoolProperty(enabled);
  }
  if (this->Property && this->Function == "boolean_invert")
  {
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) != 0;
    this->SetBoolProperty(!enabled);
  }
  if (this->Property && this->Function == "greaterthan")
  {
    double number = std::stod(this->Value);
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsDouble(this->Index) > number;
    this->SetBoolProperty(enabled);
  }
  if (this->Property && this->Function == "lessthan")
  {
    double number = std::stod(this->Value);
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsDouble(this->Index) < number;
    this->SetBoolProperty(enabled);
  }
  if (this->Property && this->Function == "equals")
  {
    bool enabled =
      this->Value == vtkSMUncheckedPropertyHelper(this->Property).GetAsString(this->Index);
    this->SetBoolProperty(enabled);
  }
  if (this->Property && this->Function == "contains")
  {
    std::string tmp = vtkSMUncheckedPropertyHelper(this->Property).GetAsString(this->Index);

    bool enabled = tmp.find(this->Value) != std::string::npos;
    this->SetBoolProperty(enabled);
  }
}

//-----------------------------------------------------------------------------
void vtkBoolPropertyDecorator::SetBoolProperty(bool val)
{
  if (this->BoolProperty != val)
  {
    this->BoolProperty = val;
    this->InvokeEvent(BoolPropertyChangedEvent);
  }
}
