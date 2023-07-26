// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRearrangeLayoutReaction_h
#define pqRearrangeLayoutReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction for resizing views inside the active layout.
 * Resize can occur only Vertically, Horizontally or both.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRearrangeLayoutReaction : public pqReaction
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

  pqRearrangeLayoutReaction(Orientation orientation, QAction* parent);
  ~pqRearrangeLayoutReaction() override = default;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqRearrangeLayoutReaction)
  const Orientation ActionOrientation = Orientation::BOTH;
};

#endif
