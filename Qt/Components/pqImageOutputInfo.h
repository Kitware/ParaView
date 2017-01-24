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
#ifndef pqImageOutputInfo_h
#define pqImageOutputInfo_h

#include "pqComponentsModule.h"
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QWidget>

class pqView;
namespace Ui
{
class ImageOutputInfo;
}

class PQCOMPONENTS_EXPORT pqImageOutputInfo : public QWidget
{
  Q_OBJECT

public:
  pqImageOutputInfo(QWidget* parent_ = NULL);
  pqImageOutputInfo(
    QWidget* parentObject, Qt::WindowFlags parentFlags, pqView* view, QString& viewName);

  ~pqImageOutputInfo();

  void setupScreenshotInfo();

  pqView* getView();
  QString getImageFileName();
  void hideFrequencyInput();
  void showFrequencyInput();
  void hideFitToScreen();
  void showFitToScreen();
  void hideMagnification();
  void showMagnification();
  void hideFilenameDetails();
  void showFilenameDetails();
  int getWriteFrequency();
  bool fitToScreen();
  int getMagnification();
  bool getComposite();
  bool getUseFloatValues();
  bool getNoValues();

  /**
  * Remove or add options depending on whether cinema is visible.
  */
  void setCinemaVisible(bool status);
  const QString getCameraType();
  double getPhi();
  double getTheta();
  double getRoll();
  QString getTrackObjectName();
  void setView(pqView* const view);

signals:
  void compositeChanged(bool checked);

public slots:
  void updateImageFileName();
  void updateImageFileNameExtension(const QString&);
  void updateCinemaType(const QString&);
  void updateComposite(int);
  void endisAbleDirectFloat(int);

private:
  void initialize(Qt::WindowFlags parentFlags, pqView* view, QString const& viewName);

  void updateSpherical();

  Q_DISABLE_COPY(pqImageOutputInfo)
  QScopedPointer<Ui::ImageOutputInfo> Ui;
  pqView* View;
};
#endif
