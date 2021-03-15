/*=========================================================================

   Program: ParaView
   Module:  pqDataAssemblyPropertyWidget.cxx

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
#include "pqDataAssemblyPropertyWidget.h"
#include "ui_pqDataAssemblyPropertyWidget.h"

#include "pqApplicationCore.h"
#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDataAssemblyTreeModel.h"
#include "pqDoubleRangeDialog.h"
#include "pqOutputPort.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqTreeViewExpandState.h"
#include "pqTreeViewSelectionHelper.h"
#include "vtkCommand.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMDataAssemblyDomain.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <QColorDialog>
#include <QIdentityProxyModel>
#include <QPainter>
#include <QScopedValueRollback>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QVBoxLayout>

#include <algorithm>
#include <iterator>
#include <set>

namespace detail
{
constexpr int ColorRole = Qt::UserRole;
constexpr int OpacityRole = Qt::UserRole + 1;

//-----------------------------------------------------------------------------
/**
 * Creates a pixmap for color.
 */
//-----------------------------------------------------------------------------
QPixmap ColorPixmap(int iconSize, const QColor& color, bool inherited)
{
  QPixmap pixmap(iconSize, iconSize);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  if (color.isValid())
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    QBrush brush = painter.brush();
    brush.setColor(color);
    brush.setStyle(inherited ? Qt::Dense5Pattern : Qt::SolidPattern);
    painter.setBrush(brush);
  }
  else
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    painter.setPen(pen);

    painter.setBrush(Qt::NoBrush);
  }
  const int radius = 3 * iconSize / 8;
  painter.drawEllipse(QPoint(iconSize / 2, iconSize / 2), radius, radius);
  return pixmap;
}

//-----------------------------------------------------------------------------
/**
 * Creates a pixmap for opacity.
 */
//-----------------------------------------------------------------------------
QPixmap OpacityPixmap(int iconSize, double opacity, bool inherited)
{
  QPixmap pixmap(iconSize, iconSize);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);

  if (opacity >= 0.0 && opacity <= 1.0)
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);

    QBrush brush = painter.brush();
    brush.setColor(Qt::gray);
    brush.setStyle(inherited ? Qt::Dense7Pattern : Qt::SolidPattern);
    painter.setBrush(brush);
  }
  else
  {
    QPen pen = painter.pen();
    pen.setColor(Qt::black);
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);
    painter.setPen(pen);

    painter.setBrush(Qt::NoBrush);
    opacity = 1.0;
  }

  const int delta = 3 * iconSize / 4;
  QRect rect(0, 0, delta, delta);
  rect.moveCenter(QPoint(iconSize / 2, iconSize / 2));
  painter.drawPie(rect, 0, 5760 * opacity);
  return pixmap;
}

} // end of detail

namespace
{
//=================================================================================
/**
 * A table model to show lists for chosen selectors + properties. This helps use
 * show a compact/non-hierarchical view of selectors and properties like opacity
 * or color specified.
 */
class TableModel : public QAbstractTableModel
{
  using Superclass = QAbstractTableModel;

public:
  enum ModeT
  {
    SELECTORS,
    COLORS,
    OPACITIES,
  };

  TableModel(int pixmapSize, ModeT mode, QObject* prnt)
    : Superclass(prnt)
    , PixmapSize(pixmapSize)
    , Mode(mode)
  {
  }
  ~TableModel() override = default;

  int columnCount(const QModelIndex&) const override
  {
    switch (this->Mode)
    {
      case COLORS:
        // selector, r, g, b
        return 4;
      case OPACITIES:
        // selector, opacity
        return 2;
      case SELECTORS:
      default:
        // selector
        return 1;
    }
  }

  int rowCount(const QModelIndex&) const override { return this->Values.size(); }

  Qt::ItemFlags flags(const QModelIndex& indx) const override
  {
    return this->Superclass::flags(indx) | Qt::ItemIsEditable;
  }

  void setData(const QList<QPair<QString, QVariant> >& values)
  {
    if (this->Values != values)
    {
      this->beginResetModel();
      this->Values = values;
      this->endResetModel();
    }
  }

  void setData(const QStringList& selectors)
  {
    Q_ASSERT(this->Mode == SELECTORS);
    QList<QPair<QString, QVariant> > values;
    for (const auto& value : selectors)
    {
      values.push_back(qMakePair(value, QVariant()));
    }
    this->setData(values);
  }

  QStringList selectors() const
  {
    QStringList val;
    for (auto& pair : this->Values)
    {
      val.push_back(pair.first);
    }
    return val;
  }

  const QList<QPair<QString, QVariant> >& values() const { return this->Values; }

  QVariant headerData(int section, Qt::Orientation orientation, int role) const override
  {
    if (orientation != Qt::Horizontal)
    {
      return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
      switch (section)
      {
        case 0:
          return "Selector";
        case 1:
          return this->Mode == OPACITIES ? "Opacity" : "Red  ";
        case 2:
          return "Green";
        case 3:
          return "Blue ";
      }
    }
    else if (role == Qt::DecorationRole)
    {
      switch (section)
      {
        case 1:
          return this->Mode == OPACITIES
            ? detail::OpacityPixmap(this->PixmapSize, 0.75, false)
            : detail::ColorPixmap(this->PixmapSize, QColor(Qt::red), false);

        case 2:
          return detail::ColorPixmap(this->PixmapSize, QColor(Qt::green), false);

        case 3:
          return detail::ColorPixmap(this->PixmapSize, QColor(Qt::blue), false);
      }
    }
    return QVariant();
  }

  bool setData(const QModelIndex& index, const QVariant& value, int role) override
  {
    if (index.row() >= 0 && index.row() < this->Values.size() &&
      (role == Qt::EditRole || role == Qt::DisplayRole))
    {
      bool changed = false;
      auto& pair = this->Values[index.row()];
      if (index.column() == 0)
      {
        if (pair.first != value.toString())
        {
          pair.first = value.toString();
          changed = true;
        }
      }
      else if (this->Mode == OPACITIES)
      {
        if (pair.second != value)
        {
          pair.second = value;
          changed = true;
        }
      }
      else if (this->Mode == COLORS)
      {
        auto color = pair.second.value<QColor>();
        double rgbF[3] = { color.redF(), color.greenF(), color.blueF() };
        rgbF[index.column() - 1] = value.toDouble();
        if (color != QColor::fromRgbF(rgbF[0], rgbF[1], rgbF[2]))
        {
          pair.second = QColor::fromRgbF(rgbF[0], rgbF[1], rgbF[2]);
          changed = true;
        }
      }

      if (changed)
      {
        emit this->dataChanged(index, index, { role });
      }
      return changed;
    }
    return false;
  }
  QVariant data(const QModelIndex& indx, int role) const override
  {
    const auto& pair = this->Values[indx.row()];
    if (indx.column() == 0)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
          return pair.first;
        case Qt::DecorationRole:
          return this->Mode == COLORS
            ? detail::ColorPixmap(this->PixmapSize, pair.second.value<QColor>(), false)
            : detail::OpacityPixmap(this->PixmapSize, pair.second.value<double>(), false);
      }
    }
    else if (indx.column() == 1 && this->Mode == OPACITIES)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
          return QString::number(pair.second.toDouble(), 'f', 2);
      }
    }
    else if (indx.column() >= 1 && indx.column() < 4 && this->Mode == COLORS)
    {
      switch (role)
      {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
        {
          const auto color = pair.second.value<QColor>();
          const double rgbF[3] = { color.redF(), color.greenF(), color.blueF() };
          return QString::number(rgbF[indx.column() - 1], 'f', 2);
        }
      }
    }
    return QVariant();
  }

  bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
  {
    if (count < 1 || row < 0 || row > this->rowCount(parent))
    {
      return false;
    }
    this->beginInsertRows(parent, row, row + count - 1);
    for (int r = 0; r < count; ++r)
    {
      this->Values.insert(row, {});
    }
    this->endInsertRows();
    return true;
  }

  bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
  {
    if (count <= 0 || row < 0 || (row + count) > this->rowCount(parent))
    {
      return false;
    }

    this->beginRemoveRows(parent, row, row + count - 1);
    for (int r = 0; r < count; ++r)
    {
      this->Values.removeAt(row);
    }
    this->endRemoveRows();
    return true;
  }

private:
  Q_DISABLE_COPY(TableModel);
  ModeT Mode;
  int PixmapSize;
  QList<QPair<QString, QVariant> > Values;
};

//=================================================================================
/**
 * Specialization to ensure the header checkstate/label etc. reflects the root-node.
 * This also handles mapping custom roles such as detail::ColorRole and detail::OpacityRole
 * to columns and rendering appropriate swatches for the same.
 */
class pqDAPModel : public QIdentityProxyModel
{
  int PixmapSize = 16;
  using Superclass = QIdentityProxyModel;

  QString HeaderText; // header text for column 0.
public:
  pqDAPModel(int pixmapSize, QObject* prnt)
    : Superclass(prnt)
    , PixmapSize(pixmapSize)
  {
    QObject::connect(this, &QAbstractItemModel::dataChanged,
      [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&) {
        if (topLeft.row() == 0 && !topLeft.parent().isValid())
        {
          Q_EMIT this->headerDataChanged(Qt::Horizontal, topLeft.column(), topLeft.column());
        }
      });
  }

  void setHeaderText(const QString& txt)
  {
    if (this->HeaderText != txt)
    {
      this->HeaderText = txt;
      this->headerDataChanged(Qt::Horizontal, 0, 0);
    }
  }

  void setSourceModel(QAbstractItemModel* smodel) override
  {
    this->Superclass::setSourceModel(smodel);
    if (smodel)
    {
      // when data with custom roles on the source model change, we need to map
      // it to decorate role on appropriate column to ensure the view updates correctly.
      QObject::connect(smodel, &QAbstractItemModel::dataChanged,
        [&](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
          for (int cc = 0; cc < this->ExtraColumns.size(); ++cc)
          {
            const auto& customRole = this->ExtraColumns[cc];
            if (roles.contains(customRole))
            {
              emit this->dataChanged(topLeft.siblingAtColumn(cc + 1),
                bottomRight.siblingAtColumn(cc + 1), { Qt::DecorationRole });
            }
          }
        });
    }
  }

  QVariant headerData(
    int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
  {
    if (orientation == Qt::Horizontal)
    {
      switch (role)
      {
        case Qt::TextAlignmentRole:
          return QVariant(Qt::AlignLeft | Qt::AlignVCenter);

        case Qt::DisplayRole:
        case Qt::ToolTipRole:
          if (section == 0 && !this->HeaderText.isEmpty())
          {
            return this->HeaderText;
          }
          VTK_FALLTHROUGH;
        default:
          return this->data(this->index(0, section), role);
      }
    }
    return this->Superclass::headerData(section, orientation, role);
  }

  bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
    int role = Qt::EditRole) override
  {
    if (section == 0 && orientation == Qt::Horizontal && this->columnCount() > 0)
    {
      return this->setData(this->index(0, section), value, role);
    }
    return this->Superclass::setHeaderData(section, orientation, value, role);
  }

  int columnCount(const QModelIndex& indx = QModelIndex()) const override
  {
    return this->Superclass::columnCount(indx) + static_cast<int>(this->ExtraColumns.size());
  }

  QVariant data(const QModelIndex& indx, int role) const override
  {
    if (indx.column() == 0 || !indx.isValid())
    {
      return this->Superclass::data(indx, role);
    }

    const int columnRole = this->ExtraColumns.at(indx.column() - 1);
    auto isDerived = [&]() {
      const auto var = this->Superclass::data(
        indx.siblingAtColumn(0), pqDataAssemblyTreeModel::GetIsDerivedRole(columnRole));
      return var.isValid() == false || var.toBool();
    };

    if (columnRole == detail::ColorRole)
    {
      switch (role)
      {
        case Qt::DecorationRole:
        {
          const auto itemdata = this->Superclass::data(indx.siblingAtColumn(0), columnRole);
          return detail::ColorPixmap(this->PixmapSize, itemdata.value<QColor>(), isDerived());
        }

        case Qt::ToolTipRole:
          return isDerived()
            ? QVariant("Double-click to set color.")
            : QVariant("Double-click to change color,\nControl-click to remove color.");
          break;

        case detail::ColorRole:
          return this->Superclass::data(indx.siblingAtColumn(0), detail::ColorRole);
      }
    }
    else if (columnRole == detail::OpacityRole)
    {
      switch (role)
      {
        case Qt::DecorationRole:
        {
          const auto itemdata = this->Superclass::data(indx.siblingAtColumn(0), columnRole);
          return detail::OpacityPixmap(
            this->PixmapSize, itemdata.isValid() ? itemdata.toDouble() : -1.0, isDerived());
        }

        case Qt::ToolTipRole:
          return isDerived()
            ? QVariant("Double-click to set opacity.")
            : QVariant("Double-click to change opacity,\nControl-click to remove opacity.");
          break;

        case detail::OpacityRole:
          return this->Superclass::data(indx.siblingAtColumn(0), detail::OpacityRole);
      }
    }
    return QVariant();
  }

  /**
   * Add a new column for the specified role.
   */
  void addColumn(int role)
  {
    this->beginResetModel();
    this->ExtraColumns.push_back(role);
    this->endResetModel();
  }

  /**
   * Returns the role associated with a column.
   */
  int roleForColumn(int column) const
  {
    column -= 1;
    try
    {
      return this->ExtraColumns.at(column);
    }
    catch (std::out_of_range&)
    {
      return 0;
    }
  }

private:
  Q_DISABLE_COPY(pqDAPModel);
  std::vector<int> ExtraColumns;
};

//=================================================================================
void adjustHeader(QHeaderView* header)
{
  if (header->count() > 1)
  {
    header->moveSection(0, header->count() - 1);
    header->setResizeContentsPrecision(10);
    header->resizeSections(QHeaderView::ResizeToContents);
  }
}

//=================================================================================
void hookupTableView(QTableView* view, TableModel* model, QAbstractButton* addButton,
  QAbstractButton* removeButton, QAbstractButton* removeAllButton)
{
  // hookup add button
  QObject::connect(addButton, &QAbstractButton::clicked, [=](bool) {
    auto currentIndex = view->currentIndex();
    const int row =
      currentIndex.isValid() ? (currentIndex.row() + 1) : model->rowCount(QModelIndex());
    if (model->insertRows(row, 1))
    {
      currentIndex = model->index(row, view->horizontalHeader()->logicalIndex(0));
      view->setCurrentIndex(currentIndex);
      view->edit(currentIndex);
    }
  });

  // enabled-state for remove button.
  QObject::connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
    [=](const QItemSelection&, const QItemSelection&) {
      removeButton->setEnabled(view->selectionModel()->hasSelection());
    });

  // hookup remove button.
  QObject::connect(removeButton, &QAbstractButton::clicked, [=](bool) {
    auto rowIndexes = view->selectionModel()->selectedIndexes();
    std::set<int> rows;
    std::transform(rowIndexes.begin(), rowIndexes.end(), std::inserter(rows, rows.end()),
      [](const QModelIndex& idx) { return idx.row(); });
    for (auto iter = rows.rbegin(); iter != rows.rend(); ++iter)
    {
      model->removeRow(*iter);
    }
  });

  // hookup remove all
  QObject::connect(removeAllButton, &QAbstractButton::clicked,
    [=](bool) { model->setData(QList<QPair<QString, QVariant> >{}); });
}

//=================================================================================
template <typename T1>
QList<QVariant> colorsToVariantList(const QList<QPair<T1, QVariant> >& colors)
{
  QList<QVariant> result;
  for (const auto& pair : colors)
  {
    result.push_back(pair.first);

    const QVariant& variant = pair.second;

    auto color = variant.value<QColor>();
    result.push_back(color.redF());
    result.push_back(color.greenF());
    result.push_back(color.blueF());
  }
  return result;
}

//=================================================================================
template <typename T1>
QList<QVariant> opacitiesToVariantList(const QList<QPair<T1, QVariant> >& opacities)
{
  QList<QVariant> result;
  for (const auto& pair : opacities)
  {
    result.push_back(pair.first);
    result.push_back(pair.second.toDouble());
  }
  return result;
}

//=================================================================================
QList<QVariant> compositeIndicesToVariantList(const std::vector<unsigned int>& cids)
{
  QList<QVariant> result;
  for (const auto& cid : cids)
  {
    result.push_back(cid);
  }
  return result;
}

//=================================================================================
/**
 * A quick way to support use of pqDataAssemblyPropertyWidget for single
 * properties or property groups. When single property, it's treated as a group
 * with just 1 property for function Selectors.
 */
vtkSmartPointer<vtkSMPropertyGroup> createGroup(vtkSMProperty* property)
{
  vtkNew<vtkSMPropertyGroup> group;
  group->AddProperty("Selectors", property);
  group->SetXMLLabel(property->GetXMLLabel());
  return group;
}
}

class pqDataAssemblyPropertyWidget::pqInternals
{
public:
  Ui::DataAssemblyPropertyWidget Ui;
  QPointer<pqDataAssemblyTreeModel> AssemblyTreeModel;
  QPointer<pqDAPModel> ProxyModel;

  //@{
  /**
   * Table models for advanced editing of the selectors + properties.
   * This is not supported in composite-indices mode.
   */
  QPointer<TableModel> SelectorsTableModel;
  QPointer<TableModel> ColorsTableModel;
  QPointer<TableModel> OpacitiesTableModel;
  //@}

  //@{
  // Cached values.
  QStringList Selectors;
  QList<QVariant> Colors;
  QList<QVariant> Opacities;

  QList<QVariant> CompositeIndices;
  QList<QVariant> CompositeIndexColors;
  QList<QVariant> CompositeIndexOpacities;
  //@}

  bool BlockUpdates = false;

  bool InCompositeIndicesMode = false;
  // this is only supported in composite-indices mode.
  bool LeafNodesOnly = false;

  // Indicates if input's proxy name is being shown as header.
  bool UseInputNameAsHeader = false;

  vtkDataAssembly* assembly() const { return this->AssemblyTreeModel->dataAssembly(); }
};

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::pqDataAssemblyPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : pqDataAssemblyPropertyWidget(smproxy, ::createGroup(smproperty), parentObject)
{
}

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::pqDataAssemblyPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqDataAssemblyPropertyWidget::pqInternals())
{
  this->setShowLabel(false);

  // grab hints from the group.
  auto groupHints = smgroup->GetHints()
    ? smgroup->GetHints()->FindNestedElementByName("DataAssemblyPropertyWidget")
    : nullptr;

  int tempValue; // use to make `GetScalarAttribute` calls compact.
  const int iconSize = std::max(this->style()->pixelMetric(QStyle::PM_SmallIconSize), 16);

  auto& internals = (*this->Internals);
  internals.Ui.setupUi(this);
  internals.Ui.hierarchy->header()->setDefaultSectionSize(iconSize + 4);
  internals.Ui.hierarchy->header()->setMinimumSectionSize(iconSize + 4);

  internals.AssemblyTreeModel = new pqDataAssemblyTreeModel(this);

  // Setup proxy model to add check-state support in the header.
  internals.ProxyModel = new pqDAPModel(iconSize, this);
  internals.ProxyModel->setSourceModel(internals.AssemblyTreeModel);
  internals.ProxyModel->setHeaderText(smgroup->GetXMLLabel());

  int useInputNameAsHeader = 0;
  if (groupHints &&
    groupHints->GetScalarAttribute("use_inputname_as_header", &useInputNameAsHeader) &&
    useInputNameAsHeader == 1)
  {
    internals.UseInputNameAsHeader = true;
  }

  // Add a sort-filter model to enable sorting and filtering of item in the
  // tree.
  auto sortmodel = new QSortFilterProxyModel(this);
  sortmodel->setSourceModel(internals.ProxyModel);
  sortmodel->setRecursiveFilteringEnabled(true);
  internals.Ui.hierarchy->setModel(sortmodel);

  internals.SelectorsTableModel = new TableModel(iconSize, TableModel::SELECTORS, this);
  internals.Ui.table->setModel(internals.SelectorsTableModel);

  internals.ColorsTableModel = new TableModel(iconSize, TableModel::COLORS, this);
  internals.Ui.colors->setModel(internals.ColorsTableModel);
  internals.Ui.colors->horizontalHeader()->setDefaultSectionSize(iconSize + 6);
  internals.Ui.colors->horizontalHeader()->setMinimumSectionSize(iconSize + 6);

  internals.OpacitiesTableModel = new TableModel(iconSize, TableModel::OPACITIES, this);
  internals.Ui.opacities->setModel(internals.OpacitiesTableModel);
  internals.Ui.opacities->horizontalHeader()->setDefaultSectionSize(iconSize + 6);
  internals.Ui.opacities->horizontalHeader()->setMinimumSectionSize(iconSize + 6);

  // change pqTreeView header.
  internals.Ui.hierarchy->setupCustomHeader(/*use_pqHeaderView=*/true);
  new pqTreeViewSelectionHelper(internals.Ui.hierarchy);

  hookupTableView(internals.Ui.table, internals.SelectorsTableModel, internals.Ui.add,
    internals.Ui.remove, internals.Ui.removeAll);
  hookupTableView(internals.Ui.colors, internals.ColorsTableModel, internals.Ui.addColors,
    internals.Ui.removeColors, internals.Ui.removeAllColors);
  hookupTableView(internals.Ui.opacities, internals.OpacitiesTableModel, internals.Ui.addOpacities,
    internals.Ui.removeOpacities, internals.Ui.removeAllOpacities);

  // A domain that can provide us the assembly we use to show in this property.
  vtkSMDomain* domain = nullptr;
  QVariant usingCompositeIndices;
  if (auto smproperty = smgroup->GetProperty("Selectors"))
  {
    const bool userCheckable = (groupHints == nullptr ||
      !groupHints->GetScalarAttribute("is_checkable", &tempValue) || tempValue == 1);
    if (!userCheckable)
    {
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));
    }

    internals.AssemblyTreeModel->setUserCheckable(userCheckable);
    if ((domain = smproperty->FindDomain<vtkSMDataAssemblyDomain>()))
    {
      // not in composite-indices mode.
      usingCompositeIndices = false;

      // monitor SelectorsTableModel data changes.
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::dataChanged, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::rowsInserted,
        this, &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::rowsRemoved, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);
      QObject::connect(internals.SelectorsTableModel.data(), &QAbstractItemModel::modelReset, this,
        &pqDataAssemblyPropertyWidget::selectorsTableModified);

      if (userCheckable)
      {
        // link with property
        this->addPropertyLink(this, "selectors", SIGNAL(selectorsChanged()), smproperty);
      }
    }
    else if ((domain = smproperty->FindDomain<vtkSMCompositeTreeDomain>()))
    {
      // in composite-indices mode.
      usingCompositeIndices = true;

      // indicates if we should only returns composite indices for leaf nodes.
      internals.LeafNodesOnly = (vtkSMCompositeTreeDomain::SafeDownCast(domain)->GetMode() ==
        vtkSMCompositeTreeDomain::LEAVES);

      // remove selectors tab, since it's not supported in composite-ids mode.
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));

      if (userCheckable)
      {
        // link with property
        this->addPropertyLink(this, "compositeIndices", SIGNAL(selectorsChanged()), smproperty);
      }
    }
    else
    {
      qWarning()
        << "Missing vtkSMDataAssemblyDomain or vtkSMCompositeTreeDomain domain on property '"
        << smproperty->GetXMLName() << "'. pqDataAssemblyPropertyWidget will not work correctly!";
    }
  }
  else
  {
    // no "Selectors" property i.e. we're not showing checkboxes in our widget.
    internals.AssemblyTreeModel->setUserCheckable(false);
    // remove selectors tab, since it's not applicable..
    internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.selectorsTab));
  }

  if (auto smproperty = smgroup->GetProperty("ActiveAssembly"))
  {
    auto adaptor = new pqSignalAdaptorComboBox(internals.Ui.assemblyCombo);
    new pqComboBoxDomain(internals.Ui.assemblyCombo, smproperty);
    this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), smproperty);
  }
  else
  {
    internals.Ui.assemblyCombo->hide();
  }

  if (auto smproperty = smgroup->GetProperty("Colors"))
  {
    internals.ProxyModel->addColumn(detail::ColorRole);
    internals.AssemblyTreeModel->setRoleProperty(
      detail::ColorRole, pqDataAssemblyTreeModel::InheritedUntilOverridden);

    if (vtkSMDomain* colorsDomain = smproperty->FindDomain<vtkSMCompositeTreeDomain>())
    {
      // compositeIndices mode is exclusive: either all properties in the group are using it
      // or none are.
      Q_ASSERT(!usingCompositeIndices.isValid() || usingCompositeIndices.toBool() == true);

      usingCompositeIndices = true;

      domain = domain ? domain : colorsDomain;

      // remove colors tab, we only show it when not in composite indices mode.
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.colorsTab));

      this->addPropertyLink(this, "compositeIndexColors", SIGNAL(colorsChanged()), smproperty);
    }
    else if ((colorsDomain = smproperty->FindDomain<vtkSMDataAssemblyDomain>()))
    {
      // compositeIndices mode is exclusive: either all properties in the group are using it
      // or none are.
      Q_ASSERT(!usingCompositeIndices.isValid() || usingCompositeIndices.toBool() == false);

      usingCompositeIndices = false;

      domain = domain ? domain : colorsDomain;

      // monitor ColorsTableModel.
      QObject::connect(internals.ColorsTableModel.data(), &QAbstractItemModel::dataChanged, this,
        &pqDataAssemblyPropertyWidget::colorsTableModified);
      QObject::connect(internals.ColorsTableModel.data(), &QAbstractItemModel::rowsInserted, this,
        &pqDataAssemblyPropertyWidget::colorsTableModified);
      QObject::connect(internals.ColorsTableModel.data(), &QAbstractItemModel::rowsRemoved, this,
        &pqDataAssemblyPropertyWidget::colorsTableModified);
      QObject::connect(internals.ColorsTableModel.data(), &QAbstractItemModel::modelReset, this,
        &pqDataAssemblyPropertyWidget::colorsTableModified);

      this->addPropertyLink(this, "selectorColors", SIGNAL(colorsChanged()), smproperty);
    }
    else
    {
      qWarning("Missing suitable domain on property for 'Colors'");
    }
  }
  else
  {
    internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.colorsTab));
  }

  if (auto smproperty = smgroup->GetProperty("Opacities"))
  {
    internals.ProxyModel->addColumn(detail::OpacityRole);
    internals.AssemblyTreeModel->setRoleProperty(
      detail::OpacityRole, pqDataAssemblyTreeModel::InheritedUntilOverridden);

    if (vtkSMDomain* opacitiesDomain = smproperty->FindDomain<vtkSMCompositeTreeDomain>())
    {
      // compositeIndices mode is exclusive: either all properties in the group are using it
      // or none are.
      Q_ASSERT(!usingCompositeIndices.isValid() || usingCompositeIndices.toBool() == true);

      usingCompositeIndices = true;

      domain = domain ? domain : opacitiesDomain;

      // remove opacities tab; we only show it when not in composite indices
      // mode.
      internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.opacitiesTab));
      this->addPropertyLink(
        this, "compositeIndexOpacities", SIGNAL(opacitiesChanged()), smproperty);
    }
    else if ((opacitiesDomain = smproperty->FindDomain<vtkSMDataAssemblyDomain>()))
    {
      // compositeIndices mode is exclusive: either all properties in the group are using it
      // or none are.
      Q_ASSERT(!usingCompositeIndices.isValid() || usingCompositeIndices.toBool() == false);

      usingCompositeIndices = false;

      domain = domain ? domain : opacitiesDomain;

      // monitor OpacitiesTableModel.
      QObject::connect(internals.OpacitiesTableModel.data(), &QAbstractItemModel::dataChanged, this,
        &pqDataAssemblyPropertyWidget::opacitiesTableModified);
      QObject::connect(internals.OpacitiesTableModel.data(), &QAbstractItemModel::rowsInserted,
        this, &pqDataAssemblyPropertyWidget::opacitiesTableModified);
      QObject::connect(internals.OpacitiesTableModel.data(), &QAbstractItemModel::rowsRemoved, this,
        &pqDataAssemblyPropertyWidget::opacitiesTableModified);
      QObject::connect(internals.OpacitiesTableModel.data(), &QAbstractItemModel::modelReset, this,
        &pqDataAssemblyPropertyWidget::opacitiesTableModified);
      this->addPropertyLink(this, "selectorOpacities", SIGNAL(opacitiesChanged()), smproperty);
    }
    else
    {
      qWarning("Missing suitable domain on property for 'Opacities'");
    }
  }
  else
  {
    internals.Ui.tabWidget->removeTab(internals.Ui.tabWidget->indexOf(internals.Ui.opacitiesTab));
  }

  internals.InCompositeIndicesMode =
    (usingCompositeIndices.isValid() && usingCompositeIndices.toBool());

  if (domain)
  {
    // monitor AssemblyTreeModel data changes.
    QObject::connect(internals.AssemblyTreeModel.data(), &pqDataAssemblyTreeModel::modelDataChanged,
      this, &pqDataAssemblyPropertyWidget::assemblyTreeModified);

    // observe the property's domain to update the hierarchy when it changes.
    pqCoreUtilities::connect(
      domain, vtkCommand::DomainModifiedEvent, this, SLOT(updateDataAssembly(vtkObject*)));
    this->updateDataAssembly(domain);
  }
  else
  {
    qWarning("No suitable domain has been found. pqDataAssemblyPropertyWidget will not be "
             "able to show any hierarchy / data assembly. Please check your Proxy XML.");
  }

  // move columns so that all swatches are shown to the right of the tree
  // this is easier to read than the default.
  auto header = internals.Ui.hierarchy->header();
  if (header->count() > 1)
  {
    // hookup double click to edit color and opacity.
    QObject::connect(
      internals.Ui.hierarchy, &QTreeView::doubleClicked, [&](const QModelIndex& idx) {
        auto model = internals.Ui.hierarchy->model();
        switch (internals.ProxyModel->roleForColumn(idx.column()))
        {
          case detail::ColorRole:
          {
            QColor color = model->data(idx, detail::ColorRole).value<QColor>();
            color = QColorDialog::getColor(
              color, this, "Select Color", QColorDialog::DontUseNativeDialog);
            if (color.isValid())
            {
              model->setData(idx, color, detail::ColorRole);
            }
          }
          break;

          case detail::OpacityRole:
          {
            double opacity = model->data(idx, detail::OpacityRole).toDouble();
            pqDoubleRangeDialog dialog("Opacity:", 0.0, 1.0, this);
            dialog.setObjectName("OpacityDialog");
            dialog.setWindowTitle("Select Opacity");
            dialog.setValue(opacity);
            if (dialog.exec() == QDialog::Accepted)
            {
              model->setData(idx, qBound(0.0, dialog.value(), 1.0), detail::OpacityRole);
            }
          }
          break;
        }
      });

    // hookup cntrl+clip to clear color and opacity.
    QObject::connect(internals.Ui.hierarchy, &QTreeView::clicked, [&](const QModelIndex& idx) {
      if (!QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
      {
        return;
      }
      auto model = internals.Ui.hierarchy->model();
      const int role = internals.ProxyModel->roleForColumn(idx.column());
      model->setData(idx.siblingAtColumn(0), QVariant(), role);
    });
  }

  adjustHeader(internals.Ui.hierarchy->header());
  adjustHeader(internals.Ui.colors->horizontalHeader());
  adjustHeader(internals.Ui.opacities->horizontalHeader());
}

//-----------------------------------------------------------------------------
pqDataAssemblyPropertyWidget::~pqDataAssemblyPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::assemblyTreeModified(int role)
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }
  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  auto assembly = internals.assembly();

  if (role == Qt::CheckStateRole)
  {
    internals.Selectors = internals.AssemblyTreeModel->checkedNodes();
    internals.SelectorsTableModel->setData(internals.Selectors);
    internals.CompositeIndices.clear();
    if (assembly != nullptr && internals.InCompositeIndicesMode)
    {
      // compute composite indices.
      std::vector<std::string> selectors(internals.Selectors.size());
      std::transform(internals.Selectors.begin(), internals.Selectors.end(), selectors.begin(),
        [](const QString& str) { return str.toStdString(); });
      internals.CompositeIndices =
        compositeIndicesToVariantList(vtkDataAssemblyUtilities::GetSelectedCompositeIds(
          selectors, assembly, nullptr, internals.LeafNodesOnly));
    }
    Q_EMIT this->selectorsChanged();
  }
  else if (role == detail::ColorRole)
  {
    const auto colors = internals.AssemblyTreeModel->data(detail::ColorRole);
    internals.ColorsTableModel->setData(colors);
    internals.Colors = colorsToVariantList(colors);
    internals.CompositeIndexColors.clear();
    if (assembly != nullptr && internals.InCompositeIndicesMode)
    {
      // convert selector:colors map to composite-index:color map.
      QList<QPair<unsigned int, QVariant> > idColors;
      for (const auto& pair : colors)
      {
        auto ids =
          vtkDataAssemblyUtilities::GetSelectedCompositeIds({ pair.first.toStdString() }, assembly);
        Q_ASSERT(ids.size() == 1);
        idColors.push_back(qMakePair(ids.front(), pair.second));
      }
      internals.CompositeIndexColors = colorsToVariantList(idColors);
    }

    Q_EMIT this->colorsChanged();
  }
  else if (role == detail::OpacityRole)
  {
    const auto opacities = internals.AssemblyTreeModel->data(detail::OpacityRole);
    internals.OpacitiesTableModel->setData(opacities);
    internals.Opacities = opacitiesToVariantList(opacities);
    internals.CompositeIndexOpacities.clear();
    if (assembly != nullptr && internals.InCompositeIndicesMode)
    {
      // convert selector:opacities map to composite-index:opacities map.
      QList<QPair<unsigned int, QVariant> > idOpacities;
      for (const auto& pair : opacities)
      {
        auto ids =
          vtkDataAssemblyUtilities::GetSelectedCompositeIds({ pair.first.toStdString() }, assembly);
        Q_ASSERT(ids.size() == 1);
        idOpacities.push_back(qMakePair(ids.front(), pair.second));
      }
      internals.CompositeIndexOpacities = opacitiesToVariantList(idOpacities);
    }

    Q_EMIT this->opacitiesChanged();
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::selectorsTableModified()
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.Selectors = internals.SelectorsTableModel->selectors();
  internals.AssemblyTreeModel->setCheckedNodes(internals.Selectors);
  Q_EMIT this->selectorsChanged();
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::colorsTableModified()
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  const auto& colors = internals.ColorsTableModel->values();
  internals.Colors = colorsToVariantList(colors);
  internals.AssemblyTreeModel->setData(colors, detail::ColorRole);
  Q_EMIT this->colorsChanged();
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::opacitiesTableModified()
{
  auto& internals = (*this->Internals);
  if (internals.BlockUpdates)
  {
    return;
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  const auto& opacities = internals.OpacitiesTableModel->values();
  internals.Opacities = opacitiesToVariantList(opacities);
  internals.AssemblyTreeModel->setData(opacities, detail::OpacityRole);
  Q_EMIT this->opacitiesChanged();
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::updateDataAssembly(vtkObject* sender)
{
  vtkDataAssembly* assembly = nullptr;
  const char* name = "(?)";
  if (auto domain = vtkSMDataAssemblyDomain::SafeDownCast(sender))
  {
    assembly = domain->GetDataAssembly();
    name = domain->GetDataAssemblyName();
  }
  else if (auto cdomain = vtkSMCompositeTreeDomain::SafeDownCast(sender))
  {
    assembly = cdomain->GetHierarchy();
    name = "Hierarchy";
  }

  auto& internals = (*this->Internals);
  pqTreeViewExpandState helper;
  helper.save(internals.Ui.hierarchy);
  internals.AssemblyTreeModel->setDataAssembly(assembly);
  internals.AssemblyTreeModel->setCheckedNodes(internals.Selectors);
  internals.Ui.tabWidget->setTabText(0, name);
  // internals.Ui.hierarchy->setRootIndex(internals.Ui.hierarchy->model()->index(0, 0));
  internals.Ui.hierarchy->expandToDepth(2);
  helper.restore(internals.Ui.hierarchy);

  if (internals.InCompositeIndicesMode)
  {
    this->setCompositeIndices(internals.CompositeIndices);
    this->setCompositeIndexColors(internals.CompositeIndexColors);
    this->setCompositeIndexOpacities(internals.CompositeIndexOpacities);
  }
  else
  {
    this->setSelectors(internals.Selectors);
    this->setSelectorColors(internals.Colors);
    this->setSelectorOpacities(internals.Opacities);
  }

  auto domain = vtkSMDomain::SafeDownCast(sender);
  if (internals.UseInputNameAsHeader && domain)
  {
    vtkSMPropertyHelper inputHelper(domain->GetRequiredProperty("Input"));
    auto proxy = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(0));
    auto port = inputHelper.GetOutputPort(0);
    if (proxy && proxy)
    {
      auto smmodel = pqApplicationCore::instance()->getServerManagerModel();
      auto pqport = smmodel->findItem<pqOutputPort*>(proxy->GetOutputPort(port));
      if (pqport)
      {
        internals.ProxyModel->setHeaderText(pqport->prettyName());
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectors(const QStringList& paths)
{
  auto& internals = (*this->Internals);
  internals.Selectors = paths;

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setCheckedNodes(paths);
  internals.SelectorsTableModel->setData(paths);
  Q_EMIT this->selectorsChanged();
}

//-----------------------------------------------------------------------------
const QStringList& pqDataAssemblyPropertyWidget::selectors() const
{
  auto& internals = (*this->Internals);
  return internals.Selectors;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectors(const QList<QVariant>& paths)
{
  QStringList string_paths;
  std::transform(paths.begin(), paths.end(), std::back_inserter(string_paths),
    [](const QVariant& value) { return value.toString(); });
  this->setSelectors(string_paths);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::selectorsAsVariantList() const
{
  QList<QVariant> variant_paths;
  const auto& string_paths = this->selectors();
  std::transform(string_paths.begin(), string_paths.end(), std::back_inserter(variant_paths),
    [](const QString& value) { return QVariant(value); });
  return variant_paths;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setCompositeIndices(const QList<QVariant>& values)
{
  auto& internals = (*this->Internals);
  internals.CompositeIndices = values;

  QList<QVariant> cidSelectors;
  std::transform(values.begin(), values.end(), std::back_inserter(cidSelectors),
    [](const QVariant& var) { return QString("//*[@cid=%1]").arg(var.value<unsigned int>()); });
  this->setSelectors(cidSelectors);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::compositeIndicesAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.CompositeIndices;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setCompositeIndexColors(const QList<QVariant>& values)
{
  auto& internals = (*this->Internals);
  internals.CompositeIndexColors = values;

  QList<QVariant> cidColorSelectors;
  for (int cc = 0; (cc + 3) < values.size(); cc += 4)
  {
    cidColorSelectors.push_back(QString("//*[@cid=%1]").arg(values[cc].value<unsigned int>()));
    cidColorSelectors.push_back(values[cc + 1]);
    cidColorSelectors.push_back(values[cc + 2]);
    cidColorSelectors.push_back(values[cc + 3]);
  }
  this->setSelectorColors(cidColorSelectors);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::compositeIndexColorsAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.CompositeIndexColors;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectorColors(const QList<QVariant>& values)
{
  auto& internals = (*this->Internals);
  internals.Colors = values;

  QList<QPair<QString, QVariant> > colors;
  for (int cc = 0, max = values.size(); (cc + 3) < max; cc += 4)
  {
    const auto color = QColor::fromRgbF(
      values[cc + 1].toDouble(), values[cc + 2].toDouble(), values[cc + 3].toDouble());
    colors.push_back(qMakePair(values[cc].toString(), color));
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setData(colors, detail::ColorRole);
  internals.ColorsTableModel->setData(colors);
  Q_EMIT this->colorsChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::selectorColorsAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.Colors;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setCompositeIndexOpacities(const QList<QVariant>& values)
{
  auto& internals = (*this->Internals);
  internals.CompositeIndexOpacities = values;

  QList<QVariant> cidOpacitySelectors;
  for (int cc = 0; (cc + 1) < values.size(); cc += 2)
  {
    cidOpacitySelectors.push_back(QString("//*[@cid=%1]").arg(values[cc].value<unsigned int>()));
    cidOpacitySelectors.push_back(values[cc + 1]);
  }
  this->setSelectorOpacities(cidOpacitySelectors);
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::compositeIndexOpacitiesAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.CompositeIndexOpacities;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::setSelectorOpacities(const QList<QVariant>& values)
{
  auto& internals = (*this->Internals);
  internals.Opacities = values;

  QList<QPair<QString, QVariant> > opacities;
  for (int cc = 0, max = values.size(); (cc + 1) < max; cc += 2)
  {
    opacities.push_back(qMakePair(values[cc].toString(), values[cc + 1].toDouble()));
  }

  QScopedValueRollback<bool> rollback(internals.BlockUpdates, true);
  internals.AssemblyTreeModel->setData(opacities, detail::OpacityRole);
  internals.OpacitiesTableModel->setData(opacities);
  Q_EMIT this->opacitiesChanged();
}

//-----------------------------------------------------------------------------
QList<QVariant> pqDataAssemblyPropertyWidget::selectorOpacitiesAsVariantList() const
{
  const auto& internals = (*this->Internals);
  return internals.Opacities;
}

//-----------------------------------------------------------------------------
void pqDataAssemblyPropertyWidget::updateWidget(bool showing_advanced_properties)
{
  // we want to show the extra tabs for editing color/opacity/selectors manually
  // only in advanced mode.
  auto& internals = (*this->Internals);
  if (showing_advanced_properties && internals.Ui.tabWidget->count() > 1)
  {
    internals.Ui.tabWidget->tabBar()->show();
  }
  else
  {
    internals.Ui.tabWidget->setCurrentIndex(0);
    internals.Ui.tabWidget->tabBar()->hide();
  }
}
