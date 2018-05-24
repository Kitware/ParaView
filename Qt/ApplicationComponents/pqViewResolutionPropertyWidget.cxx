/*=========================================================================

   Program: ParaView
   Module:  pqViewResolutionPropertyWidget.cxx

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
#include "pqViewResolutionPropertyWidget.h"
#include "ui_pqViewResolutionPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "vtkCommand.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QIntValidator>
#include <QStyle>

class pqViewResolutionPropertyWidget::pqInternals
{
public:
  Ui::ViewResolutionPropertyWidget Ui;
  QSize SizeForAspect;

  void resetAspect()
  {
    this->SizeForAspect = QSize(this->Ui.width->text().toInt(), this->Ui.height->text().toInt());
    if (this->SizeForAspect.isNull())
    {
      this->SizeForAspect = QSize(1, 1);
    }
  }
};

//-----------------------------------------------------------------------------
pqViewResolutionPropertyWidget::pqViewResolutionPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals())
{
  pqViewResolutionPropertyWidget::pqInternals& internals = (*this->Internals);
  Ui::ViewResolutionPropertyWidget& ui = internals.Ui;
  ui.setupUi(this);
  ui.reset->setIcon(ui.reset->style()->standardIcon(QStyle::SP_BrowserReload));

  QIntValidator* iv = new QIntValidator(ui.width);
  iv->setBottom(1);
  ui.width->setValidator(iv);

  iv = new QIntValidator(ui.height);
  iv->setBottom(1);
  ui.height->setValidator(iv);

  this->addPropertyLink(ui.width, "text2", SIGNAL(textChanged(const QString&)), smproperty, 0);
  this->addPropertyLink(ui.height, "text2", SIGNAL(textChanged(const QString&)), smproperty, 1);
  internals.resetAspect();

  // these need to be QueuedConnection since otherwise it interacts with
  // pqPropertyLinks since both widgets are connected to same smproperty.
  this->connect(ui.width, SIGNAL(textEdited(const QString&)), SLOT(widthTextEdited(const QString&)),
    Qt::QueuedConnection);
  this->connect(ui.height, SIGNAL(textEdited(const QString&)),
    SLOT(heightTextEdited(const QString&)), Qt::QueuedConnection);

  this->connect(ui.scaleBy, SIGNAL(scale(double)), SLOT(scale(double)));
  this->connect(ui.lockAspectRatio, SIGNAL(toggled(bool)), SLOT(lockAspectRatioToggled(bool)));

  this->connect(ui.reset, SIGNAL(clicked()), SLOT(resetButtonClicked()));
  ui.reset->connect(this, SIGNAL(highlightResetButton()), SLOT(highlight()));
  ui.reset->connect(this, SIGNAL(clearHighlight()), SLOT(clear()));

  pqCoreUtilities::connect(
    smproperty, vtkCommand::DomainModifiedEvent, this, SLOT(resetButtonClicked()));
  pqCoreUtilities::connect(
    smproperty, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(highlightResetButton()));
}

//-----------------------------------------------------------------------------
pqViewResolutionPropertyWidget::~pqViewResolutionPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::resetButtonClicked()
{
  if (vtkSMProperty* smproperty = this->property())
  {
    smproperty->ResetToDomainDefaults(/*use_unchecked_values*/ false);
    emit this->changeAvailable();
    emit this->changeFinished();
    this->Internals->resetAspect();
  }
  emit this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::apply()
{
  this->Superclass::apply();
  emit this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::reset()
{
  this->Superclass::reset();
  emit this->clearHighlight();
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::scale(double factor)
{
  Ui::ViewResolutionPropertyWidget& ui = this->Internals->Ui;
  ui.width->setTextAndResetCursor(
    QString::number(static_cast<int>(ui.width->text().toInt() * factor)));
  ui.height->setTextAndResetCursor(
    QString::number(static_cast<int>(ui.height->text().toInt() * factor)));
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::widthTextEdited(const QString& txt)
{
  pqViewResolutionPropertyWidget::pqInternals& internals = (*this->Internals);
  if (!internals.Ui.lockAspectRatio->isChecked())
  {
    return;
  }

  const int newWidth = txt.toInt();
  const int newHeight =
    (internals.SizeForAspect.height() * newWidth) / internals.SizeForAspect.width();
  internals.Ui.height->setTextAndResetCursor(QString::number(newHeight));
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::heightTextEdited(const QString& txt)
{
  pqViewResolutionPropertyWidget::pqInternals& internals = (*this->Internals);
  if (!internals.Ui.lockAspectRatio->isChecked())
  {
    return;
  }

  const int newHeight = txt.toInt();
  const int newWidth =
    (internals.SizeForAspect.width() * newHeight) / internals.SizeForAspect.height();
  internals.Ui.width->setTextAndResetCursor(QString::number(newWidth));
}

//-----------------------------------------------------------------------------
void pqViewResolutionPropertyWidget::lockAspectRatioToggled(bool checked)
{
  if (checked)
  {
    // when lock is turned on, save the current aspect ratio.
    this->Internals->resetAspect();
  }
}
