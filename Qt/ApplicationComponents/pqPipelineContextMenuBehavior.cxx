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
#include "pqPipelineContextMenuBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqEditColorMapReaction.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqServerManagerModel.h"

#include "vtkSMProxy.h"

#include <QWidget>
#include <QAction>
#include <QMenu>
#include <QMouseEvent>

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::pqPipelineContextMenuBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)),
    this, SLOT(onViewAdded(pqView*)),
    Qt::QueuedConnection);
  this->Menu = new QMenu();
}

//-----------------------------------------------------------------------------
pqPipelineContextMenuBehavior::~pqPipelineContextMenuBehavior()
{
  delete this->Menu;
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::onViewAdded(pqView* view)
{
  if (view && view->getProxy()->IsA("vtkSMRenderViewProxy"))
    {
    // add a link view menu
    view->getWidget()->installEventFilter(this);
    }
}

//-----------------------------------------------------------------------------
bool pqPipelineContextMenuBehavior::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
      {
      this->Position = me->pos();
      }
    }
  else if (e->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Position.isNull())
      {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Position;
      QWidget* senderWidget = qobject_cast<QWidget*>(caller);
      if (delta.manhattanLength() < 3 && senderWidget != NULL)
        {
        pqRenderView* view = qobject_cast<pqRenderView*>(
          pqActiveObjects::instance().activeView());
        if (view)
          {
          int pos[2] = { newPos.x(), newPos.y() } ;
          pqDataRepresentation* picked_repr = view->pick(pos);
          if (picked_repr)
            {
            this->Menu->clear();
            this->buildMenu(picked_repr);
            this->Menu->popup(senderWidget->mapToGlobal(newPos));
            }
          }
        }
      this->Position = QPoint();
      }
    }

  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::buildMenu(pqDataRepresentation* repr)
{
  pqPipelineRepresentation* pipelineRepr =
    qobject_cast<pqPipelineRepresentation*>(repr);

  QAction* action;
  action = this->Menu->addAction("Hide");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(hide()));

  QMenu* reprMenu = this->Menu->addMenu("Representation");

  // populate the representation types menu.
  QList<QVariant> rTypes = pqSMAdaptor::getEnumerationPropertyDomain(
    repr->getProxy()->GetProperty("Representation"));
  QVariant curRType = pqSMAdaptor::getEnumerationProperty(
    repr->getProxy()->GetProperty("Representation"));
  foreach (QVariant rtype, rTypes)
    {
    QAction* raction = reprMenu->addAction(rtype.toString());
    raction->setCheckable(true);
    raction->setChecked(rtype == curRType);
    }
  QObject::connect(reprMenu, SIGNAL(triggered(QAction*)),
    this, SLOT(reprTypeChanged(QAction*)));

  this->Menu->addSeparator();
  action = this->Menu->addAction("Edit Color");
  new pqEditColorMapReaction(action);

  if (pipelineRepr)
    {
    QMenu* colorFieldsMenu = this->Menu->addMenu("Color By");
    QList<QString> available_fields = pipelineRepr->getColorFields();
    foreach (QString field, available_fields)
      {
      colorFieldsMenu->addAction(field);
      //raction->setCheckable(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::hide()
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  if (repr)
    {
    repr->setVisible(false);
    repr->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineContextMenuBehavior::reprTypeChanged(QAction* action)
{
  pqDataRepresentation* repr =
    pqActiveObjects::instance().activeRepresentation();
  if (repr)
    {
    pqSMAdaptor::setEnumerationProperty(
      repr->getProxy()->GetProperty("Representation"),
      action->text());
    repr->getProxy()->UpdateVTKObjects();
    repr->renderViewEventually();
    }
}
