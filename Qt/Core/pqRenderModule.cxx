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

// Qt includes.
#include <QFileInfo>
#include <QList>
#include <QPointer>
#include <QtDebug>

// ParaView includes.
#include "pqRenderViewProxy.h"
#include "pqSetName.h"
#include "pqUndoStack.h"
#include "vtkPVAxesWidget.h"

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
}

//-----------------------------------------------------------------------------
pqRenderModule::~pqRenderModule()
{
  delete this->Internal->Viewport;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy* pqRenderModule::getRenderModuleProxy() const
{
  return this->Internal->RenderModuleProxy;
}

//-----------------------------------------------------------------------------
QVTKWidget* pqRenderModule::getWidget() const
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

