/*=========================================================================

   Program: ParaView
   Module:    pqImageOutputInfo.h

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
#ifndef __pqImageOutputInfo_h
#define __pqImageOutputInfo_h

#include "pqComponentsModule.h"
#include <QString>
#include <QStringList>
#include <QWidget>
#include "ui_pqImageOutputInfo.h"

class pqView;

class PQCOMPONENTS_EXPORT pqImageOutputInfo : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqImageOutputInfo(
    QWidget *parentObject, Qt::WindowFlags parentFlags, pqView* view, QString& viewName);

  void setupScreenshotInfo();

  pqView* getView()
  {
    return this->View;
  }

  QString getImageFileName()
  {
    return this->Info.imageFileName->displayText();
  }

  void hideFrequencyInput()
  {
    this->Info.imageWriteFrequency->hide();
    this->Info.imageWriteFrequencyLabel->hide();
  }

  void showFrequencyInput()
  {
    this->Info.imageWriteFrequency->show();
    this->Info.imageWriteFrequencyLabel->show();
  }

  void hideFitToScreen()
  {
    this->Info.fitToScreen->hide();
  }

  void showFitToScreen()
  {
    this->Info.fitToScreen->show();
  }

  int getWriteFrequency()
  {
    return this->Info.imageWriteFrequency->value();
  }

  bool fitToScreen()
  {
    return this->Info.fitToScreen->isChecked();
  }

  int getMagnification()
  {
    return this->Info.imageMagnification->value();
  }

public slots:
  void updateImageFileName();
  void updateImageFileNameExtension(const QString&);

private:
  Q_DISABLE_COPY(pqImageOutputInfo)
  Ui::ImageOutputInfo Info;
  pqView* View;
};
#endif
