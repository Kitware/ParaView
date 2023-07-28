// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqViewStreamingBehavior_h
#define pqViewStreamingBehavior_h

#include "pqApplicationComponentsModule.h"
#include "pqTimer.h"
#include <QObject>

class pqView;
class vtkObject;

/**
 * @ingroup Behaviors
 * pqViewStreamingBehavior is used to manage rendering of render-view when
 * streaming is enabled.
 *
 * pqViewStreamingBehavior listens to updates on render-views when
 * vtkPVView::GetEnableStreaming() returns true and periodically calls
 * vtkSMRenderViewProxy::StreamingUpdate() on the view until the view reports
 * there is no more data to be streamed. The periodic updates resume after the
 * next time the view updates since the view now may have newer data that needs
 * to be streamed.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewStreamingBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqViewStreamingBehavior(QObject* parent = nullptr);
  ~pqViewStreamingBehavior() override;

  /**
   * This API is for testing purposes. It enables pausing/stepping/resuming
   * automatic updates.
   */
  void stopAutoUpdates();
  void resumeAutoUpdates();
  void triggerSingleUpdate();

protected Q_SLOTS:
  void onViewAdded(pqView*);
  void onViewUpdated(vtkObject*, unsigned long, void*);
  void onTimeout();

private:
  Q_DISABLE_COPY(pqViewStreamingBehavior)
  pqTimer Timer;
  int Pass;
  bool DelayUpdate;
  bool DisableAutomaticUpdates;

  void onStartInteractionEvent();
  void onEndInteractionEvent();
};

#endif
