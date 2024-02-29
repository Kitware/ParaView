// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSeriesGeneratorDialog_h
#define pqSeriesGeneratorDialog_h

#include "pqWidgetsModule.h" // for exports
#include <QDialog>
#include <QScopedPointer> // for ivar

/**
 * @class pqSeriesGeneratorDialog
 * @brief dialog to generate a number series
 *
 * pqSeriesGeneratorDialog is a simple dialog that lets the user generate a
 * series of numbers. Multiple series are supported including linear,
 * logarithmic and geometric.
 *
 */
class PQWIDGETS_EXPORT pqSeriesGeneratorDialog : public QDialog
{
  Q_OBJECT
  using Superclass = QDialog;

public:
  pqSeriesGeneratorDialog(
    double min, double max, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~pqSeriesGeneratorDialog() override;

  /**
   * Returns the generated number series.
   */
  QVector<double> series() const;

  /**
   * Set the dataMin and dataMax on the dialog
   * They will be used only if reset is true
   * or if resetRangeToDataRange is called.
   */
  void setDataRange(double dataMin, double dataMax, bool reset = false);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * Reset the range to the data range
   */
  void resetRangeToDataRange();

private:
  Q_DISABLE_COPY(pqSeriesGeneratorDialog);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
