// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPlotMatrixView.h"

#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqPlotMatrixView::pqPlotMatrixView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* parentObj)
  : pqContextView(viewType(), group, name, viewModule, server, parentObj)
{
}

//-----------------------------------------------------------------------------
pqPlotMatrixView::~pqPlotMatrixView() = default;
