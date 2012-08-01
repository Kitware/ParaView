/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqThreeSliceView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqThreeSliceView.h"

#include <QtCore>
#include <QtGui>

#include "QVTKWidget.h"

#include "pqRepresentation.h"
#include "pqServer.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVRenderView.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkView.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <vector>

//----------------------------------------------------------------------------
struct pqThreeSliceView::pqInternal
{
  QPointer<QVTKWidget> InternalWidget;
  QPointer<QVTKWidget> InternalWidgetX;
  QPointer<QVTKWidget> InternalWidgetY;
  QPointer<QVTKWidget> InternalWidgetZ;

  QPointer<pqRenderView> AxisX;
  QPointer<pqRenderView> AxisY;
  QPointer<pqRenderView> AxisZ;

  vtkSmartPointer<vtkSMViewProxy> ViewProxy;
  vtkSmartPointer<vtkSMRenderViewProxy> ViewProxyForX;
  vtkSmartPointer<vtkSMRenderViewProxy> ViewProxyForY;
  vtkSmartPointer<vtkSMRenderViewProxy> ViewProxyForZ;

  std::vector<vtkSmartPointer<vtkSMPropertyLink> > Links;

  // ------

  void AddLink(const char* property)
  {
    vtkNew<vtkSMPropertyLink> link;
    link->AddLinkedProperty(this->ViewProxy, property, vtkSMLink::INPUT);
    link->AddLinkedProperty(this->ViewProxyForX, property, vtkSMLink::OUTPUT);
    link->AddLinkedProperty(this->ViewProxyForY, property, vtkSMLink::OUTPUT);
    link->AddLinkedProperty(this->ViewProxyForZ, property, vtkSMLink::OUTPUT);

    // Keep it with auto-delete
    this->Links.push_back(link.GetPointer());
  }
};

//-----------------------------------------------------------------------------
pqThreeSliceView::pqThreeSliceView(
    const QString& viewType, const QString& group, const QString& name,
    vtkSMViewProxy* viewProxy, pqServer* server, QObject* p)
  : pqRenderView(viewType, group, name, viewProxy, server, p)
{
  this->Internal = new pqInternal();
  this->Internal->ViewProxy = viewProxy;

  this->Internal->ViewProxyForX =
      vtkSMRenderViewProxy::SafeDownCast(
        server->proxyManager()->NewProxy("views", "2DRenderView"));
  this->Internal->ViewProxyForX->FastDelete();
  this->Internal->ViewProxyForY =
      vtkSMRenderViewProxy::SafeDownCast(
        server->proxyManager()->NewProxy("views", "2DRenderView"));
  this->Internal->ViewProxyForY->FastDelete();
  this->Internal->ViewProxyForZ =
      vtkSMRenderViewProxy::SafeDownCast(
        server->proxyManager()->NewProxy("views", "2DRenderView"));
  this->Internal->ViewProxyForZ->FastDelete();

  this->Internal->AxisX =
      new pqRenderView(viewType, group, name, this->Internal->ViewProxyForX, server, this);
  this->Internal->AxisY =
      new pqRenderView(viewType, group, name, this->Internal->ViewProxyForY, server, this);
  this->Internal->AxisZ =
      new pqRenderView(viewType, group, name, this->Internal->ViewProxyForZ, server, this);

  // Bind properties across views
  this->Internal->AddLink("UseLight");
  this->Internal->AddLink("LightSwitch");
  this->Internal->AddLink("CenterAxesVisibility");
  this->Internal->AddLink("OrientationAxesVisibility");
  this->Internal->AddLink("KeyLightWarmth");
  this->Internal->AddLink("KeyLightIntensity");
  this->Internal->AddLink("KeyLightElevation");
  this->Internal->AddLink("KeyLightAzimuth");
  this->Internal->AddLink("FillLightWarmth");
  this->Internal->AddLink("FillLightK:F Ratio");
  this->Internal->AddLink("FillLightElevation");
  this->Internal->AddLink("FillLightAzimuth");
  this->Internal->AddLink("BackLightWarmth");
  this->Internal->AddLink("BackLightK:B Ratio");
  this->Internal->AddLink("BackLightElevation");
  this->Internal->AddLink("BackLightAzimuth");
  this->Internal->AddLink("HeadLightWarmth");
  this->Internal->AddLink("HeadLightK:H Ratio");
  this->Internal->AddLink("MaintainLuminance");
  this->Internal->AddLink("Background");
  this->Internal->AddLink("Background2");
  this->Internal->AddLink("BackgroundTexture");
  this->Internal->AddLink("UseGradientBackground");
  this->Internal->AddLink("UseTexturedBackground");
  this->Internal->AddLink("LightAmbientColor");
  this->Internal->AddLink("LightSpecularColor");
  this->Internal->AddLink("LightDiffuseColor");
  this->Internal->AddLink("LightIntensity");
  this->Internal->AddLink("LightType");

  // TMP for debuging/testing
  this->Internal->AddLink("Representations");
  this->Internal->AddLink("ResetCamera");
}

//-----------------------------------------------------------------------------
pqThreeSliceView::~pqThreeSliceView()
{
  delete this->Internal;
  this->Internal = NULL;
}

//-----------------------------------------------------------------------------
QWidget* pqThreeSliceView::createWidget()
{
  // Get the internal widget that we want to decorate
  this->Internal->InternalWidget = qobject_cast<QVTKWidget*>(this->pqRenderView::createWidget());
  this->Internal->InternalWidgetX = qobject_cast<QVTKWidget*>(this->Internal->AxisX->getWidget());
  this->Internal->InternalWidgetY = qobject_cast<QVTKWidget*>(this->Internal->AxisY->getWidget());
  this->Internal->InternalWidgetZ = qobject_cast<QVTKWidget*>(this->Internal->AxisZ->getWidget());

  // Build the widget hierarchy
  QWidget* container = new QWidget();
  container->setStyleSheet("background-color: white");
  container->setAutoFillBackground(true);

  QGridLayout* gridLayout = new QGridLayout(container);
  this->Internal->InternalWidget->setParent(container);

  gridLayout->addWidget(this->Internal->InternalWidgetX, 0, 0); // TOP-LEFT
  gridLayout->addWidget(this->Internal->InternalWidgetY, 0, 1); // TOP-RIGHT
  gridLayout->addWidget(this->Internal->InternalWidgetZ, 1, 0); // BOTTOM-LEFT
  gridLayout->addWidget(this->Internal->InternalWidget,  1, 1);    // BOTTOM-RIGHT
  gridLayout->setContentsMargins(0,0,0,0);
  gridLayout->setSpacing(0);

  // Properly do the binding between the proxy and the 3D widget
  vtkSMRenderViewProxy* renModule = this->getRenderViewProxy();
  if (this->Internal->InternalWidget && renModule)
    {
    this->Internal->InternalWidget->SetRenderWindow(renModule->GetRenderWindow());
    }

  // For axis
  this->Internal->InternalWidgetX->SetRenderWindow(this->Internal->ViewProxyForX->GetRenderWindow());
  this->Internal->InternalWidgetY->SetRenderWindow(this->Internal->ViewProxyForY->GetRenderWindow());
  this->Internal->InternalWidgetZ->SetRenderWindow(this->Internal->ViewProxyForZ->GetRenderWindow());

  // Init orientation
  double cameraPosition[3];
  double viewUp[3] = {0,0,0};

  // Update X
  vtkSMPropertyHelper(this->Internal->ViewProxyForX, "CameraFocalPointInfo").Get(cameraPosition, 3);
  cameraPosition[0] += 100;
  viewUp[1] = 1;
  vtkSMPropertyHelper(this->Internal->ViewProxyForX, "CameraPosition").Set(cameraPosition, 3);
  vtkSMPropertyHelper(this->Internal->ViewProxyForX, "CameraViewUp").Set(viewUp, 3);
  this->Internal->ViewProxyForX->InvokeCommand("ResetCamera");

  // Update Z
  vtkSMPropertyHelper(this->Internal->ViewProxyForZ, "CameraFocalPointInfo").Get(cameraPosition, 3);
  cameraPosition[2] += 100;
  vtkSMPropertyHelper(this->Internal->ViewProxyForZ, "CameraPosition").Set(cameraPosition, 3);
  vtkSMPropertyHelper(this->Internal->ViewProxyForZ, "CameraViewUp").Set(viewUp, 3);
  this->Internal->ViewProxyForZ->InvokeCommand("ResetCamera");

  // Update Y
  vtkSMPropertyHelper(this->Internal->ViewProxyForY, "CameraFocalPointInfo").Get(cameraPosition, 3);
  cameraPosition[1] += 100;
  viewUp[1] = 0;
  viewUp[2] = 1;
  vtkSMPropertyHelper(this->Internal->ViewProxyForY, "CameraPosition").Set(cameraPosition, 3);
  vtkSMPropertyHelper(this->Internal->ViewProxyForY, "CameraViewUp").Set(viewUp, 3);
  this->Internal->ViewProxyForY->InvokeCommand("ResetCamera");

  return container;
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::resetCamera()
{
  this->pqRenderView::resetCamera();

  this->Internal->ViewProxyForX->InvokeCommand("ResetCamera");
  this->Internal->ViewProxyForY->InvokeCommand("ResetCamera");
  this->Internal->ViewProxyForZ->InvokeCommand("ResetCamera");
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::render()
{
  this->pqRenderView::render();

  this->Internal->AxisX->render();
  this->Internal->AxisY->render();
  this->Internal->AxisZ->render();
}
