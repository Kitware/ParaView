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
#include "ui_pqPopoutPlaceholder.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqHierarchicalGridLayout.h"
#include "pqHierarchicalGridWidget.h"
#include "pqInterfaceTracker.h"
#include "pqObjectBuilder.h"
#include "pqPropertyLinks.h"
#include "pqQVTKWidget.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "pqViewFrame.h"
#include "pqViewFrameActionsInterface.h"
#include "vtkCommand.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkWeakPointer.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QVector>
#include <cassert>
#include <functional>

namespace
{
static const int PARAVIEW_DEFAULT_LAYOUT_SPACING = 4;
}

class pqMultiViewWidget::pqInternals
{
public:
  QVector<QPointer<pqViewFrame> > Frames;

  // This map is used to avoid reassigning frames. Once a view is assigned a
  // frame, we preserve that frame as long as possible.
  QMap<vtkSMViewProxy*, QPointer<pqViewFrame> > ViewFrames;

  // This is a collection for empty frames.
  QVector<QPointer<pqViewFrame> > EmptyFrames;

  std::vector<unsigned long> ObserverIds;
  vtkWeakPointer<vtkSMViewLayoutProxy> LayoutManager;
  QPointer<pqViewFrame> ActiveFrame;

  // Set to true to place views in a separate popout widget.
  bool Popout;
  QScopedPointer<QWidget> PopoutWindow;
  QPointer<pqHierarchicalGridWidget> Container;

  QScopedPointer<QWidget> PopoutPlaceholder;

  pqPropertyLinks Links;

  double CustomDevicePixelRatio = 0.0;

  pqInternals(pqMultiViewWidget* self)
    : Popout(false)
    , SavedButtons(pqViewFrame::NoButton)
  {
    this->Container = new pqHierarchicalGridWidget(self);
    this->Container->setObjectName("Container");
    this->Container->setAutoFillBackground(true);
    auto hlayout = new pqHierarchicalGridLayout(this->Container);
    hlayout->setSpacing(PARAVIEW_DEFAULT_LAYOUT_SPACING);

    QObject::connect(this->Container.data(), &pqHierarchicalGridWidget::splitterMoved,
      [self](int location, double fraction) {
        if (auto vlayout = self->layoutManager())
        {
          BEGIN_UNDO_SET("Resize Frame");
          vlayout->SetSplitFraction(location, fraction);
          END_UNDO_SET();
        }
      });

    QVBoxLayout* slayout = new QVBoxLayout(self);
    slayout->setMargin(0);
    slayout->addWidget(this->Container);

    this->PopoutPlaceholder.reset(new QWidget());
    Ui::PopoutPlaceholder ui;
    ui.setupUi(this->PopoutPlaceholder.data());
    QObject::connect(
      ui.restoreButton, &QPushButton::clicked, [self](bool) { self->togglePopout(); });

    this->CustomDevicePixelRatio = 0.0;
  }

  ~pqInternals()
  {
    if (this->LayoutManager)
    {
      for (auto id : this->ObserverIds)
      {
        this->LayoutManager->RemoveObserver(id);
      }
    }
    delete this->Container;
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

  void createViewFrames(const std::vector<vtkSMViewProxy*>& viewProxies, pqMultiViewWidget* self)
  {
    for (auto proxy : viewProxies)
    {
      if (!this->ViewFrames.contains(proxy))
      {
        this->ViewFrames.insert(proxy, self->newFrame(proxy));
      }
    }
  }

  void setPreviewSpacing(int spacing)
  {
    this->PreviewSpacing = spacing;
    if (!this->PreviewSize.isEmpty())
    {
      this->Container->layout()->setSpacing(spacing);
    }
  }

  void setPreviewSeparatorColor(const QColor& color)
  {
    this->PreviewColor = color;
    if (!this->PreviewSize.isEmpty())
    {
      auto palette = this->Container->palette();
      palette.setColor(QPalette::Window, color);
      this->Container->setPalette(palette);
    }
  }

  void preview(const QSize& size)
  {
    vtkLogScopeF(TRACE, "preview (%d, %d)", size.width(), size.height());
    this->PreviewSize = size;
    if (size.isEmpty())
    {
      this->setCustomDevicePixelRatio(0.0); // set to 0 to not use custom value.
      this->Container->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
      this->Container->layout()->setSpacing(PARAVIEW_DEFAULT_LAYOUT_SPACING);

      auto palette = this->Container->parentWidget()->palette();
      this->Container->setPalette(palette);
    }
    else
    {
      this->Container->layout()->setSpacing(this->PreviewSpacing);
      auto palette = this->Container->palette();
      palette.setColor(QPalette::Window, this->PreviewColor);
      this->Container->setPalette(palette);

      // ensure that the max size we set on the dialog is less that what we have
      // available.
      vtkVector2i tsize(size.width(), size.height());
      const QRect crect = this->Container->parentWidget()->contentsRect();
      vtkVector2i csize(crect.width(), crect.height());
      const int magnification = vtkSMSaveScreenshotProxy::ComputeMagnification(tsize, csize);
      this->setCustomDevicePixelRatio(magnification);
      this->Container->setMaximumSize(csize[0], csize[1]);
      vtkLogF(
        TRACE, "cur=(%d, %d), new=(%d, %d)", crect.width(), crect.height(), csize[0], csize[1]);
    }
    this->updateDecorations();
  }

  const QSize& previewSize() const { return this->PreviewSize; }

  void setCustomDevicePixelRatio(double sf)
  {
    if (this->CustomDevicePixelRatio != sf)
    {
      this->CustomDevicePixelRatio = sf;
      for (auto renderWidget : this->Container->findChildren<pqQVTKWidget*>())
      {
        renderWidget->setCustomDevicePixelRatio(sf);
        // need to disable font-scaling if custom ratio is being used.
        renderWidget->setEnableHiDPI(sf == 0.0 ? true : false);
      }
    }
  }

  void setDecorationsVisibility(bool val)
  {
    this->DecorationsVisibility = val;
    this->updateDecorations();
  }

  bool decorationsVisibility() const { return this->DecorationsVisibility; }

  void updateDecorations()
  {
    // we show decorations if explicitly requested *and* we're not in preview mode.
    const bool showDecorations = this->DecorationsVisibility && this->PreviewSize.isEmpty();
    for (auto iter = this->ViewFrames.begin(); iter != this->ViewFrames.end(); ++iter)
    {
      if (iter.value() != nullptr)
      {
        iter.value()->setDecorationsVisibility(showDecorations);
      }
    }
  }

  void lockViewSize(const QSize& size)
  {
    this->LockedSize = size.isEmpty() ? QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX) : size;
    for (pqViewFrame* frame : this->Frames)
    {
      if (auto centralWidget = frame ? frame->centralWidget() : nullptr)
      {
        centralWidget->setMaximumSize(this->LockedSize);
      }
    }
  }
  const QSize& lockedSize() const { return this->LockedSize; }

private:
  QPointer<pqViewFrame> MaximizedWidget;
  pqViewFrame::StandardButtons SavedButtons;

  int PreviewSpacing = 1;
  QColor PreviewColor;
  QSize PreviewSize;
  bool DecorationsVisibility = true;

  QSize LockedSize;
};

//=============================================================================
namespace
{
pqView* getPQView(vtkSMProxy* view)
{
  if (view)
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    return smmodel->findItem<pqView*>(view);
  }
  return nullptr;
}

void ConnectFrameToView(pqViewFrame* frame, pqView* pqview)
{
  assert(frame);
  // if pqview == nullptr, then the frame is either being assigned to a empty
  // view, or pqview for a view-proxy just isn't present yet.
  // it's possible that pqview is nullptr, if the view proxy hasn't been registered
  // yet. This happens often when initialization state is being loaded in
  // collaborative sessions.
  if (pqview != nullptr)
  {
    QWidget* viewWidget = pqview->widget();
    frame->setCentralWidget(viewWidget, pqview);
  }
}
}

//=============================================================================
pqMultiViewWidget::pqMultiViewWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals(this))
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
  this->Internals = nullptr;
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
      for (auto id : this->Internals->ObserverIds)
      {
        internals.LayoutManager->RemoveObserver(id);
      }
    }
    internals.Links.clear();
    internals.ObserverIds.clear();
    if (vlayout)
    {
      internals.ObserverIds.push_back(
        vlayout->AddObserver(vtkCommand::ConfigureEvent, this, &pqMultiViewWidget::reload));
      internals.ObserverIds.push_back(vlayout->AddObserver(
        vtkCommand::PropertyModifiedEvent, this, &pqMultiViewWidget::layoutPropertyModified));

      // explicitly call `layoutPropertyModified` for all properties we care
      // about to ensure our state is initialized to the current values from the
      // layout proxy.
      this->layoutPropertyModified(
        vlayout, vtkCommand::PropertyModifiedEvent, const_cast<char*>("SeparatorWidth"));
      this->layoutPropertyModified(
        vlayout, vtkCommand::PropertyModifiedEvent, const_cast<char*>("SeparatorColor"));
      this->layoutPropertyModified(
        vlayout, vtkCommand::PropertyModifiedEvent, const_cast<char*>("PreviewMode"));
    }
  }

  // we delay the setting of the LayoutManager to avoid the duplicate `reload`
  // call when `addPropertyLink` is called if the window decorations
  // visibility changed.
  internals.LayoutManager = vlayout;
  this->reload();
}

//-----------------------------------------------------------------------------
vtkSMViewLayoutProxy* pqMultiViewWidget::layoutManager() const
{
  return this->Internals->LayoutManager;
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::layoutPropertyModified(
  vtkObject* sender, unsigned long eventid, void* vdata)
{
  assert(eventid == vtkCommand::PropertyModifiedEvent);
  Q_UNUSED(eventid);

  auto& internals = (*this->Internals);
  auto vlayout = vtkSMViewLayoutProxy::SafeDownCast(sender);
  assert(vlayout);
  if (const char* pname = reinterpret_cast<const char*>(vdata))
  {
    if (strcmp(pname, "SeparatorWidth") == 0)
    {
      internals.setPreviewSpacing(vtkSMPropertyHelper(vlayout, "SeparatorWidth").GetAsInt());
    }
    else if (strcmp(pname, "SeparatorColor") == 0)
    {
      double color[3];
      vtkSMPropertyHelper(vlayout, "SeparatorColor").Get(color, 3);
      internals.setPreviewSeparatorColor(QColor::fromRgbF(color[0], color[1], color[2]));
    }
    else if (strcmp(pname, "PreviewMode") == 0)
    {
      int size[2];
      vtkSMPropertyHelper(vlayout, "PreviewMode").Get(size, 2);
      internals.preview(QSize(size[0], size[1]));
    }
  }
}

//-----------------------------------------------------------------------------
bool pqMultiViewWidget::eventFilter(QObject* caller, QEvent* evt)
{
  if (evt->type() == QEvent::MouseButtonPress)
  {
    QWidget* wdg = qobject_cast<QWidget*>(caller);
    if (wdg && ((!this->Internals->Popout && this->isAncestorOf(wdg)) ||
                 (this->Internals->Popout && this->Internals->PopoutWindow->isAncestorOf(wdg))))
    {
      // If the new widget that is getting the focus is a child widget of any of the
      // frames, then the frame should be made active.
      for (pqViewFrame* frame : this->Internals->Frames)
      {
        if (frame && frame->isAncestorOf(wdg))
        {
          this->makeActive(frame);
        }
      }
    }
  }
  else if (evt->type() == QEvent::Close && caller == this->Internals->PopoutWindow.data())
  {
    // the popout-frame is being closed. We interpret that as "un-popping" the
    // frame.
    this->togglePopout();
  }

  return this->Superclass::eventFilter(caller, evt);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::resizeEvent(QResizeEvent* evt)
{
  this->Superclass::resizeEvent(evt);
  auto& internals = (*this->Internals);
  const auto& psize = internals.previewSize();
  if (!psize.isEmpty())
  {
    internals.preview(psize);
  }
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
void pqMultiViewWidget::makeFrameActive()
{
  /// note pqMultiViewWidget::markActive(pqViewFrame*) fires a signal that
  /// results in this method being called. So we need to ensure that we don't
  /// do anything silly here that could cause infinite recursion.
  if (!this->Internals->ActiveFrame)
  {
    for (auto frame : this->Internals->Frames)
    {
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
    this->markActive(static_cast<pqViewFrame*>(nullptr));
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
    Q_EMIT this->frameActivated();
    // NOTE: this signal will result in call to makeFrameActive().
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeActive(pqViewFrame* frame)
{
  if (this->Internals->ActiveFrame != frame)
  {
    pqView* view = nullptr;
    if (frame)
    {
      int index = frame->property("FRAME_INDEX").toInt();
      view = getPQView(this->layoutManager()->GetView(index));
    }
    pqActiveObjects::instance().setActiveView(view);
    // this needs to called only when view == nullptr since in that case when
    // markActive(pqView*) slot is called, we have no idea what frame is really
    // to be made active.
    this->markActive(frame);
  }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::lockViewSize(const QSize& viewSize)
{
  pqInternals& internals = (*this->Internals);
  internals.lockViewSize(viewSize);
}

//-----------------------------------------------------------------------------
pqViewFrame* pqMultiViewWidget::newFrame(vtkSMProxy* view)
{
  pqViewFrame* frame = new pqViewFrame();
  QObject::connect(frame, SIGNAL(buttonPressed(int)), this, SLOT(standardButtonPressed(int)));
  QObject::connect(frame, SIGNAL(swapPositions(const QString&)), this,
    SLOT(swapPositions(const QString&)), Qt::QueuedConnection);

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smmodel->findItem<pqView*>(view);
  // it's possible that pqview is nullptr, if the view proxy hasn't been registered
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
void pqMultiViewWidget::reload()
{
  vtkSMViewLayoutProxy* vlayout = this->layoutManager();
  if (!vlayout)
  {
    return;
  }

  auto& internals = (*this->Internals);
  auto hlayout = qobject_cast<pqHierarchicalGridLayout*>(internals.Container->layout());
  assert(hlayout != nullptr);

  internals.Frames.clear();

  // for all non-nullptr views known to vlayout, let's make sure we have created pqViewFrame
  // for each of them. No need to delete any obsolete view frames just yet, they'll get cleaned
  // up following `pqHierarchicalGridLayout::rearrange()`
  internals.createViewFrames(vlayout->GetViews(), this);

  // Make sure the `hlayout` matches `vlayout`.
  QVector<pqHierarchicalGridLayout::Item> hitems;

  QVector<QPointer<pqViewFrame> > empty_frames;
  empty_frames = std::move(internals.EmptyFrames);

  auto& all_frames = internals.Frames;

  std::function<void(int)> builder = [&](int location) {
    const auto vdirection = vlayout->GetSplitDirection(location);
    if (hitems.size() <= location)
    {
      hitems.resize(location + 1);
    }
    if (all_frames.size() <= location)
    {
      all_frames.resize(location + 1);
    }

    if (vdirection != vtkSMViewLayoutProxy::NONE)
    {
      hitems[location] = pqHierarchicalGridLayout::Item(
        vdirection == vtkSMViewLayoutProxy::VERTICAL ? Qt::Vertical : Qt::Horizontal,
        vlayout->GetSplitFraction(location));
      builder(vtkSMViewLayoutProxy::GetFirstChild(location));
      builder(vtkSMViewLayoutProxy::GetSecondChild(location));
    }
    else
    {
      auto viewProxy = vlayout->GetView(location); // may be nullptr.
      auto frame = internals.ViewFrames.value(viewProxy, nullptr);
      assert(viewProxy == nullptr || frame != nullptr);
      if (viewProxy == nullptr && frame == nullptr)
      {
        if (empty_frames.size() > 0)
        {
          frame = empty_frames.takeLast();
        }
        else
        {
          frame = this->newFrame(nullptr);
        }
        internals.EmptyFrames.push_back(frame);
      }
      frame->setObjectName(QString("Frame.%1").arg(location));
      frame->setProperty("FRAME_INDEX", QVariant(location));
      frame->setDecorationsVisibility(internals.decorationsVisibility());
      hitems[location] = pqHierarchicalGridLayout::Item(frame);
      all_frames[location] = frame;
    }
  };
  builder(0);

  auto obsoleteWidgets = hlayout->rearrange(hitems);

  // delete frames no longer in the layout.
  for (auto wdg : obsoleteWidgets)
  {
    delete wdg;
  }

  // delete empty frames no longer needed.
  for (auto eframe : empty_frames)
  {
    delete eframe;
  }

  // cleanup ViewFrames map to remove empty frames
  for (auto iter = internals.ViewFrames.begin(); iter != internals.ViewFrames.end();)
  {
    if (iter.value() == nullptr)
    {
      iter = internals.ViewFrames.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
  assert(internals.ViewFrames.size() == static_cast<int>(vlayout->GetViews().size()));

  // handle maximization state.
  const int maximized_cell = vlayout->GetMaximizedCell();
  if (maximized_cell >= 0 && internals.Frames.value(maximized_cell, nullptr))
  {
    internals.setMaximizedWidget(internals.Frames.value(maximized_cell));
    hlayout->maximize(maximized_cell);
  }
  else
  {
    internals.setMaximizedWidget(nullptr);
    hlayout->maximize(0);
  }

  // let's make sure maximum size on all `pqViewFrame`s is set correctly.
  internals.lockViewSize(internals.lockedSize());

  // post reload, the active frames may have changed.
  // so we ensure we mark the right one active.
  this->markActive(pqActiveObjects::instance().activeView());

  // we let the GUI updated immediately. This is needed since when a new view is
  // created (for example), it may depend on the size of the view during its
  // initialization to ensure camera is reset correctly.
  // We have gone back and forth between whether we should let the qt app
  // process events on just process posted events. Calling `processEvents` was
  // finally chosen to address issues like paraview/paraview#18963.
  pqEventDispatcher::processEvents();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::standardButtonPressed(int button)
{
  pqViewFrame* frame = qobject_cast<pqViewFrame*>(this->sender());
  QVariant index = frame ? frame->property("FRAME_INDEX") : QVariant();
  if (!index.isValid() || this->layoutManager() == nullptr)
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
      this->makeActive(this->Internals->Frames[new_index + 1]);
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
      vtkSmartPointer<vtkSMViewProxy> viewProxy = this->layoutManager()->GetView(index.toInt());
      if (viewProxy)
      {
        auto session = viewProxy->GetSession();
        // trigger progress request here delays render requests that happen as
        // the UI is being updated to remove the view. That fixes #18077.
        session->PrepareProgress();
        this->layoutManager()->RemoveView(viewProxy);
        pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
        builder->destroy(getPQView(viewProxy));
        session->CleanupPendingProgress();
      }
      if (index.toInt() != 0)
      {
        int location = index.toInt();
        int parent_idx = vtkSMViewLayoutProxy::GetParent(location);
        this->layoutManager()->Collapse(location);
        pqViewFrame* frameToActivate = this->Internals->Frames[parent_idx];
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
#if !defined(VTK_LEGACY_REMOVE)
void pqMultiViewWidget::setDecorationsVisible(bool val)
{
  VTK_LEGACY_REPLACED_BODY(pqMultiViewWidget::setDecorationsVisible, "ParaView 5.7",
    pqMultiViewWidget::setDecorationsVisibility);
  this->setDecorationsVisibility(val);
}
#endif

//-----------------------------------------------------------------------------
void pqMultiViewWidget::setDecorationsVisibility(bool val)
{
  auto& internals = (*this->Internals);
  internals.setDecorationsVisibility(val);
  Q_EMIT this->decorationsVisibilityChanged(val);
}

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
bool pqMultiViewWidget::isDecorationsVisible() const
{
  VTK_LEGACY_REPLACED_BODY(pqMultiViewWidget::isDecorationsVisible, "ParaView 5.7",
    pqMultiViewWidget::decorationsVisibility);
  return this->decorationsVisibility();
}
#endif

//-----------------------------------------------------------------------------
bool pqMultiViewWidget::decorationsVisibility() const
{
  auto& internals = (*this->Internals);
  return internals.decorationsVisibility();
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

  pqViewFrame* swapWith = nullptr;
  for (pqViewFrame* frame : this->Internals->Frames)
  {
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

  if (view1 == nullptr && view2 == nullptr)
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
  auto& internals = (*this->Internals);
  internals.Popout = !internals.Popout;
  if (internals.Popout)
  {
    if (internals.PopoutWindow == nullptr)
    {
      internals.PopoutWindow.reset(new QWidget(this, Qt::Window | Qt::CustomizeWindowHint |
          Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint));
      internals.PopoutWindow->setObjectName("PopoutWindow");
      auto l = new QVBoxLayout(internals.PopoutWindow.data());
      l->setMargin(0);
      internals.PopoutWindow->resize(this->size());
    }

    if (vtkSMViewLayoutProxy* vlayout = this->layoutManager())
    {
      QString layoutName = vlayout->GetSessionProxyManager()->GetProxyName("layouts", vlayout);
      internals.PopoutWindow->setWindowTitle(layoutName);
    }

    this->layout()->removeWidget(internals.Container);
    this->layout()->addWidget(internals.PopoutPlaceholder.data());
    internals.PopoutPlaceholder->show();

    internals.PopoutWindow->layout()->addWidget(internals.Container);
    internals.PopoutWindow->show();
  }
  else
  {
    assert(internals.PopoutWindow != nullptr);
    internals.PopoutWindow->hide();
    internals.PopoutWindow->layout()->removeWidget(internals.Container);

    internals.PopoutPlaceholder->hide();
    this->layout()->removeWidget(internals.PopoutPlaceholder.data());
    this->layout()->addWidget(internals.Container);
  }
  return internals.Popout;
}

//-----------------------------------------------------------------------------
QSize pqMultiViewWidget::preview(const QSize& nsize)
{
  if (vtkSMViewLayoutProxy* vlayout = this->layoutManager())
  {
    int resolution[2];
    if (nsize.isEmpty())
    {
      resolution[0] = 0;
      resolution[1] = 0;
    }
    else
    {
      resolution[0] = nsize.width();
      resolution[1] = nsize.height();
    }
    SM_SCOPED_TRACE(PropertiesModified)
      .arg("proxy", vlayout)
      .arg("comment", nsize.isEmpty() ? "Exit preview mode" : "Enter preview mode");

    vtkSMPropertyHelper(vlayout, "PreviewMode").Set(resolution, 2);
    //< results in a call to "layoutPropertyModified" if changed.
    vlayout->UpdateVTKObjects();
  }
  return this->Internals->Container->maximumSize();
}

//-----------------------------------------------------------------------------
int pqMultiViewWidget::activeFrameLocation() const
{
  if (auto frame = this->Internals->ActiveFrame)
  {
    return frame->property("FRAME_INDEX").toInt();
  }
  return -1;
}
