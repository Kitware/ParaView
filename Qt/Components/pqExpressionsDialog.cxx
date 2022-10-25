/*=========================================================================

   Program: ParaView
   Module:    pqExpressionsDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqExpressionsDialog.h"
#include "ui_pqExpressionsDialog.h"

#include "pqApplicationCore.h"
#include "pqExpressionsManager.h"
#include "pqExpressionsTableModel.h"
#include "pqFileDialog.h"
#include "pqSettings.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QtDebug>

#include "vtksys/FStream.hxx"

#include "vtk_nlohmannjson.h"
#include VTK_NLOHMANN_JSON(json.hpp)

using json = nlohmann::json;

constexpr const char* DEFAULT_GROUP_NAME = pqExpressionsManager::EXPRESSION_GROUP();
constexpr const char* ALL_GROUP_NAME = "All";
constexpr const char* BROWSE_FILE_FILTER = "ParaView Expressions (*.json);;All Files (*)";

constexpr const char* JSON_FILE_VERSION = "1.0";
constexpr const char* JSON_FILE_VERSION_KEY = "version";
constexpr const char* JSON_EXPRESSIONS_LIST_KEY = "Expressions";
constexpr const char* JSON_EXPRESSION_GROUP_KEY = "Group";
constexpr const char* JSON_EXPRESSION_NAME_KEY = "Name";
constexpr const char* JSON_EXPRESSION_KEY = "Expression";

class pqEditGroupDelegate : public QStyledItemDelegate
{
public:
  pqEditGroupDelegate(QObject* parent = nullptr)
    : QStyledItemDelegate(parent)
  {
  }

  //-----------------------------------------------------------------------------
  void closeCurrentEditor()
  {
    QWidget* editor = qobject_cast<QWidget*>(this->sender());
    Q_EMIT this->commitData(editor);
    Q_EMIT this->closeEditor(editor);
  }

  QWidget* createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
  {
    auto groupList = new QComboBox(parent);
    groupList->addItem(pqExpressionsManager::EXPRESSION_GROUP());
    groupList->addItem(pqExpressionsManager::PYTHON_EXPRESSION_GROUP());

    // Make the combobox selection automatically apply
    QObject::connect(groupList, QOverload<int>::of(&QComboBox::activated), this,
      &pqEditGroupDelegate::closeCurrentEditor);

    return groupList;
  }

  void setEditorData(QWidget* editor, const QModelIndex& index) const override
  {
    auto groupList = static_cast<QComboBox*>(editor);
    QString group = index.model()->data(index, Qt::EditRole).toString();
    groupList->setCurrentText(group);
  }

  void setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
  {
    auto groupList = static_cast<QComboBox*>(editor);
    QString group = groupList->currentText();
    model->setData(index, group, Qt::EditRole);
  }

  void updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override
  {
    editor->setGeometry(option.rect);
  }
};

class pqExpressionFilterProxyModel : public QSortFilterProxyModel
{
  typedef QSortFilterProxyModel Superclass;
  QRegularExpression Group;
  QRegularExpression UserFilter;

public:
  pqExpressionFilterProxyModel(QObject* parent)
    : Superclass(parent)
    , Group("")
    , UserFilter("")
  {
    this->Group.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    this->UserFilter.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
  }

  ~pqExpressionFilterProxyModel() override = default;

  void update() { this->invalidateFilter(); }

  /**
   * Set inner regular expression from group name.
   * If group is ALL_GROUP_NAME, set an empty pattern to match all.
   * Otherwise, force an exact match (with ^ and $).
   */
  void setGroupFilter(const QString& group)
  {
    if (group == ALL_GROUP_NAME)
    {
      this->Group.setPattern("");
    }
    else
    {
      this->Group.setPattern(QString("^%1$").arg(group));
    }
    this->invalidateFilter();
  }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set filter string as the regular expression pattern.
   */
  void setUserFilter(const QString& filter)
  {
    this->UserFilter.setPattern(filter);
    this->invalidateFilter();
  }

protected:
  /**
   * Reimplement superclass to filter according to selected group and user filter text.
   * Group column should match exactly given group.
   * Name column OR Expression column should match user given string (as regular expression)
   */
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
  {
    Q_UNUSED(sourceParent);
    const QModelIndex& groupIdx =
      this->sourceModel()->index(sourceRow, pqExpressionsTableModel::GroupColumn);
    // if group match
    if (this->sourceModel()->data(groupIdx).toString().contains(this->Group))
    {
      const QModelIndex& nameIdx =
        this->sourceModel()->index(sourceRow, pqExpressionsTableModel::NameColumn);
      if (this->sourceModel()->data(nameIdx).toString().contains(this->UserFilter))
      {
        return true;
      }
      const QModelIndex& valueIdx =
        this->sourceModel()->index(sourceRow, pqExpressionsTableModel::ExpressionColumn);
      if (this->sourceModel()->data(valueIdx).toString().contains(this->UserFilter))
      {
        return true;
      }
    }

    return false;
  };

  /**
   * Do QString comparison between data from given rows, same column.
   */
  int compare(int leftRow, int rightRow, int column) const
  {
    const QModelIndex& leftStringIdx = this->sourceModel()->index(leftRow, column);
    const QModelIndex& rightStringIdx = this->sourceModel()->index(rightRow, column);
    const QString& leftString = this->sourceModel()->data(leftStringIdx).toString();
    const QString& rightString = this->sourceModel()->data(rightStringIdx).toString();

    return leftString.compare(rightString, this->sortCaseSensitivity());
  }

  /**
   * Reimplement superclass method to do multi-columns sort.
   * Sort order: Group then Name then Expression.
   */
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
  {
    int leftRow = left.row();
    int rightRow = right.row();
    int column = left.column();

    // start with user defined main sort column.
    int compare = this->compare(leftRow, rightRow, column);
    if (compare != 0)
    {
      return compare < 0;
    }

    for (column = 0; column < pqExpressionsTableModel::NumberOfColumns; column++)
    {
      // user defined column already processed, skip.
      if (column == left.column())
      {
        continue;
      }

      compare = this->compare(leftRow, rightRow, column);
      if (compare != 0)
      {
        return compare < 0;
      }
    }

    return false;
  }
};

class pqExpressionsManagerDialog::pqInternals
{
public:
  pqInternals(pqExpressionsManagerDialog* self, const QString& group)
    : Model(new pqExpressionsTableModel(self))
    , Group(group)
    , ExpressionProxyFilter(new pqExpressionFilterProxyModel(self))
  {
    this->Ui.setupUi(self);

    this->ExpressionProxyFilter->setSourceModel(this->Model.get());
    // use all column for textual filtering
    this->ExpressionProxyFilter->setFilterKeyColumn(-1);
    this->ExpressionProxyFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
    this->Ui.expressions->setModel(this->ExpressionProxyFilter.get());
    this->Ui.expressions->setPasteEnabled(false);

    QObject::connect(this->Ui.searchBox, &pqSearchBox::textChanged,
      [&](const QString& filter) { this->ExpressionProxyFilter->setUserFilter(filter); });

    this->Ui.groupChooser->addItem(ALL_GROUP_NAME);
    this->Ui.groupChooser->addItem(pqExpressionsManager::EXPRESSION_GROUP());
    this->Ui.groupChooser->addItem(pqExpressionsManager::PYTHON_EXPRESSION_GROUP());

    int width = this->Ui.groupChooser->sizeHint().width();
    this->Ui.expressions->setColumnWidth(pqExpressionsTableModel::GroupColumn, width);

    this->Ui.expressions->setItemDelegateForColumn(
      pqExpressionsTableModel::GroupColumn, new pqEditGroupDelegate());
  }

  void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
  {
    this->ExpressionProxyFilter->sort(column, order);
  }

  QModelIndex mapToSource(const QModelIndex& index)
  {
    return this->ExpressionProxyFilter->mapToSource(index);
  }

  QModelIndex mapFromSource(const QModelIndex& index)
  {
    return this->ExpressionProxyFilter->mapFromSource(index);
  }

  void updateGroupFilter(const QString& group)
  {
    this->ExpressionProxyFilter->setGroupFilter(group);
  }

  QString selectedExpression()
  {
    QModelIndexList selectedRows = this->Ui.expressions->selectionModel()->selectedRows();
    QModelIndex idx = this->mapToSource(selectedRows.first());
    return this->Model->getExpressionAsString(idx);
  }

  void updateFilter() { this->ExpressionProxyFilter->update(); }

  std::unique_ptr<pqExpressionsTableModel> Model;
  Ui::pqExpressionsManagerDialog Ui;
  QString Group;

protected:
  std::unique_ptr<pqExpressionFilterProxyModel> ExpressionProxyFilter;
};

//----------------------------------------------------------------------------
pqExpressionsManagerDialog::pqExpressionsManagerDialog(QWidget* parent, const QString& group)
  : Superclass(parent)
  , Internals(new pqInternals(this, group))
{
  Ui::pqExpressionsManagerDialog& ui = this->Internals->Ui;

  QHeaderView* header = ui.expressions->horizontalHeader();
  header->setSortIndicatorShown(true);

  this->connect(header, &QHeaderView::sortIndicatorChanged,
    [&](int col, Qt::SortOrder order) { this->Internals->sort(col, order); });

  this->connect(this->Internals->Model.get(), &pqExpressionsTableModel::dataChanged, this,
    &pqExpressionsManagerDialog::updateUi);

  this->connect(ui.expressions->selectionModel(), &QItemSelectionModel::selectionChanged, this,
    &pqExpressionsManagerDialog::updateUi);

  this->connect(ui.add, &QPushButton::clicked, this, &pqExpressionsManagerDialog::addNewExpression);
  this->connect(
    ui.useCurrent, &QPushButton::clicked, this, &pqExpressionsManagerDialog::onUseCurrent);
  this->connect(
    ui.remove, &QPushButton::clicked, this, &pqExpressionsManagerDialog::removeSelectedExpressions);
  this->connect(
    ui.removeAll, &QPushButton::clicked, this, &pqExpressionsManagerDialog::removeAllExpressions);
  this->connect(ui.close, &QPushButton::clicked, this, &pqExpressionsManagerDialog::onClose);
  this->connect(ui.save, &QPushButton::clicked, this, &pqExpressionsManagerDialog::save);
  this->connect(ui.cancel, &QPushButton::clicked, this, &pqExpressionsManagerDialog::close);
  this->connect(
    ui.importExpressions, &QPushButton::clicked, this, &pqExpressionsManagerDialog::importFromFile);
  this->connect(
    ui.exportExpressions, &QPushButton::clicked, this, &pqExpressionsManagerDialog::exportToFile);
  this->connect(ui.groupChooser, &QComboBox::currentTextChanged, this,
    &pqExpressionsManagerDialog::filterGroup);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->restoreState(pqExpressionsManager::SETTINGS_GROUP(), *this);
  QList<pqExpressionsManager::pqExpression> expressions =
    pqExpressionsManager::getExpressionsFromSettings();

  this->Internals->Model->setExpressions(expressions);

  this->Internals->Ui.groupChooser->setCurrentText(group);
  this->filterGroup();
  header->setSortIndicator(pqExpressionsTableModel::GroupColumn, Qt::AscendingOrder);
}

//----------------------------------------------------------------------------
pqExpressionsManagerDialog::~pqExpressionsManagerDialog() = default;

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::updateUi()
{
  const Ui::pqExpressionsManagerDialog& ui = this->Internals->Ui;
  QModelIndexList selectedRows = ui.expressions->selectionModel()->selectedRows();
  ui.remove->setEnabled(!selectedRows.empty());
  ui.removeAll->setEnabled(this->Internals->Model->rowCount(QModelIndex()) > 0);

  // useCurrent is visible if dialog was open from a property widget
  if (this->Internals->Group.isEmpty())
  {
    ui.useCurrent->hide();
  }
  else
  {
    // useCurrent is enabled if current group match the current property.
    bool canUseCurrent = selectedRows.size() == 1;
    if (canUseCurrent)
    {
      auto expr =
        this->Internals->Model->getGroup(this->Internals->mapToSource(selectedRows.first()));
      canUseCurrent = expr == this->Internals->Group;
    }
    ui.useCurrent->setEnabled(canUseCurrent);
  }
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::filterGroup()
{
  const Ui::pqExpressionsManagerDialog& ui = this->Internals->Ui;

  this->Internals->updateGroupFilter(ui.groupChooser->currentText());
  this->updateUi();
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::removeAllExpressions()
{
  QMessageBox dialog;
  dialog.setText("Remove all expressions ?");
  dialog.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  dialog.setDefaultButton(QMessageBox::Cancel);
  if (dialog.exec() == QMessageBox::Ok)
  {
    this->Internals->Model->removeAllExpressions();
  }
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::removeSelectedExpressions()
{
  const Ui::pqExpressionsManagerDialog& ui = this->Internals->Ui;
  QModelIndexList selectedRows = ui.expressions->selectionModel()->selectedRows();
  QModelIndexList selectedExpressions;
  for (auto index : selectedRows)
  {
    selectedExpressions << this->Internals->mapToSource(index);
  }

  if (!selectedExpressions.isEmpty())
  {
    this->Internals->Model->removeExpressions(selectedExpressions);

    ui.expressions->selectRow(
      std::min(selectedRows.first().row(), this->Internals->Model->rowCount(QModelIndex()) - 1));
  }
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::addNewExpression()
{
  QString group = this->Internals->Ui.groupChooser->currentText();
  if (group == ALL_GROUP_NAME)
  {
    group = DEFAULT_GROUP_NAME;
  }

  pqExpressionsManager::pqExpression newExpr = { group, "", "" };
  QModelIndex index = this->Internals->Model->expressionIndex(newExpr);
  if (!index.isValid())
  {
    index = this->Internals->Model->addNewExpression();
    this->Internals->Model->setExpressionGroup(index.row(), group);
  }

  this->Internals->updateFilter();

  // this->Internals->Ui.expressions->selectRow(this->Internals->mapFromSource(index).row());
  this->Internals->Ui.expressions->setCurrentIndex(this->Internals->mapFromSource(index));
  this->Internals->Ui.expressions->edit(this->Internals->mapFromSource(index));
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::onUseCurrent()
{
  Q_EMIT this->expressionSelected(this->Internals->selectedExpression());
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::save()
{
  QList<pqExpressionsManager::pqExpression> expressions = this->Internals->Model->getExpressions();
  pqExpressionsManager::storeToSettings(expressions);
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::onClose()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->saveState(*this, pqExpressionsManager::SETTINGS_GROUP());

  this->save();
  this->close();
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::exportToFile()
{
  pqFileDialog dialog(nullptr, this, tr("Export Expressions(s)"), QString(), BROWSE_FILE_FILTER);
  dialog.setObjectName("ExportExpressions");
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (dialog.exec() != QDialog::Accepted || dialog.getSelectedFiles().empty())
  {
    return;
  }

  vtksys::ofstream outfs;
  QString filename = dialog.getSelectedFiles()[0];
  outfs.open(filename.toUtf8());
  if (!outfs.is_open())
  {
    qCritical() << "Failed to open file for writing: " << filename;
    return;
  }

  json root;
  root[JSON_FILE_VERSION_KEY] = JSON_FILE_VERSION;
  json list = json::array();

  // add expressions
  auto expressions = this->Internals->Model->getExpressions();
  for (const auto& expression : expressions)
  {
    json expr;
    expr[JSON_EXPRESSION_KEY] = expression.Value.toStdString();
    expr[JSON_EXPRESSION_NAME_KEY] = expression.Name.toStdString();
    expr[JSON_EXPRESSION_GROUP_KEY] = expression.Group.toStdString();
    list.push_back(expr);
  }
  root[JSON_EXPRESSIONS_LIST_KEY] = list;

  outfs << root.dump(2) << endl;
  outfs.close();
}

//----------------------------------------------------------------------------
bool pqExpressionsManagerDialog::importFromFile()
{
  pqFileDialog dialog(nullptr, this, tr("Import Expressions(s)"), QString(), BROWSE_FILE_FILTER);
  dialog.setObjectName("ImportExpressions");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() != QDialog::Accepted || dialog.getSelectedFiles().empty())
  {
    return false;
  }

  QString filename = dialog.getSelectedFiles()[0];
  vtksys::ifstream file;
  file.open(filename.toUtf8());
  if (!file)
  {
    qWarning() << "Failed to open file: " << filename;
    return false;
  }
  json root;
  file >> root;

  QList<pqExpressionsManager::pqExpression> expressionsList;
  try
  {
    std::string fileVersion = root.at(JSON_FILE_VERSION_KEY);
    if (fileVersion != JSON_FILE_VERSION)
    {
      qWarning() << "File version is " << fileVersion.c_str() << " but reader is "
                 << JSON_FILE_VERSION << ", errors may occurs";
    }

    for (auto expression : root[JSON_EXPRESSIONS_LIST_KEY])
    {
      std::string exprValue = expression.at(JSON_EXPRESSION_KEY);
      std::string exprName = expression.at(JSON_EXPRESSION_NAME_KEY);
      std::string exprGroup = expression.at(JSON_EXPRESSION_GROUP_KEY);
      expressionsList.push_back({ exprGroup.c_str(), exprName.c_str(), exprValue.c_str() });
    }
  }
  catch (nlohmann::detail::out_of_range const& exception)
  {
    qCritical() << "Invalid expression file. " << exception.what()
                << "\nSee documentation for accepted format";
    return false;
  }
  catch (nlohmann::detail::type_error const& exception)
  {
    qCritical() << "Invalid expression file. " << exception.what()
                << "\nSee documentation for accepted format";
    return false;
  }

  this->Internals->Model->addExpressions(expressionsList);
  this->filterGroup();

  return true;
}

//----------------------------------------------------------------------------
void pqExpressionsManagerDialog::keyReleaseEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete)
  {
    this->removeSelectedExpressions();
  }

  this->Superclass::keyPressEvent(event);
}
