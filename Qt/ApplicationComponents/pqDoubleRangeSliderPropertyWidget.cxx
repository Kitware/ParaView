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
#include "pqDoubleRangeSliderPropertyWidget.h"
#include "ui_pqDoubleRangeSliderPropertyWidget.h"

#include "ctkDoubleRangeSlider.h"
#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "pqWidgetRangeDomain.h"
#include "vtkSMProperty.h"

#include <QDoubleValidator>
#include <QGridLayout>

class pqDoubleRangeSliderPropertyWidget::pqInternals
{
public:
  Ui::DoubleRangeSliderPropertyWidget Ui;
  bool IgnoreSliders;

  pqInternals() : IgnoreSliders(false) { }
};

//-----------------------------------------------------------------------------
pqDoubleRangeSliderPropertyWidget::pqDoubleRangeSliderPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* smProperty, QWidget* parentObject)
  : Superclass(smProxy, parentObject),
  Internals(new pqDoubleRangeSliderPropertyWidget::pqInternals())
{
  this->setShowLabel(false);
  this->setChangeAvailableAsChangeFinished(false);

  Ui::DoubleRangeSliderPropertyWidget &ui = this->Internals->Ui;
  ui.setupUi(this);

  ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
  ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

  ui.gridLayout->removeWidget(ui.placeHolder);
  delete ui.placeHolder;
  ui.placeHolder = NULL;

  QWidget* labelWidget = pqProxyWidget::newGroupLabelWidget(
    smProperty->GetXMLLabel(), this);
  ui.gridLayout->addWidget(labelWidget, 0, 0, 1, -1);

  ui.RangeSlider->setOrientation(Qt::Horizontal);
  ui.ThresholdBetween_0->setValidator(new QDoubleValidator(this));
  ui.ThresholdBetween_1->setValidator(new QDoubleValidator(this));

  this->addPropertyLink(ui.ThresholdBetween_0, "text2",
    SIGNAL(textChanged(const QString&)), smProperty, 0);
  this->addPropertyLink(ui.ThresholdBetween_1, "text2",
    SIGNAL(textChanged(const QString&)), smProperty, 1);
  this->connect(ui.ThresholdBetween_0, SIGNAL(textChangedAndEditingFinished()),
                this, SIGNAL(changeFinished()));
  this->connect(ui.ThresholdBetween_1, SIGNAL(textChangedAndEditingFinished()),
                this, SIGNAL(changeFinished()));

  QObject::connect(ui.ThresholdBetween_0, SIGNAL(textChanged(const QString&)),
    this, SLOT(updateSlider()));
  QObject::connect(ui.ThresholdBetween_1, SIGNAL(textChanged(const QString&)),
    this, SLOT(updateSlider()));

  QObject::connect(ui.RangeSlider, SIGNAL(minimumValueChanged(double)),
    this, SLOT(minimumValueSliderChanged(double)));
  QObject::connect(ui.RangeSlider, SIGNAL(maximumValueChanged(double)),
    this, SLOT(maximumValueSliderChanged(double)));

  /// pqWidgetRangeDomain ensures that whenever the domain changes, the slider's
  /// ranges are updated.
  new pqWidgetRangeDomain(ui.RangeSlider, "minimum", "maximum", smProperty, 0);

  // TODO: singleStep must be updated when the slider range changes to be
  // proportional to the slider range.
  ui.RangeSlider->setSingleStep(0.01);

  this->updateSlider();
}

//-----------------------------------------------------------------------------
pqDoubleRangeSliderPropertyWidget::~pqDoubleRangeSliderPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqDoubleRangeSliderPropertyWidget::updateSlider()
{
  // updateSlider gets called as the user is editing the texts as well. Thus
  // it's possible that max < min. But that's okay since the DoubleRangeWidget
  // handles that reasonably okay.
  if (!this->Internals->IgnoreSliders)
    {
    this->Internals->IgnoreSliders = true;
    const Ui::DoubleRangeSliderPropertyWidget &ui = this->Internals->Ui;
    ui.RangeSlider->setMinimumValue(ui.ThresholdBetween_0->text().toDouble());
    ui.RangeSlider->setMaximumValue(ui.ThresholdBetween_1->text().toDouble());
    this->Internals->IgnoreSliders = false;
    }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeSliderPropertyWidget::minimumValueSliderChanged(double val)
{
  if (!this->Internals->IgnoreSliders)
    {
    this->Internals->IgnoreSliders = true;
    const Ui::DoubleRangeSliderPropertyWidget &ui = this->Internals->Ui;
    ui.ThresholdBetween_0->setTextAndResetCursor(QVariant(val).toString());
    this->Internals->IgnoreSliders = false;
    emit this->changeFinished();
    }
}

//-----------------------------------------------------------------------------
void pqDoubleRangeSliderPropertyWidget::maximumValueSliderChanged(double val)
{
  if (!this->Internals->IgnoreSliders)
    {
    this->Internals->IgnoreSliders = true;
    const Ui::DoubleRangeSliderPropertyWidget &ui = this->Internals->Ui;
    ui.ThresholdBetween_1->setTextAndResetCursor(QVariant(val).toString());
    this->Internals->IgnoreSliders = false;
    emit this->changeFinished();
    }
}
