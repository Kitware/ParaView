// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqUse2DTransferFunctionReaction_h
#define pqUse2DTransferFunctionReaction_h

#include "pqPropertyLinks.h" // For Links
#include "pqReaction.h"

#include <QPointer>

class pqDataRepresentation;

/**
 * @ingroup Reactions
 * Reaction to toggle the use of 2D transfer function for a representation
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqUse2DTransferFunctionReaction : public pqReaction
{
  Q_OBJECT
  using Superclass = pqReaction;

public:
  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqUse2DTransferFunctionReaction(QAction* parent, bool track_active_objects = true);
  ~pqUse2DTransferFunctionReaction() override;

  /**
   * Returns the representation currently being used by the reaction.
   */
  pqDataRepresentation* representation() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Set the active representation.
   */
  void setRepresentation(pqDataRepresentation*);
  void setActiveRepresentation();
  ///@}

protected Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqUse2DTransferFunctionReaction)

  pqPropertyLinks Links;
  QPointer<pqDataRepresentation> Representation;
  bool TrackActiveObjects;
};

#endif
