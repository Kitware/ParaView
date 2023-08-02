// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqXYBarChartView_h
#define pqXYBarChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
 * pqContextView subclass for "Bar Chart View". Doesn't do much expect adds
 * the API to get the chartview type and name.
 */
class PQCORE_EXPORT pqXYBarChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString XYBarChartViewType() { return "XYBarChartView"; }

  /**
   * Currently the bar chart view is not supporting selection.
   */
  bool supportsSelection() const override { return false; }

  pqXYBarChartView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);

  ~pqXYBarChartView() override;

private:
  Q_DISABLE_COPY(pqXYBarChartView)
};

#endif
