/*=========================================================================

   Program:   ParaView
   Module:    pqKeyFrameTypeWidget.h

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
#ifndef pqKeyFrameTypeWidget_h
#define pqKeyFrameTypeWidget_h

#include "pqComponentsModule.h"
#include <QWidget>
class QComboBox;

class PQCOMPONENTS_EXPORT pqKeyFrameTypeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString type READ type WRITE setType)

  Q_PROPERTY(QString base READ base WRITE setBase)
  Q_PROPERTY(QString startPower READ startPower WRITE setStartPower)
  Q_PROPERTY(QString endPower READ endPower WRITE setEndPower)

  Q_PROPERTY(double phase READ phase WRITE setPhase)
  Q_PROPERTY(QString offset READ offset WRITE setOffset)
  Q_PROPERTY(QString frequency READ frequency WRITE setFrequency)

public:
  pqKeyFrameTypeWidget(QWidget* parent = NULL);
  ~pqKeyFrameTypeWidget() override;

public Q_SLOTS:
  void setType(const QString& text);
  void setBase(const QString& text);
  void setStartPower(const QString& text);
  void setEndPower(const QString& text);
  void setPhase(double);
  void setOffset(const QString& text);
  void setFrequency(const QString& text);

public:
  QString type() const;
  QString base() const;
  QString startPower() const;
  QString endPower() const;
  double phase() const;
  QString offset() const;
  QString frequency() const;

  QComboBox* typeComboBox() const;

Q_SIGNALS:
  void typeChanged(const QString&);
  void baseChanged(const QString&);
  void startPowerChanged(const QString&);
  void endPowerChanged(const QString&);
  void phaseChanged(double);
  void offsetChanged(const QString&);
  void frequencyChanged(const QString&);

protected Q_SLOTS:
  void onTypeChanged();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
