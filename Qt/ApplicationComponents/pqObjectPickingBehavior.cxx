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
#include "pqObjectPickingBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkPVRenderView.h"
#include "vtkSMOutputPort.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxySelectionModel.h"

#include <QMouseEvent>
#include <QWidget>

//-----------------------------------------------------------------------------
pqObjectPickingBehavior::pqObjectPickingBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
}

//-----------------------------------------------------------------------------
pqObjectPickingBehavior::~pqObjectPickingBehavior() = default;

//-----------------------------------------------------------------------------
void pqObjectPickingBehavior::onViewAdded(pqView* view)
{
  if (qobject_cast<pqRenderView*>(view))
  {
    // add a link view menu
    view->widget()->installEventFilter(this);
  }
}

//-----------------------------------------------------------------------------
bool pqObjectPickingBehavior::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::LeftButton)
    {
      this->Position = me->pos();
    }
  }
  else if (e->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::LeftButton && !this->Position.isNull())
    {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Position;
      QWidget* senderWidget = qobject_cast<QWidget*>(caller);
      if (delta.manhattanLength() < 3.0 && senderWidget != nullptr)
      {
        pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
        if (view)
        {
          BEGIN_UNDO_EXCLUDE();

          // check if the view is currently doing a selection, in that case,
          // ignore the "pick".
          int mode = vtkSMPropertyHelper(view->getProxy(), "InteractionMode").GetAsInt();
          if (mode == vtkPVRenderView::INTERACTION_MODE_3D ||
            mode == vtkPVRenderView::INTERACTION_MODE_2D)
          {
            int pos[2] = { newPos.x(), newPos.y() };
            // we need to flip Y.
            int height = senderWidget->size().height();
            pos[1] = height - pos[1];
            pqDataRepresentation* picked = view->pick(pos);
            // in pick-on-click, we don't change the current item when user clicked on
            // a blank area. BUG #11428.
            if (picked)
            {
              vtkSMProxySelectionModel* selModel = view->getServer()->activeSourcesSelectionModel();
              if (selModel)
              {
                selModel->SetCurrentProxy(picked->getOutputPortFromInput()->getOutputPortProxy(),
                  vtkSMProxySelectionModel::CLEAR_AND_SELECT);
              }
            }
          }

          END_UNDO_EXCLUDE();
        }
      }
      this->Position = QPoint();
    }
  }

  return this->Superclass::eventFilter(caller, e);
}
