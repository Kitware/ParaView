// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  this->setToolTip(tr("Choose Expression"));
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
