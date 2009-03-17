/*=========================================================================

   Program: ParaView
   Module:    pqCameraWidget.cxx

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

=========================================================================*/
#include "pqCameraWidget.h"
#include "ui_pqCameraWidget.h"

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqCameraPathCreator.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqSplineWidget.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProxyManager.h"


//-----------------------------------------------------------------------------
class pqCameraWidget::pqInternal: public Ui::pqCameraWidget 
{
public:
  pqSplineWidget* PositionSplineWidget;
  pqSplineWidget* FocalSplineWidget;
  vtkSmartPointer<vtkSMProxy> PositionSpline;
  vtkSmartPointer<vtkSMProxy> FocalSpline;

  pqInternal()
    {
    this->PositionSplineWidget = 0;
    this->FocalSplineWidget = 0;
    }

  ~pqInternal()
    {
    delete this->PositionSplineWidget;
    delete this->FocalSplineWidget;
    }
};

//-----------------------------------------------------------------------------
pqCameraWidget::pqCameraWidget(QWidget* _p): QWidget(_p)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  this->connect(this->Internal->position0,
    SIGNAL(valueChanged(double)),
    SIGNAL(positionChanged()));
  this->connect(this->Internal->position1,
    SIGNAL(valueChanged(double)),
    SIGNAL(positionChanged()));
  this->connect(this->Internal->position2,
    SIGNAL(valueChanged(double)),
    SIGNAL(positionChanged()));

  this->connect(this->Internal->focalPoint0,
    SIGNAL(valueChanged(double)),
    SIGNAL(focalPointChanged()));
  this->connect(this->Internal->focalPoint1,
    SIGNAL(valueChanged(double)),
    SIGNAL(focalPointChanged()));
  this->connect(this->Internal->focalPoint2,
    SIGNAL(valueChanged(double)),
    SIGNAL(focalPointChanged()));

  this->connect(this->Internal->viewUp0,
    SIGNAL(valueChanged(double)),
    SIGNAL(viewUpChanged()));
  this->connect(this->Internal->viewUp1,
    SIGNAL(valueChanged(double)),
    SIGNAL(viewUpChanged()));
  this->connect(this->Internal->viewUp2,
    SIGNAL(valueChanged(double)),
    SIGNAL(viewUpChanged()));

  QObject::connect(this->Internal->viewUpX,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUp0,
    SLOT(setValue(double)));
  QObject::connect(this->Internal->viewUpY,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUp1,
    SLOT(setValue(double)));
  QObject::connect(this->Internal->viewUpZ,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUp2,
    SLOT(setValue(double)));

  QObject::connect(this->Internal->viewUp0,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUpX,
    SLOT(setValue(double)));
  QObject::connect(this->Internal->viewUp1,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUpY,
    SLOT(setValue(double)));
  QObject::connect(this->Internal->viewUp2,
    SIGNAL(valueChanged(double)),
    this->Internal->viewUpZ,
    SLOT(setValue(double)));

  this->connect(this->Internal->viewAngle,
    SIGNAL(valueChanged(double)),
    SIGNAL(viewAngleChanged()));
  this->connect(this->Internal->useCurrent,
    SIGNAL(clicked(bool)),
    SIGNAL(useCurrent()));
  this->connect(this->Internal->createPathPosition,
    SIGNAL(clicked(bool)),
    SLOT(createPositionPath()));
  this->connect(this->Internal->createPathFocalPoint,
    SIGNAL(clicked(bool)),
    SLOT(createFocalPointPath()));

  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
    {
    // this happens in pqAnimationPanel. However, that panel needs to be
    // deprecated anyways, so I am going to ignore that case.
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  this->Internal->PositionSpline.TakeReference(
    pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->PositionSpline->SetConnectionID(server->GetConnectionID());
  this->Internal->PositionSpline->SetServers(vtkProcessModule::CLIENT);
  this->Internal->PositionSpline->UpdateVTKObjects();

  this->Internal->PositionSplineWidget = new pqSplineWidget(
    this->Internal->PositionSpline,
    this->Internal->PositionSpline,
    this);
  this->Internal->PositionSplineWidget->setHints(
    this->Internal->PositionSpline->GetHints()->FindNestedElementByName(
      "PropertyGroup"));

  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this->Internal->PositionSplineWidget,
    SLOT(setView(pqView*)));
  this->Internal->PositionSplineWidget->setView(pqActiveView::instance().current());

  (new QVBoxLayout(this->Internal->positionSplineContainer))->addWidget(
    this->Internal->PositionSplineWidget);
  this->Internal->positionSplineContainer->layout()->setMargin(0);

  this->Internal->FocalSpline.TakeReference(
    pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->FocalSpline->SetConnectionID(server->GetConnectionID());
  this->Internal->FocalSpline->SetServers(vtkProcessModule::CLIENT);
  this->Internal->FocalSpline->UpdateVTKObjects();

  this->Internal->FocalSplineWidget = new pqSplineWidget(
    this->Internal->FocalSpline,
    this->Internal->FocalSpline,
    this);
  this->Internal->PositionSplineWidget->setLineColor(Qt::cyan);
  this->Internal->FocalSplineWidget->setHints(
    this->Internal->FocalSpline->GetHints()->FindNestedElementByName(
      "PropertyGroup"));
  (new QVBoxLayout(this->Internal->focalSplineContainer))->addWidget(
    this->Internal->FocalSplineWidget);
  this->Internal->focalSplineContainer->layout()->setMargin(0);
  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this->Internal->FocalSplineWidget,
    SLOT(setView(pqView*)));
  this->Internal->FocalSplineWidget->setView(pqActiveView::instance().current());
}

//-----------------------------------------------------------------------------
pqCameraWidget::~pqCameraWidget()
{
  delete this->Internal;
}
  
//-----------------------------------------------------------------------------
void pqCameraWidget::setUsePaths(bool use_path)
{
  this->Internal->stackedWidget->setCurrentIndex(use_path? 1 : 0);
}

//-----------------------------------------------------------------------------
bool pqCameraWidget::usePaths() const
{
  return (this->Internal->stackedWidget->currentIndex() == 1);
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setPosition(QList<QVariant> v)
{
  if(v.size() == 3 && this->position() != v)
    {
    this->blockSignals(true);
    this->Internal->position0->setValue(v[0].toDouble());
    this->Internal->position1->setValue(v[1].toDouble());
    this->Internal->position2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->positionChanged();
    }
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setFocalPoint(QList<QVariant> v)
{
  if(v.size() == 3 && this->focalPoint() != v)
    {
    this->blockSignals(true);
    this->Internal->focalPoint0->setValue(v[0].toDouble());
    this->Internal->focalPoint1->setValue(v[1].toDouble());
    this->Internal->focalPoint2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->focalPointChanged();
    }
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setViewUp(QList<QVariant> v)
{
  if(v.size() == 3 && this->viewUp() != v)
    {
    this->blockSignals(true);
    this->Internal->viewUp0->setValue(v[0].toDouble());
    this->Internal->viewUp1->setValue(v[1].toDouble());
    this->Internal->viewUp2->setValue(v[2].toDouble());
    this->blockSignals(false);
    emit this->viewUpChanged();
    }
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setViewAngle(QVariant v)
{
  if(this->viewAngle() != v)
    {
    this->Internal->viewAngle->setValue(v.toDouble());
    }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraWidget::position() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->position0->value());
  ret.append(this->Internal->position1->value());
  ret.append(this->Internal->position2->value());
  return ret;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraWidget::focalPoint() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->focalPoint0->value());
  ret.append(this->Internal->focalPoint1->value());
  ret.append(this->Internal->focalPoint2->value());
  return ret;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraWidget::viewUp() const
{
  QList<QVariant> ret;
  ret.append(this->Internal->viewUp0->value());
  ret.append(this->Internal->viewUp1->value());
  ret.append(this->Internal->viewUp2->value());
  return ret;
}

//-----------------------------------------------------------------------------
QVariant pqCameraWidget::viewAngle() const
{
  return this->Internal->viewAngle->value();
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setFocalPointPath(const QList<QVariant>& points)
{
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->FocalSpline->GetProperty("Points"),
    points);
  this->Internal->FocalSplineWidget->reset();
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setPositionPath(const QList<QVariant>& points)
{
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->PositionSpline->GetProperty("Points"),
    points);
  this->Internal->PositionSplineWidget->reset();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraWidget::focalPath() const
{
  this->Internal->FocalSplineWidget->accept();
  return pqSMAdaptor::getMultipleElementProperty(
    this->Internal->FocalSpline->GetProperty("Points"));
}

//-----------------------------------------------------------------------------
QList<QVariant> pqCameraWidget::positionPath() const
{
  this->Internal->PositionSplineWidget->accept();
  return pqSMAdaptor::getMultipleElementProperty(
    this->Internal->PositionSpline->GetProperty("Points"));
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setClosedPositionPath(bool closed)
{
  pqSMAdaptor::setElementProperty(
    this->Internal->PositionSplineWidget->getWidgetProxy()->GetProperty("Closed"),
    closed);
}

//-----------------------------------------------------------------------------
bool pqCameraWidget::closedPositionPath() const
{
  return pqSMAdaptor::getElementProperty(
    this->Internal->PositionSplineWidget->getWidgetProxy()->GetProperty("Closed")).toBool();
}

//-----------------------------------------------------------------------------
void pqCameraWidget::setClosedFocalPath(bool closed)
{
  pqSMAdaptor::setElementProperty(
    this->Internal->FocalSplineWidget->getWidgetProxy()->GetProperty("Closed"),
    closed);
}

//-----------------------------------------------------------------------------
bool pqCameraWidget::closedFocalPath() const
{
  return pqSMAdaptor::getElementProperty(
    this->Internal->FocalSplineWidget->getWidgetProxy()->GetProperty("Closed")).toBool();
}

//-----------------------------------------------------------------------------
void pqCameraWidget::createFocalPointPath()
{
  this->hideDialog();
  pqCameraPathCreator *creator = new pqCameraPathCreator(this);
  creator->setAttribute(Qt::WA_DeleteOnClose);
  creator->show();
  QObject::connect(creator, SIGNAL(pathSelected(const QList<QVariant>&)),
    this, SLOT(setFocalPointPath(const QList<QVariant>&)));
  QObject::connect(creator, SIGNAL(closedPath(bool)),
    this, SLOT(setClosedFocalPath(bool)));
  QObject::connect(creator, SIGNAL(finished(int)),
    this, SLOT(showDialog()));
}

//-----------------------------------------------------------------------------
void pqCameraWidget::createPositionPath()
{
  this->hideDialog();
  pqCameraPathCreator *creator = new pqCameraPathCreator(this);
  creator->setAttribute(Qt::WA_DeleteOnClose);
  creator->show();
  QObject::connect(creator, SIGNAL(pathSelected(const QList<QVariant>&)),
    this, SLOT(setPositionPath(const QList<QVariant>&)));
  QObject::connect(creator, SIGNAL(pathNormal(const QList<QVariant>&)),
    this, SLOT(setViewUp(QList<QVariant>)));
  QObject::connect(creator, SIGNAL(closedPath(bool)),
    this, SLOT(setClosedPositionPath(bool)));
  QObject::connect(creator, SIGNAL(finished(int)),
    this, SLOT(showDialog()));
}

//-----------------------------------------------------------------------------
void pqCameraWidget::hideDialog()
{
  // HACK, since we are ending up with too many open non-modal dialogs.
  this->window()->hide();  
}

//-----------------------------------------------------------------------------
void pqCameraWidget::showDialog()
{
  // HACK, since we are ending up with too many open non-modal dialogs.
  this->window()->show();
}

//-----------------------------------------------------------------------------
void pqCameraWidget::showEvent(QShowEvent* event)
{
  this->Superclass::showEvent(event);
  if (this->Internal->PositionSplineWidget && this->usePaths())
    {
    this->Internal->PositionSplineWidget->select();
    this->Internal->FocalSplineWidget->select();
    }
}

//-----------------------------------------------------------------------------
void pqCameraWidget::hideEvent(QHideEvent* event)
{
  this->Superclass::hideEvent(event);
  if (this->Internal->PositionSplineWidget)
    {
    this->Internal->PositionSplineWidget->deselect();
    this->Internal->FocalSplineWidget->deselect();
    }
}
