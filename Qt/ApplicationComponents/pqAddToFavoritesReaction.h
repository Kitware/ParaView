// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAddToFavoritesReaction_h
#define pqAddToFavoritesReaction_h

#include "pqReaction.h"

#include "vtkParaViewDeprecation.h" // for deprecation macro

#include <memory>

class pqProxyCategory;
class pqProxyGroupMenuManager;

/**
 * @ingroup Reactions
 * Reaction to add selected filter in favorites
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAddToFavoritesReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  PARAVIEW_DEPRECATED_IN_5_13_0("Favorites are integrated into the categories. Please initialize "
                                "from a pqProxyCategory instead.")
  pqAddToFavoritesReaction(QAction* parent, QVector<QString>& filters);

  pqAddToFavoritesReaction(QAction* parent, pqProxyGroupMenuManager* manager);

  ~pqAddToFavoritesReaction() override;

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
  void onTriggered() override { this->addActiveSourceToFavorites(); }

private:
  Q_DISABLE_COPY(pqAddToFavoritesReaction)

  void addActiveSourceToFavorites();

  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif
