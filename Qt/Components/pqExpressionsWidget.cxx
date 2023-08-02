// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqExpressionsWidget.h"

#include "pqCoreUtilities.h"
#include "pqExpressionChooserButton.h"
#include "pqExpressionsDialog.h"
#include "pqExpressionsManager.h"
#include "pqOneLinerTextEdit.h"

#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QTextEdit>
#include <QToolButton>

pqExpressionsWidget::pqExpressionsWidget(QWidget* parent, const QString& groupName)
  : Superclass(parent)
  , OneLiner(nullptr)
{
  QGridLayout* grid = new QGridLayout(this);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setSpacing(0);

  if (!groupName.isEmpty())
  {
    this->setupButtons(groupName);
  }

  this->setLayout(grid);
}

void pqExpressionsWidget::clear()
{
  lineEdit()->clear();
}

void pqExpressionsWidget::setupButtons(const QString& groupName)
{
  auto grid = dynamic_cast<QGridLayout*>(this->layout());

  this->OneLiner = new pqOneLinerTextEdit(this);
  this->OneLiner->setObjectName("OneLiner");
  grid->addWidget(this->OneLiner, 0, 0, 1, 5);

  // add stats line
  auto statusLine = new QLabel(this);
  statusLine->hide();
  statusLine->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);
  grid->addWidget(statusLine, 1, 0);

  // add expressions manager buttons
  auto chooser = new pqExpressionChooserButton(this, groupName);
  chooser->setObjectName("Chooser");
  chooser->setIcon(QIcon(":/pqWidgets/Icons/pqFavorites.svg"));
  grid->addWidget(chooser, 1, 1);
  auto addToList = new QToolButton(this);
  addToList->setObjectName("AddToList");
  addToList->setIcon(QIcon(":/pqWidgets/Icons/pqSave.svg"));
  addToList->setToolTip(tr("Save expression"));
  grid->addWidget(addToList, 1, 2);
  auto openManager = new QToolButton(this);
  openManager->setObjectName("OpenManager");
  openManager->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced.svg"));
  openManager->setToolTip(tr("Open Expressions Manager"));
  grid->addWidget(openManager, 1, 3);
  auto clearExpression = new QToolButton(this);
  clearExpression->setObjectName("ClearExpression");
  clearExpression->setIcon(QIcon(":/pqWidgets/Icons/pqCancel.svg"));
  clearExpression->setToolTip(tr("Clear expression"));
  grid->addWidget(clearExpression, 1, 4);

  QObject::connect(chooser, SIGNAL(expressionSelected(const QString&)), this->OneLiner,
    SLOT(setPlainText(const QString&)));

  this->connect(addToList, &QToolButton::clicked, [=]() {
    bool saved =
      pqExpressionsManager::addExpressionToSettings(groupName, this->OneLiner->toPlainText());
    statusLine->setText(saved ? tr("Saved! ") : tr("Already Exists"));
    statusLine->show();
    QTimer::singleShot(5000, [=]() { statusLine->hide(); });
  });

  this->connect(openManager, &QToolButton::clicked, [=]() {
    pqExpressionsManagerDialog dialog(pqCoreUtilities::mainWidget(), groupName);
    dialog.setObjectName("ExpressionsManagerDialog");
    QObject::connect(&dialog, &pqExpressionsManagerDialog::expressionSelected, this->OneLiner,
      &QTextEdit::setPlainText);
    dialog.exec();
  });

  this->connect(clearExpression, &QToolButton::clicked, [=]() { this->OneLiner->clear(); });
}

pqOneLinerTextEdit* pqExpressionsWidget::lineEdit()
{
  return this->OneLiner;
}
