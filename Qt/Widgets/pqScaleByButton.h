/*=========================================================================

   Program: ParaView
   Module:  pqScaleByButton.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqScaleByButton_h
#define pqScaleByButton_h

#include "pqWidgetsModule.h"
#include <QMap> // needed for QMap
#include <QToolButton>

/**
 * @class pqScaleByButton
 * @brief Custom QToolButton that adds a menu to key user scale by a factor.
 *
 * This is simply a QToolButton with a menu. The menu has actions which
 * correspond to scale factors. When user clicks any of those actions,
 * `pqScaleByButton::scale` signal is fired with the argument as the factor.
 */
class PQWIDGETS_EXPORT pqScaleByButton : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  /**
   * Creates the button with default scale factors or `0.5X` and `2.0X`.
   */
  pqScaleByButton(QWidget* parent = nullptr);

  /**
   * Creates the button with specified scale factors. The label for the actions
   * is created by use the suffix specified.
   */
  pqScaleByButton(
    const QList<double>& scaleFactors, const QString& suffix = "X", QWidget* parent = nullptr);

  /**
   * Creates the button with specified scale factors and labels.
   */
  pqScaleByButton(const QMap<double, QString>& scaleFactorsAndLabels, QWidget* parent = nullptr);

  virtual ~pqScaleByButton();

Q_SIGNALS:
  /**
   * Fired when the action corresponding to a scale factor is triggered.
   */
  void scale(double factor);

private Q_SLOTS:
  void scaleTriggered();

private:
  Q_DISABLE_COPY(pqScaleByButton);
  void constructor(const QMap<double, QString>& scaleFactors);
};

#endif
