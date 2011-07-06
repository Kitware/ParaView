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
#include "pqQVTKWidget.h"

#include <QMoveEvent>
#include <QResizeEvent>
#include <QTimer>

#include "pqUndoStack.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WFlags f)
  : Superclass(parentObject, f)
{
  this->setAutomaticImageCacheEnabled(getenv("DASHBOARD_TEST_FROM_CTEST")==NULL);
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget()
{
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setPositionReference(QWidget* widget)
{
  this->PositionReference = widget;
}

//----------------------------------------------------------------------------
QWidget* pqQVTKWidget::positionReference() const
{
  if (this->PositionReference)
    {
    return this->PositionReference;
    }
  return this->parentWidget();
}

//----------------------------------------------------------------------------
void pqQVTKWidget::resizeEvent(QResizeEvent* e)
{
  this->Superclass::resizeEvent(e);
  this->updateSizeProperties();
}

//----------------------------------------------------------------------------
void pqQVTKWidget::updateSizeProperties()
{
  if (this->ViewProxy)
    {
    BEGIN_UNDO_EXCLUDE();
    int view_size[2];
    view_size[0] = this->size().width();
    view_size[1] = this->size().height();
    vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Set(view_size, 2);

    QPoint view_pos = this->mapTo(this->positionReference(), QPoint(0,0)) -
      this->mapToParent(QPoint(0, 0));
    int view_position[2];
    view_position[0] = view_pos.x();
    view_position[1] = view_pos.y();;
    vtkSMPropertyHelper(this->ViewProxy, "ViewPosition").Set(view_position, 2);
    this->ViewProxy->UpdateProperty("ViewSize");
    this->ViewProxy->UpdateProperty("ViewPosition");
    END_UNDO_EXCLUDE();
    }

  this->markCachedImageAsDirty();

  // need to request a render after the "resizing" is done.
  this->update();
}

//----------------------------------------------------------------------------
// moveEvent doesn't help us, since this is fired when the pqQVTKWidget is moved
// inside its parent, which rarely happens.
void pqQVTKWidget::moveEvent(QMoveEvent* e)
{
  this->Superclass::moveEvent(e);
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setViewProxy(vtkSMProxy* view)
{
  this->ViewProxy = view;
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setSession(vtkSMSession* session)
{
  this->Session = session;
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::paintCachedImage()
{
  // In future we can update this code to ensure that view->Render() is never
  // called from the pqQVTKWidget. For now, we are letting the default path
  // execute when not resizing.

  if (this->Superclass::paintCachedImage())
    {
    return true;
    }

  // despite our best efforts, it's possible that the paint event happens while
  // the server manager is busy processing some other request that yields
  // progress (e.g. pvcrs.UndoRedo2 test).
  // Triggering renders in that case is hazardous. So we skip calling
  // rendering in those cases.
  if (this->ViewProxy && this->ViewProxy->GetSession()->GetPendingProgress())
    {
    return true;
    }

  if (this->Session && this->Session->GetPendingProgress())
    {
    return true;
    }
  return false;
}
