// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDeleteReaction_h
#define pqDeleteReaction_h

#include "pqReaction.h"

class pqPipelineSource;
class pqProxy;

/**
 * @ingroup Reactions
 * Reaction for delete sources (all or selected only).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDeleteReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum DeleteModes
  {
    SELECTED,
    ALL,
    TREE
  };
  /**
   * if delete_all is false, then only selected items will be deleted if
   * possible.
   */
  pqDeleteReaction(QAction* parent, DeleteModes mode = SELECTED);

  static void deleteAll();
  static void deleteSelected();
  static bool canDeleteSelected();
  static void deleteTree();
  static bool canDeleteTree();

  /**
   * Deletes all sources in the set, if possible.
   * All variants of public methods on this class basically call this method
   * with the sources set built up appropriately.
   * The sources set is
   * modified to remove all deleted sources. Any undeleted sources will remain
   * in the set.
   */
  static void deleteSources(const QSet<pqProxy*>& sources);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

  /**
   * Request deletion of a particular source.
   */
  void deleteSource(pqProxy* source);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqDeleteReaction)
  DeleteModes DeleteMode;

  /**
   * Method called just before deleting a source.
   * Updates to the UI before deletion are done here.
   */
  static void aboutToDelete(pqProxy* source);
};

#endif
