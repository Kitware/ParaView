/*=========================================================================

   Program: ParaView
   Module:    SourceToolbarActions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "SourceToolbarActions.h"

#include <QApplication>
#include <QStyle>

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
SourceToolbarActions::SourceToolbarActions(QObject* p)
  : QActionGroup(p)
{
  // let's use a Qt icon (we could make our own)
  QIcon icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxCritical);
  QAction* a = new QAction(icon, "Create Sphere", this);
  a->setData("SphereSource");
  this->addAction(a);
  a = new QAction(icon, "Create Cylinder", this);
  a->setData("CylinderSource");
  this->addAction(a);
  QObject::connect(this, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));
}

//-----------------------------------------------------------------------------
void SourceToolbarActions::onAction(QAction* a)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();
  pqServerManagerModel* sm = core->getServerManagerModel();
  pqUndoStack* stack = core->getUndoStack();

  /// Check that we are connect to some server (either builtin or remote).
  if (sm->getNumberOfItems<pqServer*>())
  {
    // just create it on the first server connection
    pqServer* s = sm->getItemAtIndex<pqServer*>(0);
    QString source_type = a->data().toString();
    // make this operation undo-able if undo is enabled
    if (stack)
    {
      stack->beginUndoSet(QString("Create %1").arg(source_type));
    }
    builder->createSource("sources", source_type.toLocal8Bit().data(), s);
    if (stack)
    {
      stack->endUndoSet();
    }
  }
}
