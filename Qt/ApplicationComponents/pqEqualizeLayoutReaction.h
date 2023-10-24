// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEqualizeLayoutReaction_h
#define pqEqualizeLayoutReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for resizing views inside the active layout.
 * Resize can occur only Vertically, Horizontally or both.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEqualizeLayoutReaction : public pqReaction
{
  Q_OBJECT
  using Superclass = pqReaction;

public:
  enum class Orientation
  {
    HORIZONTAL,
    VERTICAL,
    BOTH
  };

  pqEqualizeLayoutReaction(Orientation orientation, QAction* parent);
  ~pqEqualizeLayoutReaction() override = default;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEqualizeLayoutReaction)
  const Orientation ActionOrientation = Orientation::BOTH;
};

#endif
