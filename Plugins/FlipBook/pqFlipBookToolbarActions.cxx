// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFlipBookToolbarActions.h"

#include <QApplication>
#include <QSpinBox>
#include <QStyle>

#include "pqFlipBookReaction.h"

//-----------------------------------------------------------------------------
pqFlipBookToolbarActions::pqFlipBookToolbarActions(QWidget* p)
  : QToolBar(tr("Iterative Visibility"), p)
{
  QAction* toggleFlipBookToggleAction = new QAction(
    QIcon(":/pqFlipBook/Icons/pqFlipBook.png"), tr("Toggle iterative visibility"), this);
  toggleFlipBookToggleAction->setCheckable(true);

  QAction* toggleFlipBookPlayAction = new QAction(QIcon(":/pqFlipBook/Icons/pqFlipBookPlay.png"),
    tr("Toggle automatic iterative visibility"), this);

  QAction* toggleFlipBookStepAction = new QAction(QIcon(":/pqFlipBook/Icons/pqFlipBookForward.png"),
    QString("%1 (%2)").arg(tr("Toggle visibility")).arg(tr("shortcut: <SPACE>")), this);

  QSpinBox* playDelay = new QSpinBox(this);
  playDelay->setMaximumSize(70, 30);
  playDelay->setMaximum(1000);
  playDelay->setMinimum(10);
  playDelay->setValue(100);

  this->addAction(toggleFlipBookToggleAction);
  this->addAction(toggleFlipBookPlayAction);
  this->addAction(toggleFlipBookStepAction);
  this->addWidget(playDelay);

  this->setObjectName("FlipBook");

  new pqFlipBookReaction(
    toggleFlipBookToggleAction, toggleFlipBookPlayAction, toggleFlipBookStepAction, playDelay);
}
