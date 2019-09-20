/*=========================================================================

   Program: ParaView
   Module:    pqFlipBookToolbarActions.cxx

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

#include "pqFlipBookToolbarActions.h"

#include <QApplication>
#include <QSpinBox>
#include <QStyle>

#include "pqFlipBookReaction.h"

//-----------------------------------------------------------------------------
pqFlipBookToolbarActions::pqFlipBookToolbarActions(QWidget* p)
  : QToolBar("Iterative Visibility", p)
{
  QAction* toggleFlipBookToggleAction =
    new QAction(QIcon(":/pqFlipBook/Icons/pqFlipBook.png"), "Toggle iterative visibility", this);
  toggleFlipBookToggleAction->setCheckable(true);

  QAction* toggleFlipBookPlayAction = new QAction(
    QIcon(":/pqFlipBook/Icons/pqFlipBookPlay.png"), "Toggle automatic iterative visibility", this);

  QAction* toggleFlipBookStepAction = new QAction(QIcon(":/pqFlipBook/Icons/pqFlipBookForward.png"),
    "Toggle visibility (shortcut: <SPACE>)", this);

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
