// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqXYChartView.h"

#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqXYChartView::pqXYChartView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(XYChartViewType(), group, name, viewModule, server, p)
{
}

//-----------------------------------------------------------------------------
pqXYChartView::~pqXYChartView() = default;
