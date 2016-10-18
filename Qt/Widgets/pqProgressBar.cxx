/*=========================================================================

   Program: ParaView
   Module:    pqProgressBar.cxx

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
#include "pqProgressBar.h"

#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QString>

//-----------------------------------------------------------------------------
pqProgressBar::pqProgressBar(QWidget* _p)
  : QWidget(_p)
  , PreviousMessage("")
{

  QGridLayout* gridlayout = new QGridLayout(this);
  gridlayout->setSpacing(0);
  gridlayout->setContentsMargins(0, 0, 4, 0);

  this->ProgressBar = new QProgressBar(this);
  this->ProgressBar->setRange(0, 100);
  this->ProgressBar->setValue(0);
  this->ProgressBar->setTextVisible(false);
  this->ProgressBar->setMinimumWidth(200);
  this->ProgressBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  this->ProgressLabel = new QLabel(this);
  this->ProgressLabel->setMinimumWidth(100);
  this->ProgressLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  gridlayout->addWidget(this->ProgressBar, 0, 0);
  gridlayout->addWidget(this->ProgressLabel, 0, 1);
  this->setLayout(gridlayout);

  this->reset();
}

//-----------------------------------------------------------------------------
pqProgressBar::~pqProgressBar()
{
  delete this->ProgressBar;
  delete this->ProgressLabel;
}

//-----------------------------------------------------------------------------
void pqProgressBar::setProgress(const QString& message, int value)
{
  this->ProgressBar->setValue(value);
  QString msg = QString("%1: %2").arg(message, QString::number(value));
  if (msg.length() > this->PreviousMessage.length())
  {
    // we need to get the label to be resized correctly so it fits
    // this is trickier than it seems. By hiding it and than calling show
    // we invalidate numerous flags which causes an ensurePolished and
    // it also FORCES a updateGeometry. If you now a cleaner way, please
    // change this
    this->ProgressLabel->hide();
  }
  this->ProgressLabel->setText(msg);
  this->ProgressLabel->show();

  this->PreviousMessage = msg;
}

//-----------------------------------------------------------------------------
void pqProgressBar::reset()
{
  this->ProgressBar->reset();
  this->ProgressLabel->hide();
  this->PreviousMessage = "";
}
