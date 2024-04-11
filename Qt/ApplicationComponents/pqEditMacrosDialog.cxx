// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditMacrosDialog.h"
#include "ui_pqEditMacrosDialog.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqIconBrowser.h"
#include "pqPVApplicationCore.h"
#include "pqPythonMacroSupervisor.h"
#include "pqPythonManager.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonTabWidget.h"

// Qt
#include <QAbstractItemModel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScopedPointer>
#include <QShortcut>
#include <QStringList>
#include <QStyledItemDelegate>

//----------------------------------------------------------------------------
struct pqEditMacrosDialog::pqInternals
{
  class pqPythonMacrosModel : public QAbstractItemModel
  {
  public:
    pqPythonMacrosModel(QObject* parent = nullptr)
      : QAbstractItemModel(parent)
    {
    }

    // qt item role to store macro path
    static constexpr int MACRO_PATH_ROLE() { return Qt::UserRole + 1; }

    int columnCount(const QModelIndex&) const override { return pqInternals::Columns::Count; }

    int rowCount(const QModelIndex&) const override
    {
      return pqPythonMacroSupervisor::getMacrosFilePaths().size();
    }

    QModelIndex index(int row, int column, const QModelIndex&) const override
    {
      return this->createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex&) const override { return QModelIndex(); }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
      return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
      if (!index.isValid())
      {
        return QVariant();
      }
      const int& row = index.row();
      const int& column = index.column();
      QVariant result = QVariant();
      const QStringList fileNames = pqPythonMacroSupervisor::getMacrosFilePaths();
      if (row >= fileNames.size())
      {
        return result;
      }
      const QString fileName = fileNames[row];
      if (column == pqInternals::Icons)
      {
        switch (role)
        {
          case Qt::EditRole:
            result = pqPythonMacroSupervisor::iconPathFromFileName(fileName);
            break;
          case Qt::DecorationRole:
            result = QIcon(pqPythonMacroSupervisor::iconPathFromFileName(fileName));
            break;
          case Qt::ToolTipRole:
            result = QString("Select an icon for the macro.");
            break;
          case MACRO_PATH_ROLE():
            result = fileNames[index.row()];
            break;
          default:
            break;
        }
      }
      else if (column == pqInternals::Tooltips)
      {
        switch (role)
        {
          case Qt::EditRole:
          case Qt::DisplayRole:
            result = pqPythonMacroSupervisor::macroToolTipFromFileName(fileName);
            break;
          case Qt::ToolTipRole:
            result = QString("Edit the tooltip of a macro.");
            break;
          case MACRO_PATH_ROLE():
            result = fileNames[index.row()];
            break;
          default:
            break;
        }
      }
      else if (column == pqInternals::Macros)
      {
        switch (role)
        {
          case Qt::EditRole:
          case Qt::DisplayRole:
            result = pqPythonMacroSupervisor::macroNameFromFileName(fileName);
            break;
          case Qt::ToolTipRole:
            result = QString("Edit the macro.");
            break;
          case MACRO_PATH_ROLE():
            result = fileNames[index.row()];
            break;
          default:
            break;
        }
      }
      return result;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
      if (role != Qt::DisplayRole)
      {
        return QVariant();
      }
      if (orientation == Qt::Orientation::Vertical)
      {
        return QVariant();
      }
      if (section >= pqEditMacrosDialog::pqInternals::Columns::Count || section < 0)
      {
        return QVariant();
      }
      const std::vector<std::string> headerLabels = { "Icon", "Macro", "Script", "Tool tip" };
      return QVariant(headerLabels.at(section).c_str());
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role) override
    {
      if (!index.isValid())
      {
        return false;
      }
      const int& row = index.row();
      const int& column = index.column();
      const QStringList fileNames = pqPythonMacroSupervisor::getMacrosFilePaths();
      if (row >= fileNames.size())
      {
        return false;
      }
      const QString macroPath = fileNames[row];
      const QString text = value.toString();
      if (column == pqInternals::Icons)
      {
        pqPythonMacroSupervisor::setIconForMacro(macroPath, text);
        Q_EMIT this->dataChanged(index, index, { role });
        return true;
      }
      else if (column == pqInternals::Tooltips)
      {
        pqPythonMacroSupervisor::setTooltipForMacro(macroPath, text);
        Q_EMIT this->dataChanged(index, index, { role });
        return true;
      }
      else if (column == pqInternals::Macros)
      {
        pqPythonMacroSupervisor::setNameForMacro(macroPath, text);
        Q_EMIT this->dataChanged(index, index, { role });
        return true;
      }
      else
      {
        return false;
      }
    }

    bool insertRows(int row, int count, const QModelIndex& parent) override
    {
      const int last = row + count - 1;
      this->beginInsertRows(parent, row, last);
      this->endInsertRows();
      return count > 0;
    }

    bool removeRows(int row, int count, const QModelIndex& parent) override
    {
      const int last = row + count - 1;
      this->beginRemoveRows(parent, row, last);
      this->endRemoveRows();
      return count > 0;
    }
  };

  class pqPythonMacrosItemDelegate : public QStyledItemDelegate
  {
  public:
    pqPythonMacrosItemDelegate(QObject* parent = nullptr)
      : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
      const QModelIndex& index) const override
    {
      Q_ASSERT(index.isValid());
      switch (index.column())
      {
        case pqInternals::Columns::Icons:
        {
          QStyleOptionViewItem opt = option;
          initStyleOption(&opt, index);
          if (option.state & QStyle::State_Selected)
          {
            painter->fillRect(option.rect, option.palette.highlight());
          }
          auto value = index.data(Qt::DecorationRole);
          if (value.isValid() && !value.isNull())
          {
            auto icon = qvariant_cast<QIcon>(value);
            icon.paint(painter, option.rect);
          }
          break;
        }
        case pqInternals::Columns::Scripts:
        {
          QStyleOptionViewItem opt = option;
          initStyleOption(&opt, index);
          if (option.state & QStyle::State_Selected)
          {
            painter->fillRect(option.rect, option.palette.highlight());
          }
          QStyleOptionButton buttonOpt;
          buttonOpt.rect = option.rect;
          buttonOpt.icon = QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton);
          buttonOpt.iconSize = buttonOpt.rect.size();
          buttonOpt.features = QStyleOptionButton::None;
          QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOpt, painter);
          break;
        }
        case pqInternals::Columns::Macros:
        case pqInternals::Columns::Tooltips:
        default:
          QStyledItemDelegate::paint(painter, option, index);
      }
    }

    QRect calculateButtonRect(const QRect& cellRect) const
    {
      // lets the button paint in a rectangle whose width is cell height - 2 pixels
      // so that the button icon doesn't look stretched
      int buttonWidth = cellRect.height() - 2;
      int buttonLeft = cellRect.left() + (cellRect.width() - buttonWidth) * 0.5;
      return QRect(buttonLeft, cellRect.top(), buttonWidth, cellRect.height());
    }

    QWidget* createEditor(
      QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
      switch (index.column())
      {
        case pqInternals::Columns::Icons:
        {
          auto iconBrowser = new pqIconBrowser(pqCoreUtilities::mainWidget());
          iconBrowser->setObjectName("IconBrowser");
          QObject::connect(iconBrowser, &pqIconBrowser::accepted, this,
            &pqPythonMacrosItemDelegate::commitAndCloseEditor);
          iconBrowser->setModal(true);
          iconBrowser->setSizePolicy(
            QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
          return iconBrowser;
        }
        case pqInternals::Columns::Scripts:
        {
          auto macroPath =
            index.data(pqInternals::pqPythonMacrosModel::MACRO_PATH_ROLE()).toString();
          // Python Manager can't be nullptr as this dialog is built only when Python is enabled
          pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
          pythonManager->editMacro(macroPath);
          return nullptr;
        }
        case pqInternals::Columns::Macros:
        case pqInternals::Columns::Tooltips:
        default:
          return QStyledItemDelegate::createEditor(parent, option, index);
      }
    }

    void setModelData(
      QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
      switch (index.column())
      {
        case pqInternals::Columns::Icons:
        {
          pqIconBrowser* iconBrowser = qobject_cast<pqIconBrowser*>(editor);
          auto currentIconPath = index.data(Qt::EditRole).toString();
          auto newIconPath = iconBrowser->getSelectedIconPath();
          model->setData(index, newIconPath, Qt::DisplayRole);
          break;
        }
        case pqInternals::Columns::Scripts:
        case pqInternals::Columns::Macros:
        case pqInternals::Columns::Tooltips:
        default:
          QStyledItemDelegate::setModelData(editor, model, index);
          break;
      }
    }

    void updateEditorGeometry(
      QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
      switch (index.column())
      {
        case pqInternals::Columns::Icons:
          // editor for icons opens up in a new dialog, doesn't make sense to fit the entire dialog
          // inside one table cell.
          break;
        case pqInternals::Columns::Scripts:
          // editor for script opens up in a new dialog, doesn't make sense to fit the entire dialog
          // inside one table cell.
          break;
        case pqInternals::Columns::Macros:
        case pqInternals::Columns::Tooltips:
        default:
          return QStyledItemDelegate::updateEditorGeometry(editor, option, index);
      }
    }

  protected:
    // skip QEvent::FocusOut when editor is a pqIconBrowser.
    // Or else, the icon browser dialog will close when mouse is clicked outside it.
    bool eventFilter(QObject* editor, QEvent* event) override
    {
      pqIconBrowser* iconBrowser = qobject_cast<pqIconBrowser*>(editor);
      if (iconBrowser)
      {
        if (event->type() == QEvent::FocusOut)
        {
          return true;
        }
      }
      return QStyledItemDelegate::eventFilter(editor, event);
    }

  private:
    void commitAndCloseEditor()
    {
      pqIconBrowser* editor = qobject_cast<pqIconBrowser*>(sender());
      Q_EMIT this->commitData(editor);
      Q_EMIT this->closeEditor(editor);
    }
  };

  pqInternals()
    : Ui(new Ui::pqEditMacrosDialog)
  {
  }

  enum Columns
  {
    Icons,
    Macros,
    Scripts, // simply displays a push button
    Tooltips,
    Count
  };

  /**
   * Joins macro names into one string, aggregated with given separator.
   * Joins only the first maxCount macros of the list, adding "..." at
   * the end when more elements are found.
   */
  QString joinMacrosNames(QList<QModelIndex> indices, QString separator, int maxCount)
  {
    QStringList macroNames;
    int numberOfDisplayedMacros = 0;
    for (auto& index : indices)
    {
      macroNames << this->model()->data(index, Qt::DisplayRole).toString();
      numberOfDisplayedMacros++;
      if (numberOfDisplayedMacros >= maxCount)
      {
        const int extraCount = indices.size() - maxCount;
        if (extraCount > 0)
        {
          macroNames << tr("... (%1 more)").arg(extraCount);
        }
        break;
      }
    }
    return macroNames.join(separator);
  }

  inline pqPythonMacrosModel* model() const
  {
    return static_cast<pqPythonMacrosModel*>(this->view()->model());
  }

  inline QTreeView* view() const { return this->Ui->macrosTree; }

  QScopedPointer<Ui::pqEditMacrosDialog> Ui;
};

//----------------------------------------------------------------------------
pqEditMacrosDialog::pqEditMacrosDialog(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqInternals)
{
  this->Internals->Ui->setupUi(this);
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  this->Internals->view()->setModel(new pqInternals::pqPythonMacrosModel());
  this->Internals->view()->setItemDelegate(new pqInternals::pqPythonMacrosItemDelegate());
  this->Internals->view()->sortByColumn(pqInternals::Macros, Qt::AscendingOrder);
  this->Internals->view()->setSelectionBehavior(QTreeView::SelectionBehavior::SelectRows);
  this->Internals->view()->resizeColumnToContents(pqInternals::Scripts);
  this->Internals->view()->resizeColumnToContents(pqInternals::Icons);
  this->Internals->view()->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

  QObject::connect(pqPVApplicationCore::instance()->pythonManager()->macroSupervisor(),
    &pqPythonMacroSupervisor::onAddedMacro, this, [this]() {
      this->Internals->view()->resizeColumnToContents(pqInternals::Icons);
      this->Internals->view()->resizeColumnToContents(pqInternals::Macros);
      this->Internals->view()->resizeColumnToContents(pqInternals::Scripts);
      this->Internals->view()->resizeColumnToContents(pqInternals::Tooltips);
      this->Internals->view()->selectionModel()->setCurrentIndex(
        QModelIndex(), QItemSelectionModel::NoUpdate);
      this->updateUIState();
    });
  QObject::connect(this->Internals->model(), &QAbstractItemModel::dataChanged, []() {
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->updateMacroList();
  });

  QObject::connect(
    this->Internals->Ui->add, &QToolButton::released, this, &pqEditMacrosDialog::onAddPressed);

  QObject::connect(
    this->Internals->Ui->edit, &QToolButton::released, this, &pqEditMacrosDialog::onEditPressed);

  QObject::connect(this->Internals->Ui->setIcon, &QToolButton::released, this,
    &pqEditMacrosDialog::onSetIconPressed);

  QObject::connect(this->Internals->Ui->remove, &QToolButton::released, this,
    &pqEditMacrosDialog::onRemovePressed);
  QShortcut* deleteShortcut =
    new QShortcut(QKeySequence(Qt::Key_Delete), this->Internals->Ui->macrosTree);
  QObject::connect(
    deleteShortcut, &QShortcut::activated, this, &pqEditMacrosDialog::onRemovePressed);

  QObject::connect(this->Internals->Ui->removeAll, &QToolButton::released, this,
    &pqEditMacrosDialog::onRemoveAllPressed);

  QObject::connect(this->Internals->Ui->searchBox, &pqSearchBox::textChanged, this,
    &pqEditMacrosDialog::onSearchTextChanged);

  this->connect(this->Internals->view()->selectionModel(), &QItemSelectionModel::selectionChanged,
    this, &pqEditMacrosDialog::updateUIState);

  this->connect(this->Internals->view(), &QTreeView::clicked, [this](const QModelIndex& index) {
    if (index.column() == pqInternals::Columns::Scripts)
    {
      this->onEditPressed();
    }
  });

  this->updateUIState();
}

//----------------------------------------------------------------------------
pqEditMacrosDialog::~pqEditMacrosDialog() = default;

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onAddPressed()
{
  const auto& internals = (*this->Internals);
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(),
    tr("Open Python File to create a Macro:"), QString(),
    tr("Python Files") + QString(" (*.py);;") + tr("All Files") + QString(" (*)"), false, false);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    const auto& root = internals.view()->rootIndex();
    internals.model()->insertRow(internals.model()->rowCount(root), root);
    pythonManager->addMacro(fileDialog.getSelectedFiles()[0], fileDialog.getSelectedLocation());
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onEditPressed()
{
  const auto& internals = (*this->Internals);
  auto* selectionModel = internals.view()->selectionModel();
  auto selectedIndices = selectionModel->selectedRows(pqInternals::Macros);
  for (const auto& index : selectedIndices)
  {
    auto macroPath = internals.model()
                       ->data(index, pqInternals::pqPythonMacrosModel::MACRO_PATH_ROLE())
                       .toString();
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->editMacro(macroPath);
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onSetIconPressed()
{
  const auto& internals = (*this->Internals);
  auto* selectionModel = internals.view()->selectionModel();
  auto selectedIndices = selectionModel->selectedRows(pqInternals::Icons);
  if (selectedIndices.empty())
  {
    return;
  }

  const auto& index = selectedIndices.first();
  auto macroPath =
    internals.model()->data(index, pqInternals::pqPythonMacrosModel::MACRO_PATH_ROLE()).toString();
  QString iconPath = pqPythonMacroSupervisor::iconPathFromFileName(macroPath);

  auto newIconPath = pqIconBrowser::getIconPath(iconPath);
  if (newIconPath == iconPath)
  {
    return;
  }

  internals.model()->setData(index, newIconPath, Qt::DisplayRole);
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onRemovePressed()
{
  const auto& internals = (*this->Internals);
  auto* selectionModel = internals.view()->selectionModel();
  auto selectedIndices = selectionModel->selectedRows(pqInternals::Macros);
  if (selectedIndices.empty())
  {
    return;
  }

  QString intro = QCoreApplication::translate("pqMacrosMenu", "Selected macros will be deleted: ");
  static const int maxNumberOfNames = 5;
  QString displayedNames =
    this->Internals->joinMacrosNames(selectedIndices, QString(", "), maxNumberOfNames);
  QString question = QCoreApplication::translate("pqMacrosMenu", "Are you sure?");
  QString fullmessage = QString("%1\n%2\n%3").arg(intro).arg(displayedNames).arg(question);

  QMessageBox::StandardButton ret = QMessageBox::question(pqCoreUtilities::mainWidget(),
    QCoreApplication::translate("pqMacrosMenu", "Delete Macro(s)"), fullmessage);

  if (ret == QMessageBox::StandardButton::Yes)
  {
    this->deleteItems(selectedIndices);
    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->updateMacroList();
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onRemoveAllPressed()
{
  const auto& internals = (*this->Internals);
  QMessageBox::StandardButton ret = QMessageBox::question(pqCoreUtilities::mainWidget(),
    QCoreApplication::translate("pqMacrosMenu", "Delete All"),
    QCoreApplication::translate("pqMacrosMenu", "All macros will be deleted. Are you sure?"));
  if (ret == QMessageBox::StandardButton::Yes)
  {
    // The script editor shows macros about to be deleted. remove those tabs.
    pqPythonTabWidget* const tWidget =
      pqPythonScriptEditor::getUniqueInstance()->findChild<pqPythonTabWidget*>();
    for (int i = tWidget->count() - 1; i >= 0; --i)
    {
      Q_EMIT tWidget->tabCloseRequested(i);
    }

    // remove user Macros dir
    const auto& root = internals.view()->rootIndex();
    internals.model()->removeRows(0, internals.model()->rowCount(root), root);
    pqCoreUtilities::removeRecursively(pqPythonScriptEditor::getMacrosDir());
    this->updateUIState();

    // Python Manager can't be nullptr as this dialog is built only when Python is enabled
    pqPythonManager* pythonManager = pqPVApplicationCore::instance()->pythonManager();
    pythonManager->updateMacroList();
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::onSearchTextChanged(const QString& pattern)
{
  const auto& internals = (*this->Internals);
  const auto& root = internals.view()->rootIndex();
  for (int i = 0; i < internals.model()->rowCount(root); ++i)
  {
    if (pattern.isEmpty())
    {
      internals.view()->setRowHidden(i, root, false);
    }
    else
    {
      const auto& index = internals.model()->index(i, pqInternals::Macros, root);
      const auto& macroName = internals.model()->data(index, Qt::DisplayRole).toString();
      internals.view()->setRowHidden(i, root, !macroName.contains(pattern, Qt::CaseInsensitive));
    }
  }
}

//----------------------------------------------------------------------------
bool pqEditMacrosDialog::treeHasItems()
{
  return this->Internals->model()->rowCount(this->Internals->view()->rootIndex()) > 0;
}

//----------------------------------------------------------------------------
bool pqEditMacrosDialog::treeHasSelectedItems()
{
  return this->Internals->view()->selectionModel()->hasSelection();
}

//----------------------------------------------------------------------------
QModelIndex pqEditMacrosDialog::getSelectedItem()
{
  const auto& internals = (*this->Internals);
  if (this->treeHasSelectedItems())
  {
    return internals.view()->selectionModel()->selectedIndexes().first();
  }

  return internals.view()->rootIndex();
}

//----------------------------------------------------------------------------
QModelIndex pqEditMacrosDialog::getNearestItem(const QModelIndex& index)
{
  const auto& internals = (*this->Internals);
  auto above = internals.view()->indexAbove(index);
  auto below = internals.view()->indexBelow(index);

  bool itemIsLastChild = false;
  auto numberOfItems = internals.model()->rowCount(internals.view()->rootIndex());
  itemIsLastChild = index.row() == numberOfItems - 1;

  return itemIsLastChild ? above : below;
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::deleteItem(const QModelIndex& index)
{
  const auto& internals = (*this->Internals);
  pqPythonMacroSupervisor::hideFile(
    internals.model()->data(index, pqInternals::pqPythonMacrosModel::MACRO_PATH_ROLE()).toString());
  internals.model()->removeRow(index.row(), internals.view()->rootIndex());
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::deleteItems(const QList<QModelIndex>& indices)
{
  const auto& internals = (*this->Internals);
  for (const auto& index : indices)
  {
    auto nextSelectedItem = this->getNearestItem(index);
    internals.view()->scrollTo(nextSelectedItem);
    internals.view()->setCurrentIndex(nextSelectedItem);
    this->deleteItem(index);
  }
}

//----------------------------------------------------------------------------
void pqEditMacrosDialog::updateUIState()
{
  const auto& internals = (*this->Internals);
  const bool hasItems = this->treeHasItems();
  const bool hasSelectedItems = this->treeHasSelectedItems();

  internals.Ui->edit->setEnabled(hasSelectedItems);
  internals.Ui->remove->setEnabled(hasSelectedItems);
  internals.Ui->setIcon->setEnabled(hasSelectedItems);
  internals.Ui->removeAll->setEnabled(hasItems);
}
