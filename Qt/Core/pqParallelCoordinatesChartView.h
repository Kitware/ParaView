// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqParallelCoordinatesChartView_h
#define pqParallelCoordinatesChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
 * pqView subclass of ParallelCoordinatesChartView chart view. Does not do
 * anything specific besides passing the view type of pqView in the
 * constructor.
 */
class PQCORE_EXPORT pqParallelCoordinatesChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString chartViewType() { return "ParallelCoordinatesChartView"; }

  pqParallelCoordinatesChartView(const QString& group, const QString& name,
    vtkSMContextViewProxy* viewModule, pqServer* server, QObject* parent = nullptr);
  ~pqParallelCoordinatesChartView() override;

private:
  Q_DISABLE_COPY(pqParallelCoordinatesChartView)
};

#endif
