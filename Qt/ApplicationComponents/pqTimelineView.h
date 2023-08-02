// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimelineView_h
#define pqTimelineView_h

#include "pqApplicationComponentsModule.h"

#include "pqAnimationCue.h" // for pqAnimatedProperty struct
#include "pqTimelineModel.h"

#include <QTreeView>

/**
 * pqTimelineView holds the timeline views for a pqTimelineModel.
 *
 * This is based on a QTreeView, mainly to be able to collapse some sections.
 * It contains some widgets to handle animation cues (creation and deletion).
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelineView : public QTreeView
{
  Q_OBJECT
  typedef QTreeView Superclass;

public:
  pqTimelineView(QWidget* parent = nullptr);
  ~pqTimelineView() override;

  /**
   * Return inner model as a pqTimelineModel.
   */
  pqTimelineModel* timelineModel();

  /**
   * Enable the "add track" button.
   */
  void enableTrackCreationWidget(bool enable);

  /**
   * Emits validateTrackRequested()
   */
  void validateTrackCreationWidget();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * Create row widgets, such as the Delete button in animation track.
   */
  void createRowWidgets(const QModelIndexList& indexes);

  /**
   * Force repainting timelines items.
   */
  void updateTimelines();

Q_SIGNALS:
  /**
   * Emited on Proxy or Property box changes.
   */
  void validateTrackRequested(const pqAnimatedPropertyInfo& prop);

  /**
   * Emited on "add track" button clicked.
   */
  void newTrackRequested(const pqAnimatedPropertyInfo& prop);

  /**
   * Emited on "delete track" button clicked, with the pqAnimationCue registration name as argument.
   */
  void deleteTrackRequested(const QString& registrationName);

  /**
   * Emited on double click on an editable track.
   */
  void editTrackRequested(const QString& registrationName);

  /**
   * Emited on "reset start end" button clicked.
   */
  void resetStartEndTimeRequested();

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
