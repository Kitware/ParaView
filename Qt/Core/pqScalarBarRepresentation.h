// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScalarBarRepresentation_h
#define pqScalarBarRepresentation_h

#include "pqRepresentation.h"

class pqPipelineRepresentation;
class pqScalarsToColors;
class vtkUndoElement;

/**
 * pqScalarBarRepresentation is created for "ScalarBarWidgetRepresentation"
 * proxies. The only reason why pqScalarBarRepresentation is used is to keep
 * create undo elements to aid with undo/redo for scalar bar interaction i.e.
 * if user drags the scalar-bar widget, we capture the entire operation in a
 * single undo-able action.
 */
class PQCORE_EXPORT pqScalarBarRepresentation : public pqRepresentation
{
  Q_OBJECT
  typedef pqRepresentation Superclass;

public:
  pqScalarBarRepresentation(const QString& group, const QString& name, vtkSMProxy* scalarbar,
    pqServer* server, QObject* parent = nullptr);
  ~pqScalarBarRepresentation() override;

protected Q_SLOTS:
  void startInteraction();
  void endInteraction();
};
#endif
