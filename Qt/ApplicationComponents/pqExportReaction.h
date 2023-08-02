// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExportReaction_h
#define pqExportReaction_h

#include "pqReaction.h"

class pqProxyWidget;
class pqView;

/**
 * @ingroup Reactions
 * Reaction for exporting a view. Uses pqViewExporterManager for actual
 * exporting.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqExportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqExportReaction(QAction* parent);

  /**
   * Exports the current view. Returns the exported filename of successful
   * export, otherwise returns an empty QString.
   */
  QString exportActiveView();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->exportActiveView(); }

private:
  pqView* ConnectedView;

  Q_DISABLE_COPY(pqExportReaction)
};

#endif
