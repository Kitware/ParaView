/*=========================================================================

   Program: ParaView
   Module:    pqRenderViewBase.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqRenderViewBase.h"

// Server Manager Includes.
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxyInteractorHelper.h"

// Qt Includes.
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMapIterator>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QStatusBar>
#include <QtDebug>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqQVTKWidget.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimer.h"

#include <cassert>
#include <string>

class pqRenderViewBase::pqInternal
{
public:
  QPoint MouseOrigin;
  bool IsInteractiveDelayActive;
  double TimeLeftBeforeFullResolution;

  pqInternal() = default;
  ~pqInternal() = default;

  void writeToStatusBar(const char* txt)
  {
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
    if (mainWindow)
    {
      mainWindow->statusBar()->showMessage(txt);
    }
  }

  void startInteractiveRenderDelay(double timeLeft)
  {
    this->IsInteractiveDelayActive = true;
    this->TimeLeftBeforeFullResolution = timeLeft;
    tick();
  }

  //-----------------------------------------------------------------------------
  void interactiveRenderDelayTimeOut()
  {
    this->IsInteractiveDelayActive = false;
    tick();
  }

  //-----------------------------------------------------------------------------
  void tick()
  {
    if (this->IsInteractiveDelayActive)
    {
      QString txt = "Full resolution render in: ";
      txt += QString::number(this->TimeLeftBeforeFullResolution);
      txt += " s";
      this->writeToStatusBar(txt.toLocal8Bit().data());
      this->TimeLeftBeforeFullResolution -= 0.1;
    }
    else
    {
      this->writeToStatusBar("");
    }
  }
};

//-----------------------------------------------------------------------------
pqRenderViewBase::pqRenderViewBase(const QString& type, const QString& group, const QString& name,
  vtkSMViewProxy* renViewProxy, pqServer* server, QObject* _parent /*=nullptr*/)
  : Superclass(type, group, name, renViewProxy, server, _parent)
{
  this->Internal = new pqRenderViewBase::pqInternal();
  this->InteractiveDelayUpdateTimer = new pqTimer(this);
}

//-----------------------------------------------------------------------------
pqRenderViewBase::~pqRenderViewBase()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderViewBase::createWidget()
{
  pqQVTKWidget* vtkwidget = new pqQVTKWidget();
  vtkwidget->setViewProxy(this->getProxy());
  vtkwidget->setContextMenuPolicy(Qt::NoContextMenu);
  vtkwidget->installEventFilter(this);
  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::initialize()
{
  this->Superclass::initialize();

  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the pqQVTKWidgetBase
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we
  // do the following.
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy->GetObjectsCreated())
  {
    // Wait till first UpdateVTKObjects() call on the render module.
    // Under usual circumstances, after UpdateVTKObjects() the
    // render module objects will be created.
    this->getConnector()->Connect(
      proxy, vtkCommand::UpdateEvent, this, SLOT(initializeAfterObjectsCreated()));
  }
  else
  {
    this->initializeAfterObjectsCreated();
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::initializeAfterObjectsCreated()
{
  // Attach Qt Signal to VTK interactor Delay event
  vtkSMRenderViewProxy* renderViewProxy;
  renderViewProxy = vtkSMRenderViewProxy::SafeDownCast(this->getProxy());
  if (renderViewProxy != nullptr)
  {
    vtkSMViewProxyInteractorHelper* helper = renderViewProxy->GetInteractorHelper();
    assert(helper);
    vtkEventQtSlotConnect* cntor = this->getConnector();
    cntor->Connect(helper, vtkCommand::CreateTimerEvent, this, SLOT(beginDelayInteractiveRender()));
    cntor->Connect(helper, vtkCommand::DestroyTimerEvent, this, SLOT(endDelayInteractiveRender()));
    cntor->Connect(helper, vtkCommand::TimerEvent, this, SLOT(endDelayInteractiveRender()));

    this->InteractiveDelayUpdateTimer->setSingleShot(false);
    QObject::connect(
      this->InteractiveDelayUpdateTimer, SIGNAL(timeout()), this, SLOT(updateStatusMessage()));
  }
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::resetDisplay(bool closest)
{
  this->resetCamera(closest);
}

//-----------------------------------------------------------------------------
bool pqRenderViewBase::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
    {
      this->Internal->MouseOrigin = me->pos();
    }
  }
  else if (e->type() == QEvent::MouseMove && !this->Internal->MouseOrigin.isNull())
  {
    QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
    QPoint delta = newPos - this->Internal->MouseOrigin;
    if (delta.manhattanLength() < 3)
    {
      this->Internal->MouseOrigin = QPoint();
    }
  }
  else if (e->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Internal->MouseOrigin.isNull())
    {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if (delta.manhattanLength() < 3 && qobject_cast<QWidget*>(caller))
      {
        QList<QAction*> actions = this->widget()->actions();
        if (!actions.isEmpty())
        {
          QMenu* menu = new QMenu(this->widget());
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(qobject_cast<QWidget*>(caller)->mapToGlobal(newPos));
        }
      }
      this->Internal->MouseOrigin = QPoint();
    }
  }

  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::beginDelayInteractiveRender()
{
  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
    this->getProxy()->GetProperty("NonInteractiveRenderDelay"));
  double value = prop ? static_cast<double>(prop->GetElement(0)) : 0;
  this->Internal->startInteractiveRenderDelay(value);
  this->InteractiveDelayUpdateTimer->start(100);
}
//-----------------------------------------------------------------------------
void pqRenderViewBase::endDelayInteractiveRender()
{
  this->Internal->interactiveRenderDelayTimeOut();
  this->InteractiveDelayUpdateTimer->stop();
}
//-----------------------------------------------------------------------------
void pqRenderViewBase::updateStatusMessage()
{
  this->Internal->tick();
}
