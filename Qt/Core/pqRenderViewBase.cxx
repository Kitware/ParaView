// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
      QString txt = tr("Full resolution render in: %1 s")
                      .arg(QString::number(this->TimeLeftBeforeFullResolution));
      this->writeToStatusBar(txt.toUtf8().data());
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
          QObject::connect(menu, &QWidget::close, menu, &QObject::deleteLater);
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
  double value = prop ? prop->GetElement(0) : 0;
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
