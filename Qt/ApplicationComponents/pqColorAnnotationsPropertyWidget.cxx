/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqColorAnnotationsPropertyWidget.h"
#include "ui_pqColorAnnotationsPropertyWidget.h"
#include "ui_pqSavePresetOptions.h"

#include "pqActiveObjects.h"
#include "pqAnnotationsModel.h"
#include "pqChooseColorPresetReaction.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqDoubleLineEdit.h"
#include "pqDoubleRangeDialog.h"
#include "pqDoubleRangeWidget.h"
#include "pqPropertiesPanel.h"
#include "pqPropertyWidgetDecorator.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqUndoStack.h"
#include "vtkAbstractArray.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <vtk_jsoncpp.h>

#include <QAbstractTableModel>
#include <QColorDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QString>
#include <QTextStream>

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>

namespace
{

//-----------------------------------------------------------------------------
// Given a list of existing annotations and a list of potentially new
// annotations, merge the lists. The candidate annotations are first
// selected to fill in empty annotation values in the existing
// annotations list, then they are added to the end.
// Arguments are vectors of pairs where the pair.first is the annotated value
// while the pair.second is the label/text for the value.
QVector<std::pair<QString, QString> > MergeAnnotations(
  const QVector<std::pair<QString, QString> >& current_pairs,
  const QVector<std::pair<QString, QString> >& new_pairs)
{
  QMap<QString, QString> old_values;
  for (const auto& pair : current_pairs)
  {
    old_values[pair.first] = pair.second;
  }

  // Subset candidate annotations to only those not in existing annotations.
  // At the same time, update old_values map to have better annotation
  // text/labels, if present in the new values.
  QVector<std::pair<QString, QString> > real_new_pairs;
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
  QVector<std::pair<QString, QString> > merged_pairs;
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

//-----------------------------------------------------------------------------
// Decorator used to hide the widget when using IndexedLookup.
class pqColorAnnotationsPropertyWidgetDecorator : public pqPropertyWidgetDecorator
{
  typedef pqPropertyWidgetDecorator Superclass;
  bool IsAdvanced;

public:
  pqColorAnnotationsPropertyWidgetDecorator(vtkPVXMLElement* xmlArg, pqPropertyWidget* parentArg)
    : Superclass(xmlArg, parentArg)
    , IsAdvanced(false)
  {
  }
  ~pqColorAnnotationsPropertyWidgetDecorator() override {}

  void setIsAdvanced(bool val)
  {
    if (val != this->IsAdvanced)
    {
      this->IsAdvanced = val;
      emit this->visibilityChanged();
    }
  }
  bool canShowWidget(bool show_advanced) const override
  {
    return this->IsAdvanced ? show_advanced : true;
  }

private:
  Q_DISABLE_COPY(pqColorAnnotationsPropertyWidgetDecorator)
};
}

//=============================================================================
class pqColorAnnotationsPropertyWidget::pqInternals
{
public:
  Ui::ColorAnnotationsPropertyWidget Ui;
  pqAnnotationsModel Model;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  QPointer<pqColorAnnotationsPropertyWidgetDecorator> Decorator;
  QScopedPointer<QAction> TempAction;
  QScopedPointer<pqChooseColorPresetReaction> ChoosePresetReaction;

  pqInternals(pqColorAnnotationsPropertyWidget* self)
    : TempAction(new QAction(self))
    , ChoosePresetReaction(new pqChooseColorPresetReaction(this->TempAction.data(), false))
  {
    this->Ui.setupUi(self);
    this->Ui.gridLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.verticalLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.verticalLayout->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->Ui.AnnotationsTable->setModel(&this->Model);
    this->Ui.AnnotationsTable->horizontalHeader()->setHighlightSections(false);
#if QT_VERSION >= 0x050000
    this->Ui.AnnotationsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
#else
    this->Ui.AnnotationsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
    this->Ui.AnnotationsTable->horizontalHeader()->setStretchLastSection(true);

    this->Decorator = new pqColorAnnotationsPropertyWidgetDecorator(nullptr, self);

    QObject::connect(
      this->ChoosePresetReaction.data(), SIGNAL(presetApplied()), self, SIGNAL(changeFinished()));
  }

  // updates annotations model with values provided. If extend is true, existing
  // values will be extended otherwise old values will be replaced.
  // Returns true if annotations changed otherwise returns false.
  bool updateAnnotations(vtkAbstractArray* values, bool extend = true);
};

//-----------------------------------------------------------------------------
bool pqColorAnnotationsPropertyWidget::pqInternals::updateAnnotations(
  vtkAbstractArray* values, bool extend)
{
  QVector<std::pair<QString, QString> > candidate_tuples;
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
  const auto& current_value_annotation_tuples = this->Model.annotations();
  auto merged_value_annotation_tuples =
    extend ? MergeAnnotations(current_value_annotation_tuples, candidate_tuples) : candidate_tuples;
  if (current_value_annotation_tuples != merged_value_annotation_tuples)
  {
    this->Model.setAnnotations(merged_value_annotation_tuples);
    return true;
  }

  return false;
}

//=============================================================================
pqColorAnnotationsPropertyWidget::pqColorAnnotationsPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals(this))
{
  this->addPropertyLink(
    this, "annotations", SIGNAL(annotationsChanged()), smproxy->GetProperty("Annotations"));

  this->addPropertyLink(
    this, "indexedColors", SIGNAL(indexedColorsChanged()), smproxy->GetProperty("IndexedColors"));

  this->addPropertyLink(this, "indexedOpacities", SIGNAL(indexedOpacitiesChanged()),
    smproxy->GetProperty("IndexedOpacities"));

  // if proxy has a property named IndexedLookup, "Color" can be controlled only
  // when IndexedLookup is ON.
  if (smproxy->GetProperty("IndexedLookup"))
  {
    // we are not controlling the IndexedLookup property, we are merely
    // observing it to ensure the UI is updated correctly. Hence we don't fire
    // any signal to update the smproperty.
    this->Internals->VTKConnector->Connect(smproxy->GetProperty("IndexedLookup"),
      vtkCommand::ModifiedEvent, this, SLOT(updateIndexedLookupState()));
    this->updateIndexedLookupState();

    // Add decorator so the widget can be marked as advanced when IndexedLookup
    // is OFF.
    this->addDecorator(this->Internals->Decorator);
  }

  // Hookup UI buttons.
  Ui::ColorAnnotationsPropertyWidget& ui = this->Internals->Ui;
  QObject::connect(ui.Add, SIGNAL(clicked()), this, SLOT(addAnnotation()));
  QObject::connect(ui.AddActive, SIGNAL(clicked()), this, SLOT(addActiveAnnotations()));
  QObject::connect(ui.AddActiveFromVisible, SIGNAL(clicked()), this,
    SLOT(addActiveAnnotationsFromVisibleSources()));
  QObject::connect(ui.Remove, SIGNAL(clicked()), this, SLOT(removeAnnotation()));
  QObject::connect(ui.DeleteAll, SIGNAL(clicked()), this, SLOT(removeAllAnnotations()));
  QObject::connect(ui.ChoosePreset, SIGNAL(clicked()), this, SLOT(choosePreset()));
  QObject::connect(ui.SaveAsPreset, SIGNAL(clicked()), this, SLOT(saveAsPreset()));

  QObject::connect(&this->Internals->Model,
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

  // Opacity mapping enable/disable mechanism
  this->connect(
    ui.EnableOpacityMapping, SIGNAL(stateChanged(int)), SLOT(updateOpacityColumnState()));
  vtkSMProperty* smproperty = smgroup->GetProperty("EnableOpacityMapping");
  if (smproperty)
  {
    this->addPropertyLink(ui.EnableOpacityMapping, "checked", SIGNAL(toggled(bool)), smproperty);
  }
  else
  {
    ui.EnableOpacityMapping->hide();
  }
  this->updateOpacityColumnState();

  // Default colors
  if (!this->Internals->Model.hasColors())
  {
    this->applyPreset("KAAMS");
  }
}

//-----------------------------------------------------------------------------
pqColorAnnotationsPropertyWidget::~pqColorAnnotationsPropertyWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::applyPreset(const char* presetName)
{
  auto presets = vtkSMTransferFunctionPresets::GetInstance();
  const Json::Value& preset = presets->GetFirstPresetWithName(presetName);
  const Json::Value& indexedColors = preset["IndexedColors"];
  if (indexedColors.isNull() || !indexedColors.isArray() || (indexedColors.size() % 4) != 0 ||
    indexedColors.size() == 0)
  {
    QString warningMessage = "Could not use " + QString(presetName) +
      " preset as default preset. Preset may not be valid. Please "
      "validate this preset";
    qWarning("%s", warningMessage.toUtf8().data());
  }
  else
  {
    QList<QVariant> defaultPresetVariant;
    for (Json::ArrayIndex cc = 0; cc < indexedColors.size(); cc++)
    {
      defaultPresetVariant.append(indexedColors[cc].asDouble());
    }
    this->setIndexedColors(defaultPresetVariant);
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::updateIndexedLookupState()
{
  if (this->proxy()->GetProperty("IndexedLookup"))
  {
    bool val = vtkSMPropertyHelper(this->proxy(), "IndexedLookup").GetAsInt() != 0;
    this->Internals->Ui.AnnotationsTable->horizontalHeader()->setSectionHidden(0, !val);
    this->Internals->Ui.ChoosePreset->setVisible(val);
    this->Internals->Ui.SaveAsPreset->setVisible(val);
    this->Internals->Decorator->setIsAdvanced(!val);

    pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
    if (val && repr && repr->isVisible())
    {
      vtkSMPropertyHelper annotationsInitialized(this->proxy(), "AnnotationsInitialized");
      if (!annotationsInitialized.GetAsInt())
      {
        // Attempt to add active annotations.
        bool success = this->addActiveAnnotations(false /* do not force generation */);
        if (!success)
        {
          QString promptMessage;
          QTextStream qs(&promptMessage);
          qs << "Could not initialize annotations for categorical coloring. There may be too many "
             << "discrete values in your data, (more than " << vtkAbstractArray::MAX_DISCRETE_VALUES
             << ") "
             << "or you may be coloring by a floating point data array. Please "
             << "add annotations manually.";
          pqCoreUtilities::promptUser("pqColorAnnotationsPropertyWidget::updatedIndexedLookupState",
            QMessageBox::Information, "Could not determine discrete values to use for annotations",
            promptMessage, QMessageBox::Ok | QMessageBox::Save);
        }
        else
        {
          annotationsInitialized.Set(1);
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::updateOpacityColumnState()
{
  Ui::ColorAnnotationsPropertyWidget& ui = this->Internals->Ui;
  ui.AnnotationsTable->setColumnHidden(1, !ui.EnableOpacityMapping->isChecked());
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::onDataChanged(
  const QModelIndex& topleft, const QModelIndex& btmright)
{
  if (topleft.column() == 0)
  {
    emit this->indexedColorsChanged();
  }
  if (topleft.column() == 1)
  {
    emit this->indexedOpacitiesChanged();
  }
  if (btmright.column() >= 2)
  {
    emit this->annotationsChanged();
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::onHeaderDoubleClicked(int index)
{
  if (index == 1)
  {
    this->execGlobalOpacityDialog();
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::execGlobalOpacityDialog()
{
  pqGlobalOpacityRangeDialog dialog(this->Internals->Model.globalOpacity(), 1.0, this);
  dialog.setWindowTitle(tr("Set Global Opacity"));
  dialog.move(QCursor::pos());
  if (dialog.exec() == QDialog::Accepted)
  {
    this->Internals->Model.setGlobalOpacity(dialog.globalOpacity());

    QList<int> selectedRows;
    QItemSelectionModel* select = this->Internals->Ui.AnnotationsTable->selectionModel();
    foreach (QModelIndex idx, select->selectedRows(1))
    {
      selectedRows.append(idx.row());
    }
    this->Internals->Model.setSelectedOpacity(selectedRows, dialog.selectedOpacity());
  }
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::onDoubleClicked(const QModelIndex& idx)
{
  if (idx.column() == 0)
  {
    QColor color = this->Internals->Model.data(idx, Qt::EditRole).value<QColor>();
    color = QColorDialog::getColor(
      color, this, tr("Choose Annotation Color"), QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      this->Internals->Model.setData(idx, color);
    }
  }
  if (idx.column() == 1)
  {
    double opacity = this->Internals->Model.data(idx, Qt::EditRole).toDouble();
    pqDoubleRangeDialog dialog(tr("Opacity:"), 0.0, 1.0, this);
    dialog.setWindowTitle(tr("Select Opacity"));
    dialog.move(QCursor::pos());
    dialog.setValue(opacity);
    if (dialog.exec() == QDialog::Accepted)
    {
      this->Internals->Model.setData(idx, dialog.value());
    }
  }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::annotations() const
{
  const auto& value = this->Internals->Model.annotations();

  QList<QVariant> reply;
  for (int cc = 0; cc < value.size(); cc++)
  {
    reply.push_back(value[cc].first);
    reply.push_back(value[cc].second);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setAnnotations(const QList<QVariant>& value)
{
  QVector<std::pair<QString, QString> > annotationsData;
  annotationsData.resize(value.size() / 2);

  for (int cc = 0; (cc + 1) < value.size(); cc += 2)
  {
    annotationsData[cc / 2] =
      std::pair<QString, QString>(value[cc].toString(), value[cc + 1].toString());
  }
  this->Internals->Model.setAnnotations(annotationsData);

  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::indexedColors() const
{
  QList<QVariant> reply;
  QVector<QColor> colors = this->Internals->Model.indexedColors();
  foreach (const QColor& color, colors)
  {
    reply.push_back(color.redF());
    reply.push_back(color.greenF());
    reply.push_back(color.blueF());
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setIndexedColors(const QList<QVariant>& value)
{
  int nbEntry = value.size() / 3;
  QVector<QColor> colors;
  colors.resize(nbEntry);
  QVector<double> opacities;
  opacities.resize(nbEntry);

  for (int cc = 0; cc < nbEntry; cc++)
  {
    QColor color;
    color.setRgbF(
      value[cc * 3].toDouble(), value[cc * 3 + 1].toDouble(), value[cc * 3 + 2].toDouble());
    colors[cc] = color;
  }

  this->Internals->Model.setIndexedColors(colors);
  emit this->indexedColorsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqColorAnnotationsPropertyWidget::indexedOpacities() const
{
  QList<QVariant> reply;
  QVector<double> opacities = this->Internals->Model.indexedOpacities();
  for (int i = 0; i < opacities.count(); i++)
  {
    reply.push_back(opacities[i]);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::setIndexedOpacities(const QList<QVariant>& value)
{
  QVector<double> opacities;
  opacities.resize(value.size());
  for (int cc = 0; cc < value.size(); cc++)
  {
    opacities[cc] = value[cc].toDouble();
  }
  this->Internals->Model.setIndexedOpacities(opacities);
  emit this->indexedOpacitiesChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addAnnotation()
{
  auto& internals = (*this->Internals);
  QModelIndex idx = internals.Model.addAnnotation(internals.Ui.AnnotationsTable->currentIndex());
  // now select the newly added item.
  internals.Ui.AnnotationsTable->selectionModel()->setCurrentIndex(
    idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::editPastLastRow()
{
  this->Internals->Model.addAnnotation(this->Internals->Ui.AnnotationsTable->currentIndex());
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::removeAnnotation()
{
  auto& internals = (*this->Internals);
  QModelIndexList indexes = internals.Ui.AnnotationsTable->selectionModel()->selectedIndexes();
  if (indexes.size() == 0)
  {
    // Nothing selected. Nothing to remove
    return;
  }
  QModelIndex idx = internals.Model.removeAnnotations(indexes);
  internals.Ui.AnnotationsTable->selectionModel()->setCurrentIndex(
    idx, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addActiveAnnotations()
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
bool pqColorAnnotationsPropertyWidget::addActiveAnnotations(bool force)
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
  if (QString("Component") == vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
  {
    component_no = vtkSMPropertyHelper(this->proxy(), "VectorComponent").GetAsInt();
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
    emit this->annotationsChanged();
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::addActiveAnnotationsFromVisibleSources()
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
bool pqColorAnnotationsPropertyWidget::addActiveAnnotationsFromVisibleSources(bool force)
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
      vtkSMPropertyHelper(this->proxy(), "VectorMode", true).GetAsString())
    {
      component_no = vtkSMPropertyHelper(this->proxy(), "VectorComponent").GetAsInt();
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
    emit this->annotationsChanged();
  }

  return !missingValues;
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::removeAllAnnotations()
{
  this->Internals->Model.removeAllAnnotations();
  this->Internals->Model.setGlobalOpacity(1.0);
  emit this->annotationsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::choosePreset(const char* presetName)
{
  this->Internals->ChoosePresetReaction->setTransferFunction(this->proxy());
  this->Internals->ChoosePresetReaction->choosePreset(presetName);
  emit this->indexedColorsChanged();
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsPropertyWidget::saveAsPreset()
{
  QDialog dialog(this);
  Ui::SavePresetOptions ui;
  ui.setupUi(&dialog);
  ui.saveOpacities->setVisible(false);
  ui.saveAnnotations->setEnabled(
    vtkSMPropertyHelper(this->proxy(), "Annotations", true).GetNumberOfElements() > 0);

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  Json::Value cpreset = vtkSMTransferFunctionProxy::GetStateAsPreset(this->proxy());
  if (!ui.saveAnnotations->isChecked())
  {
    cpreset.removeMember("Annotations");
  }
  vtkStdString presetName;
  if (!cpreset.isNull())
  {
    auto presets = vtkSMTransferFunctionPresets::GetInstance();
    presetName = presets->AddUniquePreset(cpreset);
  }
  this->choosePreset(presetName);
}

void pqColorAnnotationsPropertyWidget::customMenuRequested(QPoint pos)
{
  // Show a contextual menu to set global opacity and selected rows opacities
  QMenu* menu = new QMenu(this);
  Ui::ColorAnnotationsPropertyWidget& ui = this->Internals->Ui;
  QAction* opacityAction = new QAction(tr("Set global and selected opacity"), this);
  opacityAction->setEnabled(ui.EnableOpacityMapping->isChecked());
  QObject::connect(opacityAction, SIGNAL(triggered(bool)), this, SLOT(execGlobalOpacityDialog()));
  menu->addAction(opacityAction);
  menu->popup(ui.AnnotationsTable->viewport()->mapToGlobal(pos));
}
