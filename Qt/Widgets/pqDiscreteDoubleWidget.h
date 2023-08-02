// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDiscreteDoubleWidget_h
#define pqDiscreteDoubleWidget_h

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
  ~pqDiscreteDoubleWidget() override;

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

#endif // pqDiscreteDoubleWidget_h
