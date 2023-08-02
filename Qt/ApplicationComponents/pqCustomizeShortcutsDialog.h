// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomizeShortcutsDialog_h
#define pqCustomizeShortcutsDialog_h

#include <QDialog>

class pqCustomizeShortcutsDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqCustomizeShortcutsDialog(QWidget* p = nullptr);
  ~pqCustomizeShortcutsDialog() override;

  static QString getActionName(QAction* action);

private Q_SLOTS:
  void onEditingFinished();
  void onSelectionChanged();
  void onClearShortcut();
  void onResetShortcut();
  void onResetAll();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCustomizeShortcutsDialog);

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
