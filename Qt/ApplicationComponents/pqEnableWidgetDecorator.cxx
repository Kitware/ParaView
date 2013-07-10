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
#include "pqEnableWidgetDecorator.h"

#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqEnableWidgetDecorator::pqEnableWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject),
  Index(0),
  ObserverId(0),
  Enabled(true)
{
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  Q_ASSERT(proxy != NULL);

  for (unsigned int cc=0; cc < config->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = config->GetNestedElement(cc);
    if (child && child->GetName() &&
      strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name"))
      {
      const char* name = child->GetAttribute("name");
      const char* function = child->GetAttributeOrDefault("function",
        "boolean");
      int index = atoi(child->GetAttributeOrDefault("index", "0"));
      if (strcmp(function, "boolean") != 0 &&
          strcmp(function, "boolean_invert") != 0)
        {
        qDebug("pqEnableWidgetDecorator currently only supports 'boolean', "
          "'boolean_invert'.");
        }

      this->Property = proxy->GetProperty(name);
      this->Index = index;
      this->Function = function;

      this->ObserverId = this->Property->AddObserver(
        vtkCommand::UncheckedPropertyModifiedEvent,
        this, &pqEnableWidgetDecorator::updateEnabledState);
      break;
      }
    }

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqEnableWidgetDecorator::~pqEnableWidgetDecorator()
{
  if (this->Property && this->ObserverId)
    {
    this->Property->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
    }
}

//-----------------------------------------------------------------------------
void pqEnableWidgetDecorator::updateEnabledState()
{
  if (this->Property && this->Function == "boolean")
    {
    bool enabled = vtkSMUncheckedPropertyHelper(
      this->Property).GetAsInt(this->Index) != 0;
    this->setEnableWidget(enabled);
    }
  if (this->Property && this->Function == "boolean_invert")
    {
    bool enabled = vtkSMUncheckedPropertyHelper(
      this->Property).GetAsInt(this->Index) != 0;
    this->setEnableWidget(!enabled);
    }
}

//-----------------------------------------------------------------------------
void pqEnableWidgetDecorator::setEnableWidget(bool val)
{
  if (this->Enabled != val)
    {
    this->Enabled = val;
    emit this->enableStateChanged();
    }
}
