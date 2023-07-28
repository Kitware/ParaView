// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDoubleRangeDialog_h
#define pqDoubleRangeDialog_h

#include "pqWidgetsModule.h"

#include <QDialog>

class pqDoubleRangeWidget;

/**
 * Provides a dialog for specifying a double between two ranges
 */
class PQWIDGETS_EXPORT pqDoubleRangeDialog : public QDialog
{
  Q_OBJECT

public:
  pqDoubleRangeDialog(
    const QString& label, double minimum, double maximum, QWidget* parent = nullptr);
  ~pqDoubleRangeDialog() override;

  void setValue(double value);
  double value() const;

private:
  pqDoubleRangeWidget* Widget;
};

#endif // !pqDoubleRangeDialog_h
