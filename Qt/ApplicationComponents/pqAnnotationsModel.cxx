/*=========================================================================

   Program: ParaView
   Module:  pqAnnotationsModel.cxx

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
#include "pqAnnotationsModel.h"

#include <QMimeData>
#include <QPainter>
#include <QPixmap>

#include "vtkSMStringListDomain.h"

#include <cassert>
#include <set>

#include <cassert>

static const int SWATCH_RADIUS = 17;
namespace
{
// Create a disk filled with the given color
QPixmap createSwatch(QColor& color)
{
  QPixmap pix(SWATCH_RADIUS, SWATCH_RADIUS);
  pix.fill(QColor(0, 0, 0, 0));

  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setBrush(QBrush(color));
  painter.drawEllipse(1, 1, SWATCH_RADIUS - 2, SWATCH_RADIUS - 2);
  painter.end();
  return pix;
}

// Create a truncated disk. Remaining angle is given by opacity (1 is full disk, 0 is no disk).
QPixmap createOpacitySwatch(double opacity)
{
  QPixmap pix(SWATCH_RADIUS, SWATCH_RADIUS);
  pix.fill(QColor(0, 0, 0, 0));

  QPainter painter(&pix);
  painter.setRenderHint(QPainter::Antialiasing, true);
  const int delta = 3 * SWATCH_RADIUS / 4;
  QRect rect(0, 0, delta, delta);
  rect.moveCenter(QPoint(SWATCH_RADIUS / 2, SWATCH_RADIUS / 2));
  // angle is given in 1/16 th of a degree, so a whole is 16 * 360 = 5760.
  painter.drawPie(rect, 0, 5760 * opacity);
  painter.end();
  return pix;
}
}

// Handle the content of a row.
class AnnotationItem
{
public:
  QColor Color;
  double Opacity;
  QPixmap Swatch;
  QPixmap OpacitySwatch;
  QString Value;
  QString Annotation;
  Qt::CheckState Visibility;

  AnnotationItem()
    : Opacity(-1)
    , Value("")
    , Annotation("")
    , Visibility(Qt::Unchecked)
  {
  }

  /**
   * Set the underlying data for the given column.
   */
  bool setData(int index, const QVariant& value)
  {
    switch (index)
    {
      case pqAnnotationsModel::COLOR:
        if (value.canConvert(QVariant::Color))
        {
          if (this->Color != value.value<QColor>())
          {
            this->Color = value.value<QColor>();
            this->Swatch = createSwatch(this->Color);
            return true;
          }
        }
        break;
      case pqAnnotationsModel::OPACITY:
      {
        if (this->Opacity != value.toDouble())
        {
          this->Opacity = value.toDouble();
          this->OpacitySwatch = createOpacitySwatch(this->Opacity);
          return true;
        }
      }
      break;
      case pqAnnotationsModel::VALUE:
      {
        if (this->Value != value.toString())
        {
          this->Value = value.toString();
          return true;
        }
      }
      break;
      case pqAnnotationsModel::LABEL:
      {
        if (this->Annotation != value.toString())
        {
          this->Annotation = value.toString();
          return true;
        }
      }
      break;
      case pqAnnotationsModel::VISIBILITY:
      {
        if (this->Visibility != Qt::CheckState(value.toInt()))
        {
          this->Visibility = Qt::CheckState(value.toInt());
          return true;
        }
      }
      break;
      default:
        break;
    }
    return false;
  }

  /**
   * Get the underlying data to display for the given column.
   */
  QVariant data(int index) const
  {
    switch (index)
    {
      case pqAnnotationsModel::COLOR:
        if (this->Color.isValid())
        {
          return this->Swatch;
        }
        break;
      case pqAnnotationsModel::OPACITY:
      {
        return this->OpacitySwatch;
      }
      break;
      case pqAnnotationsModel::VALUE:
      {
        return this->Value;
      }
      break;
      case pqAnnotationsModel::LABEL:
      {
        return this->Annotation;
      }
      break;
      case pqAnnotationsModel::COLOR_DATA:
        if (this->Color.isValid())
        {
          return this->Color;
        }
        break;
      case pqAnnotationsModel::OPACITY_DATA:
      {
        return this->Opacity;
      }
      break;
      case pqAnnotationsModel::VISIBILITY:
      //  handled by CheckStateRole, nothing to return in DisplayRole.
      default:
        break;
    }
    return QVariant();
  }
};

//=============================================================================
/**
 * Handle a list of items
 */
class pqAnnotationsModel::pqInternals
{
public:
  std::vector<AnnotationItem> Items;
  std::vector<QColor> Colors;
};

//=============================================================================
pqAnnotationsModel::pqAnnotationsModel(QObject* parentObject)
  : Superclass(parentObject)
  , MissingColorIcon(":/pqWidgets/Icons/pqUnknownData16.png")
  , GlobalOpacity(1.0)
  , SupportsReorder(false)
  , Internals(new pqInternals())
{
}

//-----------------------------------------------------------------------------
pqAnnotationsModel::~pqAnnotationsModel()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
Qt::ItemFlags pqAnnotationsModel::flags(const QModelIndex& idx) const
{
  auto value = this->Superclass::flags(idx);
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
      case VALUE:
      case LABEL:
        return value | Qt::ItemIsEditable;
      default:
        break;
    }
  }

  return value;
}

//-----------------------------------------------------------------------------
int pqAnnotationsModel::rowCount(const QModelIndex& prnt) const
{
  Q_UNUSED(prnt);
  return static_cast<int>(this->Internals->Items.size());
}

//-----------------------------------------------------------------------------
int pqAnnotationsModel::columnCount(const QModelIndex& prnt) const
{
  Q_UNUSED(prnt);
  return this->columnCount();
}

//-----------------------------------------------------------------------------
bool pqAnnotationsModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if (!idx.isValid())
    return false;
  Q_UNUSED(role);
  assert(idx.row() < this->rowCount());
  assert(idx.column() >= 0 && idx.column() < this->columnCount(idx));

  if (this->Internals->Items[idx.row()].setData(idx.column(), value))
  {
    Q_EMIT this->dataChanged(idx, idx);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
QVariant pqAnnotationsModel::data(const QModelIndex& idx, int role) const
{
  if (role == Qt::DecorationRole || role == Qt::DisplayRole)
  {
    return this->Internals->Items[idx.row()].data(idx.column());
  }
  else if (role == Qt::EditRole)
  {
    int col = idx.column();
    if (col == COLOR)
    {
      col = COLOR_DATA;
    }
    else if (col == OPACITY)
    {
      col = OPACITY_DATA;
    }
    return this->Internals->Items[idx.row()].data(col);
  }
  else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
  {
    switch (idx.column())
    {
      case COLOR:
        return tr("Color");
      case OPACITY:
        return tr("Opacity");
      case VALUE:
      case LABEL:
        return this->Internals->Items[idx.row()].data(idx.column());
      default:
        return QVariant();
    }
  }
  else if (role == Qt::UserRole && idx.column() == VISIBILITY)
  {
    // hide only if domain says so.
    bool res = true;
    if (this->VisibilityDomain)
    {
      auto value = this->Internals->Items[idx.row()].Value;
      unsigned int unused = 0;
      res = this->VisibilityDomain->IsInDomain(value.toLocal8Bit().data(), unused) != 0;
    }

    return res ? true : false;
  }
  else if (role == Qt::CheckStateRole && idx.column() == VISIBILITY)
  {
    return this->Internals->Items[idx.row()].Visibility;
  }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqAnnotationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case VISIBILITY:
        return "";
      case COLOR:
        return "";
      case OPACITY:
        return "";
      case VALUE:
        return tr("Value");
      case LABEL:
        return tr("Annotation");
    }
  }
  else if (orientation == Qt::Horizontal && role == Qt::DecorationRole && section == OPACITY)
  {
    return createOpacitySwatch(this->GlobalOpacity);
  }
  else if (orientation == Qt::Horizontal && role == Qt::CheckStateRole && section == VISIBILITY)
  {
    if (this->Internals->Items.size() == 0)
    {
      return Qt::Unchecked;
    }
    Qt::CheckState ret = this->Internals->Items[0].Visibility;
    for (const auto item : this->Internals->Items)
    {
      if (item.Visibility != ret)
      {
        ret = Qt::PartiallyChecked;
        break;
      }
    }
    return ret;
  }

  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
bool pqAnnotationsModel::setHeaderData(
  int section, Qt::Orientation orientation, const QVariant& value, int role)
{
  if (orientation == Qt::Horizontal && role == Qt::CheckStateRole && section == VISIBILITY)
  {
    for (int row = 0; row < this->rowCount(); row++)
    {
      this->setData(this->index(row, VISIBILITY), value, role);
    }

    return true;
  }

  return this->Superclass::setHeaderData(section, orientation, value, role);
}

//--------- Drag-N-Drop support when enabled --------
Qt::DropActions pqAnnotationsModel::supportedDropActions() const
{
  return this->SupportsReorder ? (Qt::CopyAction | Qt::MoveAction)
                               : this->Superclass::supportedDropActions();
}

//-----------------------------------------------------------------------------
QStringList pqAnnotationsModel::mimeTypes() const
{
  if (this->SupportsReorder)
  {
    QStringList types;
    types << "application/paraview.series.list";
    return types;
  }

  return this->Superclass::mimeTypes();
}

//-----------------------------------------------------------------------------
QMimeData* pqAnnotationsModel::mimeData(const QModelIndexList& indexes) const
{
  if (!this->SupportsReorder)
  {
    return this->Superclass::mimeData(indexes);
  }
  QMimeData* mime_data = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  QList<int> keys;
  for (const QModelIndex& idx : indexes)
  {
    if (idx.isValid() && !keys.contains(idx.row()))
    {
      keys << idx.row();
      stream << idx.row();
    }
  }

  mime_data->setData("application/paraview.series.list", encodedData);
  return mime_data;
}

//-----------------------------------------------------------------------------
bool pqAnnotationsModel::dropMimeData(const QMimeData* mime_data, Qt::DropAction action, int row,
  int column, const QModelIndex& parentIdx)
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
  QList<int> movingItems;
  while (!stream.atEnd())
  {
    int value;
    stream >> value;
    movingItems << value;
  }

  // now re-order the item list.
  std::vector<AnnotationItem> newItems;

  // create list without the moving items
  int realBeginRow = -1;
  for (int cc = 0; cc < static_cast<int>(this->Internals->Items.size()); cc++)
  {
    if (cc == beginRow)
    {
      realBeginRow = static_cast<int>(newItems.size());
    }
    if (!movingItems.contains(cc))
    {
      newItems.push_back(this->Internals->Items[cc]);
    }
  }
  if (realBeginRow == -1)
  {
    realBeginRow = static_cast<int>(newItems.size());
  }

  // insert moving items
  newItems.reserve(this->Internals->Items.size());
  auto beginIt = newItems.begin() + realBeginRow;
  for (double item : movingItems)
  {
    newItems.insert(beginIt, this->Internals->Items[item]);
    beginIt++;
  }

  // set new list
  this->beginResetModel();
  this->Internals->Items = newItems;

  // reset color cache
  this->Internals->Colors.clear();
  for (const AnnotationItem& item : this->Internals->Items)
  {
    this->Internals->Colors.push_back(item.Color);
  }
  this->endResetModel();

  Q_EMIT this->dataChanged(
    this->index(0, 0), this->index(this->rowCount() - 1, this->columnCount(parentIdx) - 1));
  return true;
}
//-----------------------------------------------------------------------------
QModelIndex pqAnnotationsModel::addAnnotation(const QModelIndex& after)
{
  int row = after.isValid() ? after.row() : (this->rowCount(QModelIndex()) - 1);
  // insert after the current one.
  row++;

  Q_EMIT this->beginInsertRows(QModelIndex(), row, row);
  auto it = this->Internals->Items.begin();
  this->Internals->Items.insert(it + row, AnnotationItem());
  Q_EMIT this->endInsertRows();
  if (this->hasColors())
  {
    this->Internals->Items[row].setData(
      COLOR, this->Internals->Colors[row % this->Internals->Colors.size()]);
  }
  this->Internals->Items[row].setData(OPACITY, this->globalOpacity());
  return this->index(row, 0);
}

//-----------------------------------------------------------------------------
QModelIndex pqAnnotationsModel::removeAnnotations(const QModelIndexList& toRemove)
{
  std::set<int> rowsToRemove;
  for (const QModelIndex& idx : toRemove)
  {
    rowsToRemove.insert(idx.row());
  }

  auto startItemIter = this->Internals->Items.begin();
  for (auto riter = rowsToRemove.rbegin(); riter != rowsToRemove.rend(); ++riter)
  {
    Q_EMIT this->beginRemoveRows(QModelIndex(), *riter, *riter);
    this->Internals->Items.erase(startItemIter + *riter);
    Q_EMIT this->endRemoveRows();
  }

  if (rowsToRemove.size() > 0 &&
    *rowsToRemove.begin() > static_cast<int>(this->Internals->Items.size()))
  {
    return this->index(*rowsToRemove.begin(), 0);
  }

  if (this->Internals->Items.size() > 0)
  {
    return this->index(static_cast<int>(this->Internals->Items.size()) - 1, 0);
  }
  return QModelIndex();
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::removeAllAnnotations()
{
  Q_EMIT this->beginResetModel();
  this->Internals->Items.clear();
  Q_EMIT this->endResetModel();
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setAnnotations(
  const std::vector<std::pair<QString, QString> >& newAnnotations)
{
  if (newAnnotations.size() == 0)
  {
    this->removeAllAnnotations();
  }
  else
  {
    int size = static_cast<int>(this->Internals->Items.size());
    int annotationSize = static_cast<int>(newAnnotations.size());
    if (annotationSize < size)
    {
      this->removeAllAnnotations();
      size = 0;
    }
    if (annotationSize > size)
    {
      // rows are added.
      Q_EMIT this->beginInsertRows(QModelIndex(), size, annotationSize - 1);
      this->Internals->Items.resize(annotationSize);
      Q_EMIT this->endInsertRows();
    }

    // now check for data changes.
    bool valueFlag = false;
    bool annotationFlag = false;
    bool colorFlag = false;
    bool opacityFlag = false;
    for (int cc = 0; cc < annotationSize; cc++)
    {
      if (this->Internals->Items[cc].Value != newAnnotations[cc].first)
      {
        this->Internals->Items[cc].Value = newAnnotations[cc].first;
        valueFlag = true;
      }
      if (this->Internals->Items[cc].Annotation != newAnnotations[cc].second)
      {
        this->Internals->Items[cc].Annotation = newAnnotations[cc].second;
        annotationFlag = true;
      }

      if (this->hasColors())
      {
        if (!this->Internals->Items[cc].Color.isValid())
        {
          // Copy color, using modulo if annotation are bigger than current number of colors
          // and color is not yet defined for this item
          this->Internals->Items[cc].setData(
            COLOR, this->Internals->Colors[cc % this->Internals->Colors.size()]);
          colorFlag = true;
        }
        // Initialize Opacities if not defined
        if (this->Internals->Items[cc].Opacity == -1)
        {
          this->Internals->Items[cc].setData(OPACITY, 1.0);
          opacityFlag = true;
        }
      }
    }
    if (colorFlag)
    {
      Q_EMIT this->dataChanged(this->index(0, COLOR),
        this->index(static_cast<int>(this->Internals->Items.size()) - 1, COLOR));
    }
    if (opacityFlag)
    {
      Q_EMIT this->dataChanged(this->index(0, OPACITY),
        this->index(static_cast<int>(this->Internals->Items.size()) - 1, OPACITY));
    }
    if (valueFlag)
    {
      Q_EMIT this->dataChanged(this->index(0, VALUE),
        this->index(static_cast<int>(this->Internals->Items.size()) - 1, VALUE));
    }
    if (annotationFlag)
    {
      Q_EMIT this->dataChanged(this->index(0, LABEL),
        this->index(static_cast<int>(this->Internals->Items.size()) - 1, LABEL));
    }
  }
}

//-----------------------------------------------------------------------------
std::vector<std::pair<QString, QString> > pqAnnotationsModel::annotations() const
{
  std::vector<std::pair<QString, QString> > strAnnotations;
  strAnnotations.reserve(this->Internals->Items.size());
  for (const AnnotationItem& item : this->Internals->Items)
  {
    strAnnotations.push_back(std::make_pair(item.Value, item.Annotation));
  }
  return strAnnotations;
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setVisibilities(
  const std::vector<std::pair<QString, int> >& newVisibilities)
{
  bool visibilityFlag = false;

  for (auto vis : newVisibilities)
  {
    auto name = vis.first;
    auto foundItem = std::find_if(this->Internals->Items.begin(), this->Internals->Items.end(),
      [name](const AnnotationItem& item) { return name == item.Value; });
    if (foundItem == this->Internals->Items.end())
    {
      this->beginResetModel();
      foundItem = this->Internals->Items.emplace(this->Internals->Items.end());
      foundItem->setData(VALUE, name);
      this->endResetModel();
    }
    if (foundItem->setData(VISIBILITY, vis.second))
    {
      visibilityFlag = true;
    }
  }

  if (visibilityFlag)
  {
    Q_EMIT this->dataChanged(this->index(0, VISIBILITY),
      this->index(static_cast<int>(this->Internals->Items.size()) - 1, VISIBILITY));
  }
}

//-----------------------------------------------------------------------------
std::vector<std::pair<QString, int> > pqAnnotationsModel::visibilities() const
{
  std::vector<std::pair<QString, int> > visibilities;
  for (const AnnotationItem& item : this->Internals->Items)
  {
    visibilities.push_back(std::make_pair(item.Value, item.Visibility));
  }

  return visibilities;
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setIndexedColors(const std::vector<QColor>& newColors)
{
  this->Internals->Colors = newColors;
  auto colorSize = newColors.size();

  // now check for data changes.
  if (colorSize > 0)
  {
    bool colorFlag = false;
    for (std::size_t cc = 0; cc < this->Internals->Items.size(); cc++)
    {
      if (!this->Internals->Items[cc].Color.isValid() ||
        this->Internals->Items[cc].Color != newColors[cc % colorSize])
      {
        // Add color with a modulo so all values have colors
        this->Internals->Items[cc].setData(COLOR, newColors[cc % colorSize]);
        colorFlag = true;
      }
    }
    if (colorFlag)
    {
      Q_EMIT this->dataChanged(this->index(0, COLOR),
        this->index(static_cast<int>(this->Internals->Items.size()) - 1, COLOR));
    }
  }
}

//-----------------------------------------------------------------------------
std::vector<QColor> pqAnnotationsModel::indexedColors() const
{
  std::vector<QColor> icolors;
  icolors.reserve(this->Internals->Items.size());
  for (const AnnotationItem& item : this->Internals->Items)
  {
    icolors.push_back(item.Color);
  }
  return icolors;
}

//-----------------------------------------------------------------------------
bool pqAnnotationsModel::hasColors() const
{
  return this->Internals->Colors.size() != 0;
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setIndexedOpacities(const std::vector<double>& newOpacities)
{
  bool opacityFlag = false;
  for (std::size_t cc = 0; cc < this->Internals->Items.size() && cc < newOpacities.size(); cc++)
  {
    if (this->Internals->Items[cc].Opacity == -1 ||
      this->Internals->Items[cc].Opacity != newOpacities[cc])
    {
      this->Internals->Items[cc].setData(OPACITY, newOpacities[cc]);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    Q_EMIT this->dataChanged(this->index(0, OPACITY),
      this->index(static_cast<int>(this->Internals->Items.size()) - 1, OPACITY));
  }
}

//-----------------------------------------------------------------------------
std::vector<double> pqAnnotationsModel::indexedOpacities() const
{
  std::vector<double> opacities;
  opacities.reserve(this->Internals->Items.size());
  for (const AnnotationItem& item : this->Internals->Items)
  {
    opacities.push_back(item.Opacity);
  }
  return opacities;
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setGlobalOpacity(double opacity)
{
  this->GlobalOpacity = opacity;
  bool opacityFlag = false;
  for (std::size_t cc = 0; cc < this->Internals->Items.size(); cc++)
  {
    if (this->Internals->Items[cc].Opacity == -1 || this->Internals->Items[cc].Opacity != opacity)
    {
      this->Internals->Items[cc].setData(OPACITY, opacity);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    Q_EMIT this->dataChanged(this->index(0, OPACITY),
      this->index(static_cast<int>(this->Internals->Items.size()) - 1, OPACITY));
  }
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setSelectedOpacity(QList<int> rows, double opacity)
{
  bool opacityFlag = false;
  for (int cc : rows)
  {
    if (this->Internals->Items[cc].Opacity == -1 || this->Internals->Items[cc].Opacity != opacity)
    {
      this->Internals->Items[cc].setData(OPACITY, opacity);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    Q_EMIT this->dataChanged(this->index(0, OPACITY),
      this->index(static_cast<int>(this->Internals->Items.size()) - 1, OPACITY));
  }
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setSupportsReorder(bool reorder)
{
  this->SupportsReorder = reorder;
}

//-----------------------------------------------------------------------------
bool pqAnnotationsModel::supportsReorder() const
{
  return this->SupportsReorder;
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::reorder(std::vector<int> oldOrder)
{
  if (oldOrder.size() != this->Internals->Items.size())
  {
    return;
  }

  std::vector<AnnotationItem> newItems;
  for (auto previousIndex : oldOrder)
  {
    if (previousIndex >= 0 && previousIndex < static_cast<int>(this->Internals->Items.size()))
    {
      newItems.push_back(this->Internals->Items[previousIndex]);
    }
  }

  assert(oldOrder.size() == this->Internals->Items.size());

  this->beginResetModel();
  this->Internals->Items = newItems;
  this->endResetModel();
  this->setIndexedColors(this->Internals->Colors);
  Q_EMIT this->dataChanged(
    this->index(0, 0), this->index(this->rowCount() - 1, this->columnCount() - 1));
}

//-----------------------------------------------------------------------------
void pqAnnotationsModel::setVisibilityDomain(vtkSMStringListDomain* domain)
{
  this->VisibilityDomain = domain;
}
