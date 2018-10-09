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
#include <QLineEdit>

// ParaView Includes.
#include "pqWidgetsModule.h"

/**
 * pqDoubleLineEdit allows to edit a real number in full precision and display
 * a simpified version.
 *
 * The precision and notation associated with the displayed number can be
 * configured using the corresponding properties.
 *
 * In its constructor, pqDoubleLineEdit sets up a QDoubleValidator as the
 * validator for the line edit.
 */
class PQWIDGETS_EXPORT pqDoubleLineEdit : public QLineEdit
{
  Q_OBJECT
  Q_ENUMS(RealNumberNotation)
  Q_PROPERTY(QString fullPrecisionText READ fullPrecisionText WRITE setFullPrecisionText)
  Q_PROPERTY(RealNumberNotation notation READ notation WRITE setNotation)
  Q_PROPERTY(int precision READ precision WRITE setPrecision)
  Q_PROPERTY(bool useGlobalPrecisionAndNotation READ useGlobalPrecisionAndNotation WRITE
      setUseGlobalPrecisionAndNotation)

  using Superclass = QLineEdit;

public:
  pqDoubleLineEdit(QWidget* parent = 0);
  ~pqDoubleLineEdit() override;

  /**
   * Get the real number
   * \sa setFullPrecisionText()
   */
  QString fullPrecisionText() const;

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
public slots:
  /**
   * Set the real number in standard notation.
   *
   * The signal fullPrecisionTextChanged() is emitted whenever the full precision text changes.
   *
   * \sa fullPrecisionText(), fullPrecisionTextChanged()
   */
  void setFullPrecisionText(const QString& _text);

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

signals:
  /**
   * This signal is emitted when the Return or Enter key is pressed or the line edit loses focus
   * and the full precision text is updated and is different from its original value.
   */
  void fullPrecisionTextChangedAndEditingFinished();

  /**
   * This signal is emitted whenever the full precision text changes. The text argument is the
   * new text.
   * Unlike fullPrecisionTextChangedAndEditingFinished(), this signal is also emitted when the
   * text is changed programmatically, for example, by calling setFullPrecisionText().
   *
   * \sa setFullPrecisionText()
   */
  void fullPrecisionTextChanged(const QString&);

protected slots:
  void onEditingStarted();
  void onEditingFinished();

  friend class pqDoubleLineEditEventPlayer;
  /**
   * This is called by pqDoubleLineEditEventPlayer during event playback to ensure
   * that the fullPrecisionTextChangedAndEditingFinished() signal is fired when text
   * is changed using setFullPrecisionText() in playback.
   */
  void triggerFullPrecisionTextChangedAndEditingFinished();

protected:
  void focusInEvent(QFocusEvent* event) override;
  void updateLimitedPrecisionText();

private:
  Q_DISABLE_COPY(pqDoubleLineEdit)

  QString FullPrecisionText;
  RealNumberNotation Notation;

  int Precision;
  bool UseGlobalPrecisionAndNotation;

  static int GlobalPrecision;
  static RealNumberNotation GlobalNotation;
};

#endif
