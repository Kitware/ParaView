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

#include "pqFileDialog.h"
#include "pqPresetToPixmap.h"
#include "pqPropertiesPanel.h"
#include "vtkNew.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"

#include <vtk_jsoncpp.h>

#include <QList>
#include <QPixmap>
#include <QPointer>
#include <QSize>
#include <QSortFilterProxyModel>
#include <QtDebug>

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
  vtkNew<vtkSMTransferFunctionPresets> Presets;

  pqPresetDialogTableModel(QObject* parentObject)
    : Superclass(parentObject)
  {
    this->Pixmaps.reserve(this->Presets->GetNumberOfPresets());
  }

  virtual ~pqPresetDialogTableModel() {}

  void importPresets(const QString& filename)
  {
    this->beginResetModel();
    this->Presets->ImportPresets(filename.toStdString().c_str());
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

  virtual int rowCount(const QModelIndex& idx) const
  {
    return idx.isValid() ? 0 : static_cast<int>(this->Presets->GetNumberOfPresets());
  }

  virtual int columnCount(const QModelIndex& /*parent*/) const { return 1; }

  virtual QVariant data(const QModelIndex& idx, int role) const
  {
    if (!idx.isValid() || idx.model() != this)
    {
      return QVariant();
    }

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
    }
    return QVariant();
  }

  virtual bool setData(const QModelIndex& idx, const QVariant& value, int role)
  {
    Q_UNUSED(role);
    if (!idx.isValid() || idx.model() != this)
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

  virtual Qt::ItemFlags flags(const QModelIndex& idx) const
  {
    Qt::ItemFlags flgs = this->Superclass::flags(idx);
    if (!idx.isValid() || idx.model() != this)
    {
      return flgs;
    }
    // mark non-builtin presets as editable.
    return this->Presets->IsPresetBuiltin(idx.row()) ? flgs : (flgs | Qt::ItemIsEditable);
  }

  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
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

public:
  pqPresetDialogProxyModel(pqPresetDialog::Modes mode, QObject* parentObject = NULL)
    : Superclass(parentObject)
    , Mode(mode)
  {
  }
  virtual ~pqPresetDialogProxyModel() {}

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
  {
    if (!this->Superclass::filterAcceptsRow(sourceRow, sourceParent))
    {
      return false;
    }

    QModelIndex idx = this->sourceModel()->index(sourceRow, 0, sourceParent);
    switch (this->Mode)
    {
      case pqPresetDialog::SHOW_ALL:
        return true;

      case pqPresetDialog::SHOW_INDEXED_COLORS_ONLY:
        return this->sourceModel()->data(idx, Qt::UserRole).toBool();

      case pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY:
        return !this->sourceModel()->data(idx, Qt::UserRole).toBool();
    }
    return false;
  }

private:
  Q_DISABLE_COPY(pqPresetDialogProxyModel)
};

class pqPresetDialog::pqInternals
{
public:
  Ui::pqPresetDialog Ui;
  QPointer<pqPresetDialogTableModel> Model;
  QPointer<QSortFilterProxyModel> ProxyModel;

  pqInternals(pqPresetDialog::Modes mode, pqPresetDialog* self)
    : Model(new pqPresetDialogTableModel(self))
    , ProxyModel(new pqPresetDialogProxyModel(mode, self))
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
    this->Ui.gradients->setModel(this->ProxyModel);
  }
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
}

//-----------------------------------------------------------------------------
pqPresetDialog::~pqPresetDialog()
{
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
  QModelIndex idx = internals.Model->indexFromName(presetName);
  idx = internals.ProxyModel->mapFromSource(idx);
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
  QModelIndexList selectedRows = ui.gradients->selectionModel()->selectedRows();
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
  QModelIndex idx = internals.ProxyModel->mapToSource(proxyIndex);
  const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
  Q_ASSERT(preset.empty() == false);

  const Ui::pqPresetDialog& ui = internals.Ui;

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
  QModelIndex idx = internals.ProxyModel->mapToSource(proxyIndex);
  const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
  Q_ASSERT(preset.empty() == false);
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
    selectedRows = ui.gradients->selectionModel()->selectedRows();
  }
  for (int cc = (selectedRows.size() - 1); cc >= 0; cc--)
  {
    const QModelIndex& proxyIndex = selectedRows[cc];
    QModelIndex idx = internals.ProxyModel->mapToSource(proxyIndex);
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
    QModelIndex idx = internals.ProxyModel->mapToSource(proxyIndex);
    const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
    Q_ASSERT(preset.empty() == false);
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
      QModelIndex endProxyIdx =
        internals.ProxyModel->mapFromSource(internals.Model->index(newCount - 1, 0, QModelIndex()));

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
  QModelIndexList selectedRows = ui.gradients->selectionModel()->selectedRows();
  foreach (const QModelIndex& proxyIndex, selectedRows)
  {
    QModelIndex idx = internals.ProxyModel->mapToSource(proxyIndex);
    const Json::Value& preset = internals.Model->Presets->GetPreset(idx.row());
    presetCollection.append(preset);
  }
  Q_ASSERT(presetCollection.size() > 0);

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
