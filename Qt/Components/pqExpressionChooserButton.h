/*=========================================================================

   Program: ParaView
   Module:  pqExpressionChooserButton.h

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
#ifndef pqExpressionChooserButton_h
#define pqExpressionChooserButton_h

#include "pqComponentsModule.h"
#include <QToolButton>

/**
 * @class pqExpressionChooserButton
 * @brief Custom QToolButton that adds a menu to select expression.
 *
 * This is simply a QToolButton with a menu. The menu has actions which
 * correspond to expressions. When user clicks any of those actions,
 * `pqExpressionChooserButton::expressionSelected` signal is fired with the
 * expression as argument.
 */
class PQCOMPONENTS_EXPORT pqExpressionChooserButton : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  pqExpressionChooserButton(QWidget* parent, const QString& group = "");
  ~pqExpressionChooserButton() override;

  void setGroup(const QString& group);

Q_SIGNALS:
  void expressionSelected(const QString& expr) const;

protected Q_SLOTS:
  void updateMenu();

protected: // NOLINT(readability-redundant-access-specifiers)
  QString Group;

private:
  Q_DISABLE_COPY(pqExpressionChooserButton);
};

#endif
