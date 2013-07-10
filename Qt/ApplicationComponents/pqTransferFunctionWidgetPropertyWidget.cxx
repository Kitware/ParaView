/*=========================================================================

   Program: ParaView
   Module: pqTransferFunctionWidgetPropertyWidget.cxx

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
#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "ui_pqTransferFunctionWidgetPropertyWidgetDialog.h"

#include "pqPVApplicationCore.h"
#include "pqTransferFunctionWidget.h"
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace
{
  class pqTransferFunctionWidgetPropertyWidgetDialog : public QDialog
  {
  vtkSmartPointer<vtkPiecewiseFunction> TransferFunction;
  Ui::TransferFunctionWidgetPropertyWidgetDialog Ui;
public:
  pqTransferFunctionWidgetPropertyWidgetDialog(
    const QString& label,
    vtkPiecewiseFunction* transferFunction, QWidget* parentWdg=NULL) :
    QDialog(parentWdg),
    TransferFunction(transferFunction)
    {
    this->setWindowTitle(label);
    this->Ui.setupUi(this);
    this->Ui.Label->setText(
      this->Ui.Label->text().arg(label));
    this->Ui.TransferFunctionEditor->initialize(NULL, false,
      this->TransferFunction.GetPointer(), true);
    }
  ~pqTransferFunctionWidgetPropertyWidgetDialog()
    {
    }
  };
};

pqTransferFunctionWidgetPropertyWidget::pqTransferFunctionWidgetPropertyWidget(vtkSMProxy *smProxy,
                                                                               vtkSMProperty *proxyProperty,
                                                                               QWidget *pWidget)
  : pqPropertyWidget(smProxy, pWidget),
    Property(proxyProperty)
{
  QVBoxLayout *l = new QVBoxLayout;
  l->setMargin(0);

  QPushButton *button = new QPushButton("Edit");
  connect(button, SIGNAL(clicked()), this, SLOT(buttonClicked()));
  l->addWidget(button);

  this->setLayout(l);

  PV_DEBUG_PANELS() << "pqTransferFunctionWidgetPropertyWidget for a property with "
                << "the panel_widget=\"transfer_function_editor\" attribute";
}

pqTransferFunctionWidgetPropertyWidget::~pqTransferFunctionWidgetPropertyWidget()
{
}

void pqTransferFunctionWidgetPropertyWidget::buttonClicked()
{
  vtkSMProxyProperty *proxyProperty = vtkSMProxyProperty::SafeDownCast(this->Property);
  if(!proxyProperty)
    {
    qDebug() << "error, property is not a proxy property";
    return;
    }
  if(proxyProperty->GetNumberOfProxies() < 1)
    {
    qDebug() << "error, no proxies for property";
    return;
    }

  vtkSMProxy *pxy = proxyProperty->GetProxy(0);
  if(!pxy)
    {
    qDebug() << "error, no proxy property";
    return;
    }
  vtkObjectBase *object = pxy->GetClientSideObject();

  vtkPiecewiseFunction *transferFunction =
    vtkPiecewiseFunction::SafeDownCast(object);

  pqTransferFunctionWidgetPropertyWidgetDialog dialog(
    this->Property->GetXMLLabel(), transferFunction, this);
  dialog.exec();

  // BUG: we are never propagating the vtkPiecewiseFunction change to the
  // SMProperty on the proxy!!! This will never work in client-server mode.
  emit this->changeAvailable();
  emit this->changeFinished();
}
