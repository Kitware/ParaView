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
#include "vtkSMProxyManager.h"
#include "vtkSMViewLayoutProxy.h"

#include <QMultiMap>

class pqTabbedMultiViewWidget::pqInternals
{
public:
  QMultiMap<vtkIdType, pqMultiViewWidget*> TabWidgets;
};

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::pqTabbedMultiViewWidget(QWidget* parentObject)
  : Superclass(parentObject),
  Internals(new pqInternals())
{
  pqServerManagerObserver* smobserver =
    pqApplicationCore::instance()->getServerManagerObserver();

  QObject::connect(smobserver,
    SIGNAL(proxyRegistered(const QString&, const QString&, vtkSMProxy*)),
    this,
    SLOT(proxyRegistered(const QString&, const QString&, vtkSMProxy*)));

  QObject::connect(smobserver,
    SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this,
    SLOT(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)));

  QObject::connect(smobserver, SIGNAL(connectionCreated(vtkIdType)),
    this, SLOT(connectionCreated(vtkIdType)));

  QObject::connect(smobserver, SIGNAL(connectionClosed(vtkIdType)),
    this, SLOT(connectionClosed(vtkIdType)));

  this->addTab(new QWidget(this), "+");
  QObject::connect(this, SIGNAL(currentChanged(int)),
    this, SLOT(checkToAddTab(int)));
}

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::~pqTabbedMultiViewWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyRegistered(
  const QString& group, const QString& name, vtkSMProxy* proxy)
{
  if (group == "layouts" && proxy && proxy->IsA("vtkSMViewLayoutProxy"))
    {
    this->createTab(vtkSMViewLayoutProxy::SafeDownCast(proxy)); 
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyUnRegistered(
  const QString& group, const QString& name, vtkSMProxy* proxy)
{
  if (group == "layouts" && proxy && proxy->IsA("vtkSMViewLayoutProxy"))
    {
    QList<pqMultiViewWidget*> widgets = this->Internals->TabWidgets.values();
    foreach (pqMultiViewWidget* widget, widgets)
      {
      if (widget && widget->layoutManager() == proxy)
        {
        this->closeTab(this->indexOf(widget));
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::connectionCreated(vtkIdType connectionId)
{
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::connectionClosed(vtkIdType connectionId)
{
  // remove all tabs corresponding to the closed session.
  QList<pqMultiViewWidget*> widgets =
    this->Internals->TabWidgets.values(connectionId);
  foreach (pqMultiViewWidget* widget, widgets)
    {
    int cur_index = this->indexOf(widget);
    if (cur_index != -1)
      {
      this->removeTab(cur_index);
      }
    delete widget;
    }

  this->Internals->TabWidgets.remove(connectionId);
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::checkToAddTab(int index)
{
  if (index == (this->count()-1))
    {
    this->createTab();
    }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::closeTab(int index)
{
  BEGIN_UNDO_SET("Remove View Tab");

  pqMultiViewWidget* widget = qobject_cast<pqMultiViewWidget*>(
    this->widget(index));
  this->removeTab(index);

  vtkSMProxy* vlayout = widget? widget->layoutManager() : NULL;
  delete widget;

  if (vlayout)
    {
    vtkSMProxyManager* pxm = vlayout->GetProxyManager();
    pxm->UnRegisterProxy(vlayout);
    }
  END_UNDO_SET();
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
