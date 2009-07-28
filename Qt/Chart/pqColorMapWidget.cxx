/*=========================================================================

   Program: ParaView
   Module:    pqColorMapWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqColorMapWidget.cxx
/// \date 7/7/2006

#include "pqColorMapWidget.h"

#include "pqChartValue.h"
#include "pqColorMapModel.h"
#include "pqPointMarker.h"
#include "pqChartPixelScale.h"

#include <QColor>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTimer>

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626
#endif

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

  QList<int> Items;           ///< The list of point locations.
  QRect ImageArea;            ///< The color scale rectangle.
  QPoint LastPoint;           ///< Used for interaction.
  pqChartPixelScale PixelMap; ///< The pixel to value map.
  QTimer *MoveTimer;          ///< Used for mouse interaction.
  MouseMode Mode;             ///< The current mouse mode.
  int PointIndex;             ///< Used for mouse interaction.
  int CurrentPoint;           ///< Used for point selection.
  bool PointMoved;            ///< True if point was moved.
};

//=============================================================================
// Given two angular orientations, returns the smallest angle between the two.
inline double pqColorMapWidgetAngleDiff(double a1, double a2)
{
  double adiff = a1 - a2;
  if (adiff < 0.0) adiff = -adiff;
  while (adiff >= M_PI) adiff -= M_PI;
  return adiff;
}

// For the case when interpolating from a saturated color to an unsaturated
// color, find a hue for the unsaturated color that makes sense.
inline double pqColorMapWidgetAdjustHue(double satM, double satS, double satH,
                                        double unsatM)
{
  if (satM >= unsatM - 0.1)
    {
    // The best we can do is hold hue constant.
    return satH;
    }
  else
    {
    // This equation is designed to make the perceptual change of the
    // interpolation to be close to constant.
    double hueSpin = (  satS*sqrt(unsatM*unsatM - satM*satM)
                      / (satM*sin(satS)) );
    // Spin hue away from 0 except in purple hues.
    if (satH > -0.3*M_PI)
      {
      return satH + hueSpin;
      }
    else
      {
      return satH - hueSpin;
      }
    }
}

//-----------------------------------------------------------------------------
pqColorMapWidgetInternal::pqColorMapWidgetInternal()
  : Items(), ImageArea(), LastPoint(), PixelMap()
{
  this->MoveTimer = 0;
  this->Mode = pqColorMapWidgetInternal::NoMode;
  this->PointIndex = -1;
  this->CurrentPoint = -1;
  this->PointMoved = false;
}


//-----------------------------------------------------------------------------
pqColorMapWidget::pqColorMapWidget(QWidget *widgetParent)
  : QAbstractScrollArea(widgetParent)
{
  this->Internal = new pqColorMapWidgetInternal();
  this->Model = 0;
  this->DisplayImage = 0;
  this->TableSize = 0;
  this->Margin = 2;
  this->PointWidth = 9;
  this->AddingAllowed = true;
  this->MovingAllowed = true;

  this->setContextMenuPolicy(Qt::CustomContextMenu);
}

pqColorMapWidget::~pqColorMapWidget()
{
  delete this->Internal;
  if(this->DisplayImage)
    {
    delete this->DisplayImage;
    }
}

void pqColorMapWidget::setModel(pqColorMapModel *model)
{
  if(this->Model)
    {
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Internal->CurrentPoint = -1;
  this->Model = model;
  if(this->Model)
    {
    this->connect(this->Model, SIGNAL(colorSpaceChanged()),
        this, SLOT(updateColorGradient()));
    this->connect(this->Model, SIGNAL(tableSizeChanged()),
        this, SLOT(updateColorGradient()));
    this->connect(this->Model, SIGNAL(colorChanged(int, const QColor &)),
        this, SLOT(updateColorGradient()));
    this->connect(this->Model, SIGNAL(pointsReset()),
        this, SLOT(handlePointsReset()));
    this->connect(this->Model, SIGNAL(pointAdded(int)),
        this, SLOT(addPoint(int)));
    this->connect(this->Model, SIGNAL(removingPoint(int)),
        this, SLOT(startRemovingPoint(int)));
    this->connect(this->Model, SIGNAL(pointRemoved(int)),
        this, SLOT(finishRemovingPoint(int)));
    this->connect(this->Model, SIGNAL(valueChanged(int, const pqChartValue &)),
        this, SLOT(updatePointValue(int, const pqChartValue &)));
    }

  this->handlePointsReset();
}

void pqColorMapWidget::setTableSize(int resolution)
{
  if(resolution != this->TableSize)
    {
    this->TableSize = resolution;
    if(this->Model && !this->Model->isDataBeingModified())
      {
      this->layoutColorMap();
      this->viewport()->update();
      }
    }
}

int pqColorMapWidget::getCurrentPoint() const
{
  return this->Internal->CurrentPoint;
}

void pqColorMapWidget::setCurrentPoint(int index)
{
  if(!this->Model || index < 0 || index >= this->Model->getNumberOfPoints())
    {
    return;
    }

  if(index != this->Internal->CurrentPoint)
    {
    this->Internal->CurrentPoint = index;
    emit this->currentPointChanged(this->Internal->CurrentPoint);
    this->viewport()->update();
    }
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
    int left = this->Internal->ImageArea.left();
    this->Internal->PixelMap.setPixelRange(left,
        left + this->Internal->ImageArea.width() - 1);
    }
  else
    {
    this->Internal->PixelMap.setPixelRange(0, 0);
    }

  // Make sure the value range is set as well. There must be at least
  // two point for a valid range.
  pqChartValue min, max;
  if(this->Model)
    {
    this->Model->getValueRange(min, max);
    }

  this->Internal->PixelMap.setValueRange(min, max);

  // Layout the points.
  this->layoutPoints();

  // Layout the gradient image.
  this->generateGradient();
}

QSize pqColorMapWidget::sizeHint() const
{
  // Set up the prefered size for the color gradient image. Add space
  // for the point control line and layout spacing.
  int w = 100;
  int h = 22 + this->PointWidth + (2 * this->Margin);
  return QSize(w, h);
}

void pqColorMapWidget::keyPressEvent(QKeyEvent *e)
{
  if(!this->Model)
    {
    return;
    }

  if(e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace)
    {
    if(this->Internal->CurrentPoint != -1 && this->AddingAllowed &&
        this->Internal->PointIndex > 0 &&
        this->Internal->PointIndex < this->Internal->Items.size() - 1)
      {
      this->Model->removePoint(this->Internal->CurrentPoint);
      }
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(this->Internal->CurrentPoint > 0)
      {
      this->Internal->CurrentPoint--;
      emit this->currentPointChanged(this->Internal->CurrentPoint);
      this->viewport()->update();
      }
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(this->Internal->CurrentPoint < this->Model->getNumberOfPoints() - 1)
      {
      this->Internal->CurrentPoint++;
      emit this->currentPointChanged(this->Internal->CurrentPoint);
      this->viewport()->update();
      }
    }
}

void pqColorMapWidget::mousePressEvent(QMouseEvent *e)
{
  if(!this->Model)
    {
    return;
    }

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
  int ex = e->x();
  this->Internal->PointIndex = -1;
  if(this->isInScaleRegion(ex, e->y()))
    {
    // The mouse is in the point scale region. Determine if it is over
    // one of the points.
    int halfWidth = this->PointWidth / 2 + 1;
    QList<int>::Iterator iter = this->Internal->Items.begin();
    for(int i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      if(ex < *iter - halfWidth)
        {
        // The mouse is between points.
        break;
        }
      else if(ex <= *iter + halfWidth)
        {
        // The mouse is over a point.
        this->Internal->PointIndex = i;
        break;
        }
      }
    }
}

void pqColorMapWidget::mouseMoveEvent(QMouseEvent *e)
{
  if(!this->Model)
    {
    return;
    }

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
      int pixel = this->Internal->Items[index];
      int delta = e->x() - this->Internal->LastPoint.x();
      index = delta > 0 ? index + 1 : index - 1;
      int space = this->Internal->Items[index] - pixel;
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
        pixel += delta;
        this->Internal->Items[this->Internal->PointIndex] = pixel;
        this->Internal->LastPoint.rx() += delta;
        this->Internal->PointMoved = true;

        pqChartValue value;
        this->Internal->PixelMap.getValue(pixel, value);
        this->Model->setPointValue(this->Internal->PointIndex, value);

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

  if(!this->Model)
    {
    return;
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
      if(this->Internal->CurrentPoint == this->Internal->PointIndex)
        {
        // This is a click on the current point. Make a request for
        // the color change event.
        emit this->colorChangeRequested(this->Internal->CurrentPoint);
        }
      else
        {
        this->Internal->CurrentPoint = this->Internal->PointIndex;
        emit this->currentPointChanged(this->Internal->CurrentPoint);
        this->viewport()->update();
        }
      }
    else if(this->AddingAllowed && e->modifiers() == Qt::NoModifier &&
        this->Internal->PixelMap.isValid() &&
        this->Internal->ImageArea.contains(e->pos()) &&
        !this->Internal->Items.contains(e->x()))
      {
      // Add a point to the list. Use the mouse position to determine
      // the value and color.
      pqChartValue value;
      this->Internal->PixelMap.getValue(e->x(), value);
      QImage image = this->DisplayImage->toImage();
      QColor color = image.pixel(e->x() - this->Internal->ImageArea.left(), 0);
      this->Model->addPoint(value, color);
      }
    }
}

void pqColorMapWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(this->Internal->Mode == pqColorMapWidgetInternal::NoMode &&
      e->button() == Qt::LeftButton && this->Model &&
      this->Internal->CurrentPoint != -1)
    {
    // Double click behaves the same as selected-clicked.
    emit this->colorChangeRequested(this->Internal->CurrentPoint);
    }
}

void pqColorMapWidget::paintEvent(QPaintEvent *e)
{
  if(!this->Model || this->Internal->Items.size() < 2)
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
    QColor color;
    painter.setPen(QColor(Qt::black));
    //painter.setRenderHint(QPainter::Antialiasing, true);
    pqDiamondPointMarker marker(QSize(this->PointWidth, this->PointWidth));
    int highlightWidth = this->PointWidth + (2 * this->Margin);
    pqDiamondPointMarker highlight(QSize(highlightWidth, highlightWidth));
    QList<int>::Iterator iter = this->Internal->Items.begin();
    for(int i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      painter.save();
      this->Model->getPointColor(i, color);
      painter.setBrush(QBrush(color));
      painter.translate(*iter, 0);
      if(color.red() < 60 && color.green() < 60 && color.blue() < 60)
        {
        // If the color is too dark, a black outline won't show up.
        painter.setPen(QColor(128, 128, 128));
        }

      if(i == this->Internal->CurrentPoint)
        {
        highlight.drawMarker(painter);
        }
      else
        {
        marker.drawMarker(painter);
        }

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

void pqColorMapWidget::updateColorGradient()
{
  this->generateGradient();
  this->viewport()->update();
}

void pqColorMapWidget::handlePointsReset()
{
  this->Internal->CurrentPoint = -1;
  this->Internal->Items.clear();
  if(this->Model)
    {
    for(int i = 0; i < this->Model->getNumberOfPoints(); i++)
      {
      this->Internal->Items.append(0);
      }
    }

  this->layoutColorMap();
  this->viewport()->update();
}

void pqColorMapWidget::addPoint(int index)
{
  if(index < 0)
    {
    return;
    }

  if(index < this->Internal->Items.size())
    {
    this->Internal->Items.insert(index, 0);
    }
  else
    {
    this->Internal->Items.append(0);
    }

  this->layoutColorMap();
  this->viewport()->update();
}

void pqColorMapWidget::startRemovingPoint(int index)
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    this->Internal->Items.removeAt(index);
    }
}

void pqColorMapWidget::finishRemovingPoint(int index)
{
  this->generateGradient();
  if(index == this->Internal->CurrentPoint &&
      index >= this->Model->getNumberOfPoints())
    {
    this->Internal->CurrentPoint = this->Model->getNumberOfPoints() - 1;
    }

  this->viewport()->update();
}

void pqColorMapWidget::updatePointValue(int index, const pqChartValue &value)
{
  if(!this->Internal->PointMoved && this->Internal->PixelMap.isValid() &&
      index >= 0 && index < this->Internal->Items.size())
    {
    // Update the pixel position of the point.
    this->Internal->Items[index] = this->Internal->PixelMap.getPixel(value);

    if(index == 0 || index == this->Internal->Items.size() - 1)
      {
      this->layoutColorMap();
      }
    else
      {
      this->generateGradient();
      }

    this->viewport()->update();
    }
}

bool pqColorMapWidget::isInScaleRegion(int px, int py)
{
  return py >= this->Margin && py <= this->Margin + this->PointWidth &&
      px >= this->Margin && px <= this->viewport()->width() - this->Margin;
}

void pqColorMapWidget::layoutPoints()
{
  if(this->Model && this->Internal->PixelMap.isValid())
    {
    // Loop through the list of items and set the pixel position.
    pqChartValue value;
    QList<int>::Iterator iter = this->Internal->Items.begin();
    for(int i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      this->Model->getPointValue(i, value);
      *iter = this->Internal->PixelMap.getPixel(value);
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

  if(this->Model && this->Internal->ImageArea.isValid() &&
      this->Internal->Items.size() > 1)
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
    int i = 0;
    QColor next, previous;
    QList<int>::Iterator iter = this->Internal->Items.begin();
    this->Model->getPointColor(i, previous);
    int imageHeight = this->DisplayImage->height();
    QPainter painter(this->DisplayImage);
    painter.setPen(previous);
    painter.drawLine(0, 0, 0, imageHeight);

    // Loop through the points to draw the gradient(s).
    int px = 1;
    int pixel = *iter;
    int imageWidth = this->DisplayImage->width();
    int pixelWidth = this->Internal->ImageArea.width() - 1;
    for(++i, ++iter; iter != this->Internal->Items.end(); ++i, ++iter)
      {
      // Draw the colors between the previous and next color.
      this->Model->getPointColor(i, next);
      int w = *iter - pixel;
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
          else if(this->Model->getColorSpace() == pqColorMapModel::RgbSpace)
            {
            // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
            int r=0, g=0, b=0;
            r = ((px - x1)*(next.red() - previous.red()))/w + previous.red();
            g = ((px - x1)*(next.green() - previous.green()))/w +
                previous.green();
            b = ((px - x1)*(next.blue() - previous.blue()))/w + previous.blue();
            painter.setPen(QColor(r, g, b));
            }
          else if(this->Model->getColorSpace() == pqColorMapModel::HsvSpace ||
              this->Model->getColorSpace() == pqColorMapModel::WrappedHsvSpace)
            {
            // vx = ((px - p1)*(v2 - v1))/(p2 - p1) + v1
            int s=0, v=0;
            int h = next.hue();
            int h1 = previous.hue();
            if(this->Model->getColorSpace() == pqColorMapModel::WrappedHsvSpace
                && (h - h1 > 180 || h1 - h > 180))
              {
              if(h1 > h)
                {
                h1 -= 360;
                }
              else
                {
                h -= 360;
                }
              }

            h = ((px - x1)*(h - h1))/w + h1;
            if(h < 0)
              {
              h += 360;
              }

            s = ((px - x1)*(next.saturation() - previous.saturation()))/w +
                previous.saturation();
            v = ((px - x1)*(next.value() - previous.value()))/w + previous.value();
            painter.setPen(QColor::fromHsv(h, s, v));
            }
          else if (this->Model->getColorSpace() == pqColorMapModel::LabSpace)
            {
            double L_next, a_next, b_next, L_previous, a_previous, b_previous;
            pqColorMapModel::RGBToLab(next.redF(), next.greenF(), next.blueF(),
                                      &L_next, &a_next, &b_next);
            pqColorMapModel::RGBToLab(
                           previous.redF(), previous.greenF(), previous.blueF(),
                           &L_previous, &a_previous, &b_previous);
            double L = ((px - x1)*(L_next - L_previous))/w + L_previous;
            double a = ((px - x1)*(a_next - a_previous))/w + a_previous;
            double b = ((px - x1)*(b_next - b_previous))/w + b_previous;
            double red, green, blue;
            pqColorMapModel::LabToRGB(L, a, b, &red, &green, &blue);
            QColor color;
            color.setRgbF(red, green, blue);
            painter.setPen(color);
            }
          else if(this->Model->getColorSpace()==pqColorMapModel::DivergingSpace)
            {
            double M_next, s_next, h_next, M_previous, s_previous, h_previous;
            pqColorMapModel::RGBToMsh(next.redF(), next.greenF(), next.blueF(),
                                      &M_next, &s_next, &h_next);
            pqColorMapModel::RGBToMsh(
                           previous.redF(), previous.greenF(), previous.blueF(),
                           &M_previous, &s_previous, &h_previous);

            double interp = (double)(px - x1)/w;

            // If the endpoints are distinct saturated colors, then place white
            // in between them.
            if (   (s_next > 0.05) && (s_previous > 0.05)
                && (pqColorMapWidgetAngleDiff(h_next, h_previous) > 0.33*M_PI) )
              {
              // Insert the white midpoint by setting one end to white and
              // adjusting the interpolation value.
              if (interp < 0.5)
                {
                M_next = 95.0;  s_next = 0.0;  h_next = 0.0;
                interp = 2.0*interp;
                }
              else
                {
                M_previous = 95.0;  s_previous = 0.0; h_previous = 0.0;
                interp = 2.0*interp - 1.0;
                }
              }

            // If one color has no saturation, thin its hue is invalid.  In this
            // case, we want to set it to something logical so that the
            // interpolation of hue makes sense.
            if ((s_previous < 0.05) && (s_next > 0.05))
              {
              h_previous = pqColorMapWidgetAdjustHue(M_next, s_next, h_next,
                                                     M_previous);
              }
            else if ((s_next < 0.01) && (s_previous > 0.01))
              {
              h_next = pqColorMapWidgetAdjustHue(
                                             M_previous, s_previous, h_previous,
                                             M_next);
              }

            double M = (1-interp)*M_previous + interp*M_next;
            double s = (1-interp)*s_previous + interp*s_next;
            double h = (1-interp)*h_previous + interp*h_next;

            double red, green, blue;
            pqColorMapModel::MshToRGB(M, s, h, &red, &green, &blue);
            QColor color;
            color.setRgbF(red, green, blue);
            painter.setPen(color);
            }

          painter.drawLine(px, 0, px, imageHeight);
          }
        }

      previous = next;
      pixel = *iter;
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


