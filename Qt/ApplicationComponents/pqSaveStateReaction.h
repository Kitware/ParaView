// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveStateReaction_h
#define pqSaveStateReaction_h

#include "pqReaction.h"
#include "vtkType.h" // needed for vtkTypeUInt32

class pqServer;

/**
 * @ingroup Reactions
 * Reaction for saving state file.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveStateReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSaveStateReaction(QAction* parent);
  ~pqSaveStateReaction() override = default;

  /**
   * Open File dialog, with the active server, in order to choose the location and the type of
   * the state file that should be saved
   * Returns true if the user selected a file to save and false if they canceled the dialog
   */
  static bool saveState();

  /**
   * Open File dialog, with the specified server, in order to choose the location and the type of
   * the state file that should be saved
   * If the server is nullptr, files are browsed locally else remotely and optionally locally
   * Returns true if the user selected a file to save and false if they canceled the dialog
   */
  static bool saveState(pqServer* server);

  /**
   * Saves the state file.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   * Return true if the operation succeeded otherwise return false.
   */
  static bool saveState(
    const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

  /**
   * Saves the state file as a python state.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   * Return true if the operation succeeded otherwise return false.
   */
  static bool savePythonState(
    const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveStateReaction::saveState(); }

private:
  Q_DISABLE_COPY(pqSaveStateReaction)
};

#endif
