/*=========================================================================

  Program:   ParaView
  Module:    pqDiscreteDoubleWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqDiscreteDoubleWidget_H
#define pqDiscreteDoubleWidget_H

#include "pqDoubleSliderWidget.h"

#include <QVector>
#include <QWidget>

/**
 * Customize pqDoubleSliderWidget to use a custom set of allowed values
 */
class PQWIDGETS_EXPORT pqDiscreteDoubleWidget : public pqDoubleSliderWidget
{
  typedef pqDoubleSliderWidget Superclass;
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue USER true)

public:
  pqDiscreteDoubleWidget(QWidget* parent = nullptr);
  ~pqDiscreteDoubleWidget();

  /**
   * Gets vector of allowed values
   */
  std::vector<double> values() const;
  void setValues(std::vector<double> values);

protected:
  int valueToSliderPos(double val) override;
  double sliderPosToValue(int pos) override;

private:
  QVector<double> Values;
};

#endif // pqDiscreteDoubleWidget_H
