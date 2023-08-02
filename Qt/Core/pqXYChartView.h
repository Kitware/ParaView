// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqXYChartView_h
#define pqXYChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
 * pqContextView subclass for "Line Chart View". Doesn't do much expect adds
 * the API to get the chartview type and name.
 */
class PQCORE_EXPORT pqXYChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString XYChartViewType() { return "XYChartView"; }

  pqXYChartView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);

  ~pqXYChartView() override;

private:
  Q_DISABLE_COPY(pqXYChartView)
};

#endif
