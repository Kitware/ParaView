// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqXYBarChartView.h"

#include "vtkSMContextViewProxy.h"

//-----------------------------------------------------------------------------
pqXYBarChartView::pqXYBarChartView(const QString& group, const QString& name,
  vtkSMContextViewProxy* viewModule, pqServer* server, QObject* p /*=nullptr*/)
  : Superclass(XYBarChartViewType(), group, name, viewModule, server, p)
{
}

//-----------------------------------------------------------------------------
pqXYBarChartView::~pqXYBarChartView() = default;
