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
#include "pqClipScalarsDecorator.h"

#include "pqCoreUtilities.h"
#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
pqClipScalarsDecorator::pqClipScalarsDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject),
  ObserverId(0)
{
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy? proxy->GetProperty("ClipFunction") : NULL;
  if (!prop)
    {
    qDebug("Could not locate property named 'ClipFunction'. "
      "pqClipScalarsDecorator will have no effect.");
    return;
    }

  this->ObservedObject = prop;
  this->ObserverId = pqCoreUtilities::connect(
    prop, vtkCommand::UncheckedPropertyModifiedEvent,
    this, SIGNAL(visibilityChanged()));
}

//-----------------------------------------------------------------------------
pqClipScalarsDecorator::~pqClipScalarsDecorator()
{
  if (this->ObservedObject && this->ObserverId)
    {
    this->ObservedObject->RemoveObserver(this->ObserverId);
    }
}

//-----------------------------------------------------------------------------
bool pqClipScalarsDecorator::canShowWidget(bool show_advanced) const
{
  pqPropertyWidget* parentObject = this->parentWidget();
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy? proxy->GetProperty("ClipFunction") : NULL;
  if (prop)
    {
    int exclude = 0;
    if(!this->xml()->GetScalarAttribute("exclude", &exclude))
      {
      exclude = 0;
      }

    vtkSMProxy* functionProxy = vtkSMUncheckedPropertyHelper(prop).GetAsProxy();
    bool match = functionProxy && functionProxy->GetXMLName() &&
      !strcmp(functionProxy->GetXMLName(),"Scalar");
    if(exclude)
      {
      if(match)
        {
        return false;
        }
      }
    else if(!match)
      {
      return false;
      }
    }

  return this->Superclass::canShowWidget(show_advanced);
}
