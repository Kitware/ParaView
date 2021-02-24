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
#include "pqExtrusionPropertyWidgetDecorator.h"

#include "pqCoreUtilities.h"
#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqExtrusionPropertyWidgetDecorator::pqExtrusionPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = parentObject->proxy();
  this->ObservedObject1 = proxy ? proxy->GetProperty("ExtrusionNormalizeData") : nullptr;
  this->ObservedObject2 = proxy ? proxy->GetProperty("ExtrusionAutoScaling") : nullptr;

  if (!this->ObservedObject1)
  {
    qDebug("Could not locate property named 'ExtrusionNormalizeData'. "
           "pqExtrusionPropertyWidgetDecorator will have no effect.");
    return;
  }

  if (!this->ObservedObject2)
  {
    qDebug("Could not locate property named 'ExtrusionAutoScaling'. "
           "pqExtrusionPropertyWidgetDecorator will have no effect.");
    return;
  }

  this->ObserverId1 = pqCoreUtilities::connect(this->ObservedObject1,
    vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(visibilityChanged()));

  this->ObserverId2 = pqCoreUtilities::connect(this->ObservedObject2,
    vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(visibilityChanged()));
}

//-----------------------------------------------------------------------------
pqExtrusionPropertyWidgetDecorator::~pqExtrusionPropertyWidgetDecorator()
{
  if (this->ObservedObject1 && this->ObserverId1)
  {
    this->ObservedObject1->RemoveObserver(this->ObserverId1);
  }
  if (this->ObservedObject2 && this->ObserverId2)
  {
    this->ObservedObject2->RemoveObserver(this->ObserverId2);
  }
}

//-----------------------------------------------------------------------------
bool pqExtrusionPropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  if (this->ObservedObject1 && this->ObservedObject2)
  {
    bool normalizeData = vtkSMUncheckedPropertyHelper(this->ObservedObject1).GetAsInt() == 1;
    bool autoScaling = vtkSMUncheckedPropertyHelper(this->ObservedObject2).GetAsInt() == 1;
    return normalizeData && !autoScaling;
  }

  return this->Superclass::canShowWidget(show_advanced);
}
