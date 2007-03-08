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
#include <QRect>
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
  QRect ImageArea;                     ///< The color scale rectangle.
  QPoint LastPoint;                    ///< Used for interaction.
  pqPixelTransferFunction PixelMap;    ///< The pixel to value map.
  QTimer *MoveTimer;                   ///< Used for mouse interaction.
  int PointIndex;                      ///< Used for mouse interaction.
  MouseMode Mode;                      ///< The current mouse mode.
  bool PointMoved;                     ///< True if point was moved.
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
  : Items(), ImageArea(), LastPoint(), PixelMap()
{
  this->MoveTimer = 0;
  this->PointIndex = -1;
  this->Mode = pqColorMapWidgetInternal::NoMode;
  this->PointMoved = false;
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
    this->generateGradient();
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
  this->viewport()->update();
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
    this->generateGradient();
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
    this->generateGradient();
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
  // Set up the gradient area. The gradient area is inside the view
  // area. Space should be left for the margin and point size.
  int margin = this->PointWidth / 2 + this->Margin;
  this->Internal->ImageArea.setTop(margin);
  this->Internal->ImageArea.setLeft(margin);
  this->Internal->ImageArea.setWidth(this->viewport()->width() - 2 * margin);
  this->Internal->ImageArea.setHeight(this->viewport()->height() - 2 * margin);

  // Use the gradient area to set up the pixel to value map.
  if(this->Internal->ImageArea.isValid())
    {
    this->Internal->PixelMap.setPixelRange(this->Internal->ImageArea.left(),
        this->Internal->ImageArea.right());
    }
  else
    {
    this->Internal->PixelMap.setPixelRange(0, 0);
    }

  // Make sure the value range is set as well. There must be at least
  // two point for a valid range.
  if(this->Internal->Items.size() < 2)
    {
    this->Internal->PixelMap.setValueRange(pqChartValue(), pqChartValue());
    }
  else
    {
    this->Internal->PixelMap.setValueRange(
        this->Internal->Items.first()->Value,
        this->Internal->Items.last()->Value);
    }

  // Layout the points.
  this->layoutPoints();

  // Layout the gradient image.
  this->generateGradient();
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

  // Save the mouse position.
  this->Internal->LastPoint = e->pos();

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
      // Make sure the point does not run into neighboring points.
      int index = this->Internal->PointIndex;
      pqColorMapWidgetItem *item = this->Internal->Items[index];
      int delta = e->x() - this->Internal->LastPoint.x();
      index = delta > 0 ? index + 1 : index - 1;
      int space = this->Internal->Items[index]->Pixel - item->Pixel;
      if(delta > 0 && delta >= space)
        {
        delta = space > 0 ? space - 1 : space;
        }
      else if(delta < 0 && delta <= space)
        {
        delta = space < 0 ? space + 1 : space;
        }

      // Only update the last position if the point can be moved.
      if(delta != 0)
        {
        this->Internal->LastPoint.rx() += delta;
        this->Internal->PointMoved = true;
        item->Pixel += delta;
        item->Value = this->Internal->PixelMap.getValueFor(item->Pixel);
        this->generateGradient();
        this->viewport()->update();
        }
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
    this->Internal->Mode = pqColorMapWidgetInternal::NoMode;
    if(this->Internal->PointMoved)
      {
      // Signal that the point was moved.
      this->Internal->PointMoved = false;
      emit this->pointMoved(this->Internal->PointIndex);
      }
    }
  else if(e->button() == Qt::LeftButton)
    {
    if(this->Internal->PointIndex != -1)
      {
      if(e->modifiers() == Qt::ControlModifier)
        {
        // Ctrl+click removes a point.
        if(this->AddingAllowed && this->Internal->PointIndex > 0 &&
            this->Internal->PointIndex < this->Internal->Items.size() - 1)
          {
          int index = this->Internal->PointIndex;
          this->Internal->PointIndex = -1;
          this->removePoint(index);
          emit this->pointAdded(index);
          }
        }
      else
        {
        // Make a request for the color change event.
        emit this->colorChangeRequested(this->Internal->PointIndex);
        }
      }
    else if(this->AddingAllowed && this->Internal->PixelMap.isValid() &&
        this->isInScaleRegion(e->x(), e->y()))
      {
      // Add a point to the list. Use the mouse position to determine
      // the value and color.
      pqChartValue value = this->Internal->PixelMap.getValueFor(e->x());
      QImage image = this->DisplayImage->toImage();
      QColor color = image.pixel(e->x() - this->Internal->ImageArea.left(), 0);
      int index = this->addPoint(value, color);
      if(index != -1)
        {
        emit this->pointAdded(index);
        }
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

  if(this->DisplayImage)
    {
    painter.drawPixmap(this->Internal->ImageArea.topLeft(),
        *this->DisplayImage);

    // Draw an outline for the color gradient image.
    painter.setPen(QColor(100, 100, 100));
    QRect border = this->Internal->ImageArea;
    border.setLeft(border.left() - 1);
    border.setBottom(border.bottom() - 1);
    painter.drawRect(border);
    }

  if(this->Internal->PixelMap.isValid())
    {
    // Draw the line for the point scale.
    painter.translate(0, this->Internal->ImageArea.top());
    painter.setPen(QColor(0, 0, 0));
    painter.drawLine(this->Internal->PixelMap.getMinPixel(), 0,
        this->Internal->PixelMap.getMaxPixel(), 0);

    // Draw the points on the scale.
    painter.setPen(QColor(Qt::black));
    //painter.setRenderHint(QPainter::Antialiasing, true);
    pqDiamondPointMarker marker(QSize(this->PointWidth, this->PointWidth));
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter)
      {
      painter.save();
      if((*iter)->Color.red() < 60 && (*iter)->Color.green() < 60 &&
          (*iter)->Color.blue() < 60)
        {
        // If the color is too dark, a black outline won't show up.
        painter.setPen(QColor(128, 128, 128));
        }

      painter.setBrush(QBrush((*iter)->Color));
      painter.translate((*iter)->Pixel, 0);
      marker.drawMarker(painter);
      painter.restore();
      }
    }

  e->accept();
}

void pqColorMapWidget::resizeEvent(QResizeEvent *)
{
  this->layoutColorMap();
  this->viewport()->update();
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

void pqColorMapWidget::layoutPoints()
{
  if(this->Internal->PixelMap.isValid())
    {
    // Loop through the list of items and set the pixel position.
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter)
      {
      (*iter)->Pixel = this->Internal->PixelMap.getPixelFor((*iter)->Value);
      }
    }
}

void pqColorMapWidget::generateGradient()
{
  if(this->DisplayImage)
    {
    delete this->DisplayImage;
    this->DisplayImage = 0;
    }

  if(this->Internal->ImageArea.isValid() && this->Internal->Items.size() > 1)
    {
    if(this->TableSize > 0)
      {
      this->DisplayImage = new QPixmap(this->TableSize, 1);
      }
    else
      {
      this->DisplayImage = new QPixmap(this->Internal->ImageArea.size());
      }

    // Draw the first color.
    QList<pqColorMapWidgetItem *>::Iterator iter =
        this->Internal->Items.begin();
    pqColorMapWidgetItem *item = *iter;
    QColor previous = item->Color;
    int imageHeight = this->DisplayImage->height();
    QPainter painter(this->DisplayImage);
    painter.setPen(previous);
    painter.drawLine(0, 0, 0, imageHeight);

    // Loop through the points to draw the gradient(s).
    int px = 1;
    int imageWidth = this->DisplayImage->width();
    int pixelWidth = this->Internal->ImageArea.width() - 1;
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
      *this->DisplayImage = this->DisplayImage->scaled(
          this->Internal->ImageArea.size());
      }
    }
}


