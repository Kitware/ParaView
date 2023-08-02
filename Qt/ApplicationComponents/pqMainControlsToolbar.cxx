// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMainControlsToolbar.h"
#include "ui_pqMainControlsToolbar.h"

#include "pqAutoApplyReaction.h"
#include "pqCatalystExportReaction.h"
#include "pqDataQueryReaction.h"
#include "pqDeleteReaction.h"
#include "pqLoadDataReaction.h"
#include "pqLoadPaletteReaction.h"
#include "pqSaveDataReaction.h"
#include "pqSaveExtractsReaction.h"
#include "pqSaveStateReaction.h"
#include "pqServerConnectReaction.h"
#include "pqServerDisconnectReaction.h"
#include "pqUndoRedoReaction.h"

#include <QToolButton>

//-----------------------------------------------------------------------------
void pqMainControlsToolbar::constructor()
{
  Ui::pqMainControlsToolbar ui;
  ui.setupUi(this);
  new pqLoadDataReaction(ui.actionOpenData);
  new pqSaveDataReaction(ui.actionSaveData);
  new pqServerConnectReaction(ui.actionServerConnect);
  new pqServerDisconnectReaction(ui.actionServerDisconnect);
  new pqDeleteReaction(ui.actionDelete, pqDeleteReaction::DeleteModes::ALL);
  new pqUndoRedoReaction(ui.actionUndo, true);
  new pqUndoRedoReaction(ui.actionRedo, false);
  new pqAutoApplyReaction(ui.actionAutoApply);
  new pqDataQueryReaction(ui.actionQuery);
  new pqLoadPaletteReaction(ui.actionLoadPalette);
  new pqSaveStateReaction(ui.actionSaveState);
  new pqSaveExtractsReaction(ui.actionGenerateExtracts);
#if VTK_MODULE_ENABLE_ParaView_pqPython
  new pqCatalystExportReaction(ui.actionSaveCatalystState);
#else
  ui.actionSaveCatalystState->setEnabled(false);
#endif

  QToolButton* tb = qobject_cast<QToolButton*>(this->widgetForAction(ui.actionLoadPalette));
  if (tb)
  {
    tb->setPopupMode(QToolButton::InstantPopup);
  }
}
