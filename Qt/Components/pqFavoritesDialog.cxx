// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

// Hide PARAVIEW_DEPRECATED_IN_5_13_0() warnings for this class.
#define PARAVIEW_DEPRECATION_LEVEL 0

// self
#include "pqFavoritesDialog.h"
#include "pqQtDeprecated.h"
#include "ui_pqFavoritesDialog.h"

// Qt
#include <QDropEvent>
#include <QHeaderView>
#include <QRegularExpression>
#include <QSequentialIterable>
#include <QSet>
#include <QShortcut>
#include <QStringList>

// pqCore
#include "pqApplicationCore.h"
#include "pqSettings.h"

namespace
{
//----------------------------------------------------------------------------
bool isItemCategory(QTreeWidgetItem* item)
{
  return item->data(0, Qt::UserRole).toBool();
}

//----------------------------------------------------------------------------
void ensureUniqueCategoryName(QTreeWidgetItem* parent, QTreeWidgetItem* item, int v = 0)
{
  QString itemText = item->text(0);
  for (int i = 0; parent && i < parent->childCount(); i++)
  {
    QTreeWidgetItem* child = parent->child(i);
    if (child != item && isItemCategory(child) && child->text(0) == itemText)
    {
      QRegularExpression rx("(.*)_([0-9]+)");
      QRegularExpressionMatch match = rx.match(itemText);
      if (match.hasMatch())
      {
        itemText = match.captured(1);
        v = match.captured(2).toInt();
      }
      item->setText(0, QString("%1_%2").arg(itemText, QString::number(v + 1)));
      ensureUniqueCategoryName(parent, item, v + 1);
      break;
    }
  }
}

//----------------------------------------------------------------------------
void ensureUniqueCategoryName(QTreeWidget* w, QTreeWidgetItem* item, int v = 0)
{
  QTreeWidgetItem* parent = item->parent() ? item->parent() : w->invisibleRootItem();
  ensureUniqueCategoryName(parent, item, v);
}

//----------------------------------------------------------------------------
QTreeWidgetItem* createItem(QTreeWidgetItem* parent, const QString& name, bool isCategory,
  const QString& proxyName = "", QTreeWidgetItem* preceding = nullptr, bool allowDuplicated = false)
{
  if (name.isEmpty())
  {
    return nullptr;
  }
  for (int i = 0; !allowDuplicated && i < parent->childCount(); i++)
  {
    QTreeWidgetItem* child = parent->child(i);
    if (isItemCategory(child) == isCategory && child->text(0) == name)
    {
      return child;
    }
  }

  if (preceding == nullptr)
  {
    preceding = parent->child(parent->childCount() - 1);
  }

  QTreeWidgetItem* item = new QTreeWidgetItem(parent, preceding, QTreeWidgetItem::UserType);
  Qt::ItemFlags itemFlags =
    Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;

  if (isCategory)
  {
    itemFlags |= Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
    QFont f = item->font(0);
    f.setItalic(true);
    item->setFont(0, f);
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    item->setExpanded(true);
  }

  item->setText(0, name);
  item->setData(0, Qt::UserRole, QVariant(isCategory));
  item->setData(0, Qt::UserRole + 1, proxyName);
  item->setFlags(itemFlags);

  return item;
}

//----------------------------------------------------------------------------
void removeTreeViewItemRec(
  QTreeWidgetItem* item, QSet<QTreeWidgetItem*>& deletedItems, bool parentIsDeleted = false)
{
  if (!deletedItems.contains(item))
  {
    deletedItems.insert(item);
    for (int i = 0; i < item->childCount(); i++)
    {
      removeTreeViewItemRec(item->child(i), deletedItems, true);
    }
    if (!parentIsDeleted)
    {
      delete item;
    }
  }
}

//----------------------------------------------------------------------------
void fetchTreeViewItemRec(QTreeWidget* widget, QTreeWidgetItem* item,
  QSet<QTreeWidgetItem*>& aboveItems, QSet<QTreeWidgetItem*>& belowItems)
{
  aboveItems.insert(widget->itemAbove(item));
  belowItems.insert(widget->itemBelow(item));
  for (int i = 0; i < item->childCount(); i++)
  {
    fetchTreeViewItemRec(widget, item->child(i), aboveItems, belowItems);
  }
}
}

//----------------------------------------------------------------------------
pqFavoritesDialog::pqFavoritesDialog(const QVariant& filtersList, QWidget* p)
  : Superclass(p)
  , Ui(new Ui::pqFavoritesDialog())
{
  this->Ui->setupUi(this);
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  this->Ui->availableFilters->setAcceptDrops(false);
  this->Ui->availableFilters->sortByColumn(0, Qt::AscendingOrder);
  this->Ui->availableFilters->setHeaderLabels(QStringList() << QString("Name"));

  QObject::connect(this->Ui->addCategory, SIGNAL(released()), this, SLOT(createCategory()));
  QObject::connect(
    this->Ui->removeFavorite, SIGNAL(released()), this, SLOT(onRemoveFavoritePressed()));
  QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this->Ui->favorites);
  QObject::connect(shortcut, SIGNAL(activated()), this, SLOT(onRemoveFavoritePressed()));

  this->Ui->favorites->setHeaderLabels(QStringList() << QString("Name"));
  this->Ui->favorites->viewport()->installEventFilter(this);
  QObject::connect(this->Ui->addFavorite, SIGNAL(released()), this, SLOT(onAddFavoritePressed()));
  QObject::connect(this->Ui->favorites, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this,
    SLOT(onItemChanged(QTreeWidgetItem*, int)));
  QObject::connect(
    this->Ui->searchBox, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));

  this->connect(this, SIGNAL(accepted()), SLOT(onAccepted()));

  this->populateFiltersTree(filtersList);
  this->populateFavoritesTree();
}

//----------------------------------------------------------------------------
pqFavoritesDialog::~pqFavoritesDialog() = default;

//----------------------------------------------------------------------------
void pqFavoritesDialog::onSearchTextChanged(QString pattern)
{
  auto* root = this->Ui->availableFilters->invisibleRootItem();
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
bool pqFavoritesDialog::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Drop)
  {
    auto* dropEvent = static_cast<QDropEvent*>(event);
    auto* sourceTreeView = dynamic_cast<QTreeWidget*>(dropEvent->source());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto pos = dropEvent->pos();
#else
    auto pos = dropEvent->position().toPoint();
#endif
    auto* destItem = this->Ui->favorites->itemAt(pos);
    QTreeWidgetItem* categoryItem = nullptr;
    if (destItem)
    {
      bool onItem = this->Ui->favorites->isDropOnItem();
      categoryItem = onItem ? destItem : destItem->parent();
    }
    if (categoryItem == nullptr)
    {
      categoryItem = this->Ui->favorites->invisibleRootItem();
    }
    else
    {
      categoryItem->setExpanded(true);
    }

    for (auto* item : sourceTreeView->selectedItems())
    {
      for (int i = 0; i < categoryItem->childCount(); i++)
      {
        auto* child = categoryItem->child(i);
        if (child != item && !::isItemCategory(child) && !::isItemCategory(item) &&
          child->text(0) == item->text(0))
        {
          // This item already exists at this level, skip it!
          dropEvent->setDropAction(Qt::IgnoreAction);
          event->ignore();
          return true;
        }
      }
    }
    for (auto* item : sourceTreeView->selectedItems())
    {
      if (::isItemCategory(item))
      {
        ::ensureUniqueCategoryName(categoryItem, item);
      }
    }
  }

  return QDialog::eventFilter(object, event);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::populateFavoritesTree()
{
  this->Ui->favorites->clear();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QStringLiteral("favorites.ParaViewFilters/");
  if (settings->contains(key))
  {
    QString settingValue = settings->value(key).toString();
    QStringList bmList = settingValue.split("|", PV_QT_SKIP_EMPTY_PARTS);
    for (const QString& bm : bmList)
    {
      QStringList bmPath = bm.split(";", PV_QT_SKIP_EMPTY_PARTS);
      if (bmPath.size() >= 2)
      {
        QString group = bmPath.takeFirst();
        bool isCategory = group.compare("categories") == 0;
        QString proxyName = isCategory ? "" : bmPath.takeLast();
        QString displayName = bmPath.takeLast();

        QTreeWidgetItem* root = this->Ui->favorites->invisibleRootItem();
        for (const QString& category : bmPath)
        {
          root = ::createItem(root, category, true);
        }
        ::createItem(root, displayName, isCategory, proxyName);
      }
    }
  }
  this->Ui->favorites->resizeColumnToContents(0);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::populateFiltersTree(const QVariant& filtersList)
{
  this->Ui->availableFilters->clear();
  QSequentialIterable iterableList = filtersList.value<QSequentialIterable>();

  for (const QVariant& variant : iterableList)
  {
    if (variant.canConvert<QStringList>())
    {
      QStringList filter = variant.toStringList();
      ::createItem(
        this->Ui->availableFilters->invisibleRootItem(), filter.at(0), false, filter.at(1));
    }
  }
  this->Ui->availableFilters->resizeColumnToContents(0);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::onItemChanged(QTreeWidgetItem* item, int)
{
  if (::isItemCategory(item))
  {
    // Ensure unicity of categories
    ::ensureUniqueCategoryName(this->Ui->favorites, item);
  }
  else
  {
    // ensure that item that are not categories have drop disabled
    if (item->flags() & Qt::ItemIsDropEnabled)
    {
      item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
    }
  }
  this->Ui->favorites->scrollToItem(item);
  this->Ui->favorites->setCurrentItem(item);
}

//----------------------------------------------------------------------------
QTreeWidgetItem* pqFavoritesDialog::getSelectedCategory()
{
  QTreeWidgetItem* item = this->Ui->favorites->invisibleRootItem();
  if (!this->Ui->favorites->selectedItems().isEmpty())
  {
    item = this->Ui->favorites->selectedItems().first();
    if (!::isItemCategory(item))
    {
      item = item->parent();
    }
  }

  return item;
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::createCategory()
{
  QTreeWidgetItem* root = this->getSelectedCategory() ? this->getSelectedCategory()
                                                      : this->Ui->favorites->invisibleRootItem();
  QTreeWidgetItem* precedingItem = nullptr;
  if (!this->Ui->favorites->selectedItems().isEmpty() &&
    !::isItemCategory(this->Ui->favorites->selectedItems().first()))
  {
    precedingItem = this->Ui->favorites->selectedItems().first();
  }

  QTreeWidgetItem* newItem =
    ::createItem(root, QStringLiteral("New Category"), true, "", precedingItem, true);
  this->Ui->favorites->scrollToItem(newItem);
  this->Ui->favorites->setCurrentItem(newItem);
  this->Ui->favorites->editItem(newItem, 0);
  this->Ui->favorites->resizeColumnToContents(0);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::onRemoveFavoritePressed()
{
  QSet<QTreeWidgetItem*> deletedItems, aboveItems, belowItems;
  QList<QTreeWidgetItem*> selected = this->Ui->favorites->selectedItems();
  if (selected.empty())
  {
    return;
  }

  // Fetch all items that are above and below the selected items
  for (auto* item : selected)
  {
    ::fetchTreeViewItemRec(this->Ui->favorites, item, aboveItems, belowItems);
  }
  // Remove the selected items recursively

  for (auto* item : selected)
  {
    ::removeTreeViewItemRec(item, deletedItems);
  }
  // deleteItems contains all the removed item. We add the null pointer
  // as aboveItems and belowItems might contains the null pointer.
  deletedItems.insert(nullptr);
  // Select the/a below item if it exists otherwise, select the/a above one.
  // We remove all the deleted items from those sets to make sure we select
  // an item that is still alive. Note: on sparsed multiple selection, the
  // selection is not predictive, it might occurs around any of the selected
  // element, not the one with the highest row id.
  belowItems = belowItems.subtract(deletedItems);
  if (belowItems.count() == 0)
  {
    belowItems = aboveItems.subtract(deletedItems);
  }
  if (belowItems.count() != 0)
  {
    auto* item = *belowItems.begin();
    this->Ui->favorites->scrollToItem(item);
    this->Ui->favorites->setCurrentItem(item);
  }

  this->Ui->favorites->resizeColumnToContents(0);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::onAddFavoritePressed()
{
  QTreeWidgetItem* root = this->getSelectedCategory() ? this->getSelectedCategory()
                                                      : this->Ui->favorites->invisibleRootItem();
  QTreeWidgetItem* precedingItem = nullptr;
  if (!this->Ui->favorites->selectedItems().isEmpty() &&
    !::isItemCategory(this->Ui->favorites->selectedItems().first()))
  {
    precedingItem = this->Ui->favorites->selectedItems().first();
  }

  for (auto item : this->Ui->availableFilters->selectedItems())
  {
    auto* qitem = ::createItem(
      root, item->text(0), false, item->data(0, Qt::UserRole + 1).toString(), precedingItem);

    this->Ui->favorites->scrollToItem(qitem);
  }

  root->setExpanded(true);
  this->Ui->favorites->resizeColumnToContents(0);
}

//----------------------------------------------------------------------------
void pqFavoritesDialog::onAccepted()
{
  QTreeWidgetItem* item = this->Ui->favorites->invisibleRootItem();
  QString value;
  value = this->populateSettingsValue(value, item);
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QStringLiteral("favorites.ParaViewFilters/");
  settings->setValue(key, value);
}

//----------------------------------------------------------------------------
QString pqFavoritesDialog::populateSettingsValue(const QString& value, QTreeWidgetItem* item)
{
  QString newValue;
  for (auto child : item->takeChildren())
  {
    if (child->childCount() > 0)
    {
      QString currentPath = QString("%1;%2").arg(value).arg(child->text(0));
      newValue += this->populateSettingsValue(currentPath, child);
    }
    else
    {
      QString group =
        ::isItemCategory(child) ? QStringLiteral("categories") : QStringLiteral("filters");
      QString proxyName = child->data(0, Qt::UserRole + 1).toString();
      newValue += QString("%1;%2;%3;%4|")
                    .arg(group)
                    .arg(value)
                    .arg(child->text(0))
                    .arg(child->data(0, Qt::UserRole + 1).toString());
    }
  }
  return newValue;
}
