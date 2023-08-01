// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqComparativeXYChartView.h"

//-----------------------------------------------------------------------------
pqComparativeXYChartView::pqComparativeXYChartView(const QString& group, const QString& name,
  vtkSMComparativeViewProxy* view, pqServer* server, QObject* parentObject)
  : Superclass(chartViewType(), group, name, view, server, parentObject)
{
}

//-----------------------------------------------------------------------------
pqComparativeXYChartView::~pqComparativeXYChartView() = default;
