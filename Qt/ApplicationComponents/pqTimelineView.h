/*=========================================================================

   Program: ParaView
   Module:  pqTimelineView.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
