/*=========================================================================

   Program: ParaView
   Module:  pqExpressionChooserButton.cxx

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

#include "pqExpressionChooserButton.h"

#include "pqApplicationCore.h"
#include "pqExpressionsManager.h"
#include "pqSettings.h"

#include <QAction>
#include <QMenu>

//-----------------------------------------------------------------------------
pqExpressionChooserButton::pqExpressionChooserButton(QWidget* parent, const QString& group)
  : Superclass(parent)
  , Group(group)
{
  this->setToolButtonStyle(Qt::ToolButtonIconOnly);
  this->setToolTip("Choose Expression");
  this->setPopupMode(QToolButton::InstantPopup);

  QMenu* menuList = new QMenu(this);
  menuList->setContentsMargins(-20, 0, -20, 0);
  this->setMenu(menuList);

  // make it one-column menu, with smaller margin
  QString styleSheet = QString("QMenu { menu-scrollable: %1; }").arg(1);
  menuList->setStyleSheet(styleSheet);

  this->connect(menuList, &QMenu::aboutToShow, this, &pqExpressionChooserButton::updateMenu);

  this->connect(this, &QToolButton::triggered, [=](QAction* sender) {
    if (sender != this->defaultAction())
    {
      Q_EMIT this->expressionSelected(sender->data().toString());
    }
  });
}

//-----------------------------------------------------------------------------
pqExpressionChooserButton::~pqExpressionChooserButton() = default;

//-----------------------------------------------------------------------------
void pqExpressionChooserButton::setGroup(const QString& group)
{
  this->Group = group;
}

//-----------------------------------------------------------------------------
void pqExpressionChooserButton::updateMenu()
{
  QList<pqExpressionsManager::pqExpression> expressions =
    pqExpressionsManager::getExpressionsFromSettings(this->Group);
  QFontMetrics fm = this->fontMetrics();
  this->menu()->clear();
  int maxWidth = this->parentWidget()->size().width();

  std::sort(expressions.begin(), expressions.end());

  for (const pqExpressionsManager::pqExpression& expr : expressions)
  {
    QString text = expr.Name.isEmpty() ? expr.Value : expr.Name;
    if (text.trimmed().isEmpty())
    {
      continue;
    }

    QString displayText = fm.elidedText(text, Qt::ElideMiddle, maxWidth);
    QAction* action = this->menu()->addAction(displayText);
    action->setIconVisibleInMenu(false);
    action->setShortcutVisibleInContextMenu(false);
    action->setData(expr.Value);
    if (!expr.Name.isEmpty())
    {
      auto font = action->font();
      font.setBold(true);
      action->setFont(font);
    }
  }
}
