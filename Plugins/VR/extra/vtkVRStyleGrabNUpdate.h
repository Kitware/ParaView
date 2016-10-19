/*=========================================================================

   Program: ParaView
   Module:    vtkVRStyleGrabNUpdate.h

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
#ifndef vtkVRStyleGrabNUpdate_h_
#define vtkVRStyleGrabNUpdate_h_

#include "vtkVRStyleTracking.h"

class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkTransform;
struct vtkVREventData;

class vtkVRStyleGrabNUpdate : public vtkVRStyleTracking
{
  Q_OBJECT
  typedef vtkVRStyleTracking Superclass;

public:
  vtkVRStyleGrabNUpdate(QObject* parent);
  ~vtkVRStyleGrabNUpdate();
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);
  virtual vtkPVXMLElement* saveConfiguration() const;

protected:
  virtual void HandleButton(const vtkVREventData& data);
  virtual void HandleTracker(const vtkVREventData& data);
  virtual bool GetProxyNProperty();
  virtual void GetProperty() {}
protected:
  vtkSMProxy* Proxy;
  vtkSMDoubleVectorProperty* Property;
  std::string ProxyName;
  std::string PropertyName;
  bool IsFoundProxyProperty;
  std::string Button;
  bool Enabled;
  bool IsInitialRecorded;
  vtkTransform* InitialInvertedPose;
};

#endif // vtkVRStyleGrabNUpdate.h_
