// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorSelectorPropertyWidget.h"

#include "pqColorChooserButtonWithPalettes.h"
#include "pqProxyWidget.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QCoreApplication>
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
  vbox->setContentsMargins(0, 0, 0, 0);

  if (useDocumentationForLabels)
  {
    QLabel* label =
      new QLabel(QString("<p>%1</p>").arg(pqProxyWidget::documentationText(smProperty)));
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    vbox->addWidget(label, /*stretch=*/1);
  }

  pqColorChooserButton* button = nullptr;
  pqColorChooserButtonWithPalettes* paletteButton = nullptr;
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
  button->setText(QCoreApplication::translate("ServerManagerXML", smProperty->GetXMLLabel()));
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
pqColorSelectorPropertyWidget::~pqColorSelectorPropertyWidget() = default;
