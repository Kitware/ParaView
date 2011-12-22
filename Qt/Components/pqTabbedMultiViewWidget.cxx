/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqTabbedMultiViewWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqMultiViewWidget.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkSMProxyManager.h"
#include "vtkSMViewLayoutProxy.h"

#include <QMultiMap>
#include <QtDebug>

class pqTabbedMultiViewWidget::pqInternals
{
public:
  QMultiMap<vtkIdType, QPointer<pqMultiViewWidget> > TabWidgets;
};

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::pqTabbedMultiViewWidget(QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqInternals())
{
  pqApplicationCore::instance()->registerManager(
    "MULTIVIEW_WIDGET", this);

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();

  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(proxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(proxyRemoved(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(preServerRemoved(pqServer*)),
    this, SLOT(serverRemoved(pqServer*)));

  this->addTab(new QWidget(this), "+");
  QObject::connect(this, SIGNAL(currentChanged(int)),
    this, SLOT(currentTabChanged(int)));
}

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::~pqTabbedMultiViewWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyAdded(pqProxy* proxy)
{
  if (proxy->getSMGroup() == "layouts" &&
    proxy->getProxy()->IsA("vtkSMViewLayoutProxy"))
    {
    this->createTab(vtkSMViewLayoutProxy::SafeDownCast(proxy->getProxy())); 
    }
  else if (qobject_cast<pqView*>(proxy))
    {
    // FIXME: we may want to give server-manager the opportunity to place the
    // view after creation, if it wants. The GUI should try to find a place for
    // it, only if the server-manager (through undo-redo, or loading state or
    // Python or collaborative-client).
    pqMultiViewWidget* frame =
      qobject_cast<pqMultiViewWidget*>(this->currentWidget());

    if (!frame)
      {
      // implies no vtkSMViewLayoutProxy was registered for this session.
      this->createTab();
      frame = qobject_cast<pqMultiViewWidget*>(this->currentWidget());
      }

    if (frame)
      {
      frame->assignToFrame(qobject_cast<pqView*>(proxy));
      }
    else
      {
      qCritical() <<
        "A new view was added, but pqTabbedMultiViewWidget has no "
        "idea where to put this view.";
      }
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyRemoved(pqProxy* proxy)
{
  if (proxy->getSMGroup() == "layouts" &&
    proxy->getProxy()->IsA("vtkSMViewLayoutProxy"))
    {
    vtkSMProxy* smproxy = proxy->getProxy();

    QList<QPointer<pqMultiViewWidget> > widgets =
      this->Internals->TabWidgets.values();
    foreach (QPointer<pqMultiViewWidget> widget, widgets)
      {
      if (widget && widget->layoutManager() == smproxy)
        {
        this->Internals->TabWidgets.remove(
          proxy->getServer()->GetConnectionID(), widget);
        int index = this->indexOf(widget);
        if (this->currentWidget() == widget)
          {
          this->setCurrentIndex(((index-1) > 0)? (index-1) : 0);
          }
        this->removeTab(index);
        delete widget;
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::serverRemoved(pqServer* server)
{
  // remove all tabs corresponding to the closed session.
  QList<QPointer<pqMultiViewWidget> > widgets =
    this->Internals->TabWidgets.values(server->GetConnectionID());
  foreach (pqMultiViewWidget* widget, widgets)
    {
    int cur_index = this->indexOf(widget);
    if (cur_index != -1)
      {
      this->removeTab(cur_index);
      }
    delete widget;
    }

  this->Internals->TabWidgets.remove(server->GetConnectionID());
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::currentTabChanged(int index)
{
  if (index < (this->count()-1))
    {
    // make the first frame active.
    pqMultiViewWidget* frame =
      qobject_cast<pqMultiViewWidget*>(this->currentWidget());
    frame->makeFrameActive();
    }
  else if (index == (this->count()-1) && index != 0)
    {
    // index !=0 check keeps this widget from creating new tabs as the tabs are
    // being removed.
    this->createTab();
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::closeTab(int index)
{
  pqMultiViewWidget* widget = qobject_cast<pqMultiViewWidget*>(
    this->widget(index));
  vtkSMProxy* vlayout = widget? widget->layoutManager() : NULL;
  if (vlayout)
    {
    pqServerManagerModel* smmodel =
      pqApplicationCore::instance()->getServerManagerModel();
    pqObjectBuilder* builder =
      pqApplicationCore::instance()->getObjectBuilder();

    BEGIN_UNDO_SET("Remove View Tab");
    builder->destroy(smmodel->findItem<pqProxy*>(vlayout));
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::createTab()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
    {
    BEGIN_UNDO_SET("Add View Tab");
    vtkSMProxy* vlayout = pqApplicationCore::instance()->getObjectBuilder()->
      createProxy("misc", "ViewLayout", server, "layouts");
    Q_ASSERT(vlayout != NULL);
    (void)vlayout;
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::createTab(vtkSMViewLayoutProxy* vlayout)
{
  pqMultiViewWidget* widget = new pqMultiViewWidget(this);
  widget->setLayoutManager(vlayout);

  int tab_index = this->insertTab(
    this->count()-1, widget, QString("Layout #%1").arg(this->count()));
  this->setCurrentIndex(tab_index);

  vtkIdType cid =
    pqApplicationCore::instance()->getServerManagerModel()->findServer(
      vlayout->GetSession())->GetConnectionID();
  this->Internals->TabWidgets.insert(cid, widget);
}


//-----------------------------------------------------------------------------
vtkImageData* pqTabbedMultiViewWidget::captureImage(int dx, int dy)
{
  pqMultiViewWidget* widget = qobject_cast<pqMultiViewWidget*>(
    this->currentWidget());
  if (widget)
    {
    return widget->captureImage(dx, dy);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
QSize pqTabbedMultiViewWidget::clientSize() const
{
  if (this->currentWidget())
    {
    return this->currentWidget()->size();
    }

  return this->size();
}
