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
#include "pqPropertyWidgetDecorator.h"

#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqPropertyWidget.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqPropertyWidgetInterface.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator::pqPropertyWidgetDecorator(
  vtkPVXMLElement* xmlConfig, pqPropertyWidget* parentObject)
  : Superclass(parentObject)
  , XML(xmlConfig)
{
  assert(parentObject);
  parentObject->addDecorator(this);
}

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator::~pqPropertyWidgetDecorator()
{
  if (auto* pwdg = this->parentWidget())
  {
    pwdg->removeDecorator(this);
  }
}

//-----------------------------------------------------------------------------
pqPropertyWidgetDecorator* pqPropertyWidgetDecorator::create(
  vtkPVXMLElement* xmlconfig, pqPropertyWidget* prnt)
{
  if (xmlconfig == nullptr || strcmp(xmlconfig->GetName(), "PropertyWidgetDecorator") != 0 ||
    xmlconfig->GetAttribute("type") == nullptr)
  {
    qWarning("Invalid xml config specified. Cannot create a decorator.");
    return nullptr;
  }

  auto tracker = pqApplicationCore::instance()->interfaceTracker();
  auto interfaces = tracker->interfaces<pqPropertyWidgetInterface*>();

  const QString type = xmlconfig->GetAttribute("type");
  for (auto* anInterface : interfaces)
  {
    if (pqPropertyWidgetDecorator* decorator =
          anInterface->createWidgetDecorator(type, xmlconfig, prnt))
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "created decorator `%s`",
        decorator->metaObject()->className());
      return decorator;
    }
  }

  qWarning() << "Cannot create decorator of type " << type;
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pqPropertyWidgetDecorator::xml() const
{
  return this->XML.GetPointer();
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqPropertyWidgetDecorator::parentWidget() const
{
  return qobject_cast<pqPropertyWidget*>(this->parent());
}
