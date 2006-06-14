/*=========================================================================

   Program:   ParaQ
   Module:    pqRenderModule.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

// ParaView includes.
#include "QVTKWidget.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderModuleProxy.h"

// Qt includes.
#include <QFileInfo>
#include <QPointer>
#include <QtDebug>

// ParaQ includes.
#include "pqRenderViewProxy.h"
#include "pqSetName.h"
#include "vtkPVAxesWidget.h"

class pqRenderModuleInternal
{
public:
  QPointer<QVTKWidget> Viewport;
  QString Name;
  vtkSmartPointer<pqRenderViewProxy> RenderViewProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMRenderModuleProxy> RenderModuleProxy;
  vtkSmartPointer<vtkPVAxesWidget> AxesWidget;

  pqRenderModuleInternal()
    {
    this->Viewport = 0;
    this->RenderViewProxy = vtkSmartPointer<pqRenderViewProxy>::New();
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->AxesWidget = vtkSmartPointer<vtkPVAxesWidget>::New();
    }

  ~pqRenderModuleInternal()
    {
    this->RenderViewProxy->setRenderModule(0);
    }
};

//-----------------------------------------------------------------------------
pqRenderModule::pqRenderModule(const QString& name, 
  vtkSMRenderModuleProxy* renModule, pqServer* server, QObject* _parent/*=null*/)
: pqProxy("render_modules", name, renModule, server, _parent)
{
  this->Internal = new pqRenderModuleInternal();
  this->Internal->Name = name;
  this->Internal->RenderViewProxy->setRenderModule(this);
  this->Internal->RenderModuleProxy = renModule;

  this->Internal->Viewport = new QVTKWidget() 
    << pqSetName("Viewport" + this->Internal->Name);
  this->Internal->Viewport->installEventFilter(this);

  vtkRenderWindow* renWin = renModule->GetRenderWindow();
  if (renWin)
    {
    this->renderModuleInit();
    }
  else
    {
    // the render module hasn't been created yet.
    // listen to the create event to connect the QVTKWidget.
    this->Internal->VTKConnect->Connect(renModule, vtkCommand::UpdateEvent,
      this, SLOT(onUpdateVTKObjects()));
    }
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
void pqRenderModule::setWindowParent(QWidget* _parent)
{
  this->Internal->Viewport->setParent(_parent);
  this->Internal->Viewport->update();
}

//-----------------------------------------------------------------------------
void pqRenderModule::renderModuleInit()
{
  if (!this->Internal->Viewport || !this->Internal->RenderViewProxy)
    {
    qDebug() << "renderModuleInit() missing information.";
    return;
    }
  this->Internal->Viewport->SetRenderWindow(
    this->Internal->RenderModuleProxy->GetRenderWindow());

  // Enable interaction on this client.
  vtkPVGenericRenderWindowInteractor* iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(
      this->Internal->RenderModuleProxy->GetInteractor());
  iren->SetPVRenderView(this->Internal->RenderViewProxy);
  iren->Enable();

  // Init axes actor.
  this->Internal->AxesWidget->SetParentRenderer(
    this->Internal->RenderModuleProxy->GetRenderer());
  this->Internal->AxesWidget->SetViewport(0, 0, 0.25, 0.25);
  this->Internal->AxesWidget->SetInteractor(iren);
  this->Internal->AxesWidget->SetEnabled(1);
  this->Internal->AxesWidget->SetInteractive(0);
}

//-----------------------------------------------------------------------------
void pqRenderModule::onUpdateVTKObjects()
{
  this->renderModuleInit();
  // no need to listen to any more events.
  this->Internal->VTKConnect->Disconnect(
    this->Internal->RenderModuleProxy);
}

//-----------------------------------------------------------------------------
void pqRenderModule::render()
{
  if (this->Internal->RenderModuleProxy)
    {
    this->Internal->Viewport->update();
    }
}

//-----------------------------------------------------------------------------
void pqRenderModule::forceRender()
{
  if (this->Internal->RenderModuleProxy)
    {
    this->Internal->RenderModuleProxy->StillRender();
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
bool pqRenderModule::eventFilter(QObject* caller, QEvent* e)
{
  // TODO, apparently, this should watch for window position changes, not resizes
  /* FIXME
  if(e->type() == QEvent::Resize)
    {
    // find top level window;
    QWidget* me = qobject_cast<QWidget*>(caller);

    vtkSMIntVectorProperty* prop = 0;

    // set size of main window
    prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("GUISize"));
    if(prop)
      {
      prop->SetElements2(this->TopWidget->width(), this->TopWidget->height());
      }

    // position relative to main window
    prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("WindowPosition"));
    if(prop)
      {
      QPoint pos(0,0);
      pos = me->mapTo(this->TopWidget, pos);
      prop->SetElements2(pos.x(), pos.y());
      }
    }
    */
  return QObject::eventFilter(caller, e);
}
