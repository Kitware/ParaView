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
#ifndef pqIntegrationModelHelperWidget_h
#define pqIntegrationModelHelperWidget_h

#include "pqPropertyWidget.h"
#include "vtkEventQtSlotConnect.h" // For the connector
#include "vtkNew.h"                // For the connector

class vtkSMProxyProperty;

/// Base class to represent the "ArraysToGenerate" Property.
/// pqIntegrationModelHelperWidget is a convenience class inherited
/// by pqIntegrationModelSeedHelperWidget and pqIntegrationModelSurfaceHelperWidget
/// It connects an "IntegrationModel" properties changes to a call
/// to updateWidget method.
class pqIntegrationModelHelperWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqIntegrationModelHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0);
  ~pqIntegrationModelHelperWidget() override = default;

protected Q_SLOTS:
  virtual void resetWidget() = 0;

protected:
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  vtkSMProxyProperty* ModelProperty;
  vtkSMProxy* ModelPropertyValue;

private:
  Q_DISABLE_COPY(pqIntegrationModelHelperWidget)
};

#endif
