/*=========================================================================

   Program: ParaView
   Module:    pqRenderModule.cxx

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
#include "pqRenderModule.h"

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
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkTrackballPan.h"
#include "vtkSMIntVectorProperty.h"

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
#include "pqSetName.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "vtkPVAxesWidget.h"

static QSet<pqRenderModule*> RenderModules;

/// method to keep server side views in sync with client side views
/// the client has multiple windows, whereas the server has just one window with
/// multiple sub views.  We need to tell the server where to put the views.
static void UpdateServerViews(pqRenderModule* renderModule)
{
  // get all the render modules on the server
  pqServer* server = renderModule->getServer();
  QList<pqRenderModule*> rms;
  foreach(pqRenderModule* rm, RenderModules)
    {
    if(rm->getServer() == server)
      {
      rms.append(rm);
      }
    }

  // find a rectangle that bounds all views
  QRect totalBounds;
  
  foreach(pqRenderModule* rm, rms)
    {
    QRect bounds = rm->getWidget()->rect();
    bounds.moveTo(rm->getWidget()->mapToGlobal(QPoint(0,0)));
    totalBounds |= bounds;
    }

  // now loop through all render modules and set the server size window size
  foreach(pqRenderModule* rm, rms)
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

template<class T>
inline uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqRenderModuleInternal
{
public:
  QPointer<QVTKWidget> Viewport;
  vtkSmartPointer<pqRenderViewProxy> RenderViewProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMRenderModuleProxy> RenderModuleProxy;
  vtkSmartPointer<vtkPVAxesWidget> AxesWidget;
  pqUndoStack* UndoStack;

  pqRenderModuleInternal()
    {
    this->Viewport = 0;
    this->RenderViewProxy = vtkSmartPointer<pqRenderViewProxy>::New();
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->AxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();
    this->UndoStack = new pqUndoStack(true);
    }

  ~pqRenderModuleInternal()
    {
    this->RenderViewProxy->setRenderModule(0);
    delete this->UndoStack;
    }
};

//-----------------------------------------------------------------------------
pqRenderModule::pqRenderModule(const QString& name, 
  vtkSMRenderModuleProxy* renModule, pqServer* server, QObject* _parent/*=null*/)
: pqGenericViewModule("render_modules", name, renModule, server, _parent)
{
  this->Internal = new pqRenderModuleInternal();
  this->Internal->RenderViewProxy->setRenderModule(this);
  this->Internal->RenderModuleProxy = renModule;
  this->Internal->UndoStack->setActiveServer(this->getServer());

  this->Internal->Viewport = new QVTKWidget() 
    << pqSetName("Viewport");
  // do image caching for performance
  this->Internal->Viewport->setAutomaticImageCacheEnabled(true);
  RenderModules.insert(this);

  this->Internal->Viewport->installEventFilter(this);
}

//-----------------------------------------------------------------------------
pqRenderModule::~pqRenderModule()
{
  RenderModules.remove(this);

  delete this->Internal->Viewport;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy* pqRenderModule::getRenderModuleProxy() const
{
  return this->Internal->RenderModuleProxy;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderModule::getWidget()
{
  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
pqUndoStack* pqRenderModule::getInteractionUndoStack() const
{
  return this->Internal->UndoStack;
}

//-----------------------------------------------------------------------------
void pqRenderModule::setWindowParent(QWidget* _parent)
{
  this->Internal->Viewport->setParent(_parent);
  this->Internal->Viewport->update();
}

//-----------------------------------------------------------------------------
QWidget* pqRenderModule::getWindowParent() const
{
  return this->Internal->Viewport->parentWidget();
}

//-----------------------------------------------------------------------------
void pqRenderModule::viewModuleInit()
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
  this->Internal->AxesWidget->SetParentRenderer(
    this->Internal->RenderModuleProxy->GetRenderer());
  this->Internal->AxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->AxesWidget->SetInteractor(iren);
  this->Internal->AxesWidget->SetEnabled(1);
  this->Internal->AxesWidget->SetInteractive(0);

  // Set up interactor styles.
  vtkPVInteractorStyle* style = vtkPVInteractorStyle::New();
  vtkCameraManipulator* manip = vtkPVTrackballRotate::New();
  style->AddManipulator(manip);
  manip->Delete();
  manip = vtkTrackballPan::New();
  manip->SetButton(2);
  style->AddManipulator(manip);
  manip->Delete();
  manip = vtkPVTrackballZoom::New();
  manip->SetButton(3);
  style->AddManipulator(manip);
  manip->Delete();
  manip = vtkTrackballPan::New();
  manip->SetButton(1);
  manip->SetControl(1);
  style->AddManipulator(manip);
  manip->Delete();
  manip = vtkPVTrackballZoom::New();
  manip->SetButton(1);
  manip->SetShift(1);
  style->AddManipulator(manip);
  manip->Delete();
   
  iren->SetInteractorStyle(style);
  style->Delete();

  this->Internal->VTKConnect->Connect(style,
    vtkCommand::StartInteractionEvent, 
    this, SLOT(startInteraction()));
  this->Internal->VTKConnect->Connect(style,
    vtkCommand::EndInteractionEvent, 
    this, SLOT(endInteraction()));
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

  iren->Enable();

  this->Superclass::viewModuleInit();
}

//-----------------------------------------------------------------------------
void pqRenderModule::startInteraction()
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
void pqRenderModule::endInteraction()
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
void pqRenderModule::onStartEvent()
{
  emit this->beginRender();
}

//-----------------------------------------------------------------------------
void pqRenderModule::onEndEvent()
{
  emit this->endRender();
}

//-----------------------------------------------------------------------------
void pqRenderModule::render()
{
  if (this->Internal->RenderModuleProxy && this->Internal->Viewport)
    {
    this->Internal->Viewport->update();
    }
}

//-----------------------------------------------------------------------------
void pqRenderModule::resetCamera()
{
  if (this->Internal->RenderModuleProxy)
    {
    this->Internal->RenderModuleProxy->ResetCamera();
    }
}

//-----------------------------------------------------------------------------
bool pqRenderModule::saveImage(int width, int height, const QString& filename)
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
      << filename.toStdString().c_str();
    return false;
    }

  int ret = this->Internal->RenderModuleProxy->WriteImage(
    filename.toStdString().c_str(), writername);
  if (width>0 && height>0)
    {
    this->Internal->Viewport->resize(width, height);
    this->Internal->Viewport->resize(cur_size);
    this->render();
    }
  return (ret == vtkErrorCode::NoError);
}

//-----------------------------------------------------------------------------
void pqRenderModule::setDefaults()
{
  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODResolution"), 50);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODThreshold"), 5);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("CompositeThreshold"), 3);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("SquirtLevel"), 3);

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QList<QString> propertyNames;
  propertyNames.push_back("CameraParallelProjection");
  propertyNames.push_back("UseTriangleStrips");
  propertyNames.push_back("UseImmediateMode");
  propertyNames.push_back("LODThreshold");
  propertyNames.push_back("LODResolution");
  propertyNames.push_back("RenderInterruptsEnabled");
  propertyNames.push_back("CompositeThreshold");
  propertyNames.push_back("ReductionFactor");
  propertyNames.push_back("SquirtLevel");
  propertyNames.push_back("OrderedCompositing");
  foreach(QString property_name, propertyNames)
    {
    QString key = QString("renderModule/") + property_name;
    if (proxy->GetProperty(property_name.toAscii().data()) && settings->contains(key))
      {
      pqSMAdaptor::setElementProperty(
        proxy->GetProperty(property_name.toAscii().data()),
        settings->value("renderModule/" + property_name));
      }
    }
  if (settings->contains("renderModule/Background"))
    {
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("Background"),
      settings->value("renderModule/Background").value<QList<QVariant> >());
    }
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqRenderModule::eventFilter(QObject* caller, QEvent* e)
{
  // TODO, apparently, this should watch for window position changes, not resizes
  
  if(e->type() == QEvent::Resize)
    {
    UpdateServerViews(this);
    }
  
  return QObject::eventFilter(caller, e);
}

