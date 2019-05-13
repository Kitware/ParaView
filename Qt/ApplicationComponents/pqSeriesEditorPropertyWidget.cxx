/*=========================================================================

   Program: ParaView
   Module:  pqSeriesEditorPropertyWidget.cxx

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
#include "pqSeriesEditorPropertyWidget.h"
#include "ui_pqSeriesEditorPropertyWidget.h"

#include "pqPropertiesPanel.h"
#include "pqSMAdaptor.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QAbstractTableModel>
#include <QByteArray>
#include <QColorDialog>
#include <QDataStream>
#include <QFontMetrics>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QPair>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QVector>

#include <cassert>

//=============================================================================
// QAbstractTableModel for showing the series properties. Since series
// properties are specified on several different vtkSMProperty instances on the
// proxy, there could easily be a mismatch i.e. a series is specified in
// "SeriesVisibility", but not in "SeriesColor". To overcome that issues, this
// model uses the "SeriesVisibility" as the main driver for the model. There
// will exactly as many rows as the number of series specified in
// SeriesVisibility. For all other properties, extra values will not be shown
// and default values will be made up for missing series.
class pqSeriesParametersModel : public QAbstractTableModel
{
  typedef QAbstractTableModel Superclass;

  bool SupportsReorder;
  QIcon MissingColorIcon;
  QVector<QPair<QString, bool> > Visibilities;
  QMap<QString, QString> Labels;
  QMap<QString, QColor> Colors;
  QPointer<QWidget> Widget; // used to determine text size.
  vtkSmartPointer<vtkSMChartSeriesSelectionDomain> Domain;

  QColor seriesColor(const QModelIndex& idx) const
  {
    if (idx.isValid() && (idx.row() < this->Visibilities.size()) &&
      this->Colors.contains(this->Visibilities[idx.row()].first))
    {
      return this->Colors[this->Visibilities[idx.row()].first];
    }
    return QColor();
  }

  // returns true is the series is in Domain else false.
  bool isSeriesInDomain(const QString& _seriesName) const
  {
    unsigned int unused = 0;
    if (this->Domain && this->Domain->IsInDomain(_seriesName.toLocal8Bit().data(), unused) != 0)
    {
      return true;
    }
    // if no domain is present, then the series is treated is always visible.
    return (this->Domain.GetPointer() != NULL) ? false : true;
  }

public:
  pqSeriesParametersModel(bool supportsReorder, QObject* parentObject = 0)
    : Superclass(parentObject)
    , SupportsReorder(supportsReorder)
    , MissingColorIcon(":/pqWidgets/Icons/pqUnknownData16.png")
  {
    this->Visibilities.push_back(QPair<QString, bool>("Alpha", true));
    this->Visibilities.push_back(QPair<QString, bool>("Beta", false));
  }
  ~pqSeriesParametersModel() override {}
  void setWidget(QWidget* wdg) { this->Widget = wdg; }
  void setVisibilityDomain(vtkSMChartSeriesSelectionDomain* domain) { this->Domain = domain; }
  void domainChanged()
  {
    // fire dataChanged signal so the filtering logic can kick in.
    emit this->dataChanged(
      this->index(0, VISIBILITY), this->index(this->rowCount() - 1, VISIBILITY));
  }

  enum ColumnRoles
  {
    VISIBILITY = 0,
    COLOR = 1,
    LABEL = 2
  };

  /// Drag/drop of rows is enabled for cases were the series ordering is
  /// relevant e.g. parallel coordinates/scatter plot matrix.
  Qt::ItemFlags flags(const QModelIndex& idx) const override
  {
    Qt::ItemFlags value = this->Superclass::flags(idx);
    if (this->SupportsReorder)
    {
      value |= Qt::ItemIsDropEnabled;
    }
    if (idx.isValid())
    {
      value |= Qt::ItemIsDragEnabled;
      switch (idx.column())
      {
        case VISIBILITY:
          return value | Qt::ItemIsUserCheckable;
        case LABEL:
          return value | Qt::ItemIsEditable;
        case COLOR:
        default:
          break;
      }
    }
    return value;
  }

  int rowCount(const QModelIndex& idx = QModelIndex()) const override
  {
    return idx.isValid() ? 0 : this->Visibilities.size();
  }

  int columnCount(const QModelIndex& idx = QModelIndex()) const override
  {
    Q_UNUSED(idx);
    return 3;
  }

  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override
  {
    assert(idx.row() < this->Visibilities.size());
#ifndef __APPLE__
    // On OSX, the default row-size ends up being reasonable. Hence, don't override it on OsX.
    if (role == Qt::SizeHintRole)
    {
      // this make the rows appear less crowded.
      if (this->Widget && idx.column() != COLOR)
      {
        int height = this->Widget->fontMetrics().boundingRect("(").height();
        return QSize(0, static_cast<int>(height * 1.30)); // pad each row
      }
      return QVariant();
    }
#endif
    if (idx.column() == VISIBILITY)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
          return this->Visibilities[idx.row()].first;

        case Qt::UserRole:
          // check if the series is in domain and then show/hide it.
          return this->isSeriesInDomain(this->Visibilities[idx.row()].first) ? "1" : "0";

        case Qt::CheckStateRole:
          return this->Visibilities[idx.row()].second ? Qt::Checked : Qt::Unchecked;
      }
    }
    else if (idx.column() == COLOR)
    {
      QColor color;
      switch (role)
      {
        case Qt::DecorationRole:
          color = this->seriesColor(idx);
          return color.isValid() ? QVariant(color) : QVariant(this->MissingColorIcon);

        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
          return "Series Color";
      }
    }
    else if (idx.column() == LABEL)
    {
      QString label;
      if (this->Labels.contains(this->Visibilities[idx.row()].first))
      {
        label = this->Labels[this->Visibilities[idx.row()].first];
      }
      else
      {
        label = this->Visibilities[idx.row()].first;
      }

      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::EditRole:
          return label;
      }
    }

    return QVariant();
  }

  bool setData(const QModelIndex& idx, const QVariant& value, int role = Qt::EditRole) override
  {
    if (idx.column() == VISIBILITY && role == Qt::CheckStateRole)
    {
      bool checkState = (value.toInt() == Qt::Checked);
      assert(idx.row() < this->Visibilities.size());
      this->Visibilities[idx.row()].second = checkState;
      emit this->dataChanged(idx, idx);
      return true;
    }
    else if (idx.column() == COLOR && role == Qt::EditRole)
    {
      assert(idx.row() < this->Visibilities.size());
      if (value.canConvert(QVariant::Color))
      {
        this->Colors[this->Visibilities[idx.row()].first] = value.value<QColor>();
        emit this->dataChanged(idx, idx);
        return true;
      }
    }
    else if (idx.column() == LABEL && role == Qt::EditRole)
    {
      assert(idx.row() < this->Visibilities.size());
      this->Labels[this->Visibilities[idx.row()].first] = value.toString();
      emit this->dataChanged(idx, idx);
      return true;
    }
    return false;
  }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole))
    {
      switch (section)
      {
        case VISIBILITY:
          return role == Qt::DisplayRole ? "Variable" : "Toggle series visibility";
        case COLOR:
          return role == Qt::DisplayRole ? "" : "Set color to use for the series";
        case LABEL:
          return role == Qt::DisplayRole ? "Legend Name"
                                         : "Set the text to use for the series in the legend";
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  QString seriesName(const QModelIndex& idx) const
  {
    if (idx.isValid() && idx.row() < this->Visibilities.size())
    {
      return this->Visibilities[idx.row()].first;
    }
    return QString();
  }

  void setVisibilities(const QVector<QPair<QString, bool> >& new_visibilies)
  {
    emit this->beginResetModel();
    this->Visibilities = new_visibilies;
    emit this->endResetModel();
  }

  const QVector<QPair<QString, bool> >& visibilities() const { return this->Visibilities; }

  void setLabels(const QVector<QPair<QString, QString> >& new_labels)
  {
    typedef QPair<QString, QString> item_type;
    foreach (const item_type& pair, new_labels)
    {
      this->Labels[pair.first] = pair.second;
    }

    if (this->rowCount() > 0)
    {
      emit this->dataChanged(this->index(0, LABEL), this->index(this->rowCount() - 1, LABEL));
    }
  }

  const QVector<QPair<QString, QString> > labels() const
  {
    QVector<QPair<QString, QString> > reply;

    // return labels for the ones we have visibility information.
    typedef QPair<QString, bool> item_type;
    foreach (const item_type& pair, this->Visibilities)
    {
      if (this->Labels.contains(pair.first))
      {
        reply.push_back(QPair<QString, QString>(pair.first, this->Labels[pair.first]));
      }
      else
      {
        reply.push_back(QPair<QString, QString>(pair.first, pair.first));
      }
    }
    return reply;
  }

  void setColors(const QVector<QPair<QString, QColor> >& new_colors)
  {
    typedef QPair<QString, QColor> item_type;
    foreach (const item_type& pair, new_colors)
    {
      this->Colors[pair.first] = pair.second;
    }
    if (this->rowCount() > 0)
    {
      emit this->dataChanged(this->index(0, COLOR), this->index(this->rowCount() - 1, COLOR));
    }
  }

  const QVector<QPair<QString, QColor> > colors() const
  {
    QVector<QPair<QString, QColor> > reply;

    // return labels for the ones we have visibility information.
    typedef QPair<QString, bool> item_type;
    foreach (const item_type& pair, this->Visibilities)
    {
      if (this->Colors.contains(pair.first))
      {
        reply.push_back(QPair<QString, QColor>(pair.first, this->Colors[pair.first]));
      }
    }
    return reply;
  }

  //--------- Drag-N-Drop support when enabled --------
  Qt::DropActions supportedDropActions() const override
  {
    return this->SupportsReorder ? (Qt::CopyAction | Qt::MoveAction)
                                 : this->Superclass::supportedDropActions();
  }

  QStringList mimeTypes() const override
  {
    if (this->SupportsReorder)
    {
      QStringList types;
      types << "application/paraview.series.list";
      return types;
    }

    return this->Superclass::mimeTypes();
  }

  QMimeData* mimeData(const QModelIndexList& indexes) const override
  {
    if (!this->SupportsReorder)
    {
      return this->Superclass::mimeData(indexes);
    }
    QMimeData* mime_data = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<QString> keys;
    foreach (const QModelIndex& idx, indexes)
    {
      QString name = this->seriesName(idx);
      if (!name.isEmpty() && !keys.contains(name))
      {
        keys << this->seriesName(idx);
      }
    }
    foreach (const QString& str, keys)
    {
      stream << str;
    }
    mime_data->setData("application/paraview.series.list", encodedData);
    return mime_data;
  }

  bool dropMimeData(const QMimeData* mime_data, Qt::DropAction action, int row, int column,
    const QModelIndex& parentIdx) override
  {
    if (!this->SupportsReorder)
    {
      return this->Superclass::dropMimeData(mime_data, action, row, column, parentIdx);
    }
    if (action == Qt::IgnoreAction)
    {
      return true;
    }
    if (!mime_data->hasFormat("application/paraview.series.list"))
    {
      return false;
    }

    int beginRow = -1;
    if (row != -1)
    {
      beginRow = row;
    }
    else if (parentIdx.isValid())
    {
      beginRow = parentIdx.row();
    }
    else
    {
      beginRow = this->rowCount();
    }
    if (beginRow < 0)
    {
      return false;
    }

    QByteArray encodedData = mime_data->data("application/paraview.series.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    while (!stream.atEnd())
    {
      QString text;
      stream >> text;
      newItems << text;
    }

    // now re-order the visibilities list.
    QVector<QPair<QString, bool> > new_visibilies;
    QMap<QString, bool> to_insert;

    int real_begin_row = -1;
    for (int cc = 0; cc < this->Visibilities.size(); cc++)
    {
      if (cc == beginRow)
      {
        real_begin_row = new_visibilies.size();
      }
      if (!newItems.contains(this->Visibilities[cc].first))
      {
        new_visibilies.push_back(this->Visibilities[cc]);
      }
      else
      {
        to_insert.insert(this->Visibilities[cc].first, this->Visibilities[cc].second);
      }
    }
    if (real_begin_row == -1)
    {
      real_begin_row = new_visibilies.size();
    }
    foreach (const QString& item, newItems)
    {
      if (to_insert.contains(item))
      {
        new_visibilies.insert(real_begin_row, QPair<QString, bool>(item, to_insert[item]));
        real_begin_row++;
      }
    }
    this->setVisibilities(new_visibilies);
    emit this->dataChanged(this->index(0, VISIBILITY), this->index(this->rowCount() - 1, LABEL));
    return true;
  }

private:
  Q_DISABLE_COPY(pqSeriesParametersModel)
};

//=============================================================================

class pqSeriesEditorPropertyWidget::pqInternals
{
public:
  Ui::SeriesEditorPropertyWidget Ui;
  vtkSmartPointer<vtkSMPropertyGroup> PropertyGroup;
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  pqSeriesParametersModel Model;
  QMap<QString, int> Thickness;
  QMap<QString, int> Style;
  QMap<QString, int> MarkerStyle;
  QMap<QString, int> PlotCorner;
  bool RefreshingWidgets;

  pqInternals(bool supportsReorder, pqSeriesEditorPropertyWidget* self)
    : Model(supportsReorder)
    , RefreshingWidgets(false)
  {
    this->Ui.setupUi(self);
    this->Ui.wdgLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.wdgLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    this->Ui.wdgLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());

    this->Ui.SeriesTable->setDragEnabled(supportsReorder);
    this->Ui.SeriesTable->setDragDropMode(
      supportsReorder ? QAbstractItemView::InternalMove : QAbstractItemView::NoDragDrop);
    this->Ui.SeriesTable->header()->setHighlightSections(false);

    // give the model a widget so it can compute text sizes better.
    this->Model.setWidget(this->Ui.SeriesTable);

    QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(self);
    proxyModel->setSourceModel(&this->Model);
    this->Ui.SeriesTable->setModel(proxyModel);
    // sorting is enabled only when re-ordering is not supported i.e. when the
    // order of the series is irrelevant for the plot.
    this->Ui.SeriesTable->setSortingEnabled(!supportsReorder);

    // Adds support for "Check/UnCheck" context menu to toggle check state for
    // selected items easily.
    new pqTreeViewSelectionHelper(this->Ui.SeriesTable);

    // Add filtering capabilities.
    // Conditionally hides rows that are no longer present in the domain.
    // This keeps the view showing too many rows that are no longer applicable.
    // The UI will (TODO) a mechanism to see all available values.
    proxyModel->setFilterRole(Qt::UserRole);
    proxyModel->setFilterRegExp("^1$");
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // this is needed so that the filter is updated every time data changes.
    proxyModel->setDynamicSortFilter(true);

// this needs to be done after the columns exist.
#if QT_VERSION >= 0x050000
    this->Ui.SeriesTable->header()->setSectionResizeMode(
      pqSeriesParametersModel::VISIBILITY, QHeaderView::Interactive);
    this->Ui.SeriesTable->header()->setSectionResizeMode(
      pqSeriesParametersModel::COLOR, QHeaderView::ResizeToContents);
    this->Ui.SeriesTable->header()->setSectionResizeMode(
      pqSeriesParametersModel::LABEL, QHeaderView::Interactive);
#else
    this->Ui.SeriesTable->header()->setResizeMode(
      pqSeriesParametersModel::VISIBILITY, QHeaderView::Interactive);
    this->Ui.SeriesTable->header()->setResizeMode(
      pqSeriesParametersModel::COLOR, QHeaderView::ResizeToContents);
    this->Ui.SeriesTable->header()->setResizeMode(
      pqSeriesParametersModel::LABEL, QHeaderView::Interactive);
#endif
  }

  //---------------------------------------------------------------------------
  // maps an index to that for this->Model. This is needed since we sometimes
  // use a proxy-model for sorting. When unsure, just call this. The logic here
  // handles correctly when this is called on an index already associated with
  // this->Model.
  QModelIndex modelIndex(const QModelIndex& idx) const
  {
    const QSortFilterProxyModel* proxyModel =
      qobject_cast<const QSortFilterProxyModel*>(idx.model());
    return proxyModel ? proxyModel->mapToSource(idx) : idx;
  }
};
//=============================================================================

//-----------------------------------------------------------------------------
pqSeriesEditorPropertyWidget::pqSeriesEditorPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(NULL)
{
  if (vtkSMProperty* smproperty = smgroup->GetProperty("SeriesVisibility"))
  {
    vtkPVXMLElement* hints = smproperty->GetHints()
      ? smproperty->GetHints()->FindNestedElementByName("SeriesEditor")
      : NULL;
    int value = 0;
    bool supportsReorder = (hints != NULL &&
      hints->GetScalarAttribute("supports_reordering", &value) != 0 && value == 1);
    this->Internals = new pqInternals(supportsReorder, this);
  }
  else
  {
    qCritical("SeriesVisibility property is required by pqSeriesEditorPropertyWidget."
              " This widget is not going to work.");
    return;
  }
  this->Internals->PropertyGroup = smgroup;

  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;

  this->addPropertyLink(this, "seriesVisibility", SIGNAL(seriesVisibilityChanged()),
    smgroup->GetProperty("SeriesVisibility"));
  this->Internals->Model.setVisibilityDomain(
    smgroup->GetProperty("SeriesVisibility")->FindDomain<vtkSMChartSeriesSelectionDomain>());
  this->Internals->Ui.SeriesTable->sortByColumn(0, Qt::AscendingOrder);

  this->Internals->VTKConnector->Connect(smgroup->GetProperty("SeriesVisibility"),
    vtkCommand::DomainModifiedEvent, this, SLOT(domainModified(vtkObject*)));

  if (smgroup->GetProperty("SeriesLabel"))
  {
    this->addPropertyLink(
      this, "seriesLabel", SIGNAL(seriesLabelChanged()), smgroup->GetProperty("SeriesLabel"));
  }
  else
  {
    ui.SeriesTable->hideColumn(pqSeriesParametersModel::LABEL);
  }

  if (smgroup->GetProperty("SeriesColor"))
  {
    this->addPropertyLink(
      this, "seriesColor", SIGNAL(seriesColorChanged()), smgroup->GetProperty("SeriesColor"));
  }
  else
  {
    ui.SeriesTable->hideColumn(pqSeriesParametersModel::COLOR);
  }

  if (smgroup->GetProperty("SeriesLineThickness"))
  {
    this->addPropertyLink(this, "seriesLineThickness", SIGNAL(seriesLineThicknessChanged()),
      smgroup->GetProperty("SeriesLineThickness"));
  }
  else
  {
    ui.ThicknessLabel->hide();
    ui.Thickness->hide();
  }

  if (smgroup->GetProperty("SeriesLineStyle"))
  {
    this->addPropertyLink(this, "seriesLineStyle", SIGNAL(seriesLineStyleChanged()),
      smgroup->GetProperty("SeriesLineStyle"));
  }
  else
  {
    ui.StyleListLabel->hide();
    ui.StyleList->hide();
  }

  if (smgroup->GetProperty("SeriesMarkerStyle"))
  {
    this->addPropertyLink(this, "seriesMarkerStyle", SIGNAL(seriesMarkerStyleChanged()),
      smgroup->GetProperty("SeriesMarkerStyle"));
  }
  else
  {
    ui.MarkerStyleListLabel->hide();
    ui.MarkerStyleList->hide();
  }

  if (smgroup->GetProperty("SeriesPlotCorner"))
  {
    this->addPropertyLink(this, "seriesPlotCorner", SIGNAL(seriesPlotCornerChanged()),
      smgroup->GetProperty("SeriesPlotCorner"));
  }
  else
  {
    ui.AxisListLabel->hide();
    ui.AxisList->hide();
  }

  QObject::connect(&this->Internals->Model,
    SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this,
    SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));

  this->connect(ui.SeriesTable, SIGNAL(doubleClicked(const QModelIndex&)),
    SLOT(onDoubleClicked(const QModelIndex&)));

  this->connect(ui.SeriesTable->selectionModel(),
    SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
    SLOT(refreshPropertiesWidgets()));

  this->connect(ui.Thickness, SIGNAL(valueChanged(int)), SLOT(savePropertiesWidgets()));
  this->connect(ui.StyleList, SIGNAL(currentIndexChanged(int)), SLOT(savePropertiesWidgets()));
  this->connect(
    ui.MarkerStyleList, SIGNAL(currentIndexChanged(int)), SLOT(savePropertiesWidgets()));
  this->connect(ui.AxisList, SIGNAL(currentIndexChanged(int)), SLOT(savePropertiesWidgets()));
}

//-----------------------------------------------------------------------------
pqSeriesEditorPropertyWidget::~pqSeriesEditorPropertyWidget()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::onDataChanged(
  const QModelIndex& topleft, const QModelIndex& btmright)
{
  // We don't need to worry about proxyModel here since we're directly observing
  // signal on this->Internals->Model.
  if (topleft.column() == 0)
  {
    emit this->seriesVisibilityChanged();
  }
  if (topleft.column() <= 1 && btmright.column() >= 1)
  {
    emit this->seriesColorChanged();
  }
  if (btmright.column() >= 2)
  {
    emit this->seriesLabelChanged();
  }
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::onDoubleClicked(const QModelIndex& idx)
{
  // when user double-clicks on the color-swatch in the table view, we popup the
  // color selector dialog.
  if (idx.column() == 1)
  {
    QModelIndex index = this->Internals->modelIndex(idx);
    QColor color = this->Internals->Model.data(index, Qt::DecorationRole).value<QColor>();
    color =
      QColorDialog::getColor(color, this, "Choose Series Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
      this->Internals->Model.setData(index, color);
    }
  }
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesVisibility(const QList<QVariant>& values)
{
  QVector<QPair<QString, bool> > vdata;
  vdata.resize(values.size() / 2);
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    vdata[cc / 2].first = values[cc].toString();
    vdata[cc / 2].second = values[cc + 1].toString() == "1";
  }
  this->Internals->Model.setVisibilities(vdata);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesVisibility() const
{
  const QVector<QPair<QString, bool> >& vdata = this->Internals->Model.visibilities();

  QList<QVariant> reply;
  for (int cc = 0; cc < vdata.size(); cc++)
  {
    reply.push_back(vdata[cc].first);
    reply.push_back(vdata[cc].second ? "1" : "0");
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLabel(const QList<QVariant>& values)
{
  QVector<QPair<QString, QString> > vdata;
  vdata.resize(values.size() / 2);
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    vdata[cc / 2].first = values[cc].toString();
    vdata[cc / 2].second = values[cc + 1].toString();
  }
  this->Internals->Model.setLabels(vdata);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLabel() const
{
  const QVector<QPair<QString, QString> >& vdata = this->Internals->Model.labels();
  QList<QVariant> reply;
  for (int cc = 0; cc < vdata.size(); cc++)
  {
    reply.push_back(vdata[cc].first);
    reply.push_back(vdata[cc].second);
  }
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesColor(const QList<QVariant>& values)
{
  QVector<QPair<QString, QColor> > vdata;
  vdata.resize(values.size() / 4);
  for (int cc = 0; (cc + 3) < values.size(); cc += 4)
  {
    QColor color;
    color.setRedF(values[cc + 1].toDouble());
    color.setGreenF(values[cc + 2].toDouble());
    color.setBlueF(values[cc + 3].toDouble());

    vdata[cc / 4].first = values[cc].toString();
    vdata[cc / 4].second = color;
  }
  this->Internals->Model.setColors(vdata);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesColor() const
{
  const QVector<QPair<QString, QColor> >& vdata = this->Internals->Model.colors();
  QList<QVariant> reply;
  for (int cc = 0; cc < vdata.size(); cc++)
  {
    reply.push_back(vdata[cc].first);
    reply.push_back(QString::number(vdata[cc].second.redF()));
    reply.push_back(QString::number(vdata[cc].second.greenF()));
    reply.push_back(QString::number(vdata[cc].second.blueF()));
  }
  return reply;
}

namespace
{
template <class T>
void setSeriesValues(QMap<QString, T>& data, const QList<QVariant>& values)
{
  data.clear();
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    data[values[cc].toString()] = values[cc + 1].value<T>();
  }
}

template <class T>
void getSeriesValues(const QMap<QString, T>& data, QList<QVariant>& reply)
{
  QMap<QString, int>::const_iterator iter = data.constBegin();
  for (; iter != data.constEnd(); ++iter)
  {
    reply.push_back(iter.key());
    reply.push_back(QString::number(iter.value()));
  }
}
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLineThickness(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->Thickness, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLineThickness() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->Thickness, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesLineStyle(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->Style, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesLineStyle() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->Style, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesMarkerStyle(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->MarkerStyle, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesMarkerStyle() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->MarkerStyle, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::setSeriesPlotCorner(const QList<QVariant>& values)
{
  setSeriesValues<int>(this->Internals->PlotCorner, values);
  this->refreshPropertiesWidgets();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqSeriesEditorPropertyWidget::seriesPlotCorner() const
{
  QList<QVariant> reply;
  getSeriesValues(this->Internals->PlotCorner, reply);
  return reply;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::refreshPropertiesWidgets()
{
  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;
  pqSeriesParametersModel& model = this->Internals->Model;

  QModelIndex idx = this->Internals->modelIndex(ui.SeriesTable->currentIndex());
  QString key = model.seriesName(idx);
  if (!idx.isValid() || key.isEmpty())
  {
    // nothing is selected, disable all properties widgets.
    ui.AxisList->setEnabled(false);
    ui.MarkerStyleList->setEnabled(false);
    ui.StyleList->setEnabled(false);
    ui.Thickness->setEnabled(false);
    return;
  }

  this->Internals->RefreshingWidgets = true;
  ui.Thickness->setValue(this->Internals->Thickness[key]);
  ui.Thickness->setEnabled(true);

  ui.StyleList->setCurrentIndex(this->Internals->Style[key]);
  ui.StyleList->setEnabled(true);

  ui.MarkerStyleList->setCurrentIndex(this->Internals->MarkerStyle[key]);
  ui.MarkerStyleList->setEnabled(true);

  ui.AxisList->setCurrentIndex(this->Internals->PlotCorner[key]);
  ui.AxisList->setEnabled(true);
  this->Internals->RefreshingWidgets = false;
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::savePropertiesWidgets()
{
  if (this->Internals->RefreshingWidgets)
  {
    return;
  }

  Ui::SeriesEditorPropertyWidget& ui = this->Internals->Ui;
  pqSeriesParametersModel& model = this->Internals->Model;

  QWidget* senderWidget = qobject_cast<QWidget*>(this->sender());
  assert(senderWidget);

  QModelIndexList selectedIndexes = ui.SeriesTable->selectionModel()->selectedIndexes();
  foreach (QModelIndex selIdx, selectedIndexes)
  {
    QModelIndex idx = this->Internals->modelIndex(selIdx);
    QString key = model.seriesName(idx);
    if (!idx.isValid() || key.isEmpty())
    {
      continue;
    }

    // update the parameter corresponding to the modified widget.
    if (ui.Thickness == senderWidget && this->Internals->Thickness[key] != ui.Thickness->value())
    {
      this->Internals->Thickness[key] = ui.Thickness->value();
      emit this->seriesLineThicknessChanged();
    }
    else if (ui.StyleList == senderWidget &&
      this->Internals->Style[key] != ui.StyleList->currentIndex())
    {
      this->Internals->Style[key] = ui.StyleList->currentIndex();
      emit this->seriesLineStyleChanged();
    }
    else if (ui.MarkerStyleList == senderWidget &&
      this->Internals->MarkerStyle[key] != ui.MarkerStyleList->currentIndex())
    {
      this->Internals->MarkerStyle[key] = ui.MarkerStyleList->currentIndex();
      emit this->seriesMarkerStyleChanged();
    }
    else if (ui.AxisList == senderWidget &&
      this->Internals->PlotCorner[key] != ui.AxisList->currentIndex())
    {
      this->Internals->PlotCorner[key] = ui.AxisList->currentIndex();
      emit this->seriesPlotCornerChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void pqSeriesEditorPropertyWidget::domainModified(vtkObject*)
{
  // Trigger dataChanged() signals on the model so that the list refreshes to
  // hide series that are no longer in domain.
  this->Internals->Model.domainChanged();

  // Sort the table
  this->Internals->Ui.SeriesTable->sortByColumn(0, Qt::AscendingOrder);
}
