/*=========================================================================

   Program: ParaView
   Module:    pqScaledSpinBox.h

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

========================================================================*/

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
