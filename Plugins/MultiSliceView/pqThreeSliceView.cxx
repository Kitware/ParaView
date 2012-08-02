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
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPVRenderView.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkView.h"

#include <vector>

//----------------------------------------------------------------------------
class pqThreeSliceView::pqInternal
{
public:
  QPointer<QVTKWidget> MainWidget;
  QPointer<QVTKWidget> TopLeftWidget;
  QPointer<QVTKWidget> TopRightWidget;
  QPointer<QVTKWidget> BottomLeftWidget;

  QPointer<pqRenderView> TopLeftRenderer;
  QPointer<pqRenderView> TopRightRenderer;
  QPointer<pqRenderView> BottomLeftRenderer;

  vtkSmartPointer<vtkSMViewProxy> ViewProxy;
  vtkSmartPointer<vtkSMRenderViewProxy> TopLeftProxy;
  vtkSmartPointer<vtkSMRenderViewProxy> TopRightProxy;
  vtkSmartPointer<vtkSMRenderViewProxy> BottomLeftProxy;

  std::vector<vtkSmartPointer<vtkSMPropertyLink> > Links;

  double TopLeftNormal[3];
  double TopRightNormal[3];
  double BottomLeftNormal[3];
  double TopLeftViewUp[3];
  double TopRightViewUp[3];
  double BottomLeftViewUp[3];
  double SlicesOrigin[3];

  // ------

  void AddLink(const char* property)
  {
    vtkNew<vtkSMPropertyLink> link;
    link->AddLinkedProperty(this->ViewProxy, property, vtkSMLink::INPUT);
    link->AddLinkedProperty(this->TopLeftProxy, property, vtkSMLink::OUTPUT);
    link->AddLinkedProperty(this->TopRightProxy, property, vtkSMLink::OUTPUT);
    link->AddLinkedProperty(this->BottomLeftProxy, property, vtkSMLink::OUTPUT);

    // Keep it with auto-delete
    this->Links.push_back(link.GetPointer());
  }

  // ------

  void ResetToDefault()
  {
    // Reset to 0.0 everywhere
    for(int i=0;i<3;i++)
      {
      this->SlicesOrigin[i]
          = this->TopLeftNormal[i] = this->TopLeftViewUp[i]
          = this->TopRightNormal[i] = this->TopRightViewUp[i]
          = this->BottomLeftNormal[i] = this->BottomLeftViewUp[i] = 0.0;
      }

    // Set 1.0 only where it is necessery
    this->TopLeftNormal[0] = this->TopLeftViewUp[1]
        = this->TopRightNormal[1] = this->TopRightViewUp[2]
        = this->BottomLeftNormal[2] = this->BottomLeftViewUp[1]
        = 1.0;
  }

  // ------

  void UpdateSlices()
  {
    // Init orientation
    double cameraPosition[3] = {0,0,0};
    const char* focalPoint = "CameraFocalPointInfo";
    const char* position   = "CameraPosition";
    const char* viewUp     = "CameraViewUp";

    // Update X (Top-Left)
    vtkSMPropertyHelper(this->TopLeftProxy, focalPoint).Get(cameraPosition, 3);
    vtkMath::Add(cameraPosition, this->TopLeftNormal, cameraPosition);
    vtkSMPropertyHelper(this->TopLeftProxy, position).Set(cameraPosition, 3);
    vtkSMPropertyHelper(this->TopLeftProxy, viewUp).Set(this->TopLeftViewUp, 3);
    this->TopLeftProxy->InvokeCommand("ResetCamera");

    // Update Y (Top-Right)
    vtkSMPropertyHelper(this->TopRightProxy, focalPoint).Get(cameraPosition, 3);
    vtkMath::Add(cameraPosition, this->TopRightNormal, cameraPosition);
    vtkSMPropertyHelper(this->TopRightProxy, position).Set(cameraPosition, 3);
    vtkSMPropertyHelper(this->TopRightProxy, viewUp).Set(this->TopRightViewUp, 3);
    this->TopRightProxy->InvokeCommand("ResetCamera");

    // Update Z (Bottom-Left)
    vtkSMPropertyHelper(this->BottomLeftProxy, focalPoint).Get(cameraPosition, 3);
    vtkMath::Add(cameraPosition, this->BottomLeftNormal, cameraPosition);
    vtkSMPropertyHelper(this->BottomLeftProxy, position).Set(cameraPosition, 3);
    vtkSMPropertyHelper(this->BottomLeftProxy, viewUp).Set(this->BottomLeftViewUp, 3);
    this->BottomLeftProxy->InvokeCommand("ResetCamera");
  }

  // ------

  void Initialize(pqServer* server, const QString& viewType, const QString& group,
                  const QString& name, vtkSMViewProxy* mainViewProxy, QObject* p)
  {
    vtkSMSessionProxyManager* spxm = server->proxyManager();
    const char* g = "views";
    const char* t = "2DRenderView";

    this->ResetToDefault();

    this->ViewProxy = mainViewProxy;

    this->TopLeftProxy = vtkSMRenderViewProxy::SafeDownCast(spxm->NewProxy(g,t));
    this->TopLeftProxy->FastDelete();

    this->TopRightProxy = vtkSMRenderViewProxy::SafeDownCast(spxm->NewProxy(g,t));
    this->TopRightProxy->FastDelete();

    this->BottomLeftProxy = vtkSMRenderViewProxy::SafeDownCast(spxm->NewProxy(g,t));
    this->BottomLeftProxy->FastDelete();

    this->TopLeftRenderer = new pqRenderView(viewType, group, name, this->TopLeftProxy, server, p);
    this->TopRightRenderer = new pqRenderView(viewType, group, name, this->TopRightProxy, server, p);
    this->BottomLeftRenderer = new pqRenderView(viewType, group, name, this->BottomLeftProxy, server, p);
  }

  // ------

  QWidget* CreateWidget(pqThreeSliceView* view)
  {
    // Get the internal widget that we want to decorate
    this->MainWidget = qobject_cast<QVTKWidget*>(view->pqRenderView::createWidget());
    this->TopLeftWidget = qobject_cast<QVTKWidget*>(this->TopLeftRenderer->getWidget());
    this->TopRightWidget = qobject_cast<QVTKWidget*>(this->TopRightRenderer->getWidget());
    this->BottomLeftWidget = qobject_cast<QVTKWidget*>(this->BottomLeftRenderer->getWidget());

    // Build the widget hierarchy
    QWidget* container = new QWidget();
    container->setStyleSheet("background-color: white");
    container->setAutoFillBackground(true);

    QGridLayout* gridLayout = new QGridLayout(container);
    this->MainWidget->setParent(container);

    gridLayout->addWidget(this->TopLeftWidget,    0, 0); // TOP-LEFT
    gridLayout->addWidget(this->TopRightWidget,   0, 1); // TOP-RIGHT
    gridLayout->addWidget(this->BottomLeftWidget, 1, 0); // BOTTOM-LEFT
    gridLayout->addWidget(this->MainWidget,       1, 1); // BOTTOM-RIGHT
    gridLayout->setContentsMargins(0,0,0,0);
    gridLayout->setSpacing(0);

    // Properly do the binding between the proxy and the 3D widget
    vtkSMRenderViewProxy* renModule = view->getRenderViewProxy();
    if (this->MainWidget && renModule)
      {
      this->MainWidget->SetRenderWindow(renModule->GetRenderWindow());
      }

    // For axis
    this->TopLeftWidget->SetRenderWindow(this->TopLeftProxy->GetRenderWindow());
    this->TopRightWidget->SetRenderWindow(this->TopRightProxy->GetRenderWindow());
    this->BottomLeftWidget->SetRenderWindow(this->BottomLeftProxy->GetRenderWindow());

    return container;
  }
};

//-----------------------------------------------------------------------------
pqThreeSliceView::pqThreeSliceView(
    const QString& viewType, const QString& group, const QString& name,
    vtkSMViewProxy* viewProxy, pqServer* server, QObject* p)
  : pqRenderView(viewType, group, name, viewProxy, server, p)
{
  this->Internal = new pqInternal();
  this->Internal->Initialize(server, viewType, group, name, viewProxy, this);

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
  // Create the widget with all the axes
  QWidget* container = this->Internal->CreateWidget(this);

  // Make sure the view are normal to the slices
  this->Internal->UpdateSlices();

  return container;
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::resetCamera()
{
  this->pqRenderView::resetCamera();

  this->Internal->TopLeftProxy->InvokeCommand("ResetCamera");
  this->Internal->TopRightProxy->InvokeCommand("ResetCamera");
  this->Internal->BottomLeftProxy->InvokeCommand("ResetCamera");
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::render()
{
  this->pqRenderView::render();

  this->Internal->TopLeftRenderer->render();
  this->Internal->TopRightRenderer->render();
  this->Internal->BottomLeftRenderer->render();
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getTopLeftNormal() const
{
  return this->Internal->TopLeftNormal;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getTopRightNormal() const
{
  return this->Internal->TopRightNormal;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getBottomLeftNormal() const
{
  return this->Internal->BottomLeftNormal;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getTopLeftViewUp() const
{
  return this->Internal->TopLeftViewUp;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getTopRightViewUp() const
{
  return this->Internal->TopRightViewUp;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getBottomLeftViewUp() const
{
  return this->Internal->BottomLeftViewUp;
}
//-----------------------------------------------------------------------------
const double* pqThreeSliceView::getSlicesOrigin() const
{
  return this->Internal->SlicesOrigin;
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setTopLeftNormal(double x, double y, double z)
{
  this->Internal->TopLeftNormal[0] = x;
  this->Internal->TopLeftNormal[1] = y;
  this->Internal->TopLeftNormal[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setTopRightNormal(double x, double y, double z)
{
  this->Internal->TopRightNormal[0] = x;
  this->Internal->TopRightNormal[1] = y;
  this->Internal->TopRightNormal[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setBottomLeftNormal(double x, double y, double z)
{
  this->Internal->BottomLeftNormal[0] = x;
  this->Internal->BottomLeftNormal[1] = y;
  this->Internal->BottomLeftNormal[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setTopLeftViewUp(double x, double y, double z)
{
  this->Internal->TopLeftViewUp[0] = x;
  this->Internal->TopLeftViewUp[1] = y;
  this->Internal->TopLeftViewUp[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setTopRightViewUp(double x, double y, double z)
{
  this->Internal->TopRightViewUp[0] = x;
  this->Internal->TopRightViewUp[1] = y;
  this->Internal->TopRightViewUp[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setBottomLeftViewUp(double x, double y, double z)
{
  this->Internal->BottomLeftViewUp[0] = x;
  this->Internal->BottomLeftViewUp[1] = y;
  this->Internal->BottomLeftViewUp[2] = z;

  this->Internal->UpdateSlices();
}
//-----------------------------------------------------------------------------
void pqThreeSliceView::setSlicesOrigin(double x, double y, double z)
{
  this->Internal->SlicesOrigin[0] = x;
  this->Internal->SlicesOrigin[1] = y;
  this->Internal->SlicesOrigin[2] = z;

  this->Internal->UpdateSlices();
}

//-----------------------------------------------------------------------------
void pqThreeSliceView::resetDefaultSettings()
{
  this->Internal->ResetToDefault();
  this->Internal->UpdateSlices();
}
