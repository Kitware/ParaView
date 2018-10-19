/*=========================================================================

   Program: ParaView
   Module: pqTransferFunctionWidgetPropertyWidget.h

   Copyright (c) 2005-2012 Kitware Inc.
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

=========================================================================*/
#ifndef _pqTransferFunctionWidgetPropertyWidget_h
#define _pqTransferFunctionWidgetPropertyWidget_h

#include "pqApplicationComponentsModule.h" // for export macros
#include "pqPropertyWidget.h"

class vtkSMTransferFunctionProxy;
class vtkEventQtSlotConnect;
class vtkSMRangedTransferFunctionDomain;
class pqTransferFunctionWidgetPropertyDialog;

/**
* A property widget for editing a transfer function.
*
* To use this widget for a property add the
* 'panel_widget="transfer_function_editor"' to the property's XML.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqTransferFunctionWidgetPropertyWidget
  : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pqTransferFunctionWidgetPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = 0);
  ~pqTransferFunctionWidgetPropertyWidget() override;

  friend pqTransferFunctionWidgetPropertyDialog;

signals:
  void domainChanged();

protected:
  void UpdateProperty();

  void getRange(double range[2]);
  void setRange(const double& min, const double& max);

protected slots:
  void buttonClicked();
  void propagateProxyPointsProperty();
  void resetRangeToDomainDefault();

private:
  vtkEventQtSlotConnect* Connection;
  vtkSMTransferFunctionProxy* TFProxy;
  QDialog* Dialog;
  vtkSMRangedTransferFunctionDomain* Domain;
};

#endif // _pqTransferFunctionWidgetPropertyWidget_h
