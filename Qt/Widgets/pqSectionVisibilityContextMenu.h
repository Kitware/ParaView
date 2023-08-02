// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSectionVisibilityContextMenu_h
#define pqSectionVisibilityContextMenu_h

// In case of QTableView, or any other spreadsheet-like view, the user
// should be provided an option to toggle the visibility of
// columns/rows. pqSectionVisibilityContextMenu is a context menu
// that can be used for the same.

#include "pqWidgetsModule.h"
#include <QHeaderView>
#include <QMenu>
#include <QPointer>

class PQWIDGETS_EXPORT pqSectionVisibilityContextMenu : public QMenu
{
  Q_OBJECT
public:
  pqSectionVisibilityContextMenu(QWidget* parent = nullptr);
  ~pqSectionVisibilityContextMenu() override;

  // Set the QHeaderView whose section visibility is affected by
  // this menu. This leads to clearing of any actions
  // already present in the menu and populating the menu
  // with the section headings from the header.
  // This must be set before calling exec().
  void setHeaderView(QHeaderView* header);
  QHeaderView* headerView() { return this->HeaderView; }

protected Q_SLOTS:
  void toggleSectionVisibility(QAction* action);

protected: // NOLINT(readability-redundant-access-specifiers)
  QPointer<QHeaderView> HeaderView;
};

#endif
