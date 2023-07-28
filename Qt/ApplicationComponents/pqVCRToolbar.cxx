// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVCRToolbar.h"
#include "ui_pqVCRToolbar.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqPVApplicationCore.h"
#include "pqUndoStack.h"
#include "pqVCRController.h"

class pqVCRToolbar::pqInternals : public Ui::pqVCRToolbar
{
};

//-----------------------------------------------------------------------------
void pqVCRToolbar::constructor()
{
  this->UI = new pqInternals();
  Ui::pqVCRToolbar& ui = *this->UI;
  ui.setupUi(this);

  pqVCRController* controller = new pqVCRController(this);
  this->Controller = controller;
  QObject::connect(pqPVApplicationCore::instance()->animationManager(),
    SIGNAL(activeSceneChanged(pqAnimationScene*)), controller,
    SLOT(setAnimationScene(pqAnimationScene*)));

  // Ideally pqVCRController needs to be deprecated in lieu of a more
  // action-reaction friendly implementation. But for now, I am simply reusing
  // the old code.
  QObject::connect(ui.actionVCRPlay, SIGNAL(triggered()), controller, SLOT(onPlay()));
  QObject::connect(ui.actionVCRReverse, SIGNAL(triggered()), controller, SLOT(onReverse()));
  QObject::connect(ui.actionVCRFirstFrame, SIGNAL(triggered()), controller, SLOT(onFirstFrame()));
  QObject::connect(
    ui.actionVCRPreviousFrame, SIGNAL(triggered()), controller, SLOT(onPreviousFrame()));
  QObject::connect(ui.actionVCRNextFrame, SIGNAL(triggered()), controller, SLOT(onNextFrame()));
  QObject::connect(ui.actionVCRLastFrame, SIGNAL(triggered()), controller, SLOT(onLastFrame()));
  QObject::connect(ui.actionVCRLoop, SIGNAL(toggled(bool)), controller, SLOT(onLoop(bool)));

  QObject::connect(controller, SIGNAL(enabled(bool)), ui.actionVCRPlay, SLOT(setEnabled(bool)));
  QObject::connect(controller, SIGNAL(enabled(bool)), ui.actionVCRReverse, SLOT(setEnabled(bool)));
  QObject::connect(
    controller, SIGNAL(enabled(bool)), ui.actionVCRFirstFrame, SLOT(setEnabled(bool)));
  QObject::connect(
    controller, SIGNAL(enabled(bool)), ui.actionVCRPreviousFrame, SLOT(setEnabled(bool)));
  QObject::connect(
    controller, SIGNAL(enabled(bool)), ui.actionVCRNextFrame, SLOT(setEnabled(bool)));
  QObject::connect(
    controller, SIGNAL(enabled(bool)), ui.actionVCRLastFrame, SLOT(setEnabled(bool)));
  QObject::connect(controller, SIGNAL(enabled(bool)), ui.actionVCRLoop, SLOT(setEnabled(bool)));
  QObject::connect(
    controller, SIGNAL(timeRanges(double, double)), this, SLOT(setTimeRanges(double, double)));
  QObject::connect(controller, SIGNAL(loop(bool)), ui.actionVCRLoop, SLOT(setChecked(bool)));
  QObject::connect(controller, SIGNAL(playing(bool, bool)), this, SLOT(onPlaying(bool, bool)));

  this->Controller->setAnimationScene(
    pqPVApplicationCore::instance()->animationManager()->getActiveScene());
}

//-----------------------------------------------------------------------------
pqVCRToolbar::~pqVCRToolbar()
{
  delete this->UI;
  this->UI = nullptr;
}

//-----------------------------------------------------------------------------
void pqVCRToolbar::setTimeRanges(double start, double end)
{
  this->UI->actionVCRFirstFrame->setToolTip(tr("First Frame (%1)").arg(start, 0, 'g'));
  this->UI->actionVCRLastFrame->setToolTip(tr("Last Frame (%1)").arg(end, 0, 'g'));
}

//-----------------------------------------------------------------------------
void pqVCRToolbar::onPlaying(bool playing, bool reversed)
{
  QAction* const actn = reversed ? this->UI->actionVCRReverse : this->UI->actionVCRPlay;
  QString actnName = reversed ? tr("Reverse") : tr("Play");
  const char* slotFn = reversed ? SLOT(onReverse()) : SLOT(onPlay());

  // goal of action depends on context. ex: play/reverse vs pause
  if (playing)
  {
    // remap the signals to controller
    disconnect(actn, SIGNAL(triggered()), this->Controller, slotFn);
    connect(actn, SIGNAL(triggered()), this->Controller, SLOT(onPause()));
    actn->setIcon(QIcon(":/pqWidgets/Icons/pqVcrPause.svg"));
    actn->setText(tr("Pa&use"));
  }
  else
  {
    // remap the signals to controller
    connect(actn, SIGNAL(triggered()), this->Controller, slotFn);
    disconnect(actn, SIGNAL(triggered()), this->Controller, SLOT(onPause()));
    actn->setIcon(QIcon(QString(":/pqWidgets/Icons/pqVcr%1.svg").arg(actnName)));
    actn->setText(QString("&%1").arg(actnName));
  }

  // this becomes a behavior.
  // this->Implementation->Core->setSelectiveEnabledState(!playing);
}
