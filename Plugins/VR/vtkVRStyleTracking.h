/*=========================================================================

   Program: ParaView
   Module:    vtkVRStyleTracking.h

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
#ifndef __vtkVRStyleTracking_h_
#define __vtkVRStyleTracking_h_

#include "vtkVRInteractorStyle.h"
#include "vtkWeakPointer.h"

class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREventData;

class vtkVRStyleTracking : public vtkVRInteractorStyle
{
  Q_OBJECT
  typedef vtkVRInteractorStyle Superclass;
public:
  vtkVRStyleTracking(QObject* parent);
  ~vtkVRStyleTracking();

  // Specify the proxy and property to control. The property needs to have 16
  // elements and must be a numerical property.
  void setControlledProxy(vtkSMProxy* proxy);
  vtkSMProxy* controlledProxy() const;

  void setControlledPropertyName(const QString& pname)
    { this->ControlledPropertyName = pname; }
  const QString& controlledPropertyName() const
    { return this->ControlledPropertyName; }


  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);
  virtual vtkPVXMLElement* saveConfiguration() const;

protected:
  virtual void handleTracker( const vtkVREventData& data );

protected:
  QString TrackerName;
  QString ControlledPropertyName;
  vtkWeakPointer<vtkSMProxy> ControlledProxy;
};

#endif //__vtkVRStyleTracking.h_
