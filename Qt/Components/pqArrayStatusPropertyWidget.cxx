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
#include "pqArrayStatusPropertyWidget.h"

#include "pqExodusIIVariableSelectionWidget.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

#include <QHBoxLayout>

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* activeProxy, vtkSMPropertyGroup* group, QWidget* parentObject)
  : Superclass(activeProxy, parentObject)
{
  pqExodusIIVariableSelectionWidget* selectorWidget = new pqExodusIIVariableSelectionWidget(this);
  selectorWidget->setObjectName("SelectionWidget");
  selectorWidget->setRootIsDecorated(false);
  selectorWidget->setHeaderLabel(group->GetXMLLabel());
  selectorWidget->setMaximumRowCountBeforeScrolling(group);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  for (unsigned int cc = 0; cc < group->GetNumberOfProperties(); cc++)
  {
    vtkSMProperty* prop = group->GetProperty(cc);
    if (prop && prop->GetInformationOnly() == 0)
    {
      const char* property_name = activeProxy->GetPropertyName(prop);
      this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), prop);
    }
  }

  // dont show label
  this->setShowLabel(false);
}

pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* activeProxy, vtkSMProperty* proxyProperty, QWidget* parentObject)
  : Superclass(activeProxy, parentObject)
{
  pqExodusIIVariableSelectionWidget* selectorWidget = new pqExodusIIVariableSelectionWidget(this);
  selectorWidget->setObjectName("SelectionWidget");

  selectorWidget->setRootIsDecorated(false);
  selectorWidget->setHeaderLabel(proxyProperty->GetXMLLabel());
  selectorWidget->setMaximumRowCountBeforeScrolling(proxyProperty);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  const char* property_name = activeProxy->GetPropertyName(proxyProperty);
  this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), proxyProperty);

  // dont show label
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::~pqArrayStatusPropertyWidget()
{
}
