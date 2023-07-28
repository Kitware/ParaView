// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqLinksManager_h
#define pqLinksManager_h

#include <QDialog>
#include <QModelIndex>
#include <QScopedPointer>

#include "pqComponentsModule.h"

namespace Ui
{
class pqLinksManager;
}
/**
 * dialog for viewing, creating, editing, removing proxy/property/camera links
 */
class PQCOMPONENTS_EXPORT pqLinksManager : public QDialog
{
  Q_OBJECT
  typedef QDialog base;

public:
  /**
   * create this dialog with a parent
   */
  pqLinksManager(QWidget* p = nullptr);
  /**
   * destroy this dialog
   */
  ~pqLinksManager() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * add a link
   */
  void addLink();
  /**
   * edit the currently selected link
   */
  void editLink();
  /**
   * edit the currently selected link
   */
  void removeLink();

private Q_SLOTS:
  void selectionChanged(const QModelIndex& idx);

private: // NOLINT(readability-redundant-access-specifiers)
  QScopedPointer<Ui::pqLinksManager> Ui;
};

#endif
