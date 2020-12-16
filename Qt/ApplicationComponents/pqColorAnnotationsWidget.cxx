/*=========================================================================

   Program: ParaView
   Module:  pqColorAnnotationsWidget.cxx

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
#include "pqColorAnnotationsWidget.h"
#include "ui_pqColorAnnotationsWidget.h"
#include "ui_pqSavePresetOptions.h"

#include "pqActiveObjects.h"
#include "pqChooseColorPresetReaction.h"
#include "pqDataRepresentation.h"
#include "pqDoubleLineEdit.h"
#include "pqDoubleRangeDialog.h"
#include "pqDoubleRangeWidget.h"
#include "pqHeaderView.h"
#include "pqPropertiesPanel.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqTreeViewSelectionHelper.h"

#include "vtkAbstractArray.h"
#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtk_jsoncpp.h>

#include <QColorDialog>
#include <QItemSelection>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTextStream>

#include <cassert>
#include <set>
#include <sstream>
#include <vector>

static const std::string INDEXED_COLORS = "IndexedColors";
static const std::string ANNOTATIONS = "Annotations";

namespace
{

//-----------------------------------------------------------------------------
// Given a list of existing annotations and a list of potentially new
// annotations, merge the lists. The candidate annotations are first
// selected to fill in empty annotation values in the existing
// annotations list, then they are added to the end.
// Arguments are vectors of pairs where the pair.first is the annotated value
// while the pair.second is the label/text for the value.
std::vector<std::pair<QString, QString> > MergeAnnotations(
  const std::vector<std::pair<QString, QString> >& current_pairs,
  const std::vector<std::pair<QString, QString> >& new_pairs)
{
  QMap<QString, QString> old_values;
  for (const auto& pair : current_pairs)
  {
    old_values[pair.first] = pair.second;
  }

  // Subset candidate annotations to only those not in existing annotations.
  // At the same time, update old_values map to have better annotation
  // text/labels, if present in the new values.
  std::vector<std::pair<QString, QString> > real_new_pairs;
  for (const auto& pair : new_pairs)
  {
    auto iter = old_values.find(pair.first);
    if (iter == old_values.end())
    {
      real_new_pairs.push_back(pair);
    }
    else if (iter.value().isEmpty())
    {
      // update old values with text from new pairs if it's `better`
      iter.value() = pair.second;
    }
  }

  // Iterate over existing annotations, backfilling annotation texts/labels
  // from the new_pairs, if old texts were empty.
  std::vector<std::pair<QString, QString> > merged_pairs;
  for (const auto& pair : current_pairs)
  {
    // using old_values to get updated annotation texts, if any.
    merged_pairs.push_back(std::pair<QString, QString>(pair.first, old_values[pair.first]));
  }
  for (const auto& pair : real_new_pairs)
  {
    merged_pairs.push_back(pair);
  }

  return merged_pairs;
}

class ColorAnnotationsFilterProxyModel : public QSortFilterProxyModel
{
public:
  explicit ColorAnnotationsFilterProxyModel(QObject* parent = nullptr)
    : QSortFilterProxyModel(parent)
  {
  }

  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
  {
    QModelIndex domainIndex =
      sourceModel()->index(sourceRow, pqAnnotationsModel::VISIBILITY, sourceParent);
    return sourceModel()->data(domainIndex, Qt::UserRole).toBool() &&
      QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
  }
};

//-----------------------------------------------------------------------------
// Dialog to set global and selected lines opacity
class pqGlobalOpacityRangeDialog : public QDialog
{
public:
  pqGlobalOpacityRangeDialog(
    double globalOpacity = 1.0, double selectedOpacity = 1.0, QWidget* parent = 0)
    : QDialog(parent)
  {
    this->GlobalOpacityWidget = new pqDoubleRangeWidget(this);
    this->GlobalOpacityWidget->setMinimum(0.0);
    this->GlobalOpacityWidget->setMaximum(1.0);
    this->GlobalOpacityWidget->setValue(globalOpacity);
    this->SelectedOpacityWidget = new pqDoubleRangeWidget(this);
    this->SelectedOpacityWidget->setMinimum(0.0);
    this->SelectedOpacityWidget->setMaximum(1.0);
    this->SelectedOpacityWidget->setValue(selectedOpacity);

    QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QHBoxLayout* widgetLayout = new QHBoxLayout;
    widgetLayout->addWidget(new QLabel(tr("Global Opacity :"), this));
    widgetLayout->addWidget(this->GlobalOpacityWidget);
    QHBoxLayout* widgetLayout2 = new QHBoxLayout;
    widgetLayout2->addWidget(new QLabel(tr("Selected Categories Opacity :"), this));
    widgetLayout2->addWidget(this->SelectedOpacityWidget);

    QVBoxLayout* layout_ = new QVBoxLayout;
    layout_->addLayout(widgetLayout);
    layout_->addLayout(widgetLayout2);
    layout_->addWidget(buttonBox);
    this->setLayout(layout_);
  }

  ~pqGlobalOpacityRangeDialog() override{};

  double globalOpacity() const { return this->GlobalOpacityWidget->value(); }
  double selectedOpacity() const { return this->SelectedOpacityWidget->value(); }

private:
  pqDoubleRangeWidget* GlobalOpacityWidget;
  pqDoubleRangeWidget* SelectedOpacityWidget;
};
}

//=============================================================================
class pqColorAnnotationsWidget::pqInternals
{
public:
  Ui::ColorAnnotationsWidget Ui;
  pqAnnotationsModel* Model;
  QScopedPointer<QAction> TempAction;
  QScopedPointer<pqChooseColorPresetReaction> ChoosePresetReaction;
  vtkSmartPointer<vtkSMTransferFunctionProxy> LookupTableProxy;
  std::string CurrentPresetName;

  void SetCurrentPresetName(std::string newName) { this->CurrentPresetName = newName; }

  pqInternals(pqColorAnnotationsWidget* self)
    : TempAction(new QAction(self))
    , ChoosePresetReaction(new pqChooseColorPresetReaction(this->TempAction.data(), false))
  {
    this->SetCurrentPresetName("");
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->Model = new pqAnnotationsModel(self);
    this->Ui.AnnotationsTable->setModel(this->Model);

    pqHeaderView* myheader = new pqHeaderView(Qt::Horizontal, this->Ui.AnnotationsTable);
    myheader->setToggleCheckStateOnSectionClick(true);
    this->Ui.AnnotationsTable->setHorizontalHeader(myheader);

    this->Ui.AnnotationsTable->horizontalHeader()->setHighlightSections(false);
    this->Ui.AnnotationsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
    this->Ui.AnnotationsTable->horizontalHeader()->setStretchLastSection(true);

    QObject::connect(this->ChoosePresetReaction.data(), SIGNAL(presetApplied(const QString&)), self,
      SLOT(onPresetApplied(const QString&)));
  }

  // updates annotations model with values provided. If extend is true, existing
  // values will be extended otherwise old values will be replaced.
  // Returns true if annotations changed otherwise returns false.
  bool updateAnnotations(vtkAbstractArray* values, bool extend = true);
};

//-----------------------------------------------------------------------------
bool pqColorAnnotationsWidget::pqInternals::updateAnnotations(vtkAbstractArray* values, bool extend)
{
  std::vector<std::pair<QString, QString> > candidate_tuples;
  for (vtkIdType idx = 0; idx < values->GetNumberOfTuples(); idx++)
  {
    const auto val = values->GetVariantValue(idx);
    if (val.IsDouble())
    {
      std::ostringstream str;
      str << vtkNumberToString()(val.ToDouble());
      candidate_tuples.push_back(std::make_pair(QString(str.str().c_str()),
        pqDoubleLineEdit::formatDoubleUsingGlobalPrecisionAndNotation(val.ToDouble())));
    }
    else if (val.IsFloat())
    {
      std::ostringstream str;
      str << vtkNumberToString()(val.ToFloat());
      candidate_tuples.push_back(std::make_pair(QString(str.str().c_str()),
        pqDoubleLineEdit::formatDoubleUsingGlobalPrecisionAndNotation(val.ToDouble())));
    }
    else
    {
      auto str = val.ToString();
      candidate_tuples.push_back(std::pair<QString, QString>(str.c_str(), str.c_str()));
    }
  }

  // Combined annotation values (old and new)
  const auto& current_value_annotation_tuples = this->Model->annotations();
  auto merged_value_annotation_tuples =
    extend ? MergeAnnotations(current_value_annotation_tuples, candidate_tuples) : candidate_tuples;
  if (current_value_annotation_tuples != merged_value_annotation_tuples)
  {
    this->Model->setAnnotations(merged_value_annotation_tuples);
    return true;
  }

  return false;
}

//=============================================================================
pqColorAnnotationsWidget::pqColorAnnotationsWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , Internals()
{
  this->Internals = new pqInternals(this);

  // Hookup UI buttons.
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(addAnnotation()));
  QObject::connect(ui.AddActive, SIGNAL(clicked()), this, SLOT(addActiveAnnotations()));
  QObject::connect(ui.AddActiveFromVisible, SIGNAL(clicked()), this,
    SLOT(addActiveAnnotationsFromVisibleSources()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(removeAnnotation()));
  QObject::connect(ui.DeleteAll, SIGNAL(clicked()), this, SLOT(removeAllAnnotations()));
  QObject::connect(ui.ChoosePreset, &QToolButton::clicked, this,
    [&]() { choosePreset(this->Internals->CurrentPresetName.c_str()); });
  QObject::connect(
    ui.SaveAsNewPreset, &QToolButton::clicked, this, [&]() { this->saveAsNewPreset(); });
  QObject::connect(ui.SaveAsPreset, &QToolButton::clicked, this,
    [&]() { this->saveAsPreset(this->Internals->CurrentPresetName.c_str(), false, true); });

  QObject::connect(this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(ui.AnnotationsTable, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onDoubleClicked(const QModelIndex&)));
  QObject::connect(ui.AnnotationsTable->horizontalHeader(), SIGNAL(sectionDoubleClicked(int)), this,
    SLOT(onHeaderDoubleClicked(int)));
  QObject::connect(ui.AnnotationsTable, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
  ui.AnnotationsTable->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(ui.AnnotationsTable, SIGNAL(customContextMenuRequested(QPoint)), this,
    SLOT(customMenuRequested(QPoint)));

  this->connect(ui.AnnotationsTable->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)));

  // Opacity mapping enable/disable mechanism
  this->connect(
    ui.EnableOpacityMapping, SIGNAL(stateChanged(int)), SLOT(updateOpacityColumnState()));
  this->connect(ui.EnableOpacityMapping, SIGNAL(clicked()), SIGNAL(opacityMappingChanged()));

  this->updateOpacityColumnState();

  this->setSupportsReorder(false);

  ColorAnnotationsFilterProxyModel* proxyModel = new ColorAnnotationsFilterProxyModel(this);
  proxyModel->setSourceModel(this->Internals->Model);
  ui.AnnotationsTable->setModel(proxyModel);
  ui.AnnotationsTable->setSortingEnabled(false);
}

//-----------------------------------------------------------------------------
pqColorAnnotationsWidget::~pqColorAnnotationsWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setLookupTableProxy(vtkSMProxy* proxy)
{
  this->Internals->LookupTableProxy = vtkSMTransferFunctionProxy::SafeDownCast(proxy);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::applyPreset(const char* presetName)
{
  auto presets = vtkSMTransferFunctionPresets::GetInstance();
  const Json::Value& preset = presets->GetFirstPresetWithName(presetName);
  const Json::Value& indexedColors = preset[INDEXED_COLORS];
  if (indexedColors.isNull() || !indexedColors.isArray() || (indexedColors.size() % 3) != 0 ||
    indexedColors.size() == 0)
  {
    QString warningMessage = "Could not use " + QString(presetName) + " (size " +
      QString::number(indexedColors.size()) + ")" +
      " preset as default preset. Preset may not be valid. Please "
      "validate this preset";
    qWarning("%s", warningMessage.toStdString().c_str());
  }
  else
  {
    this->Internals->SetCurrentPresetName(presetName);
    QList<QVariant> defaultPresetVariant;
    for (Json::ArrayIndex cc = 0; cc < indexedColors.size(); cc++)
    {
      defaultPresetVariant.append(indexedColors[cc].asDouble());
    }

    this->setIndexedColors(defaultPresetVariant);
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::indexedLookupStateUpdated(bool indexed)
{
  this->setColumnVisibility(pqAnnotationsModel::COLOR, indexed);
  this->Internals->Ui.ChoosePreset->setVisible(indexed);
  this->Internals->Ui.SaveAsPreset->setVisible(indexed);
  this->Internals->Ui.SaveAsNewPreset->setVisible(indexed);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::updateOpacityColumnState()
{
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  this->setColumnVisibility(pqAnnotationsModel::OPACITY, ui.EnableOpacityMapping->isChecked());
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::onDataChanged(
  const QModelIndex& topleft, const QModelIndex& btmright)
{
  if (topleft.column() <= pqAnnotationsModel::VISIBILITY)
  {
    Q_EMIT this->visibilitiesChanged();
  }
  if (topleft.column() == pqAnnotationsModel::COLOR)
  {
    Q_EMIT this->indexedColorsChanged();
  }
  if (topleft.column() == pqAnnotationsModel::OPACITY)
  {
    Q_EMIT this->indexedOpacitiesChanged();
  }
  if (btmright.column() >= pqAnnotationsModel::VALUE)
  {
    Q_EMIT this->annotationsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::onHeaderDoubleClicked(int index)
{
  if (index == pqAnnotationsModel::OPACITY)
  {
    this->execGlobalOpacityDialog();
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::execGlobalOpacityDialog()
{
  pqGlobalOpacityRangeDialog dialog(this->Internals->Model->globalOpacity(), 1.0, this);
  dialog.setWindowTitle(tr("Set Global Opacity"));
  dialog.move(QCursor::pos());
  if (dialog.exec() == QDialog::Accepted)
  {
    this->Internals->Model->setGlobalOpacity(dialog.globalOpacity());

    QList<int> selectedRows;
    QItemSelectionModel* select = this->Internals->Ui.AnnotationsTable->selectionModel();
    for (QModelIndex idx : select->selectedRows(1))
    {
      selectedRows.append(idx.row());
    }
    this->Internals->Model->setSelectedOpacity(selectedRows, dialog.selectedOpacity());
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::onDoubleClicked(const QModelIndex& index)
{
  auto model = this->Internals->Ui.AnnotationsTable->model();
  auto proxyModel = dynamic_cast<QSortFilterProxyModel*>(model);
  auto idx = proxyModel ? proxyModel->mapToSource(index) : index;

  if (idx.column() == pqAnnotationsModel::COLOR)
  {
    QColor color = this->Internals->Model->data(idx, Qt::EditRole).value<QColor>();
    color = QColorDialog::getColor(
      color, this, tr("Choose Annotation Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      this->Internals->Model->setData(idx, color);
    }
  }
  if (idx.column() == pqAnnotationsModel::OPACITY)
  {
    double opacity = this->Internals->Model->data(idx, Qt::EditRole).toDouble();
    pqDoubleRangeDialog dialog(tr("Opacity:"), 0.0, 1.0, this);
    dialog.setWindowTitle(tr("Select Opacity"));
    dialog.move(QCursor::pos());
    dialog.setValue(opacity);
    if (dialog.exec() == QDialog::Accepted)
    {
      this->Internals->Model->setData(idx, dialog.value());
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsWidget::annotations() const
{
  const auto& values = this->Internals->Model->annotations();

  QList<QVariant> reply;
  for (const auto& val : values)
  {
    reply.push_back(val.first);
    reply.push_back(val.second);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setAnnotations(const QList<QVariant>& value)
{
  std::vector<std::pair<QString, QString> > annotationsData;
  annotationsData.reserve(value.size() / 2);

  for (int cc = 0; (cc + 1) < value.size(); cc += 2)
  {
    annotationsData.push_back(
      std::pair<QString, QString>(value[cc].toString(), value[cc + 1].toString()));
  }
  this->Internals->Model->setAnnotations(annotationsData);

  Q_EMIT this->annotationsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsWidget::indexedColors() const
{
  QList<QVariant> reply;
  std::vector<QColor> colors = this->Internals->Model->indexedColors();
  for (const QColor& color : colors)
  {
    reply.push_back(color.redF());
    reply.push_back(color.greenF());
    reply.push_back(color.blueF());
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setIndexedColors(const QList<QVariant>& value)
{
  int nbEntry = value.size() / 3;
  std::vector<QColor> colors;
  colors.resize(nbEntry);

  for (int cc = 0; cc < nbEntry; cc++)
  {
    QColor color;
    color.setRgbF(
      value[cc * 3].toDouble(), value[cc * 3 + 1].toDouble(), value[cc * 3 + 2].toDouble());
    colors[cc] = color;
  }

  this->Internals->Model->setIndexedColors(colors);
  Q_EMIT this->indexedColorsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsWidget::visibilities() const
{
  QList<QVariant> reply;
  auto visibilities = this->Internals->Model->visibilities();
  std::sort(visibilities.begin(), visibilities.end(),
    [](std::pair<QString, int> a, std::pair<QString, int> b) {
      return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
    });
  for (const auto& vis : visibilities)
  {
    reply.push_back(vis.first);
    reply.push_back(vis.second == Qt::Checked ? "1" : "0");
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setVisibilities(const QList<QVariant>& values)
{
  std::vector<std::pair<QString, int> > visibilities;

  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    visibilities.push_back(std::make_pair(
      values[cc].toString(), values[cc + 1].toString() == "1" ? Qt::Checked : Qt::Unchecked));
  }

  this->Internals->Model->setVisibilities(visibilities);
  Q_EMIT this->visibilitiesChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsWidget::indexedOpacities() const
{
  QList<QVariant> reply;
  std::vector<double> opacities = this->Internals->Model->indexedOpacities();
  for (const auto& opac : opacities)
  {
    reply.push_back(opac);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setIndexedOpacities(const QList<QVariant>& values)
{
  std::vector<double> opacities;
  opacities.reserve(values.size());
  for (const auto& val : values)
  {
    opacities.push_back(val.toDouble());
  }
  this->Internals->Model->setIndexedOpacities(opacities);
  Q_EMIT this->indexedOpacitiesChanged();
}

//-----------------------------------------------------------------------------
QVariant pqColorAnnotationsWidget::opacityMapping() const
{
  return this->Internals->Ui.EnableOpacityMapping->isChecked() ? 1 : 0;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setOpacityMapping(const QVariant& value)
{
  this->Internals->Ui.EnableOpacityMapping->setChecked(value.toInt() == 1);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::addAnnotation()
{
  auto& internals = (*this->Internals);
  QModelIndex idx = internals.Model->addAnnotation(internals.Ui.AnnotationsTable->currentIndex());
  // now select the newly added item.
  internals.Ui.AnnotationsTable->selectionModel()->setCurrentIndex(
    idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
  Q_EMIT this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::editPastLastRow()
{
  this->Internals->Model->addAnnotation(this->Internals->Ui.AnnotationsTable->currentIndex());
  Q_EMIT this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::removeAnnotation()
{
  auto& internals = (*this->Internals);
  QModelIndexList indexes = internals.Ui.AnnotationsTable->selectionModel()->selectedIndexes();
  if (indexes.size() == 0)
  {
    // Nothing selected. Nothing to remove
    return;
  }
  QModelIndex idx = internals.Model->removeAnnotations(indexes);
  internals.Ui.AnnotationsTable->selectionModel()->setCurrentIndex(
    idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
  Q_EMIT this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::addActiveAnnotations()
{
  if (!this->addActiveAnnotations(false))
  {
    QString warningTitle("Could not determine discrete values");
    QString warningMessage;
    QTextStream qs(&warningMessage);
    qs << "Could not automatically determine annotation values. Usually this means "
       << "too many discrete values (more than " << vtkAbstractArray::MAX_DISCRETE_VALUES << ") "
       << "are available in the data produced by the "
       << "current source/filter. This can happen if the data array type is floating "
       << "point. Please add annotations manually or force generation. Forcing the "
       << "generation will automatically hide the Scalar Bar.";
    QMessageBox* box = new QMessageBox(this);
    box->setWindowTitle(warningTitle);
    box->setText(warningMessage);
    box->addButton(tr("Force"), QMessageBox::AcceptRole);
    box->addButton(QMessageBox::Cancel);
    if (box->exec() != QMessageBox::Cancel)
    {
      QAction action(nullptr);
      pqScalarBarVisibilityReaction reaction(&action);
      reaction.setScalarBarVisibility(false);

      if (!this->addActiveAnnotations(true))
      {
        QMessageBox::warning(this, warningTitle,
          tr("Could not force generation of discrete values using the data "
             "produced by the current source/filter. Please add annotations "
             "manually."),
          QMessageBox::Ok);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool pqColorAnnotationsWidget::addActiveAnnotations(bool force)
{
  // obtain prominent values from the server and add them
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr)
  {
    return false;
  }

  vtkPVProminentValuesInformation* info =
    vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
      repr->getProxy(), 1e-3, 1e-6, force);
  if (!info || !info->GetValid())
  {
    return false;
  }

  int component_no = -1;
  if (QString("Component") ==
    vtkSMPropertyHelper(this->Internals->LookupTableProxy, "VectorMode", true).GetAsString())
  {
    component_no =
      vtkSMPropertyHelper(this->Internals->LookupTableProxy, "VectorComponent").GetAsInt();
  }
  if (component_no == -1 && info->GetNumberOfComponents() == 1)
  {
    component_no = 0;
  }

  vtkSmartPointer<vtkAbstractArray> uniqueValues;
  uniqueValues.TakeReference(info->GetProminentComponentValues(component_no));
  if (uniqueValues == nullptr)
  {
    return false;
  }

  // Set the merged annotations
  auto& internals = (*this->Internals);
  if (internals.updateAnnotations(uniqueValues, true))
  {
    Q_EMIT this->annotationsChanged();
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::addActiveAnnotationsFromVisibleSources()
{
  if (!this->addActiveAnnotationsFromVisibleSources(false))
  {
    QString warningTitle("Could not determine discrete values");
    QString warningMessage;
    QTextStream qs(&warningMessage);
    qs << "Could not automatically determine annotation values. Usually this means "
       << "too many discrete values (more than " << vtkAbstractArray::MAX_DISCRETE_VALUES << ") "
       << "are available in the data produced by the "
       << "current source/filter. This can happen if the data array type is floating "
       << "point. Please add annotations manually or force generation. Forcing the "
       << "generation will automatically hide the Scalar Bar.";

    QMessageBox* box = new QMessageBox(this);
    box->setWindowTitle(warningTitle);
    box->setText(warningMessage);
    box->addButton(tr("Force"), QMessageBox::AcceptRole);
    box->addButton(QMessageBox::Cancel);
    if (box->exec() != QMessageBox::Cancel)
    {
      QAction action(nullptr);
      pqScalarBarVisibilityReaction reaction(&action);
      reaction.setScalarBarVisibility(false);

      if (!this->addActiveAnnotationsFromVisibleSources(true))
      {
        QMessageBox::warning(this, warningTitle,
          tr("Could not force generation of discrete values using the data "
             "produced by the current source/filter. Please add annotations "
             "manually."),
          QMessageBox::Ok);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool pqColorAnnotationsWidget::addActiveAnnotationsFromVisibleSources(bool force)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return false;
  }

  // obtain prominent values from all visible sources colored by the same
  // array name as the name of color array for the active representation.
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr)
  {
    return false;
  }

  vtkSMPVRepresentationProxy* activeRepresentationProxy =
    vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!activeRepresentationProxy)
  {
    return false;
  }

  vtkPVArrayInformation* activeArrayInfo =
    vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(activeRepresentationProxy);

  vtkSMSessionProxyManager* pxm = server->proxyManager();

  // Iterate over representations, collecting prominent values from each.
  std::set<vtkVariant> uniqueAnnotations;
  vtkSmartPointer<vtkCollection> collection = vtkSmartPointer<vtkCollection>::New();
  pxm->GetProxies("representations", collection);
  bool missingValues = false;
  for (int i = 0; i < collection->GetNumberOfItems(); ++i)
  {
    vtkSMProxy* representationProxy = vtkSMProxy::SafeDownCast(collection->GetItemAsObject(i));
    if (!representationProxy || !vtkSMPropertyHelper(representationProxy, "Visibility").GetAsInt())
    {
      continue;
    }

    vtkPVArrayInformation* currentArrayInfo =
      vtkSMPVRepresentationProxy::GetArrayInformationForColorArray(representationProxy);
    if (!activeArrayInfo || !activeArrayInfo->GetName() || !currentArrayInfo ||
      !currentArrayInfo->GetName() ||
      strcmp(activeArrayInfo->GetName(), currentArrayInfo->GetName()))
    {
      continue;
    }

    vtkPVProminentValuesInformation* info =
      vtkSMPVRepresentationProxy::GetProminentValuesInformationForColorArray(
        representationProxy, 1e-3, 1e-6, force);
    if (!info)
    {
      continue;
    }

    int component_no = -1;
    if (QString("Component") ==
      vtkSMPropertyHelper(this->Internals->LookupTableProxy, "VectorMode", true).GetAsString())
    {
      component_no =
        vtkSMPropertyHelper(this->Internals->LookupTableProxy, "VectorComponent").GetAsInt();
    }
    if (component_no == -1 && info->GetNumberOfComponents() == 1)
    {
      component_no = 0;
    }

    vtkSmartPointer<vtkAbstractArray> uniqueValues;
    uniqueValues.TakeReference(info->GetProminentComponentValues(component_no));
    if (uniqueValues == nullptr)
    {
      missingValues = true;
      continue;
    }
    for (vtkIdType idx = 0, max = uniqueValues->GetNumberOfTuples(); idx < max; ++idx)
    {
      uniqueAnnotations.insert(uniqueValues->GetVariantValue(idx));
    }
  }

  vtkNew<vtkVariantArray> uniqueAnnotationsArray;
  uniqueAnnotationsArray->Allocate(static_cast<vtkIdType>(uniqueAnnotations.size()));
  for (const vtkVariant& val : uniqueAnnotations)
  {
    uniqueAnnotationsArray->InsertNextValue(val);
  }

  auto& internals = (*this->Internals);
  if (internals.updateAnnotations(uniqueAnnotationsArray, true))
  {
    Q_EMIT this->annotationsChanged();
  }

  return !missingValues;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::removeAllAnnotations()
{
  this->Internals->Model->removeAllAnnotations();
  this->Internals->Model->setGlobalOpacity(1.0);
  Q_EMIT this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::choosePreset(const char* presetName)
{
  this->Internals->ChoosePresetReaction->setTransferFunction(this->Internals->LookupTableProxy);
  this->Internals->ChoosePresetReaction->choosePreset(presetName);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::saveAsNewPreset()
{
  QDialog dialog(this);
  Ui::SavePresetOptions ui;
  ui.setupUi(&dialog);
  ui.saveOpacities->setVisible(false);
  auto name =
    this->Internals->Model->headerData(pqAnnotationsModel::LABEL, Qt::Horizontal, Qt::DisplayRole)
      .toString();
  ui.saveAnnotations->setText(QString("Save %1s").arg(name));

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();
  ui.presetName->setText("Preset");

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  this->saveAsPreset(qPrintable(ui.presetName->text()), !ui.saveAnnotations->isChecked(), false);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::saveAsPreset(
  const char* defaultName, bool removeAnnotations, bool allowOverride)
{
  Json::Value cpreset =
    vtkSMTransferFunctionProxy::GetStateAsPreset(this->Internals->LookupTableProxy);

  if (!cpreset[INDEXED_COLORS].size())
  {
    qWarning("Cannot save an empty preset");
    return;
  }

  // Sanity check
  Json::Value nullValue;
  Json::Value colors = cpreset.get(INDEXED_COLORS, nullValue);
  Json::Value annotations = cpreset.get(ANNOTATIONS, nullValue);
  if (annotations.size() / 2 != colors.size() / 3)
  {
    qWarning("Preset has unexpected size");
    return;
  }

  // Keep only rows visible in the widget
  Json::Value cleanAnnotations;
  Json::Value cleanColors;
  auto table = this->Internals->Ui.AnnotationsTable;
  Json::ArrayIndex idxAnno, idxColor;
  for (idxAnno = idxColor = 0; idxAnno < annotations.size(); idxAnno += 2, idxColor += 3)
  {
    std::string serieName = annotations[idxAnno].asString();
    for (int j = 0; j < table->model()->rowCount(); j++)
    {
      auto idx = table->model()->index(j, pqAnnotationsModel::VALUE);
      QString annotation = table->model()->data(idx).toString();
      if (serieName == annotation.toStdString() &&
        table->model()
          ->data(table->model()->index(j, pqAnnotationsModel::VISIBILITY), Qt::UserRole)
          .toBool())
      {
        cleanColors.append(colors.get(idxColor, nullValue));
        cleanColors.append(colors.get(idxColor + 1, nullValue));
        cleanColors.append(colors.get(idxColor + 2, nullValue));

        if (!removeAnnotations)
        {
          cleanAnnotations.append(annotations.get(idxAnno, nullValue));
          cleanAnnotations.append(annotations.get(idxAnno + 1, nullValue));
        }
        break;
      }
    }
  }

  cpreset[INDEXED_COLORS] = cleanColors;
  if (!removeAnnotations)
  {
    cpreset[ANNOTATIONS] = cleanAnnotations;
  }
  else
  {
    cpreset.removeMember(ANNOTATIONS);
  }

  std::string presetName = defaultName;
  {
    // This scoping is necessary to ensure that the vtkSMTransferFunctionPresets
    // saves the new preset to the "settings" before the choosePreset dialog is
    // shown.
    auto presets = vtkSMTransferFunctionPresets::GetInstance();
    if (!allowOverride || !presets->SetPreset(presetName.c_str(), cpreset))
    {
      presetName = presets->AddUniquePreset(cpreset, presetName.c_str());
    }
  }

  this->applyPreset(presetName.c_str());
  this->onPresetApplied(QString(presetName.c_str()));
  this->choosePreset(presetName.c_str());
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::customMenuRequested(QPoint pos)
{
  // Show a contextual menu to set global opacity and selected rows opacities
  QMenu* menu = new QMenu(this);
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  QAction* opacityAction = new QAction(tr("Set global and selected opacity"), this);
  opacityAction->setEnabled(ui.EnableOpacityMapping->isChecked());
  QObject::connect(opacityAction, SIGNAL(triggered(bool)), this, SLOT(execGlobalOpacityDialog()));
  menu->addAction(opacityAction);
  menu->popup(ui.AnnotationsTable->viewport()->mapToGlobal(pos));
}

//-----------------------------------------------------------------------------
QModelIndex pqColorAnnotationsWidget::currentIndex()
{
  auto table = this->Internals->Ui.AnnotationsTable;
  auto idx = table->currentIndex();

  const auto proxyModel = qobject_cast<const QSortFilterProxyModel*>(this->Internals->Model);
  idx = proxyModel ? proxyModel->mapToSource(idx) : idx;

  return idx.isValid() && table->selectionModel()->isRowSelected(idx.row(), idx.parent())
    ? idx
    : QModelIndex();
}

//-----------------------------------------------------------------------------
QString pqColorAnnotationsWidget::currentAnnotationValue()
{
  auto idx = this->currentIndex();
  QModelIndex valueIdx = idx.sibling(idx.row(), pqAnnotationsModel::VALUE);
  auto value = valueIdx.data();
  return value.toString();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setSelectedAnnotations(const QStringList& annotations)
{
  auto table = this->Internals->Ui.AnnotationsTable;
  auto prevSelection = table->selectionModel()->selection();
  table->selectionModel()->clearSelection();

  for (int i = 0; i < table->model()->rowCount(); i++)
  {
    auto idx = table->model()->index(i, pqAnnotationsModel::VALUE);
    if (annotations.contains(idx.data().toString()))
    {
      table->selectionModel()->select(idx, QItemSelectionModel::Rows | QItemSelectionModel::Select);
      table->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::Rows);
    }
  }

  auto currentSelection = table->selectionModel()->selection();
  Q_EMIT this->selectionChanged(currentSelection, prevSelection);
}

//-----------------------------------------------------------------------------
QStringList pqColorAnnotationsWidget::selectedAnnotations()
{
  auto selection = this->selectedIndexes();
  QStringList selectedAnnotations;
  for (auto idx : selection)
  {
    QModelIndex valueIdx = idx.sibling(idx.row(), pqAnnotationsModel::VALUE);
    selectedAnnotations << valueIdx.data().toString();
  }

  selectedAnnotations.removeDuplicates();

  return selectedAnnotations;
}

//-----------------------------------------------------------------------------
QModelIndexList pqColorAnnotationsWidget::selectedIndexes()
{
  return this->Internals->Ui.AnnotationsTable->selectionModel()->selectedIndexes();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setAnnotationsModel(pqAnnotationsModel* model)
{
  if (this->Internals->Model != nullptr)
  {
    delete this->Internals->Model;
  }

  auto previousModel = this->Internals->Ui.AnnotationsTable->model();
  auto proxyModel = dynamic_cast<QSortFilterProxyModel*>(previousModel);
  this->Internals->Model = model;
  if (proxyModel)
  {
    proxyModel->setSourceModel(this->Internals->Model);
  }
  else
  {
    this->Internals->Ui.AnnotationsTable->setModel(this->Internals->Model);
  }

  QObject::connect(this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));

  this->connect(this->Internals->Ui.AnnotationsTable->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)));
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setSupportsReorder(bool reorder)
{
  if (!reorder)
  {
    this->Internals->Ui.AnnotationsTable->sortByColumn(
      pqAnnotationsModel::VALUE, Qt::AscendingOrder);
  }

  this->Internals->Model->setSupportsReorder(reorder);

  this->Internals->Ui.AnnotationsTable->setDragEnabled(reorder);
  this->Internals->Ui.AnnotationsTable->setDragDropMode(
    reorder ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::sort(int column, Qt::SortOrder order)
{
  this->Internals->Ui.AnnotationsTable->model()->sort(column, order);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::allowsRegexpMatching(bool allow)
{
  this->Internals->ChoosePresetReaction->setAllowsRegexpMatching(allow);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::allowsUserDefinedValues(bool allow)
{
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  ui.Add->setVisible(allow);
  ui.Remove->setVisible(allow);
  ui.AddActive->setVisible(allow);
  ui.AddActiveFromVisible->setVisible(allow);
  ui.DeleteAll->setVisible(allow);
  if (!allow)
  {
    QObject::disconnect(
      ui.AnnotationsTable, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
  }
  else
  {
    QObject::connect(ui.AnnotationsTable, SIGNAL(editPastLastRow()), this, SLOT(editPastLastRow()));
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::enablePresets(bool enable)
{
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  ui.SaveAsPreset->setVisible(enable);
  ui.SaveAsNewPreset->setVisible(enable);
  ui.ChoosePreset->setVisible(enable);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::supportsOpacityMapping(bool val)
{
  Ui::ColorAnnotationsWidget& ui = this->Internals->Ui;
  ui.EnableOpacityMapping->setVisible(val);
  ui.EnableOpacityMapping->setChecked(val);

  if (!val)
  {
    ui.AnnotationsTable->disconnect(SIGNAL(customContextMenuRequested(QPoint)));
    new pqTreeViewSelectionHelper(ui.AnnotationsTable, false);
  }
  else
  {
    auto selectionHelper = this->findChild<pqTreeViewSelectionHelper*>();
    if (selectionHelper)
    {
      selectionHelper->deleteLater();
    }
    QObject::connect(ui.AnnotationsTable, SIGNAL(customContextMenuRequested(QPoint)), this,
      SLOT(customMenuRequested(QPoint)));
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::onPresetApplied(const QString& name)
{
  this->Internals->SetCurrentPresetName(name.toStdString());
  Q_EMIT this->presetChanged(name);
}

//-----------------------------------------------------------------------------
const char* pqColorAnnotationsWidget::currentPresetName()
{
  return this->Internals->CurrentPresetName.c_str();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setCurrentPresetName(const char* name)
{
  this->Internals->SetCurrentPresetName(name);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::setColumnVisibility(
  pqAnnotationsModel::ColumnRoles col, bool visible)
{
  this->Internals->Ui.AnnotationsTable->setColumnHidden(col, !visible);
}

//-----------------------------------------------------------------------------
bool pqColorAnnotationsWidget::presetLoadAnnotations()
{
  return this->Internals->ChoosePresetReaction->loadAnnotations();
}

//-----------------------------------------------------------------------------
QRegularExpression pqColorAnnotationsWidget::presetRegularExpression()
{
  return this->Internals->ChoosePresetReaction->regularExpression();
}
