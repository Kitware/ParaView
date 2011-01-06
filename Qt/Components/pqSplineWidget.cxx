/*=========================================================================

   Program: ParaView
   Module:    pqSplineWidget.cxx

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
#include "pqSplineWidget.h"
#include "ui_pqSplineWidget.h"

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqSMAdaptor.h"

#include <QDoubleValidator>
#include <QShortcut>
#include <QtDebug>

#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPropertyHelper.h"

class pqSplineWidget::pqInternals : public Ui::SplineWidget
{
public:
  pqPropertyLinks Links;
  pqSignalAdaptorTreeWidget* PointsAdaptor;
};

//-----------------------------------------------------------------------------
pqSplineWidget::pqSplineWidget(
  vtkSMProxy* _smproxy, vtkSMProxy* pxy, QWidget* p) :
  Superclass(_smproxy, pxy, p)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  this->Internals->PointsAdaptor = new pqSignalAdaptorTreeWidget(
    this->Internals->HandlePositions, true);

  this->Internals->Visibility->setChecked(this->widgetVisible());
  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this->Internals->Visibility, SLOT(setChecked(bool)));

  QObject::connect(this->Internals->Visibility,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(&this->Internals->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  QObject::connect(&this->Internals->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(render()));

  QObject::connect(this->Internals->AddPoint, SIGNAL(clicked()),
    this, SLOT(addPoint()));
  QObject::connect(this->Internals->Delete, SIGNAL(clicked()),
    this, SLOT(removePoints()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(_smproxy->GetSession()));
}

//-----------------------------------------------------------------------------
pqSplineWidget::~pqSplineWidget()
{
  this->cleanupWidget();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSplineWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("SplineWidgetRepresentation", server);
  this->setWidgetProxy(widget);
  
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  this->Internals->Links.addPropertyLink(
    this->Internals->Closed, "checked",
    SIGNAL(toggled(bool)),
    widget, widget->GetProperty("Closed"));

  this->Internals->Links.addPropertyLink(
    this->Internals->PointsAdaptor, "values",
    SIGNAL(valuesChanged()),
    widget, widget->GetProperty("HandlePositions"));
}

//-----------------------------------------------------------------------------
void pqSplineWidget::cleanupWidget()
{
  this->Internals->Links.removeAllPropertyLinks();
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqSplineWidget::setLineColor(const QColor& color)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  vtkSMPropertyHelper(widget,
    "LineColor").Set(0, color.redF());
 vtkSMPropertyHelper(widget,
    "LineColor").Set(1,color.greenF());
 vtkSMPropertyHelper(widget,
    "LineColor").Set(2 , color.blueF());
  widget->UpdateVTKObjects(); 
}

//-----------------------------------------------------------------------------
void pqSplineWidget::addPoint()
{
  QTreeWidgetItem* newItem = this->Internals->PointsAdaptor->growTable();
  QTreeWidget* tree = this->Internals->HandlePositions;
  tree->setCurrentItem(newItem);
  // edit the first column.
  tree->editItem(newItem, 0);
}

//-----------------------------------------------------------------------------
void pqSplineWidget::removePoints()
{
  QList<QTreeWidgetItem*> items =
    this->Internals->HandlePositions->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
    {
    if (this->Internals->HandlePositions->topLevelItemCount() <= 1)
      {
      qDebug() <<
        "At least one point location is required. Deletion request ignored.";
      // don't allow deletion of the last point.
      break;
      }
    delete item;
    }
}
