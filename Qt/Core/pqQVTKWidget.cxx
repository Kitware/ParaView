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

//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , SizePropertyName("ViewSize")
{
  this->connect(this, SIGNAL(resized()), SLOT(updateSizeProperties()));

  // disable HiDPI if we are running tests
  this->setEnableHiDPI(getenv("DASHBOARD_TEST_FROM_CTEST") ? false : true);
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget()
{
}

//----------------------------------------------------------------------------
void pqQVTKWidget::updateSizeProperties()
{
  if (this->ViewProxy)
  {
    BEGIN_UNDO_EXCLUDE();
    int view_size[2];
    view_size[0] = this->size().width() * this->GetInteractorAdapter()->GetDevicePixelRatio();
    view_size[1] = this->size().height() * this->GetInteractorAdapter()->GetDevicePixelRatio();
    vtkSMPropertyHelper(this->ViewProxy, this->SizePropertyName.toLocal8Bit().data())
      .Set(view_size, 2);
    this->ViewProxy->UpdateProperty(this->SizePropertyName.toLocal8Bit().data());
    END_UNDO_EXCLUDE();
  }
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
  Q_UNUSED(xLocation);
  Q_UNUSED(yLocation);
  // TODO: need to add support to paint collaboration mouse pointer in Qt 5.
}
