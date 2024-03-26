// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqImportReaction_h
#define pqImportReaction_h

#include "pqReaction.h"

class pqProxyWidget;
class pqServer;

/**
 * @ingroup Reactions
 * Reaction for importing views.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqImportReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqImportReaction(QAction* parent);

  /**
   * Opens a file dialog and imports scenes from the selected file.
   */
  void import();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->import(); }

private:
  Q_DISABLE_COPY(pqImportReaction)
};

#endif
