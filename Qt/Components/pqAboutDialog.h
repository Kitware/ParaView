// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAboutDialog_h
#define pqAboutDialog_h

#include "pqComponentsModule.h"
#include <QDialog>

namespace Ui
{
class pqAboutDialog;
}

class pqServer;
class QTreeWidget;

/**
 * pqAboutDialog is the about dialog used by ParaView.
 * It provides information about ParaView and current configuration.
 */
class PQCOMPONENTS_EXPORT pqAboutDialog : public QDialog
{
  Q_OBJECT

public:
  pqAboutDialog(QWidget* Parent);
  ~pqAboutDialog() override;

  /**
   * Format the about dialog content into textual form
   */
  QString formatToText();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * Saves about dialog formatted output to a file.
   */
  void saveToFile();

  /**
   * Copy about dialog formatted output to the clipboard.
   */
  void copyToClipboard();

protected:
  void AddClientInformation();
  void AddServerInformation();
  void AddServerInformation(pqServer* server, QTreeWidget* tree);
  QString formatToText(QTreeWidget* tree);

private:
  Q_DISABLE_COPY(pqAboutDialog)
  Ui::pqAboutDialog* const Ui;
};

#endif // !pqAboutDialog_h
