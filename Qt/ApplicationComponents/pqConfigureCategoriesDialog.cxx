// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqConfigureCategoriesDialog.h"
#include "ui_pqConfigureCategoriesDialog.h"

// Qt
#include <QAction>
#include <QDropEvent>
#include <QHeaderView>
#include <QRegularExpression>
#include <QSequentialIterable>
#include <QSet>
#include <QShortcut>
#include <QStringList>
#include <QTreeWidgetItem>

#include "pqIconBrowser.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"
#include "pqProxyInfo.h"

namespace
{
static constexpr int PROXY_ROLE = Qt::UserRole + 1;
static constexpr int CATEGORY_ROLE = Qt::UserRole + 2;
static QString DEFAULT_NAME = "New Category";

//----------------------------------------------------------------------------
/**
 * Return parent item.
 * As Qt choose to return nullptr for top level items,
 * explicitly fallback to invisibleRootItem in that case.
 *
 * Not described in Qt doc, but found some forum confirmation, like here
 * https://www.qtcentre.org/threads/40149-Determining-QTreeWidget-parent?s=ed6e7f2f45f2accd6237382b2e768e88&p=184437#post184437
 */
QTreeWidgetItem* getParent(QTreeWidgetItem* item)
{
  auto parent = item->parent();
  if (!parent)
  {
    parent = item->treeWidget()->invisibleRootItem();
  }

  return parent;
}

//----------------------------------------------------------------------------
/**
 * Return true if item contains a pqProxyCategory.
 */
bool isCategory(QTreeWidgetItem* item)
{
  auto storedCategory = item->data(0, CATEGORY_ROLE);
  return storedCategory.isValid();
}

//----------------------------------------------------------------------------
/**
 * Return the pqProxyInfo stored in the item.
 */
pqProxyInfo* getProxy(QTreeWidgetItem* item)
{
  return item->data(0, PROXY_ROLE).value<pqProxyInfo*>();
}

//----------------------------------------------------------------------------
/**
 * Return the pqProxyCategory stored in the item.
 */
pqProxyCategory* getCategory(QTreeWidgetItem* item)
{
  return item->data(0, CATEGORY_ROLE).value<pqProxyCategory*>();
}

//----------------------------------------------------------------------------
/**
 * Return true if item has one of its ancestor in the list.
 */
bool hasAncestorInList(QTreeWidgetItem* item, QList<QTreeWidgetItem*> candidates)
{
  if (!item->parent())
  {
    return false;
  }

  if (candidates.contains(item->parent()))
  {
    return true;
  }

  return hasAncestorInList(item->parent(), candidates);
}

} // end of anonymous namespace

struct pqConfigureCategoriesDialog::pqInternal
{
  pqInternal(pqProxyGroupMenuManager* manager)
    : MenuManager(manager)
    , Ui(new Ui::pqConfigureCategoriesDialog())
    , SettingsCategory(new pqProxyCategory())
  {
  }

  //----------------------------------------------------------------------------
  bool isFavorites(pqProxyCategory* category) { return this->MenuManager->isFavorites(category); }

  //----------------------------------------------------------------------------
  bool isAvailable(pqProxyInfo* proxyInfo)
  {
    auto action = this->MenuManager->getAction(proxyInfo);
    return action != nullptr;
  }

  //----------------------------------------------------------------------------
  /**
   * Set a unique name from current item text.
   * Avoid empty name.
   * Modify display text and update the underlying category.
   */
  void ensureValidCategoryName(QTreeWidgetItem* item)
  {
    if (this->BlockEdition)
    {
      return;
    }

    auto category = getCategory(item);
    if (!category)
    {
      return;
    }

    const QString& itemText = item->text(0);

    QString newName = itemText.isEmpty() ? ::DEFAULT_NAME : itemText;
    QString newLabel = category->parentCategory()->makeUniqueCategoryLabel(newName);
    newName = category->parentCategory()->makeUniqueCategoryName(newName);

    bool blocked = this->BlockEdition;
    this->BlockEdition = true;
    if (newName != category->name())
    {
      category->rename(newName);
    }
    if (newLabel != category->label())
    {
      item->setText(0, newLabel);
      category->updateLabel(newLabel);
    }
    this->BlockEdition = blocked;
  }

  /**
   * Set an icon to the proxy stored under the given item.
   */
  void setIcon(QTreeWidgetItem* item, const QString& iconPath)
  {
    auto proxyInfo = ::getProxy(item);
    if (!proxyInfo)
    {
      return;
    }

    proxyInfo->setIcon(iconPath);
    this->MenuManager->updateActionIcon(proxyInfo);
    item->setIcon(0, QIcon(iconPath));
  }

  /**
   * Update the icon of the given item, based on its internal proxy info.
   * Also make it grey if there is no associated action.
   *
   * This means that the underlying filter is not available in the proxy manager.
   * This is usually the case for not-yet-loaded plugins.
   */
  void updateItemDisplay(QTreeWidgetItem* item)
  {
    auto proxyInfo = ::getProxy(item);

    if (!this->isAvailable(proxyInfo))
    {
      auto font = item->font(0);
      item->setForeground(0, QBrush(QColor(200, 200, 200)));
      item->setToolTip(
        0, tr("Filter is not available.\nIt may be part of a plugin that is not loaded."));
    }

    // use the same icon as the associated menu action.
    auto action = this->MenuManager->getAction(proxyInfo);
    if (action && !action->icon().isNull())
    {
      item->setIcon(0, QIcon(action->icon()));
    }
  }

  bool BlockEdition = false;
  pqProxyGroupMenuManager* MenuManager;
  QScopedPointer<Ui::pqConfigureCategoriesDialog> Ui;
  std::unique_ptr<pqProxyCategory> SettingsCategory;
};

//----------------------------------------------------------------------------
pqConfigureCategoriesDialog::pqConfigureCategoriesDialog(
  pqProxyGroupMenuManager* manager, QWidget* parent)
  : Superclass(parent)
  , Internal(new pqInternal(manager))
{
  this->Internal->Ui->setupUi(this);
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  this->Internal->Ui->defaultCategoriesTree->setAcceptDrops(false);
  this->Internal->Ui->defaultCategoriesTree->sortByColumn(0, Qt::AscendingOrder);
  this->Internal->Ui->defaultCategoriesTree->setHeaderLabels(QStringList() << tr("Name"));

  this->Internal->Ui->customCategoriesTree->setHeaderLabels(QStringList() << tr("Name"));
  this->Internal->Ui->customCategoriesTree->viewport()->installEventFilter(this);

  QObject::connect(this->Internal->Ui->addCategory, &QToolButton::released, this,
    [&]()
    {
      auto parentItem = this->Internal->Ui->customCategoriesTree->invisibleRootItem();
      auto precedingItem = parentItem;
      this->createCategory(::DEFAULT_NAME, parentItem, precedingItem);
    });
  QObject::connect(this->Internal->Ui->addSubCategory, &QToolButton::released, this,
    [&]()
    {
      QTreeWidgetItem* parentItem = this->getSelectedCategoryItem();
      QTreeWidgetItem* precedingItem = this->getSelectedItem();
      this->createCategory(::DEFAULT_NAME, parentItem, precedingItem);
    });

  QObject::connect(this->Internal->Ui->setIcon, &QToolButton::released, this,
    &pqConfigureCategoriesDialog::onSetIconPressed);

  QObject::connect(this->Internal->Ui->remove, &QToolButton::released, this,
    &pqConfigureCategoriesDialog::onRemovePressed);
  QShortcut* shortcut =
    new QShortcut(QKeySequence(Qt::Key_Delete), this->Internal->Ui->customCategoriesTree);
  QObject::connect(
    shortcut, &QShortcut::activated, this, &pqConfigureCategoriesDialog::onRemovePressed);
  QObject::connect(this->Internal->Ui->resetAll, &QToolButton::released, this,
    &pqConfigureCategoriesDialog::resetToApplicationCategories);
  QObject::connect(this->Internal->Ui->addFilter, &QToolButton::released, this,
    &pqConfigureCategoriesDialog::onAddPressed);

  QObject::connect(this->Internal->Ui->searchBox, &pqSearchBox::textChanged, this,
    &pqConfigureCategoriesDialog::onSearchTextChanged);
  QObject::connect(this->Internal->Ui->useAsToolbar, &QCheckBox::clicked, this,
    &pqConfigureCategoriesDialog::updateToolbarState);

  this->connect(this, &QDialog::accepted, this, &pqConfigureCategoriesDialog::onAccepted);

  this->populateAvailableProxiesTree();
  this->Internal->Ui->defaultCategoriesTree->resizeColumnToContents(0);

  this->Internal->SettingsCategory->deepCopy(this->Internal->MenuManager->getMenuCategory());
  this->populateCurrentCategoriesTree();
  QObject::connect(this->Internal->Ui->customCategoriesTree, &pqFavoritesTreeWidget::itemChanged,
    this, &pqConfigureCategoriesDialog::onItemChanged);

  this->connect(this->Internal->Ui->defaultCategoriesTree, &QTreeWidget::itemSelectionChanged, this,
    &pqConfigureCategoriesDialog::updateUIState);
  this->connect(this->Internal->Ui->customCategoriesTree, &QTreeWidget::itemSelectionChanged, this,
    &pqConfigureCategoriesDialog::updateUIState);
  this->updateUIState();
}

//----------------------------------------------------------------------------
pqConfigureCategoriesDialog::~pqConfigureCategoriesDialog() = default;

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onSearchTextChanged(const QString& pattern)
{
  auto* root = this->Internal->Ui->defaultCategoriesTree->invisibleRootItem();
  for (int i = 0; i < root->childCount(); i++)
  {
    auto* item = root->child(i);
    if (pattern == "")
    {
      item->setHidden(false);
    }
    else
    {
      item->setHidden(!item->text(0).contains(pattern, Qt::CaseInsensitive));
    }
  }
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Drop)
  {
    auto dropEvent = static_cast<QDropEvent*>(event);

    auto destItem = this->getDestinationItem(dropEvent);
    auto sourceItem = this->getSourceItem(dropEvent);
    if (!destItem && !::isCategory(sourceItem))
    {
      return true;
    }

    auto parentCategoryItem = getDestinationParentItem(dropEvent);
    if (!parentCategoryItem)
    {
      return true;
    }

    bool inserted = false;
    if (::isCategory(sourceItem))
    {
      inserted = this->insertCategoryFromItem(sourceItem, parentCategoryItem, destItem);
    }
    else
    {
      inserted = this->insertProxyFromItem(sourceItem, parentCategoryItem, destItem);
    }

    if (inserted && this->sourceIsCustomTree(dropEvent))
    {
      this->deleteCustomItems(QList<QTreeWidgetItem*>() << sourceItem);
    }

    return true;
  }

  if (event->type() == QEvent::MouseButtonRelease)
  {
    auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
    auto itemClicked = this->Internal->Ui->customCategoriesTree->indexAt(mouseEvent->pos());
    if (!itemClicked.isValid())
    {
      this->Internal->Ui->customCategoriesTree->clearSelection();
    }
  }

  return QDialog::eventFilter(object, event);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::keyPressEvent(QKeyEvent* keyEvent)
{
  if (keyEvent->matches(QKeySequence::Cancel))
  {
    this->Internal->Ui->customCategoriesTree->clearSelection();
    return;
  }

  if (keyEvent->key() == Qt::Key_Return)
  {
    return;
  }

  this->Superclass::keyReleaseEvent(keyEvent);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::populateCurrentCategoriesTree()
{
  this->Internal->Ui->customCategoriesTree->clear();
  auto treeRoot = this->Internal->Ui->customCategoriesTree->invisibleRootItem();
  auto mainCategory = this->Internal->SettingsCategory.get();

  treeRoot->setData(0, ::CATEGORY_ROLE, QVariant::fromValue<pqProxyCategory*>(mainCategory));
  for (auto subCat : mainCategory->getCategoriesAlphabetically())
  {
    auto catItem = this->createCategoryItem(treeRoot, subCat);
    this->populateTree(catItem);
  }

  this->Internal->Ui->customCategoriesTree->resizeColumnToContents(0);
  this->Internal->Ui->customCategoriesTree->setCurrentItem(nullptr);
  this->Internal->Ui->customCategoriesTree->collapseAll();
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::populateAvailableProxiesTree()
{
  this->Internal->Ui->defaultCategoriesTree->clear();

  pqProxyCategory* applicationCategory = this->Internal->MenuManager->getApplicationCategory();
  for (auto proxy : applicationCategory->getProxiesRecursive())
  {
    this->createProxyItem(this->Internal->Ui->defaultCategoriesTree->invisibleRootItem(), proxy);
  }

  this->Internal->Ui->defaultCategoriesTree->sortByColumn(0, Qt::AscendingOrder);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::populateTree(QTreeWidgetItem* treeRoot)
{
  auto parentCategory = ::getCategory(treeRoot);
  if (!parentCategory)
  {
    return;
  }

  for (auto proxy : parentCategory->getRootProxies())
  {
    if (!proxy->hideFromMenu())
    {
      this->createProxyItem(treeRoot, proxy);
    }
  }

  for (auto subCat : parentCategory->getCategoriesAlphabetically())
  {
    auto catItem = this->createCategoryItem(treeRoot, subCat);
    this->populateTree(catItem);
  }
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onItemChanged(QTreeWidgetItem* item, int)
{
  if (::isCategory(item))
  {
    // Ensure unicity of categories
    this->Internal->ensureValidCategoryName(item);
  }
  else
  {
    // ensure that item that are not categories have drop disabled
    // this occurs when dropping item from a tree to another.
    if (item->flags() & Qt::ItemIsDropEnabled)
    {
      item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
    }
  }

  this->Internal->Ui->customCategoriesTree->scrollToItem(item);
  this->Internal->Ui->customCategoriesTree->setCurrentItem(item);
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::customTreeHasSelection()
{
  return !this->Internal->Ui->customCategoriesTree->selectedItems().isEmpty();
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::applicationTreeHasSelection()
{
  return !this->Internal->Ui->defaultCategoriesTree->selectedItems().isEmpty();
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getSelectedItem()
{
  if (this->customTreeHasSelection())
  {
    return this->Internal->Ui->customCategoriesTree->selectedItems().first();
  }

  return this->Internal->Ui->customCategoriesTree->invisibleRootItem();
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getSelectedCategoryItem()
{
  auto item = getSelectedItem();
  if (!::isCategory(item))
  {
    item = ::getParent(item);
  }

  return item;
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getSelectedProxyItem()
{
  auto item = getSelectedItem();
  if (::isCategory(item))
  {
    return nullptr;
  }

  return item;
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getNearestItem(QTreeWidgetItem* item)
{
  auto above = this->Internal->Ui->customCategoriesTree->itemAbove(item);
  auto below = this->Internal->Ui->customCategoriesTree->itemBelow(item);

  bool itemIsLastChild = false;
  auto parent = item->parent();
  if (parent)
  {
    auto numberOfItems = parent->childCount();
    auto index = parent->indexOfChild(item);
    itemIsLastChild = index == numberOfItems - 1;
  }

  return itemIsLastChild ? above : below;
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::createNewCategory()
{
  QTreeWidgetItem* parentItem = this->getSelectedCategoryItem();
  QTreeWidgetItem* precedingItem = this->getSelectedItem();

  this->createCategory(::DEFAULT_NAME, parentItem, precedingItem);
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::createCategory(
  const QString& name, QTreeWidgetItem* parentItem, QTreeWidgetItem* precedingItem)
{
  auto parentCategory = ::getCategory(parentItem);
  auto newName = parentCategory->makeUniqueCategoryName(name);

  auto newCategory = new pqProxyCategory(parentCategory, newName, "");

  QTreeWidgetItem* newItem = this->createCategoryItem(parentItem, newCategory, precedingItem);

  this->Internal->Ui->customCategoriesTree->scrollToItem(newItem);
  this->Internal->Ui->customCategoriesTree->setCurrentItem(newItem);
  this->Internal->Ui->customCategoriesTree->editItem(newItem, 0);

  return newItem;
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::deleteCustomItems(QList<QTreeWidgetItem*> items)
{
  QList<QTreeWidgetItem*> itemsToDelete;

  // cleanup: skip items that have an ancestor in the list: removal will be recursive anyway.
  for (auto selectedItem : items)
  {
    if (::hasAncestorInList(selectedItem, items))
    {
      continue;
    }

    itemsToDelete << selectedItem;
  }

  for (auto item : itemsToDelete)
  {
    if (item)
    {
      auto nextSelectedItem = this->getNearestItem(item);
      this->Internal->Ui->customCategoriesTree->scrollToItem(nextSelectedItem);
      this->Internal->Ui->customCategoriesTree->setCurrentItem(nextSelectedItem);
      this->deleteItem(item);
    }
  }
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::deleteItem(QTreeWidgetItem* item)
{
  if (isCategory(item))
  {
    auto category = getCategory(item);
    auto parent = category->parentCategory();
    if (parent)
    {
      parent->removeCategory(category->name());
    }
  }
  else
  {
    auto proxy = getProxy(item);
    if (proxy)
    {
      proxy->setHideFromMenu(true);
    }
  }

  delete item;
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::resetToApplicationCategories()
{
  this->Internal->SettingsCategory->clear();
  this->Internal->SettingsCategory->deepCopy(this->Internal->MenuManager->getApplicationCategory());
  this->populateCurrentCategoriesTree();
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onRemovePressed()
{
  QList<QTreeWidgetItem*> selected = this->Internal->Ui->customCategoriesTree->selectedItems();
  if (selected.empty())
  {
    return;
  }

  this->deleteCustomItems(selected);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::updateToolbarState()
{
  QTreeWidgetItem* categoryItem = this->getSelectedItem();
  if (::isCategory(categoryItem))
  {
    auto category = ::getCategory(categoryItem);
    category->setShowInToolbar(this->Internal->Ui->useAsToolbar->isChecked());
  }
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::insertCategoryFromItem(
  QTreeWidgetItem* sourceItem, QTreeWidgetItem* parentCategoryItem, QTreeWidgetItem* precedingItem)
{
  auto category = ::getCategory(sourceItem);
  auto newItem = this->createCategory(category->name(), parentCategoryItem, precedingItem);
  auto newCategory = ::getCategory(newItem);
  newCategory->deepCopy(category);
  this->populateTree(newItem);
  return true;
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::insertProxyFromItem(
  QTreeWidgetItem* sourceItem, QTreeWidgetItem* categoryItem, QTreeWidgetItem* precedingItem)
{
  auto parentCategory = ::getCategory(categoryItem);
  auto proxyInfo = ::getProxy(sourceItem);
  if (!parentCategory || !proxyInfo)
  {
    return false;
  }

  parentCategory->addProxy(new pqProxyInfo(parentCategory, proxyInfo));
  auto* qitem = this->createProxyItem(categoryItem, proxyInfo, precedingItem);

  this->Internal->Ui->customCategoriesTree->scrollToItem(qitem);
  categoryItem->setExpanded(true);

  return true;
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onAddPressed()
{
  QTreeWidgetItem* categoryItem = this->getSelectedCategoryItem();
  QTreeWidgetItem* precedingItem = this->getSelectedItem();
  auto appSelection = this->Internal->Ui->defaultCategoriesTree->selectedItems();
  if (appSelection.empty())
  {
    return;
  }

  this->insertProxyFromItem(appSelection.first(), categoryItem, precedingItem);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onSetIconPressed()
{
  auto item = this->getSelectedProxyItem();
  if (!item)
  {
    return;
  }

  auto newIconPath = pqIconBrowser::getIconPath();
  this->Internal->setIcon(item, newIconPath);
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::onAccepted()
{
  this->Internal->SettingsCategory->writeSettings(this->ResourceTag);
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::createProxyItem(
  QTreeWidgetItem* parent, pqProxyInfo* proxyInfo, QTreeWidgetItem* preceding)
{
  if (proxyInfo->hideFromMenu())
  {
    return nullptr;
  }

  // avoid proxy duplication under the specified category.
  for (int i = 0; i < parent->childCount(); i++)
  {
    QTreeWidgetItem* child = parent->child(i);
    if (!isCategory(child) && child->text(0) == proxyInfo->label())
    {
      return child;
    }
  }

  auto item = this->createItem(parent, proxyInfo->label(), preceding);
  item->setData(0, ::PROXY_ROLE, QVariant::fromValue(proxyInfo));

  this->Internal->updateItemDisplay(item);

  return item;
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::createCategoryItem(
  QTreeWidgetItem* parent, pqProxyCategory* categoryInfo, QTreeWidgetItem* preceding)
{
  bool blocked = this->Internal->BlockEdition;
  this->Internal->BlockEdition = true;

  auto displayLabel = categoryInfo->label();

  auto item = this->createItem(parent, displayLabel, preceding);
  item->setData(0, ::CATEGORY_ROLE, QVariant::fromValue(categoryInfo));
  item->setToolTip(0, "Category name. The letter after a '&' character defines a menu shortcut.");

  auto itemFlags = item->flags();
  itemFlags |= Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
  item->setFlags(itemFlags);

  QFont font = item->font(0);
  font.setItalic(true);
  item->setFont(0, font);
  item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

  this->Internal->BlockEdition = blocked;
  return item;
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::createItem(
  QTreeWidgetItem* parent, const QString& displayName, QTreeWidgetItem* preceding)
{
  if (displayName.isEmpty())
  {
    return nullptr;
  }

  if (preceding == nullptr)
  {
    preceding = parent->child(parent->childCount() - 1);
  }

  QTreeWidgetItem* item = new QTreeWidgetItem(parent, preceding);
  item->setText(0, displayName);
  item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);

  return item;
}

//----------------------------------------------------------------------------
void pqConfigureCategoriesDialog::updateUIState()
{
  bool hasCustomItemSelection = this->customTreeHasSelection();
  bool hasApplicationItemSelection = this->applicationTreeHasSelection();
  this->Internal->Ui->addFilter->setEnabled(hasApplicationItemSelection && hasCustomItemSelection);
  this->Internal->Ui->addSubCategory->setEnabled(hasCustomItemSelection);

  auto selectedItem = this->getSelectedItem();
  auto category = ::getCategory(selectedItem);
  bool isCategory = ::isCategory(selectedItem);
  this->Internal->Ui->useAsToolbar->setEnabled(hasCustomItemSelection && isCategory && category);
  this->Internal->Ui->useAsToolbar->setChecked(category && category->showInToolbar());

  bool isFavoritesMainCategory = this->Internal->isFavorites(category);
  this->Internal->Ui->remove->setEnabled(hasCustomItemSelection && !isFavoritesMainCategory);

  auto proxyInfo = ::getProxy(selectedItem);
  bool isAvailable = proxyInfo && this->Internal->isAvailable(proxyInfo);
  this->Internal->Ui->setIcon->setEnabled(hasCustomItemSelection && isAvailable);
}

//----------------------------------------------------------------------------
bool pqConfigureCategoriesDialog::sourceIsCustomTree(QDropEvent* dropEvent)
{
  auto sourceTreeWidget = dynamic_cast<QTreeWidget*>(dropEvent->source());
  return sourceTreeWidget == this->Internal->Ui->customCategoriesTree;
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getSourceItem(QDropEvent* dropEvent)
{
  auto sourceTreeWidget = dynamic_cast<QTreeWidget*>(dropEvent->source());
  auto selectedItems = sourceTreeWidget->selectedItems();
  return selectedItems.first();
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getDestinationItem(QDropEvent* dropEvent)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  auto pos = dropEvent->pos();
#else
  auto pos = dropEvent->position().toPoint();
#endif
  return this->Internal->Ui->customCategoriesTree->itemAt(pos);
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqConfigureCategoriesDialog::getDestinationParentItem(QDropEvent* dropEvent)
{
  auto sourceItem = this->getSourceItem(dropEvent);
  bool sourceIsCategory = ::isCategory(sourceItem);
  auto destItem = this->getDestinationItem(dropEvent);

  if (!destItem && sourceIsCategory)
  {
    return this->Internal->Ui->customCategoriesTree->invisibleRootItem();
  }

  bool destIsCategory = ::isCategory(destItem);
  bool onItem = this->Internal->Ui->customCategoriesTree->isDropOnItem();
  if (onItem && destIsCategory)
  {
    return destItem;
  }
  else
  {
    return destItem->parent();
  }
}
