/*=========================================================================

   Program: ParaView
   Module:    pqSelectionToolbar.cxx

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
#include "pqSelectionToolbar.h"
#include "ui_pqSelectionToolbar.h"

#include "pqRubberBandHelper.h"
#include "pqActiveObjects.h"

#include <QActionGroup>

class pqSelectionToolbar::pqUI : public Ui::pqSelectionToolbar
{
};

//-----------------------------------------------------------------------------
void pqSelectionToolbar::constructor()
{
  this->SelectionHelper = new pqRubberBandHelper(this);
  // Set up connection with selection helpers for all views.
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this->SelectionHelper, SLOT(setView(pqView*)));

  this->UI = new pqUI();
  Ui::pqSelectionToolbar &ui = *this->UI;
  ui.setupUi(this);

  // Setup the 'modes' so that they are exclusively selected
  QActionGroup *modeGroup = new QActionGroup(this);
  modeGroup->addAction(ui.actionMoveMode);
  modeGroup->addAction(ui.actionSelectionMode);
  modeGroup->addAction(ui.actionSelectSurfacePoints);
  modeGroup->addAction(ui.actionSelect_Frustum);
  modeGroup->addAction(ui.actionSelectFrustumPoints);
  modeGroup->addAction(ui.actionSelect_Block);
  modeGroup->addAction(ui.actionPickObject);

  // Set up selection buttons.
  QObject::connect(
    ui.actionMoveMode, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(endSelection()));

  // 3d Selection Modes
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enableSurfaceSelection(bool)),
    ui.actionSelectionMode, SLOT(setEnabled(bool)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enableSurfacePointsSelection(bool)),
    ui.actionSelectSurfacePoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enableFrustumSelection(bool)),
    ui.actionSelect_Frustum, SLOT(setEnabled(bool)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enableFrustumPointSelection(bool)),
    ui.actionSelectFrustumPoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enableBlockSelection(bool)),
    ui.actionSelect_Block, SLOT(setEnabled(bool)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(enablePick(bool)),
    ui.actionPickObject, SLOT(setEnabled(bool)));

  QObject::connect(
    ui.actionSelectionMode, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginSurfaceSelection()));
  QObject::connect(
    ui.actionSelectSurfacePoints, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginSurfacePointsSelection()));
  QObject::connect(
    ui.actionSelect_Frustum, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginFrustumSelection()));
  QObject::connect(
    ui.actionSelectFrustumPoints, SIGNAL(triggered()),
    this->SelectionHelper, SLOT(beginFrustumPointsSelection()));
  QObject::connect(
    ui.actionSelect_Block, SIGNAL(triggered()),
    this->SelectionHelper,
    SLOT(beginBlockSelection()));
  QObject::connect(
    ui.actionPickObject, SIGNAL(triggered()),
    this->SelectionHelper,
    SLOT(beginPick()));

  QObject::connect(
    this->SelectionHelper,
    SIGNAL(selectionModeChanged(int)),
    this, SLOT(onSelectionModeChanged(int)));
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(interactionModeChanged(bool)),
    ui.actionMoveMode, SLOT(setChecked(bool)));

  // When a selection is marked, we revert to interaction mode.
  QObject::connect(
    this->SelectionHelper,
    SIGNAL(selectionFinished(int, int, int, int)),
    this->SelectionHelper, SLOT(endSelection()));
}

//-----------------------------------------------------------------------------
pqSelectionToolbar::~pqSelectionToolbar()
{
  delete this->UI;
  this->UI = NULL;
}

//-----------------------------------------------------------------------------
void pqSelectionToolbar::onSelectionModeChanged(int mode)
{
  if (this->isEnabled())
    {
    switch (mode)
      {
    case pqRubberBandHelper::SELECT://surface selection
      this->UI->actionSelectionMode->setChecked(true);
      break;

    case pqRubberBandHelper::SELECT_POINTS: //surface selection
      this->UI->actionSelectSurfacePoints->setChecked(true);
      break;

    case pqRubberBandHelper::FRUSTUM:
      this->UI->actionSelect_Frustum->setChecked(true);
      break;

    case pqRubberBandHelper::FRUSTUM_POINTS:
      this->UI->actionSelectFrustumPoints->setChecked(true);
      break;

    case pqRubberBandHelper::BLOCKS:
      this->UI->actionSelect_Block->setChecked(true);
      break;

    case pqRubberBandHelper::INTERACT:
      this->UI->actionMoveMode->setChecked(true);
      break;

    default:
      break;
      }
    }
}
