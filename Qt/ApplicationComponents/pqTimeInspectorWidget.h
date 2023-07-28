// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimeInspectorWidget_h
#define pqTimeInspectorWidget_h

#include "pqApplicationComponentsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QScopedPointer>
#include <QVariant>
#include <QWidget>

class pqAnimationScene;
class pqAnimationTrack;
class pqServer;
class pqServerManagerModelItem;
class vtkSMProxy;

/**
 * pqTimeInspectorWidget is a widget that allows the user to look at all sources that
 * provide time. The animation timesteps as well as the timesteps provided by
 * each of the sources can be seen in this widget. This widget allows the user
 * to change 2 things: whether to suppress timesteps from a source, and the
 * current time. Everything else is read-only.
 *
 * The implementation relies on linking (using pqPropertyLinks) sm-properties on TimeKeeper
 * and AnimationScene proxy for the active session to qt-properties on this
 * widget.
 *
 * To use this widget since set the animation scene using setAnimationScene().
 * The widget takes care of the rest. One typically never needs to use any of
 * the other public API which is internally "linked" to the properties on the
 * animation scene proxy.
 */

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "Use `pqTimeManagerWidget` instead") PQAPPLICATIONCOMPONENTS_EXPORT pqTimeInspectorWidget
  : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double sceneStartTime READ sceneStartTime WRITE setSceneStartTime)
  Q_PROPERTY(double sceneEndTime READ sceneEndTime WRITE setSceneEndTime)
  Q_PROPERTY(QString scenePlayMode READ scenePlayMode WRITE setScenePlayMode)
  Q_PROPERTY(QList<QVariant> sceneTimeSteps READ sceneTimeSteps WRITE setSceneTimeSteps)
  Q_PROPERTY(int sceneNumberOfFrames READ sceneNumberOfFrames WRITE setSceneNumberOfFrames)
  Q_PROPERTY(QList<QVariant> timeSources READ timeSources WRITE setTimeSources)
  Q_PROPERTY(
    QList<QVariant> suppressedTimeSources READ suppressedTimeSources WRITE setSuppressedTimeSources)

  typedef QWidget Superclass;

public:
  pqTimeInspectorWidget(QWidget* parent = nullptr);
  ~pqTimeInspectorWidget() override;

  /**
   * access the server being reflected on the widget.
   */
  pqServer* server() const;

  /**
   * Get/set the start time for the scene.
   */
  double sceneStartTime() const;
  void setSceneStartTime(double);

  /**
   * Get/Set the end time for the scene.
   */
  double sceneEndTime() const;
  void setSceneEndTime(double);

  /**
   * Get/set the play-mode. Used to control how the global ticks are shown
   * and how the current time slider snaps on interaction.
   */
  QString scenePlayMode() const;
  void setScenePlayMode(const QString&);

  /**
   * Get/Set the timesteps in the scene. This is timesteps known to the active
   * sessions TimeKeeper proxy.
   */
  QList<QVariant> sceneTimeSteps() const;
  void setSceneTimeSteps(const QList<QVariant>& val);

  /**
   * Get/Set the number of frames. This is used when the playmode is set to
   * sequence.
   */
  int sceneNumberOfFrames() const;
  void setSceneNumberOfFrames(int val);

  /**
   * Get/Set the proxies that are registered with the TimeKeeper as the
   * time sources. Not all proxies in this list may have time. Hence,
   * setTimeSources() needs to cull the list appropriately.
   */
  QList<QVariant> timeSources() const;
  void setTimeSources(const QList<QVariant>& proxies);

  /**
   * Get/Set the list of proxies which are to be marked as "suppressed" i.e.
   * their timesteps are ignored by the animation playback.
   */
  QList<QVariant> suppressedTimeSources() const;
  void setSuppressedTimeSources(const QList<QVariant>& proxies);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * set the active server.
   */
  void setServer(pqServer* server);

  /**
   * set the current time for the scene.
   */
  void setSceneCurrentTime(double);

  void setAnimationScene(pqAnimationScene* scene);

protected Q_SLOTS:
  /**
   * called when scene time changes
   */
  void updateSceneTime();

private Q_SLOTS:
  /**
   * Updates the panel based on the scene on this->server().
   */
  void updateScene();

  /**
   * handle change in proxy names.
   */
  void handleProxyNameChanged(pqServerManagerModelItem*);

  /**
   * toggle changing of proxies being suppressed from the animation.
   */
  void toggleTrackSuppression(pqAnimationTrack*);

  void generalSettingsChanged();

Q_SIGNALS:
  void dummySignal();
  void suppressedTimeSourcesChanged();

private:
  Q_DISABLE_COPY(pqTimeInspectorWidget)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
  class PropertyLinksConnection;
  class TimeTrack;
};

#endif
