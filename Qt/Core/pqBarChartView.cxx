/*=========================================================================

   Program: ParaView
   Module:    pqBarChartView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqBarChartView.h"

#include "vtkSMProperty.h"
#include "vtkSMChartViewProxy.h"
#include "pqSMAdaptor.h"

// Qt Includes.
#include <QDebug>
#include <QColor>

//-----------------------------------------------------------------------------
pqBarChartView::pqBarChartView(const QString& group,
                               const QString& name, 
                               vtkSMChartViewProxy* viewModule,
                               pqServer* server, 
                               QObject* parent/*=NULL*/):
  Superclass(barChartViewType(), group, name, viewModule, server, parent)
{
//  this->Internal = new pqInternal();
//  this->Internal->BarChartView = vtkSMBarChartViewProxy::SafeDownCast(
//    viewModule)->GetBarChartView();
//
//  // Set up the paraview style interactor.
//  vtkQtChartArea* area = this->Internal->BarChartView->GetChartArea();
//  vtkQtChartMouseSelection* selector =
//    vtkQtChartInteractorSetup::createSplitZoom(area);
//  this->Internal->BarChartView->AddChartSelectionHandlers(selector);
//  vtkQtChartInteractorSetup::setupDefaultKeys(area->getInteractor());
}

//-----------------------------------------------------------------------------
pqBarChartView::~pqBarChartView()
{
}

//-----------------------------------------------------------------------------
void pqBarChartView::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  /*
  // Load defaults for the properties that need them.
  int i = 0;
  QList<QVariant> values;
  for(i = 0; i < 4; i++)
    {
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    values.append(QVariant((double)0.0));
    }

  vtkSMProxy *proxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelColor"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    if(i < 2)
      {
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      }
    else
      {
      // Use a different color for the right and top axis.
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.0));
      values.append(QVariant((double)0.5));
      }
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisColor"), values);
  values.clear();
  for(i = 0; i < 4; i++)
    {
    QColor grid = Qt::lightGray;
    values.append(QVariant((double)grid.redF()));
    values.append(QVariant((double)grid.greenF()));
    values.append(QVariant((double)grid.blueF()));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisGridColor"), values);
  QFont chartFont = this->Internal->BarChartView->GetWidget()->font();
  values.clear();
  values.append(chartFont.family());
  values.append(QVariant(chartFont.pointSize()));
  values.append(QVariant(chartFont.bold() ? 1 : 0));
  values.append(QVariant(chartFont.italic() ? 1 : 0));
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("ChartTitleFont"), values);
  for(i = 0; i < 3; i++)
    {
    values.append(chartFont.family());
    values.append(QVariant(chartFont.pointSize()));
    values.append(QVariant(chartFont.bold() ? 1 : 0));
    values.append(QVariant(chartFont.italic() ? 1 : 0));
    }

  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisLabelFont"), values);
  pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("AxisTitleFont"), values);
  */
}


