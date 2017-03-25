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
#include "pqMultiViewWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqViewFrame.h"
#include "pqViewFrameActionsInterface.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QMap>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>
#include <QVariant>
#include <QVector>

class pqMultiViewWidget::pqInternals
{
public:
  QVector<QPointer<QWidget> > Widgets;

  // This map is used to avoid reassigning frames. Once a view is assigned a
  // frame, we preserve that frame as long as possible.
  QMap<vtkSMViewProxy*, QPointer<pqViewFrame> > ViewFrames;

  unsigned long ObserverId;
  vtkWeakPointer<vtkSMViewLayoutProxy> LayoutManager;
  QPointer<pqViewFrame> ActiveFrame;

  // Set to true to place views in a separate popout widget.
  bool Popout;
  QWidget PopoutFrame;

  pqPropertyLinks Links;

  pqInternals(QWidget* self)
    : ObserverId(0)
    , Popout(false)
    , PopoutFrame(self)
    , SavedButtons(pqViewFrame::NoButton)
  {
    this->PopoutFrame.setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
      Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
  }

  ~pqInternals()
  {
    if (this->LayoutManager && this->ObserverId)
    {
      this->LayoutManager->RemoveObserver(this->ObserverId);
    }
  }

  void setMaximizedWidget(QWidget* wdg)
  {
    pqViewFrame* frame = qobject_cast<pqViewFrame*>(wdg);
    if (frame)
    {
      this->SavedButtons = frame->standardButtons();
      frame->setStandardButtons(pqViewFrame::Restore);
    }
    if (this->MaximizedWidget)
    {
      this->MaximizedWidget->setStandardButtons(this->SavedButtons);
      this->SavedButtons = pqViewFrame::NoButton;
    }
    this->MaximizedWidget = frame;
  }

private:
  QPointer<pqViewFrame> MaximizedWidget;
  pqViewFrame::StandardButtons SavedButtons;
};

//-----------------------------------------------------------------------------
namespace
{
pqView* getPQView(vtkSMProxy* view)
{
  if (view)
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    return smmodel->findItem<pqView*>(view);
  }
  return NULL;
}

void ConnectFrameToView(pqViewFrame* frame, pqView* pqview)
{
  Q_ASSERT(frame);
  // if pqview == NULL, then the frame is either being assigned to a empty
  // view, or pqview for a view-proxy just isn't present yet.
  // it's possible that pqview is NULL, if the view proxy hasnt been registered
  // yet. This happens often when initialization state is being loaded in
  // collaborative sessions.
  if (pqview != NULL)
  {
    QWidget* viewWidget = pqview->widget();
    frame->setCentralWidget(viewWidget, pqview);
  }
}

/// A simple subclass of QBoxLayout mimicking the layout style of a QSplitter
/// with just 2 widgets.
class pqSplitterLayout : public QBoxLayout
{
  double SplitFraction;

public:
  pqSplitterLayout(QBoxLayout::Direction dir, QWidget* parentWdg)
    : QBoxLayout(dir, parentWdg)
    , SplitFraction(0.5)
  {
    this->setContentsMargins(0, 0, 0, 0);
    this->setSpacing(0);
  };

  virtual ~pqSplitterLayout() {}

  void setSplitFraction(double val) { this->SplitFraction = val; }

  virtual void setGeometry(const QRect& rect)
  {
    this->QLayout::setGeometry(rect);

    Q_ASSERT(this->count() <= 2);

    int offset = 0;
    double fractions[2] = { this->SplitFraction, 1.0 - this->SplitFraction };
    for (int cc = 0; cc < this->count(); cc++)
    {
      QLayoutItem* item = this->itemAt(cc);
      if (this->direction() == LeftToRight)
      {
        item->setGeometry(QRect(offset + rect.x(), rect.y(),
          static_cast<int>(fractions[cc] * rect.width()), rect.height()));
        offset += static_cast<int>(fractions[cc] * rect.width());
      }
      else if (this->direction() == TopToBottom)
      {
        item->setGeometry(QRect(rect.x(), offset + rect.y(), rect.width(),
          static_cast<int>(fractions[cc] * rect.height())));
        offset += static_cast<int>(fractions[cc] * rect.height());
      }
    }
  }
};
}

//-----------------------------------------------------------------------------
pqMultiViewWidget::pqMultiViewWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals(this))
  , DecorationsVisible(true)
{
  qApp->installEventFilter(this);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(markActive(pqView*)));

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)), this, SLOT(proxyRemoved(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)), this, SLOT(viewAdded(pqView*)));
}

//-----------------------------------------------------------------------------
pqMultiViewWidget::~pqMultiViewWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
bool pqMultiViewWidget::isViewAssigned(pqView* view) const
{
  return (view && this->Internals->LayoutManager &&
    this->Internals->LayoutManager->GetViewLocation(view->getViewProxy()) != -1);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::viewAdded(pqView* view)
{
  if (view && this->Internals->LayoutManager &&
    this->Internals->LayoutManager->GetViewLocation(view->getViewProxy()) != -1)
  {
    // a pqview was created for a view that this layout knows about. we have to
    // reload the layout to ensure that the view gets placed correctly.
    pqViewFrame* frame = this->Internals->ViewFrames[view->getViewProxy()];
    if (frame)
    {
      ConnectFrameToView(frame, view);
    }
    else
    {
      this->reload();
    }
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::setLayoutManager(vtkSMViewLayoutProxy* vlayout)
{
  pqInternals& internals = (*this->Internals);
  if (internals.LayoutManager != vlayout)
  {
    if (internals.LayoutManager)
    {
      internals.LayoutManager->RemoveObserver(this->Internals->ObserverId);
    }
    internals.Links.clear();
    internals.ObserverId = 0;
    if (vlayout)
    {
      internals.ObserverId =
        vlayout->AddObserver(vtkCommand::ConfigureEvent, this, &pqMultiViewWidget::reload);
      internals.Links.addPropertyLink(this, "decorationsVisibility",
        SIGNAL(decorationsVisibilityChanged(bool)), vlayout,
        vlayout->GetProperty("ShowWindowDecorations"));
    }
    // we delay the setting of the LayoutManager to avoid the duplicate `reload`
    // call when `addPropertyLink` is called if the window decorations
    // visibility changed.
    internals.LayoutManager = vlayout;
    this->reload();
  }
}

//-----------------------------------------------------------------------------
vtkSMViewLayoutProxy* pqMultiViewWidget::layoutManager() const
{
  return this->Internals->LayoutManager;
}

//-----------------------------------------------------------------------------
bool pqMultiViewWidget::eventFilter(QObject* caller, QEvent* evt)
{
  if (evt->type() == QEvent::MouseButtonPress)
  {
    QWidget* wdg = qobject_cast<QWidget*>(caller);
    if (wdg && ((!this->Internals->Popout && this->isAncestorOf(wdg)) ||
                 (this->Internals->Popout && this->Internals->PopoutFrame.isAncestorOf(wdg))))
    {
      // If the new widget that is getting the focus is a child widget of any of the
      // frames, then the frame should be made active.
      foreach (QPointer<QWidget> frame_or_splitter, this->Internals->Widgets)
      {
        pqViewFrame* frame = qobject_cast<pqViewFrame*>(frame_or_splitter.data());
        if (frame && frame->isAncestorOf(wdg))
        {
          this->makeActive(frame);
        }
      }
    }
  }
  else if (evt->type() == QEvent::Close && caller == &this->Internals->PopoutFrame)
  {
    // the popout-frame is being closed. We interpret that as "un-popping" the
    // frame.
    this->togglePopout();
  }

  return this->Superclass::eventFilter(caller, evt);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::proxyRemoved(pqProxy* proxy)
{
  vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(proxy->getProxy());
  if (view && this->Internals->ViewFrames.contains(view) && this->layoutManager())
  {
    this->layoutManager()->RemoveView(view);
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::assignToFrame(pqView* view)
{
  if (this->layoutManager() && view)
  {
    int active_index = 0;
    if (this->Internals->ActiveFrame)
    {
      active_index = this->Internals->ActiveFrame->property("FRAME_INDEX").toInt();
    }
    this->layoutManager()->AssignViewToAnyCell(view->getViewProxy(), active_index);
  }
  pqActiveObjects::instance().setActiveView(view);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeFrameActive()
{
  /// note pqMultiViewWidget::markActive(pqViewFrame*) fires a signal that
  /// results in this method being called. So we need to ensure that we don't
  /// do anything silly here that could cause infinite recursion.
  if (!this->Internals->ActiveFrame)
  {
    foreach (QWidget* wdg, this->Internals->Widgets)
    {
      pqViewFrame* frame = qobject_cast<pqViewFrame*>(wdg);
      if (frame)
      {
        this->makeActive(frame);
        break;
      }
    }
  }

  if (this->layoutManager())
  {
    this->layoutManager()->ShowViewsOnTileDisplay();
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::markActive(pqView* view)
{
  if (view && this->Internals->ViewFrames.contains(view->getViewProxy()))
  {
    this->markActive(this->Internals->ViewFrames[view->getViewProxy()]);
  }
  else
  {
    this->markActive(static_cast<pqViewFrame*>(NULL));
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::markActive(pqViewFrame* frame)
{
  if (this->Internals->ActiveFrame)
  {
    this->Internals->ActiveFrame->setBorderVisibility(false);
  }
  this->Internals->ActiveFrame = frame;
  if (frame)
  {
    frame->setBorderVisibility(true);
    // indicate to the world that a frame on this widget has been activated.
    // pqTabbedMultiViewWidget listens to this signal to raise that tab.
    emit this->frameActivated();
    // NOTE: this signal will result in call to makeFrameActive().
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeActive(pqViewFrame* frame)
{
  if (this->Internals->ActiveFrame != frame)
  {
    pqView* view = NULL;
    if (frame)
    {
      int index = frame->property("FRAME_INDEX").toInt();
      view = getPQView(this->layoutManager()->GetView(index));
    }
    pqActiveObjects::instance().setActiveView(view);
    // this needs to called only when view == null since in that case when
    // markActive(pqView*) slot is called, we have no idea what frame is really
    // to be made active.
    this->markActive(frame);
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::lockViewSize(const QSize& viewSize)
{
  if (this->LockViewSize != viewSize)
  {
    this->LockViewSize = viewSize;
    this->reload();
  }
}

//-----------------------------------------------------------------------------
pqViewFrame* pqMultiViewWidget::newFrame(vtkSMProxy* view)
{
  pqViewFrame* frame = new pqViewFrame(this);
  QObject::connect(frame, SIGNAL(buttonPressed(int)), this, SLOT(standardButtonPressed(int)));
  QObject::connect(frame, SIGNAL(swapPositions(const QString&)), this,
    SLOT(swapPositions(const QString&)), Qt::QueuedConnection);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smmodel->findItem<pqView*>(view);
  // it's possible that pqview is NULL, if the view proxy hasnt been registered
  // yet. This happens often when initialization state is being loaded in
  // collaborative sessions.
  ConnectFrameToView(frame, pqview);

  // Search for view frame actions plugins and allow them to decide
  // whether to add their actions to this view type's frame or not.
  pqInterfaceTracker* tracker = pqApplicationCore::instance()->interfaceTracker();
  foreach (pqViewFrameActionsInterface* vfai, tracker->interfaces<pqViewFrameActionsInterface*>())
  {
    vfai->frameConnected(frame, pqview);
  }
  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiViewWidget::createWidget(
  int index, vtkSMViewLayoutProxy* vlayout, QWidget* parentWdg, int& max_index)
{
  if (this->Internals->Widgets.size() <= static_cast<int>(index))
  {
    this->Internals->Widgets.resize(index + 1);
  }

  vtkSMViewLayoutProxy::Direction direction = vlayout->GetSplitDirection(index);
  switch (direction)
  {
    case vtkSMViewLayoutProxy::NONE:
    {
      vtkSMViewProxy* view = vlayout->GetView(index);
      pqViewFrame* frame = view ? this->Internals->ViewFrames[view] : NULL;
      if (!frame)
      {
        frame = this->newFrame(view);
        if (view)
        {
          this->Internals->ViewFrames[view] = frame;
        }
      }
      Q_ASSERT(frame != NULL);

      frame->setParent(parentWdg);
      this->Internals->Widgets[index] = frame;
      frame->setObjectName(QString("Frame.%1").arg(index));
      frame->setProperty("FRAME_INDEX", QVariant(index));
      frame->setDecorationsVisibility(this->DecorationsVisible);
      QWidget* centralWidget = frame->centralWidget();
      if (centralWidget)
      {
        if (this->LockViewSize.isEmpty())
        {
          centralWidget->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        }
        else
        {
          centralWidget->setMaximumSize(this->LockViewSize);
        }
      }
      if (max_index < index)
      {
        max_index = index;
      }
      return frame;
    }

    case vtkSMViewLayoutProxy::VERTICAL:
    case vtkSMViewLayoutProxy::HORIZONTAL:
      if (this->DecorationsVisible)
      {
        QSplitter* splitter = qobject_cast<QSplitter*>(this->Internals->Widgets[index]);
        if (!splitter)
        {
          splitter = new QSplitter(parentWdg);
        }
        Q_ASSERT(splitter);

        this->Internals->Widgets[index] = splitter;
        splitter->setParent(parentWdg);
        splitter->setHandleWidth(this->DecorationsVisible ? 3 : 1);
        splitter->setObjectName(QString("Splitter.%1").arg(index));
        splitter->setProperty("FRAME_INDEX", QVariant(index));
        splitter->setOpaqueResize(false);
        splitter->setOrientation(
          direction == vtkSMViewLayoutProxy::VERTICAL ? Qt::Vertical : Qt::Horizontal);
        splitter->insertWidget(
          0, this->createWidget(vlayout->GetFirstChild(index), vlayout, splitter, max_index));
        splitter->insertWidget(
          1, this->createWidget(vlayout->GetSecondChild(index), vlayout, splitter, max_index));

        // set the sizes are percentage. QSplitter uses the initially specified
        // sizes as reference.
        QList<int> sizes;
        sizes << static_cast<int>(vlayout->GetSplitFraction(index) * 10000);
        sizes << static_cast<int>((1.0 - vlayout->GetSplitFraction(index)) * 10000);
        splitter->setSizes(sizes);

        // FIXME: Don't like this as this QueuedConnection may cause multiple
        // renders.
        QObject::disconnect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved()));
        QObject::connect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved()),
          Qt::QueuedConnection);
        if (max_index < index)
        {
          max_index = index;
        }
        return splitter;
      }
      else
      {
        QWidget* container = this->Internals->Widgets[index];
        // since old widget may be a splitter, which is a QWidget too, we
        // compare with className rather than using a `qobject_cast`.
        if (strcmp(container->metaObject()->className(), "QWidget") != 0)
        {
          container = new QWidget(parentWdg);
          new pqSplitterLayout(direction == vtkSMViewLayoutProxy::VERTICAL
              ? pqSplitterLayout::TopToBottom
              : pqSplitterLayout::LeftToRight,
            container);
        }
        Q_ASSERT(container);
        container->setObjectName(QString("Container.%1").arg(index));
        this->Internals->Widgets[index] = container;

        pqSplitterLayout* slayout = dynamic_cast<pqSplitterLayout*>(container->layout());
        Q_ASSERT(slayout);
        slayout->setDirection(direction == vtkSMViewLayoutProxy::VERTICAL
            ? pqSplitterLayout::TopToBottom
            : pqSplitterLayout::LeftToRight);
        slayout->setSplitFraction(vlayout->GetSplitFraction(index));
        slayout->addWidget(
          this->createWidget(vlayout->GetFirstChild(index), vlayout, container, max_index));
        slayout->addWidget(
          this->createWidget(vlayout->GetSecondChild(index), vlayout, container, max_index));
        if (max_index < index)
        {
          max_index = index;
        }
        return container;
      }
      break;
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::reload()
{
  vtkSMViewLayoutProxy* vlayout = this->layoutManager();
  if (!vlayout)
  {
    return;
  }

  this->setUpdatesEnabled(false);

  QList<QPointer<QWidget> > oldWidgets;
  foreach (QWidget* widget, this->Internals->Widgets)
  {
    if (widget)
    {
      oldWidgets.push_back(widget);
    }
  }

  // if popout is true, we add the widgets to the PopoutFrame, rather that
  // ourselves.
  QWidget* parentWdg = this->Internals->Popout ? &this->Internals->PopoutFrame : this;

  int max_index = 0;
  QWidget* child = this->createWidget(0, vlayout, parentWdg, max_index);

  // resize Widgets to remove any obsolete indices. These indices weren't
  // touched at all during the last call to createWidget().
  this->Internals->Widgets.resize(max_index + 1);

  // now destroy any old widgets no longer being used.
  QSet<QWidget*> newWidgets;
  foreach (QWidget* widget, this->Internals->Widgets)
  {
    if (widget)
    {
      newWidgets.insert(widget);
    }
  }
  foreach (QWidget* aWidget, oldWidgets)
  {
    if (aWidget && !newWidgets.contains(aWidget))
    {
      aWidget->setParent(NULL);
      delete aWidget;
    }
  }
  oldWidgets.clear();
  newWidgets.clear();

  delete parentWdg->layout();
  QVBoxLayout* vbox = new QVBoxLayout(parentWdg);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(child);

  if (this->Internals->Popout)
  {
    // if the PopoutFrame is being shown for the first time, we resize it to
    // match this widgets size so that it doesn't end up too small.
    if (!this->Internals->PopoutFrame.property("pqMultiViewWidget::SizeInitialized").isValid())
    {
      this->Internals->PopoutFrame.setProperty("pqMultiViewWidget::SizeInitialized", true);
      this->Internals->PopoutFrame.resize(this->size());
    }
    this->Internals->PopoutFrame.show();

    QString layoutName = vlayout->GetSessionProxyManager()->GetProxyName("layouts", vlayout);
    this->Internals->PopoutFrame.setWindowTitle(layoutName);
  }
  else
  {
    this->Internals->PopoutFrame.hide();
  }

  int maximized_cell = vlayout->GetMaximizedCell();
  this->Internals->setMaximizedWidget(NULL);
  for (int cc = 0; cc < this->Internals->Widgets.size(); cc++)
  {
    pqViewFrame* frame = qobject_cast<pqViewFrame*>(this->Internals->Widgets[cc]);
    if (frame)
    {
      bool visibility = true;
      if (cc == maximized_cell)
      {
        this->Internals->setMaximizedWidget(frame);
      }
      else if (maximized_cell != -1)
      {
        visibility = false;
      }
      frame->setVisible(visibility);
    }
  }

  // ensure the active view is marked appropriately.
  this->markActive(pqActiveObjects::instance().activeView());

  // Cleanup deleted objects. "cleaner" helps us avoid any dangling widgets and
  // deletes them whwn delete cleaner is called. Now we prune internal
  // datastructures to get rid of these NULL ptrs.

  // remove any deleted view frames.
  QMutableMapIterator<vtkSMViewProxy*, QPointer<pqViewFrame> > iter(this->Internals->ViewFrames);
  while (iter.hasNext())
  {
    iter.next();
    if (iter.value() == NULL)
    {
      // since we are in the process of destroying the view, cancel any pending
      // render requests. This addresses a Windows issue where the view would
      // occassionally popout and render when undoing the creation of the view
      // or closing it.
      pqView* view = getPQView(iter.key());
      if (view)
      {
        view->cancelPendingRenders();
      }
      iter.remove();
    }
  }

  this->setUpdatesEnabled(true);

  // we let the GUI updated immediately. This is needed since when a new view is
  // created (for example), it may depend on the size of the view during its
  // initialization to ensure camera is reset correctly.
  QCoreApplication::sendPostedEvents();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::standardButtonPressed(int button)
{
  pqViewFrame* frame = qobject_cast<pqViewFrame*>(this->sender());
  QVariant index = frame ? frame->property("FRAME_INDEX") : QVariant();
  if (!index.isValid() || this->layoutManager() == NULL)
  {
    return;
  }

  switch (button)
  {
    case pqViewFrame::SplitVertical:
    case pqViewFrame::SplitHorizontal:
    {
      BEGIN_UNDO_SET("Split View");
      int new_index = this->layoutManager()->Split(index.toInt(),
        (button == pqViewFrame::SplitVertical ? vtkSMViewLayoutProxy::VERTICAL
                                              : vtkSMViewLayoutProxy::HORIZONTAL),
        0.5);
      this->makeActive(qobject_cast<pqViewFrame*>(this->Internals->Widgets[new_index + 1]));
      END_UNDO_SET();
    }
    break;

    case pqViewFrame::Maximize:
      // ensure that the frame being maximized is active.
      this->makeActive(frame);
      this->layoutManager()->MaximizeCell(index.toInt());
      break;

    case pqViewFrame::Restore:
      // ensure that the frame being minimized is active.
      this->makeActive(frame);
      this->layoutManager()->RestoreMaximizedState();
      break;

    case pqViewFrame::Close:
    {
      BEGIN_UNDO_SET("Close View");
      vtkSMViewProxy* viewProxy = this->layoutManager()->GetView(index.toInt());
      if (viewProxy)
      {
        this->layoutManager()->RemoveView(viewProxy);
        pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
        builder->destroy(getPQView(viewProxy));
      }
      if (index.toInt() != 0)
      {
        int location = index.toInt();
        int parent_idx = vtkSMViewLayoutProxy::GetParent(location);
        this->layoutManager()->Collapse(location);
        QWidget* widgetToActivate = this->Internals->Widgets[parent_idx];
        pqViewFrame* frameToActivate = qobject_cast<pqViewFrame*>(widgetToActivate);
        if (!frameToActivate)
        {
          QSplitter* splitter = qobject_cast<QSplitter*>(widgetToActivate);
          frameToActivate = splitter && splitter->count() > 0
            ? qobject_cast<pqViewFrame*>(splitter->widget(0))
            : NULL;
        }
        this->makeActive(frameToActivate);
      }
      END_UNDO_SET();
    }
    break;
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::destroyAllViews()
{
  BEGIN_UNDO_SET("Destroy all views");
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  QList<vtkSMViewProxy*> views = this->viewProxies();
  foreach (vtkSMViewProxy* view, views)
  {
    if (view)
    {
      builder->destroy(getPQView(view));
    }
  }
  END_UNDO_SET();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitterMoved()
{
  QSplitter* splitter = qobject_cast<QSplitter*>(this->sender());
  QVariant index = splitter ? splitter->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
  {
    QList<int> sizes = splitter->sizes();
    if (sizes.size() == 2)
    {
      BEGIN_UNDO_SET("Resize Frame");
      double fraction = sizes[0] * 1.0 / (sizes[0] + sizes[1]);
      this->layoutManager()->SetSplitFraction(index.toInt(), fraction);
      END_UNDO_SET();
    }
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::setDecorationsVisible(bool val)
{
  if (this->DecorationsVisible == val)
  {
    return;
  }

  this->DecorationsVisible = val;
  this->reload();
  emit this->decorationsVisibilityChanged(val);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::swapPositions(const QString& uid_str)
{
  QUuid other(uid_str);

  vtkSMViewLayoutProxy* vlayout = this->layoutManager();
  pqViewFrame* senderFrame = qobject_cast<pqViewFrame*>(this->sender());
  if (!senderFrame || !vlayout)
  {
    return;
  }

  pqViewFrame* swapWith = NULL;
  foreach (QWidget* wdg, this->Internals->Widgets)
  {
    pqViewFrame* frame = qobject_cast<pqViewFrame*>(wdg);
    if (frame && frame->uniqueID() == other)
    {
      swapWith = frame;
      break;
    }
  }

  if (!swapWith)
  {
    return;
  }

  int id1 = senderFrame->property("FRAME_INDEX").toInt();
  int id2 = swapWith->property("FRAME_INDEX").toInt();
  vtkSMViewProxy* view1 = vlayout->GetView(id1);
  vtkSMViewProxy* view2 = vlayout->GetView(id2);

  if (view1 == NULL && view2 == NULL)
  {
    return;
  }

  BEGIN_UNDO_SET("Swap Views");
  vlayout->SwapCells(id1, id2);
  END_UNDO_SET();
  this->reload();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::reset()
{
  this->layoutManager()->Reset();
}

//-----------------------------------------------------------------------------
QList<vtkSMViewProxy*> pqMultiViewWidget::viewProxies() const
{
  return this->Internals->ViewFrames.keys();
}

//-----------------------------------------------------------------------------
bool pqMultiViewWidget::togglePopout()
{
  this->Internals->Popout = this->Internals->Popout ? false : true;
  this->reload();
  return this->Internals->Popout;
}

//=================================================================================
// LEGACY METHODS
//=================================================================================
#if !defined(VTK_LEGACY_REMOVE)
vtkImageData* pqMultiViewWidget::captureImage(int width, int height)
{
  VTK_LEGACY_BODY(pqMultiViewWidget::captureImage, "ParaView 5.4");
  vtkSmartPointer<vtkImageData> img =
    vtkSMSaveScreenshotProxy::CaptureImage(this->layoutManager(), vtkVector2i(width, height));
  if (img)
  {
    img->Register(nullptr);
    return img.GetPointer();
  }
  return nullptr;
}

int pqMultiViewWidget::prepareForCapture(int, int)
{
  VTK_LEGACY_BODY(pqMultiViewWidget::prepareForCapture, "ParaView 5.4");
  return 0;
}

void pqMultiViewWidget::cleanupAfterCapture()
{
  VTK_LEGACY_BODY(pqMultiViewWidget::cleanupAfterCapture, "ParaView 5.4");
}

bool pqMultiViewWidget::writeImage(
  const QString& filename, int width, int height, int quality /*=-1*/)
{
  VTK_LEGACY_BODY(pqMultiViewWidget::writeImage, "ParaView 5.4");
  vtkSmartPointer<vtkImageData> img =
    vtkSMSaveScreenshotProxy::CaptureImage(this->layoutManager(), vtkVector2i(width, height));
  if (img)
  {
    return vtkSMUtilities::SaveImage(img.GetPointer(), filename.toLocal8Bit().data(), quality) ==
      vtkErrorCode::NoError;
  }
  return false;
}

#endif // !defined(VTK_LEGACY_REMOVE)
//=================================================================================
