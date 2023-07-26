// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqParallelCoordinatesChartView.h"

#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartView::pqParallelCoordinatesChartView(const QString& group,
  const QString& name, vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(chartViewType(), group, name, viewModule, server, p)
{
}

//-----------------------------------------------------------------------------
pqParallelCoordinatesChartView::~pqParallelCoordinatesChartView() = default;
