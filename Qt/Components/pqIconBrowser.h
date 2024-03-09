// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqIconBrowser_h
#define pqIconBrowser_h

#include <QDialog>

#include "pqComponentsModule.h"

#include <QString>

#include <memory>

/**
 * pqIconBrowser is a dialog to browse available icons.
 * Available icons includes icons provided by the application
 * but also user defined ones.
 * This dialog allows to import and remove user defined icons.
 */
class PQCOMPONENTS_EXPORT pqIconBrowser : public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT;

public:
  pqIconBrowser(QWidget* parent);
  ~pqIconBrowser() override;

  /**
   * Returns the absolute path to the selected icon.
   * May be empty if no icon were selected.
   * Create a dialog window.
   * Argument defaultPath is used if dialog is rejected.
   */
  static QString getIconPath(const QString& defaultPath = QString());

  /**
   * Returns the absolute path of the selected icon.
   */
  QString getSelectedIconPath();

private:
  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;
};

#endif // pqIconBrowser_h
