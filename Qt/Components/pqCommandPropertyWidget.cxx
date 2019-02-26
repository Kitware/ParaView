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
#include "pqCommandPropertyWidget.h"

#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqCommandPropertyWidget::pqCommandPropertyWidget(
  vtkSMProperty* smproperty, vtkSMProxy* smproxy, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  if (!smproperty)
  {
    qCritical() << "pqCommandPropertyWidget cannot be instantiated without a vtkSMProperty";
    return;
  }

  QPushButton* button = new QPushButton(smproperty->GetXMLLabel(), this);
  button->setObjectName("PushButton");
  QObject::connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));

  QHBoxLayout* layoutLocal = new QHBoxLayout(this);
  layoutLocal->setMargin(0);
  layoutLocal->addWidget(button);
  layoutLocal->addStretch();
  this->setShowLabel(false);
}

//-----------------------------------------------------------------------------
pqCommandPropertyWidget::~pqCommandPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqCommandPropertyWidget::buttonClicked()
{
  vtkSMProxy* smproxy = this->proxy();
  vtkSMProperty* smproperty = this->property();
  if (smproperty != NULL && smproxy != NULL)
  {
    const char* pname = smproxy->GetPropertyName(smproperty);
    if (pname)
    {
      smproxy->InvokeCommand(pname);

      // Update pipeline information, if possible, otherwise, simple update
      // information properties.
      vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(smproxy);
      if (source)
      {
        source->UpdatePipelineInformation();
      }
      else
      {
        smproxy->UpdatePropertyInformation();
      }
    }
  }
}
