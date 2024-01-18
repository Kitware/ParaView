// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqRescaleScalarRangeToDataOverTimeDialog_h
#define pqRescaleScalarRangeToDataOverTimeDialog_h

#include "pqComponentsModule.h"

#include <QDialog>
#include <memory>

class pqRescaleScalarRangeToDataOverTimeDialogForm;

/**
 * pqRescaleScalarRangeToDataOverTimeDialog provides a dialog to be able to
 * rescale the active lookup table's range to data range over time.
 */
class PQCOMPONENTS_EXPORT pqRescaleScalarRangeToDataOverTimeDialog : public QDialog
{
  Q_OBJECT
public:
  pqRescaleScalarRangeToDataOverTimeDialog(QWidget* parent = nullptr);
  ~pqRescaleScalarRangeToDataOverTimeDialog() override;

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
   * Emit apply and close the dialog.
   */
  void rescale();

protected: // NOLINT(readability-redundant-access-specifiers)
  std::unique_ptr<pqRescaleScalarRangeToDataOverTimeDialogForm> Form;
};

#endif
