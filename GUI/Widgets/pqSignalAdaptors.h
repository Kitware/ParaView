/*=========================================================================

   Program:   ParaQ
   Module:    pqSignalAdaptors.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef pq_SignalAdaptors_h
#define pq_SignalAdaptors_h

#include <QObject>
#include <QString>
#include <QVariant>
class QComboBox;
class QSlider;
#include "QtWidgetsExport.h"


/// signal adaptor to allow getting/setting/observing of a pseudo 'currentText' property of a combo box 
/// the QComboBox currentIndexChanged signal is forwarded to this currentTextChanged signal
class QTWIDGETS_EXPORT pqSignalAdaptorComboBox : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText)
public:
  /// constructor requires a QComboBox
  pqSignalAdaptorComboBox(QComboBox* p);
  /// get the current text of a combo box
  QString currentText() const;
signals:
  /// signal text changed in a combo box
  void currentTextChanged(const QString&);  
public slots:
  /// set the current text of a combo box (actually sets the index for the text)
  void setCurrentText(const QString&);
protected:
};

/// signal adaptor to allow getting/setting/observing of an rgba (0.0 - 1.0 range)
class QTWIDGETS_EXPORT pqSignalAdaptorColor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant color READ color WRITE setColor)
public:
  /// constructor requires a QObject, the name of the QColor property, and a signal for property changes
  pqSignalAdaptorColor(QObject* p, const char* colorProperty, const char* signal);
  /// get the color components
  QVariant color() const;
signals:
  /// signal the color changed
  void colorChanged(const QVariant&);
public slots:
  /// set the red component
  void setColor(const QVariant&);
protected slots:
  void handleColorChanged();
protected:
  QByteArray PropertyName;
};


/// signal adaptor to adjust the range of a int slider to (0.0-1.0)
class QTWIDGETS_EXPORT pqSignalAdaptorSliderRange : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue)
public:
  /// constructor requires the QSlider
  pqSignalAdaptorSliderRange(QSlider* p);
  /// get the value components
  double value() const;
signals:
  /// signal the color changed
  void valueChanged(double val);
public slots:
  /// set the red component
  void setValue(double val);
protected slots:
  void handleValueChanged();
};

#endif

