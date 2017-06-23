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

#include "pqWidgetsModule.h"
#include <QWidget>

class QSlider;
class pqLineEdit;
class QDoubleValidator;

/**
 * A widget with a tied slider and line edit for editing a double property
 */
class PQWIDGETS_EXPORT pqDoubleSliderWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue USER true)

public:
  pqDoubleSliderWidget(QWidget* parent = NULL);
  ~pqDoubleSliderWidget();

  /**
   * get the value
   */
  double value() const;

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
  pqLineEdit* LineEdit;
  bool BlockUpdate;
  bool InteractingWithSlider;
  bool DeferredValueEdited;
};

#endif
