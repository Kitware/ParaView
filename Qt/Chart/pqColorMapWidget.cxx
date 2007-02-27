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
#include "pqPointMarker.h"
#include "pqPixelTransferFunction.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QTimer>


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


class pqColorMapWidgetInternal
{
public:
  enum MouseMode
    {
    NoMode,
    MoveWait,
    MoveMode,
    ZoomMode,
    PanMode
    };

public:
  pqColorMapWidgetInternal();
  ~pqColorMapWidgetInternal() {}

  QList<pqColorMapWidgetItem *> Items; ///< The list of points.
  QPoint ImagePoint;                   ///< The color scale position.
  pqPixelTransferFunction PixelMap;    ///< The pixel to value map.
  QTimer *MoveTimer;                   ///< Used for mouse interaction.
  int PointIndex;                      ///< Used for mouse interaction.
  MouseMode Mode;                      ///< The current mouse mode.
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
  this->MoveTimer = 0;
  this->PointIndex = -1;
  this->Mode = pqColorMapWidgetInternal::NoMode;
}


//-----------------------------------------------------------------------------
pqColorMapWidget::pqColorMapWidget(QWidget *widgetParent)
  : QAbstractScrollArea(widgetParent)
{
  this->Internal = new pqColorMapWidgetInternal();
  this->DisplayImage = 0;
  this->Space = pqColorMapWidget::HsvSpace;
  this->TableSize = 0;
  this->Spacing = 1;
  this->Margin = 2;
  this->PointWidth = 9;
  this->AddingAllowed = true;
  this->MovingAllowed = true;

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

void pqColorMapWidget::setColorSpace(pqColorMapWidget::ColorSpace space)
{
  if(this->Space != space)
    {
    this->Space = space;
    this->layoutColorMap();
    this->viewport()->update();
    }
}

int pqColorMapWidget::getPointCount() const
{
  return this->Internal->Items.size();
}

int pqColorMapWidget::addPoint(const pqChartValue &value, const QColor &color)
{
  // The list of points should be in ascending value order. Add the
  // new point according to its value.
  QList<pqColorMapWidgetItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++ iter)
    {
    if(value == (*iter)->Value)
      {
      return -1;
      }
    else if(value < (*iter)->Value)
      {
      break;
      }
    }

  pqColorMapWidgetItem *item = new pqColorMapWidgetItem(value, color);
  if(iter == this->Internal->Items.end())
    {
    // Add the point to the end of the list if it is greater than all
    // the current points.
    this->Internal->Items.append(item);
    }
  else
    {
    this->Internal->Items.insert(iter, item);
    }

  this->layoutColorMap();
  this->viewport()->update();
  return this->Internal->Items.indexOf(item);
}

void pqColorMapWidget::removePoint(int index)
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    pqColorMapWidgetItem *item = this->Internal->Items.takeAt(index);
    delete item;
    this->layoutColorMap();
    this->viewport()->update();
    }
}

void pqColorMapWidget::clearPoints()
{
  QList<pqColorMapWidgetItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Items.clear();
  this->layoutColorMap();
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
    this->viewport()->update();

    // Notify observers that the color changed.
    emit this->colorChanged(index, color);
    }
}

void pqColorMapWidget::setTableSize(int tableSize)
{
  if(this->TableSize != tableSize)
    {
    this->TableSize = tableSize;
    this->layoutColorMap();
    this->viewport()->update();
    }
}

void pqColorMapWidget::setValueRange(const pqChartValue &min,
  const pqChartValue &max)
{
  if(this->Internal->Items.size() == 0)
    {
    return;
    }

  // Scale the current points to fit the given range.
  if(this->Internal->Items.size() == 1)
    {
    this->Internal->Items.first()->Value = min;
    }
  else
    {
    pqChartValue newMin, newRange;
    pqChartValue oldMin = this->Internal->Items.first()->Value;
    pqChartValue oldRange = this->Internal->Items.last()->Value - oldMin;
    if(max < min)
      {
      newMin = max;
      newRange = min - max;
      }
    else
      {
      newMin = min;
      newRange = max - min;
      }

    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter)
      {
      (*iter)->Value = (((*iter)->Value - oldMin) * newRange) / oldRange;
      (*iter)->Value += newMin;
      }
    }

  this->layoutColorMap();
  this->viewport()->update();
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
  int imageWidth = this->DisplayImage->width();
  int pixelWidth = this->Internal->PixelMap.getPixelRange();
  for(++iter; iter != this->Internal->Items.end(); ++iter)
    {
    // Draw the colors between the previous and next color.
    QColor next = (*iter)->Color;
    int w = (*iter)->Pixel - item->Pixel;
    w = (w * imageWidth) / pixelWidth;

    if(w > 0)
      {
      int x1 = px - 1;
      int x2 = x1 + w;
      for( ; px <= x2; px++)
        {
        // Use rgb or hsv space depending on the user option.
        if(px == x2)
          {
          painter.setPen(next);
          }
        else if(this->Space == pqColorMapWidget::RgbSpace)
          {
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int r=0, g=0, b=0;
          r = ((px - x1)*(next.red() - previous.red()))/w + previous.red();
          g = ((px - x1)*(next.green() - previous.green()))/w +
              previous.green();
          b = ((px - x1)*(next.blue() - previous.blue()))/w + previous.blue();
          painter.setPen(QColor(r, g, b));
          }
        else if(this->Space == pqColorMapWidget::HsvSpace)
          {
          // TODO: Add wrapping HSV
          // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
          int h=0, s=0, v=0;
          h = ((px - x1)*(next.hue() - previous.hue()))/w + previous.hue();
          s = ((px - x1)*(next.saturation() - previous.saturation()))/w +
              previous.saturation();
          v = ((px - x1)*(next.value() - previous.value()))/w + previous.value();
          painter.setPen(QColor::fromHsv(h, s, v));
          }

        painter.drawLine(px, 0, px, imageHeight);
        }
      }

    previous = next;
    item = *iter;
    }

  // Make sure the last pixel is drawn.
  if(px < imageWidth - 1)
    {
    painter.drawLine(px, 0, px, imageHeight);
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
  // Make sure the timer is allocated and connected.
  if(!this->Internal->MoveTimer)
    {
    this->Internal->MoveTimer = new QTimer(this);
    this->Internal->MoveTimer->setObjectName("MouseMoveTimeout");
    this->Internal->MoveTimer->setSingleShot(true);
    connect(this->Internal->MoveTimer, SIGNAL(timeout()),
        this, SLOT(moveTimeout()));
    }

  // Use the mouse press coordinates to determine if the user picked a
  // point.
  bool foundPoint = false;
  int ex = e->x();
  if(this->isInScaleRegion(ex, e->y()))
    {
    // The mouse is in the point scale region. Determine if it is over
    // one of the points.
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
  if(this->Internal->Mode == pqColorMapWidgetInternal::MoveWait)
    {
    this->Internal->Mode = pqColorMapWidgetInternal::NoMode;
    if(this->Internal->MoveTimer)
      {
      this->Internal->MoveTimer->stop();
      }
    }

  if(this->Internal->Mode == pqColorMapWidgetInternal::NoMode)
    {
    if(e->buttons() == Qt::LeftButton)
      {
      // Enter point move mode.
      this->Internal->Mode = pqColorMapWidgetInternal::MoveMode;
      }
    }

  if(this->Internal->Mode == pqColorMapWidgetInternal::MoveMode)
    {
    // Move the current point. The first and last point are never
    // movable.
    if(this->MovingAllowed && this->Internal->PointIndex > 0 &&
        this->Internal->PointIndex < this->Internal->Items.size() - 1)
      {
      // TODO
      }
    }
}

void pqColorMapWidget::mouseReleaseEvent(QMouseEvent *e)
{
  if(this->Internal->Mode == pqColorMapWidgetInternal::MoveWait)
    {
    this->Internal->Mode = pqColorMapWidgetInternal::NoMode;
    if(this->Internal->MoveTimer)
      {
      this->Internal->MoveTimer->stop();
      }
    }

  if(this->Internal->Mode == pqColorMapWidgetInternal::MoveMode)
    {
    // TODO: Finish point move mode.
    this->Internal->Mode = pqColorMapWidgetInternal::NoMode;
    }
  else if(e->button() == Qt::LeftButton)
    {
    if(this->Internal->PointIndex != -1)
      {
      // Make a request for the color change event.
      emit this->colorChangeRequested(this->Internal->PointIndex);
      }
    else if(this->AddingAllowed && this->Internal->PixelMap.isValid() &&
        this->isInScaleRegion(e->x(), e->y()))
      {
      // Add a point to the list. Use the mouse position to determine
      // the value and color.
      pqChartValue value = this->Internal->PixelMap.getValueFor(e->x());
      QImage image = this->DisplayImage->toImage();
      QColor color = image.pixel(e->x() - this->Internal->ImagePoint.x(), 0);
      int index = this->addPoint(value, color);
      if(index != -1)
        {
        emit this->pointAdded(index);
        }
      }
    }
  // TEMP: Use the middle button to remove points.
  else if(e->button() == Qt::MidButton)
    {
    if(this->AddingAllowed && this->Internal->PointIndex > 0 &&
        this->Internal->PointIndex < this->Internal->Items.size() - 1)
      {
      int index = this->Internal->PointIndex;
      this->Internal->PointIndex = -1;
      this->removePoint(index);
      emit this->pointAdded(index);
      }
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
    painter.save();
    painter.translate(0, this->Margin + (this->PointWidth / 2));
    painter.setPen(QColor(128, 128, 128));
    painter.drawLine(this->Internal->PixelMap.getMinPixel(), 0,
        this->Internal->PixelMap.getMaxPixel(), 0);

    // Draw the points on the scale.
    painter.setPen(QColor(Qt::black));
    QSize pointSize(this->PointWidth, this->PointWidth);
    pqDiamondPointMarker marker(pointSize);
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter)
      {
      painter.save();
      painter.setBrush(QBrush((*iter)->Color));
      painter.translate((*iter)->Pixel, 0);
      marker.drawMarker(painter);
      painter.restore();
      }

    painter.restore();
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
  this->viewport()->update();
}

void pqColorMapWidget::scrollContentsBy(int, int)
{
}

void pqColorMapWidget::moveTimeout()
{
  this->Internal->Mode = pqColorMapWidgetInternal::NoMode;
}

bool pqColorMapWidget::isInScaleRegion(int px, int py)
{
  return py >= this->Margin && py <= this->Margin + this->PointWidth &&
      px >= this->Margin && px <= this->viewport()->width() - this->Margin;
}


