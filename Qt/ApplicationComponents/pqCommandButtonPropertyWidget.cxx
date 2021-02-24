/*=========================================================================

   Program: ParaView
   Module: pqCommandButtonPropertyWidget.cxx

   Copyright (c) 2005-2012 Kitware Inc.
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

=========================================================================*/

#include "pqCommandButtonPropertyWidget.h"

#include "pqPVApplicationCore.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QPushButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqCommandButtonPropertyWidget::pqCommandButtonPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* proxyProperty, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
  , Property(proxyProperty)
{
  QVBoxLayout* l = new QVBoxLayout;
  l->setSpacing(0);
  l->setMargin(0);

  QPushButton* button = new QPushButton(proxyProperty->GetXMLLabel());
  connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));
  l->addWidget(button);

  this->setShowLabel(false);
  this->setLayout(l);
}

//-----------------------------------------------------------------------------
pqCommandButtonPropertyWidget::~pqCommandButtonPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqCommandButtonPropertyWidget::buttonClicked()
{
  auto aproxy = this->proxy();
  if (this->Property->GetCommand() != nullptr)
  {
    const char* pname = aproxy->GetPropertyName(this->Property);
    SM_SCOPED_TRACE(CallMethod).arg("proxy", aproxy).arg("InvokeCommand").arg(pname);
    this->proxy()->InvokeCommand(pname);
  }
  else
  {
    // if property has no command, then we simply recreate the proxies.
    SM_SCOPED_TRACE(CallMethod).arg("proxy", aproxy).arg("RecreateVTKObjects");
    this->proxy()->RecreateVTKObjects();
  }

  // Trigger a rendering
  pqPVApplicationCore::instance()->render();
}
