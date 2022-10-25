/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsWidget.cxx

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
  grid->setMargin(0);
  grid->setSpacing(0);

  if (!groupName.isEmpty())
  {
    this->setupButtons(groupName);
  }

  this->setLayout(grid);
}

void pqExpressionsWidget::setupButtons(const QString& groupName)
{
  auto grid = dynamic_cast<QGridLayout*>(this->layout());

  this->OneLiner = new pqOneLinerTextEdit(this);
  this->OneLiner->setObjectName("OneLiner");
  grid->addWidget(this->OneLiner, 0, 0, 1, 4);

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
  addToList->setToolTip("Save expression");
  grid->addWidget(addToList, 1, 2);
  auto openManager = new QToolButton(this);
  openManager->setObjectName("OpenManager");
  openManager->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced.svg"));
  openManager->setToolTip("Open Expressions Manager");
  grid->addWidget(openManager, 1, 3);

  QObject::connect(chooser, SIGNAL(expressionSelected(const QString&)), this->OneLiner,
    SLOT(setPlainText(const QString&)));

  this->connect(addToList, &QToolButton::clicked, [=]() {
    bool saved =
      pqExpressionsManager::addExpressionToSettings(groupName, this->OneLiner->toPlainText());
    statusLine->setText(saved ? "Saved! " : "Already Exists");
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
}

pqOneLinerTextEdit* pqExpressionsWidget::lineEdit()
{
  return this->OneLiner;
}
