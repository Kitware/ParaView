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
#ifndef vtkVRStyleGrabNRotateSliceNormal_h
#define vtkVRStyleGrabNRotateSliceNormal_h

#include "vtkSmartPointer.h"
#include "vtkVRStyleGrabNUpdateMatrix.h"
#include <map>
#include <vector>

class vtkSMProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkMatrix4x4;
class vtkSMDoubleVectorProperty;

class vtkVRStyleGrabNRotateSliceNormal : public vtkVRStyleGrabNUpdateMatrix
{
  Q_OBJECT
  typedef vtkVRStyleGrabNUpdateMatrix Superclass;

public:
  vtkVRStyleGrabNRotateSliceNormal(QObject* parent = 0);
  virtual ~vtkVRStyleGrabNRotateSliceNormal();
  virtual bool configure(vtkPVXMLElement* child, vtkSMProxyLocator*);
  virtual vtkPVXMLElement* saveConfiguration() const;
  virtual void HandleButton(const vtkVREventData& data);
  virtual void HandleTracker(const vtkVREventData& data);
  virtual void GetPropertyData();
  virtual void SetProperty();
  virtual bool update();
  bool GetNormalProxyNProperty();

protected:
  std::string NormalProxyName;
  std::string NormalPropertyName;
  bool IsFoundNormalProxyProperty;

  vtkSMProxy* NormalProxy;
  vtkSMDoubleVectorProperty* NormalProperty;
  double Normal[4];

private:
  Q_DISABLE_COPY(vtkVRStyleGrabNRotateSliceNormal)
};

#endif
