/*=========================================================================

   Program: ParaView
   Module: pqCubeAxesPropertyWidget.cxx

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

#include "pqCubeAxesPropertyWidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QPushButton>

#include "vtkSMProxy.h"

#include "pqProxy.h"
#include "pqCubeAxesEditorDialog.h"

pqCubeAxesPropertyWidget::pqCubeAxesPropertyWidget(vtkSMProxy *proxy, QWidget
  *parentObject)
  : pqPropertyWidget(proxy, parentObject)
{
  QHBoxLayout *layout = new QHBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(4);
  QCheckBox *checkBox = new QCheckBox("Visible");
  checkBox->setObjectName("VisibilityCheckBox");
  this->addPropertyLink(checkBox, "checked", SIGNAL(toggled(bool)), proxy->GetProperty("CubeAxesVisibility"));
  layout->addWidget(checkBox);
  QPushButton *button = new QPushButton("Edit");
  button->setObjectName("EditButton");
  connect(button, SIGNAL(clicked()), SLOT(showEditorDialog()));
  layout->addWidget(button);
  setLayout(layout);
}

void pqCubeAxesPropertyWidget::showEditorDialog()
{
  pqCubeAxesEditorDialog dialog;
  dialog.setRepresentationProxy(this->proxy());
  dialog.exec();
}
