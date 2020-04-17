/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
  pqViewStreamingBehavior(QObject* parent = 0);
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
