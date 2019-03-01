/*=========================================================================

   Program: ParaView
   Module: pqColorSelectorPropertyWidget.h

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
#include "pqColorSelectorPropertyWidget.h"

#include "pqColorChooserButtonWithPalettes.h"
#include "pqProxyWidget.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QLabel>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqColorSelectorPropertyWidget::pqColorSelectorPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, bool withPalette, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
{
  this->setShowLabel(false);

  bool useDocumentationForLabels = pqProxyWidget::useDocumentationForLabels(smProxy);

  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setSpacing(0);
  vbox->setMargin(0);

  if (useDocumentationForLabels)
  {
    QLabel* label =
      new QLabel(QString("<p>%1</p>").arg(pqProxyWidget::documentationText(smProperty)));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    vbox->addWidget(label, /*stretch=*/1);
  }

  pqColorChooserButton* button = NULL;
  pqColorChooserButtonWithPalettes* paletteButton = NULL;
  if (withPalette)
  {
    paletteButton = new pqColorChooserButtonWithPalettes(this);
    button = paletteButton;
  }
  else
  {
    button = new pqColorChooserButton(this);
  }
  button->setObjectName("ColorButton");
  button->setText(smProperty->GetXMLLabel());
  button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

  if (vtkSMPropertyHelper(smProperty).GetNumberOfElements() == 3)
  {
    button->setShowAlphaChannel(false);
    this->addPropertyLink(
      button, "chosenColorRgbF", SIGNAL(chosenColorChanged(const QColor&)), smProperty);
  }
  else if (vtkSMPropertyHelper(smProperty).GetNumberOfElements() == 4)
  {
    button->setShowAlphaChannel(true);
    this->addPropertyLink(
      button, "chosenColorRgbaF", SIGNAL(chosenColorChanged(const QColor&)), smProperty);
  }
  else
  {
    qDebug("Currently, only SMProperty with 3 or 4 elements is supported.");
  }

  if (withPalette)
  {
    // pqColorPaletteLinkHelper makes it possible to set this color to one of
    // the colors in the application palette..
    new pqColorPaletteLinkHelper(paletteButton, smProxy, smProxy->GetPropertyName(smProperty));
  }
  vbox->addWidget(button, 1);
}

//-----------------------------------------------------------------------------
pqColorSelectorPropertyWidget::~pqColorSelectorPropertyWidget()
{
}
