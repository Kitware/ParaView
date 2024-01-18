// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqRescaleScalarRangeCustomDialog_h
#define pqRescaleScalarRangeCustomDialog_h

#include "pqComponentsModule.h"

#include <QDialog>
#include <memory>

class pqRescaleScalarRangeToCustomDialogForm;

/**
 * pqRescaleScalarRangeToCustomDialog provides a dialog to be able to
 * rescale the active lookup table's range to a custom range.
 */
class PQCOMPONENTS_EXPORT pqRescaleScalarRangeToCustomDialog : public QDialog
{
  Q_OBJECT
public:
  pqRescaleScalarRangeToCustomDialog(QWidget* parent = nullptr);
  ~pqRescaleScalarRangeToCustomDialog() override;

  /**
   * Get the minimum of the color map range
   */
  double minimum() const;

  /**
   * Get the maximum of the color map range
   */
  double maximum() const;

  /**
   * Show or hide the opacity controls
   */
  void showOpacityControls(bool show);

  /**
   * Get the minimum of the opacity map range
   */
  double opacityMinimum() const;

  /**
   * Get the maximum of the opacity map range
   */
  double opacityMaximum() const;

  /**
   * Initialize the range for the color map
   */
  void setRange(double min, double max);

  /**
   * Initialize the range for a separate opacity map,
   * if the option is available
   */
  void setOpacityRange(double min, double max);

  /**
   * Initialize AutomaticRescaling checkbox value.
   */
  void setLock(bool lock);

  /**
   * Get lock value from AutomaticRescaling checkbox.
   */
  bool doLock() const;

Q_SIGNALS:
  /**
   * Fired when the user wants to apply his changes.
   */
  void apply();

protected Q_SLOTS:
  /**
   * Check that minimum scalar value < maximum scalar value.
   * Deactivate apply and rescale button if that's not the case.
   */
  void validate();

  /**
   * Emit apply and close the dialog.
   */
  void rescale();

protected: // NOLINT(readability-redundant-access-specifiers)
  std::unique_ptr<pqRescaleScalarRangeToCustomDialogForm> Form;
};

#endif
