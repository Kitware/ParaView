// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraKeyFrameWidget.h"
#include "ui_pqCameraKeyFrameWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqProxyWidget.h"
#include "pqSMAdaptor.h"
#include "pqServer.h"
#include "vtkCamera.h"
#include "vtkPVSession.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtk_jsoncpp.h"

#include <QDebug>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QPointer>
#include <QVBoxLayout>

class pqCameraKeyFrameWidget::pqInternal : public Ui::CameraKeyFrameWidget
{
public:
  vtkSmartPointer<vtkSMProxy> PSplineProxy;
  QPointer<pqProxyWidget> PSplineWidget;

  vtkSmartPointer<vtkSMProxy> FSplineProxy;
  QPointer<pqProxyWidget> FSplineWidget;
  double Data[3];
  pqInternal() = default;

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

    this->viewAngle->setValidator(new QDoubleValidator(parent));
    this->parallelScale->setValidator(new QDoubleValidator(parent));
  }

  void setPosition(const double pos[3])
  {
    this->position0->setTextAndResetCursor(QString::number(pos[0]));
    this->position1->setTextAndResetCursor(QString::number(pos[1]));
    this->position2->setTextAndResetCursor(QString::number(pos[2]));
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
    this->focalPoint0->setTextAndResetCursor(QString::number(pos[0]));
    this->focalPoint1->setTextAndResetCursor(QString::number(pos[1]));
    this->focalPoint2->setTextAndResetCursor(QString::number(pos[2]));
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
    this->viewUp0->setTextAndResetCursor(QString::number(pos[0]));
    this->viewUp1->setTextAndResetCursor(QString::number(pos[1]));
    this->viewUp2->setTextAndResetCursor(QString::number(pos[2]));

    this->viewUpX->setTextAndResetCursor(QString::number(pos[0]));
    this->viewUpY->setTextAndResetCursor(QString::number(pos[1]));
    this->viewUpZ->setTextAndResetCursor(QString::number(pos[2]));
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

  void setViewAngle(double val) { this->viewAngle->setTextAndResetCursor(QString("%1").arg(val)); }

  double getViewAngle() { return this->viewAngle->text().toDouble(); }

  void setParallelScale(double val)
  {
    this->parallelScale->setTextAndResetCursor(QString("%1").arg(val));
  }

  double getParallelScale() const { return this->parallelScale->text().toDouble(); }
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
  this->Internal->leftPane->setCurrentItem(nullptr);

  this->connect(this->Internal->leftPane,
    SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this,
    SLOT(changeCurrentPage()));

  // * when user clicks useCurrent, we fire the useCurrentCamera signal.
  this->connect(this->Internal->useCurrent, SIGNAL(clicked(bool)), SIGNAL(useCurrentCamera()));
  this->connect(
    this->Internal->updateCurrent, SIGNAL(clicked(bool)), SIGNAL(updateCurrentCamera()));

  // * Create the spline widget used for defining the paths.
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (!server)
  {
    qCritical() << "pqCameraKeyFrameWidget cannot be created without a server connection.";
    return;
  }

  vtkSMSessionProxyManager* pxm = server->proxyManager();

  this->Internal->PSplineProxy.TakeReference(pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->PSplineProxy->SetLocation(vtkPVSession::CLIENT);
  vtkSMPropertyHelper(this->Internal->PSplineProxy, "Closed").Set(1);
  this->Internal->PSplineProxy->UpdateVTKObjects();

  this->Internal->PSplineWidget = new pqProxyWidget(this->Internal->PSplineProxy, this);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this->Internal->PSplineWidget, SLOT(setView(pqView*)));
  this->Internal->PSplineWidget->setView(pqActiveObjects::instance().activeView());
  this->Internal->PSplineWidget->filterWidgets();

  (new QVBoxLayout(this->Internal->positionContainer))->addWidget(this->Internal->PSplineWidget);
  this->Internal->positionContainer->layout()->setContentsMargins(0, 0, 0, 0);

  this->Internal->FSplineProxy.TakeReference(pxm->NewProxy("parametric_functions", "Spline"));
  this->Internal->PSplineProxy->SetLocation(vtkPVSession::CLIENT);
  this->Internal->FSplineProxy->UpdateVTKObjects();

  this->Internal->FSplineWidget = new pqProxyWidget(this->Internal->FSplineProxy, this);
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this->Internal->FSplineWidget, SLOT(setView(pqView*)));
  this->Internal->FSplineWidget->setView(pqActiveObjects::instance().activeView());
  this->Internal->FSplineWidget->filterWidgets();

  (new QVBoxLayout(this->Internal->focusContainer))->addWidget(this->Internal->FSplineWidget);
  this->Internal->focusContainer->layout()->setContentsMargins(0, 0, 0, 0);
}

//-----------------------------------------------------------------------------
pqCameraKeyFrameWidget::~pqCameraKeyFrameWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::setUsePathBasedMode(bool use_paths)
{
  this->Internal->stackedWidgetMode->setCurrentIndex(use_paths ? 0 : 1);
}

//-----------------------------------------------------------------------------
bool pqCameraKeyFrameWidget::usePathBasedMode() const
{
  return (this->Internal->stackedWidgetMode->currentIndex() == 0);
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::initializeUsingJSON(const Json::Value& json)
{
  std::vector<double> value;
  auto parseJSONVector = [&json, &value](const char* name, unsigned int size) -> bool {
    value.resize(size);
    for (unsigned int idx = 0; idx < size; idx++)
    {
      if (json[name][idx].isDouble())
      {
        value[idx] = json[name][idx].asDouble();
      }
      else
      {
        return false;
      }
    }
    return true;
  };

  if (json["viewUp"].size() == 3 && parseJSONVector("viewUp", 3))
  {
    this->Internal->setViewUp(value.data());
  }
  if (this->usePathBasedMode())
  {
    unsigned int size = json["positions"].size();
    if (size >= 3 && size % 3 == 0 && parseJSONVector("positions", size))
    {
      vtkSMPropertyHelper(this->Internal->PSplineProxy, "Points").Set(value.data(), size);
    }
    size = json["focalPoints"].size();
    if (size >= 3 && size % 3 == 0 && parseJSONVector("focalPoints", size))
    {
      vtkSMPropertyHelper(this->Internal->FSplineProxy, "Points").Set(value.data(), size);
    }
    if (json["positionPathClosed"].isInt())
    {
      vtkSMPropertyHelper(this->Internal->PSplineProxy, "Closed")
        .Set(json["positionPathClosed"].asInt());
    }
    if (json["focalPointPathClosed"].isInt())
    {
      vtkSMPropertyHelper(this->Internal->FSplineProxy, "Closed")
        .Set(json["focalPointPathClosed"].asInt());
    }
  }
  else
  {
    if (json["position"].size() == 3 && parseJSONVector("position", 3))
    {
      this->Internal->setPosition(value.data());
    }
    if (json["focalPoint"].size() == 3 && parseJSONVector("focalPoint", 3))
    {
      this->Internal->setFocalPoint(value.data());
    }
    if (json["viewAngle"].isDouble())
    {
      this->Internal->setViewAngle(json["viewAngle"].asDouble());
    }
    if (json["parallelScale"].isDouble())
    {
      this->Internal->setParallelScale(json["parallelScale"].asDouble());
    }
  }
}

//-----------------------------------------------------------------------------
Json::Value pqCameraKeyFrameWidget::serializeToJSON() const
{
  Json::Value keyFrame;

  auto addJSONVector = [&keyFrame](const char* name, const double* value, size_t size) {
    for (unsigned int idx = 0; idx < size; idx++)
    {
      keyFrame[name].insert(idx, value[idx]);
    }
  };

  if (this->usePathBasedMode())
  {
    auto positions = vtkSMPropertyHelper(this->Internal->PSplineProxy, "Points").GetDoubleArray();
    addJSONVector("positions", positions.data(), positions.size());
    keyFrame["positionPathClosed"] =
      vtkSMPropertyHelper(this->Internal->PSplineProxy, "Closed").GetAsInt();
    auto focalPoints = vtkSMPropertyHelper(this->Internal->FSplineProxy, "Points").GetDoubleArray();
    addJSONVector("focalPoints", focalPoints.data(), focalPoints.size());
    keyFrame["focalPointPathClosed"] =
      vtkSMPropertyHelper(this->Internal->FSplineProxy, "Closed").GetAsInt();
    addJSONVector("viewUp", this->Internal->viewUp_Path(), 3);
  }
  else
  {
    keyFrame["parallelScale"] = this->Internal->getParallelScale();
    keyFrame["viewAngle"] = this->Internal->getViewAngle();
    addJSONVector("viewUp", this->Internal->viewUp_NonPath(), 3);
    addJSONVector("position", this->Internal->position(), 3);
    addJSONVector("focalPoint", this->Internal->focalPoint(), 3);
  }
  return keyFrame;
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::setPositionPoints(const std::vector<double>& positions)
{
  this->Internal->setPosition(positions.data());

  QList<QVariant> varPos;
  for (auto pos : positions)
  {
    varPos << pos;
  }
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->PSplineProxy->GetProperty("Points"), varPos);

  this->Internal->PSplineProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::setFocalPoints(const std::vector<double>& focals)
{
  this->Internal->setFocalPoint(focals.data());

  QList<QVariant> varFocus;
  for (auto focus : focals)
  {
    varFocus << focus;
  }
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->FSplineProxy->GetProperty("FocalPoints"), varFocus);

  this->Internal->FSplineProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::setViewUp(double viewUp[3])
{
  this->Internal->setViewUp(viewUp);
}

//-----------------------------------------------------------------------------
/// Initialize the widget using the values from the key frame proxy.
void pqCameraKeyFrameWidget::initializeUsingKeyFrame(vtkSMProxy* keyFrame)
{
  this->Internal->setPosition(&vtkSMPropertyHelper(keyFrame, "Position").GetDoubleArray()[0]);
  this->Internal->setFocalPoint(&vtkSMPropertyHelper(keyFrame, "FocalPoint").GetDoubleArray()[0]);
  this->Internal->setViewUp(&vtkSMPropertyHelper(keyFrame, "ViewUp").GetDoubleArray()[0]);
  this->Internal->setViewAngle(vtkSMPropertyHelper(keyFrame, "ViewAngle").GetAsDouble());
  this->Internal->setParallelScale(vtkSMPropertyHelper(keyFrame, "ParallelScale").GetAsDouble());

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
  this->Internal->setParallelScale(camera->GetParallelScale());
}

//-----------------------------------------------------------------------------
void pqCameraKeyFrameWidget::applyToCamera(vtkCamera* camera)
{
  camera->SetPosition(this->Internal->position());
  camera->SetFocalPoint(this->Internal->focalPoint());
  camera->SetViewUp(this->Internal->viewUp_Path());
  camera->SetViewAngle(this->Internal->getViewAngle());
  camera->SetParallelScale(this->Internal->getParallelScale());
}

//-----------------------------------------------------------------------------
/// Write the user chosen values for this key frame to the proxy.
void pqCameraKeyFrameWidget::saveToKeyFrame(vtkSMProxy* keyFrame)
{
  this->Internal->PSplineWidget->apply();
  this->Internal->FSplineWidget->apply();

  vtkSMPropertyHelper(keyFrame, "Position").Set(this->Internal->position(), 3);

  vtkSMPropertyHelper(keyFrame, "FocalPoint").Set(this->Internal->focalPoint(), 3);

  vtkSMPropertyHelper(keyFrame, "ViewUp")
    .Set(
      this->usePathBasedMode() ? this->Internal->viewUp_Path() : this->Internal->viewUp_NonPath(),
      3);

  vtkSMPropertyHelper(keyFrame, "ViewAngle").Set(this->Internal->getViewAngle());

  vtkSMPropertyHelper(keyFrame, "ParallelScale").Set(this->Internal->getParallelScale());

  keyFrame->GetProperty("PositionPathPoints")
    ->Copy(this->Internal->PSplineProxy->GetProperty("Points"));

  keyFrame->GetProperty("FocalPathPoints")
    ->Copy(this->Internal->FSplineProxy->GetProperty("Points"));

  keyFrame->GetProperty("ClosedPositionPath")
    ->Copy(this->Internal->PSplineProxy->GetProperty("Closed"));

  keyFrame->GetProperty("ClosedFocalPath")
    ->Copy(this->Internal->FSplineProxy->GetProperty("Closed"));
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
  else if (currentItem->text(0) == "Camera Focus")
  {
    this->Internal->stackedWidget->setCurrentIndex(2);
  }
  else
  {
    this->Internal->stackedWidget->setCurrentIndex(3);
  }
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
  this->Internal->leftPane->setCurrentItem(nullptr);
}
