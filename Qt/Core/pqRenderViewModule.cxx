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
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkTrackballPan.h"

// Qt includes.
#include <QFileInfo>
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QEvent>
#include <QSet>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqRenderViewProxy.h"
#include "pqServer.h"
#include "pqSetName.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "vtkPVAxesWidget.h"

static QSet<pqRenderViewModule*> RenderModules;

/// method to keep server side views in sync with client side views
/// the client has multiple windows, whereas the server has just one window with
/// multiple sub views.  We need to tell the server where to put the views.
static void UpdateServerViews(pqRenderViewModule* renderModule)
{
  // get all the render modules on the server
  pqServer* server = renderModule->getServer();
  QList<pqRenderViewModule*> rms;
  foreach(pqRenderViewModule* rm, RenderModules)
    {
    if(rm->getServer() == server)
      {
      rms.append(rm);
      }
    }

  // find a rectangle that bounds all views
  QRect totalBounds;
  
  foreach(pqRenderViewModule* rm, rms)
    {
    QRect bounds = rm->getWidget()->rect();
    bounds.moveTo(rm->getWidget()->mapToGlobal(QPoint(0,0)));
    totalBounds |= bounds;
    }

  // now loop through all render modules and set the server size window size
  foreach(pqRenderViewModule* rm, rms)
    {
    vtkSMIntVectorProperty* prop = 0;

    // set size containing all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      rm->getRenderModuleProxy()->GetProperty("GUISize"));
    if(prop)
      {
      prop->SetElements2(totalBounds.width(), totalBounds.height());
      }

    // position relative to the bounds of all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      rm->getRenderModuleProxy()->GetProperty("WindowPosition"));
    if(prop)
      {
      QPoint pos = rm->getWidget()->mapToGlobal(QPoint(0,0));
      pos -= totalBounds.topLeft();
      prop->SetElements2(pos.x(), pos.y());
      }
    }

}

class pqRenderViewModuleInternal
{
public:
  QPointer<QVTKWidget> Viewport;
  vtkSmartPointer<pqRenderViewProxy> RenderViewProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMRenderModuleProxy> RenderModuleProxy;
  vtkSmartPointer<vtkPVAxesWidget> OrientationAxesWidget;
  vtkSmartPointer<vtkSMProxy> CenterAxesProxy;
  vtkSmartPointer<vtkSMPropertyLink> ViewTimeLink;
  vtkSmartPointer<vtkSMProxy> InteractorStyleProxy;
  pqUndoStack* UndoStack;
  int DefaultBackground[3];

  pqRenderViewModuleInternal()
    {
    this->Viewport = 0;
    this->RenderViewProxy = vtkSmartPointer<pqRenderViewProxy>::New();
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->OrientationAxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();
    this->InteractorStyleProxy = 0;
    this->UndoStack = new pqUndoStack(true);
    this->DefaultBackground[0] = 84;
    this->DefaultBackground[1] = 89;
    this->DefaultBackground[2] = 109;
    }

  ~pqRenderViewModuleInternal()
    {
    this->RenderViewProxy->setRenderModule(0);
    delete this->UndoStack;
    }
};

//-----------------------------------------------------------------------------
pqRenderViewModule::pqRenderViewModule(const QString& name, 
  vtkSMRenderModuleProxy* renModule, pqServer* server, QObject* _parent/*=null*/)
: pqGenericViewModule("render_modules", name, renModule, server, _parent)
{
  this->Internal = new pqRenderViewModuleInternal();
  this->Internal->RenderViewProxy->setRenderModule(this);
  this->Internal->RenderModuleProxy = renModule;
  this->Internal->UndoStack->setActiveServer(this->getServer());

  this->Internal->Viewport = new QVTKWidget() 
    << pqSetName("Viewport");
  // do image caching for performance
  //this->Internal->Viewport->setAutomaticImageCacheEnabled(true);
  RenderModules.insert(this);

  this->Internal->Viewport->installEventFilter(this);

  this->ResetCenterWithCamera = true;

  this->Internal->CenterAxesProxy.TakeReference(
    vtkSMObject::GetProxyManager()->NewProxy("axes","Axes"));
  this->Internal->CenterAxesProxy->SetConnectionID(
    this->Internal->RenderModuleProxy->GetConnectionID());
  this->Internal->CenterAxesProxy->SetServers(
    vtkProcessModule::CLIENT);
  QList<QVariant> scaleValues;
  scaleValues << .25 << .25 << .25;
  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Scale"),
    scaleValues);
  this->Internal->CenterAxesProxy->UpdateVTKObjects();

  /// When a state is loaded, if this render module is reused, its interactor style will change
  QObject::connect(pqApplicationCore::instance(), SIGNAL(stateLoaded()), this,
    SLOT(updateInteractorStyleFromState()));

}

//-----------------------------------------------------------------------------
pqRenderViewModule::~pqRenderViewModule()
{
  RenderModules.remove(this);

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
pqUndoStack* pqRenderViewModule::getInteractionUndoStack() const
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setWindowParent(QWidget* _parent)
{
  this->Internal->Viewport->setParent(_parent);
  this->Internal->Viewport->update();
}

//-----------------------------------------------------------------------------
QWidget* pqRenderViewModule::getWindowParent() const
{
  return this->Internal->Viewport->parentWidget();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::viewModuleInit()
{
  if (!this->Internal->Viewport || !this->Internal->RenderViewProxy)
    {
    qDebug() << "viewModuleInit() missing information.";
    return;
    }
  this->Internal->Viewport->SetRenderWindow(
    this->Internal->RenderModuleProxy->GetRenderWindow());

  // Enable interaction on this client.
  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  iren->SetPVRenderView(this->Internal->RenderViewProxy);

  // Init axes actor.
  this->Internal->OrientationAxesWidget->SetParentRenderer(
    this->Internal->RenderModuleProxy->GetRenderer());
  this->Internal->OrientationAxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->OrientationAxesWidget->SetInteractor(iren);
  this->Internal->OrientationAxesWidget->SetEnabled(1);
  this->Internal->OrientationAxesWidget->SetInteractive(0);

  this->Internal->RenderModuleProxy->ResetCamera();

  // Set up interactor styles and their manipulators.

  // If viewModuleInit() is called while state is being loaded, 
  // then everything inside this conditional will be taken care of by the state and we don't want to override it.
  if(!pqApplicationCore::instance()->isLoadingState())
    {
    // Create the interactor style proxy:
    this->Internal->InteractorStyleProxy.TakeReference(
      vtkSMObject::GetProxyManager()->NewProxy("interactorstyles","InteractorStyle"));
    this->Internal->InteractorStyleProxy->SetConnectionID(
      this->Internal->RenderModuleProxy->GetConnectionID());
    this->Internal->InteractorStyleProxy->SetServers(
      vtkProcessModule::CLIENT);
    this->addInternalProxy("InteractorStyles",this->Internal->InteractorStyleProxy);
    vtkSMProperty *styleManips = this->Internal->InteractorStyleProxy->GetProperty("CameraManipulators");
  
    // It is possible that the interactor style is setup via python in which case it may be something 
    // other than PVInteractorStyle which may not accept manipulators, hence this conditional
    if(styleManips)
      {
      // Create and register manipulators, then add to interactor style

      vtkSMProxy *manip = vtkSMObject::GetProxyManager()->NewProxy("cameramanipulators","TrackballRotate");
      manip->SetConnectionID(
        this->Internal->RenderModuleProxy->GetConnectionID());
      manip->SetServers(vtkProcessModule::CLIENT);
      this->addInternalProxy("CameraManipulators",manip);
      pqSMAdaptor::addProxyProperty(styleManips, manip);
      manip->Delete();

      manip = vtkSMObject::GetProxyManager()->NewProxy("cameramanipulators","TrackballPan1");
      manip->SetConnectionID(
        this->Internal->RenderModuleProxy->GetConnectionID());
      manip->SetServers(vtkProcessModule::CLIENT);
      vtkSMIntVectorProperty *button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Button"));
      button->SetElement(0,2);
      this->addInternalProxy("CameraManipulators",manip);
      pqSMAdaptor::addProxyProperty(styleManips, manip);
      manip->UpdateVTKObjects();
      manip->Delete();

      manip = vtkSMObject::GetProxyManager()->NewProxy("cameramanipulators","TrackballPan1");
      manip->SetConnectionID(
        this->Internal->RenderModuleProxy->GetConnectionID());
      manip->SetServers(vtkProcessModule::CLIENT);
      button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Button"));
      button->SetElement(0,1);
      button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Control"));
      button->SetElement(0,1);
      this->addInternalProxy("CameraManipulators",manip);
      pqSMAdaptor::addProxyProperty(styleManips, manip);
      manip->UpdateVTKObjects();
      manip->Delete();

      manip = vtkSMObject::GetProxyManager()->NewProxy("cameramanipulators","TrackballZoom");
      manip->SetConnectionID(
        this->Internal->RenderModuleProxy->GetConnectionID());
      manip->SetServers(vtkProcessModule::CLIENT);
      button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Button"));
      button->SetElement(0,3);
      this->addInternalProxy("CameraManipulators",manip);
      pqSMAdaptor::addProxyProperty(styleManips, manip);
      manip->UpdateVTKObjects();
      manip->Delete();

      manip = vtkSMObject::GetProxyManager()->NewProxy("cameramanipulators","TrackballZoom");
      manip->SetConnectionID(
        this->Internal->RenderModuleProxy->GetConnectionID());
      manip->SetServers(vtkProcessModule::CLIENT);
      button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Button"));
      button->SetElement(0,1);
      button = vtkSMIntVectorProperty::SafeDownCast(manip->GetProperty("Shift"));
      button->SetElement(0,1);
      this->addInternalProxy("CameraManipulators",manip);
      pqSMAdaptor::addProxyProperty(styleManips, manip);
      manip->UpdateVTKObjects();
      manip->Delete();
      }

    // Is this needed?
    this->Internal->InteractorStyleProxy->UpdateVTKObjects();

    // Set interactor style as a proxy property of the render module so it will get saved in the state
    vtkSMProxyProperty *iStyle = vtkSMProxyProperty::SafeDownCast(this->Internal->RenderModuleProxy->GetProperty("InteractorStyle"));
    iStyle->RemoveAllProxies();
    iStyle->AddProxy(this->Internal->InteractorStyleProxy);

    }
  else
    {
    // If we got here, this render module was created and initialized from state

    vtkSMProxyProperty *iStyle = vtkSMProxyProperty::SafeDownCast(this->Internal->RenderModuleProxy->GetProperty("InteractorStyle"));

    // if the state is an older version, then it may not support interactor style proxies, thus the following conditional 
    //     (and all subsequent conditionals in this class that make sure the interactor style proxy objct is valid)
    if(iStyle->GetNumberOfProxies() > 0)
      {
      // Just grab its interactor style and use it as our own
      this->Internal->InteractorStyleProxy = iStyle->GetProxy(0);
      }
    }

  if(this->Internal->InteractorStyleProxy)
    {
    vtkProcessModule* pvm = vtkProcessModule::GetProcessModule();
    vtkInteractorStyle *style = vtkInteractorStyle::SafeDownCast(pvm->GetObjectFromID(this->Internal->InteractorStyleProxy->GetID(0)));
    this->Internal->VTKConnect->Connect(style,
      vtkCommand::StartInteractionEvent, 
      this, SLOT(startInteraction()));
    this->Internal->VTKConnect->Connect(style,
      vtkCommand::EndInteractionEvent, 
      this, SLOT(endInteraction()));
    }

  this->Internal->VTKConnect->Connect(
    this->Internal->RenderModuleProxy,
    vtkCommand::StartEvent, this, SLOT(onStartEvent()));
  this->Internal->VTKConnect->Connect(
    this->Internal->RenderModuleProxy,
    vtkCommand::EndEvent,this, SLOT(onEndEvent()));  
  
  // help the QVTKWidget know when to clear the cache
  this->Internal->VTKConnect->Connect(
    this->Internal->RenderModuleProxy, vtkCommand::ModifiedEvent,
    this->Internal->Viewport, SLOT(markCachedImageAsDirty()));  

  this->Internal->VTKConnect->Connect(
    this->Internal->RenderModuleProxy, vtkCommand::ResetCameraEvent,
    this, SLOT(onResetCameraEvent()));

  iren->Enable();

  this->Superclass::viewModuleInit();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::updateInteractorStyleFromState()
{
  if(!this->Internal->InteractorStyleProxy)
    {
    return;
    }

  vtkProcessModule* pvm = vtkProcessModule::GetProcessModule();
  vtkInteractorStyle *currentStyle = vtkInteractorStyle::SafeDownCast(pvm->GetObjectFromID(this->Internal->InteractorStyleProxy->GetID(0)));
  vtkInteractorStyle *newStyle = vtkInteractorStyle::SafeDownCast(this->Internal->RenderModuleProxy->GetInteractor()->GetInteractorStyle());

  if(currentStyle != newStyle)
    {
    this->setInteractorStyle(newStyle);
    }

  // This is a temporary fix to make sure the center axes gets displayed after the state is loaded
  pqSMAdaptor::removeProxyProperty(this->Internal->RenderModuleProxy->GetProperty("Displays"),
    this->Internal->CenterAxesProxy);
  pqSMAdaptor::addProxyProperty(this->Internal->RenderModuleProxy->GetProperty("Displays"),
    this->Internal->CenterAxesProxy);
  this->Internal->RenderModuleProxy->UpdateVTKObjects();

  this->updateCenterAxes();
}


//-----------------------------------------------------------------------------
void pqRenderViewModule::setInteractorStyle(vtkInteractorStyle* style)
{
  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  vtkInteractorObserver* old_style = iren->GetInteractorStyle();
  if (old_style)
    {
    this->Internal->VTKConnect->Disconnect(old_style, 0, this, 0);
    }

  iren->SetInteractorStyle(style);

  // Grab the new interactor style's proxy and  register it:
  this->removeInternalProxy("InteractorStyles",this->Internal->InteractorStyleProxy);
  vtkSMProxyProperty *iStyle = vtkSMProxyProperty::SafeDownCast(this->Internal->RenderModuleProxy->GetProperty("InteractorStyle"));
  this->Internal->InteractorStyleProxy = iStyle->GetProxy(0);
  this->addInternalProxy("InteractorStyles",this->Internal->InteractorStyleProxy);

  this->Internal->VTKConnect->Connect(style,
    vtkCommand::StartInteractionEvent, 
    this, SLOT(startInteraction()));
  this->Internal->VTKConnect->Connect(style,
    vtkCommand::EndInteractionEvent, 
    this, SLOT(endInteraction()));
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::startInteraction()
{
  // It is essential to synchronize camera properties prior to starting the 
  // interaction since the current state of the camera might be different from 
  // that reflected by the properties.
  this->Internal->RenderModuleProxy->SynchronizeCameraProperties();
  
  // NOTE: bewary of the server used while calling
  // BeginOrContinueUndoSet on vtkSMUndoStack.
  this->Internal->UndoStack->BeginUndoSet("Interaction");

  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  if (!iren)
    {
    return;
    }
  iren->SetInteractiveRenderEnabled(true);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::endInteraction()
{
  this->Internal->RenderModuleProxy->SynchronizeCameraProperties();
  this->Internal->UndoStack->EndUndoSet();

  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  if (!iren)
    {
    return;
    }
  iren->SetInteractiveRenderEnabled(false);
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::onStartEvent()
{
  emit this->beginRender();
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::onEndEvent()
{
  emit this->endRender();
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

int* pqRenderViewModule::defaultBackgroundColor()
{
  return this->Internal->DefaultBackground;
}

//-----------------------------------------------------------------------------
void pqRenderViewModule::setDefaults()
{
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
 
  pqSMAdaptor::addProxyProperty(proxy->GetProperty("Displays"),
    this->Internal->CenterAxesProxy);

  proxy->UpdateVTKObjects();

  // Link ViewTime with global time.
  vtkSMProxy* timekeeper = this->getServer()->getTimeKeeper()->getProxy();

  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  link->AddLinkedProperty(timekeeper->GetProperty("Time"), vtkSMLink::INPUT);
  link->AddLinkedProperty(proxy->GetProperty("ViewTime"), vtkSMLink::OUTPUT);
  this->Internal->ViewTimeLink = link;
  link->Delete();
  timekeeper->GetProperty("Time")->Modified();

  this->restoreSettings();
}

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
    qDebug() 
      << "Cannot set center of rotation since interaction style has changed.";
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
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::getCenterAxesVisibility() const
{
  return pqSMAdaptor::getElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility")).toBool();
}

//-----------------------------------------------------------------------------
bool pqRenderViewModule::eventFilter(QObject* caller, QEvent* e)
{
  // TODO, apparently, this should watch for window position changes, not resizes
  
  if(e->type() == QEvent::Resize)
    {
    UpdateServerViews(this);
    }
  
  return QObject::eventFilter(caller, e);
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

