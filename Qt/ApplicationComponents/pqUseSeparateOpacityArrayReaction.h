// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqUseSeparateOpacityArrayReaction_h
#define pqUseSeparateOpacityArrayReaction_h

#include "pqPropertyLinks.h" // For Links
#include "pqReaction.h"

#include <QPointer>

class pqDataRepresentation;

/**
 * @ingroup Reactions
 * Reaction to toggle the use of separated array for opacity in a representation
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqUseSeparateOpacityArrayReaction : public pqReaction
{
  Q_OBJECT
  using Superclass = pqReaction;

public:
  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqUseSeparateOpacityArrayReaction(QAction* parent, bool track_active_objects = true);
  ~pqUseSeparateOpacityArrayReaction() override;

  /**
   * Returns the representation currently being used by the reaction.
   */
  pqDataRepresentation* representation() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the active representation.
   */
  void setRepresentation(pqDataRepresentation*);

protected Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

private:
  Q_DISABLE_COPY(pqUseSeparateOpacityArrayReaction)

  pqPropertyLinks Links;
  QPointer<pqDataRepresentation> CachedRepresentation;
  bool TrackActiveObjects;
};

#endif
