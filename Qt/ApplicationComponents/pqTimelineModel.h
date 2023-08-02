// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimelineModel_h
#define pqTimelineModel_h

#include "pqApplicationComponentsModule.h"

#include <QStandardItemModel>

#include <memory> // for std::unique_ptr

namespace pqTimelineItemRole
{
enum Role
{
  TYPE = Qt::UserRole + 1,
  PROXY,
  TIMES,
  SOURCE_TIME,
  REGISTRATION_NAME,
  LABELS
};
};

namespace pqTimelineColumn
{
enum Column
{
  NAME = 0,
  TIMELINE,
  WIDGET,
  COUNT
};
};

namespace pqTimelineTrack
{
enum TrackType
{
  NONE = 0,
  TIME,
  SOURCE,
  ANIMATION_HEADER,
  ANIMATION
};
};

/**
 * pqTimelineModel is a standard item model for ParaView timelines,
 * intended to be used with pqTimelineView, and mainly through pqTimelineWidget.
 *
 * A timeline correspond to any element containing a list of meaningful times,
 * as temporal sources and animation cue (see pqTimelineWidget).
 *
 * This is a tree-like structure with only two levels.
 * Each model row is known as a "Track", than can be of several type (see pqTimelineTrack enum).
 * SOURCE and ANIMATION tracks are grouped under a parent track, resp. TIME and ANIMATION_HEADER
 * making it easier to have dedicated code path.
 *
 * One column contains the timeline itself, while others contains associated data
 * such as name and custom widget. See pqTimelineColumn enum.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelineModel : public QStandardItemModel
{
  Q_OBJECT
  typedef QStandardItemModel Superclass;

public:
  pqTimelineModel(QObject* parent = nullptr);
  ~pqTimelineModel() override;

  /**
   * Create items under appropriate parent, and set some data on it.
   *
   * Parent is computed depending on type:
   * SOURCE goes under TIME, ANIMATION under ANIMATION_HEADER. Themself are under the root.
   *
   * @param type is always stored under `pqTimelineItemRole::TYPE` data role. By default
   * @param name is stored under `pqTimelineItemRole::REGISTRATIONNAME` but can be overriden
   * by additionalData.
   * @param additionalData is a map of data associated to their role.
   */
  QList<QStandardItem*> createRow(
    pqTimelineTrack::TrackType type, const QString& name, QMap<int, QVariant> additionalData = {});

  /**
   * Clear rows of given type.
   */
  void clearRows(pqTimelineTrack::TrackType type);

  /**
   * Remove row under type with given registrationName.
   */
  void deleteRow(pqTimelineTrack::TrackType type, const QString& registrationName);

  /**
   * Override to handle checkstate.
   */
  bool setData(
    const QModelIndex& index, const QVariant& value, int role = Qt::DisplayRole) override;

  /**
   * Set/Get enable state of the row.
   */
  ///@{
  void setRowEnabled(
    pqTimelineTrack::TrackType type, bool enabled, const QString& name = QString());
  bool isRowEnabled(pqTimelineTrack::TrackType type, const QString& name = QString());
  void toggleRow(pqTimelineTrack::TrackType type, const QString& name);
  void setRowsEnabled(pqTimelineTrack::TrackType type, bool enabled);
  ///@}

  /**
   * Returns the row list of given type.
   */
  QList<QList<QStandardItem*>> rows(pqTimelineTrack::TrackType type);

  /**
   * Returns the unchecked row list of given type.
   */
  QList<QList<QStandardItem*>> uncheckedRows(pqTimelineTrack::TrackType type);

Q_SIGNALS:
  void sourceStateChanged();
  void animationStateChanged();

private:
  Q_DISABLE_COPY(pqTimelineModel)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
