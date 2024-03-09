// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqEditMacrosDialog_h
#define pqEditMacrosDialog_h

#include "pqApplicationComponentsModule.h"

#include <QDialog>

#include <memory>

class QString;
class QTreeWidgetItem;

/**
 * pqEditMacrosDialog is the Edit Macros dialog used by ParaView.
 * It allows to add, edit and remove macros.
 *
 * @warning This class is built only when Python is enabled.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditMacrosDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqEditMacrosDialog(QWidget* parent = nullptr);
  ~pqEditMacrosDialog() override;

private Q_SLOTS:
  /**
   * Open "open file" dialog to add macro.
   */
  void onAddPressed();

  /**
   * Open selected item to edit.
   */
  void onEditPressed();

  /**
   * Set an icon for the selected macro.
   */
  void onSetIconPressed();

  /**
   * Remove selected item from macros tree.
   */
  void onRemovePressed();

  /**
   * Remove all items from macros tree.
   */
  void onRemoveAllPressed();

  /**
   * Update macros visibility according the search pattern.
   */
  void onSearchTextChanged(const QString& pattern);

private: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Return true if the macros tree contains at least one item.
   */
  bool treeHasItems();

  /**
   * Return true if at least one item is selected in the macros tree.
   */
  bool treeHasSelectedItems();

  /**
   * Return the first selected item in the macros tree.
   * If no selection, return the invisibleRootItem.
   */
  QModelIndex getSelectedItem();

  /**
   * Return the nearest item.
   * This is, in order of choose:
   *  * next sibling
   *  * previous sibling
   */
  QModelIndex getNearestItem(const QModelIndex& index);

  /**
   * Delete item and remove associated macro.
   * Do not use item after!
   */
  void deleteItem(const QModelIndex& item);

  /**
   * Delete items and update current item.
   */
  void deleteItems(const QList<QModelIndex>& items);

  /**
   * Update UI enabled state.
   */
  void updateUIState();

  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
