/*=========================================================================

   Program: ParaView
   Module:    pqProgressWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqProgressWidget.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPainter>
#include <QPair>
#include <QPointer>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOptionProgressBar>
#include <QTimerEvent>
#include <QToolButton>
#include <QtDebug>

class pqProgressWidgetLabel : public QLabel
{
  int ProgressPercentage;
  bool ShowProgress;
  QStyle* Style;

public:
  pqProgressWidgetLabel(QWidget* parentObj)
    : QLabel(parentObj)
    , ProgressPercentage(0)
    , ShowProgress(false)
    , Style(nullptr)
  {
    this->setFrameShape(QFrame::Panel);
    this->setFrameShadow(QFrame::Sunken);
    this->setLineWidth(1);

#ifdef __APPLE__
    // We don't use mac style on apple since it render horrible progress bars.
    this->Style = QStyleFactory::create("fusion");
    if (!this->Style)
    {
      this->Style = QStyleFactory::create("cleanlooks");
    }
#endif
  }
  ~pqProgressWidgetLabel() override { delete this->Style; }
  // returns true if value changed.
  bool setProgressPercentage(int val)
  {
    val = std::min(std::max(0, val), 100);
    if (this->ProgressPercentage != val)
    {
      this->ProgressPercentage = val;
      return true;
    }
    return false;
  }
  int progressPercentage() const { return this->ProgressPercentage; }
  void setShowProgress(bool val) { this->ShowProgress = val; }
  bool showProgress() const { return this->ShowProgress; }
protected:
  QStyle* astyle() { return this->Style ? this->Style : this->style(); }
  void paintEvent(QPaintEvent* evt) override
  {
    QStyleOptionProgressBar pbstyle;
    pbstyle.initFrom(this);
    pbstyle.minimum = 0;
    pbstyle.progress = this->ShowProgress ? this->ProgressPercentage : 0;
    pbstyle.maximum = 100;
    pbstyle.text = this->ShowProgress
      ? QString("%1 (%2%)").arg(this->text()).arg(this->ProgressPercentage)
      : this->text();
    pbstyle.textAlignment = this->alignment();
    pbstyle.rect = this->rect();

    if (this->ShowProgress && this->ProgressPercentage > 0)
    {
      // let's print the frame border etc.
      // note we're skipping QLabel code to avoid overlapping label text.
      this->QFrame::paintEvent(evt);

      // we deliberately don't draw the progress bar groove to avoid a dramatic
      // change in the progress bar.
      QPainter painter(this);
      pbstyle.textVisible = false;
      this->astyle()->drawControl(QStyle::CE_ProgressBarContents, &pbstyle, &painter, this);
      pbstyle.textVisible = true;
      this->astyle()->drawControl(QStyle::CE_ProgressBarLabel, &pbstyle, &painter, this);
    }
    else
    {
      this->QLabel::paintEvent(evt);
    }
  }

private:
  Q_DISABLE_COPY(pqProgressWidgetLabel);
};

//-----------------------------------------------------------------------------
pqProgressWidget::pqProgressWidget(QWidget* _parent /*=0*/)
  : Superclass(_parent, Qt::FramelessWindowHint)
  , ReadyText()
  , BusyText("Busy")
{
  QHBoxLayout* hbox = new QHBoxLayout(this);
  hbox->setSpacing(2);
  hbox->setMargin(0);

  this->AbortButton = new QToolButton(this);
  this->AbortButton->setObjectName("AbortButton");
  this->AbortButton->setIcon(QIcon(QString::fromUtf8(":/QtWidgets/Icons/pqDelete.svg")));
  this->AbortButton->setIconSize(QSize(12, 12));
  this->AbortButton->setToolTip(tr("Abort"));
  this->AbortButton->setEnabled(false);
  QObject::connect(this->AbortButton, SIGNAL(pressed()), this, SIGNAL(abortPressed()));
  hbox->addWidget(this->AbortButton);

  this->ProgressBar = new pqProgressWidgetLabel(this);
  this->ProgressBar->setObjectName("ProgressBar");
  this->ProgressBar->setAlignment(Qt::AlignCenter);
  this->ProgressBar->setText(this->readyText());
  hbox->addWidget(this->ProgressBar, 1);
}

//-----------------------------------------------------------------------------
pqProgressWidget::~pqProgressWidget()
{
  delete this->ProgressBar;
  delete this->AbortButton;
}

//-----------------------------------------------------------------------------
void pqProgressWidget::setReadyText(const QString& txt)
{
  this->ReadyText = txt;
  if (!this->ProgressBar->showProgress())
  {
    this->ProgressBar->setText(txt);
  }
}

//-----------------------------------------------------------------------------
void pqProgressWidget::setBusyText(const QString& txt)
{
  this->BusyText = txt;
  if (this->ProgressBar->showProgress())
  {
    this->ProgressBar->setText(txt);
  }
}

//-----------------------------------------------------------------------------
void pqProgressWidget::setProgress(const QString& message, int value)
{
  if (this->ProgressBar->showProgress() &&
    (this->ProgressBar->progressPercentage() != value || this->ProgressBar->text() != message))
  {
    this->ProgressBar->setText(message);
    if (this->ProgressBar->setProgressPercentage(value))
    {
      this->updateUI();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProgressWidget::enableProgress(bool enabled)
{
  if (this->ProgressBar->showProgress() != enabled)
  {
    this->ProgressBar->setShowProgress(enabled);
    bool changed = this->ProgressBar->setProgressPercentage(0);
    this->ProgressBar->setText(enabled ? this->busyText() : this->readyText());
    if (changed)
    {
      this->updateUI();
    }
  }
}

//-----------------------------------------------------------------------------
void pqProgressWidget::updateUI()
{
  this->ProgressBar->repaint();
  this->AbortButton->repaint();
}

//-----------------------------------------------------------------------------
void pqProgressWidget::enableAbort(bool enabled)
{
  this->AbortButton->setEnabled(enabled);
}
