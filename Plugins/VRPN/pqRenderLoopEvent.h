/*=========================================================================

   Program: ParaView
   Module:    ParaViewVRPN.h

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
#ifndef __pqRenderLoopEvent_h_
#define __pqRenderLoopEvent_h_

#include <QObject>
#include "vtkVRQueue.h"

class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;

class pqRenderLoopEvent : public QObject
{
Q_OBJECT

public:
  pqRenderLoopEvent();
  ~pqRenderLoopEvent();

  // Description:
  // Sets the Event Queue into which the vrpn data needs to be written
  void SetQueue( vtkVRQueue* queue );

protected slots:
  void Handle();
  void HandleButton ( const vtkVREventData& data );
  void HandleAnalog ( const vtkVREventData& data );
  void HandleTracker( const vtkVREventData& data );
  bool GetHeadPoseProxyNProperty( vtkSMRenderViewProxy** proxy,
                                  vtkSMDoubleVectorProperty** prop);
  bool SetHeadPoseProperty( vtkVREventData data );
  bool UpdateNRenderWithHeadPose();
  void HandleSpaceNavigatorAnalog( const vtkVREventData& data );
protected:
  vtkVRQueue *EventQueue;
};

#endif //__pqRenderLoopEvent.h_
