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

#include <QImage>
#include <QMoveEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QResizeEvent>

#include "pqUndoStack.h"
#include "vtkRenderWindow.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"

#include "QVTKInteractorAdapter.h"

/**
 * Note on Qt 4 resizing:
 *
 * With Qt 4, if one directly changed the size of a View proxy using the "ViewSize" property
 * it has no effect on OsX (and may be other platforms too). With new screenshot saving mechanism,
 * we rely on changing the ViewSize property during saving of images. If it won't get respected,
 * we have a problem!
 *
 * We handle that by observing modified events from "ViewSize" property. When the property
 * is modified outsize the pqQVTKWidget code itself, we explicitly call `pqQVTKWidget::resize`
 * with the requested size. Thus request Qt to resize the widget to the requested size.
 *
 * This is not needed for Qt 5 and hence this entire commit should be reverted once we drop
 * Qt 4 support.
 */
//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , SizePropertyName("ViewSize")
  , SkipHandleViewSizeForModifiedQt4(false)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // caching only support for QVTKWidget (Qt 4), and not for QVTKOpenGLWidget (Qt 5).
  this->setAutomaticImageCacheEnabled(getenv("DASHBOARD_TEST_FROM_CTEST") == NULL);
#endif

  // Tmp objects
  QPixmap mousePixmap(":/pqCore/Icons/pqMousePick15.png");
  int w = mousePixmap.width();
  int h = mousePixmap.height();
  QImage image(w, h, QImage::Format_ARGB32);
  QPainter painter(&image);
  painter.drawPixmap(0, 0, mousePixmap);
  painter.end();
  image = image.rgbSwapped();

  // Save the loaded image
  this->MousePointerToDraw = image.mirrored();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  this->connect(this, SIGNAL(resized()), SLOT(updateSizeProperties()));

  // disable HiDPI if we are running tests
  this->setEnableHiDPI(getenv("DASHBOARD_TEST_FROM_CTEST") ? false : true);
#endif
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget()
{
}

//----------------------------------------------------------------------------
void pqQVTKWidget::resizeEvent(QResizeEvent* e)
{
  this->Superclass::resizeEvent(e);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  this->updateSizeProperties();
#endif
}

//----------------------------------------------------------------------------
void pqQVTKWidget::updateSizeProperties()
{
  if (this->ViewProxy)
  {
    // see comment at the top on Qt 4 resizing
    bool prev = this->SkipHandleViewSizeForModifiedQt4;
    this->SkipHandleViewSizeForModifiedQt4 = true;
    BEGIN_UNDO_EXCLUDE();
    int view_size[2];
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    view_size[0] = this->size().width() * this->InteractorAdaptor->GetDevicePixelRatio();
    view_size[1] = this->size().height() * this->InteractorAdaptor->GetDevicePixelRatio();
#else
    view_size[0] = this->size().width();
    view_size[1] = this->size().height();
#endif
    vtkSMPropertyHelper(this->ViewProxy, this->SizePropertyName.toLocal8Bit().data())
      .Set(view_size, 2);
    this->ViewProxy->UpdateProperty(this->SizePropertyName.toLocal8Bit().data());
    END_UNDO_EXCLUDE();
    this->SkipHandleViewSizeForModifiedQt4 = prev;
  }

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // all of this is not needed for Qt 5 since updateSizeProperties() is called
  // after resize but before `paintGL`.
  this->markCachedImageAsDirty();

  // need to request a render after the "resizing" is done.
  this->update();
#endif
}

//----------------------------------------------------------------------------
void pqQVTKWidget::handleViewSizeForModifiedQt4()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // see comment at the top on Qt 4 resizing
  if (!this->SkipHandleViewSizeForModifiedQt4)
  {
    vtkSMPropertyHelper h(this->ViewProxy, this->SizePropertyName.toLocal8Bit().data());
    this->resize(QSize(h.GetAsInt(0), h.GetAsInt(1)));
  }
#endif
}
//----------------------------------------------------------------------------
void pqQVTKWidget::setViewProxy(vtkSMProxy* view)
{
  this->ViewProxy = view;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // see comment at the top on Qt 4 resizing
  this->VTKConnect->Disconnect();
  if (vtkSMProperty* prop =
        (view ? view->GetProperty(this->SizePropertyName.toLocal8Bit().data()) : nullptr))
  {
    this->VTKConnect->Connect(
      prop, vtkCommand::ModifiedEvent, this, SLOT(handleViewSizeForModifiedQt4()));
  }
#endif
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setSession(vtkSMSession* session)
{
  this->Session = session;
}

//----------------------------------------------------------------------------
void pqQVTKWidget::doDeferredRender()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  if (this->canRender())
  {
    this->Superclass::doDeferredRender();
  }
#endif
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::renderVTK()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  return this->canRender() ? this->Superclass::renderVTK() : false;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::canRender()
{
  // despite our best efforts, it's possible that the paint event happens while
  // the server manager is busy processing some other request that yields
  // progress (e.g. pvcrs.UndoRedo2 test).
  // Triggering renders in that case is hazardous. So we skip calling
  // rendering in those cases.
  if (this->ViewProxy && this->ViewProxy->GetSession()->GetPendingProgress())
  {
    return false;
  }

  if (this->Session && this->Session->GetPendingProgress())
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 pqQVTKWidget::getProxyId()
{
  if (this->ViewProxy)
  {
    return this->ViewProxy->GetGlobalID();
  }
  return 0;
}

//----------------------------------------------------------------------------
void pqQVTKWidget::paintMousePointer(int xLocation, int yLocation)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  Q_UNUSED(xLocation);
  Q_UNUSED(yLocation);
#else
  // Local repaint
  QVTKWidget::paintEvent(NULL);

  // Paint mouse pointer image on top of it
  int imagePointingDelta = 10;
  this->GetRenderWindow()->SetRGBACharPixelData(xLocation - imagePointingDelta,
    this->height() - yLocation + imagePointingDelta,
    this->MousePointerToDraw.width() + xLocation - 1 - imagePointingDelta,
    this->height() - (this->MousePointerToDraw.height() + yLocation + 1) + imagePointingDelta,
    this->MousePointerToDraw.bits(), 1, 1);
#endif
}
