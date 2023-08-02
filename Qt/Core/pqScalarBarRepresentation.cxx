// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqScalarBarRepresentation.h"

#include "pqUndoStack.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::pqScalarBarRepresentation(const QString& group, const QString& name,
  vtkSMProxy* scalarbar, pqServer* server, QObject* _parent)
  : Superclass(group, name, scalarbar, server, _parent)
{
  vtkEventQtSlotConnect* connector = this->getConnector();

  // Listen to start/end interactions to update the application undo-redo stack
  // correctly.
  connector->Connect(scalarbar, vtkCommand::StartInteractionEvent, this, SLOT(startInteraction()));
  connector->Connect(scalarbar, vtkCommand::EndInteractionEvent, this, SLOT(endInteraction()));
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::~pqScalarBarRepresentation() = default;

//-----------------------------------------------------------------------------
#define PUSH_PROPERTY(name)                                                                        \
  {                                                                                                \
    vtkNew<vtkSMPropertyModificationUndoElement> elem;                                             \
    elem->ModifiedProperty(proxy, name);                                                           \
    ADD_UNDO_ELEM(elem);                                                                           \
  }

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::startInteraction()
{
  BEGIN_UNDO_SET(tr("Move Color Legend"));
  vtkSMProxy* proxy = this->getProxy();
  PUSH_PROPERTY("Position");
  PUSH_PROPERTY("ScalarBarLength");
  PUSH_PROPERTY("Orientation");
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::endInteraction()
{
  vtkSMProxy* proxy = this->getProxy();
  PUSH_PROPERTY("Orientation");
  PUSH_PROPERTY("ScalarBarLength");
  PUSH_PROPERTY("Position");
  END_UNDO_SET();
}
