/*=========================================================================

   Program: ParaView
   Module:  pqGenericPropertyWidgetDecorator.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqGenericPropertyWidgetDecorator.h"

#include "pqCoreUtilities.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkWeakPointer.h"

#include <QtDebug>

#include <cassert>
#include <sstream>

class pqGenericPropertyWidgetDecorator::pqInternals
{
public:
  vtkWeakPointer<vtkSMProperty> Property;
  int Index;
  std::vector<std::string> Values;
  bool Inverse;
  bool Enabled;
  bool Visible;
  int NumberOfComponents;

  enum
  {
    VISIBILITY,
    ENABLED_STATE
  } Mode;

  pqInternals()
    : Index(0)
    , Inverse(false)
    , Enabled(true)
    , Visible(true)
    , NumberOfComponents(-1)
    , Mode(ENABLED_STATE)
  {
  }

  bool valueMatch()
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
      qCritical() << "ProxyProperty not properly specified in XML. "
                  << "pqGenericPropertyWidgetDecorator may not work as expected.";
      return false;
    }

    // The "number_of_components" attribute is used to enable/disable a widget based on
    // whether the referenced property value refers to an array in the input that has
    // the specified number of components.
    if (this->NumberOfComponents > -1)
    {
      if (!this->Property->IsA("vtkSMStringVectorProperty") || helper.GetNumberOfElements() != 5)
      {
        qCritical()
          << "The NumberOfComponents attribute is applicable only to a vtkSMStringVectorProperty "
          << "with 5 elements. This property is a " << this->Property->GetClassName() << " with "
          << helper.GetNumberOfElements() << " elements.";
        return false;
      }

      // Look for array list domain
      auto ald = this->Property->FindDomain<vtkSMArrayListDomain>();
      if (!ald)
      {
        qCritical() << "The NumberOfComponents attribute requires an ArrayListDomain in the "
                    << "StringVectorProperty '" << this->Property->GetXMLName()
                    << "', but none was found.";
        return false;
      }

      // Field association is always one element before the array name element.
      int arrayAssociation = helper.GetAsInt(this->Index - 1);
      const char* arrayName = helper.GetAsString(this->Index);
      vtkPVDataInformation* dataInfo = ald->GetInputDataInformation("Input");
      if (!dataInfo)
      {
        qCritical() << "Could not get data information for Input property";
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
pqGenericPropertyWidgetDecorator::pqGenericPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
  , Internals(new pqGenericPropertyWidgetDecorator::pqInternals())
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  assert(proxy != nullptr);

  const char* propertyName = config->GetAttribute("property");
  if (propertyName == nullptr || proxy->GetProperty(propertyName) == nullptr)
  {
    // this can happen with compound proxies. In which case, silently ignore.
    // qCritical() << "Invalid property='" << (propertyName? propertyName : "(null)")
    //  << "' specified in the configuration.";
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
    this->Internals->Mode = pqInternals::VISIBILITY;
  }
  else if (mode && strcmp(mode, "enabled_state") == 0)
  {
    this->Internals->Mode = pqInternals::ENABLED_STATE;
  }
  else
  {
    qCritical() << "Invalid mode: " << (mode ? mode : "(null)");
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

  pqCoreUtilities::connect(this->Internals->Property, vtkCommand::UncheckedPropertyModifiedEvent,
    this, SLOT(updateState()));
  this->updateState();
}

//-----------------------------------------------------------------------------
pqGenericPropertyWidgetDecorator::~pqGenericPropertyWidgetDecorator()
{
}

//-----------------------------------------------------------------------------
void pqGenericPropertyWidgetDecorator::updateState()
{
  if (this->Internals->Property == nullptr || this->parentWidget() == nullptr)
  {
    return;
  }

  bool valueMatch = this->Internals->valueMatch();

  switch (this->Internals->Mode)
  {
    case pqInternals::ENABLED_STATE:
      this->Internals->Enabled = valueMatch;
      Q_EMIT this->enableStateChanged();
      break;

    case pqInternals::VISIBILITY:
      this->Internals->Visible = valueMatch;
      Q_EMIT this->visibilityChanged();
      break;
  }
}

//-----------------------------------------------------------------------------
bool pqGenericPropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  Q_UNUSED(show_advanced);
  return this->Internals->Visible;
}

//-----------------------------------------------------------------------------
bool pqGenericPropertyWidgetDecorator::enableWidget() const
{
  return this->Internals->Enabled;
}
