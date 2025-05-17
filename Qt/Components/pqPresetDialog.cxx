// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPresetDialog.h"
#include "ui_pqPresetDialog.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPresetGroupsManager.h"
#include "pqPresetToPixmap.h"
#include "pqPropertiesPanel.h"
#include "pqServer.h"

#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionPresets.h"

#include "vtk_jsoncpp.h"

#include <QInputEvent>
#include <QList>
#include <QMenu>
#include <QPixmap>
#include <QPointer>
#include <QRegularExpression>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QtDebug>

#include <cassert>

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define pqCheckBoxSignal checkStateChanged
using pqCheckState = Qt::CheckState;
#else
#define pqCheckBoxSignal stateChanged
using pqCheckState = int;
#endif

class pqPresetDialogTableModel : public QAbstractTableModel
{
  typedef QAbstractTableModel Superclass;
  pqPresetToPixmap PixmapRenderer;

  // 'mutable' allows us to avoid having to pregenerate all the pixmaps.
  mutable QList<QPixmap> Pixmaps;

  const QPixmap& pixmap(int row) const
  {
    this->Pixmaps.reserve(row + 1);

    // grow Pixmaps if needed.
    for (int cc = this->Pixmaps.size(); cc <= row; cc++)
    {
      this->Pixmaps.push_back(QPixmap());
    }
    if (this->Pixmaps[row].isNull())
    {
      this->Pixmaps[row] =
        this->PixmapRenderer.render(this->Presets->GetPreset(row), QSize(180, 20));
    }
    return this->Pixmaps[row];
  }

public:
  vtkSmartPointer<vtkSMTransferFunctionPresets> Presets;
  pqPresetGroupsManager* GroupManager;

  pqPresetDialogTableModel(QObject* parentObject)
    : Superclass(parentObject)
  {
    this->Presets = vtkSMTransferFunctionPresets::GetInstance();
    this->Pixmaps.reserve(this->Presets->GetNumberOfPresets());
    this->GroupManager = qobject_cast<pqPresetGroupsManager*>(
      pqApplicationCore::instance()->manager("PRESET_GROUP_MANAGER"));
    this->connect(this->GroupManager, &pqPresetGroupsManager::groupsUpdated, this,
      [this]()
      {
        this->beginResetModel();
        this->endResetModel();
      });
  }

  ~pqPresetDialogTableModel() override = default;

  void importPresets(
    const QString& filename, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/)
  {
    this->beginResetModel();
    std::vector<vtkSMTransferFunctionPresets::ImportedPreset> importedPresets;
    const bool imported =
      this->Presets->ImportPresets(filename.toStdString().c_str(), &importedPresets, location);
    if (imported)
    {
      for (auto const& importedPreset : importedPresets)
      {
        auto const presetName = QString::fromStdString(importedPreset.name);
        if (importedPreset.potentialGroups.isValid)
        {
          for (auto const& groupName : importedPreset.potentialGroups.groups)
          {
            this->GroupManager->addToGroup(QString::fromStdString(groupName), presetName);
          }
        }
        else
        {
          this->GroupManager->addToGroup("User", presetName);
          this->GroupManager->addToGroup("Default", presetName);
        }
      }
    }
    this->endResetModel();
  }

  void reset()
  {
    this->beginResetModel();
    this->Presets->ReloadPresets();
    this->Pixmaps.clear();
    this->Pixmaps.reserve(this->Presets->GetNumberOfPresets());
    this->endResetModel();
  }

  void removePreset(unsigned int idx)
  {
    this->beginRemoveRows(QModelIndex(), idx, idx);
    auto presetName = this->Presets->GetPresetName(idx);
    this->GroupManager->removeFromAllGroups(QString::fromStdString(presetName));
    this->Presets->RemovePreset(idx);
    this->Pixmaps.removeAt(idx);
    this->endRemoveRows();
  }

  QModelIndex indexFromName(const char* presetName) const
  {
    if (!presetName)
    {
      return QModelIndex();
    }
    for (int cc = 0, max = this->rowCount(QModelIndex()); cc < max; ++cc)
    {
      if (this->Presets->GetPresetName(cc) == presetName)
      {
        return this->index(cc, 0, QModelIndex());
      }
    }
    return QModelIndex();
  }

  int rowCount(const QModelIndex& idx) const override
  {
    return idx.isValid() ? 0 : static_cast<int>(this->Presets->GetNumberOfPresets());
  }

  int columnCount(const QModelIndex& /*parent*/) const override
  {
    return 1 + this->GroupManager->numberOfGroups();
  }

  QVariant data(const QModelIndex& idx, int role) const override
  {
    if (!idx.isValid() || idx.model() != this)
    {
      return QVariant();
    }

    if (idx.column() == 0)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::EditRole:
          return this->Presets->GetPresetName(idx.row()).c_str();

        case Qt::DecorationRole:
          return this->pixmap(idx.row());

        case Qt::UserRole:
          return this->Presets->GetPresetHasIndexedColors(idx.row());
        case Qt::FontRole:
          QFont font;
          // Find the column in this model for the default group.
          auto defaultColumn = this->GroupManager->groupNames().indexOf("Default") + 1;
          // if this is a default preset, bold and underline the name
          if (defaultColumn != 0 &&
            this->data(this->index(idx.row(), defaultColumn), Qt::DisplayRole) != -1)
          {
            font.setBold(true);
            font.setUnderline(true);
          }

          if (!this->Presets->IsPresetBuiltin(idx.row()))
          {
            font.setItalic(true);
          }

          return font;
      }
    }
    else // column > 0
    {
      switch (role)
      {
        case Qt::DisplayRole:
          auto groupName = this->GroupManager->groupName(idx.column() - 1);
          auto name = this->Presets->GetPresetName(idx.row());
          auto applicationGroupIndex =
            this->GroupManager->presetRankInGroup(name.c_str(), groupName);
          return applicationGroupIndex;
      }
    }
    return QVariant();
  }

  void addPresetToDefaults(const QModelIndex& idx)
  {
    if (!idx.isValid() || idx.model() != this || idx.column() != 0)
    {
      return;
    }

    QString presetName = this->Presets->GetPresetName(idx.row()).c_str();
    this->GroupManager->addToGroup("Default", presetName);
    auto changedIndexStart = this->index(idx.row(), 0);
    auto changedIndexEnd = this->index(idx.row(), this->columnCount(idx) - 1);
    Q_EMIT this->dataChanged(changedIndexStart, changedIndexEnd);
  }

  void removePresetFromDefaults(const QModelIndex& idx)
  {
    if (!idx.isValid() || idx.model() != this || idx.column() != 0)
    {
      return;
    }

    QString presetName = this->Presets->GetPresetName(idx.row()).c_str();
    this->GroupManager->removeFromGroup("Default", presetName);
    auto changedIndexStart = this->index(idx.row(), 0);
    auto changedIndexEnd = this->index(idx.row(), this->columnCount(idx) - 1);
    Q_EMIT this->dataChanged(changedIndexStart, changedIndexEnd);
  }

  bool setData(const QModelIndex& idx, const QVariant& value, int role) override
  {
    Q_UNUSED(role);
    if (!idx.isValid() || idx.model() != this || idx.column() != 0)
    {
      return false;
    }

    std::string const previousName = this->Presets->GetPresetName(idx.row());
    std::string const newName = value.toString().toStdString();
    if (this->Presets->RenamePreset(idx.row(), newName.c_str()))
    {
      auto const previousNameQString = QString::fromStdString(previousName);
      auto const newNameQString = QString::fromStdString(newName);
      QStringList groupNames;

      for (auto const& groupName : this->GroupManager->groupNames())
      {
        if (this->GroupManager->presetRankInGroup(previousNameQString, groupName) != -1)
        {
          groupNames.push_back(groupName);
        }
      }

      this->GroupManager->removeFromAllGroups(previousNameQString);
      for (auto const& groupName : groupNames)
      {
        this->GroupManager->addToGroup(groupName, newNameQString);
      }
      Q_EMIT this->dataChanged(idx, idx);
      return true;
    }
    return true;
  }

  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    Qt::ItemFlags flgs = this->Superclass::flags(idx);
    if (!idx.isValid() || idx.model() != this)
    {
      return flgs;
    }
    // mark non-builtin presets as editable.
    return this->Presets->IsPresetBuiltin(idx.row()) ? flgs : (flgs | Qt::ItemIsEditable);
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    Q_UNUSED(section);
    if (orientation == Qt::Vertical)
    {
      return QVariant();
    }
    switch (role)
    {
      case Qt::DisplayRole:
        return "Presets";
    }
    return QVariant();
  }

private:
  Q_DISABLE_COPY(pqPresetDialogTableModel)
};

class pqPresetDialogProxyModel : public QSortFilterProxyModel
{
  typedef QSortFilterProxyModel Superclass;
  pqPresetDialog::Modes Mode;
  int CurrentGroupColumn;

public:
  pqPresetDialogProxyModel(pqPresetDialog::Modes m, QObject* parentObject = nullptr)
    : Superclass(parentObject)
    , Mode(m)
    , CurrentGroupColumn(0)
  {
  }
  ~pqPresetDialogProxyModel() override = default;

  pqPresetDialog::Modes mode() { return this->Mode; }
  void setMode(pqPresetDialog::Modes m)
  {
    this->Mode = m;
    this->invalidateFilter();
  }

  int currentGroupColumn() const { return this->CurrentGroupColumn; }
  void setCurrentGroupColumn(int currentGroupCol)
  {
    this->CurrentGroupColumn = currentGroupCol;
    this->invalidateFilter();
    this->sort(currentGroupCol == 0 ? -1 : currentGroupCol);
  }

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
  {
    if (!this->Superclass::filterAcceptsRow(sourceRow, sourceParent))
    {
      return false;
    }
    auto* source = this->sourceModel();
    if (this->CurrentGroupColumn != 0)
    {
      QModelIndex positionIdx = source->index(sourceRow, this->CurrentGroupColumn, sourceParent);
      if (this->sourceModel()->data(positionIdx, Qt::DisplayRole).toInt() < 0)
      {
        return false;
      }
    }

    QModelIndex idx = source->index(sourceRow, 0, sourceParent);
    switch (this->Mode)
    {
      case pqPresetDialog::SHOW_ALL:
        return true;

      case pqPresetDialog::SHOW_INDEXED_COLORS_ONLY:
        return source->data(idx, Qt::UserRole).toBool();

      case pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY:
        return !source->data(idx, Qt::UserRole).toBool();
    }
    return false;
  }

private:
  Q_DISABLE_COPY(pqPresetDialogProxyModel)
};

class pqPresetDialogReflowModel : public QAbstractProxyModel
{
  typedef QAbstractProxyModel Superclass;
  int NumColumns;

public:
  pqPresetDialogReflowModel(int cols, QObject* parentObject = nullptr)
    : Superclass(parentObject)
    , NumColumns(cols)
  {
  }
  ~pqPresetDialogReflowModel() override = default;

  QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override
  {
    return this->index(sourceIndex.row() / this->NumColumns, sourceIndex.row() % this->NumColumns);
  }

  QModelIndex mapToSource(const QModelIndex& proxyIndex) const override
  {
    auto resultRow = proxyIndex.row() * this->NumColumns + proxyIndex.column();
    if (resultRow < sourceModel()->rowCount())
    {
      return sourceModel()->index(resultRow, 0);
    }
    // We add dummy items to make sure there are numColumns items in the last row even
    // if the source's number of rows isn't a multiple of the number of columns.
    return QModelIndex();
  }

  int rowCount(const QModelIndex& parent = QModelIndex()) const override
  {
    auto parentRowCount = sourceModel()->rowCount(parent);
    return parentRowCount / this->NumColumns + (parentRowCount % this->NumColumns == 0 ? 0 : 1);
  }

  int columnCount(const QModelIndex& = QModelIndex()) const override { return this->NumColumns; }

  QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const override
  {
    auto resultRow = proxyIndex.row() * this->NumColumns + proxyIndex.column();
    if (resultRow < sourceModel()->rowCount())
    {
      return Superclass::data(proxyIndex, role);
    }
    else
    {
      return QVariant();
    }
  }

  Qt::ItemFlags flags(const QModelIndex& proxyIndex) const override
  {
    auto resultRow = proxyIndex.row() * this->NumColumns + proxyIndex.column();
    if (resultRow < sourceModel()->rowCount())
    {
      return Superclass::flags(proxyIndex);
    }
    else
    {
      return Superclass::flags(proxyIndex) & ~Qt::ItemIsSelectable;
    }
  }

  QModelIndex index(int row, int column, const QModelIndex& = QModelIndex()) const override
  {
    return createIndex(row, column);
  }

  QModelIndex parent(const QModelIndex&) const override { return QModelIndex(); }

private:
  Q_DISABLE_COPY(pqPresetDialogReflowModel);
};

class pqPresetDialog::pqInternals
{
public:
  Ui::pqPresetDialog Ui;
  QPointer<pqPresetDialogTableModel> Model;
  QPointer<pqPresetDialogProxyModel> ProxyModel;
  QPointer<pqPresetDialogReflowModel> ReflowModel;
  int CurrentGroupColumn;

  pqInternals(pqPresetDialog::Modes mode, pqPresetDialog* self)
    : Model(new pqPresetDialogTableModel(self))
    , ProxyModel(new pqPresetDialogProxyModel(mode, self))
    , ReflowModel(new pqPresetDialogReflowModel(2, self))
  {
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->ProxyModel->setSourceModel(this->Model);
    this->ProxyModel->setFilterKeyColumn(0);
    this->ProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    this->ProxyModel->connect(this->Ui.searchBox, SIGNAL(textChanged(const QString&)),
      SLOT(setFilterWildcard(const QString&)));
    this->ReflowModel->setSourceModel(this->ProxyModel);

    this->CurrentGroupColumn = 0;

    // Signals required for the reflow model to work correctly.
    // Everything becomes a model reset because we are breaking
    // Qt's assumptions about how rows should work
    QObject::connect(this->ProxyModel, SIGNAL(modelAboutToBeReset()), this->ReflowModel,
      SIGNAL(modelAboutToBeReset()));
    QObject::connect(
      this->ProxyModel, SIGNAL(modelReset()), this->ReflowModel, SIGNAL(modelReset()));
    QObject::connect(this->ProxyModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)),
      this->ReflowModel, SIGNAL(modelAboutToBeReset()));
    QObject::connect(this->ProxyModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
      this->ReflowModel, SIGNAL(modelReset()));
    QObject::connect(this->ProxyModel,
      SIGNAL(rowsAboutToBeMoved(const QModelIndex&, int, int, const QModelIndex&, int)),
      this->ReflowModel, SIGNAL(modelAboutToBeReset()));
    QObject::connect(this->ProxyModel,
      SIGNAL(rowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)), this->ReflowModel,
      SIGNAL(modelReset()));
    QObject::connect(this->ProxyModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
      this->ReflowModel, SIGNAL(modelAboutToBeReset()));
    QObject::connect(this->ProxyModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
      this->ReflowModel, SIGNAL(modelReset()));

    this->Ui.gradients->setModel(this->ReflowModel);
    this->Ui.gradients->setSelectionBehavior(QAbstractItemView::SelectItems);
    // This makes the two columns share the width of the table as the dialog is resized
    this->Ui.gradients->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Make the vertical spacing bigger
    this->Ui.gradients->verticalHeader()->setDefaultSectionSize(
      (int)(this->Ui.gradients->verticalHeader()->defaultSectionSize() * 1.5));

    QObject::connect(
      this->Ui.useRegexp, &QCheckBox::pqCheckBoxSignal, [&]() { this->updateRegexpWidgets(); });
    QObject::connect(
      this->Ui.annotations, &QCheckBox::pqCheckBoxSignal, [&]() { this->updateRegexpWidgets(); });
  }

  void updateRegexpWidgets()
  {
    this->Ui.useRegexp->setEnabled(this->Ui.annotations->isEnabled());
    this->Ui.regexpLine->setEnabled(
      this->Ui.useRegexp->isEnabled() && this->Ui.useRegexp->isChecked());
  }

  void setMode(Modes mode)
  {
    this->ProxyModel->setMode(mode);
    // For now groups are only for non-indexed colors mode.  I think eventually
    // we will want to make indexed presets a group and continuous presets a group
    // and unify the modes, but that is for another future branch.
    this->Ui.groupChooser->setEnabled(mode != SHOW_INDEXED_COLORS_ONLY);
    if (mode == SHOW_INDEXED_COLORS_ONLY)
    {
      this->ProxyModel->setCurrentGroupColumn(0);
    }
    else
    {
      auto groupMgr = this->Model->GroupManager;
      auto defaultColumn = groupMgr->groupNames().indexOf("Default") + 1;
      this->Ui.groupChooser->setCurrentIndex(defaultColumn);
      this->CurrentGroupColumn = defaultColumn;
    }
    this->Ui.gradients->selectionModel()->clear();
  }

  void resetModel() { this->Model->reset(); }
};

//-----------------------------------------------------------------------------
pqPresetDialog::pqPresetDialog(QWidget* parentObject, pqPresetDialog::Modes mode)
  // Set the hints to place in Qt::Tool layer, same as dock widgets, so this doesn't stay behind
  // docks on MacOs
  : Superclass(parentObject,
      Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint | Qt::Tool)
  , Internals(new pqPresetDialog::pqInternals(mode, this))
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  this->connect(ui.gradients->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
    SLOT(updateEnabledStateForSelection()));
  this->updateEnabledStateForSelection();

  this->connect(ui.remove, SIGNAL(clicked()), SLOT(removePreset()));
  this->connect(ui.gradients, SIGNAL(doubleClicked(const QModelIndex&)),
    SLOT(triggerApply(const QModelIndex&)));
  this->connect(ui.apply, SIGNAL(clicked()), SLOT(triggerApply()));
  this->connect(ui.importPresets, SIGNAL(clicked()), SLOT(importPresets()));
  this->connect(ui.exportPresets, SIGNAL(clicked()), SLOT(exportPresets()));
  this->connect(ui.groupChooser,
    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
    [&](int index)
    {
      this->Internals->ProxyModel->setCurrentGroupColumn(index);
      this->updateEnabledStateForSelection();
      this->Internals->CurrentGroupColumn = index;
    });
  this->Internals->ProxyModel->connect(
    ui.searchBox, &pqSearchBox::textChanged, this, &pqPresetDialog::updateFiltering);
  QObject::connect(ui.showDefault, &QCheckBox::pqCheckBoxSignal, this,
    [&](pqCheckState state) { this->setPresetIsAdvanced(static_cast<Qt::CheckState>(state)); });
  auto groupMgr = this->Internals->Model->GroupManager;
  this->connect(
    groupMgr, &pqPresetGroupsManager::groupsUpdated, this, &pqPresetDialog::updateGroups);
  this->connect(this, SIGNAL(rejected()), SLOT(close()));
  this->updateGroups();
}

//-----------------------------------------------------------------------------
pqPresetDialog::~pqPresetDialog() = default;

//-----------------------------------------------------------------------------
void pqPresetDialog::updateGroups()
{
  auto groupChooser = this->Internals->Ui.groupChooser;
  QString const currentGroup = groupChooser->currentText();
  groupChooser->clear();
  groupChooser->insertItem(0, "All");
  auto groupMgr = this->Internals->Model->GroupManager;
  auto groupList = groupMgr->groupNames();
  groupChooser->insertItems(1, groupList);
  if (this->Internals->ProxyModel->mode() == SHOW_INDEXED_COLORS_ONLY)
  {
    this->Internals->ProxyModel->setCurrentGroupColumn(0);
  }
  else
  {
    int const currentGroupIndex = groupChooser->findText(currentGroup);

    if (currentGroupIndex != -1)
    {
      groupChooser->setCurrentIndex(currentGroupIndex);
      this->Internals->CurrentGroupColumn = currentGroupIndex;
    }
    else
    {
      auto defaultColumn = groupMgr->groupNames().indexOf("Default") + 1;
      this->Internals->Ui.groupChooser->setCurrentIndex(defaultColumn);
      this->Internals->CurrentGroupColumn = defaultColumn;
    }
  }

  this->updateFiltering();
}

//-----------------------------------------------------------------------------
void pqPresetDialog::updateFiltering()
{
  QString const text = this->Internals->Ui.searchBox->text();

  if (!text.isEmpty())
  {
    this->Internals->ProxyModel->setCurrentGroupColumn(0);
    this->updateEnabledStateForSelection();
    this->Internals->Ui.groupChooser->setEnabled(false);
  }
  else
  {
    this->Internals->ProxyModel->setCurrentGroupColumn(this->Internals->CurrentGroupColumn);
    this->updateEnabledStateForSelection();
    this->Internals->Ui.groupChooser->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setMode(Modes mode)
{
  this->Internals->setMode(mode);
}
//-----------------------------------------------------------------------------
void pqPresetDialog::setCustomizableLoadColors(bool state, bool defaultValue)
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  ui.colors->setVisible(state);
  ui.colors->setChecked(defaultValue);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setCustomizableLoadAnnotations(bool state, bool defaultValue)
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  ui.annotations->setVisible(state);
  ui.annotations->setChecked(defaultValue);
  this->Internals->updateRegexpWidgets();
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setCustomizableAnnotationsRegexp(bool state, bool defaultValue)
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  ui.useRegexp->setVisible(state);
  ui.regexpLine->setVisible(state);
  ui.useRegexp->setChecked(defaultValue);
  ui.regexpLine->setEnabled(defaultValue);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setCustomizableLoadOpacities(bool state, bool defaultValue)
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  ui.opacities->setVisible(state);
  ui.opacities->setChecked(defaultValue);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setCustomizableUsePresetRange(bool state, bool defaultValue)
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  ui.usePresetRange->setVisible(state);
  ui.usePresetRange->setChecked(defaultValue);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setCurrentPreset(const char* presetName)
{
  pqInternals& internals = (*this->Internals);
  // since the preset dialog is persistent and a new preset could have been added,
  // reset the loaded presets.
  if (presetName)
  {
    internals.resetModel();
  }
  QModelIndex idx = internals.Model->indexFromName(presetName);
  if (!idx.isValid())
  {
    internals.Ui.gradients->selectionModel()->clear();
    return;
  }
  auto newIdx = internals.ProxyModel->mapFromSource(idx);
  newIdx = internals.ReflowModel->mapFromSource(newIdx);
  if (!newIdx.isValid() && internals.CurrentGroupColumn != 0)
  {
    // If the requested preset is not in the default list, trigger the show advanced button and try
    // to get the index again.  Since the return above was not triggered we know the index should be
    // valid with the advanced maps showing.
    internals.Ui.groupChooser->setCurrentIndex(0);
    idx = internals.ProxyModel->mapFromSource(idx);
    idx = internals.ReflowModel->mapFromSource(idx);
  }
  else
  {
    idx = newIdx;
  }
  if (idx.isValid())
  {
    internals.Ui.gradients->selectionModel()->setCurrentIndex(
      idx, QItemSelectionModel::ClearAndSelect);
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::updateEnabledStateForSelection()
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  QModelIndexList selectedRows = ui.gradients->selectionModel()->selectedIndexes();
  if (selectedRows.size() == 1)
  {
    this->updateForSelectedIndex(selectedRows[0]);
  }
  else
  {
    ui.colors->setEnabled(false);
    ui.usePresetRange->setEnabled(false);
    ui.opacities->setEnabled(false);
    ui.annotations->setEnabled(false);
    ui.apply->setEnabled(false);
    ui.exportPresets->setEnabled(!selectedRows.empty());
    ui.showDefault->setEnabled(false);

    bool isEditable = true;
    Q_FOREACH (const QModelIndex& idx, selectedRows)
    {
      isEditable &= this->Internals->ProxyModel->flags(idx).testFlag(Qt::ItemIsEditable);
    }
    ui.remove->setEnabled((!selectedRows.empty()) && isEditable);
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::updateForSelectedIndex(const QModelIndex& proxyIndex)
{
  // update "options" based on what's available.
  const pqInternals& internals = *this->Internals;
  QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
  idx = internals.ProxyModel->mapToSource(idx);
  const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
  assert(preset.empty() == false);
  auto column = internals.Model->GroupManager->groupNames().indexOf("Default") + 1;
  QModelIndex defaultColumnIndex = internals.Model->index(idx.row(), column);

  int defaultPosition = internals.Model->data(defaultColumnIndex, Qt::DisplayRole).toInt();

  const Ui::pqPresetDialog& ui = internals.Ui;

  ui.showDefault->setEnabled(true);
  ui.showDefault->setChecked(defaultPosition != -1);
  ui.colors->setEnabled(true);
  ui.usePresetRange->setEnabled(!internals.Model->Presets->GetPresetHasIndexedColors(preset));
  ui.opacities->setEnabled(internals.Model->Presets->GetPresetHasOpacities(preset));
  ui.annotations->setEnabled(internals.Model->Presets->GetPresetHasAnnotations(preset));
  this->Internals->updateRegexpWidgets();
  ui.apply->setEnabled(true);
  ui.exportPresets->setEnabled(true);
  ui.remove->setEnabled(internals.Model->flags(idx).testFlag(Qt::ItemIsEditable));
}

//-----------------------------------------------------------------------------
void pqPresetDialog::triggerApply(const QModelIndex& _proxyIndex)
{
  const pqInternals& internals = *this->Internals;
  const Ui::pqPresetDialog& ui = this->Internals->Ui;

  const QModelIndex proxyIndex =
    _proxyIndex.isValid() ? _proxyIndex : ui.gradients->selectionModel()->currentIndex();
  QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
  idx = internals.ProxyModel->mapToSource(idx);
  const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
  if (preset.empty() == false)
  {
    Q_EMIT this->applyPreset(preset);
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::removePreset(const QModelIndex& _proxyIndex)
{
  const pqInternals& internals = *this->Internals;
  const Ui::pqPresetDialog& ui = this->Internals->Ui;

  QModelIndexList selectedRows;
  if (_proxyIndex.isValid())
  {
    selectedRows.push_back(_proxyIndex);
  }
  else
  {
    selectedRows = ui.gradients->selectionModel()->selectedIndexes();
  }
  for (int cc = (selectedRows.size() - 1); cc >= 0; cc--)
  {
    const QModelIndex& proxyIndex = selectedRows[cc];
    QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
    idx = internals.ProxyModel->mapToSource(idx);
    internals.Model->removePreset(idx.row());
  }
}

//-----------------------------------------------------------------------------
const Json::Value& pqPresetDialog::currentPreset()
{
  const pqInternals& internals = *this->Internals;
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  QModelIndex proxyIndex = ui.gradients->selectionModel()->currentIndex();
  if (proxyIndex.isValid())
  {
    QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
    idx = internals.ProxyModel->mapToSource(idx);
    const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
    assert(preset.empty() == false);
    return preset;
  }

  static Json::Value nullValue;
  return nullValue;
}

//-----------------------------------------------------------------------------
bool pqPresetDialog::loadColors() const
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  return ui.colors->isEnabled() ? ui.colors->isChecked() : false;
}

//-----------------------------------------------------------------------------
bool pqPresetDialog::loadOpacities() const
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  return ui.opacities->isEnabled() ? ui.opacities->isChecked() : false;
}

//-----------------------------------------------------------------------------
bool pqPresetDialog::loadAnnotations() const
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  return ui.annotations->isEnabled() ? ui.annotations->isChecked() : false;
}

//-----------------------------------------------------------------------------
bool pqPresetDialog::usePresetRange() const
{
  const Ui::pqPresetDialog& ui = this->Internals->Ui;
  return ui.usePresetRange->isEnabled() ? ui.usePresetRange->isChecked() : false;
}
//-----------------------------------------------------------------------------
void pqPresetDialog::importPresets()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  pqFileDialog dialog(server, this, tr("Import Presets"), QString(),
    tr("Supported Presets/Color Map Files") + QString(" (*.json *.xml *.ct);;") +
      tr("ParaView Color/Opacity Presets") + QString(" (*.json);;") + tr("Legacy Color Maps") +
      QString(" (*.xml);;") + tr("VisIt Color Table") + QString(" (*.ct);;") + tr("All Files") +
      QString(" (*)"),
    false, false);
  dialog.setObjectName("ImportPresets");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted && !dialog.getSelectedFiles().empty())
  {
    QString filename = dialog.getSelectedFiles()[0];
    vtkTypeUInt32 location = dialog.getSelectedLocation();
    const pqInternals& internals = *this->Internals;
    int oldCount = internals.Model->rowCount(QModelIndex());
    internals.Model->importPresets(filename, location);
    int newCount = internals.Model->rowCount(QModelIndex());

    // highlight the newly imported presets for the user.
    if (oldCount < newCount)
    {
      QModelIndex startProxyIdx =
        internals.ProxyModel->mapFromSource(internals.Model->index(oldCount, 0, QModelIndex()));
      startProxyIdx = internals.ReflowModel->mapFromSource(startProxyIdx);
      QModelIndex endProxyIdx =
        internals.ProxyModel->mapFromSource(internals.Model->index(newCount - 1, 0, QModelIndex()));
      endProxyIdx = internals.ReflowModel->mapFromSource(endProxyIdx);

      QItemSelection selection(startProxyIdx, endProxyIdx);
      internals.Ui.gradients->selectionModel()->select(
        selection, QItemSelectionModel::ClearAndSelect);
      internals.Ui.gradients->selectionModel()->setCurrentIndex(
        startProxyIdx, QItemSelectionModel::NoUpdate);
      internals.Ui.gradients->scrollTo(startProxyIdx, QAbstractItemView::PositionAtTop);
    }
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::exportPresets()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  pqFileDialog dialog(server, this, tr("Export Preset(s)"), QString(),
    tr("ParaView Color/Opacity Presets") + QString(" (*.json);;") + tr("All Files") +
      QString(" (*)"),
    false, false);
  dialog.setObjectName("ExportPresets");
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (dialog.exec() != QDialog::Accepted || dialog.getSelectedFiles().empty())
  {
    return;
  }

  QString filename = dialog.getSelectedFiles()[0];
  vtkTypeUInt32 location = dialog.getSelectedLocation();
  const pqInternals& internals = *this->Internals;
  const Ui::pqPresetDialog& ui = this->Internals->Ui;

  Json::Value presetCollection(Json::arrayValue);
  QModelIndexList selectedRows = ui.gradients->selectionModel()->selectedIndexes();
  Q_FOREACH (const QModelIndex& proxyIndex, selectedRows)
  {
    QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
    idx = internals.ProxyModel->mapToSource(idx);
    const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
    presetCollection.append(preset);
  }
  assert(presetCollection.size() > 0);

  auto pxm = server->proxyManager();
  if (!pxm->SaveString(
        presetCollection.toStyledString().c_str(), filename.toStdString().c_str(), location))
  {
    qCritical() << tr("Failed to save preset collection to ") << filename;
  }
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setPresetIsAdvanced(Qt::CheckState newState)
{
  bool showByDefault = newState == Qt::Checked;
  const pqInternals& internals = *this->Internals;
  QModelIndexList selectedRows = internals.Ui.gradients->selectionModel()->selectedIndexes();
  if (selectedRows.size() != 1)
  {
    // This doesn't support toggling multiple at once for now.
    // This is more of a sanity check since the checkbox should be disabled anyway.
    return;
  }
  QModelIndex idx = selectedRows[0];

  idx = internals.ReflowModel->mapToSource(idx);
  idx = internals.ProxyModel->mapToSource(idx);

  auto defaultColumn = internals.Model->GroupManager->groupNames().indexOf("Default") + 1;
  QModelIndex defaultColumnIndex = internals.Model->index(idx.row(), defaultColumn);

  int defaultPosition = internals.Model->data(defaultColumnIndex, Qt::DisplayRole).toInt();

  if (showByDefault && defaultPosition == -1)
  {
    internals.Model->addPresetToDefaults(idx);
    internals.Ui.gradients->update(selectedRows[0]);
    internals.Ui.gradients->setCurrentIndex(selectedRows[0]);
  }
  else if (!showByDefault && defaultPosition != -1)
  {
    internals.Model->removePresetFromDefaults(idx);
    internals.Ui.gradients->update(selectedRows[0]);
    if (this->Internals->CurrentGroupColumn != defaultColumn)
    {
      internals.Ui.gradients->setCurrentIndex(selectedRows[0]);
    }
  }
}

//-----------------------------------------------------------------------------
QRegularExpression pqPresetDialog::regularExpression()
{
  if (!this->Internals->Ui.useRegexp->isChecked())
  {
    // create a simple invalid regex
    return QRegularExpression("[");
  }

  auto regexp = QRegularExpression(this->Internals->Ui.regexpLine->text());
  if (!regexp.isValid())
  {
    qWarning() << "invalid regular expression: `" << regexp.pattern().toStdString().c_str() << "`";
  }

  return regexp;
}
