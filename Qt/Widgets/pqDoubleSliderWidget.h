/*=========================================================================

  Program:   ParaView
  Module:    pqDoubleSliderWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqDoubleSliderWidget_H
#define pqDoubleSliderWidget_H

#include "pqDoubleLineEdit.h"
#include "pqWidgetsModule.h"
#include <QWidget>

class QDoubleValidator;
class QSlider;

/**
 * A widget with a tied slider and line edit for editing a double property
 */
class PQWIDGETS_EXPORT pqDoubleSliderWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue USER true)
  Q_PROPERTY(pqDoubleLineEdit::RealNumberNotation notation READ notation WRITE setNotation)
  Q_PROPERTY(int precision READ precision WRITE setPrecision)
  Q_PROPERTY(bool useGlobalPrecisionAndNotation READ useGlobalPrecisionAndNotation WRITE
      setUseGlobalPrecisionAndNotation)
public:
  pqDoubleSliderWidget(QWidget* parent = NULL);
  ~pqDoubleSliderWidget();

  /**
   * get the value
   */
  double value() const;

  /**
   * Return the notation used to display the number.
   * \sa setNotation()
   */
  pqDoubleLineEdit::RealNumberNotation notation() const;

  /**
   * Return the precision used to display the number.
   * \sa setPrecision()
   */
  int precision() const;

  /**
   * `useGlobalPrecisionAndNotation` indicates if the pqDoubleLineEdit used by
   * this widget should use global precision and notation values instead of
   * the parameters specified on this instance.
   */
  bool useGlobalPrecisionAndNotation() const;

signals:
  /**
   * signal the value changed
   */
  void valueChanged(double);

  /**
   * signal the value was edited
   * this means the user is done changing text
   * or the user is done moving the slider. It implies
   * value was changed and editing has finished.
   */
  void valueEdited(double);

public slots:
  /**
   * set the value
   */
  void setValue(double val);

  /**
   * Set the notation used to display the number.
   * \sa notation()
   */
  void setNotation(pqDoubleLineEdit::RealNumberNotation _notation);

  /**
   * Set the precision used to display the number.
   * \sa precision()
   */
  void setPrecision(int precision);

  /**
   * Set whether to use global precision and notation values.
   * @sa useGlobalPrecisionAndNotation()
   */
  void setUseGlobalPrecisionAndNotation(bool value);

protected:
  virtual int valueToSliderPos(double val);
  virtual double sliderPosToValue(int pos);
  void setValidator(QDoubleValidator* validator);
  const QDoubleValidator* validator() const;
  void setSliderRange(int min, int max);
  void updateSlider();

private slots:
  void sliderChanged(int val);
  void textChanged(const QString& text);
  void editingFinished();
  void sliderPressed();
  void sliderReleased();
  void emitValueEdited();
  void emitIfDeferredValueEdited();

private:
  double Value;
  QSlider* Slider;
  pqDoubleLineEdit* DoubleLineEdit;
  bool BlockUpdate;
  bool InteractingWithSlider;
  bool DeferredValueEdited;
};

#endif
