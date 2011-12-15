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

#include "vtkWeakPointer.h"
#include "vtkSMViewLayoutProxy.h"
#include "pqMultiViewFrame.h"

#include <QFrame>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>

class pqMultiViewWidget::pqInternals
{
public:
  QMap<void*, QPointer<QWidget> > ViewFrames;
  vtkWeakPointer<vtkSMViewLayoutProxy> LayoutManager;
  QPointer<QWidget> Container;
};

//-----------------------------------------------------------------------------
pqMultiViewWidget::pqMultiViewWidget(QWidget * parentObject, Qt::WindowFlags f)
: Superclass(parentObject, f),
  Internals( new pqInternals())
{
  this->setLayout(new QVBoxLayout(this));
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
QWidget* pqMultiViewWidget::newFrame(vtkSMProxy* view)
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
  return frame;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiViewWidget::createWidget(
  unsigned int index, vtkSMViewLayoutProxy* vlayout, QWidget* parentWdg)
{
  vtkSMViewLayoutProxy::Direction direction = vlayout->GetSplitDirection(index);
  switch (direction)
    {
  case vtkSMViewLayoutProxy::NONE:
      {
      vtkSMProxy* view = vlayout->GetView(index);
      QWidget* frame = NULL;
      if (view == NULL || !this->Internals->ViewFrames.contains(view))
        {
        frame = this->newFrame(view);
        if (view != NULL)
          {
          this->Internals->ViewFrames[view] = frame;
          frame->setParent(this);
          }
        else
          {
          frame->setParent(parentWdg);
          }
        }
      else
        {
        frame = this->Internals->ViewFrames[view];
        }
      frame->setObjectName(QString("Frame.%1").arg(index));
      frame->setProperty("FRAME_INDEX", QVariant(index));
      return frame;
      }

  case vtkSMViewLayoutProxy::VERTICAL:
  case vtkSMViewLayoutProxy::HORIZONTAL:
      {
      QSplitter* splitter = new QSplitter(parentWdg);
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
  delete this->Internals->Container;
  vtkSMViewLayoutProxy* vlayout = this->layoutManager();
  if (!vlayout)
    {
    return;
    }

  QWidget* container = new QWidget(this);
  QVBoxLayout* vbox = new QVBoxLayout(container);
  container->setLayout(vbox);

  QWidget* child = this->createWidget(0, vlayout, container);
  vbox->addWidget(child);

  this->layout()->addWidget(container);
  this->Internals->Container = container;
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
