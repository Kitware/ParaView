// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPlotMatrixView_h
#define pqPlotMatrixView_h

#include "pqContextView.h"

class PQCORE_EXPORT pqPlotMatrixView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  pqPlotMatrixView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);
  ~pqPlotMatrixView() override;

  static QString viewType() { return "PlotMatrixView"; }

protected:
};

#endif
