// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
    builder->createSource("sources", source_type.toUtf8().data(), s);
    if (stack)
    {
      stack->endUndoSet();
    }
  }
}
