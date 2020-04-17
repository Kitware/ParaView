/*=========================================================================

   Program: ParaView
   Module:  pqDoubleLineEdit.h

   Copyright (c) 2005-2018 Sandia Corporation, Kitware Inc.
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

========================================================================*/
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
  pqDoubleLineEdit(QWidget* parent = 0);
  ~pqDoubleLineEdit() override;

  /**
   * This enum specifies which notations to use for displaying the value.
   */
  enum RealNumberNotation
  {
    MixedNotation = 0,
    ScientificNotation,
    FixedNotation
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

  //@{
  /**
   * Get/set the global precision and notation. All pqDoubleLineEdit instances
   * that have `useGlobalPrecisionAndNotation` property set to true will
   * automatically respect the state set on the global variables.
   */
  static void setGlobalPrecisionAndNotation(int precision, RealNumberNotation notation);
  static int globalPrecision();
  static RealNumberNotation globalNotation();
  //@}

  /**
   * Returns the text being shown when the widget is not active or under mouse
   * pointer. Primarily intended for test or debugging purposes.
   */
  QString simplifiedText() const;

  //@{
  /**
   * Return a double formatted according to a QTextStream::RealNumberNotation and number
   * of digits of precision.
   */
  static QString formatDouble(
    double value, QTextStream::RealNumberNotation notation, int precision);
  static QString formatDouble(
    double value, pqDoubleLineEdit::RealNumberNotation notation, int precision);
  //@}

  //@{
  /**
   * Return a double formatted according to the values set for global precision
   * and notation.
   */
  static QString formatDoubleUsingGlobalPrecisionAndNotation(double value);
  //@}

public Q_SLOTS:
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
