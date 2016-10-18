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
#include "vtkPVXMLElement.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkWeakPointer.h"

#include <QtDebug>

class pqGenericPropertyWidgetDecorator::pqInternals
{
public:
  vtkWeakPointer<vtkSMProperty> Property;
  std::string Value;
  bool Inverse;
  bool Enabled;
  bool Visible;

  enum
  {
    VISIBILITY,
    ENABLED_STATE
  } Mode;

  pqInternals()
    : Inverse(false)
    , Enabled(true)
    , Visible(true)
    , Mode(ENABLED_STATE)
  {
  }

  bool valueMatch()
  {
    vtkSMUncheckedPropertyHelper helper(this->Property);
    if (helper.GetNumberOfElements() == 0)
    {
      // if there is no proxy, 'its value' does not match this->Value.
      bool status = false;
      return this->Inverse ? !status : status;
    }
    if (helper.GetNumberOfElements() != 1)
    {
      qCritical() << "pqGenericPropertyWidgetDecorator may not work as expected.";
      // currently, we only support 1 element properties.
      return false;
    }

    if (vtkSMProxyProperty::SafeDownCast(this->Property))
    {
      vtkSMProxyListDomain* pld =
        vtkSMProxyListDomain::SafeDownCast(this->Property->FindDomain("vtkSMProxyListDomain"));
      if (!pld)
      {
        qCritical() << "ProxyProperty without vtkSMProxyListDomain is not supported. "
                    << "pqGenericPropertyWidgetDecorator may not work as expected.";
        return false;
      }

      bool status = (helper.GetAsProxy(0) && (helper.GetAsProxy(0)->GetXMLName() == this->Value));
      return this->Inverse ? !status : status;
    }

    vtkVariant val = helper.GetAsVariant(0);
    bool status = (val.ToString() == this->Value);
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
  Q_ASSERT(proxy != NULL);

  const char* propertyName = config->GetAttribute("property");
  if (propertyName == NULL || proxy->GetProperty(propertyName) == NULL)
  {
    // this can happen with compound proxies. In which case, silently ignore.
    // qCritical() << "Invalid property='" << (propertyName? propertyName : "(null)")
    //  << "' specified in the configuration.";
    return;
  }

  this->Internals->Property = proxy->GetProperty(propertyName);
  if (vtkSMUncheckedPropertyHelper(this->Internals->Property).GetNumberOfElements() > 1)
  {
    qCritical() << "pqGenericPropertyWidgetDecorator currently only supports "
                   "single valued properties";
    return;
  }

  const char* value = config->GetAttribute("value");
  if (value == NULL)
  {
    qCritical() << "Missing 'value' in the specified configuration.";
    return;
  }
  this->Internals->Value = value;

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
  if (this->Internals->Property == NULL || this->parentWidget() == NULL)
  {
    return;
  }

  bool valueMatch = this->Internals->valueMatch();

  switch (this->Internals->Mode)
  {
    case pqInternals::ENABLED_STATE:
      this->Internals->Enabled = valueMatch;
      emit this->enableStateChanged();
      break;

    case pqInternals::VISIBILITY:
      this->Internals->Visible = valueMatch;
      emit this->visibilityChanged();
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
