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
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"

#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QMultiMap>
#include <QPointer>
#include <QShortcut>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtDebug>

//-----------------------------------------------------------------------------
// ******************** pqTabWidget **********************
//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::pqTabWidget::pqTabWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , ReadOnly(false)
{
}

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::pqTabWidget::~pqTabWidget()
{
}

//-----------------------------------------------------------------------------
int pqTabbedMultiViewWidget::pqTabWidget::tabButtonIndex(
  QWidget* wdg, QTabBar::ButtonPosition position) const
{
  for (int cc = 0; cc < this->count(); cc++)
  {
    if (this->tabBar()->tabButton(cc, position) == wdg)
    {
      return cc;
    }
  }
  return -1;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::pqTabWidget::setTabButton(
  int index, QTabBar::ButtonPosition position, QWidget* wdg)
{
  this->tabBar()->setTabButton(index, position, wdg);
}

//-----------------------------------------------------------------------------
int pqTabbedMultiViewWidget::pqTabWidget::addAsTab(
  pqMultiViewWidget* wdg, pqTabbedMultiViewWidget* self)
{
  int tab_count = this->count();
  vtkSMViewLayoutProxy* proxy = wdg->layoutManager();
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* item = smmodel->findItem<pqProxy*>(proxy);
  QString name = QString("Layout #%1").arg(tab_count);
  if (item->getSMName().startsWith("ViewLayout"))
  {
    item->rename(name);
  }
  int tab_index = this->insertTab(tab_count - 1, wdg, item->getSMName());

  SM_SCOPED_TRACE(CallFunction).arg("CreateLayout").arg(name.toUtf8().data());

  this->connect(item, SIGNAL(nameChanged(pqServerManagerModelItem*)), self,
    SLOT(onLayoutNameChanged(pqServerManagerModelItem*)));

  QLabel* label = new QLabel(this);
  label->setObjectName("popout");
  label->setToolTip(pqTabWidget::popoutLabelText(false));
  label->setStatusTip(pqTabWidget::popoutLabelText(false));
  label->setPixmap(this->style()->standardPixmap(pqTabWidget::popoutLabelPixmap(false)));
  this->setTabButton(tab_index, QTabBar::LeftSide, label);
  label->installEventFilter(self);

  label = new QLabel(this);
  label->setObjectName("close");
  label->setToolTip("Close layout");
  label->setStatusTip("Close layout");
  label->setPixmap(this->style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
  this->setTabButton(tab_index, QTabBar::RightSide, label);
  label->installEventFilter(self);
  label->setVisible(!this->ReadOnly);
  return tab_index;
}

//-----------------------------------------------------------------------------
const char* pqTabbedMultiViewWidget::pqTabWidget::popoutLabelText(bool popped_out)
{
  return popped_out ? "Bring popped out window back to the frame"
                    : "Pop out layout in separate window";
}

//-----------------------------------------------------------------------------
QStyle::StandardPixmap pqTabbedMultiViewWidget::pqTabWidget::popoutLabelPixmap(bool popped_out)
{
  return popped_out ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::pqTabWidget::setReadOnly(bool val)
{
  if (this->ReadOnly == val)
  {
    return;
  }

  this->ReadOnly = val;
  QList<QLabel*> labels = this->findChildren<QLabel*>("close");
  foreach (QLabel* label, labels)
  {
    label->setVisible(!val);
  }
}

//-----------------------------------------------------------------------------
// ****************     pqTabbedMultiViewWidget   **********************
//-----------------------------------------------------------------------------
class pqTabbedMultiViewWidget::pqInternals
{
public:
  QPointer<pqTabWidget> TabWidget;
  QMultiMap<pqServer*, QPointer<pqMultiViewWidget> > TabWidgets;
  QPointer<QWidget> FullScreenWindow;
  QPointer<QWidget> NewTabWidget;

  /// returns a frame that can be used to assign the view proxy. May return NULL
  /// if no suitable frame is found.
  pqMultiViewWidget* assignableFrame(pqProxy* view)
  {
    pqMultiViewWidget* current = qobject_cast<pqMultiViewWidget*>(this->TabWidget->currentWidget());
    if (current && this->TabWidgets.contains(view->getServer(), current))
    {
      return current;
    }
    if (this->TabWidgets.count(view->getServer()) > 0)
    {
      return this->TabWidgets.value(view->getServer());
    }
    return NULL;
  }

  void addNewTabWidget()
  {
    if (!this->NewTabWidget)
    {
      this->NewTabWidget = new QWidget(this->TabWidget);
      this->TabWidget->addTab(this->NewTabWidget, "+");
    }
  }
  void removeNewTabWidget()
  {
    if (this->NewTabWidget)
    {
      this->TabWidget->removeTab(this->TabWidget->indexOf(this->NewTabWidget));
      delete this->NewTabWidget;
    }
  }
};

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::pqTabbedMultiViewWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals(new pqInternals())
{
  this->Internals->TabWidget = new pqTabWidget(this);
  this->Internals->TabWidget->setObjectName("CoreWidget");

  this->Internals->TabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this->Internals->TabWidget->tabBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
    this, SLOT(contextMenuRequested(const QPoint&)));

  QVBoxLayout* vbox = new QVBoxLayout();
  this->setLayout(vbox);
  vbox->setMargin(0);
  vbox->setSpacing(0);
  vbox->addWidget(this->Internals->TabWidget);

  pqApplicationCore* core = pqApplicationCore::instance();

  core->registerManager("MULTIVIEW_WIDGET", this);

  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)), this, SLOT(proxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)), this, SLOT(proxyRemoved(pqProxy*)));
  QObject::connect(
    smmodel, SIGNAL(preServerRemoved(pqServer*)), this, SLOT(serverRemoved(pqServer*)));

  this->Internals->addNewTabWidget();
  QObject::connect(
    this->Internals->TabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));

  // we need to ensure that all views loaded from the state do get assigned to
  // some layout correctly.
  QObject::connect(
    core, SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this, SLOT(onStateLoaded()));

  QObject::connect(core->getObjectBuilder(), SIGNAL(aboutToCreateView(pqServer*)), this,
    SLOT(aboutToCreateView(pqServer*)));
}

//-----------------------------------------------------------------------------
pqTabbedMultiViewWidget::~pqTabbedMultiViewWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::setReadOnly(bool val)
{
  if (val != this->readOnly())
  {
    this->Internals->TabWidget->setReadOnly(val);
    if (val)
    {
      this->Internals->removeNewTabWidget();
    }
    else
    {
      this->Internals->addNewTabWidget();
    }
  }
}

//-----------------------------------------------------------------------------
bool pqTabbedMultiViewWidget::readOnly() const
{
  return this->Internals->TabWidget->readOnly();
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::setTabVisibility(bool visible)
{
  this->Internals->TabWidget->tabBar()->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool pqTabbedMultiViewWidget::tabVisibility() const
{
  return this->Internals->TabWidget->tabBar()->isVisible();
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::toggleFullScreen()
{
  if (this->Internals->FullScreenWindow)
  {
    this->Internals->FullScreenWindow->layout()->removeWidget(this->Internals->TabWidget);
    this->layout()->addWidget(this->Internals->TabWidget);
    delete this->Internals->FullScreenWindow;
  }
  else
  {
    QWidget* fullScreenWindow = new QWidget(this, Qt::Window);
    this->Internals->FullScreenWindow = fullScreenWindow;
    fullScreenWindow->setObjectName("FullScreenWindow");
    this->layout()->removeWidget(this->Internals->TabWidget);

    QVBoxLayout* vbox = new QVBoxLayout(fullScreenWindow);
    vbox->setSpacing(0);
    vbox->setMargin(0);

    vbox->addWidget(this->Internals->TabWidget);
    fullScreenWindow->showFullScreen();
    fullScreenWindow->show();

    QShortcut* esc = new QShortcut(Qt::Key_Escape, fullScreenWindow);
    QObject::connect(esc, SIGNAL(activated()), this, SLOT(toggleFullScreen()));
    QShortcut* f11 = new QShortcut(Qt::Key_F11, fullScreenWindow);
    QObject::connect(f11, SIGNAL(activated()), this, SLOT(toggleFullScreen()));
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyAdded(pqProxy* proxy)
{
  if (proxy->getSMGroup() == "layouts" && proxy->getProxy()->IsA("vtkSMViewLayoutProxy"))
  {
    this->createTab(vtkSMViewLayoutProxy::SafeDownCast(proxy->getProxy()));
  }
  else if (qobject_cast<pqView*>(proxy))
  {
    pqView* view = qobject_cast<pqView*>(proxy);
    if (pqApplicationCore::instance()->isLoadingState() || view == NULL)
    {
      return;
    }
    // also check with the proxy manager, since the pqApplicationCore's state
    // loading flag won't get set if state was being loaded from Python Shell.
    vtkSMSessionProxyManager* pxm = proxy->getServer()->proxyManager();
    if (pxm && pxm->GetInLoadXMLState())
    {
      return;
    }

    // check if this proxy has been assigned a frame already. This typically can
    // happen when loading states (for collaboration or otherwise).
    QList<QPointer<pqMultiViewWidget> > widgets = this->Internals->TabWidgets.values();

    foreach (pqMultiViewWidget* widget, widgets)
    {
      if (widget && widget->isViewAssigned(view))
      {
        return;
      }
    }

    if (!(proxy->getProxy()->HasAnnotation("ParaView::DetachedFromLayout") &&
          strcmp(proxy->getProxy()->GetAnnotation("ParaView::DetachedFromLayout"), "true") == 0))
    {
      // FIXME: we may want to give server-manager the opportunity to place the
      // view after creation, if it wants. The GUI should try to find a place for
      // it, only if the server-manager (through undo-redo, or loading state or
      // Python or collaborative-client).
      this->assignToFrame(view, true);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::proxyRemoved(pqProxy* proxy)
{
  if (proxy->getSMGroup() == "layouts" && proxy->getProxy()->IsA("vtkSMViewLayoutProxy"))
  {
    vtkSMProxy* smproxy = proxy->getProxy();

    QList<QPointer<pqMultiViewWidget> > widgets = this->Internals->TabWidgets.values();
    foreach (QPointer<pqMultiViewWidget> widget, widgets)
    {
      if (widget && widget->layoutManager() == smproxy)
      {
        this->Internals->TabWidgets.remove(proxy->getServer(), widget);
        int index = this->Internals->TabWidget->indexOf(widget);
        if (this->Internals->TabWidget->currentWidget() == widget)
        {
          this->Internals->TabWidget->setCurrentIndex(((index - 1) > 0) ? (index - 1) : 0);
        }
        this->Internals->TabWidget->removeTab(index);
        delete widget;
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::assignToFrame(pqView* view, bool warnIfTabCreated)
{
  pqMultiViewWidget* frame = this->Internals->assignableFrame(view);

  if (!frame)
  {
    if (warnIfTabCreated)
    {
      qWarning() << "This code may not work in multi-clients mode";
    }

    // implies no vtkSMViewLayoutProxy was registered for this session.
    this->createTab(view->getServer());
    frame = qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->currentWidget());
  }

  if (frame)
  {
    frame->assignToFrame(view);
  }
  else
  {
    qCritical() << "A new view was added, but pqTabbedMultiViewWidget has no "
                   "idea where to put this view.";
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::aboutToCreateView(pqServer* server)
{
  if (!this->Internals->TabWidgets.contains(server))
  {
    this->createTab(server);
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::serverRemoved(pqServer* server)
{
  // remove all tabs corresponding to the closed session.
  QList<QPointer<pqMultiViewWidget> > widgets = this->Internals->TabWidgets.values(server);
  foreach (pqMultiViewWidget* widget, widgets)
  {
    int cur_index = this->Internals->TabWidget->indexOf(widget);
    if (cur_index != -1)
    {
      this->Internals->TabWidget->removeTab(cur_index);
    }
    delete widget;
  }

  this->Internals->TabWidgets.remove(server);
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::currentTabChanged(int /* index*/)
{
  QWidget* currentWidget = this->Internals->TabWidget->currentWidget();
  if (pqMultiViewWidget* frame = qobject_cast<pqMultiViewWidget*>(currentWidget))
  {
    frame->makeFrameActive();
  }
  else if (currentWidget == this->Internals->NewTabWidget &&
    this->Internals->TabWidget->count() > 1)
  {
    // count() > 1 check keeps this widget from creating new tabs as the tabs are
    // being removed.
    this->createTab();
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::closeTab(int index)
{
  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->widget(index));
  vtkSMProxy* vlayout = widget ? widget->layoutManager() : NULL;
  if (vlayout)
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

    BEGIN_UNDO_SET("Remove View Tab");
    // first remove each of the views in the tab layout.
    widget->destroyAllViews();

    SM_SCOPED_TRACE(CallFunction).arg("RemoveLayout").arg(vlayout);

    builder->destroy(smmodel->findItem<pqProxy*>(vlayout));
    END_UNDO_SET();
  }

  if (this->Internals->TabWidget->count() == 1)
  {
    this->createTab();
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::createTab()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
  {
    this->createTab(server);
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::createTab(pqServer* server)
{
  if (server)
  {
    BEGIN_UNDO_SET("Add View Tab");
    vtkSMProxy* vlayout = pqApplicationCore::instance()->getObjectBuilder()->createProxy(
      "misc", "ViewLayout", server, "layouts");
    Q_ASSERT(vlayout != NULL);
    (void)vlayout;
    END_UNDO_SET();
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::createTab(vtkSMViewLayoutProxy* vlayout)
{
  pqMultiViewWidget* widget = new pqMultiViewWidget(this);
  QObject::connect(widget, SIGNAL(frameActivated()), this, SLOT(frameActivated()));

  int count = this->Internals->TabWidget->count();
  widget->setObjectName(QString("MultiViewWidget%1").arg(count));
  widget->setLayoutManager(vlayout);

  int tab_index = this->Internals->TabWidget->addAsTab(widget, this);
  this->Internals->TabWidget->setCurrentIndex(tab_index);
  pqServer* server =
    pqApplicationCore::instance()->getServerManagerModel()->findServer(vlayout->GetSession());
  this->Internals->TabWidgets.insert(server, widget);
}

//-----------------------------------------------------------------------------
bool pqTabbedMultiViewWidget::eventFilter(QObject* obj, QEvent* evt)
{
  // filtering events on the QLabel added as the tabButton to the tabbar to
  // close the tabs. If clicked, we close the tab.
  if (evt->type() == QEvent::MouseButtonRelease && qobject_cast<QLabel*>(obj))
  {
    QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(evt);
    if (mouseEvent->button() == Qt::LeftButton)
    {
      int index =
        this->Internals->TabWidget->tabButtonIndex(qobject_cast<QWidget*>(obj), QTabBar::RightSide);
      if (index != -1)
      {
        BEGIN_UNDO_SET("Close Tab");
        this->closeTab(index);
        END_UNDO_SET();
        return true;
      }

      // user clicked on the popout label. We pop the frame out (or back in).
      index =
        this->Internals->TabWidget->tabButtonIndex(qobject_cast<QWidget*>(obj), QTabBar::LeftSide);
      if (index != -1)
      {
        // Pop out tab in a separate window.
        pqMultiViewWidget* tabPage =
          qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->widget(index));
        if (tabPage)
        {
          QLabel* label = qobject_cast<QLabel*>(obj);
          bool popped_out = tabPage->togglePopout();
          label->setPixmap(
            this->style()->standardPixmap(pqTabWidget::popoutLabelPixmap(popped_out)));
          label->setToolTip(pqTabWidget::popoutLabelText(popped_out));
          label->setStatusTip(pqTabWidget::popoutLabelText(popped_out));
        }
        return true;
      }
    }
  }

  return this->Superclass::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::toggleWidgetDecoration()
{
  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->currentWidget());
  if (widget)
  {
    widget->setDecorationsVisible(!widget->isDecorationsVisible());
  }
}

//-----------------------------------------------------------------------------
QSize pqTabbedMultiViewWidget::clientSize() const
{
  if (this->Internals->TabWidget->currentWidget())
  {
    return this->Internals->TabWidget->currentWidget()->size();
  }

  return this->size();
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::lockViewSize(const QSize& viewSize)
{
  QList<QPointer<pqMultiViewWidget> > widgets = this->Internals->TabWidgets.values();
  foreach (QPointer<pqMultiViewWidget> widget, widgets)
  {
    if (widget)
    {
      widget->lockViewSize(viewSize);
    }
  }

  emit this->viewSizeLocked(!viewSize.isEmpty());
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::reset()
{
  for (int cc = this->Internals->TabWidget->count() - 1; cc > 1; --cc)
  {
    this->closeTab(cc - 1);
  }

  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->currentWidget());
  if (widget)
  {
    widget->reset();
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::frameActivated()
{
  pqMultiViewWidget* widget = qobject_cast<pqMultiViewWidget*>(this->sender());
  if (widget)
  {
    this->Internals->TabWidget->setCurrentWidget(widget);
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::onStateLoaded()
{
  QSet<vtkSMViewProxy*> proxies;
  foreach (pqMultiViewWidget* wdg, this->Internals->TabWidgets.values())
  {
    if (wdg)
    {
      proxies.unite(wdg->viewProxies().toSet());
    }
  }

  // check that all views are assigned to some frame or other.
  QList<pqView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqView*>();

  foreach (pqView* view, views)
  {
    if (!proxies.contains(view->getViewProxy()))
    {
      this->assignToFrame(view, false);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::contextMenuRequested(const QPoint& point)
{
  this->setFocus(Qt::OtherFocusReason);

  int tabIndex = this->Internals->TabWidget->tabBar()->tabAt(point);
  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->widget(tabIndex));
  vtkSMProxy* vlayout = widget ? widget->layoutManager() : NULL;
  if (!vlayout)
  {
    return;
  }
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* proxy = smmodel->findItem<pqProxy*>(vlayout);

  QMenu* menu = new QMenu(this);
  QAction* renameAction = menu->addAction("Rename");
  QAction* closeAction = menu->addAction(tr("Close layout"));
  QAction* action = menu->exec(this->Internals->TabWidget->tabBar()->mapToGlobal(point));
  if (action == closeAction)
  {
    BEGIN_UNDO_SET("Close Tab");
    this->closeTab(tabIndex);
    END_UNDO_SET();
  }
  else if (action == renameAction)
  {
    bool ok;
    QString oldName = proxy->getSMName();
    QString newName = QInputDialog::getText(
      this, tr("Rename Layout..."), tr("New name:"), QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName)
    {
      SM_SCOPED_TRACE(CallFunction)
        .arg("RenameLayout")
        .arg(newName.toLocal8Bit().data())
        .arg((vtkObject*)proxy->getProxy());

      proxy->rename(newName);
    }
  }
  delete menu;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::onLayoutNameChanged(pqServerManagerModelItem* item)
{
  pqProxy* proxy = dynamic_cast<pqProxy*>(item);
  for (int i = 0; i < this->Internals->TabWidget->count(); i++)
  {
    pqMultiViewWidget* wdg =
      dynamic_cast<pqMultiViewWidget*>(this->Internals->TabWidget->widget(i));
    if (wdg && wdg->layoutManager() == proxy->getProxy())
    {
      this->Internals->TabWidget->setTabText(i, proxy->getSMName());
      return;
    }
  }
}

//=================================================================================
// LEGACY METHODS
//=================================================================================
#if !defined(VTK_LEGACY_REMOVE)
//-----------------------------------------------------------------------------
vtkImageData* pqTabbedMultiViewWidget::captureImage(int dx, int dy)
{
  VTK_LEGACY_BODY(pqTabbedMultiViewWidget::captureImage, "ParaView 5.4");

  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->currentWidget());
  if (widget)
  {
    vtkSmartPointer<vtkImageData> img =
      vtkSMSaveScreenshotProxy::CaptureImage(widget->layoutManager(), vtkVector2i(dx, dy));
    if (img)
    {
      img->Register(nullptr);
      return img.GetPointer();
    }
    return nullptr;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
int pqTabbedMultiViewWidget::prepareForCapture(int, int)
{
  VTK_LEGACY_BODY(pqTabbedMultiViewWidget::prepareForCapture, "ParaView 5.4");
  return 0;
}

//-----------------------------------------------------------------------------
void pqTabbedMultiViewWidget::cleanupAfterCapture()
{
  VTK_LEGACY_BODY(pqTabbedMultiViewWidget::cleanupAfterCapture, "ParaView 5.4");
}

//-----------------------------------------------------------------------------
bool pqTabbedMultiViewWidget::writeImage(const QString& filename, int dx, int dy, int quality)
{
  VTK_LEGACY_BODY(pqTabbedMultiViewWidget::writeImage, "ParaView 5.4");

  pqMultiViewWidget* widget =
    qobject_cast<pqMultiViewWidget*>(this->Internals->TabWidget->currentWidget());
  if (widget)
  {
    vtkSmartPointer<vtkImageData> img =
      vtkSMSaveScreenshotProxy::CaptureImage(widget->layoutManager(), vtkVector2i(dx, dy));
    if (img)
    {
      return vtkSMUtilities::SaveImage(img.GetPointer(), filename.toLocal8Bit().data(), quality) ==
        vtkErrorCode::NoError;
    }
  }

  return true;
}

#endif // !defined(VTK_LEGACY_REMOVE)
//=================================================================================
