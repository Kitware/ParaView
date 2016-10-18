/*=========================================================================

   Program: ParaView
   Module:    vtkVRHeadTrackingStyle.h

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
#ifndef vtkVRHeadTrackingStyle_h_
#define vtkVRHeadTrackingStyle_h_

#include "vtkVRInteractorStyle.h"

class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREventData;

/// This is demonstration of how subclasses for vtkVRInteractorStyle can be
/// implemented.
class vtkVRHeadTrackingStyle : public vtkVRInteractorStyle
{
  Q_OBJECT
  typedef vtkVRInteractorStyle Superclass;

public:
  vtkVRHeadTrackingStyle(QObject* parent);
  ~vtkVRHeadTrackingStyle();

  /// called to handle an event. If the style does not handle this event or
  /// handles it but does not want to stop any other handlers from handling it
  /// as well, they should return false. Other return true. Returning true
  /// indicates that vtkVRQueueHandler the event has been "consumed".
  virtual bool handleEvent(const vtkVREventData& data);

  /// called to update all the remote vtkObjects and perhaps even to render.
  /// Typically processing intensive operations go here. The method should not
  /// be called from within the handler and is reserved to be called from an
  /// external interaction style manager.
  virtual bool update();

protected:
  void HandleButton(const vtkVREventData& data);
  void HandleAnalog(const vtkVREventData& data);
  void HandleTracker(const vtkVREventData& data);
  bool GetHeadPoseProxyNProperty();
  bool SetHeadPoseProperty(const vtkVREventData& data);

protected:
  vtkSMRenderViewProxy* Proxy;
  vtkSMDoubleVectorProperty* Property;
  bool IsFoundProxyProperty;
};

#endif // vtkVRHeadTrackingStyle.h_
