// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDoubleRangeWidget_h
#define pqDoubleRangeWidget_h

#include "pqDoubleSliderWidget.h"
#include "pqWidgetsModule.h"
#include <QWidget>

/**
 * Extends pqDoubleSliderWidget to use it with a range of doubles : provides
 * control on min/max, resolution and on line edit validator.
 */
class PQWIDGETS_EXPORT pqDoubleRangeWidget : public pqDoubleSliderWidget
{
  Q_OBJECT
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
  Q_PROPERTY(int resolution READ resolution WRITE setResolution)

  typedef pqDoubleSliderWidget Superclass;

public:
  /**
   * constructor requires the proxy, property
   */
  pqDoubleRangeWidget(QWidget* parent = nullptr);
  ~pqDoubleRangeWidget() override;

  // get the min range value
  double minimum() const;
  // get the max range value
  double maximum() const;

  // returns the resolution.
  int resolution() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  // set the min range value
  void setMinimum(double);
  // set the max range value
  void setMaximum(double);

  // set the resolution.
  void setResolution(int);

protected:
  int valueToSliderPos(double val) override;
  double sliderPosToValue(int pos) override;

private Q_SLOTS:
  void updateValidator();

private: // NOLINT(readability-redundant-access-specifiers)
  int Resolution;
  double Minimum;
  double Maximum;
};

#endif
