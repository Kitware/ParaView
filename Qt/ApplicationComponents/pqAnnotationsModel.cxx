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

#include <QPainter>
#include <QPixmap>

#include <cassert>
#include <set>

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

  AnnotationItem()
    : Opacity(-1)
    , Value("")
    , Annotation("")
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
        {
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
  QVector<AnnotationItem> Items;
  QVector<QColor> Colors;
};

pqAnnotationsModel::pqAnnotationsModel(QObject* parentObject)
  : Superclass(parentObject)
  , MissingColorIcon(":/pqWidgets/Icons/pqUnknownData16.png")
  , GlobalOpacity(1.0)
  , Internals(new pqInternals())
{
}

//-----------------------------------------------------------------------------
pqAnnotationsModel::~pqAnnotationsModel()
{
  delete this->Internals;
  this->Internals = nullptr;
}

/// Columns 2,3 are editable. 0,1 are not (since we show swatches). We
/// hookup double-click event on the view to allow the user to edit the color.
Qt::ItemFlags pqAnnotationsModel::flags(const QModelIndex& idx) const
{
  return idx.column() > 1 ? this->Superclass::flags(idx) | Qt::ItemIsEditable
                          : this->Superclass::flags(idx);
}

int pqAnnotationsModel::rowCount(const QModelIndex& prnt) const
{
  Q_UNUSED(prnt);
  return this->Internals->Items.size();
}

int pqAnnotationsModel::columnCount(const QModelIndex& /*parent*/) const
{
  return 4;
}

bool pqAnnotationsModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  Q_UNUSED(role);
  assert(idx.row() < this->rowCount());
  assert(idx.column() >= 0 && idx.column() < this->columnCount(idx));

  if (this->Internals->Items[idx.row()].setData(idx.column(), value))
  {
    emit this->dataChanged(idx, idx);
    return true;
  }
  return false;
}

QVariant pqAnnotationsModel::data(const QModelIndex& idx, int role) const
{
  if (role == Qt::DecorationRole || role == Qt::DisplayRole)
  {
    return this->Internals->Items[idx.row()].data(idx.column());
  }
  else if (role == Qt::EditRole)
  {
    int col = idx.column();
    if (col == 0 || col == 1)
    {
      col += 4;
    }
    return this->Internals->Items[idx.row()].data(col);
  }
  else if (role == Qt::ToolTipRole || role == Qt::StatusTipRole)
  {
    switch (idx.column())
    {
      case 0:
        return "Color";
      case 1:
        return "Opacity";
      case 2:
        return "Data Value";
      case 3:
        return "Annotation Text";
    }
  }
  return QVariant();
}

QVariant pqAnnotationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "";
      case 1:
        return "";
      case 2:
        return "Value";
      case 3:
        return "Annotation";
    }
  }
  if (orientation == Qt::Horizontal && role == Qt::DecorationRole && section == 1)
  {
    return createOpacitySwatch(this->GlobalOpacity);
  }

  return this->Superclass::headerData(section, orientation, role);
}

// Add a new annotation text-pair after the given index. Returns the inserted
// index.
QModelIndex pqAnnotationsModel::addAnnotation(const QModelIndex& after)
{
  int row = after.isValid() ? after.row() : (this->rowCount(QModelIndex()) - 1);
  // insert after the current one.
  row++;

  emit this->beginInsertRows(QModelIndex(), row, row);
  auto it = this->Internals->Items.begin();
  this->Internals->Items.insert(it + row, AnnotationItem());
  emit this->endInsertRows();
  if (this->Internals->Colors.size() > 0)
  {
    this->Internals->Items[row].setData(
      0, this->Internals->Colors[row % this->Internals->Colors.size()]);
  }
  this->Internals->Items[row].setData(1, this->globalOpacity());
  return this->index(row, 0);
}

// Remove the given annotation indexes. Returns item before or after the removed
// item, if any.
QModelIndex pqAnnotationsModel::removeAnnotations(const QModelIndexList& toRemove)
{
  std::set<int> rowsToRemove;
  foreach (const QModelIndex& idx, toRemove)
  {
    rowsToRemove.insert(idx.row());
  }

  for (auto riter = rowsToRemove.rbegin(); riter != rowsToRemove.rend(); ++riter)
  {
    emit this->beginRemoveRows(QModelIndex(), *riter, *riter);
    this->Internals->Items.remove(*riter);
    emit this->endRemoveRows();
  }

  if (rowsToRemove.size() > 0 && *rowsToRemove.begin() > this->Internals->Items.size())
  {
    return this->index(*rowsToRemove.begin(), 0);
  }

  if (this->Internals->Items.size() > 0)
  {
    return this->index(this->Internals->Items.size() - 1, 0);
  }
  return QModelIndex();
}

void pqAnnotationsModel::removeAllAnnotations()
{
  emit this->beginResetModel();
  this->Internals->Items.clear();
  emit this->endResetModel();
}

void pqAnnotationsModel::setAnnotations(const QVector<std::pair<QString, QString> >& newAnnotations)
{
  if (newAnnotations.size() == 0)
  {
    this->removeAllAnnotations();
  }
  else
  {
    int size = this->Internals->Items.size();
    int annotationSize = newAnnotations.size();
    if (annotationSize > size)
    {
      // rows are added.
      emit this->beginInsertRows(QModelIndex(), size, annotationSize - 1);
      this->Internals->Items.resize(annotationSize);
      emit this->endInsertRows();
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

      if (this->Internals->Colors.size() > 0)
      {
        if (!this->Internals->Items[cc].Color.isValid())
        {
          // Copy color, using modulo if annotation are bigger than current number of colors
          // and color is not yet defined for this item
          this->Internals->Items[cc].setData(
            0, this->Internals->Colors[cc % this->Internals->Colors.size()]);
          colorFlag = true;
        }
        // Initialize Opacities if not defined
        if (this->Internals->Items[cc].Opacity == -1)
        {
          this->Internals->Items[cc].setData(1, 1.0);
          opacityFlag = true;
        }
      }
    }
    if (colorFlag)
    {
      emit this->dataChanged(this->index(0, 0), this->index(this->Internals->Items.size() - 1, 0));
    }
    if (opacityFlag)
    {
      emit this->dataChanged(this->index(0, 1), this->index(this->Internals->Items.size() - 1, 1));
    }
    if (valueFlag)
    {
      emit this->dataChanged(this->index(0, 2), this->index(this->Internals->Items.size() - 1, 2));
    }
    if (annotationFlag)
    {
      emit this->dataChanged(this->index(0, 3), this->index(this->Internals->Items.size() - 1, 3));
    }
  }
}

QVector<std::pair<QString, QString> > pqAnnotationsModel::annotations() const
{
  QVector<std::pair<QString, QString> > theAnnotations(this->Internals->Items.size());
  int cc = 0;
  foreach (const AnnotationItem& item, this->Internals->Items)
  {
    theAnnotations[cc] = std::make_pair(item.Value, item.Annotation);
    cc++;
  }
  return theAnnotations;
}

void pqAnnotationsModel::setIndexedColors(const QVector<QColor>& newColors)
{
  this->Internals->Colors = newColors;
  int colorSize = newColors.size();

  // now check for data changes.
  if (colorSize > 0)
  {
    bool colorFlag = false;
    for (int cc = 0; cc < this->Internals->Items.size(); cc++)
    {
      if (!this->Internals->Items[cc].Color.isValid() ||
        this->Internals->Items[cc].Color != newColors[cc % colorSize])
      {
        // Add color with a modulo so all values have colors
        this->Internals->Items[cc].setData(0, newColors[cc % colorSize]);
        colorFlag = true;
      }
    }
    if (colorFlag)
    {
      emit this->dataChanged(this->index(0, 0), this->index(this->Internals->Items.size() - 1, 0));
    }
  }
}

QVector<QColor> pqAnnotationsModel::indexedColors() const
{
  QVector<QColor> icolors(this->Internals->Items.size());
  int cc = 0;
  foreach (const AnnotationItem& item, this->Internals->Items)
  {
    icolors[cc] = item.Color;
    cc++;
  }
  return icolors;
}

bool pqAnnotationsModel::hasColors() const
{
  return this->Internals->Colors.size() != 0;
}

void pqAnnotationsModel::setIndexedOpacities(const QVector<double>& newOpacities)
{
  bool opacityFlag = false;
  for (int cc = 0; cc < this->Internals->Items.size() && cc < newOpacities.size(); cc++)
  {
    if (this->Internals->Items[cc].Opacity == -1 ||
      this->Internals->Items[cc].Opacity != newOpacities[cc])
    {
      this->Internals->Items[cc].setData(1, newOpacities[cc]);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    emit this->dataChanged(this->index(0, 1), this->index(this->Internals->Items.size() - 1, 1));
  }
}

QVector<double> pqAnnotationsModel::indexedOpacities() const
{
  QVector<double> opacities(this->Internals->Items.size());
  int cc = 0;
  foreach (const AnnotationItem& item, this->Internals->Items)
  {
    opacities[cc] = item.Opacity;
    cc++;
  }
  return opacities;
}

void pqAnnotationsModel::setGlobalOpacity(double opacity)
{
  this->GlobalOpacity = opacity;
  bool opacityFlag = false;
  for (int cc = 0; cc < this->Internals->Items.size(); cc++)
  {
    if (this->Internals->Items[cc].Opacity == -1 || this->Internals->Items[cc].Opacity != opacity)
    {
      this->Internals->Items[cc].setData(1, opacity);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    emit this->dataChanged(this->index(0, 1), this->index(this->Internals->Items.size() - 1, 1));
  }
}

void pqAnnotationsModel::setSelectedOpacity(QList<int> rows, double opacity)
{
  bool opacityFlag = false;
  foreach (int cc, rows)
  {
    if (this->Internals->Items[cc].Opacity == -1 || this->Internals->Items[cc].Opacity != opacity)
    {
      this->Internals->Items[cc].setData(1, opacity);
      opacityFlag = true;
    }
  }
  if (opacityFlag)
  {
    emit this->dataChanged(this->index(0, 1), this->index(this->Internals->Items.size() - 1, 1));
  }
}
