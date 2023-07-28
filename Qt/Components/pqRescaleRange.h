// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \file pqRescaleRange.h
 * \date 3/28/2007
 */

#ifndef pqRescaleRange_h
#define pqRescaleRange_h

#include "vtkParaViewDeprecation.h" // for deprecation

#include "pqComponentsModule.h"
#include <QDialog>

class pqRescaleRangeForm;

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "Use pqRescaleScalarRangeToCustomDialog instead") PQCOMPONENTS_EXPORT pqRescaleRange
  : public QDialog
{
  Q_OBJECT
public:
  pqRescaleRange(QWidget* parent = nullptr);
  ~pqRescaleRange() override;

  // Get the minimum of the color map range
  double minimum() const;

  // Get the maximum of the color map range
  double maximum() const;

  // Show or hide the opacity controls
  void showOpacityControls(bool show);

  // Get the minimum of the opacity map range
  double opacityMinimum() const;

  // Get the maximum of the opacity map range
  double opacityMaximum() const;

  // Initialize the range for the color map
  void setRange(double min, double max);

  // Initialize the range for a separate opacity map, if the option is available
  void setOpacityRange(double min, double max);

  bool lock() const { return this->Lock; }

protected Q_SLOTS:
  void validate();
  void rescaleAndLock();

protected: // NOLINT(readability-redundant-access-specifiers)
  pqRescaleRangeForm* Form;
  bool Lock;
};

#endif
