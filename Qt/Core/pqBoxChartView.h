// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqBoxChartView_h
#define pqBoxChartView_h

#include "pqContextView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/**
 * Bar chart view
 */
class PQCORE_EXPORT pqBoxChartView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  static QString chartViewType() { return "BoxChartView"; }

  pqBoxChartView(const QString& group, const QString& name, vtkSMContextViewProxy* viewModule,
    pqServer* server, QObject* parent = nullptr);

  ~pqBoxChartView() override;

Q_SIGNALS:
  /**
   * Fired when the currently shown representation changes. \c repr may be
   * nullptr.
   */
  void showing(pqDataRepresentation* repr);

public Q_SLOTS:
  /**
   * Called when a new repr is added.
   */
  void onAddRepresentation(pqRepresentation*);
  void onRemoveRepresentation(pqRepresentation*);

protected Q_SLOTS:
  /**
   * Called to ensure that at most 1 repr is visible at a time.
   */
  void updateRepresentationVisibility(pqRepresentation* repr, bool visible);

private:
  Q_DISABLE_COPY(pqBoxChartView)
};

#endif
