/*=========================================================================

   Program: ParaView
   Module:  pqTimelineItemDelegate.h

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
#ifndef pqTimelineItemDelegate_h
#define pqTimelineItemDelegate_h

#include "pqApplicationComponentsModule.h"

#include <QStyledItemDelegate>

#include <memory> // for std::unique_ptr

class pqAnimationScene;

class pqTimelinePainter;

/**
 * pqTimelineItemDelegate draws timeline in cells and add some associated widgets.
 *
 * This handles connection with the active scene to keep the timelines up to date.
 *
 * Start and End scene times are editable and can be locked with dedicated button,
 * integrated in the pqTimelineTrack::TIME item.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelineItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT
  typedef QStyledItemDelegate Superclass;

public:
  pqTimelineItemDelegate(QObject* parent = nullptr, QWidget* parentWidget = nullptr);
  ~pqTimelineItemDelegate() override;

  /**
   * Render the timelines.
   * Depends on the type (time, animation) stored in a custom item role
   */
  void paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Reimplemented to change row height.
   */
  QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  /**
   * Reimplemented to handle mouse events.
   * So we can handle clicking on the timeline to edit start / end time.
   */
  bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
    const QModelIndex& index) override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the active scene connections.
   */
  void setActiveSceneConnections(pqAnimationScene* scene);

  /**
   * Update cache of start/end time of the scene.
   */
  void updateSceneTimeRange();

Q_SIGNALS:
  void needsRepaint();

private:
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;

  std::unique_ptr<pqTimelinePainter> TimelinePainter;
};

#endif
