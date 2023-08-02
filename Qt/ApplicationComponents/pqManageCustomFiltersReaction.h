// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqManageCustomFiltersReaction_h
#define pqManageCustomFiltersReaction_h

#include "pqMasterOnlyReaction.h"

class pqCustomFilterManagerModel;

/**
 * @ingroup Reactions
 * Reaction for showing the custom-filter manager dialog.
 * For now, this also manages loading and saving of custom filters in the
 * application settings. We may want to move that code to a separate behavior.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqManageCustomFiltersReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqManageCustomFiltersReaction(QAction* parentObject);

  /**
   * Pops up the manage custom filters dialog.
   */
  void manageCustomFilters();

protected:
  void onTriggered() override { this->manageCustomFilters(); }

private:
  Q_DISABLE_COPY(pqManageCustomFiltersReaction)
  pqCustomFilterManagerModel* Model;
};

#endif
