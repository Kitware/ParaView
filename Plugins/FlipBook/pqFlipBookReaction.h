// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqFlipBookReaction_h
#define pqFlipBookReaction_h

#include "pqReaction.h"

#include <QPointer>
#include <QSpinBox>

class pqDataRepresentation;
class pqModalShortcut;
class pqPipelineModel;
class pqRepresentation;
class pqView;

/**
 * @ingroup Reactions
 * pqFlipBookReaction is a reaction to iterative visibility button.
 */
class pqFlipBookReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqFlipBookReaction(QAction* parent, QAction* playAction, QAction* stepAction, QSpinBox* autoVal);
  ~pqFlipBookReaction() override = default;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected Q_SLOTS:
  /**
   * Called when the action is toggled.
   */
  void onToggled(bool checked);

  /**
   * Called when the play action is toggled.
   */
  void onPlay();

  /**
   * Called when the step action is clicked.
   */
  void onStepClicked();

  /**
   * Update visibility of data representations based on current index
   */
  void updateVisibility();

  /**
   * Triggered when a data representation is added or removed
   */
  void representationsModified(pqRepresentation*);

  void representationVisibilityChanged(pqRepresentation*, bool);

protected: // NOLINT(readability-redundant-access-specifiers)
  bool hasEnoughVisibleRepresentations();

  int getNumberOfVisibleRepresentations();

  void parseVisibleRepresentations();
  void parseVisibleRepresentations(pqPipelineModel*, QModelIndex);

  void onPlay(bool play);

private:
  Q_DISABLE_COPY(pqFlipBookReaction);

  QPointer<QAction> PlayAction;
  QPointer<QAction> StepAction;
  QPointer<QSpinBox> PlayDelay;

  QPointer<pqView> View;
  QTimer* Timer;
  QPointer<pqModalShortcut> StepActionMode;

  QList<QPointer<pqDataRepresentation>> VisibleRepresentations;
  int VisibilityIndex;
};

#endif
