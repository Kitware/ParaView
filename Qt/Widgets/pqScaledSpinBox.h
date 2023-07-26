// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#ifndef pqScaledSpinBox_h
#define pqScaledSpinBox_h

#include "pqWidgetsModule.h"
#include <QDoubleSpinBox>

/**
 * \brief Identical to a QDoubleSpinBox, but doubles/halves the value upon button presses.
 */
class PQWIDGETS_EXPORT pqScaledSpinBox : public QDoubleSpinBox
{
  Q_OBJECT

public Q_SLOTS:
  /** \brief Overrides QDoubleSpinBox::setValue() */
  void setValue(double val);

public:
  /** \brief Constructor */
  explicit pqScaledSpinBox(QWidget* parent = nullptr);
  /** \brief Copy constructor */
  explicit pqScaledSpinBox(QDoubleSpinBox* other);
  /** \brief Destructor */
  ~pqScaledSpinBox();

  /** \brief Sets the Scale Factor used to increase / decrease the widget value.
   *           When scaling up, the value is multiplied by the Scale Factor.
   *           When scaling down, the value is divided by the Scale Factor. */
  void setScalingFactor(double scaleFactor);

protected:
  /** \brief Overrides QDoubleSpinBox::keyPressEvent() */
  void keyPressEvent(QKeyEvent* event);

protected Q_SLOTS:
  void onValueChanged(double newValue);

private:
  void initialize();
  void scaledStepUp();
  void scaledStepDown();

  double LastValue;
  double ScaleFactor;
};

#endif // pqScaledSpinBox_h
