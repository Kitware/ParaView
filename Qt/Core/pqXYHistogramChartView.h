// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqXYHistogramChartView_h
#define pqXYHistogramChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
 * pqContextView subclass for "HistogramView". Doesn't do much expect adds
 * the API to get the chartview type and indicates that this view supports
 * selection.
 */
class PQCORE_EXPORT pqXYHistogramChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString XYHistogramChartViewType() { return "XYHistogramChartView"; }

  /**
   * Currently the bar chart view is not supporting selection.
   */
  bool supportsSelection() const override { return true; }

  pqXYHistogramChartView(const QString& group, const QString& name,
    vtkSMContextViewProxy* viewModule, pqServer* server, QObject* parent = nullptr);

  ~pqXYHistogramChartView() override;

private:
  Q_DISABLE_COPY(pqXYHistogramChartView)
};

#endif
