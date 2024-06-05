// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDoubleLineEdit_h
#define pqDoubleLineEdit_h

// Qt Includes.
#include <QScopedPointer> // for ivar
#include <QTextStream>    // for formatDouble

// ParaView Includes.
#include "pqLineEdit.h"
#include "pqWidgetsModule.h"

/**
 * @class pqDoubleLineEdit
 * @brief pqLineEdit subclass that supports a low precision view when inactive
 *
 * pqDoubleLineEdit allows to edit a real number in full precision and display
 * a simplified version when the widget is not in focus or under mouse pointer.
 *
 * The precision and notation associated with the displayed number can be
 * configured using the `precision` and `notation` properties on the
 * pqDoubleLineEdit instance. Additionally, pqDoubleLineEdit can be configure to
 * simply respect a global precision and notation specification by using the
 * property `useGlobalPrecisionAndNotation` (which is default).
 *
 * When using FullNotation, the displayed text logic is deactivated and the
 * user will be able to see the value that they will edit directly.
 *
 * Since pqDoubleLineEdit is intended for numeric values, in its constructor, a
 * `QDoubleValidator` is created for convenience.
 *
 */
class PQWIDGETS_EXPORT pqDoubleLineEdit : public pqLineEdit
{
  Q_OBJECT
  Q_ENUMS(RealNumberNotation)
  Q_PROPERTY(RealNumberNotation notation READ notation WRITE setNotation)
  Q_PROPERTY(int precision READ precision WRITE setPrecision)
  Q_PROPERTY(bool useGlobalPrecisionAndNotation READ useGlobalPrecisionAndNotation WRITE
      setUseGlobalPrecisionAndNotation)
  using Superclass = pqLineEdit;

public:
  pqDoubleLineEdit(QWidget* parent = nullptr);
  ~pqDoubleLineEdit() override;

  /**
   * This enum specifies which notations to use for displaying the value.
   */
  enum RealNumberNotation
  {
    MixedNotation = 0,
    ScientificNotation,
    FixedNotation,
    FullNotation
  };

  /**
   * Return the notation used to display the number.
   * \sa setNotation()
   */
  RealNumberNotation notation() const;

  /**
   * Return the precision used to display the number.
   * \sa setPrecision()
   */
  int precision() const;

  /**
   * `useGlobalPrecisionAndNotation` indicates if the pqDoubleLineEdit should
   * use global precision and notation values instead of the parameters
   * specified on this instance. Default is true.
   */
  bool useGlobalPrecisionAndNotation() const;

  ///@{
  /**
   * Get/set the global precision and notation. All pqDoubleLineEdit instances
   * that have `useGlobalPrecisionAndNotation` property set to true will
   * automatically respect the state set on the global variables.
   */
  static void setGlobalPrecisionAndNotation(int precision, RealNumberNotation notation);
  static int globalPrecision();
  static RealNumberNotation globalNotation();
  ///@}

  /**
   * Returns the text being shown when the widget is not active or under mouse
   * pointer. Primarily intended for test or debugging purposes.
   */
  QString simplifiedText() const;

  /**
   * Return a double formatted according to a pqDoubleLineEdit::RealNumberNotation
   * notation and a number of digits of precision.
   * Supports QLocale::FloatingPointShortest as precision.
   * When using pqDoubleLineEdit::RealNumberNotation::FullNotation, precision is not used and
   * fullLowExponent and fullHighExponent will be used to determine when to switch between
   * scientific notation and fixed notation.
   */
  static QString formatDouble(double value, pqDoubleLineEdit::RealNumberNotation notation,
    int precision, int fullLowExponent = -6, int fullHighExponent = 20);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the notation used to display the number.
   * \sa notation()
   */
  void setNotation(RealNumberNotation _notation);

  /**
   * Set the precision used to display the number.
   * \sa precision()
   */
  void setPrecision(int precision);

  /**
   * Set whether to use global precision and notation values. Default is true.
   * @sa useGlobalPrecisionAndNotation()
   */
  void setUseGlobalPrecisionAndNotation(bool value);

protected:
  void paintEvent(QPaintEvent* evt) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  Q_DISABLE_COPY(pqDoubleLineEdit)

  static int GlobalPrecision;
  static RealNumberNotation GlobalNotation;

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
