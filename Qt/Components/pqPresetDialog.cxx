/*=========================================================================

   Program: ParaView
   Module:  pqPresetDialog.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqPresetDialog.h"
#include "ui_pqPresetDialog.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqPresetGroupsManager.h"
#include "pqPresetToPixmap.h"
#include "pqPropertiesPanel.h"
#include "pqQVTKWidget.h"
#include "pqSettings.h"
#include "vtkNew.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"

#include <vtk_jsoncpp.h>

#include <QInputEvent>
#include <QList>
#include <QMenu>
#include <QPixmap>
#include <QPointer>
#include <QRegExp>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QtDebug>

#include <cassert>

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
    this->Presets = vtkSmartPointer<vtkSMTransferFunctionPresets>::New();
    this->Pixmaps.reserve(this->Presets->GetNumberOfPresets());
    this->GroupManager = qobject_cast<pqPresetGroupsManager*>(
      pqApplicationCore::instance()->manager("PRESET_GROUP_MANAGER"));
    this->connect(this->GroupManager, &pqPresetGroupsManager::groupsUpdated, this, [this]() {
      this->beginResetModel();
      this->endResetModel();
    });
  }

  ~pqPresetDialogTableModel() override {}

  void importPresets(const QString& filename)
  {
    this->beginResetModel();
    this->Presets->ImportPresets(filename.toStdString().c_str());
    this->endResetModel();
  }

  void reset()
  {
    this->beginResetModel();
    this->Presets = vtkSmartPointer<vtkSMTransferFunctionPresets>::New();
    this->Pixmaps.clear();
    this->Pixmaps.reserve(this->Presets->GetNumberOfPresets());
    this->endResetModel();
  }

  void removePreset(unsigned int idx)
  {
    this->beginRemoveRows(QModelIndex(), idx, idx);
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
          auto column = this->GroupManager->groupNames().indexOf("default") + 1;
          // if this is a default preset, bold and underline the name
          if (column != 0 && this->data(this->index(idx.row(), column), Qt::DisplayRole) != -1)
          {
            font.setBold(true);
            font.setUnderline(true);
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
          if (groupName != "default")
          {
            return applicationGroupIndex;
          }
          else
          {
            pqSettings settings;
            auto userChosenPresets =
              settings.value("pqSettingdDialog/userChosenPresets", QStringList()).toStringList();
            auto presetIdx = userChosenPresets.indexOf(QRegExp(QRegExp::escape(name.c_str())));
            return (presetIdx != -1)
              ? presetIdx + this->GroupManager->numberOfPresetsInGroup(groupName)
              : applicationGroupIndex;
          }
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
    pqSettings settings;
    auto userChosenPresets =
      settings.value("pqSettingdDialog/userChosenPresets", QStringList()).toStringList();
    if (!userChosenPresets.contains(presetName))
    {
      userChosenPresets.push_back(presetName);
      settings.setValue("pqSettingdDialog/userChosenPresets", userChosenPresets);
      auto changedIndexStart = this->index(idx.row(), 0);
      auto changedIndexEnd = this->index(idx.row(), this->columnCount(idx) - 1);
      emit this->dataChanged(changedIndexStart, changedIndexEnd);
    }
  }

  void removePresetFromDefaults(const QModelIndex& idx)
  {
    if (!idx.isValid() || idx.model() != this || idx.column() != 0)
    {
      return;
    }
    QString presetName = this->Presets->GetPresetName(idx.row()).c_str();
    pqSettings settings;
    auto userChosenPresets =
      settings.value("pqSettingdDialog/userChosenPresets", QStringList()).toStringList();
    if (userChosenPresets.contains(presetName))
    {
      userChosenPresets.removeOne(presetName);
      settings.setValue("pqSettingdDialog/userChosenPresets", userChosenPresets);
      auto changedIndexStart = this->index(idx.row(), 0);
      auto changedIndexEnd = this->index(idx.row(), this->columnCount(idx) - 1);
      emit this->dataChanged(changedIndexStart, changedIndexEnd);
    }
  }

  bool setData(const QModelIndex& idx, const QVariant& value, int role) override
  {
    Q_UNUSED(role);
    if (!idx.isValid() || idx.model() != this || idx.column() != 0)
    {
      return false;
    }

    if (this->Presets->RenamePreset(idx.row(), value.toString().toStdString().c_str()))
    {
      emit this->dataChanged(idx, idx);
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
  pqPresetDialogProxyModel(pqPresetDialog::Modes m, QObject* parentObject = NULL)
    : Superclass(parentObject)
    , Mode(m)
    , CurrentGroupColumn(0)
  {
  }
  ~pqPresetDialogProxyModel() override {}

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
  ~pqPresetDialogReflowModel() override {}

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

namespace
{
class pqPresetDialogFakeModality : public QObject
{
public:
  pqPresetDialogFakeModality(pqPresetDialog* p)
    : QObject(p)
    , Self(p)
  {
  }
  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (!dynamic_cast<QInputEvent*>(event))
    {
      // if the event is not an input event let it through
      return false;
    }
    // The event filter is called multiple times for each event.  The first time
    // is for the Window the event is within.  We have to let theset through so
    // that the event filter will be called again on the event with the widget
    // that will recieve it.
    //
    // VTK gets events from the QVTKOpenGLWidow so we don't even need to let the
    // pqQVTKWidget accept events.  This prevents it from popping up a context
    // menu too.
    if (obj->inherits("QWidgetWindow") || obj->inherits("QVTKOpenGLWindow"))
    {
      return false;
    }
    while (obj != NULL)
    {
      if (obj == this->Self)
      {
        // if the event is on a child of the preset dialog let it go through
        return false;
      }
      else if (obj->inherits("QMenu"))
      {
        // Future proofing: it is really bad if you have a context menu up and
        // all events to it are blocked.  It locks X completely.
        return false;
      }
      else if (obj->inherits("QListView") && obj->parent() == nullptr)
      {
        // This is the list that pops up in the export/import file dialog to
        // complete the filename you have started typing.  Similar to the QMenu,
        // if not handled it locks X completely.
        return false;
      }
      obj = obj->parent();
    }
    return true;
  }

private:
  pqPresetDialog* Self;
};
}

class pqPresetDialog::pqInternals
{
public:
  Ui::pqPresetDialog Ui;
  QPointer<pqPresetDialogTableModel> Model;
  QPointer<pqPresetDialogProxyModel> ProxyModel;
  QPointer<pqPresetDialogReflowModel> ReflowModel;
  QScopedPointer<QObject> EventFilter;
  int CurrentGroupColumn;

  pqInternals(pqPresetDialog::Modes mode, pqPresetDialog* self)
    : Model(new pqPresetDialogTableModel(self))
    , ProxyModel(new pqPresetDialogProxyModel(mode, self))
    , ReflowModel(new pqPresetDialogReflowModel(2, self))
    , EventFilter(new pqPresetDialogFakeModality(self))
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
      auto defaultColumn = groupMgr->groupNames().indexOf("default") + 1;
      this->Ui.groupChooser->setCurrentIndex(defaultColumn);
      this->CurrentGroupColumn = defaultColumn;
    }
    this->Ui.gradients->selectionModel()->clear();
  }

  void resetModel() { this->Model->reset(); }
};

//-----------------------------------------------------------------------------
pqPresetDialog::pqPresetDialog(QWidget* parentObject, pqPresetDialog::Modes mode)
  : Superclass(parentObject)
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
    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [&](int index) {
      this->Internals->ProxyModel->setCurrentGroupColumn(index);
      this->updateEnabledStateForSelection();
      this->Internals->CurrentGroupColumn = index;
    });
  this->Internals->ProxyModel->connect(
    ui.searchBox, &pqSearchBox::textChanged, [this](const QString& text) {
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
    });
  this->connect(ui.showDefault, SIGNAL(stateChanged(int)), SLOT(setPresetIsAdvanced(int)));
  auto groupMgr = this->Internals->Model->GroupManager;
  this->connect(
    groupMgr, &pqPresetGroupsManager::groupsUpdated, this, &pqPresetDialog::updateGroups);
  this->connect(this, SIGNAL(rejected()), SLOT(close()));
  this->updateGroups();
}

//-----------------------------------------------------------------------------
pqPresetDialog::~pqPresetDialog()
{
}

//-----------------------------------------------------------------------------
void pqPresetDialog::showEvent(QShowEvent* e)
{
  QApplication::instance()->installEventFilter(this->Internals->EventFilter.data());
  QDialog::showEvent(e);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::closeEvent(QCloseEvent* e)
{
  QApplication::instance()->removeEventFilter(this->Internals->EventFilter.data());
  QDialog::closeEvent(e);
}

//-----------------------------------------------------------------------------
void pqPresetDialog::updateGroups()
{
  auto groupChooser = this->Internals->Ui.groupChooser;
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
    auto defaultColumn = groupMgr->groupNames().indexOf("default") + 1;
    this->Internals->Ui.groupChooser->setCurrentIndex(defaultColumn);
    this->Internals->CurrentGroupColumn = defaultColumn;
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
    ui.exportPresets->setEnabled(selectedRows.size() > 0);
    ui.showDefault->setEnabled(false);

    bool isEditable = true;
    foreach (const QModelIndex& idx, selectedRows)
    {
      isEditable &= this->Internals->ProxyModel->flags(idx).testFlag(Qt::ItemIsEditable);
    }
    ui.remove->setEnabled((selectedRows.size() > 0) && isEditable);
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
  auto column = internals.Model->GroupManager->groupNames().indexOf("default") + 1;
  QModelIndex defaultColumnIndex = internals.Model->index(idx.row(), column);

  int defaultPosition = internals.Model->data(defaultColumnIndex, Qt::DisplayRole).toInt();
  int numDefaultPresets = internals.Model->GroupManager->numberOfPresetsInGroup("default");

  const Ui::pqPresetDialog& ui = internals.Ui;

  ui.showDefault->setChecked(defaultPosition != -1);
  ui.showDefault->setEnabled(defaultPosition == -1 || defaultPosition >= numDefaultPresets);
  ui.colors->setEnabled(true);
  ui.usePresetRange->setEnabled(!internals.Model->Presets->GetPresetHasIndexedColors(preset));
  ui.opacities->setEnabled(internals.Model->Presets->GetPresetHasOpacities(preset));
  ui.annotations->setEnabled(internals.Model->Presets->GetPresetHasAnnotations(preset));
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
  assert(preset.empty() == false);
  emit this->applyPreset(preset);
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
  pqFileDialog dialog(NULL, this, tr("Import Presets"), QString(),
    "Supported Presets/Color Map Files (*.json *.xml);;"
    "ParaView Color/Opacity Presets (*.json);;Legacy Color Maps (*.xml);;All Files (*)");
  dialog.setObjectName("ImportPresets");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted && dialog.getSelectedFiles().size() > 0)
  {
    QString filename = dialog.getSelectedFiles()[0];
    const pqInternals& internals = *this->Internals;
    int oldCount = internals.Model->rowCount(QModelIndex());
    internals.Model->importPresets(filename);
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
  pqFileDialog dialog(NULL, this, tr("Export Preset(s)"), QString(),
    "ParaView Color/Opacity Presets (*.json);;All Files (*)");
  dialog.setObjectName("ExportPresets");
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (dialog.exec() != QDialog::Accepted || dialog.getSelectedFiles().size() == 0)
  {
    return;
  }

  QString filename = dialog.getSelectedFiles()[0];
  const pqInternals& internals = *this->Internals;
  const Ui::pqPresetDialog& ui = this->Internals->Ui;

  Json::Value presetCollection(Json::arrayValue);
  QModelIndexList selectedRows = ui.gradients->selectionModel()->selectedIndexes();
  foreach (const QModelIndex& proxyIndex, selectedRows)
  {
    QModelIndex idx = internals.ReflowModel->mapToSource(proxyIndex);
    idx = internals.ProxyModel->mapToSource(idx);
    const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
    presetCollection.append(preset);
  }
  assert(presetCollection.size() > 0);

  ofstream outfs;
  outfs.open(filename.toStdString().c_str());
  if (!outfs.is_open())
  {
    qCritical() << "Failed to open file for writing: " << filename;
    return;
  }
  outfs << presetCollection.toStyledString().c_str() << endl;
  outfs.close();
}

//-----------------------------------------------------------------------------
void pqPresetDialog::setPresetIsAdvanced(int newState)
{
  bool showByDefault = newState == Qt::Checked;
  const pqInternals& internals = *this->Internals;
  QModelIndexList selectedRows = internals.Ui.gradients->selectionModel()->selectedIndexes();
  if (selectedRows.size() > 1 || selectedRows.size() == 0)
  {
    // This doesn't support toggling multiple at once for now.
    // This is more of a sanity check since the checkbox should be disabled anyway.
    return;
  }
  QModelIndex idx = selectedRows[0];

  idx = internals.ReflowModel->mapToSource(idx);
  idx = internals.ProxyModel->mapToSource(idx);

  auto column = internals.Model->GroupManager->groupNames().indexOf("default") + 1;
  QModelIndex defaultColumnIndex = internals.Model->index(idx.row(), column);

  int defaultPosition = internals.Model->data(defaultColumnIndex, Qt::DisplayRole).toInt();

  if (showByDefault && defaultPosition == -1)
  {
    internals.Model->addPresetToDefaults(idx);
    internals.Ui.gradients->update(selectedRows[0]);
  }
  else if (!showByDefault && defaultPosition != -1)
  {
    internals.Model->removePresetFromDefaults(idx);
    internals.Ui.gradients->update(selectedRows[0]);
  }
}
