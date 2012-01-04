/*=========================================================================

   Program: ParaView
   Module:    pqCameraKeyFrameWidget.cxx

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
#include "pqCameraKeyFrameWidget.h"
#include "ui_pqCameraKeyFrameWidget.h"

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqSplineWidget.h"
#include "vtkCamera.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMProperty.h"
#include "vtkPVSession.h"

#include <QPointer>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <QDoubleValidator>

class pqCameraKeyFrameWidget::pqInternal : public Ui::CameraKeyFrameWidget
{
public:
  vtkSmartPointer<vtkSMProxy> PSplineProxy;
  QPointer<pqSplineWidget> PSplineWidget;

  vtkSmartPointer<vtkSMProxy> FSplineProxy;
  QPointer<pqSplineWidget> FSplineWidget;
  double Data[3];
  pqInternal()
    {
    }

  void setupValidators(QObject* parent)
    {
    this->position0->setValidator(new QDoubleValidator(parent));
    this->position1->setValidator(new QDoubleValidator(parent));
    this->position2->setValidator(new QDoubleValidator(parent));

    this->focalPoint0->setValidator(new QDoubleValidator(parent));
    this->focalPoint1->setValidator(new QDoubleValidator(parent));
    this->focalPoint2->setValidator(new QDoubleValidator(parent));

    this->viewUp0->setValidator(new QDoubleValidator(parent));
    this->viewUp1->setValidator(new QDoubleValidator(parent));
    this->viewUp2->setValidator(new QDoubleValidator(parent));
    }

  void setPosition(const double pos[3])
    {
    this->position0->setText(QString::number(pos[0]));
    this->position1->setText(QString::number(pos[1]));
    this->position2->setText(QString::number(pos[2]));
    }

  const double* position()
    {
    this->Data[0] = this->position0->text().toDouble();
    this->Data[1] = this->position1->text().toDouble();
    this->Data[2] = this->position2->text().toDouble();
    return this->Data;
    }

  void setFocalPoint(const double pos[3])
    {
    this->focalPoint0->setText(QString::number(pos[0]));
    this->focalPoint1->setText(QString::number(pos[1]));
    this->focalPoint2->setText(QString::number(pos[2]));
    }

  const double* focalPoint()
    {
    this->Data[0] = this->focalPoint0->text().toDouble();
    this->Data[1] = this->focalPoint1->text().toDouble();
    this->Data[2] = this->focalPoint2->text().toDouble();
    return this->Data;
    }

  void setViewUp(const double pos[3])
    {
    this->viewUp0->setText(QString::number(pos[0]));
    this->viewUp1->setText(QString::number(pos[1]));
    this->viewUp2->setText(QString::number(pos[2]));

    this->viewUpX->setText(QString::number(pos[0]));
    this->viewUpY->setText(QString::number(pos[1]));
    this->viewUpZ->setText(QString::number(pos[2]));
    }

  const double* viewUp_NonPath()
    {
    this->Data[0] = this->viewUp0->text().toDouble();
    this->Data[1] = this->viewUp1->text().toDouble();
    this->Data[2] = this->viewUp2->text().toDouble();
    return this->Data;
    }

  const double* viewUp_Path()
    {
    this->Data[0] = this->viewUpX->text().toDouble();
    this->Data[1] = this->viewUpY->text().toDouble();
    this->Data[2] = this->viewUpZ->text().toDouble();
    return this->Data;
    }

  void setViewAngle(double val)
    {
    this->viewAngle->setValue(val);
    }

  double getViewAngle()
    {
    return this->viewAngle->value();
    }
};

//-----------------------------------------------------------------------------
pqCameraKeyFrameWidget::pqCameraKeyFrameWidget(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);

  // setup validators.
  this->Internal->setupValidators(this);

  // hide the header for the tree widget.
  this->Internal->leftPane->header()->hide();
  this->Internal->leftPane->setCurrentItem(0);

  this->connect(this->Internal->leftPane,
    SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
    this, SLOT(changeCurrentPage()));

  // * when user clicks useCurrent, we fire the useCurrentCamera signal.
  this->connect(this->Internal->useCurrent,
    SIGNAL(clicked(bool)), SIGNAL(useCurrentCamera()));

  // * Create the spline widget used for defining the paths.
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
    {
    qCritical() << 
      "pqCameraKeyFrameWidget cannot be created without a server connection.";
    return;
    }

  vtkSMSessionProxyManager* pxm = server->proxyManager();

  this->Internal->PSplineProxy.TakeReference(
    pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->PSplineProxy->SetLocation(
    vtkPVSession::CLIENT);
  this->Internal->PSplineProxy->UpdateVTKObjects();

  this->Internal->PSplineWidget = new pqSplineWidget(
    this->Internal->PSplineProxy, this->Internal->PSplineProxy, this);
  this->Internal->PSplineWidget->setHints(
    this->Internal->PSplineProxy->GetHints()->FindNestedElementByName(
      "PropertyGroup"));

  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this->Internal->PSplineWidget,
    SLOT(setView(pqView*)));
  this->Internal->PSplineWidget->setView(pqActiveView::instance().current());
  this->Internal->PSplineWidget->deselect();

  (new QVBoxLayout(this->Internal->positionContainer))->addWidget(
    this->Internal->PSplineWidget);
  this->Internal->positionContainer->layout()->setMargin(0);

  this->Internal->FSplineProxy.TakeReference(
    pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->PSplineProxy->SetLocation(vtkPVSession::CLIENT);
  this->Internal->FSplineProxy->UpdateVTKObjects();

  this->Internal->FSplineWidget = new pqSplineWidget(
    this->Internal->FSplineProxy, this->Internal->FSplineProxy, this);
  this->Internal->FSplineWidget->setHints(
    this->Internal->FSplineProxy->GetHints()->FindNestedElementByName(
      "PropertyGroup"));

  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this->Internal->FSplineWidget,
    SLOT(setView(pqView*)));
  this->Internal->FSplineWidget->setView(pqActiveView::instance().current());
  this->Internal->FSplineWidget->deselect();

  (new QVBoxLayout(this->Internal->focusContainer))->addWidget(
    this->Internal->FSplineWidget);
  this->Internal->focusContainer->layout()->setMargin(0);
}

//-----------------------------------------------------------------------------
pqCameraKeyFrameWidget::~pqCameraKeyFrameWidget()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::setUsePathBasedMode(bool use_paths)
{
  this->Internal->stackedWidgetMode->setCurrentIndex(use_paths? 0 : 1);
}

//-----------------------------------------------------------------------------
bool pqCameraKeyFrameWidget::usePathBasedMode() const
{
  return (this->Internal->stackedWidgetMode->currentIndex() == 0);
}

//-----------------------------------------------------------------------------
/// Initialize the widget using the values from the key frame proxy.
void pqCameraKeyFrameWidget::initializeUsingKeyFrame(vtkSMProxy* keyFrame)
{
  this->Internal->setPosition(
    &vtkSMPropertyHelper(keyFrame, "Position").GetDoubleArray()[0]);
  this->Internal->setFocalPoint(
    &vtkSMPropertyHelper(keyFrame, "FocalPoint").GetDoubleArray()[0]);
  this->Internal->setViewUp(
    &vtkSMPropertyHelper(keyFrame, "ViewUp").GetDoubleArray()[0]);
  this->Internal->setViewAngle(
    vtkSMPropertyHelper(keyFrame, "ViewAngle").GetAsDouble());

  this->Internal->PSplineProxy->GetProperty("Points")->Copy(
    keyFrame->GetProperty("PositionPathPoints"));

  this->Internal->PSplineProxy->GetProperty("Closed")->Copy(
    keyFrame->GetProperty("ClosedPositionPath"));

  this->Internal->FSplineProxy->GetProperty("Points")->Copy(
    keyFrame->GetProperty("FocalPathPoints"));

  this->Internal->FSplineProxy->GetProperty("Closed")->Copy(
    keyFrame->GetProperty("ClosedFocalPath"));

  this->Internal->PSplineProxy->UpdateVTKObjects();
  this->Internal->FSplineProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
/// Initialize the widget using the camera.
void pqCameraKeyFrameWidget::initializeUsingCamera(vtkCamera* camera)
{
  this->Internal->setPosition(camera->GetPosition());
  this->Internal->setFocalPoint(camera->GetFocalPoint());
  this->Internal->setViewUp(camera->GetViewUp());
  this->Internal->setViewAngle(camera->GetViewAngle());
}

//-----------------------------------------------------------------------------
/// Write the user chosen values for this key frame to the proxy.
void pqCameraKeyFrameWidget::saveToKeyFrame(vtkSMProxy* keyFrame)
{
  this->Internal->PSplineWidget->accept();
  this->Internal->FSplineWidget->accept();

  vtkSMPropertyHelper(keyFrame,"Position").Set(
    this->Internal->position(), 3);

  vtkSMPropertyHelper(keyFrame, "FocalPoint").Set(
    this->Internal->focalPoint(), 3);

  vtkSMPropertyHelper(keyFrame, "ViewUp").Set(
    this->usePathBasedMode()?
    this->Internal->viewUp_Path():
    this->Internal->viewUp_NonPath(), 3);
  vtkSMPropertyHelper(keyFrame, "ViewAngle").Set(
    this->Internal->getViewAngle());

  keyFrame->GetProperty("PositionPathPoints")->Copy(
    this->Internal->PSplineProxy->GetProperty("Points"));

  keyFrame->GetProperty("FocalPathPoints")->Copy(
    this->Internal->FSplineProxy->GetProperty("Points"));

  keyFrame->GetProperty("ClosedPositionPath")->Copy(
    this->Internal->PSplineProxy->GetProperty("Closed"));

  keyFrame->GetProperty("ClosedFocalPath")->Copy(
    this->Internal->FSplineProxy->GetProperty("Closed"));
  keyFrame->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::changeCurrentPage()
{
  QTreeWidgetItem* currentItem = this->Internal->leftPane->currentItem();
  if (!currentItem)
    {
    this->Internal->stackedWidget->setCurrentIndex(0);
    }
  else if (currentItem->text(0) == "Camera Position")
    {
    this->Internal->stackedWidget->setCurrentIndex(1);
    }
  else if ( currentItem->text(0) == "Camera Focus")
    {
    this->Internal->stackedWidget->setCurrentIndex(2);
    }
  else
    {
    this->Internal->stackedWidget->setCurrentIndex(3);
    }
  this->updateSplineWidget();
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::updateSplineWidget()
{
  if (this->usePathBasedMode())
    {
    switch (this->Internal->stackedWidget->currentIndex())
      {
    case 1:
      this->Internal->PSplineWidget->select();
      this->Internal->FSplineWidget->deselect();
      return;

    case 2:
      this->Internal->FSplineWidget->select();
      this->Internal->PSplineWidget->deselect();
      return;
      }
    }

  this->Internal->FSplineWidget->deselect();
  this->Internal->PSplineWidget->deselect();
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::showEvent(QShowEvent* anEvent)
{
  this->Superclass::showEvent(anEvent);
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::hideEvent(QHideEvent* anEvent)
{
  this->Superclass::hideEvent(anEvent);
  this->Internal->leftPane->setCurrentItem(0);
}
