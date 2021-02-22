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
#include "pqCTHArraySelectionDecorator.h"

#include "pqPropertyWidget.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqCTHArraySelectionDecorator::pqCTHArraySelectionDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  QObject::connect(parentObject, SIGNAL(changeFinished()), this, SLOT(updateSelection()));

  // Scan the config to determine the names of the other selection properties.
  for (unsigned int cc = 0; cc < config->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* child = config->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Property") == 0 &&
      child->GetAttribute("name"))
    {
      this->PropertyNames << child->GetAttribute("name");
    }
  }
}

//-----------------------------------------------------------------------------
pqCTHArraySelectionDecorator::~pqCTHArraySelectionDecorator() = default;

//-----------------------------------------------------------------------------
void pqCTHArraySelectionDecorator::updateSelection()
{
  vtkSMProperty* curProperty = this->parentWidget()->property();
  vtkSMProxy* proxy = this->parentWidget()->proxy();
  if (vtkSMUncheckedPropertyHelper(curProperty).GetNumberOfElements() == 0)
  {
    return;
  }

  foreach (const QString& pname, this->PropertyNames)
  {
    vtkSMProperty* prop = proxy->GetProperty(pname.toLocal8Bit().data());
    if (prop && prop != curProperty)
    {
      vtkSMUncheckedPropertyHelper(prop).SetNumberOfElements(0);
    }
  }
}
