// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLockViewSizeCustomDialog_h
#define pqLockViewSizeCustomDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

/**
 * Dialog used to ask the user what resolution to lock the views to.
 */
class PQCOMPONENTS_EXPORT pqLockViewSizeCustomDialog : public QDialog
{
  Q_OBJECT;
  typedef QDialog Superclass;

public:
  pqLockViewSizeCustomDialog(QWidget* parent, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqLockViewSizeCustomDialog() override;

  /**
   * The custom resolution currently entered by the user.
   */
  QSize customResolution() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Sets the view size to the displayed resolution.
   */
  virtual void apply();

  /**
   * Applies the resolution and accepts the dialog.
   */
  void accept() override;

  /**
   * Unlocks the size on the view.
   */
  virtual void unlock();

private:
  Q_DISABLE_COPY(pqLockViewSizeCustomDialog)

  class pqUI;
  pqUI* ui;
};

#endif // pqLockViewSizeCustomDialog_h
