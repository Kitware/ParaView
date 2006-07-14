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
#include "pqApplicationCore.h"
#include "pqMultiViewFrame.h"
#include "pqPipelineDisplay.h"
#include "pqRenderViewProxy.h"
#include "pqServerManagerModel.h"
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
  QString Name;
  vtkSmartPointer<pqRenderViewProxy> RenderViewProxy;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMRenderModuleProxy> RenderModuleProxy;
  vtkSmartPointer<vtkPVAxesWidget> AxesWidget;
  pqUndoStack* UndoStack;

  // List of displays shown by this render module.
  QList<QPointer<pqPipelineDisplay> > Displays;

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
: pqProxy("render_modules", name, renModule, server, _parent)
{
  this->Internal = new pqRenderModuleInternal();
  this->Internal->Name = name;
  this->Internal->RenderViewProxy->setRenderModule(this);
  this->Internal->RenderModuleProxy = renModule;
  this->Internal->UndoStack->setActiveServer(this->getServer());

  this->Internal->Viewport = new QVTKWidget() 
    << pqSetName("Viewport" + this->Internal->Name);
  this->Internal->Viewport->installEventFilter(this);

  // Listen to display add/remove events.
  this->Internal->VTKConnect->Connect(renModule->GetProperty("Displays"),
    vtkCommand::ModifiedEvent, this, SLOT(displaysChanged()));

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
  foreach(pqPipelineDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      disp->removeRenderModule(this);
      }
    }

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
   
  iren->SetInteractorStyle(style);
  style->Delete();

  this->Internal->VTKConnect->Connect(style,
    vtkCommand::StartInteractionEvent, 
    this, SLOT(startInteraction()));
  this->Internal->VTKConnect->Connect(style,
    vtkCommand::EndInteractionEvent, 
    this, SLOT(endInteraction()));

  iren->Enable();

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
  this->Internal->UndoStack->BeginOrContinueUndoSet("Interaction");

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
void pqRenderModule::onUpdateVTKObjects()
{
  this->renderModuleInit();
  // no need to listen to any more events.
  this->Internal->VTKConnect->Disconnect(
    this->Internal->RenderModuleProxy);
}

//-----------------------------------------------------------------------------
void pqRenderModule::displaysChanged()
{
  // Determine what changed. Add the new displays and remove the old
  // ones. Make sure new displays have a reference to this render module.
  // Remove the reference to this render module in the removed displays.
  QList<QPointer<pqPipelineDisplay> > currentDisplays;
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Displays"));
  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();

  unsigned int max = prop->GetNumberOfProxies();
  for (unsigned int cc=0; cc < max; ++cc)
    {
    vtkSMProxy* proxy = prop->GetProxy(cc);
    if (!proxy)
      {
      continue;
      }
    pqPipelineDisplay* display = smModel->getPQDisplay(proxy);
    if (!display)
      {
      continue;
      }
    currentDisplays.append(QPointer<pqPipelineDisplay>(display));
    if(!this->Internal->Displays.contains(display))
      {
      // Update the render module pointer in the display.
      display->addRenderModule(this);
      this->Internal->Displays.append(QPointer<pqPipelineDisplay>(display));
      }
    }

  QList<QPointer<pqPipelineDisplay> >::Iterator iter =
      this->Internal->Displays.begin();
  while(iter != this->Internal->Displays.end())
    {
    if(!currentDisplays.contains(*iter))
      {
      // Remove the render module pointer from the display.
      (*iter)->removeRenderModule(this);
      iter = this->Internal->Displays.erase(iter);
      }
    else
      {
      ++iter;
      }
    }
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
    vtkProcessModule::GetProcessModule()->SendPrepareProgress(
      this->Internal->RenderModuleProxy->GetConnectionID());
    this->Internal->RenderModuleProxy->StillRender();
    vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress(
      this->Internal->RenderModuleProxy->GetConnectionID());
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
bool pqRenderModule::hasDisplay(pqPipelineDisplay* display)
{
  return this->Internal->Displays.contains(display);
}

//-----------------------------------------------------------------------------
int pqRenderModule::getDisplayCount() const
{
  return this->Internal->Displays.size();
}

//-----------------------------------------------------------------------------
pqPipelineDisplay* pqRenderModule::getDisplay(int index) const
{
  if(index >= 0 && index < this->Internal->Displays.size())
    {
    return this->Internal->Displays[index];
    }

  return 0;
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
  
  if(e->type() == QEvent::MouseButtonPress)
    {
    QVTKWidget* view = qobject_cast<QVTKWidget*>(caller);
    if (view)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        view->parentWidget());
      if (frame)
        {
        frame->setActive(true);
        }
      }
    }

  return QObject::eventFilter(caller, e);
}
