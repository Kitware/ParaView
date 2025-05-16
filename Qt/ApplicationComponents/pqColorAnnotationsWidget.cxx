// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqColorAnnotationsWidget.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqPVApplicationCore.h"
#include "ui_pqColorAnnotationsWidget.h"
#include "ui_pqSavePresetOptions.h"

#include "pqActiveObjects.h"
#include "pqChooseColorPresetReaction.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqDoubleRangeDialog.h"
#include "pqDoubleRangeWidget.h"
#include "pqHeaderView.h"
#include "pqPropertiesPanel.h"
#include "pqScalarBarVisibilityReaction.h"
#include "pqTreeViewSelectionHelper.h"

#include "vtkAbstractArray.h"
#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVProminentValuesInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <qnamespace.h>
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
#define pqCheckBoxSignal checkStateChanged
using pqCheckState = Qt::CheckState;
#else
#define pqCheckBoxSignal stateChanged
using pqCheckState = int;
#endif

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
std::vector<std::pair<QString, QString>> MergeAnnotations(
  const std::vector<std::pair<QString, QString>>& current_pairs,
  const std::vector<std::pair<QString, QString>>& new_pairs)
{
  QMap<QString, QString> old_values;
  for (const auto& pair : current_pairs)
  {
    old_values[pair.first] = pair.second;
  }

  // Subset candidate annotations to only those not in existing annotations.
  // At the same time, update old_values map to have better annotation
  // text/labels, if present in the new values.
  std::vector<std::pair<QString, QString>> real_new_pairs;
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
  std::vector<std::pair<QString, QString>> merged_pairs;
  merged_pairs.reserve(current_pairs.size() + real_new_pairs.size());
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
// Custom proxy model used to apply global modifications (when editing the data from
// the header of the table) to the sorted items only.
class ColorAnnotationsFilterProxyModel : public QSortFilterProxyModel
{
public:
  explicit ColorAnnotationsFilterProxyModel(QObject* parent = nullptr)
    : QSortFilterProxyModel(parent)
  {
  }

  // Overridden to update the global checkbox based on filtered elements only.
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    pqAnnotationsModel* sourceModel = qobject_cast<pqAnnotationsModel*>(this->sourceModel());
    if (!sourceModel)
    {
      return false;
    }

    // Return the global visibility value based on the filtered element values only
    if (orientation == Qt::Horizontal && role == Qt::CheckStateRole &&
      section == pqAnnotationsModel::VISIBILITY)
    {
      // No items
      if (this->rowCount() == 0)
      {
        return Qt::Unchecked;
      }

      int currentCheckState = this->data(this->index(0, section), role).toInt();
      for (int row = 1; row < this->rowCount(); row++)
      {
        if (this->data(this->index(row, section), role).toInt() != currentCheckState)
        {
          currentCheckState = Qt::PartiallyChecked;
          break;
        }
      }
      return static_cast<Qt::CheckState>(currentCheckState);
    }

    // Forward to the source model in all other cases
    return sourceModel->headerData(section, orientation, role);
  }

  // Overridden in order to set the visibility / opacity values on the filtered elements only.
  bool setHeaderData(
    int section, Qt::Orientation orientation, const QVariant& value, int role) override
  {
    pqAnnotationsModel* sourceModel = qobject_cast<pqAnnotationsModel*>(this->sourceModel());
    if (!sourceModel)
    {
      return false;
    }

    if (orientation == Qt::Horizontal)
    {
      // Change the visibility values on filtered items only
      if (role == Qt::CheckStateRole && section == pqAnnotationsModel::VISIBILITY)
      {
        for (int row = 0; row < this->rowCount(); row++)
        {
          this->setData(this->index(row, section), value, role);
        }
        Q_EMIT this->headerDataChanged(orientation, section, section);
        return true;
      }
      // Change the opacity values on filtered items only
      else if (role == Qt::EditRole && section == pqAnnotationsModel::OPACITY)
      {
        sourceModel->setGlobalOpacity(value.toDouble());

        for (int row = 0; row < this->rowCount(); row++)
        {
          this->setData(this->index(row, section), value, role);
        }
        Q_EMIT this->headerDataChanged(orientation, section, section);
        return true;
      }
    }

    // Forward the call to the source model in all other cases
    return sourceModel->setHeaderData(section, orientation, value, role);
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
// Dialog to set opacity value(s)
class pqOpacityRangeDialog : public QDialog
{
  // Enable the use of the tr() method in a class without Q_OBJECT macro
  Q_DECLARE_TR_FUNCTIONS(pqOpacityRangeDialog)
public:
  pqOpacityRangeDialog(double initialValue = 1.0, QWidget* parent = nullptr)
    : QDialog(parent)
  {
    this->OpacityWidget = new pqDoubleRangeWidget(this);
    this->OpacityWidget->setMinimum(0.0);
    this->OpacityWidget->setMaximum(1.0);
    this->OpacityWidget->setValue(initialValue);

    QDialogButtonBox* buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QHBoxLayout* widgetLayout = new QHBoxLayout;
    widgetLayout->addWidget(new QLabel(tr("Opacity value :"), this));
    widgetLayout->addWidget(this->OpacityWidget);
    QVBoxLayout* layout_ = new QVBoxLayout;
    layout_->addLayout(widgetLayout);
    layout_->addWidget(buttonBox);
    this->setLayout(layout_);

    // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
    this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));
  }

  ~pqOpacityRangeDialog() override = default;

  double opacity() const { return this->OpacityWidget->value(); }

private:
  pqDoubleRangeWidget* OpacityWidget;
};

//-----------------------------------------------------------------------------
// Custom pqTreeViewHelper used to set opacity values on selected items.
class pqColorAnnotationsSelectionHelper : public pqTreeViewSelectionHelper
{
  // Enable the use of the tr() method in a class without Q_OBJECT macro
  Q_DECLARE_TR_FUNCTIONS(pqColorAnnotationsSelectionHelper)
public:
  explicit pqColorAnnotationsSelectionHelper(QAbstractItemView* view)
    : pqTreeViewSelectionHelper(view, false)
  {
  }

protected:
  void execOpacityDialog()
  {
    pqOpacityRangeDialog dialog(1.0, nullptr);
    dialog.setWindowTitle(tr("Set opacity of highlited items"));
    dialog.move(QCursor::pos());
    if (dialog.exec() == QDialog::Accepted)
    {
      // Set the opacity on selected elements
      QAbstractItemModel* model = this->TreeView->model();
      QItemSelectionModel* select = this->TreeView->selectionModel();
      for (QModelIndex idx : select->selectedRows(pqAnnotationsModel::OPACITY))
      {
        model->setData(idx, dialog.opacity(), Qt::EditRole);
      }
    }
  }

  void buildupMenu(QMenu& menu, int section, const QPoint& pos) override
  {
    pqTreeViewSelectionHelper::buildupMenu(menu, section, pos);

    if (section == pqAnnotationsModel::OPACITY)
    {
      // Add a menu section to set the opacity of the selected rows
      if (!menu.isEmpty())
      {
        menu.addSeparator();
      }
      if (auto actn = menu.addAction(tr("Set opacity of highlighted items")))
      {
        QObject::connect(actn, &QAction::triggered, [this](bool) { this->execOpacityDialog(); });
      }
    }
  }
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
    this->Ui.gridLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
    this->Ui.gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
    this->Ui.gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.verticalLayout->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
      pqPropertiesPanel::suggestedMargin());
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
  std::vector<std::pair<QString, QString>> candidate_tuples;
  for (vtkIdType idx = 0; idx < values->GetNumberOfTuples(); idx++)
  {
    const auto val = values->GetVariantValue(idx);
    if (val.IsDouble())
    {
      candidate_tuples.push_back(std::make_pair(pqCoreUtilities::formatFullNumber(val.ToDouble()),
        pqCoreUtilities::formatNumber(val.ToDouble())));
    }
    else if (val.IsFloat())
    {
      candidate_tuples.push_back(std::make_pair(pqCoreUtilities::formatFullNumber(val.ToFloat()),
        pqCoreUtilities::formatNumber(val.ToFloat())));
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

  this->connect(ui.AnnotationsTable->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)));

  // Opacity mapping enable/disable mechanism
  this->connect(ui.EnableOpacityMapping, &QCheckBox::pqCheckBoxSignal, this,
    &pqColorAnnotationsWidget::updateOpacityColumnState);
  this->connect(ui.EnableOpacityMapping, SIGNAL(clicked()), SIGNAL(opacityMappingChanged()));

  // Animation tick events will update list of available annotations.
  if (auto manager = pqPVApplicationCore::instance()->animationManager())
  {
    if (auto scene = manager->getActiveScene())
    {
      QObject::connect(scene, &pqAnimationScene::tick, this,
        [this](const int)
        {
          const auto& internals = *(this->Internals);
          if (internals.LookupTableProxy != nullptr)
          {
            int rescaleMode =
              vtkSMPropertyHelper(internals.LookupTableProxy, "AutomaticRescaleRangeMode", true)
                .GetAsInt();
            if (rescaleMode ==
              vtkSMTransferFunctionManager::TransferFunctionResetMode::GROW_ON_APPLY_AND_TIMESTEP)
            {
              // extends the list of annotations.
              this->addActiveAnnotations(/*force=*/false, /*extend=*/true);
            }
            else if (rescaleMode ==
              vtkSMTransferFunctionManager::TransferFunctionResetMode::RESET_ON_APPLY_AND_TIMESTEP)
            {
              // clamps the list of annotations to the scalar range of the active source.
              this->addActiveAnnotations(/*force=*/false, /*extend=*/false);
            }
          }
        });
    }
  }

  this->updateOpacityColumnState();

  this->setSupportsReorder(false);

  ColorAnnotationsFilterProxyModel* proxyModel = new ColorAnnotationsFilterProxyModel(this);
  proxyModel->setSourceModel(this->Internals->Model);
  ui.AnnotationsTable->setModel(proxyModel);
  ui.AnnotationsTable->setSortingEnabled(false);

  new pqColorAnnotationsSelectionHelper(ui.AnnotationsTable);
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
    indexedColors.empty())
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
  QAbstractItemModel* model = this->Internals->Ui.AnnotationsTable->model();
  if (!model)
  {
    return;
  }
  double globalOpacity =
    model->headerData(pqAnnotationsModel::OPACITY, Qt::Horizontal, Qt::EditRole).toDouble();
  pqOpacityRangeDialog dialog(globalOpacity, this);
  dialog.setWindowTitle(tr("Set global opacity"));
  dialog.move(QCursor::pos());
  if (dialog.exec() == QDialog::Accepted)
  {
    // Set the global opacity
    model->setHeaderData(
      pqAnnotationsModel::OPACITY, Qt::Horizontal, dialog.opacity(), Qt::EditRole);
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
  std::vector<std::pair<QString, QString>> annotationsData;
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
    [](std::pair<QString, int> a, std::pair<QString, int> b)
    { return a.first.compare(b.first, Qt::CaseInsensitive) < 0; });
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
  std::vector<std::pair<QString, int>> visibilities;

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
  if (indexes.empty())
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
    QString warningTitle(tr("Could not determine discrete values"));
    QString warningMessage;
    QTextStream qs(&warningMessage);
    qs << tr("Could not automatically determine annotation values. Usually this means "
             "too many discrete values (more than %1) "
             "are available in the data produced by the "
             "current source/filter.")
            .arg(vtkAbstractArray::MAX_DISCRETE_VALUES)
       << tr(" This can happen if the data array type is floating "
             "point. Please add annotations manually or force generation. Forcing the "
             "generation will automatically hide the Scalar Bar.");
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
  return this->addActiveAnnotations(force, true);
}

//-----------------------------------------------------------------------------
bool pqColorAnnotationsWidget::addActiveAnnotations(bool force, bool extend)
{
  // obtain prominent values from the server and add them
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr)
  {
    return false;
  }

  vtkPVProminentValuesInformation* info =
    vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(
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
  if (internals.updateAnnotations(uniqueValues, extend))
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
    QString warningTitle(tr("Could not determine discrete values"));
    QString warningMessage;
    QTextStream qs(&warningMessage);
    qs << tr("Could not automatically determine annotation values. Usually this means "
             "too many discrete values (more than %1) "
             "are available in the data produced by the "
             "current source/filter.")
            .arg(vtkAbstractArray::MAX_DISCRETE_VALUES)
       << tr(" This can happen if the data array type is floating "
             "point. Please add annotations manually or force generation. Forcing the "
             "generation will automatically hide the Scalar Bar.");

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

  vtkSMRepresentationProxy* activeRepresentationProxy =
    vtkSMRepresentationProxy::SafeDownCast(repr->getProxy());
  if (!activeRepresentationProxy)
  {
    return false;
  }

  vtkPVArrayInformation* activeArrayInfo =
    vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(activeRepresentationProxy);

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
      vtkSMColorMapEditorHelper::GetArrayInformationForColorArray(representationProxy);
    if (!activeArrayInfo || !activeArrayInfo->GetName() || !currentArrayInfo ||
      !currentArrayInfo->GetName() ||
      strcmp(activeArrayInfo->GetName(), currentArrayInfo->GetName()) != 0)
    {
      continue;
    }

    vtkPVProminentValuesInformation* info =
      vtkSMColorMapEditorHelper::GetProminentValuesInformationForColorArray(
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
  this->Internals->Model->setHeaderData(
    pqAnnotationsModel::OPACITY, Qt::Horizontal, 1.0, Qt::EditRole);
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
  ui.saveAnnotations->setText(QString(tr("Save %1s")).arg(name));

  // For now, let's not provide an option to not save colors. We'll need to fix
  // the pqPresetToPixmap to support rendering only opacities.
  ui.saveColors->setChecked(true);
  ui.saveColors->setEnabled(false);
  ui.saveColors->hide();
  ui.presetName->setText(tr("Preset"));

  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  this->saveAsPreset(
    ui.presetName->text().toUtf8().data(), !ui.saveAnnotations->isChecked(), false);
}

//-----------------------------------------------------------------------------
void pqColorAnnotationsWidget::saveAsPreset(
  const char* defaultName, bool removeAnnotations, bool allowOverride)
{
  Json::Value cpreset =
    vtkSMTransferFunctionProxy::GetStateAsPreset(this->Internals->LookupTableProxy);

  if (cpreset[INDEXED_COLORS].empty())
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
  delete this->Internals->Model;

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
