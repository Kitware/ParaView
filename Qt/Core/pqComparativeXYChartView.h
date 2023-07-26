// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqComparativeXYChartView_h
#define pqComparativeXYChartView_h

#include "pqComparativeContextView.h"

/**
 * The comparative line chart subclass.
 */
class PQCORE_EXPORT pqComparativeXYChartView : public pqComparativeContextView
{
  Q_OBJECT
  typedef pqComparativeContextView Superclass;

public:
  pqComparativeXYChartView(const QString& group, const QString& name,
    vtkSMComparativeViewProxy* view, pqServer* server, QObject* parent = nullptr);
  ~pqComparativeXYChartView() override;

  static QString chartViewType() { return "ComparativeXYChartView"; }
  static QString chartViewTypeName() { return "Line Chart View (Comparative)"; }

private:
  Q_DISABLE_COPY(pqComparativeXYChartView)
};

#endif
