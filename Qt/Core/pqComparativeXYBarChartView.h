// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComparativeXYBarChartView_h
#define pqComparativeXYBarChartView_h

#include "pqComparativeContextView.h"

/**
 * The comparative bar chart subclass.
 */
class PQCORE_EXPORT pqComparativeXYBarChartView : public pqComparativeContextView
{
  Q_OBJECT
  typedef pqComparativeContextView Superclass;

public:
  pqComparativeXYBarChartView(const QString& group, const QString& name,
    vtkSMComparativeViewProxy* view, pqServer* server, QObject* parent = nullptr);
  ~pqComparativeXYBarChartView() override;

  static QString chartViewType() { return "ComparativeXYBarChartView"; }
  static QString chartViewTypeName() { return "Bar Chart View (Comparative)"; }

private:
  Q_DISABLE_COPY(pqComparativeXYBarChartView)
};

#endif
