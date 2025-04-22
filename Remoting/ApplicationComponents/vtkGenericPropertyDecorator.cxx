// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenericPropertyDecorator.h"

#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkWeakPointer.h"

#include "vtkSMProxy.h"

#include <cassert>
#include <sstream>

class vtkGenericPropertyDecorator::vtkInternals
{
public:
  vtkWeakPointer<vtkSMProperty> Property;
  int Index;
  std::vector<std::string> Values;
  bool Inverse;
  bool Enabled;
  bool Visible;
  int NumberOfComponents;
  int ObserverId;
  vtkGenericPropertyDecorator* Self;

  enum
  {
    VISIBILITY,
    ENABLED_STATE
  } Mode;

  vtkInternals(vtkGenericPropertyDecorator* self)
    : Index(0)
    , Inverse(false)
    , Enabled(true)
    , Visible(true)
    , NumberOfComponents(-1)
    , Mode(ENABLED_STATE)
    , ObserverId(0)
    , Self(self)
  {
  }

  ~vtkInternals()
  {
    if (this->Property && this->ObserverId)
    {
      this->Property->RemoveObserver(this->ObserverId);
      this->ObserverId = 0;
    }
  }

  bool ValueMatch()
  {
    vtkSMUncheckedPropertyHelper helper(this->Property);
    if (helper.GetNumberOfElements() == 0)
    {
      // if there is no proxy, 'its value' does not match this->Value.
      bool status = this->Values.size() == 1 && this->Values[0] == "null";
      return this->Inverse ? !status : status;
    }

    if (vtkSMProxyProperty::SafeDownCast(this->Property))
    {
      if (this->Property->FindDomain<vtkSMProxyListDomain>())
      {
        bool status = false;
        for (auto it = this->Values.begin(); helper.GetAsProxy(0) && it != this->Values.end(); ++it)
        {
          status |= (helper.GetAsProxy(0)->GetXMLName() == *it);
        }
        return this->Inverse ? !status : status;
      }
      if (this->Values.size() == 1 && this->Values[0] == "null")
      {
        if (helper.GetNumberOfElements() == 1)
        {
          return (helper.GetAsProxy(0) != nullptr ? this->Inverse : !this->Inverse);
        }
        else if (helper.GetNumberOfElements() == 0)
        {
          return this->Inverse;
        }
      }
      vtkWarningWithObjectMacro(
        Self, << "ProxyProperty not properly specified in XML. "
              << "pqGenericPropertyWidgetDecorator may not work as expected.");
      return false;
    }

    // The "number_of_components" attribute is used to enable/disable a widget based on
    // whether the referenced property value refers to an array in the input that has
    // the specified number of components.
    if (this->NumberOfComponents > -1)
    {
      if (!this->Property->IsA("vtkSMStringVectorProperty") || helper.GetNumberOfElements() != 5)
      {
        vtkWarningWithObjectMacro(Self,
          << "The NumberOfComponents attribute is applicable only to a vtkSMStringVectorProperty "
          << "with 5 elements. This property is a " << this->Property->GetClassName() << " with "
          << helper.GetNumberOfElements() << " elements.");
        return false;
      }

      // Look for array list domain
      auto ald = this->Property->FindDomain<vtkSMArrayListDomain>();
      if (!ald)
      {
        vtkWarningWithObjectMacro(Self,
          << "The NumberOfComponents attribute requires an ArrayListDomain in the "
          << "StringVectorProperty '" << this->Property->GetXMLName() << "', but none was found.");
        return false;
      }

      // Field association is always one element before the array name element.
      int arrayAssociation = helper.GetAsInt(this->Index - 1);
      const char* arrayName = helper.GetAsString(this->Index);
      vtkPVDataInformation* dataInfo = ald->GetInputDataInformation("Input");
      if (!dataInfo)
      {
        vtkWarningWithObjectMacro(Self, << "Could not get data information for Input property");
        return false;
      }

      // Array components could be 0 if arrayName is the NoneString
      int arrayComponents = 0;
      vtkPVArrayInformation* arrayInfo = dataInfo->GetArrayInformation(arrayName, arrayAssociation);
      if (arrayInfo)
      {
        arrayComponents = arrayInfo->GetNumberOfComponents();
      }

      bool status = arrayComponents == this->NumberOfComponents;
      return this->Inverse ? !status : status;
    }

    vtkVariant val = helper.GetAsVariant(this->Index);
    bool status = false;
    for (auto it = this->Values.begin(); it != this->Values.end(); ++it)
    {
      status |= (val.ToString() == *it);
    }

    return this->Inverse ? !status : status;
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGenericPropertyDecorator);

//-----------------------------------------------------------------------------
void vtkGenericPropertyDecorator::Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(config, proxy);
  this->Internals =
    std::unique_ptr<vtkInternals>(new vtkGenericPropertyDecorator::vtkInternals(this));
  assert(proxy != nullptr);

  const char* propertyName = config->GetAttribute("property");
  if (propertyName == nullptr || proxy->GetProperty(propertyName) == nullptr)
  {
    // this can happen with compound proxies. In which case, silently ignore.
    // vtkErrorMacro(<< "Invalid property='" << (propertyName ? propertyName : "(null)")
    //              << "' specified in the configuration.");
    return;
  }

  this->Internals->Property = proxy->GetProperty(propertyName);

  if (config->GetAttribute("index") != nullptr)
  {
    config->GetScalarAttribute("index", &this->Internals->Index);
  }

  const char* value = config->GetAttribute("value");
  if (value == nullptr)
  {
    // see if there are multiple values instead.
    value = config->GetAttribute("values");
    if (value != nullptr)
    {
      std::stringstream ss;
      ss.str(value);
      std::string item;
      auto vec_inserter = std::back_inserter(this->Internals->Values);
      while (std::getline(ss, item, ' '))
      {
        *(vec_inserter++) = item;
      }
    }
  }
  else
  {
    this->Internals->Values.push_back(value);
  }

  const char* mode = config->GetAttribute("mode");
  if (mode && strcmp(mode, "visibility") == 0)
  {
    this->Internals->Mode = vtkInternals::VISIBILITY;
  }
  else if (mode && strcmp(mode, "enabled_state") == 0)
  {
    this->Internals->Mode = vtkInternals::ENABLED_STATE;
  }
  else
  {
    vtkWarningWithObjectMacro(this, << "Invalid mode: " << (mode ? mode : "(null)"));
    return;
  }

  if (config->GetAttribute("inverse") && strcmp(config->GetAttribute("inverse"), "1") == 0)
  {
    this->Internals->Inverse = true;
  }

  if (config->GetAttribute("number_of_components"))
  {
    this->Internals->NumberOfComponents = atoi(config->GetAttribute("number_of_components"));
  }

  this->Internals->ObserverId = this->Internals->Property->AddObserver(
    vtkCommand::UncheckedPropertyModifiedEvent, this, &vtkGenericPropertyDecorator::UpdateState);
  this->UpdateState();
}

//-----------------------------------------------------------------------------
vtkGenericPropertyDecorator::vtkGenericPropertyDecorator() = default;

//-----------------------------------------------------------------------------
vtkGenericPropertyDecorator::~vtkGenericPropertyDecorator() = default;

//-----------------------------------------------------------------------------
void vtkGenericPropertyDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Internals :" << this->Internals.get() << "\n";
  os << indent << indent << "Index :" << this->Internals->Index << "\n";
  os << indent << indent << "Enabled :" << this->Internals->Enabled << "\n";
  os << indent << indent << "Visible :" << this->Internals->Visible << "\n";
  os << indent << indent << "NumberOfComponents :" << this->Internals->NumberOfComponents << "\n";
  os << indent << indent << "ObserverId :" << this->Internals->ObserverId << "\n";
}

//-----------------------------------------------------------------------------
void vtkGenericPropertyDecorator::UpdateState()
{
  if (this->Internals->Property == nullptr || this->Proxy() == nullptr)
  {
    return;
  }

  bool valueMatch = this->Internals->ValueMatch();

  switch (this->Internals->Mode)
  {
    case vtkInternals::ENABLED_STATE:
      this->Internals->Enabled = valueMatch;
      this->InvokeEvent(EnableStateChangedEvent);
      break;

    case vtkInternals::VISIBILITY:
      this->Internals->Visible = valueMatch;
      this->InvokeEvent(VisibilityChangedEvent);
      break;
  }
}

//-----------------------------------------------------------------------------
bool vtkGenericPropertyDecorator::CanShow(bool show_advanced) const
{
  (void)(show_advanced);
  return this->Internals->Visible;
}

//-----------------------------------------------------------------------------
bool vtkGenericPropertyDecorator::Enable() const
{
  return this->Internals->Enabled;
}
