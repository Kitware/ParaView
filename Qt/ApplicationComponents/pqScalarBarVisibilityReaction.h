// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScalarBarVisibilityReaction_h
#define pqScalarBarVisibilityReaction_h

#include "pqReaction.h"
#include <QPointer>

class pqDataRepresentation;
class pqTimer;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Reaction to toggle scalar bar visibility.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqScalarBarVisibilityReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqScalarBarVisibilityReaction(QAction* parent, bool track_active_objects = true);
  ~pqScalarBarVisibilityReaction() override;

  /**
   * Returns the representation currently being used by the reaction.
   */
  pqDataRepresentation* representation() const;

  /**
   * Returns the scalar bar for the current representation, if any.
   */
  vtkSMProxy* scalarBarProxy() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the active representation.
   */
  void setRepresentation(pqDataRepresentation*);

  /**
   * set scalar bar visibility.
   */
  void setScalarBarVisibility(bool visible);

protected Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->setScalarBarVisibility(this->parentAction()->isChecked()); }

private:
  Q_DISABLE_COPY(pqScalarBarVisibilityReaction)

  bool BlockSignals;
  bool TrackActiveObjects;
  QPointer<pqDataRepresentation> CachedRepresentation;
  QPointer<QObject> CachedScalarBar;
  QPointer<QObject> CachedView;
  pqTimer* Timer;
};

#endif
