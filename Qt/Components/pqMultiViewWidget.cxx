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
#include "vtkSMViewLayout.h"
#include "pqMultiViewFrame.h"

#include <QFrame>
#include <QPointer>
#include <QSplitter>
#include <QVBoxLayout>

class pqMultiViewWidget::pqInternals
{
public:
  vtkWeakPointer<vtkSMViewLayout> LayoutManager;
  QPointer<QWidget> Container;
};

//-----------------------------------------------------------------------------
pqMultiViewWidget::pqMultiViewWidget(
  QWidget * parentObject, Qt::WindowFlags f)
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
void pqMultiViewWidget::setLayoutManager(vtkSMViewLayout* vlayout)
{
  this->Internals->LayoutManager = vlayout;
  this->reload();
}

//-----------------------------------------------------------------------------
vtkSMViewLayout* pqMultiViewWidget::layoutManager() const
{
  return this->Internals->LayoutManager;
}

//-----------------------------------------------------------------------------
QWidget* pqMultiViewWidget::newFrame(vtkSMProxy* view)
{
  return new pqMultiViewFrame();
}

namespace
{
  QWidget* GetWidget(unsigned int index, vtkSMViewLayout* vlayout)
    {
    vtkSMViewLayout::Direction direction = vlayout->GetSplitDirection(index);
    switch (direction)
      {
    case vtkSMViewLayout::NONE:
        {
        pqMultiViewFrame* frame = new pqMultiViewFrame();
        frame->setObjectName(QString("Frame.%1").arg(index));
        return frame;
        }

    case vtkSMViewLayout::VERTICAL:
    case vtkSMViewLayout::HORIZONTAL:
        {
        QSplitter* splitter = new QSplitter();
        splitter->setOpaqueResize(false);
        splitter->setOrientation(
          direction == vtkSMViewLayout::VERTICAL?
          Qt::Vertical : Qt::Horizontal);
        splitter->addWidget(GetWidget(vlayout->GetFirstChild(index), vlayout));
        splitter->addWidget(GetWidget(vlayout->GetSecondChild(index), vlayout));
        splitter->setObjectName(QString("Splitter.%1").arg(index));
        return splitter;
        }
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqMultiViewWidget::reload()
{
  delete this->Internals->Container;
  vtkSMViewLayout* vlayout = this->layoutManager();
  if (!vlayout)
    {
    return;
    }

  this->Internals->Container = GetWidget(0, vlayout);
  this->layout()->addWidget(this->Internals->Container);
}
