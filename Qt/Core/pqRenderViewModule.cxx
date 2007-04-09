/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewModule.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqRenderViewModule.h"

// ParaView Server Manager includes.
#include "QVTKWidget.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkSmartPointer.h"
#include "vtkSMDisplayProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkTrackballPan.h"
#include "vtkSMUndoStack.h"
#include "vtkSMInteractionUndoStackBuilder.h"

// Qt includes.
#include <QFileInfo>
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QSet>
#include <QPrinter>
#include <QPainter>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDisplay.h"
#include "pqLinkViewWidget.h"
#include "pqPipelineSource.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "vtkPVAxesWidget.h"

class pqRenderViewModuleInternal
{
public:
  QPointer<QVTKWidget> Viewport;
  QPoint MouseOrigin;
  vtkSmartPointer<pqRenderViewProxy> RenderViewProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMRenderModuleProxy> RenderModuleProxy;
  vtkSmartPointer<vtkPVAxesWidget> OrientationAxesWidget;
  vtkSmartPointer<vtkSMProxy> CenterAxesProxy;
  vtkSmartPointer<vtkSMProxy> InteractorStyleProxy;

  vtkSmartPointer<vtkSMUndoStack> InteractionUndoStack;
  vtkSmartPointer<vtkSMInteractionUndoStackBuilder> UndoStackBuilder;

  QList<QPointer<pqRenderViewModule> > LinkedUndoStacks;
  bool UpdatingStack;

  int DefaultBackground[3];
  bool InitializedWidgets;

  pqRenderViewModuleInternal()
    {
    this->UpdatingStack = false;
    this->InitializedWidgets = false;
    this->Viewport = 0;
    this->RenderViewProxy = vtkSmartPointer<pqRenderViewProxy>::New();
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->OrientationAxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();
    this->InteractorStyleProxy = 0;
    this->DefaultBackground[0] = 84;
    this->DefaultBackground[1] = 89;
    this->DefaultBackground[2] = 109;

    this->InteractionUndoStack = vtkSmartPointer<vtkSMUndoStack>::New();
    this->InteractionUndoStack->SetClientOnly(true);
    this->UndoStackBuilder = 
      vtkSmartPointer<vtkSMInteractionUndoStackBuilder>::New();
    this->UndoStackBuilder->SetUndoStack(
      this->InteractionUndoStack);
    }

  ~pqRenderViewModuleInternal()
    {
    this->RenderViewProxy->setRenderModule(0);
    }
};

//-----------------------------------------------------------------------------
pqRenderViewModule::pqRenderViewModule(const QString& name, 
  vtkSMRenderModuleProxy* renModule, pqServer* server, QObject* _parent/*=null*/)
: pqGenericViewModule(
  renderViewType(), "view_modules", name, renModule, server, _parent)
{
  this->Internal = new pqRenderViewModuleInternal();
  this->Internal->RenderViewProxy->setRenderModule(this);
  this->Internal->RenderModuleProxy = renModule;

  // we need to fire signals when undo stack changes.
  this->Internal->VTKConnect->Connect(this->Internal->InteractionUndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onUndoStackChanged()),
    0, 0, Qt::QueuedConnection);

  this->Internal->Viewport = new QVTKWidget();
  this->Internal->Viewport->setObjectName("Viewport");
  // we manage the context menu ourself, so it doesn't interfere with
  // render window interactions
  this->Internal->Viewport->setContextMenuPolicy(Qt::NoContextMenu);
  
  // add a link view menu
  QAction* act = new QAction("Link Camera...", this);
  this->addMenuAction(act);
  QObject::connect(act, SIGNAL(triggered(bool)),
                   this, SLOT(linkToOtherView()));

  // do image caching for performance
  // For now, we are doing this only on Apple because it can render
  // and capture a frame buffer even when it is obstructred by a
  // window. This does not work as well on other platforms.
#if defined(__APPLE__)
  this->Internal->Viewport->setAutomaticImageCacheEnabled(true);
#endif

  this->Internal->Viewport->installEventFilter(this);

  this->ResetCenterWithCamera = true;

  // help the QVTKWidget know when to clear the cache
  this->Internal->VTKConnect->Connect(
    renModule, vtkCommand::ModifiedEvent,
    this->Internal->Viewport, SLOT(markCachedImageAsDirty()));  

  this->Internal->VTKConnect->Connect(
    renModule, vtkCommand::ResetCameraEvent,
    this, SLOT(onResetCameraEvent()));

  // We need to listen to events from the render module's interactor
  // style. So whenever it changes, we set up observers.
  this->Internal->VTKConnect->Connect(
    renModule->GetProperty("InteractorStyle"), vtkCommand::ModifiedEvent,
    this, SLOT(onInteractorStyleChanged()));
  
  // If there is a InteractorStyle, initialize it.
  this->onInteractorStyleChanged();


  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the QVTKWidget
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we 
  // do the following.
  if (!renModule->GetObjectsCreated())
    {
    // Wait till first UpdateVTKObjects() call on the render module.
    // Under usual circumstances, after UpdateVTKObjects() the
    // render module objects will be created.
    this->Internal->VTKConnect->Connect(
      renModule, vtkCommand::UpdateEvent,
      this, SLOT(initializeWidgets()));
    }
  else
    {
    this->initializeWidgets();
    }
}

//-----------------------------------------------------------------------------
pqRenderViewModule::~pqRenderViewModule()
{

  delete this->Internal->Viewport;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy* pqRenderViewModule::getRenderModuleProxy() const
{
  return this->Internal->RenderModuleProxy;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderViewModule::getWidget()
{
  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
// This method is called for all pqRenderViewModule objects irrespective
// of whether it is created from state/undo-redo/python or by the GUI. Hence
// don't change any render module properties here.
void pqRenderViewModule::initializeWidgets()
{
  if (this->Internal->InitializedWidgets)
    {
    return;
    }

  this->Internal->InitializedWidgets = true;
  // Disconnect old slots.
  // this->Internal->VTKConnect->Disconnect(
  //   this->Internal->RenderModuleProxy, vtkCommand::UpdateEvent);

  vtkSMRenderModuleProxy* renModule =
    this->Internal->RenderModuleProxy;

  this->Internal->Viewport->SetRenderWindow(
    renModule->GetRenderWindow());

  // Enable interaction on this client.
  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      renModule->GetInteractor());
  iren->SetPVRenderView(this->Internal->RenderViewProxy);

  // Init axes actor.
  this->Internal->OrientationAxesWidget->SetParentRenderer(
    renModule->GetRenderer());
  this->Internal->OrientationAxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->OrientationAxesWidget->SetInteractor(iren);
  this->Internal->OrientationAxesWidget->SetEnabled(1);
  this->Internal->OrientationAxesWidget->SetInteractive(0);

  iren->Enable();

  // setup the center axes.
  this->initializeCenterAxes();

  this->Internal->UndoStackBuilder->SetRenderModule(renModule);
}

//-----------------------------------------------------------------------------
// Called when the "InteractorStyle" property on the render module proxy changes.
// We end up observers on the interactor style to know about
// start and end of interaction.
void pqRenderViewModule::onInteractorStyleChanged()
{
  if (this->Internal->InteractorStyleProxy)
    {
    // remove observers from old interactor style.
    this->Internal->VTKConnect->Disconnect(
      this->Internal->InteractorStyleProxy);
    this->Internal->InteractorStyleProxy = 0;
    }

  this->Internal->InteractorStyleProxy = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("InteractorStyle"));

  if (this->Internal->InteractorStyleProxy.GetPointer())
    {
    vtkProcessModule* pvm = vtkProcessModule::GetProcessModule();
    vtkObject *style = vtkObject::SafeDownCast(pvm->GetObjectFromID(
      this->Internal->InteractorStyleProxy->GetID(0)));
    this->Internal->VTKConnect->Connect(style,
      vtkCommand::StartInteractionEvent, 
      this, SLOT(startInteraction()));
    this->Internal->VTKConnect->Connect(style,
      vtkCommand::EndInteractionEvent, 
      this, SLOT(endInteraction()));
    }
}

//-----------------------------------------------------------------------------
// Sets default values for the underlying proxy.  This is during the 
// initialization stage of the pqProxy for proxies created by the GUI itself 
// i.e. for proxies loaded through state or created by python client or 
// undo/redo, this method won't be called. 
void pqRenderViewModule::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();

  this->createDefaultInteractors();

  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODResolution"), 50);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODThreshold"), 5);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("CompositeThreshold"), 3);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("SquirtLevel"), 3);

  vtkSMProperty* backgroundProperty;
  int* bg = this->defaultBackgroundColor();
  backgroundProperty = proxy->GetProperty("Background");
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 0, bg[0]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 1, bg[1]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 2, bg[2]/255.0);

  proxy->UpdateVTKObjects();

  this->restoreSettings();
  this->resetCamera();
}

//-----------------------------------------------------------------------------
// This method gets called only with the object is directly created by the GUI
// i.e. it wont get called when the proxy is loaded from state/undo/redo or 
// python.
// TODO: Python paraview modules createView() equivalent should make sure
// that it sets up some default interactor.
void pqRenderViewModule::createDefaultInteractors()
{
  // Create the interactor style proxy:
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkIdType cid = this->getServer()->GetConnectionID();

  vtkSMProxy* interactorStyle = 
    pxm->NewProxy("interactorstyles", "InteractorStyle");
  this->Internal->InteractorStyleProxy.TakeReference(interactorStyle);
  interactorStyle->SetConnectionID(cid);
  interactorStyle->SetServers(vtkProcessModule::CLIENT);
  this->addHelperProxy("InteractorStyles", interactorStyle);

  vtkSMProperty *styleManips = 
    interactorStyle->GetProperty("CameraManipulators");

  // Create and register manipulators, then add to interactor style

  // LeftButton -- Rotate
  vtkSMProxy *manip = pxm->NewProxy("cameramanipulators", "TrackballRotate");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 1);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // Shift + LeftButton  -- Roll.
  manip = pxm->NewProxy("cameramanipulators", "TrackballRoll");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 1);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Shift"), 1);
  this->addHelperProxy("Manipulators", manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // Control + LeftButton -- FlyIn
  // manip = pxm->NewProxy("cameramanipulators", "JoystickFly2");
  manip = pxm->NewProxy("cameramanipulators", "TrackballZoom");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 1);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Control"), 1);
  // pqSMAdaptor::setElementProperty(manip->GetProperty("In"), 1);
  this->addHelperProxy("Manipulators", manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // MiddleButton -- Pan
  manip = pxm->NewProxy("cameramanipulators", "TrackballPan1");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 2);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // Shift + MiddleButton -- Rotate
  manip = pxm->NewProxy("cameramanipulators", "TrackballRotate");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 2);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Shift"), 1);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // Control + MiddleButton -- Rotate
  manip = pxm->NewProxy("cameramanipulators", "TrackballRotate");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 2);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Control"), 1);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();
  
  // RightButton -- Zoom
  manip = pxm->NewProxy("cameramanipulators", "TrackballZoom");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 3);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  // Shift + RightButton -- Pan
  manip = pxm->NewProxy("cameramanipulators", "TrackballPan1");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 3);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Shift"), 1);
  this->addHelperProxy("Manipulators",manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();
  
  // Control + RightButton -- FlyOut
  // manip = pxm->NewProxy("cameramanipulators", "JoystickFly2");
  manip = pxm->NewProxy("cameramanipulators", "TrackballZoom");
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), 3);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Control"), 1);
  // pqSMAdaptor::setElementProperty(manip->GetProperty("In"), 0);
  this->addHelperProxy("Manipulators", manip);
  pqSMAdaptor::addProxyProperty(styleManips, manip);
  manip->UpdateVTKObjects();
  manip->Delete();

  interactorStyle->UpdateVTKObjects();

  // Set interactor style on the render module.
  // This will trigger a call to onInteractorStyleChanged() which
  // updates the observers etc.
  pqSMAdaptor::setProxyProperty(
    this->Internal->RenderModuleProxy->GetProperty("InteractorStyle"),
    interactorStyle);
  this->Internal->RenderModuleProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
// Create a center axes if one doesn't already exist.
void pqRenderViewModule::initializeCenterAxes()
{
  if (this->Internal->CenterAxesProxy.GetPointer())
    {
    // sanity check.
    return;
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* centerAxes = pxm->NewProxy("axes", "Axes");
  centerAxes->SetConnectionID(this->getServer()->GetConnectionID());
  QList<QVariant> scaleValues;
  scaleValues << .25 << .25 << .25;
  pqSMAdaptor::setMultipleElementProperty(
    centerAxes->GetProperty("Scale"), scaleValues);
  pqSMAdaptor::setElementProperty(centerAxes->GetProperty("Pickable"), 0);
  centerAxes->UpdateVTKObjects();
  this->Internal->CenterAxesProxy = centerAxes;
  
  // Add to render module without using properties. That way it does not
  // get saved in state.
  this->Internal->RenderModuleProxy->AddDisplay(
    vtkSMDisplayProxy::SafeDownCast(centerAxes));
  centerAxes->Delete();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::startInteraction()
{
  vtkPVGenericRenderWindowInteractor* iren = 
      this->Internal->RenderModuleProxy->GetInteractor();
  iren->SetInteractiveRenderEnabled(true);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::endInteraction()
{
  vtkPVGenericRenderWindowInteractor* iren = 
      this->Internal->RenderModuleProxy->GetInteractor();
  iren->SetInteractiveRenderEnabled(false);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::render()
{
  if (this->Internal->RenderModuleProxy && this->Internal->Viewport)
    {
    this->Internal->Viewport->update();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::onResetCameraEvent()
{
  if (this->ResetCenterWithCamera)
    {
    this->resetCenterOfRotation();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::resetCamera()
{
  if (this->Internal->RenderModuleProxy)
    {
    this->Internal->RenderModuleProxy->ResetCamera();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::resetCenterOfRotation()
{
  // Update center of rotation.
  this->Internal->RenderModuleProxy->UpdatePropertyInformation();
  QList<QVariant> values = 
    pqSMAdaptor::getMultipleElementProperty(
      this->Internal->RenderModuleProxy->GetProperty("CameraFocalPointInfo"));
  this->setCenterOfRotation(
    values[0].toDouble(), values[1].toDouble(), values[2].toDouble());
}

//-----------------------------------------------------------------------------
vtkImageData* pqRenderViewModule::captureImage(int magnification)
{
  return this->Internal->RenderModuleProxy->CaptureWindow(magnification);
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::saveImage(int width, int height, const QString& filename)
{
  QSize cur_size = this->Internal->Viewport->size();
  if (width>0 && height>0)
    {
    this->Internal->Viewport->resize(width, height);
    }
  this->render();
  const char* writername = 0;

  const QFileInfo file(filename);
  if(file.completeSuffix() == "bmp")
    {
    writername = "vtkBMPWriter";
    }
  else if(file.completeSuffix() == "tif" || file.completeSuffix() == "tiff")
    {
    writername = "vtkTIFFWriter"; 
    }
  else if(file.completeSuffix() == "ppm")
    {
    writername = "vtkPNMWriter";
    }
  else if(file.completeSuffix() == "png")
    {
    writername = "vtkPNGWriter";
    }
  else if(file.completeSuffix() == "jpg")
    {
    writername = "vtkJPEGWriter";
    }
  else if(file.completeSuffix() == "pdf")
    {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filename);
    
    QPixmap pix = QPixmap::grabWidget(this->Internal->Viewport);
    QPainter painter;
    painter.begin(&printer);
    QSize viewport_size(pix.rect().size());
    viewport_size.scale(printer.pageRect().size(), Qt::KeepAspectRatio);
    painter.setWindow(pix.rect());
    painter.setViewport(QRect(0,0, viewport_size.width(),
        viewport_size.height()));
    painter.drawPixmap(QPointF(0.0, 0.0), pix);
    painter.end();

    return true;
    }
  else
    {
    qCritical() << "Failed to determine file type for file:" 
      << filename.toAscii().data();
    return false;
    }

  int ret = this->Internal->RenderModuleProxy->WriteImage(
    filename.toAscii().data(), writername);
  if (width>0 && height>0)
    {
    this->Internal->Viewport->resize(width, height);
    this->Internal->Viewport->resize(cur_size);
    this->render();
    }
  return (ret == vtkErrorCode::NoError);
}

//-----------------------------------------------------------------------------
int* pqRenderViewModule::defaultBackgroundColor()
{
  return this->Internal->DefaultBackground;
}

//-----------------------------------------------------------------------------
static const char* pqRenderViewModuleLightSettings [] = {
  "LightSwitch",
  "LightIntensity",
  "UseLight",
  "KeyLightWarmth",
  "KeyLightIntensity",
  "KeyLightElevation",
  "KeyLightAzimuth",
  "FillLightWarmth",
  "FillLightK:F Ratio",
  "FillLightElevation",
  "FillLightAzimuth",
  "BackLightWarmth",
  "BackLightK:B Ratio",
  "BackLightElevation",
  "BackLightAzimuth",
  "HeadLightWarmth",
  "HeadLightK:H Ratio",
  "MaintainLuminance",
  NULL
  };

static const char* pqRenderViewModuleMiscSettings [] = {
  "CacheLimit",
  "CameraParallelProjection",
  "UseTriangleStrips",
  "UseImmediateMode",
  "LODThreshold",
  "LODResolution",
  "RenderInterruptsEnabled",
  "CompositeThreshold",
  "ReductionFactor",
  "SquirtLevel",
  "OrderedCompositing",
  "CollectGeometryThreshold",
  "StillReductionFactor",
  NULL
  };


static const char** pqRenderViewModuleSettings[] = {
  pqRenderViewModuleLightSettings,
  pqRenderViewModuleMiscSettings,
  NULL
  };

static const char* pqRenderViewModuleLightSettingsMulti[] = {
  "LightDiffuseColor",
  NULL  // keep last
  };

static const char* pqRenderViewModuleMiscSettingsMulti[] = {
  "Background",
  NULL  // keep last
  };

static const char** pqRenderViewModuleSettingsMulti[] = {
  pqRenderViewModuleLightSettingsMulti,
  pqRenderViewModuleMiscSettingsMulti,
  NULL  // keep last
};

//-----------------------------------------------------------------------------
void pqRenderViewModule::restoreSettings()
{
  vtkSMProxy* proxy = this->getProxy();

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();

  const char*** str;

  for(str=pqRenderViewModuleSettings; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = QString("renderModule/") + *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop && settings->contains(key))
        {
        pqSMAdaptor::setElementProperty(prop, settings->value(key));
        }
      }
    }
  for(str=pqRenderViewModuleSettingsMulti; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = QString("renderModule/") + *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop && settings->contains(key))
        {
        QList<QVariant> value = settings->value(key).value<QList<QVariant> >();
        pqSMAdaptor::setMultipleElementProperty(prop, value);
        }
      }
    }
  proxy->UpdateVTKObjects();

  // Orientation Axes settings.
  QString key_prefix = "renderModule/OrientationAxes/";
  if (settings->contains(key_prefix + "Visibility"))
    {
    this->setOrientationAxesVisibility(
      settings->value(key_prefix + "Visibility").toBool());
    }
  if (settings->contains(key_prefix + "Interactivity"))
    {
    this->setOrientationAxesInteractivity(
      settings->value(key_prefix + "Interactivity").toBool());
    }
  if (settings->contains(key_prefix + "OutlineColor"))
    {
    this->setOrientationAxesOutlineColor(
      settings->value(key_prefix + "OutlineColor").value<QColor>());
    }
  if (settings->contains(key_prefix + "LabelColor"))
    {
    this->setOrientationAxesLabelColor(
      settings->value(key_prefix + "LabelColor").value<QColor>());
    }
  
  // Center Axes settings.
  key_prefix = "renderModule/CenterAxes/";
  if (settings->contains(key_prefix + "Visibility"))
    {
    this->setCenterAxesVisibility(
      settings->value(key_prefix + "Visibility").toBool());
    }
  if (settings->contains(key_prefix + "ResetCenterWithCamera"))
    {
    this->ResetCenterWithCamera =
      settings->value(key_prefix + "ResetCenterWithCamera").toBool();
    }
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::saveSettings()
{
  vtkSMProxy* proxy = this->getProxy();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const char*** str;
  
  for(str=pqRenderViewModuleSettings; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = QString("renderModule/") + *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop)
        {
        settings->setValue(key, pqSMAdaptor::getElementProperty(prop));
        }
      }
    }
  for(str=pqRenderViewModuleSettingsMulti; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = QString("renderModule/") + *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop)
        {
        settings->setValue(key, pqSMAdaptor::getMultipleElementProperty(prop));
        }
      }
    }

  // Orientation Axes settings.
  QString key_prefix = "renderModule/OrientationAxes/";
  settings->setValue(key_prefix + "Visibility", 
    this->getOrientationAxesVisibility());
  settings->setValue(key_prefix + "Interactivity",
    this->getOrientationAxesInteractivity());
  settings->setValue(key_prefix + "OutlineColor",
    this->getOrientationAxesOutlineColor());
  settings->setValue(key_prefix + "LabelColor",
    this->getOrientationAxesLabelColor());

  // Center Axes settings.
  key_prefix = "renderModule/CenterAxes/";
  settings->setValue(key_prefix + "Visibility",
    this->getCenterAxesVisibility());
  settings->setValue(key_prefix + "ResetCenterWithCamera",
    this->ResetCenterWithCamera);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setOrientationAxesVisibility(bool visible)
{
  this->Internal->OrientationAxesWidget->SetEnabled(visible? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::getOrientationAxesVisibility() const
{
  return this->Internal->OrientationAxesWidget->GetEnabled();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setOrientationAxesInteractivity(bool interactive)
{
  this->Internal->OrientationAxesWidget->SetInteractive(interactive? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::getOrientationAxesInteractivity() const
{
  return this->Internal->OrientationAxesWidget->GetInteractive();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setOrientationAxesOutlineColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetOutlineColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderViewModule::getOrientationAxesOutlineColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetOutlineColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setOrientationAxesLabelColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetAxisLabelColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderViewModule::getOrientationAxesLabelColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetAxisLabelColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}


//-----------------------------------------------------------------------------
void pqRenderViewModule::updateCenterAxes()
{
  if(!this->Internal->InteractorStyleProxy)
    {
    return;
    }

  if (!this->getCenterAxesVisibility())
    {
    return;
    }

  double center[3];
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->Internal->InteractorStyleProxy->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();

  QList<QVariant> positionValues;
  positionValues << center[0] << center[1] << center[2];

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Position"),
    positionValues);

  // Reset size of the axes.
  double bounds[6];
  this->Internal->RenderModuleProxy->ComputeVisiblePropBounds(bounds);

  QList<QVariant> scaleValues;
  scaleValues << (bounds[1]-bounds[0])*0.25
    << (bounds[3]-bounds[2])*0.25 
    << (bounds[5]-bounds[4])*0.25;

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Scale"),
    scaleValues);

  this->Internal->CenterAxesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setCenterOfRotation(double x, double y, double z)
{
  if(!this->Internal->InteractorStyleProxy)
    {
    return;
    }

  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  vtkPVInteractorStyle* style = vtkPVInteractorStyle::SafeDownCast(
    iren->GetInteractorStyle());
  if (!style)
    {
    return;
    }

  QList<QVariant> positionValues;
  positionValues << x << y << z;

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->InteractorStyleProxy->GetProperty("CenterOfRotation"),
    positionValues);

  this->Internal->InteractorStyleProxy->UpdateVTKObjects();

  this->updateCenterAxes();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::getCenterOfRotation(double center[3]) const
{
  if(!this->Internal->InteractorStyleProxy)
    {
    return;
    }

  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->Internal->InteractorStyleProxy->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setCenterAxesVisibility(bool visible)
{
  pqSMAdaptor::setElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility"),
    visible? 1 : 0);
  this->Internal->CenterAxesProxy->UpdateProperty("Visibility");
  this->Internal->RenderModuleProxy->MarkModified(0);
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::getCenterAxesVisibility() const
{
  if (this->Internal->CenterAxesProxy.GetPointer()==0)
    {
    return false;
    }

  return pqSMAdaptor::getElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility")).toBool();
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::eventFilter(QObject* caller, QEvent* e)
{
  // TODO, apparently, this should watch for window position changes, not resizes
  
  if(e->type() == QEvent::MouseButtonPress)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if(me->button() & Qt::RightButton)
      {
      this->Internal->MouseOrigin = me->pos();
      }
    }
  else if(e->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if(me->button() & Qt::RightButton)
      {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if(delta.manhattanLength() < 3)
        {
        QList<QAction*> actions = this->Internal->Viewport->actions();
        if(!actions.isEmpty())
          {
          QMenu* menu = new QMenu(this->Internal->Viewport);
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(this->Internal->Viewport->mapToGlobal(newPos));
          }
        }
      }
    }
  
  return QObject::eventFilter(caller, e);
}

void pqRenderViewModule::addMenuAction(QAction* a)
{
  this->Internal->Viewport->addAction(a);
}

void pqRenderViewModule::removeMenuAction(QAction* a)
{
  this->Internal->Viewport->removeAction(a);
}

void pqRenderViewModule::restoreDefaultLightSettings()
{
  vtkSMProxy* proxy = this->getProxy();
  const char** str;

  for(str=pqRenderViewModuleLightSettings; *str != NULL; str++)
    {
    vtkSMProperty* prop = proxy->GetProperty(*str);
    if(prop)
      {
      prop->ResetToDefault();
      }
    }
  for(str=pqRenderViewModuleLightSettingsMulti; *str != NULL; str++)
    {
    vtkSMProperty* prop = proxy->GetProperty(*str);
    prop->ResetToDefault();
    }
  proxy->UpdateVTKObjects();

}
  
//-----------------------------------------------------------------------------
void pqRenderViewModule::linkToOtherView()
{
  pqLinkViewWidget* linkWidget = new pqLinkViewWidget(this);
  linkWidget->setAttribute(Qt::WA_DeleteOnClose);
  QPoint pos = this->getWidget()->mapToGlobal(QPoint(2,2));
  linkWidget->move(pos);
  linkWidget->show();
}
  
//-----------------------------------------------------------------------------
bool pqRenderViewModule::canDisplaySource(pqPipelineSource* source) const
{
  if(!source ||
     this->getServer()->GetConnectionID() !=
     source->getServer()->GetConnectionID())
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::onUndoStackChanged()
{
  bool can_undo = this->Internal->InteractionUndoStack->CanUndo();
  bool can_redo = this->Internal->InteractionUndoStack->CanRedo();

  emit this->canUndoChanged(can_undo);
  emit this->canRedoChanged(can_redo);
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::canUndo() const
{
  return this->Internal->InteractionUndoStack->CanUndo();
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::canRedo() const
{
  return this->Internal->InteractionUndoStack->CanRedo();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::undo()
{
  this->Internal->InteractionUndoStack->Undo();
  this->Internal->RenderModuleProxy->UpdateVTKObjects();
  this->render();

  this->fakeUndoRedo(false, false);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::redo()
{
  this->Internal->InteractionUndoStack->Redo();
  this->Internal->RenderModuleProxy->UpdateVTKObjects();
  this->render();
  
  this->fakeUndoRedo(true, false);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::linkUndoStack(pqRenderViewModule* other)
{
  if (other == this)
    {
    // Sanity check, nothing to link if both are same.
    return;
    }

  this->Internal->LinkedUndoStacks.push_back(other);

  // Clear all linked stacks until now.
  this->clearUndoStack();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::unlinkUndoStack(pqRenderViewModule* other)
{
  if (!other || other == this)
    {
    return;
    }
  this->Internal->LinkedUndoStacks.removeAll(other);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::clearUndoStack()
{
  if (this->Internal->UpdatingStack)
    {
    return;
    }
  this->Internal->UpdatingStack = true;
  this->Internal->InteractionUndoStack->Clear();
  foreach (pqRenderViewModule* other, this->Internal->LinkedUndoStacks)
    {
    if (other)
      {
      other->clearUndoStack();
      }
    }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::fakeUndoRedo(bool fake_redo, bool self)
{
  if (this->Internal->UpdatingStack)
    {
    return;
    }
  this->Internal->UpdatingStack = true;
  if (self)
    {
    if (fake_redo)
      {
      this->Internal->InteractionUndoStack->PopRedoStack();
      }
    else
      {
      this->Internal->InteractionUndoStack->PopUndoStack();
      }
    }
  foreach (pqRenderViewModule* other, this->Internal->LinkedUndoStacks)
    {
    if (other)
      {
      other->fakeUndoRedo(fake_redo, true);
      }
    }
  this->Internal->UpdatingStack = false;
}

