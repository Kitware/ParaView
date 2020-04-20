/*=========================================================================

   Program: ParaView
   Module:    pqSignalAdaptors.h

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

#ifndef pq_SignalAdaptors_h
#define pq_SignalAdaptors_h

#include <QObject>
#include <QString>
#include <QVariant>
class QComboBox;
class QSlider;
class QTextEdit;
class QSpinBox;

#include "pqWidgetsModule.h"

/**
* signal adaptor to allow getting/setting/observing of a pseudo 'currentText' property of a combo
* box
* the QComboBox currentIndexChanged signal is forwarded to this currentTextChanged signal
*/
class PQWIDGETS_EXPORT pqSignalAdaptorComboBox : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString currentText READ currentText WRITE setCurrentText)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex)
  Q_PROPERTY(QVariant currentData READ currentData WRITE setCurrentData)

public:
  /**
  * constructor requires a QComboBox
  */
  pqSignalAdaptorComboBox(QComboBox* p);
  /**
  * get the current text of a combo box
  */
  QString currentText() const;
  /**
  * get the current index of a combo box.
  */
  int currentIndex() const;

  /**
  * get the user data associated with the current index.
  */
  QVariant currentData() const;

Q_SIGNALS:
  /**
  * signal text changed in a combo box
  */
  void currentTextChanged(const QString&);

  void currentIndexChanged(int);
public Q_SLOTS:
  /**
  * set the current text of a combo box (actually sets the index for the text)
  */
  void setCurrentText(const QString&);

  /**
  * set the current index of a combox box.
  */
  void setCurrentIndex(int index);

  /**
  * set the current index to the index with user data as the argument.
  */
  void setCurrentData(const QVariant& data);

protected:
};

/**
* signal adaptor to allow getting/setting/observing of an rgba (0.0 - 1.0 range)
*/
class PQWIDGETS_EXPORT pqSignalAdaptorColor : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant color READ color WRITE setColor)
public:
  /**
  * constructor requires a QObject, the name of the QColor property, and a signal for property
  * changes
  */
  pqSignalAdaptorColor(QObject* p, const char* colorProperty, const char* signal, bool enableAlpha);
  /**
  * get the color components
  */
  QVariant color() const;

Q_SIGNALS:
  /**
  * signal the color changed
  */
  void colorChanged(const QVariant&);
public Q_SLOTS:
  /**
  * set the red component
  */
  void setColor(const QVariant&);
protected Q_SLOTS:
  void handleColorChanged();

protected:
  QByteArray PropertyName;
  bool EnableAlpha;
};

/**
* signal adaptor to adjust the range of a int slider to (0.0-1.0)
*/
class PQWIDGETS_EXPORT pqSignalAdaptorSliderRange : public QObject
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue)
public:
  /**
  * constructor requires the QSlider
  */
  pqSignalAdaptorSliderRange(QSlider* p);
  /**
  * get the value components
  */
  double value() const;
Q_SIGNALS:
  /**
  * signal the color changed
  */
  void valueChanged(double val);
public Q_SLOTS:
  /**
  * set the red component
  */
  void setValue(double val);
protected Q_SLOTS:
  void handleValueChanged();
};

/**
* signal adaptor that lets us get the text inside a QTextEdit
*/
class PQWIDGETS_EXPORT pqSignalAdaptorTextEdit : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)

public:
  /**
  * constructor
  */
  pqSignalAdaptorTextEdit(QTextEdit* p);
  /**
  * get the current text
  */
  QString text() const;
Q_SIGNALS:
  /**
  * signal text changed
  */
  void textChanged();
public Q_SLOTS:
  void setText(const QString&);

protected:
};

/**
* signal adaptor that lets us set/get the integer value inside a QSpinBox
*/
class PQWIDGETS_EXPORT pqSignalAdaptorSpinBox : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue)

public:
  /**
  * constructor
  */
  pqSignalAdaptorSpinBox(QSpinBox* p);
  /**
  * get the current text
  */
  int value() const;
Q_SIGNALS:
  /**
  * signal text changed
  */
  void valueChanged(int val);
public Q_SLOTS:
  void setValue(int val);

protected:
};

#endif
