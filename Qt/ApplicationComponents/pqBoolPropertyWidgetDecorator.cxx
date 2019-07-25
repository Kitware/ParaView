/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
  assert(proxy != NULL);

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
        strcmp(function, "greaterthan") != 0 && strcmp(function, "lessthan") != 0)
      {
        qDebug("pqBoolPropertyWidgetDecorator currently only "
               "supports 'boolean', 'boolean_invert', 'greaterthan', "
               "and 'lessthan'.");
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
    int number = this->Value.toInt();
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) > number;
    this->setBoolProperty(enabled);
  }
  if (this->Property && this->Function == "lessthan")
  {
    int number = this->Value.toInt();
    bool enabled = vtkSMUncheckedPropertyHelper(this->Property).GetAsInt(this->Index) < number;
    this->setBoolProperty(enabled);
  }
}

//-----------------------------------------------------------------------------
void pqBoolPropertyWidgetDecorator::setBoolProperty(bool val)
{
  if (this->BoolProperty != val)
  {
    this->BoolProperty = val;
    emit this->boolPropertyChanged();
  }
}
