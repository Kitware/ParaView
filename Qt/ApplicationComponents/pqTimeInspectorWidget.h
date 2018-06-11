/*=========================================================================

   Program: ParaView
   Module:  pqTimeInspectorWidget.h

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
#ifndef pqTimeInspectorWidget_h
#define pqTimeInspectorWidget_h

#include "pqApplicationComponentsModule.h"

#include <QScopedPointer>
#include <QVariant>
#include <QWidget>

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
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimeInspectorWidget : public QWidget
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
  Q_PROPERTY(double sceneCurrentTime READ sceneCurrentTime WRITE setSceneCurrentTime NOTIFY
      sceneCurrentTimeChanged)

  typedef QWidget Superclass;

public:
  pqTimeInspectorWidget(QWidget* parent = 0);
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

  /**
  * Get the current time for the scene.
  */
  double sceneCurrentTime() const;

public slots:
  /**
  * set the active server.
  */
  void setServer(pqServer* server);

  /**
  * set the current time for the scene.
  */
  void setSceneCurrentTime(double);

private slots:
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

signals:
  void dummySignal();
  void sceneCurrentTimeChanged();
  void suppressedTimeSourcesChanged();

private:
  Q_DISABLE_COPY(pqTimeInspectorWidget)

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
  class PropertyLinksConnection;
  class TimeTrack;
};

#endif
