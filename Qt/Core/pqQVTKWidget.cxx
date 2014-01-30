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
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QPoint>

#include "pqUndoStack.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkRenderWindow.h"

#include "QVTKInteractorAdapter.h"

//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f), SizePropertyName("ViewSize")
{
  this->setAutomaticImageCacheEnabled(getenv("DASHBOARD_TEST_FROM_CTEST")==NULL);

  // Tmp objects
  QPixmap mousePixmap(":/pqWidgets/Icons/pqMousePick15.png");
  int w = mousePixmap.width();
  int h = mousePixmap.height();
  QImage image(w, h, QImage::Format_ARGB32);
  QPainter painter(&image);
  painter.drawPixmap(0,0,mousePixmap);
  painter.end();
  image = image.rgbSwapped();

  // Save the loaded image
  this->MousePointerToDraw = image.mirrored();
  
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget()
{
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
    vtkSMPropertyHelper(
      this->ViewProxy, this->SizePropertyName.toLatin1().data()).Set(view_size, 2);
    this->ViewProxy->UpdateProperty(
      this->SizePropertyName.toLatin1().data());
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
//----------------------------------------------------------------------------
vtkTypeUInt32 pqQVTKWidget::getProxyId()
{
  if(this->ViewProxy)
    {
    return this->ViewProxy->GetGlobalID();
    }
  return 0;
}

//----------------------------------------------------------------------------
void pqQVTKWidget::paintMousePointer(int xLocation, int yLocation)
{
  // Local repaint
  QVTKWidget::paintEvent(NULL);

  // Paint mouse pointer image on top of it
  int imagePointingDelta = 10;
  this->mRenWin->SetRGBACharPixelData(
      xLocation - imagePointingDelta,
      this->height() - yLocation + imagePointingDelta,
      this->MousePointerToDraw.width()+xLocation-1 - imagePointingDelta,
      this->height() - (this->MousePointerToDraw.height() + yLocation + 1) + imagePointingDelta,
      this->MousePointerToDraw.bits(), 1, 1);
}
