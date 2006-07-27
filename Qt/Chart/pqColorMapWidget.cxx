/*=========================================================================

   Program: ParaView
   Module:    pqColorMapWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

=========================================================================*/

/// \file pqColorMapWidget.cxx
/// \date 7/7/2006

#include "pqColorMapWidget.h"

#include "pqChartValue.h"
#include "pqMarkerPen.h"
#include "pqPixelTransferFunction.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPoint>
#include <QSize>


/// \class pqColorMapWidgetItem
class pqColorMapWidgetItem
{
public:
  pqColorMapWidgetItem();
  pqColorMapWidgetItem(const pqChartValue &value, const QColor &color);
  ~pqColorMapWidgetItem() {}

  QColor Color;       ///< The Color for the point.
  pqChartValue Value; ///< The value at the point.
  int Pixel;          ///< The pixel location of the point.
};


/// \class pqColorMapWidgetInternal
class pqColorMapWidgetInternal
{
public:
  pqColorMapWidgetInternal();
  ~pqColorMapWidgetInternal() {}

  QList<pqColorMapWidgetItem *> Items; ///< The list of points.
  QPoint ImagePoint;                   ///< The color scale position.
  pqPixelTransferFunction PixelMap;    ///< The pixel to value map.
  int PointIndex;                      ///< Used for mouse interaction.
};


//-----------------------------------------------------------------------------
pqColorMapWidgetItem::pqColorMapWidgetItem()
  : Color(), Value()
{
  this->Pixel = 0;
}

pqColorMapWidgetItem::pqColorMapWidgetItem(const pqChartValue &value,
    const QColor &color)
  : Color(color), Value(value)
{
  this->Pixel = 0;
}


//-----------------------------------------------------------------------------
pqColorMapWidgetInternal::pqColorMapWidgetInternal()
  : Items(), ImagePoint(), PixelMap()
{
  this->PointIndex = -1;
}


//-----------------------------------------------------------------------------
pqColorMapWidget::pqColorMapWidget(QWidget *widgetParent)
  : QAbstractScrollArea(widgetParent)
{
  this->Internal = new pqColorMapWidgetInternal();
  this->DisplayImage = 0;
  this->TableSize = 0;
  this->Spacing = 1;
  this->Margin = 2;
  this->PointWidth = 9;

  this->setContextMenuPolicy(Qt::CustomContextMenu);
}

pqColorMapWidget::~pqColorMapWidget()
{
  QList<pqColorMapWidgetItem *>::Iterator iter = this->Internal->Items.begin();
  for(; iter != this->Internal->Items.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
  if(this->DisplayImage)
    {
    delete this->DisplayImage;
    }
}

QSize pqColorMapWidget::sizeHint() const
{
  // Set up the prefered size for the color gradient image. Add space
  // for the point control line and layout spacing.
  int w = 100;
  int h = 22 + this->PointWidth + this->Spacing + (2 * this->Margin);
  return QSize(w, h);
}

int pqColorMapWidget::getPointCount() const
{
  return this->Internal->Items.size();
}

void pqColorMapWidget::addPoint(const pqChartValue &value, const QColor &color)
{
  // The list of points should be in ascending value order. Add the
  // new point according to its value.
  pqColorMapWidgetItem *item = new pqColorMapWidgetItem(value, color);
  QList<pqColorMapWidgetItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++ iter)
    {
    if(item->Value < (*iter)->Value)
      {
      this->Internal->Items.insert(iter, item);
      break;
      }
    }

  // Add the point to the end of the list if it is greater than all
  // the current points.
  if(iter == this->Internal->Items.end())
    {
    this->Internal->Items.append(item);
    }

  this->layoutColorMap();
  this->update();
}

void pqColorMapWidget::removePoint(int index)
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    pqColorMapWidgetItem *item = this->Internal->Items.takeAt(index);
    delete item;
    }
}

void pqColorMapWidget::getPointValue(int index, pqChartValue &value) const
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    value = this->Internal->Items[index]->Value;
    }
}

void pqColorMapWidget::getPointColor(int index, QColor &color) const
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    color = this->Internal->Items[index]->Color;
    }
}

void pqColorMapWidget::setPointColor(int index, const QColor &color)
{
  if(index >= 0 && index < this->Internal->Items.size() &&
      this->Internal->Items[index]->Color != color)
    {
    this->Internal->Items[index]->Color = color;
    this->layoutColorMap();
    this->update();
    }
}

void pqColorMapWidget::setTableSize(int tableSize)
{
  if(this->TableSize != tableSize)
    {
    this->TableSize = tableSize;
    this->layoutColorMap();
    this->update();
    }
}

void pqColorMapWidget::layoutColorMap()
{
  if(this->DisplayImage)
    {
    delete this->DisplayImage;
    this->DisplayImage = 0;
    }

  if(this->Internal->Items.size() < 2)
    {
    return;
    }

  // Set up the pixel to value map. Adjust the view width to account
  // for the size of the points.
  QSize viewSize = this->viewport()->size();
  this->Internal->ImagePoint.rx() = this->PointWidth/2 + this->Margin;
  viewSize.rwidth() -= 2 * this->Internal->ImagePoint.x();
  if(viewSize.width() <= 0)
    {
    return;
    }

  // Set the pixel range for the new size. Make sure the value range
  // is set as well.
  int px = this->Internal->ImagePoint.x();
  this->Internal->PixelMap.setPixelRange(px, px + viewSize.width() - 1);
  this->Internal->PixelMap.setValueRange(this->Internal->Items.first()->Value,
      this->Internal->Items.last()->Value);
  if(!this->Internal->PixelMap.isValid())
    {
    return;
    }

  // Loop through the list of items and set the pixel position.
  QList<pqColorMapWidgetItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    (*iter)->Pixel = this->Internal->PixelMap.getPixelFor((*iter)->Value);
    }

  // Create the new color gradient image. The image should fill the
  // space not used by the point scale. Adjust the height for the size
  // of the points, the layout margin, and the outline.
  this->Internal->ImagePoint.ry() = this->PointWidth + this->Spacing +
      this->Margin + 1;
  viewSize.rheight() -= this->Internal->ImagePoint.y() + this->Margin + 1;
  if(viewSize.height() <= 0)
    {
    return;
    }

  if(this->TableSize > 0)
    {
    this->DisplayImage = new QPixmap(this->TableSize, 1);
    }
  else
    {
    this->DisplayImage = new QPixmap(viewSize);
    }

  // Draw the first color.
  iter = this->Internal->Items.begin();
  pqColorMapWidgetItem *item = *iter;
  QColor previous = item->Color;
  int imageHeight = this->DisplayImage->height();
  QPainter painter(this->DisplayImage);
  painter.setPen(previous);
  painter.drawLine(0, 0, 0, imageHeight);

  // Loop through the points to draw the gradient(s).
  px = 1;
  for(++iter; iter != this->Internal->Items.end(); ++iter)
    {
    // Draw the colors between the previous and next color.
    QColor next = (*iter)->Color;
    int w = (*iter)->Pixel - item->Pixel;
    if(this->TableSize > 0)
      {
      w = this->TableSize - 1;
      }

    int x1 = px - 1;
    int x2 = x1 + w;
    int h=0, s=0, v=0;
    //int r=0, g=0, b=0;
    for(; px <= x2; px++)
      {
      if(px == x2)
        {
        painter.setPen(next);
        }
      else
        {
        // TODO: Use rgb or hsv space depending on the user option.
        // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
        h = ((px - x1)*(next.hue() - previous.hue()))/w + previous.hue();
        s = ((px - x1)*(next.saturation() - previous.saturation()))/w +
            previous.saturation();
        v = ((px - x1)*(next.value() - previous.value()))/w + previous.value();
        painter.setPen(QColor::fromHsv(h, s, v));
        }

      painter.drawLine(px, 0, px, imageHeight);
      }

    if(this->TableSize > 0)
      {
      // If the table size is set, only two points are supported.
      break;
      }

    previous = next;
    item = *iter;
    }

  if(this->TableSize > 0)
    {
    // Scale the image to fit the widget size.
    painter.end();
    *this->DisplayImage = this->DisplayImage->scaled(viewSize);
    }
}

void pqColorMapWidget::mousePressEvent(QMouseEvent *e)
{
  // Use the mouse press coordinates to determine if the user picked a
  // point.
  bool foundPoint = false;
  if(e->y() >= this->Margin && e->y() <= this->Margin + this->PointWidth)
    {
    // The mouse is in the point scale region. Determine if it is over
    // one of the points.
    int ex = e->x();
    int halfWidth = this->PointWidth/2 + 1;
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for(int i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      if(ex < (*iter)->Pixel - halfWidth)
        {
        // The mouse is between points.
        break;
        }
      else if(ex <= (*iter)->Pixel + halfWidth)
        {
        // The mouse is over a point.
        foundPoint = true;
        this->Internal->PointIndex = i;
        break;
        }
      }
    }

  if(!foundPoint)
    {
    this->Internal->PointIndex = -1;
    }
}

void pqColorMapWidget::mouseMoveEvent(QMouseEvent *e)
{
  if(e->button() == Qt::LeftButton)
    {
    // TODO: Enter point move mode.
    }
}

void pqColorMapWidget::mouseReleaseEvent(QMouseEvent *e)
{
  if(e->button() == Qt::LeftButton)
    {
    // TODO: Finish point move mode.
    // TODO: Allow the user to add points.
    }
}

void pqColorMapWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(e->button() == Qt::LeftButton && this->Internal->PointIndex != -1)
    {
    // TODO: Make a request for the color change event.
    emit this->colorChangeRequested(this->Internal->PointIndex);
    }
}

void pqColorMapWidget::paintEvent(QPaintEvent *e)
{
  if(this->Internal->Items.size() < 2)
    {
    return;
    }

  QRect area = e->rect();
  if(!area.isValid())
    {
    return;
    }

  QPainter painter(this->viewport());
  if(!painter.isActive())
    {
    return;
    }

  if(this->Internal->PixelMap.isValid())
    {
    // Draw the line for the point scale.
    int py = this->Margin + (this->PointWidth / 2);
    painter.setPen(QColor(128, 128, 128));
    painter.drawLine(this->Internal->PixelMap.getMinPixel(), py,
        this->Internal->PixelMap.getMaxPixel(), py);

    // Draw the points on the scale.
    QSize pointSize(this->PointWidth, this->PointWidth);
    pqDiamondMarkerPen marker(painter.pen(), pointSize, QPen(QColor(0, 0, 0)),
        QBrush());
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter)
      {
      marker.setInterior(QBrush((*iter)->Color));
      marker.drawPoint(painter, (*iter)->Pixel, py);
      }
    }

  if(this->DisplayImage)
    {
    // Draw an outline for the color gradient image.
    painter.setPen(QColor(0, 0, 0));
    QRect border(this->Internal->ImagePoint, this->DisplayImage->size());
    border.setLeft(border.left() - 1);
    border.setTop(border.top() - 1);
    painter.drawRect(border);
    painter.drawPixmap(this->Internal->ImagePoint, *this->DisplayImage);
    }

  e->accept();
}

void pqColorMapWidget::resizeEvent(QResizeEvent *)
{
  this->layoutColorMap();
  this->update();
}

void pqColorMapWidget::scrollContentsBy(int, int)
{
}


