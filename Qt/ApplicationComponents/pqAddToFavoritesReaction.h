// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAddToFavoritesReaction_h
#define pqAddToFavoritesReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to add selected filter in favorites
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAddToFavoritesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqAddToFavoritesReaction(QAction* parent, QVector<QString>& filters);

  /**
   * Add filter in favorites.
   */
  static void addToFavorites(QAction* parent);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call
   * this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqAddToFavoritesReaction::addToFavorites(this->parentAction()); }

private:
  Q_DISABLE_COPY(pqAddToFavoritesReaction)

  QVector<QString> Filters;
};

#endif
