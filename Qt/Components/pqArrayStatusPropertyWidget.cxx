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

#include "pqArraySelectionWidget.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

#include <QHBoxLayout>

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  auto selectorWidget = new pqArraySelectionWidget(this);
  selectorWidget->setObjectName("SelectionWidget");
  selectorWidget->setHeaderLabel(smgroup->GetXMLLabel());
  selectorWidget->setMaximumRowCountBeforeScrolling(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smgroup->GetHints()));

  // add context menu and custom indicator for sorting and filtering.
  new pqTreeViewSelectionHelper(selectorWidget);

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  for (unsigned int cc = 0; cc < smgroup->GetNumberOfProperties(); cc++)
  {
    vtkSMProperty* prop = smgroup->GetProperty(cc);
    if (prop && prop->GetInformationOnly() == 0)
    {
      const char* property_name = smproxy->GetPropertyName(prop);
      this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), prop);
    }
  }

  // don't show label
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::pqArrayStatusPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  auto selectorWidget = new pqArraySelectionWidget(this);
  selectorWidget->setObjectName("SelectionWidget");
  selectorWidget->setHeaderLabel(smproperty->GetXMLLabel());
  selectorWidget->setMaximumRowCountBeforeScrolling(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(smproperty->GetHints()));

  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->addWidget(selectorWidget);
  hbox->setMargin(0);
  hbox->setSpacing(4);

  const char* property_name = smproxy->GetPropertyName(smproperty);
  this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), smproperty);

  // don't show label
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqArrayStatusPropertyWidget::~pqArrayStatusPropertyWidget()
{
}
