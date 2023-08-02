// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataQueryReaction_h
#define pqDataQueryReaction_h

#include "pqReaction.h"

/**
 * @class pqDataQueryReaction
 * @brief reaction to bring up "find data" panel.
 * @ingroup Reactions
 *
 * pqDataQueryReaction is the reaction that brings to forefront the
 * `FIND_DATA_PANEL` registered with the application, if any.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDataQueryReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqDataQueryReaction(QAction* parent);
  ~pqDataQueryReaction() override;

  /**
   * Show the query panel for querying the data from the active source.
   */
  void showQueryPanel();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqDataQueryReaction::showQueryPanel(); }

private:
  Q_DISABLE_COPY(pqDataQueryReaction)
};

#endif
