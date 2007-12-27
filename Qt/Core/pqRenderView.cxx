/*=========================================================================

   Program: ParaView
   Module:    pqRenderView.cxx

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
#include "pqRenderView.h"

// ParaView Server Manager includes.
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVTrackballRoll.h"
#include "vtkPVTrackballRotate.h"
#include "vtkPVTrackballZoom.h"
#include "vtkSmartPointer.h"
#include "vtkSMInteractionUndoStackBuilder.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUndoStack.h"
#include "vtkTrackballPan.h"

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
#include <QGridLayout>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqLinkViewWidget.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "vtkPVAxesWidget.h"

class pqRenderView::pqInternal
{
public:
  QPointer<QWidget> Viewport;
  QPoint MouseOrigin;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkPVAxesWidget> OrientationAxesWidget;
  vtkSmartPointer<vtkSMProxy> CenterAxesProxy;

  vtkSmartPointer<vtkSMUndoStack> InteractionUndoStack;
  vtkSmartPointer<vtkSMInteractionUndoStackBuilder> UndoStackBuilder;

  QList<pqRenderView* > LinkedUndoStacks;
  bool UpdatingStack;

  int DefaultBackground[3];
  bool InitializedWidgets;
  QList<vtkSMProxy* > DefaultCameraManipulators;
  pqInternal()
    {
    this->UpdatingStack = false;
    this->InitializedWidgets = false;
    this->Viewport = 0;
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->OrientationAxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();
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

  ~pqInternal()
    {
    if(this->DefaultCameraManipulators.size()>0)
      {
      foreach(vtkSMProxy* manip, this->DefaultCameraManipulators)
        {
        if(manip)
          {
          manip->Delete();
          }
        }
      this->DefaultCameraManipulators.clear();
      }
    }
};

//-----------------------------------------------------------------------------
pqRenderView::pqRenderView( const QString& group,
                            const QString& name, 
                            vtkSMViewProxy* renModule, 
                            pqServer* server, 
                            QObject* _parent/*=null*/) : 
  pqView(renderViewType(), group, name, renModule, server, _parent)
{
  this->Internal = new pqRenderView::pqInternal();

  // we need to fire signals when undo stack changes.
  this->Internal->VTKConnect->Connect(this->Internal->InteractionUndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onUndoStackChanged()),
    0, 0, Qt::QueuedConnection);

  this->ResetCenterWithCamera = true;
  this->Internal->VTKConnect->Connect(
    renModule, vtkCommand::ResetCameraEvent,
    this, SLOT(onResetCameraEvent()));

}

//-----------------------------------------------------------------------------
pqRenderView::~pqRenderView()
{
  delete this->Internal->Viewport;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(this->getViewProxy());
}

//-----------------------------------------------------------------------------
QWidget* pqRenderView::getWidget()
{
  if (!this->Internal->Viewport)
    {
    this->Internal->Viewport = this->createWidget();
    // we manage the context menu ourself, so it doesn't interfere with
    // render window interactions
    this->Internal->Viewport->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->Viewport->installEventFilter(this);
    this->Internal->Viewport->setObjectName("Viewport");

    // add a link view menu
    QAction* act = new QAction("Link Camera...", this);
    this->addMenuAction(act);
    QObject::connect(act, SIGNAL(triggered(bool)),
      this, SLOT(linkToOtherView()));
    }

  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderView::createWidget() 
{
  QVTKWidget* vtkwidget = new QVTKWidget();

  // do image caching for performance
  // For now, we are doing this only on Apple because it can render
  // and capture a frame buffer even when it is obstructred by a
  // window. This does not work as well on other platforms.
#if defined(__APPLE__)
  vtkwidget->setAutomaticImageCacheEnabled(true);
#endif

  // help the QVTKWidget know when to clear the cache
  this->Internal->VTKConnect->Connect(
    this->getProxy(), vtkCommand::ModifiedEvent,
    vtkwidget, SLOT(markCachedImageAsDirty()));
  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqRenderView::initialize()
{
  this->Superclass::initialize();

  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the QVTKWidget
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we 
  // do the following.
  vtkSMProxy* renModule = this->getProxy();
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
// This method is called for all pqRenderView objects irrespective
// of whether it is created from state/undo-redo/python or by the GUI. Hence
// don't change any render module properties here.
void pqRenderView::initializeWidgets()
{
  if (this->Internal->InitializedWidgets)
    {
    return;
    }

  this->Internal->InitializedWidgets = true;
  // Disconnect old slots.
  // this->Internal->VTKConnect->Disconnect(
  //   this->Internal->RenderModuleProxy, vtkCommand::UpdateEvent);

  vtkSMRenderViewProxy* renModule = this->getRenderViewProxy();

  QVTKWidget* vtkwidget = qobject_cast<QVTKWidget*>(this->getWidget());
  if (vtkwidget)
    {
    vtkwidget->SetRenderWindow(renModule->GetRenderWindow());
    }

  vtkPVGenericRenderWindowInteractor* iren = renModule->GetInteractor();

  // Init axes actor.
  // FIXME: Convert OrientationAxesWidget to a first class representation.
  this->Internal->OrientationAxesWidget->SetParentRenderer(
    renModule->GetRenderer());
  this->Internal->OrientationAxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->OrientationAxesWidget->SetInteractor(iren);
  this->Internal->OrientationAxesWidget->SetEnabled(1);
  this->Internal->OrientationAxesWidget->SetInteractive(0);

  // setup the center axes.
  this->initializeCenterAxes();

  this->Internal->UndoStackBuilder->SetRenderView(renModule);
}

//-----------------------------------------------------------------------------
// Sets default values for the underlying proxy.  This is during the 
// initialization stage of the pqProxy for proxies created by the GUI itself 
// i.e. for proxies loaded through state or created by python client or 
// undo/redo, this method won't be called. 
void pqRenderView::setDefaultPropertyValues()
{
  this->createDefaultInteractors();
  this->updateDefaultInteractors(
    this->Internal->DefaultCameraManipulators);

  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODResolution"), 50);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODThreshold"), 5);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("RemoteRenderThreshold"), 3);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("SquirtLevel"), 3);

  vtkSMProperty* backgroundProperty;
  int* bg = this->defaultBackgroundColor();
  backgroundProperty = proxy->GetProperty("Background");
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 0, bg[0]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 1, bg[1]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 2, bg[2]/255.0);

  proxy->UpdateVTKObjects();

  this->restoreSettings(false);
  this->getRenderViewProxy()->ResetCamera();
  this->clearUndoStack();
}

//-----------------------------------------------------------------------------
// This method gets called only with the object is directly created by the GUI
// i.e. it wont get called when the proxy is loaded from state/undo/redo or 
// python.
// TODO: Python paraview modules createView() equivalent should make sure
// that it sets up some default interactor.
void pqRenderView::createDefaultInteractors()
{
  if(this->Internal->DefaultCameraManipulators.size()>0)
    {
    foreach(vtkSMProxy* manip, this->Internal->DefaultCameraManipulators)
      {
      if(manip)
        {
        manip->Delete();
        }
      }
    this->Internal->DefaultCameraManipulators.clear();
    }

  // LeftButton -- Rotate
  vtkSMProxy *manip = this->createCameraManipulator(1, 0, 0, "Rotate");
  this->Internal->DefaultCameraManipulators.push_back(manip);

  // Shift + LeftButton  -- Roll.
  manip = this->createCameraManipulator(1, 1, 0, "Roll");
  this->Internal->DefaultCameraManipulators.push_back(manip);

/*
  // Control + LeftButton -- Move
  manip = this->createCameraManipulator(1, 0, 1, "Move");
  this->Internal->DefaultCameraManipulators.push_back(manip);
*/

  // Control + LeftButton -- FlyIn
  // manip = pxm->NewProxy("cameramanipulators", "JoystickFly2");
  manip = this->createCameraManipulator(1, 0, 1, "Zoom");
  // pqSMAdaptor::setElementProperty(manip->GetProperty("In"), 1);
  this->Internal->DefaultCameraManipulators.push_back(manip);

  // MiddleButton -- Pan
  manip = this->createCameraManipulator(2, 0, 0, "Pan");
  this->Internal->DefaultCameraManipulators.push_back(manip);

  // Shift + MiddleButton -- Rotate
  manip = this->createCameraManipulator(2, 1, 0, "Rotate");
  this->Internal->DefaultCameraManipulators.push_back(manip);

  // Control + MiddleButton -- Rotate
  manip = this->createCameraManipulator(2, 0, 1, "Rotate");
  this->Internal->DefaultCameraManipulators.push_back(manip);
  
  // RightButton -- Zoom
  manip = this->createCameraManipulator(3, 0, 0, "Zoom");
  this->Internal->DefaultCameraManipulators.push_back(manip);

  // Shift + RightButton -- Pan
  manip = this->createCameraManipulator(3, 1, 0, "Pan");
  this->Internal->DefaultCameraManipulators.push_back(manip);
  
  // Control + RightButton -- FlyOut
  // manip = pxm->NewProxy("cameramanipulators", "JoystickFly2");
  manip = this->createCameraManipulator(3, 0, 1, "Zoom");
  // pqSMAdaptor::setElementProperty(manip->GetProperty("In"), 0);
  this->Internal->DefaultCameraManipulators.push_back(manip);

}

//-----------------------------------------------------------------------------
bool pqRenderView::updateDefaultInteractors(
  QList<vtkSMProxy*> manipulators)
{
  if(manipulators.size()<=0)
    {
    return false;
    }

  vtkSMProxy* viewproxy = this->getProxy();
  
  this->clearHelperProxies();
  vtkSMProxyProperty *styleManips = 
    vtkSMProxyProperty::SafeDownCast(viewproxy->GetProperty("CameraManipulators"));
  styleManips->RemoveAllProxies();

  // Register manipulators, then add to interactor style
 
  foreach(vtkSMProxy *manip, manipulators)
    {
    this->addHelperProxy("Manipulators",manip);
    pqSMAdaptor::addProxyProperty(styleManips, manip);
    manip->UpdateVTKObjects();
    }

  viewproxy->UpdateVTKObjects();
  return true;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqRenderView::createCameraManipulator(
  int mouse, int shift, int control, QString name)
{
  QString strManipName;
  if(name.compare("Rotate")==0)
    {
    strManipName = "TrackballRotate";
    }
  else if(name.compare("Roll")==0)
    {
    strManipName = "TrackballRoll";
    }
  else if(name.compare("Move")==0)
    {
    strManipName = "TrackballMoveActor";
    }
  else if(name.compare("Zoom")==0)
    {
    strManipName = "TrackballZoom";
    }
  else if(name.compare("Pan")==0)
    {
    strManipName = "TrackballPan1";
    }
  else
    {
    strManipName = "None";
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkIdType cid = this->getServer()->GetConnectionID();
  vtkSMProxy *manip = pxm->NewProxy("cameramanipulators", 
    strManipName.toAscii().data());
  if(!manip)
    {
    return NULL;
    }
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), mouse);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Shift"), shift);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Control"), control);
  pqSMAdaptor::setElementProperty(manip->GetProperty("ManipulatorName"), name);
  //manip->UpdateVTKObjects();
  return manip;
}

//-----------------------------------------------------------------------------
// Create a center axes if one doesn't already exist.
void pqRenderView::initializeCenterAxes()
{
  if (this->Internal->CenterAxesProxy.GetPointer())
    {
    // sanity check.
    return;
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* centerAxes = pxm->NewProxy("representations", "AxesRepresentation");
  centerAxes->SetConnectionID(this->getServer()->GetConnectionID());
  QList<QVariant> scaleValues;
  scaleValues << .25 << .25 << .25;
  pqSMAdaptor::setMultipleElementProperty(
    centerAxes->GetProperty("Scale"), scaleValues);
  pqSMAdaptor::setElementProperty(centerAxes->GetProperty("Pickable"), 0);
  centerAxes->UpdateVTKObjects();
  this->Internal->CenterAxesProxy = centerAxes;

  vtkSMViewProxy* renView = this->getViewProxy();

  // Update the center axes position whenever the center of rotation changes.
  this->Internal->VTKConnect->Connect(
    renView->GetProperty("CenterOfRotation"), 
    vtkCommand::ModifiedEvent, this, SLOT(updateCenterAxes()));
  
  // Add to render module without using properties. That way it does not
  // get saved in state.
  renView->AddRepresentation(
    vtkSMRepresentationProxy::SafeDownCast(centerAxes));
  centerAxes->Delete();

  this->updateCenterAxes();
}

//-----------------------------------------------------------------------------
void pqRenderView::render()
{
  /* Best to leave to superclass so that this works seamlessly with comparative
   * viz views. 
  if (this->Internal->RenderModuleProxy && this->Internal->Viewport)
    {
    this->Internal->Viewport->update();
    }
    */
 
  this->Superclass::render();
}

//-----------------------------------------------------------------------------
void pqRenderView::onResetCameraEvent()
{
  if (this->ResetCenterWithCamera)
    {
    this->resetCenterOfRotation();
    }

  // Ensures that the scale factor is correctly set for the center axes
  // on reset camera.
  this->updateCenterAxes();
}

//-----------------------------------------------------------------------------
void pqRenderView::resetCamera()
{
  this->fakeInteraction(true);
  this->getRenderViewProxy()->ResetCamera();
  this->fakeInteraction(false);
}

//-----------------------------------------------------------------------------
void pqRenderView::resetCenterOfRotation()
{
  // Update center of rotation.
  vtkSMProxy* viewproxy = this->getProxy();
  viewproxy->UpdatePropertyInformation();
  QList<QVariant> values = 
    pqSMAdaptor::getMultipleElementProperty(
      viewproxy->GetProperty("CameraFocalPointInfo"));
  this->setCenterOfRotation(
    values[0].toDouble(), values[1].toDouble(), values[2].toDouble());
}

//-----------------------------------------------------------------------------
vtkImageData* pqRenderView::captureImage(int magnification)
{
  if (this->getWidget()->isVisible())
    {
    return this->getRenderViewProxy()->CaptureWindow(magnification);
    }

  // Don't return any image when the view is not visible.
  return NULL;
}

//-----------------------------------------------------------------------------
bool pqRenderView::saveImage(int width, int height, const QString& filename)
{
  QSize cursize = this->Internal->Viewport->size();
  QSize fullsize = QSize(width, height);
  QSize newsize = cursize;
  int magnification = 1;
  if (width>0 && height>0)
    {
    magnification = pqView::computeMagnification(fullsize, newsize);
    this->Internal->Viewport->resize(newsize);
    }
  this->render();

  const QFileInfo file(filename);
  int error_code = vtkErrorCode::UnknownError;
  if(file.completeSuffix() == "pdf")
    {
    // FIXME: Does not take user-specified image size into consideration.
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
    error_code = vtkErrorCode::NoError;
    }
  else
    {
    error_code = this->getRenderViewProxy()->WriteImage(
      filename.toAscii().data(), magnification);
    }

  switch (error_code)
    {
  case vtkErrorCode::UnrecognizedFileTypeError:
    qCritical() << "Failed to determine file type for file:" 
      << filename.toAscii().data();
    break;

  case vtkErrorCode::NoError:
    // success.
    break;

  default:
    qCritical() << "Failed to save image.";
    }

  if (width>0 && height>0)
    {
    this->Internal->Viewport->resize(newsize);
    this->Internal->Viewport->resize(cursize);
    this->render();
    }
  return (error_code == vtkErrorCode::NoError);
}

//-----------------------------------------------------------------------------
int* pqRenderView::defaultBackgroundColor()
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

static const char* pqGlobalRenderViewModuleMiscSettings [] = {
  "LODThreshold",
  "LODResolution",
  "UseImmediateMode",
  "UseTriangleStrips",
  "RenderInterruptsEnabled",
  "RemoteRenderThreshold",
  "ImageReductionFactor",
  "SquirtLevel",
  "OrderedCompositing",
  "StillRenderImageReductionFactor",
  "CollectGeometryThreshold",
  "DepthPeeling",
  "UseOffscreenRenderingForScreenshots",
  NULL
  };

static const char* pqRenderViewModuleMiscSettings [] = {
  "CacheLimit",
  "CameraParallelProjection",
  NULL
  };


static const char** pqRenderViewModuleSettings[] = {
  pqRenderViewModuleLightSettings,
  pqRenderViewModuleMiscSettings,
  NULL
  };

static const char** pqGlobalRenderViewModuleSettings[] = {
  pqGlobalRenderViewModuleMiscSettings,
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
void pqRenderView::restoreSettings(bool only_global)
{
  vtkSMProxy* proxy = this->getProxy();

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();

  const char*** str;

  if(!only_global)
    {
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
    }
  
  for(str=pqGlobalRenderViewModuleSettings; *str != NULL; str++)
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
  proxy->UpdateVTKObjects();
    
  if(!only_global)
    {
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

  // Active Camera Manipulators
  QString key_prefix = "renderModule/InteractorStyle/";
  if (settings->contains(key_prefix + "CameraManipulators"))
    {
    QStringList qStrManipList = settings->value(
      key_prefix + "CameraManipulators").toStringList();
    int index, mouse, shift, control;
    QString name;
    char tmpName[20];
    QList<vtkSMProxy*> smManipList;
    foreach(QString strManip, qStrManipList)
      {
      sscanf(strManip.toAscii().data(), "Manipulator%dMouse%dShift%dControl%dName%s",
        &index, &mouse, &shift, &control, tmpName);
      name = tmpName;
      vtkSMProxy* localManip = this->createCameraManipulator(
        mouse, shift, control, name);
      if(!localManip)
        {
        continue;
        }
      smManipList.push_back(localManip);
      }
    if(smManipList.size()>0)
      {
      this->updateDefaultInteractors(smManipList);
      foreach(vtkSMProxy* localManip, smManipList)
        {
        localManip->Delete();
        }
      smManipList.clear();
      }
    }
}

//-----------------------------------------------------------------------------
void pqRenderView::saveSettings()
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

  // Active Camera Manipulators
 
  int cc=1;
  int mouse, shift, control;
  QStringList qStrManipList;
  QString strManip, name;
  foreach(vtkSMProxy* pProxy, this->getCameraManipulators())
    {
    mouse = pqSMAdaptor::getElementProperty(pProxy->GetProperty("Button")).toInt();
    shift = pqSMAdaptor::getElementProperty(pProxy->GetProperty("Shift")).toInt();
    control = pqSMAdaptor::getElementProperty(pProxy->GetProperty("Control")).toInt();
    name = pqSMAdaptor::getElementProperty(pProxy->GetProperty("ManipulatorName")).toString();
    strManip = QString("Manipulator%1Mouse%2Shift%3Control%4Name%5").arg(cc++).
      arg(mouse).arg(shift).arg(control).arg(name);
    qStrManipList.append(strManip);   
    }
  
  key_prefix = "renderModule/InteractorStyle/";
  settings->setValue(key_prefix + "CameraManipulators", qStrManipList);
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesVisibility(bool visible)
{
  this->Internal->OrientationAxesWidget->SetEnabled(visible? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesVisibility() const
{
  return this->Internal->OrientationAxesWidget->GetEnabled();
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesInteractivity(bool interactive)
{
  this->Internal->OrientationAxesWidget->SetInteractive(interactive? 1: 0);
}

//-----------------------------------------------------------------------------
bool pqRenderView::getOrientationAxesInteractivity() const
{
  return this->Internal->OrientationAxesWidget->GetInteractive();
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesOutlineColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetOutlineColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesOutlineColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetOutlineColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}

//-----------------------------------------------------------------------------
void pqRenderView::setOrientationAxesLabelColor(const QColor& color)
{
  this->Internal->OrientationAxesWidget->SetAxisLabelColor(
    color.redF(), color.greenF(), color.blueF());
}

//-----------------------------------------------------------------------------
QColor pqRenderView::getOrientationAxesLabelColor() const
{
  QColor color;
  double* dcolor = this->Internal->OrientationAxesWidget->GetAxisLabelColor();
  color.setRgbF(dcolor[0], dcolor[1], dcolor[2]);
  return color;
}


//-----------------------------------------------------------------------------
void pqRenderView::updateCenterAxes()
{
  if (!this->getCenterAxesVisibility())
    {
    return;
    }

  double center[3];
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("CenterOfRotation"));
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
  this->getRenderViewProxy()->ComputeVisiblePropBounds(bounds);
  double widths[3];
  widths[0] = (bounds[1]-bounds[0]);
  widths[1] = (bounds[3]-bounds[2]);
  widths[2] = (bounds[5]-bounds[4]);
  // lets make some thickness in all directions
  double diameterOverTen = qMax(widths[0], qMax(widths[1], widths[2])) / 10.0;
  widths[0] = widths[0] < diameterOverTen ? diameterOverTen : widths[0];
  widths[1] = widths[1] < diameterOverTen ? diameterOverTen : widths[1];
  widths[2] = widths[2] < diameterOverTen ? diameterOverTen : widths[2];

  QList<QVariant> scaleValues;
  scaleValues << (widths[0])*0.25 << (widths[1])*0.25 << (widths[2])*0.25;

  pqSMAdaptor::setMultipleElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Scale"),
    scaleValues);

  this->Internal->CenterAxesProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterOfRotation(double x, double y, double z)
{
  QList<QVariant> positionValues;
  positionValues << x << y << z;

  // this modifies the CenterOfRotation property resulting to a call
  // to updateCenterAxes().
  vtkSMProxy* viewproxy = this->getProxy();
  pqSMAdaptor::setMultipleElementProperty(
    viewproxy->GetProperty("CenterOfRotation"),
    positionValues);
  viewproxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqRenderView::getCenterOfRotation(double center[3]) const
{
  QList<QVariant> val =
    pqSMAdaptor::getMultipleElementProperty(
      this->getProxy()->GetProperty("CenterOfRotation"));
  center[0] = val[0].toDouble();
  center[1] = val[1].toDouble();
  center[2] = val[2].toDouble();
}

//-----------------------------------------------------------------------------
void pqRenderView::setCenterAxesVisibility(bool visible)
{
  pqSMAdaptor::setElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility"),
    visible? 1 : 0);
  this->Internal->CenterAxesProxy->UpdateVTKObjects();
  this->getProxy()->MarkModified(0);
  if (visible)
    {
    // since updateCenterAxes does not do anything unless the axes is visible,
    // we update the center when the axes becomes visible so that we are assured
    // that the correct center of rotation is used.
    this->updateCenterAxes();
    }
}

//-----------------------------------------------------------------------------
bool pqRenderView::getCenterAxesVisibility() const
{
  if (this->Internal->CenterAxesProxy.GetPointer()==0)
    {
    return false;
    }

  return pqSMAdaptor::getElementProperty(
    this->Internal->CenterAxesProxy->GetProperty("Visibility")).toBool();
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqRenderView::getCameraManipulators() const
{
  return this->getHelperProxies("Manipulators");
}

//-----------------------------------------------------------------------------
 QList<vtkSMProxy*> pqRenderView::getDefaultCameraManipulators() const
{
  return this->Internal->DefaultCameraManipulators;
}

//-----------------------------------------------------------------------------
bool pqRenderView::eventFilter(QObject* caller, QEvent* e)
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
  else if(e->type() == QEvent::MouseMove &&
          !this->Internal->MouseOrigin.isNull())
    {
    QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
    QPoint delta = newPos - this->Internal->MouseOrigin;
    if(delta.manhattanLength() < 3)
      {
      this->Internal->MouseOrigin = QPoint();
      }
    }
  else if(e->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if(me->button() & Qt::RightButton && !this->Internal->MouseOrigin.isNull())
      {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if(delta.manhattanLength() < 3 && qobject_cast<QWidget*>(caller))
        {
        QList<QAction*> actions = this->Internal->Viewport->actions();
        if(!actions.isEmpty())
          {
          QMenu* menu = new QMenu(this->Internal->Viewport);
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(qobject_cast<QWidget*>(caller)->mapToGlobal(newPos));
          }
        }
      this->Internal->MouseOrigin = QPoint();
      }
    }
  
  return QObject::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqRenderView::addMenuAction(QAction* a)
{
  this->Internal->Viewport->addAction(a);
}

//-----------------------------------------------------------------------------
void pqRenderView::removeMenuAction(QAction* a)
{
  this->Internal->Viewport->removeAction(a);
}

//-----------------------------------------------------------------------------
void pqRenderView::restoreDefaultLightSettings()
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
void pqRenderView::linkToOtherView()
{
  pqLinkViewWidget* linkWidget = new pqLinkViewWidget(this);
  linkWidget->setAttribute(Qt::WA_DeleteOnClose);
  QPoint pos = this->getWidget()->mapToGlobal(QPoint(2,2));
  linkWidget->move(pos);
  linkWidget->show();
}
  
//-----------------------------------------------------------------------------
bool pqRenderView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource():0;
  if(!source ||
     this->getServer()->GetConnectionID() !=
     source->getServer()->GetConnectionID())
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void pqRenderView::onUndoStackChanged()
{
  bool can_undo = this->Internal->InteractionUndoStack->CanUndo();
  bool can_redo = this->Internal->InteractionUndoStack->CanRedo();

  emit this->canUndoChanged(can_undo);
  emit this->canRedoChanged(can_redo);
}

//-----------------------------------------------------------------------------
bool pqRenderView::canUndo() const
{
  return this->Internal->InteractionUndoStack->CanUndo();
}

//-----------------------------------------------------------------------------
bool pqRenderView::canRedo() const
{
  return this->Internal->InteractionUndoStack->CanRedo();
}

//-----------------------------------------------------------------------------
void pqRenderView::undo()
{
  this->Internal->InteractionUndoStack->Undo();
  this->getProxy()->UpdateVTKObjects();
  this->render();

  this->fakeUndoRedo(false, false);
}

//-----------------------------------------------------------------------------
void pqRenderView::redo()
{
  this->Internal->InteractionUndoStack->Redo();
  this->getProxy()->UpdateVTKObjects();
  this->render();
  
  this->fakeUndoRedo(true, false);
}

//-----------------------------------------------------------------------------
void pqRenderView::linkUndoStack(pqRenderView* other)
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
void pqRenderView::unlinkUndoStack(pqRenderView* other)
{
  if (!other || other == this)
    {
    return;
    }
  this->Internal->LinkedUndoStacks.removeAll(other);
}

//-----------------------------------------------------------------------------
void pqRenderView::clearUndoStack()
{
  if (this->Internal->UpdatingStack)
    {
    return;
    }
  this->Internal->UpdatingStack = true;
  this->Internal->InteractionUndoStack->Clear();
  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
    {
    if (other)
      {
      other->clearUndoStack();
      }
    }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::fakeUndoRedo(bool fake_redo, bool self)
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
  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
    {
    if (other)
      {
      other->fakeUndoRedo(fake_redo, true);
      }
    }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::fakeInteraction(bool start)
{
  if (this->Internal->UpdatingStack)
    {
    return;
    }

  this->Internal->UpdatingStack = true;

  if (start)
    {
    this->Internal->UndoStackBuilder->StartInteraction();
    }
  else
    {
    this->Internal->UndoStackBuilder->EndInteraction();
    }

  foreach (pqRenderView* other, this->Internal->LinkedUndoStacks)
    {
    other->fakeInteraction(start);
    }
  this->Internal->UpdatingStack = false;
}

//-----------------------------------------------------------------------------
void pqRenderView::resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z)
{
  vtkSMProxy* proxy = this->getProxy();

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 0, 0);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 1, 0);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraPosition"), 2, 0);

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 0, look_x);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 1, look_y);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraFocalPoint"), 2, look_z);

  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 0, up_x);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 1, up_y);
  pqSMAdaptor::setMultipleElementProperty(
    proxy->GetProperty("CameraViewUp"), 2, up_z);
  proxy->UpdateVTKObjects();

  this->resetCamera();
  this->render();
}

//-----------------------------------------------------------------------------
void pqRenderView::selectOnSurface(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> surfaceSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectOnSurface(rect[0], rect[1], rect[2], rect[3], 
      selectedRepresentations, selectionSources, surfaceSelections, false))
    {
    emit this->selected(0);
    return;
    }

  if (selectedRepresentations->GetNumberOfItems()<=0)
    {
    emit this->selected(0);
    return;
    }

  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    selectedRepresentations->GetItemAsObject(0));
  vtkSMSourceProxy* selectionSource = vtkSMSourceProxy::SafeDownCast(
    selectionSources->GetItemAsObject(0));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
  if (!repr)
    {
    // No data display was selected (or none that is registered).
    emit this->selected(0);
    return;
    }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* selectedSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());
  selectedSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  // Fire selection event to let the world know that this view selected
  // something.
  emit this->selected(opPort);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectPointsOnSurface(int rect[4])
{
  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> surfaceSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectOnSurface(rect[0], rect[1], rect[2], rect[3], 
      selectedRepresentations, selectionSources, surfaceSelections, false, true))
    {
    emit this->selected(0);
    return;
    }

  if (selectedRepresentations->GetNumberOfItems()<=0)
    {
    emit this->selected(0);
    return;
    }

  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    selectedRepresentations->GetItemAsObject(0));
  vtkSMSourceProxy* selectionSource = vtkSMSourceProxy::SafeDownCast(
    selectionSources->GetItemAsObject(0));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
  if (!repr)
    {
    // No data display was selected (or none that is registered).
    emit this->selected(0);
    return;
    }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* selectedSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());
  selectedSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  // Fire selection event to let the world know that this view selected
  // something.
  emit this->selected(opPort);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustum(int rect[4])
{

  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> frustumSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectFrustum(rect[0], rect[1], rect[2], rect[3], 
    selectedRepresentations, selectionSources, frustumSelections, false))
  {
    emit this->selected(0);
    return;
  }

  if (selectedRepresentations->GetNumberOfItems()<=0)
  {
    emit this->selected(0);
    return;
  }

  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    selectedRepresentations->GetItemAsObject(0));
  vtkSMSourceProxy* selectionSource = vtkSMSourceProxy::SafeDownCast(
    selectionSources->GetItemAsObject(0));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
  if (!repr)
  {
    // No data display was selected (or none that is registered).
    emit this->selected(0);
    return;
  }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* selectedSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());
  selectedSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  // Fire selection event to let the world know that this view selected
  // something.
  emit this->selected(opPort);
}

//-----------------------------------------------------------------------------
void pqRenderView::selectFrustumPoints(int rect[4])
{

  vtkSMRenderViewProxy* renderModuleP = this->getRenderViewProxy();

  vtkSmartPointer<vtkCollection> selectedRepresentations = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> frustumSelections = 
    vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> selectionSources = 
    vtkSmartPointer<vtkCollection>::New();
  if (!renderModuleP->SelectFrustum(rect[0], rect[1], rect[2], rect[3], 
    selectedRepresentations, selectionSources, frustumSelections, false, true))
  {
    emit this->selected(0);
    return;
  }

  if (selectedRepresentations->GetNumberOfItems()<=0)
  {
    emit this->selected(0);
    return;
  }

  vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
    selectedRepresentations->GetItemAsObject(0));
  vtkSMSourceProxy* selectionSource = vtkSMSourceProxy::SafeDownCast(
    selectionSources->GetItemAsObject(0));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqDataRepresentation* pqRepr = smmodel->findItem<pqDataRepresentation*>(repr);
  if (!repr)
  {
    // No data display was selected (or none that is registered).
    emit this->selected(0);
    return;
  }

  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* selectedSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());
  selectedSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  // Fire selection event to let the world know that this view selected
  // something.
  emit this->selected(opPort);
}
