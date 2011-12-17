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
#include "pqInterfaceTracker.h"
#include "pqMultiViewFrame.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqViewFrameActionGroupInterface.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkWeakPointer.h"

#include <QFrame>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>
#include <QApplication>
#include <QVector>
#include <QMap>

class pqMultiViewWidget::pqInternals
{
public:
  QVector<QPointer<QWidget> > Widgets;

  // This map is used to avoid reassigning frames. Once a view is assigned a
  // frame, we preserve that frame as long as possible.
  QMap<vtkSMProxy*, QPointer<pqMultiViewFrame> > ViewFrames;

  unsigned long ObserverId;
  vtkWeakPointer<vtkSMViewLayoutProxy> LayoutManager;
  QPointer<pqMultiViewFrame> ActiveFrame;

  pqInternals() : ObserverId(0)
    {

    }

  ~pqInternals()
    {
    if (this->LayoutManager && this->ObserverId)
      {
      this->LayoutManager->RemoveObserver(this->ObserverId);
      }
    }
};

//-----------------------------------------------------------------------------
namespace
{
  pqView* getPQView(vtkSMProxy* view)
    {
    if (view)
      {
      pqServerManagerModel* smmodel =
        pqApplicationCore::instance()->getServerManagerModel();
      return smmodel->findItem<pqView*>(view);
      }
    return NULL;
    }
}

//-----------------------------------------------------------------------------
pqMultiViewWidget::pqMultiViewWidget(QWidget * parentObject, Qt::WindowFlags f)
: Superclass(parentObject, f),
  Internals( new pqInternals())
{
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  this->setLayout(vbox);

  qApp->installEventFilter(this);

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(viewChanged(pqView*)), this, SLOT(markActive(pqView*)));
}

//-----------------------------------------------------------------------------
pqMultiViewWidget::~pqMultiViewWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::setLayoutManager(vtkSMViewLayoutProxy* vlayout)
{
  if (this->Internals->LayoutManager != vlayout)
    {
    if (this->Internals->LayoutManager)
      {
      this->Internals->LayoutManager->RemoveObserver(
        this->Internals->ObserverId);
      }
    this->Internals->ObserverId = 0;
    this->Internals->LayoutManager = vlayout;
    if (vlayout)
      {
      this->Internals->ObserverId =
        vlayout->AddObserver(vtkCommand::ConfigureEvent,
          this, &pqMultiViewWidget::reload);
      }
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
    if (wdg && this->isAncestorOf(wdg))
      {
      // If the new widget that is getting the focus is a child widget of any of the
      // frames, then the frame should be made active.
      foreach (QWidget* frame_or_splitter, this->Internals->Widgets)
        {
        pqMultiViewFrame* frame =
          qobject_cast<pqMultiViewFrame*>(frame_or_splitter);
        if (frame && frame->isAncestorOf(wdg))
          {
          this->makeActive(frame);
          }
        }
      }
    }

  return this->Superclass::eventFilter(caller, evt);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::assignToFrame(pqView* view)
{
  if (this->layoutManager() && view)
    {
    int active_index = 0;
    if (this->Internals->ActiveFrame)
      {
      active_index =
        this->Internals->ActiveFrame->property("FRAME_INDEX").toInt();
      }
    this->layoutManager()->AssignViewToAnyCell(view->getProxy(), active_index);

    if (view)
      {
      // FIXME: need to remove observer when pqMultiViewWidget is deleted.
      view->getProxy()->GetProperty("ViewSize")->AddObserver(
        vtkCommand::ModifiedEvent,
        this, &pqMultiViewWidget::updateViewPositions);
      }

    }
  pqActiveObjects::instance().setActiveView(view);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeFrameActive()
{
  if (!this->Internals->ActiveFrame)
    {
    foreach (QWidget* wdg, this->Internals->Widgets)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(wdg);
      if (frame)
        {
        this->makeActive(frame);
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::markActive(pqView* view)
{
   if (view &&
    this->Internals->ViewFrames.contains(view->getProxy()))
     {
     this->markActive(this->Internals->ViewFrames[view->getProxy()]);
     }
   else
     {
     this->markActive(static_cast<pqMultiViewFrame*>(NULL));
     }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::markActive(pqMultiViewFrame* frame)
{
  if (this->Internals->ActiveFrame)
    {
    this->Internals->ActiveFrame->setActive(false);
    }
  this->Internals->ActiveFrame = frame;
  if (frame)
    {
    frame->setActive(true);
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeActive(pqMultiViewFrame* frame)
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
pqMultiViewFrame* pqMultiViewWidget::newFrame(vtkSMProxy* view)
{
  pqMultiViewFrame* frame = new pqMultiViewFrame(this);
  frame->showDecorations();
  frame->MaximizeButton->show();
  frame->CloseButton->show();
  frame->SplitVerticalButton->show();
  frame->SplitHorizontalButton->show();

  QObject::connect(frame, SIGNAL(splitVerticalPressed()),
    this, SLOT(splitVertical()));
  QObject::connect(frame, SIGNAL(splitHorizontalPressed()),
    this, SLOT(splitHorizontal()));
  QObject::connect(frame, SIGNAL(closePressed()),
    this, SLOT(close()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smmodel->findItem<pqView*>(view);
  if (view)
    {
    Q_ASSERT(pqview != NULL);

    QWidget* viewWidget = pqview->getWidget();
    frame->setMainWidget(viewWidget);
    viewWidget->setParent(frame);
    }

  // Search for view frame action group plugins and allow them to decide
  // whether to add their actions to this view type's frame or not.
  pqInterfaceTracker* tracker =
    pqApplicationCore::instance()->interfaceTracker();
  foreach (pqViewFrameActionGroupInterface* agi,
    tracker->interfaces<pqViewFrameActionGroupInterface*>())
    {
    agi->connect(frame, pqview);
    }

  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiViewWidget::createWidget(
  int index, vtkSMViewLayoutProxy* vlayout, QWidget* parentWdg)
{
  if (this->Internals->Widgets.size() <= static_cast<int>(index))
    {
    this->Internals->Widgets.resize(index+1);
    }

  vtkSMViewLayoutProxy::Direction direction = vlayout->GetSplitDirection(index);
  switch (direction)
    {
  case vtkSMViewLayoutProxy::NONE:
      {
      vtkSMProxy* view = vlayout->GetView(index);
      pqMultiViewFrame* frame = view?
        this->Internals->ViewFrames[view] : NULL;
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
      return frame;
      }

  case vtkSMViewLayoutProxy::VERTICAL:
  case vtkSMViewLayoutProxy::HORIZONTAL:
      {
      QSplitter* splitter = qobject_cast<QSplitter*>(
        this->Internals->Widgets[index]);
      if (!splitter)
        {
        splitter = new QSplitter(parentWdg);
        }
      Q_ASSERT(splitter);
      splitter->setParent(parentWdg);
      this->Internals->Widgets[index] = splitter;
      splitter->setObjectName(QString("Splitter.%1").arg(index));
      splitter->setProperty("FRAME_INDEX", QVariant(index));
      splitter->setOpaqueResize(false);
      splitter->setOrientation(
        direction == vtkSMViewLayoutProxy::VERTICAL?
        Qt::Vertical : Qt::Horizontal);
      splitter->addWidget(
        this->createWidget(vlayout->GetFirstChild(index), vlayout, parentWdg));
      splitter->addWidget(
        this->createWidget(vlayout->GetSecondChild(index), vlayout, parentWdg));

      // set the sizes are percentage. QSplitter uses the initially specified
      // sizes as reference.
      QList<int> sizes;
      sizes << vlayout->GetSplitFraction(index) * 10000;
      sizes << (1.0 - vlayout->GetSplitFraction(index))  * 10000;
      splitter->setSizes(sizes);

      // FIXME: Don't like this as this QueuedConnection may cause multiple
      // renders.
      QObject::disconnect(splitter, SIGNAL(splitterMoved(int, int)),
        this, SLOT(splitterMoved()));
      QObject::connect(splitter, SIGNAL(splitterMoved(int, int)),
        this, SLOT(splitterMoved()), Qt::QueuedConnection);
      return splitter;
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

  QWidget *cleaner = new QWidget();
  foreach (QWidget* widget, this->Internals->Widgets)
    {
    if (widget)
      {
      widget->setParent(cleaner);
      }
    }
  QWidget* child = this->createWidget(0, vlayout, this);
  delete cleaner;
  cleaner = NULL;

  delete this->layout();
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(child);
  this->setLayout(vbox);

  // ensure the active view is marked appropriately.
  this->markActive(pqActiveObjects::instance().activeView());

  // Cleanup deleted objects. "cleaner" helps us avoid any dangling widgets and
  // deletes them whwn delete cleaner is called. Now we prune internal
  // datastructures to get rid of these NULL ptrs.

  // remove any deleted view frames.
  QMutableMapIterator<vtkSMProxy*, QPointer<pqMultiViewFrame> > iter(
      this->Internals->ViewFrames);
  while (iter.hasNext())
    {
    iter.next();
    if (iter.value() == NULL)
      {
      iter.remove();
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitVertical()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    BEGIN_UNDO_SET("Split View");
    int new_index = this->layoutManager()->SplitVertical(index.toInt(), 0.5);
    this->makeActive(qobject_cast<pqMultiViewFrame*>(
        this->Internals->Widgets[new_index + 1]));
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitHorizontal()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    BEGIN_UNDO_SET("Split View");
    int new_index = this->layoutManager()->SplitHorizontal(index.toInt(), 0.5);
    this->makeActive(qobject_cast<pqMultiViewFrame*>(
        this->Internals->Widgets[new_index + 1]));
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::close()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    BEGIN_UNDO_SET("Close View");
    this->layoutManager()->Collapse(index.toInt());
    END_UNDO_SET();
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitterMoved()
{
  QSplitter* splitter = qobject_cast<QSplitter*>(this->sender());
  QVariant index = splitter? splitter->property("FRAME_INDEX") : QVariant();
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
void pqMultiViewWidget::updateViewPositions()
{
  if (this->layoutManager())
    {
    this->layoutManager()->UpdateViewPositions();
    }
}
