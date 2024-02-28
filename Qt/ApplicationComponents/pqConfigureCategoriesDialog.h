// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqConfigureCategoriesDialog_h
#define pqConfigureCategoriesDialog_h

#include "pqApplicationComponentsModule.h"
#include <QDialog>
#include <QPointer>

#include <memory>

class pqProxyCategory;
class pqProxyGroupMenuManager;
class pqProxyInfo;

class QString;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * pqConfigureCategoriesDialog is the Configure Categories dialog used by ParaView.
 * It allows to create and remove categories and organize filters in them.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqConfigureCategoriesDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  pqConfigureCategoriesDialog(pqProxyGroupMenuManager* manager, QWidget* parent = nullptr);
  ~pqConfigureCategoriesDialog() override;

protected Q_SLOTS:
  /**
   * Create a category at the top level of the tree.
   */
  void createNewCategory();

  /**
   * Add selected item from the default configuration tree to
   * the top level of the tree.
   */
  void onAddPressed();

  /**
   * Remove selected item (filters/categories) from custom tree.
   * When removing a category, its children are removed too.
   */
  void onRemovePressed();

  /**
   * Reset the custom tree to match the application default tree.
   */
  void resetToApplicationCategories();

  /**
   * Set an icon for the selected filter.
   */
  void onSetIconPressed();

  /**
   * Save current categories tree and exit.
   */
  void onAccepted();

  /**
   * Update filters visibility according the search pattern.
   */
  void onSearchTextChanged(const QString& pattern);

  /**
   * Ensure category name is unique when item text is changed.
   */
  void onItemChanged(QTreeWidgetItem*, int);

  /**
   * Update category toolbar property from checkbox state.
   */
  void updateToolbarState();

private:
  /**
   * Update trees.
   */
  ///@{
  /// Populate the custom tree from the categories settings.
  void populateCurrentCategoriesTree();
  /// Populate the filters tree from the default configuration.
  void populateAvailableProxiesTree();
  /// Add children to treeRoot given parentCategory children.
  void populateTree(QTreeWidgetItem* treeRoot);
  /// Create a Proxy item, set data and flags
  QTreeWidgetItem* createProxyItem(
    QTreeWidgetItem* parent, pqProxyInfo* info, QTreeWidgetItem* preceding = nullptr);
  /// Create a Category item, set data and flags
  QTreeWidgetItem* createCategoryItem(
    QTreeWidgetItem* parent, pqProxyCategory* info, QTreeWidgetItem* preceding = nullptr);
  /// Create a generic item.
  QTreeWidgetItem* createItem(
    QTreeWidgetItem* parent, const QString& name, QTreeWidgetItem* preceding = nullptr);
  /// Create a category from a name and insert it given parent and preceding item.
  QTreeWidgetItem* createCategory(
    const QString& name, QTreeWidgetItem* parentItem, QTreeWidgetItem* precedingItem);
  /// Copy category contained in sourceItem and insert it given parent and preceding item. Return
  /// true on success.
  bool insertCategoryFromItem(
    QTreeWidgetItem* sourceItem, QTreeWidgetItem* parentItem, QTreeWidgetItem* precedingItem);
  /// Copy a proxy contained in sourceItem and insert it given parent and preceding item. Return
  /// true on success.
  bool insertProxyFromItem(
    QTreeWidgetItem* sourceItem, QTreeWidgetItem* parentItem, QTreeWidgetItem* precedingItem);
  ///@}

  /**
   * Item Selection
   */
  ///@{
  /**
   * Return true if at least on item is selected in the custom tree.
   */
  bool customTreeHasSelection();
  /**
   * Return true if at least on item is selected in the application tree.
   */
  bool applicationTreeHasSelection();
  /**
   * Return the first selected item in the custom tree.
   * If no selection, return the invisibleRootItem.
   */
  QTreeWidgetItem* getSelectedItem();
  /**
   * Return the nearest item.
   * This is, in order of choose:
   *  * next sibling
   *  * previous sibling
   *  * parent category
   */
  QTreeWidgetItem* getNearestItem(QTreeWidgetItem* item);
  /**
   * Return the current category item (from first selected item)
   * This can be:
   * - the invisible root item if nothing is selected
   * - the parent category if a proxy item is selected
   * - the selected item itself otherwise
   */
  QTreeWidgetItem* getSelectedCategoryItem();
  /**
   * Return the current proxy item or nullptr.
   */
  QTreeWidgetItem* getSelectedProxyItem();
  ///@}

  /**
   * Handle drop event to avoid proxy duplication, keep item expansion.
   */
  bool eventFilter(QObject* object, QEvent* event) override;

  void keyPressEvent(QKeyEvent* event) override;

  /**
   * Update UI enabled state.
   */
  void updateUIState();

  /**
   * Delete item and update categories.
   * Do not use item after!
   */
  void deleteItem(QTreeWidgetItem* item);
  /**
   * Delete items and update current item
   */
  void deleteCustomItems(QList<QTreeWidgetItem*> items);

  /**
   * Extract info from QDropEvent
   */
  ///@{
  /// Get the item in customTree that should be the parent of the dropped item.
  QTreeWidgetItem* getDestinationParentItem(QDropEvent* dropEvent);
  /// Get the destination item.
  QTreeWidgetItem* getDestinationItem(QDropEvent* dropEvent);
  /// Get the source item. We have a single-selection mode.
  QTreeWidgetItem* getSourceItem(QDropEvent* dropEvent);
  /// Return true if the source tree is the custom tree.
  bool sourceIsCustomTree(QDropEvent* dropEvent);
  ///@}

  struct pqInternal;
  std::unique_ptr<pqInternal> Internal;

  QString ResourceTag = "ParaViewFilters";
};

#endif
