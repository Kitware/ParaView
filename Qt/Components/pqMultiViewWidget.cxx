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

#include "pqApplicationCore.h"
#include "pqMultiViewFrame.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkWeakPointer.h"

#include <QFrame>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>
#include <QApplication>

class pqMultiViewWidget::pqInternals
{
public:
  QVector<QPointer<QWidget> > Widgets;
  QList<QPointer<pqMultiViewFrame> > EmptyFrames;

  vtkWeakPointer<vtkSMViewLayoutProxy> LayoutManager;
  QPointer<QWidget> Cleaner;

  QPointer<pqMultiViewFrame> ActiveFrame;
};

//-----------------------------------------------------------------------------
pqMultiViewWidget::pqMultiViewWidget(QWidget * parentObject, Qt::WindowFlags f)
: Superclass(parentObject, f),
  Internals( new pqInternals())
{
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  this->setLayout(vbox);

  qApp->installEventFilter(this);
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
  this->Internals->LayoutManager = vlayout;
  this->reload();
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
      foreach (QWidget* wdg, this->Internals->Widgets)
        {
        pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(wdg);
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
  // FIXME:
  if (this->Internals->EmptyFrames.size() > 0)
    {
    QWidget* frame = this->Internals->EmptyFrames[0];
    QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
    if (index.isValid())
      {
      this->layoutManager()->AssignView(index.toUInt(), view->getProxy());
      this->reload();
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::makeActive(pqMultiViewFrame* frame)
{
  if (this->Internals->ActiveFrame != frame)
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

  if (view)
    {
    pqServerManagerModel* smmodel =
      pqApplicationCore::instance()->getServerManagerModel();
    pqView* pqview = smmodel->findItem<pqView*>(view);
    Q_ASSERT(pqview != NULL);
    QWidget* viewWidget = pqview->getWidget();
    frame->setMainWidget(viewWidget);
    viewWidget->setParent(frame);
    }
  return frame;
}

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
QWidget* pqMultiViewWidget::createWidget(
  unsigned int index, vtkSMViewLayoutProxy* vlayout, QWidget* parentWdg)
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
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        this->Internals->Widgets[index]);
      if (!frame)
        {
        frame = this->newFrame(view);
        }
      Q_ASSERT(frame != NULL);

      frame->setParent(parentWdg);
      this->Internals->Widgets[index] = frame;

      if (frame->mainWidget())
        {
        frame->setMainWidget(NULL);
        }

      if (view != NULL)
        {
        pqView* pqview = getPQView(view);
        Q_ASSERT(pqview != NULL);
        QWidget* viewWidget = pqview->getWidget();
        frame->setMainWidget(viewWidget);
        viewWidget->setParent(frame);
        }
      else
        {
        this->Internals->EmptyFrames.push_back(frame);
        }
      frame->setObjectName(QString("Frame.%1").arg(index));
      frame->setProperty("FRAME_INDEX", QVariant(index));
      if (!this->Internals->ActiveFrame)
        {
        this->makeActive(frame);
        }
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
  this->Internals->EmptyFrames.clear();
  vtkSMViewLayoutProxy* vlayout = this->layoutManager();
  if (!vlayout)
    {
    return;
    }

  this->Internals->Cleaner = new QWidget(this);

  foreach (QWidget* widget, this->Internals->Widgets)
    {
    if (widget)
      {
      widget->setParent(this->Internals->Cleaner);
      }
    }
  QWidget* child = this->createWidget(0, vlayout, this);
  delete this->Internals->Cleaner;

  delete this->layout();
  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->addWidget(child);
  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitVertical()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    this->layoutManager()->SplitVertical(index.toUInt(), 0.5);
    }
  this->reload();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::splitHorizontal()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    this->layoutManager()->SplitHorizontal(index.toUInt(), 0.5);
    }
  this->reload();
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::close()
{
  QWidget* frame = qobject_cast<QWidget*>(this->sender());
  QVariant index = frame? frame->property("FRAME_INDEX") : QVariant();
  if (index.isValid() && this->layoutManager())
    {
    this->layoutManager()->Collape(index.toUInt());
    }
  this->reload();
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
      double fraction = sizes[0] * 1.0 / (sizes[0] + sizes[1]);
      this->layoutManager()->SetSplitFraction(index.toUInt(), fraction);
      }
    }
  this->reload();
}
