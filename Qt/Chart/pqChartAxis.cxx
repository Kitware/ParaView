/*=========================================================================

   Program: ParaView
   Module:    pqChartAxis.cxx

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

/// \file pqChartAxis.cxx
/// \date 12/1/2006

#include "pqChartAxis.h"

#include "pqChartAxisModel.h"
#include "pqChartAxisOptions.h"
#include "pqChartContentsSpace.h"
#include "pqChartValueFormatter.h"

#include <QFont>
#include <QFontMetrics>
#include <QList>
#include <QPainter>
#include <QRect>
#include <QtDebug>

#include <math.h>


class pqChartAxisItem
{
public:
  pqChartAxisItem();
  ~pqChartAxisItem() {}

  float Pixel;
  int Width;
};


class pqChartAxisInternal
{
public:
  pqChartAxisInternal();
  ~pqChartAxisInternal() {}

  QList<pqChartAxisItem *> Items;
  QRect Bounds;
  pqChartValue Minimum;
  pqChartValue Maximum;
  int FontHeight;
  int TickLabelSpacing;
  int TickLength;
  int SmallTickLength;
  int MaxLabelWidth;
  int Skip;
  int TickSkip;
  bool InLayout;
  bool UsingBestFit;
  bool DataAvailable;
  bool ExtraMinPadding;
  bool ExtraMaxPadding;
  bool SpaceTooSmall;
  bool ScaleChanged;
};


// The interval list is used to determine a suitable interval for a
// best-fit axis.
static pqChartValue IntervalList[] = {
    pqChartValue((float)1.0),
    pqChartValue((float)2.0),
    pqChartValue((float)2.5),
    pqChartValue((float)5.0)};
static int IntervalListLength = 4;


//----------------------------------------------------------------------------
pqChartAxisItem::pqChartAxisItem()
{
  this->Pixel = 0;
  this->Width = 0;
}


//----------------------------------------------------------------------------
pqChartAxisInternal::pqChartAxisInternal()
  : Items(), Bounds(), Minimum(), Maximum()
{
  this->FontHeight = 0;
  this->TickLabelSpacing = 0;
  this->TickLength = 5;
  this->SmallTickLength = 3;
  this->MaxLabelWidth = 0;
  this->Skip = 1;
  this->TickSkip = 1;
  this->InLayout = false;
  this->UsingBestFit = false;
  this->DataAvailable = false;
  this->ExtraMinPadding = false;
  this->ExtraMaxPadding = false;
  this->SpaceTooSmall = false;
  this->ScaleChanged = false;
}


//----------------------------------------------------------------------------
pqChartAxis::pqChartAxis(pqChartAxis::AxisLocation location,
    QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqChartAxisInternal();
  this->Options = new pqChartAxisOptions(this);
  this->Model = 0;
  this->Scale = new pqChartPixelScale();
  this->AtMin = 0;
  this->AtMax = 0;
  this->Across = 0;
  this->Zoom = 0;
  this->Formatter = 0;
  this->Location = location;

  // Set up the options object.
  this->Options->setObjectName("Options");
  this->connect(this->Options, SIGNAL(visibilityChanged()),
      this, SIGNAL(layoutNeeded()));
  this->connect(this->Options, SIGNAL(colorChanged()),
      this, SIGNAL(repaintNeeded()));
  this->connect(this->Options, SIGNAL(fontChanged()),
      this, SLOT(handleFontChange()));
  this->connect(this->Options, SIGNAL(presentationChanged()),
      this, SLOT(clearLabelWidthCache()));

  // Set the font height and tick-label space.
  QFontMetrics fm(this->Options->getLabelFont());
  this->Internal->FontHeight = fm.height();
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    this->Internal->TickLabelSpacing = fm.leading();
    }
  else
    {
    this->Internal->TickLabelSpacing = fm.width(" ");
    }
}

pqChartAxis::~pqChartAxis()
{
  QList<pqChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
  delete this->Scale;
}

void pqChartAxis::setModel(pqChartAxisModel *model)
{
  if(this->Model == model)
    {
    return;
    }

  if(this->Model)
    {
    // Clean up connections to the old model.
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    // Listen to the new model's events.
    this->connect(this->Model, SIGNAL(labelInserted(int)),
        this, SLOT(insertLabel(int)));
    this->connect(this->Model, SIGNAL(removingLabel(int)),
        this, SLOT(startLabelRemoval(int)));
    this->connect(this->Model, SIGNAL(labelRemoved(int)),
        this, SLOT(finishLabelRemoval(int)));
    this->connect(this->Model, SIGNAL(labelsReset()),
        this, SLOT(reset()));
    }

  // Clean up the old view data and request a re-layout.
  this->reset();
}

void pqChartAxis::setNeigbors(const pqChartAxis *atMin,
    const pqChartAxis *atMax)
{
  // TODO: Listen for a font change from the other axes to adjust the
  // tick length for top and bottom axes.
  this->AtMin = atMin;
  this->AtMax = atMax;
}

void pqChartAxis::setParallelAxis(const pqChartAxis *across)
{
  this->Across = across;
}

void pqChartAxis::setContentsSpace(const pqChartContentsSpace *contents)
{
  this->Zoom = contents;
}

void pqChartAxis::setScaleType(pqChartPixelScale::ValueScale scale)
{
  if(this->Scale->getScaleType() != scale)
    {
    this->Scale->setScaleType(scale);
    this->Internal->ScaleChanged = true;
    this->clearLabelWidthCache();
    }
}

bool pqChartAxis::isBestFitGenerated() const
{
  return this->Internal->UsingBestFit;
}

void pqChartAxis::setBestFitGenerated(bool on)
{
  this->Internal->UsingBestFit = on;
}

void pqChartAxis::setDataAvailable(bool available)
{
  this->Internal->DataAvailable = available;
}

void pqChartAxis::getBestFitRange(pqChartValue &min,
    pqChartValue &max) const
{
  min = this->Internal->Minimum;
  max = this->Internal->Maximum;
}

void pqChartAxis::setBestFitRange(const pqChartValue &min,
    const pqChartValue &max)
{
  if(max < min)
    {
    this->Internal->Minimum = max;
    this->Internal->Maximum = min;
    }
  else
    {
    this->Internal->Minimum = min;
    this->Internal->Maximum = max;
    }

  // Make sure the min and max are the same type.
  if(this->Internal->Minimum.getType() != this->Internal->Maximum.getType())
    {
    this->Internal->Minimum.convertTo(this->Internal->Maximum.getType());
    }
}

bool pqChartAxis::isMaxExtraPadded() const
{
  return this->Internal->ExtraMaxPadding;
}

void pqChartAxis::setExtraMaxPadding(bool on)
{
  this->Internal->ExtraMaxPadding = on;
}

bool pqChartAxis::isMinExtraPadded() const
{
  return this->Internal->ExtraMinPadding;
}

void pqChartAxis::setExtraMinPadding(bool on)
{
  this->Internal->ExtraMinPadding = on;
}

bool pqChartAxis::isSpaceTooSmall() const
{
  return this->Internal->SpaceTooSmall;
}

void pqChartAxis::setSpaceTooSmall(bool tooSmall)
{
  this->Internal->SpaceTooSmall = tooSmall;
}

void pqChartAxis::setOptions(const pqChartAxisOptions &options)
{
  // Copy the new options.
  *(this->Options) = options;

  // Handle the worst case option change: font.
  this->handleFontChange();
}

void pqChartAxis::layoutAxis(const QRect &area)
{
  // Use the total chart area and the neighboring axes to set the
  // bounding rectangle.
  int space = 0;
  QRect neighbor;
  this->Internal->Bounds = area;
  if(this->Location == pqChartAxis::Top)
    {
    int topDiff = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      space = this->getPreferredSpace();
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      this->AtMin->getBounds(neighbor);
      if(neighbor.isValid())
        {
        topDiff = neighbor.top() - area.top();
        if(topDiff > space)
          {
          space = topDiff;
          }
        }
      }

    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      this->AtMax->getBounds(neighbor);
      if(neighbor.isValid())
        {
        topDiff = neighbor.top() - area.top();
        if(topDiff > space)
          {
          space = topDiff;
          }
        }
      }

    this->Internal->Bounds.setBottom(area.top() + space);
    }
  else if(this->Location == pqChartAxis::Bottom)
    {
    int bottomDiff = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      space = this->getPreferredSpace();
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      this->AtMin->getBounds(neighbor);
      if(neighbor.isValid())
        {
        bottomDiff = area.bottom() - neighbor.bottom();
        if(bottomDiff > space)
          {
          space = bottomDiff;
          }
        }
      }

    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      this->AtMax->getBounds(neighbor);
      if(neighbor.isValid())
        {
        bottomDiff = area.bottom() - neighbor.bottom();
        if(bottomDiff > space)
          {
          space = bottomDiff;
          }
        }
      }

    this->Internal->Bounds.setTop(area.bottom() - space);
    }
  else
    {
    int halfHeight = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      halfHeight = this->getFontHeight() / 2;
      }

    if(this->Across && !this->Across->isSpaceTooSmall())
      {
      int otherHeight = this->Across->getFontHeight() / 2;
      if(otherHeight > halfHeight)
        {
        halfHeight = otherHeight;
        }
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      space = this->AtMin->getPreferredSpace();
      if(halfHeight > space)
        {
        space = halfHeight;
        }
      }
    else
      {
      space = halfHeight;
      }

    this->Internal->Bounds.setBottom(area.bottom() - space);
    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      space = this->AtMax->getPreferredSpace();
      if(halfHeight > space)
        {
        space = halfHeight;
        }
      }
    else
      {
      space = halfHeight;
      }

    this->Internal->Bounds.setTop(area.top() + space);
    }

  // Set up the contents rectangle.
  QRect contents = this->Internal->Bounds;
  if(this->Zoom)
    {
    if(this->Location == pqChartAxis::Left ||
        this->Location == pqChartAxis::Right)
      {
      contents.setBottom(contents.bottom() + this->Zoom->getMaximumYOffset());
      }
    else
      {
      contents.setRight(contents.right() + this->Zoom->getMaximumXOffset());
      }
    }

  // If the axis model is based on the size, it needs to be generated
  // here. Don't send a layout request change for the model events.
  this->Internal->InLayout = true;
  if(this->Scale->getScaleType() == pqChartPixelScale::Linear)
    {
    this->generateLabels(contents);
    }
  else
    {
    this->generateLogLabels(contents);
    }

  this->Internal->InLayout = false;

  // Calculate the label width for any new labels.
  int i = 0;
  pqChartValue value;
  QList<pqChartAxisItem *>::Iterator iter;
  if(this->Options->isVisible() && this->Options->areLabelsVisible())
    {
    QFontMetrics fm(this->Options->getLabelFont());
    bool maxWidthReset = this->Internal->MaxLabelWidth == 0;
    iter = this->Internal->Items.begin();
    for( ; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      // Get the label value from the model.
      this->Model->getLabel(i, value);
      if((*iter)->Width == 0)
        {
        // If the item has no width assigned, use the font metrics to
        // calculate the necessary width.
        QString label = this->getLabel(value);
        (*iter)->Width = fm.width(label);
        if((*iter)->Width > this->Internal->MaxLabelWidth)
          {
          this->Internal->MaxLabelWidth = (*iter)->Width;
          }
        }
      else if(maxWidthReset && (*iter)->Width > this->Internal->MaxLabelWidth)
        {
        // If the max label width was reset, use the labels widths to
        // find the new max.
        this->Internal->MaxLabelWidth = (*iter)->Width;
        }
      }
    }

  // Use the maximum label width to finish setting the bounds.
  if(this->Location == pqChartAxis::Left)
    {
    space = 0;
    if(!this->Internal->SpaceTooSmall && this->Model &&
        this->Model->getNumberOfLabels() > 1)
      {
      space = this->getPreferredSpace();
      }

    this->Internal->Bounds.setRight(area.left() + space);
    }
  else if(this->Location == pqChartAxis::Right)
    {
    space = 0;
    if(!this->Internal->SpaceTooSmall && this->Model &&
        this->Model->getNumberOfLabels() > 1)
      {
      space = this->getPreferredSpace();
      }

    this->Internal->Bounds.setLeft(area.right() - space);
    }
  else
    {
    int halfWidth = 0;
    if(!this->Internal->SpaceTooSmall)
      {
      halfWidth = this->getMaxLabelWidth() / 2;
      }

    if(this->Across && !this->Across->isSpaceTooSmall())
      {
      int otherWidth = this->Across->getMaxLabelWidth() / 2;
      if(otherWidth > halfWidth)
        {
        halfWidth = otherWidth;
        }
      }

    if(this->AtMin && !this->AtMin->isSpaceTooSmall())
      {
      this->AtMin->getBounds(neighbor);
      space = neighbor.isValid() ? neighbor.width() : 0;
      if(halfWidth > space)
        {
        space = halfWidth;
        }
      }
    else
      {
      space = halfWidth;
      }

    this->Internal->Bounds.setLeft(area.left() + space);
    contents.setLeft(contents.left() + space);
    if(this->AtMax && !this->AtMax->isSpaceTooSmall())
      {
      this->AtMax->getBounds(neighbor);
      space = neighbor.isValid() ? neighbor.width() : 0;
      if(halfWidth > space)
        {
        space = halfWidth;
        }
      }
    else
      {
      space = halfWidth;
      }

    this->Internal->Bounds.setRight(area.right() - space);
    contents.setRight(contents.right() - space);
    }

  // Set up the pixel-value scale.
  int pixelMin = 0;
  int pixelMax = 0;
  bool pixelChanged = false;
  if(this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right)
    {
    // The contents bounds determine the min and max pixel locations
    // for left and right axes.
    pixelMin = contents.bottom();
    pixelMax = contents.top();
    if(pixelMin > pixelMax)
      {
      pixelChanged = this->Scale->setPixelRange(pixelMin, pixelMax);
      }
    else
      {
      pixelChanged = this->Scale->setPixelRange(0, 0);
      }
    }
  else
    {
    pixelMin = contents.left();
    pixelMax = contents.right();
    if(pixelMin < pixelMax)
      {
      pixelChanged = this->Scale->setPixelRange(pixelMin, pixelMax);
      }
    else
      {
      pixelChanged = this->Scale->setPixelRange(0, 0);
      }
    }

  bool valueChanged = false;
  if(this->Model && this->Model->getNumberOfLabels() > 1)
    {
    pqChartValue max;
    this->Model->getLabel(0, value);
    this->Model->getLabel(this->Model->getNumberOfLabels() - 1, max);
    valueChanged = this->Scale->setValueRange(value, max);
    }
  else
    {
    valueChanged = this->Scale->setValueRange(pqChartValue(), pqChartValue());
    }

  if((valueChanged || this->Internal->ScaleChanged) &&
      this->Scale->getScaleType() == pqChartPixelScale::Logarithmic &&
      !this->Scale->isLogScaleAvailable())
    {
    qWarning() << "Warning: Invalid range for a logarithmic scale."
               << "Please select a range greater than zero.";
    }

  // Signal the chart layers if the pixel-value map changed.
  if(pixelChanged || valueChanged || this->Internal->ScaleChanged)
    {
    emit this->pixelScaleChanged();
    }

  // Calculate the pixel location for each label.
  this->Internal->ScaleChanged = false;
  this->Internal->Skip = 1;
  this->Internal->TickSkip = 1;
  if(this->Scale->isValid() && this->Options->isVisible() &&
      (this->Options->areLabelsVisible() || this->Options->isGridVisible()))
    {
    iter = this->Internal->Items.begin();
    for(i = 0; iter != this->Internal->Items.end(); ++iter, ++i)
      {
      // Use the pixel-value scale to determine the pixel location for
      // the label.
      this->Model->getLabel(i, value);
      (*iter)->Pixel = this->Scale->getPixelF(value);
      }

    // If there is not space for all the labels, set up the skip count.
    if(this->Scale->getScaleType() == pqChartPixelScale::Logarithmic ||
        !this->Internal->UsingBestFit || this->Internal->Items.size() < 3)
      {
      int needed = 0;
      if(this->Location == pqChartAxis::Left ||
          this->Location == pqChartAxis::Right)
        {
        needed = 2 * this->Internal->FontHeight;
        }
      else
        {
        needed = this->Internal->FontHeight + this->Internal->MaxLabelWidth;
        }

      needed *= this->Internal->Items.size() - 1;
      int pixelRange = this->Scale->getPixelRange();
      this->Internal->Skip = needed / pixelRange;
      if(this->Internal->Skip == 0 || needed % pixelRange > 0)
        {
        this->Internal->Skip += 1;
        }

      if(this->Internal->Skip > 1)
        {
        // If there is not enough space for the tick marks, set up the
        // tick skip count.
        int count = this->Internal->Skip;
        if(count >= this->Internal->Items.size())
          {
          count = this->Internal->Items.size() - 1;
          }

        needed = 4 * count;
        pixelRange = (int)this->Internal->Items[0]->Pixel;
        int pixel = (int)this->Internal->Items[count]->Pixel;
        if(pixel < pixelRange)
          {
          pixelRange = pixelRange - pixel;
          }
        else
          {
          pixelRange = pixel - pixelRange;
          }

        if(pixelRange > 0)
          {
          this->Internal->TickSkip = needed / pixelRange;
          }

        if(this->Internal->TickSkip == 0 || needed % pixelRange > 0)
          {
          this->Internal->TickSkip += 1;
          }
        }
      }
    }
}

void pqChartAxis::adjustAxisLayout()
{
  if(!this->Internal->Bounds.isValid())
    {
    return;
    }

  QRect bounds;
  if(this->Location == pqChartAxis::Left)
    {
    int right = this->Internal->Bounds.right();
    if(this->AtMin)
      {
      this->AtMin->getBounds(bounds);
      if(bounds.left() > right)
        {
        right = bounds.left();
        }
      }

    if(this->AtMax)
      {
      this->AtMin->getBounds(bounds);
      if(bounds.left() > right)
        {
        right = bounds.left();
        }
      }

    this->Internal->Bounds.setRight(right);
    }
  else if(this->Location == pqChartAxis::Right)
    {
    int left = this->Internal->Bounds.left();
    if(this->AtMin)
      {
      this->AtMin->getBounds(bounds);
      if(bounds.right() < left)
        {
        left = bounds.right();
        }
      }

    if(this->AtMax)
      {
      this->AtMin->getBounds(bounds);
      if(bounds.right() < left)
        {
        left = bounds.right();
        }
      }

    this->Internal->Bounds.setLeft(left);
    }
}

int pqChartAxis::getPreferredSpace() const
{
  if(this->Model && this->Options->isVisible() &&
      this->Options->areLabelsVisible())
    {
    if(this->Internal->UsingBestFit && !this->Internal->DataAvailable &&
        this->Internal->Minimum == this->Internal->Maximum)
      {
      return 0;
      }

    if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
      {
      // The preferred height is the sum of the font height, the tick
      // length and the tick-label spacing.
      return this->Internal->FontHeight + this->Internal->TickLength +
          this->Internal->TickLabelSpacing;
      }
    else
      {
      // The preferred width is the sum of the widest label, the tick
      // length and the tick-label spacing.
      return this->Internal->MaxLabelWidth + this->Internal->TickLength +
          this->Internal->TickLabelSpacing;
      }
    }

  return 0;
}

int pqChartAxis::getFontHeight() const
{
  if(this->Model && this->Options->isVisible() &&
      this->Options->areLabelsVisible())
    {
    if(this->Internal->UsingBestFit && !this->Internal->DataAvailable &&
        this->Internal->Minimum == this->Internal->Maximum)
      {
      return 0;
      }

    return this->Internal->FontHeight;
    }

  return 0;
}

int pqChartAxis::getMaxLabelWidth() const
{
  if(this->Options->isVisible() && this->Options->areLabelsVisible())
    {
    return this->Internal->MaxLabelWidth;
    }

  return 0;
}

void pqChartAxis::drawAxis(QPainter &painter, const QRect &area) const
{
  if(!painter.isActive() || !area.isValid() || !this->Model ||
      !this->Options->isVisible())
    {
    return;
    }

  // If the model is empty, there's nothing to paint.
  if(this->Model->getNumberOfLabels() == 0)
    {
    return;
    }

  // Make sure the repaint area overlaps the axis area.
  if(this->Location == pqChartAxis::Left)
    {
    if(area.left() > this->Internal->Bounds.right())
      {
      return;
      }
    }
  else if(this->Location == pqChartAxis::Top)
    {
    if(area.top() > this->Internal->Bounds.bottom())
      {
      return;
      }
    }
  else if(this->Location == pqChartAxis::Right)
    {
    if(area.right() < this->Internal->Bounds.left())
      {
      return;
      }
    }
  else if(area.bottom() < this->Internal->Bounds.top())
    {
    return;
    }

  // Draw the axis line.
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(this->Options->getAxisColor());
  if(this->Location == pqChartAxis::Left)
    {
    painter.drawLine(this->Internal->Bounds.topRight(),
        this->Internal->Bounds.bottomRight());
    }
  else if(this->Location == pqChartAxis::Top)
    {
    painter.drawLine(this->Internal->Bounds.bottomLeft(),
        this->Internal->Bounds.bottomRight());
    }
  else if(this->Location == pqChartAxis::Right)
    {
    painter.drawLine(this->Internal->Bounds.topLeft(),
        this->Internal->Bounds.bottomLeft());
    }
  else
    {
    painter.drawLine(this->Internal->Bounds.topLeft(),
        this->Internal->Bounds.topRight());
    }

  // Only draw the labels if they are visible.
  if(!this->Options->areLabelsVisible() || this->Internal->SpaceTooSmall)
    {
    painter.restore();
    return;
    }

  // Set up the constant values based on the axis location.
  float x = 0;
  float y = 0;
  float tick = 0;
  float tickSmall = 0;
  if(this->Location == pqChartAxis::Left)
    {
    x = this->Internal->Bounds.right();
    tick = x - this->Internal->TickLength;
    tickSmall = x - this->Internal->SmallTickLength;
    }
  else if(this->Location == pqChartAxis::Top)
    {
    y = this->Internal->Bounds.bottom();
    tick = y - this->Internal->TickLength;
    tickSmall = y - this->Internal->SmallTickLength;
    }
  else if(this->Location == pqChartAxis::Right)
    {
    x = this->Internal->Bounds.left();
    tick = x + this->Internal->TickLength;
    tickSmall = x + this->Internal->SmallTickLength;
    }
  else
    {
    y = this->Internal->Bounds.top();
    tick = y + this->Internal->TickLength;
    tickSmall = y + this->Internal->SmallTickLength;
    }

  QFontMetrics fm(this->Options->getLabelFont());
  int fontAscent = fm.ascent();
  int halfAscent = fontAscent/2;
  int fontDescent = fm.descent();

  bool vertical = this->Location == pqChartAxis::Left ||
      this->Location == pqChartAxis::Right;

  // Draw the axis labels.
  int i = 0;
  int skipIndex = 0;
  QString label;
  pqChartValue value;
  QList<pqChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  painter.setFont(this->Options->getLabelFont());
  for( ; iter != this->Internal->Items.end(); ++iter, ++i)
    {
    if(vertical)
      {
      // Transform the contents coordinate to bounds space.
      y = (*iter)->Pixel;
      if(this->Zoom)
        {
        y -= this->Zoom->getYOffset();
        }

      // Make sure the label is inside the axis bounds.
      if((int)y > this->Internal->Bounds.bottom())
        {
        continue;
        }
      else if((int)y < this->Internal->Bounds.top())
        {
        break;
        }

      // Draw the tick mark for the label. If the label won't fit,
      // draw a smaller tick mark.
      skipIndex = i % this->Internal->Skip;
      if(this->Internal->Skip == 1 || skipIndex == 0)
        {
        painter.setPen(this->Options->getAxisColor());
        painter.drawLine(QPointF(tick, y), QPointF(x, y));
        this->Model->getLabel(i, value);
        label = this->getLabel(value);
        painter.setPen(this->Options->getLabelColor());
        y += halfAscent;
        if(this->Location == pqChartAxis::Left)
          {
          painter.drawText(QPointF(tick - (*iter)->Width -
              this->Internal->TickLabelSpacing, y), label);
          }
        else
          {
          painter.drawText(
              QPointF(tick + this->Internal->TickLabelSpacing, y), label);
          }
        }
      else if(this->Internal->TickSkip == 1 ||
          skipIndex % this->Internal->TickSkip == 0)
        {
        painter.setPen(this->Options->getAxisColor());
        painter.drawLine(QPointF(tickSmall, y), QPointF(x, y));
        }
      }
    else
      {
      // Transform the contents coordinate to bounds space.
      x = (*iter)->Pixel;
      if(this->Zoom)
        {
        x -= this->Zoom->getXOffset();
        }

      // Make sure the label is inside the axis bounds.
      if((int)x < this->Internal->Bounds.left())
        {
        continue;
        }
      else if((int)x > this->Internal->Bounds.right())
        {
        break;
        }

      // Draw the tick mark for the label. If the label won't fit,
      // draw a smaller tick mark.
      skipIndex = i % this->Internal->Skip;
      if(this->Internal->Skip == 1 || skipIndex == 0)
        {
        painter.setPen(this->Options->getAxisColor());
        painter.drawLine(QPointF(x, tick), QPointF(x, y));
        this->Model->getLabel(i, value);
        label = this->getLabel(value);
        painter.setPen(this->Options->getLabelColor());
        x -= (*iter)->Width / 2;
        if(this->Location == pqChartAxis::Top)
          {
          painter.drawText(QPointF(x,
              tick - this->Internal->TickLabelSpacing - fontDescent), label);
          }
        else
          {
          painter.drawText(QPointF(x,
              tick + this->Internal->TickLabelSpacing + fontAscent), label);
          }
        }
      else if(this->Internal->TickSkip == 1 ||
          skipIndex % this->Internal->TickSkip == 0)
        {
        painter.setPen(this->Options->getAxisColor());
        painter.drawLine(QPointF(x, tickSmall), QPointF(x, y));
        }
      }
    }

  painter.restore();
}

void pqChartAxis::getBounds(QRect &bounds) const
{
  bounds = this->Internal->Bounds;
}

const pqChartPixelScale *pqChartAxis::getPixelValueScale() const
{
  return this->Scale;
}

bool pqChartAxis::isLabelTickVisible(int index) const
{
  if(index >= 0 || index < this->Internal->Items.size())
    {
    if(this->Internal->TickSkip > 1)
      {
      int skipIndex = index % this->Internal->Skip;
      return skipIndex % this->Internal->TickSkip == 0;
      }

    return true;
    }

  return false;
}

float pqChartAxis::getLabelLocation(int index) const
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    float pixel = this->Internal->Items[index]->Pixel;
    if(this->Zoom)
      {
      if(this->Location == pqChartAxis::Top ||
          this->Location == pqChartAxis::Bottom)
        {
        pixel -= this->Zoom->getXOffset();
        }
      else
        {
        pixel -= this->Zoom->getYOffset();
        }
      }

    return pixel;
    }

  return 0;
}

void pqChartAxis::reset()
{
  // Clean up the current view data.
  QList<pqChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    delete *iter;
    }

  this->Internal->Items.clear();
  this->Internal->MaxLabelWidth = 0;
  if(this->Model)
    {
    // Query the model for the new list of labels.
    int total = this->Model->getNumberOfLabels();
    for(int i = 0; i < total; i++)
      {
      this->Internal->Items.append(new pqChartAxisItem());
      }
    }

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

void pqChartAxis::handleFontChange()
{
  // Set the font height and tick-label spacing.
  QFontMetrics fm(this->Options->getLabelFont());
  this->Internal->FontHeight = fm.height();
  if(this->Location == pqChartAxis::Top ||
      this->Location == pqChartAxis::Bottom)
    {
    this->Internal->TickLabelSpacing = fm.leading();
    }
  else
    {
    this->Internal->TickLabelSpacing = fm.width(" ");
    }

  // clear the label width cache.
  this->clearLabelWidthCache();
}

void pqChartAxis::clearLabelWidthCache()
{
  // Clean up the current cache of label widths.
  this->Internal->MaxLabelWidth = 0;
  QList<pqChartAxisItem *>::Iterator iter = this->Internal->Items.begin();
  for( ; iter != this->Internal->Items.end(); ++iter)
    {
    (*iter)->Width = 0;
    }

  // Request a re-layout.
  emit this->layoutNeeded();
}

void pqChartAxis::insertLabel(int index)
{
  if(index < 0)
    {
    qDebug() << "Chart axis label inserted at index less than zero.";
    return;
    }

  if(index < this->Internal->Items.size())
    {
    this->Internal->Items.insert(index, new pqChartAxisItem());
    }
  else
    {
    this->Internal->Items.append(new pqChartAxisItem());
    }

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

void pqChartAxis::startLabelRemoval(int index)
{
  if(index >= 0 && index < this->Internal->Items.size())
    {
    delete this->Internal->Items.takeAt(index);
    }
}

void pqChartAxis::finishLabelRemoval(int)
{
  // Reset the max width.
  this->Internal->MaxLabelWidth = 0;

  // Request a re-layout.
  if(!this->Internal->InLayout)
    {
    emit this->layoutNeeded();
    }
}

int pqChartAxis::getLabelWidthGuess() const
{
  if(this->Internal->Maximum == this->Internal->Minimum)
    {
    return 0;
    }

  // If the axis uses logarithmic scale on integer values, the
  // values can be converted to floats.
  int length1 = 0;
  int length2 = 0;
  if(this->Scale->getScaleType() == pqChartPixelScale::Logarithmic &&
      this->Internal->Minimum.getType() == pqChartValue::IntValue)
    {
    pqChartValue value = this->Internal->Maximum;
    value.convertTo(pqChartValue::FloatValue);
    length1 = this->getLabel(value).length();
    value = this->Internal->Minimum;
    value.convertTo(pqChartValue::FloatValue);
    length2 = this->getLabel(value).length();
    }
  else
    {
    length1 = this->getLabel(this->Internal->Maximum).length();
    length2 = this->getLabel(this->Internal->Minimum).length();
    }

  if(length2 > length1)
    {
    length1 = length2;
    }

  // Use a string of '8's to determine the maximum font width
  // in case the font is not fixed-pitch.
  QFontMetrics fm(this->Options->getLabelFont());
  QString label;
  label.fill('8', length1);
  return fm.width(label);
}

void pqChartAxis::generateLabels(const QRect &contents)
{
  if(!this->Internal->UsingBestFit || !this->Model)
    {
    return;
    }

  // Clear the current labels from the model.
  this->Model->startModifyingData();
  this->Model->removeAllLabels();
  if(this->Internal->Minimum != this->Internal->Maximum)
    {
    // Find the number of labels that will fit in the contents.
    int allowed = 0;
    if(this->Location == pqChartAxis::Top ||
        this->Location == pqChartAxis::Bottom)
      {
      // The contents width doesn't account for the label width, the
      // neighbor width, or the label width from the axis parallel to
      // this one.
      QRect neighbor;
      int labelWidth = this->getLabelWidthGuess();
      int halfWidth = labelWidth / 2;
      if(this->Across && !this->Across->isSpaceTooSmall())
        {
        int otherWidth = this->Across->getMaxLabelWidth() / 2;
        if(otherWidth > halfWidth)
          {
          halfWidth = otherWidth;
          }
        }

      int total = contents.width();
      int space = halfWidth;
      if(this->AtMin && !this->AtMin->isSpaceTooSmall())
        {
        this->AtMin->getBounds(neighbor);
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < halfWidth)
          {
          space = halfWidth;
          }
        }

      total -= space;
      space = halfWidth;
      if(this->AtMax && !this->AtMax->isSpaceTooSmall())
        {
        this->AtMax->getBounds(neighbor);
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < halfWidth)
          {
          space = halfWidth;
          }
        }

      total -= space;
      allowed = total / (labelWidth + this->Internal->FontHeight);
      }
    else
      {
      allowed = contents.height() / (2 * this->Internal->FontHeight);
      }

    if(allowed > 1)
      {
      // Find the value range. Convert integers to floating point
      // values to compare with the interval list.
      pqChartValue range = this->Internal->Maximum - this->Internal->Minimum;
      if(range.getType() == pqChartValue::IntValue)
        {
        range.convertTo(pqChartValue::FloatValue);
        }

      // Convert the value interval to exponent format for comparison.
      // Save the exponent for re-application.
      range /= allowed;
      QString rangeString;
      if(range.getType() == pqChartValue::DoubleValue)
        {
        rangeString.setNum(range.getDoubleValue(), 'e', 1);
        }
      else
        {
        rangeString.setNum(range.getFloatValue(), 'e', 1);
        }

      int exponent = 0;
      int index = rangeString.indexOf("e");
      if(index != -1)
        {
        exponent = rangeString.right(rangeString.length() - index - 1).toInt();
        rangeString.truncate((unsigned int)index);
        }

      // Set the new value for the range, which excludes exponent.
      range.setValue(rangeString.toFloat());

      // Search through the interval list for the closest one.
      // Convert the negative interval to match the positive
      // list values. Make sure the interval is not too small
      // for the chart label precision.
      bool negative = range < 0.0;
      if(negative)
        {
        range *= -1;
        }

      bool found = false;
      int minExponent = -this->Options->getPrecision();
      if(this->Internal->Maximum.getType() == pqChartValue::IntValue)
        {
        minExponent = 0;
        }

      // FIX: If the range is very small (exponent<0), we want to use
      // more intervals, not fewer.
      if(exponent < minExponent && exponent > 0)
        {
        found = true;
        range = IntervalList[0];
        exponent = minExponent;
        }
      else
        {
        int i = 0;
        for( ; i < IntervalListLength; i++)
          {
          // Skip 2.5 if the precision is reached.
          if(exponent == minExponent && i == 2)
            {
            continue;
            }
          if(range <= IntervalList[i])
            {
            range = IntervalList[i];
            found = true;
            break;
            }
          }
        }

      if(!found)
        {
        range = IntervalList[0];
        exponent++;
        }

      if(negative)
        {
        range *= -1;
        }

      // After finding a suitable interval, convert it back to
      // a usable form.
      rangeString.setNum(range.getFloatValue(), 'f', 1);
      QString expString;
      expString.setNum(exponent);
      rangeString.append("e").append(expString);
      if(this->Internal->Maximum.getType() == pqChartValue::DoubleValue)
        {
        range.setValue(rangeString.toDouble());
        }
      else
        {
        range.setValue(rangeString.toFloat());
        }

      // Assign the pixel interval from the calculated value interval.
      pqChartValue interval;
      if(this->Internal->Maximum.getType() == pqChartValue::IntValue)
        {
        interval.setValue(range.getIntValue());
        if(interval == 0)
          {
          interval = this->Internal->Maximum - this->Internal->Minimum;
          }
        }
      else
        {
        interval = range;
        }

      // Adjust the displayed min/max to align to the interval.
      pqChartValue minimum = this->Internal->Minimum;
      if(this->Internal->Minimum != 0)
        {
        int numIntervals = this->Internal->Minimum / interval;
        minimum = interval * numIntervals;
        if(minimum > this->Internal->Minimum)
          {
          minimum -= interval;
          }
        else if(this->Internal->ExtraMinPadding &&
            minimum == this->Internal->Minimum)
          {
          minimum -= interval;
          }
        }

      pqChartValue maximum = this->Internal->Maximum;
      if(this->Internal->Maximum != 0)
        {
        int numIntervals = this->Internal->Maximum / interval;
        maximum = interval * numIntervals;
        if(maximum < this->Internal->Maximum)
          {
          maximum += interval;
          }
        else if(this->Internal->ExtraMaxPadding &&
            maximum == this->Internal->Maximum)
          {
          maximum += interval;
          }
        }

      // Fill in the data based on the interval.
      maximum += interval/2; // Account for round-off error.
      for( ; minimum < maximum; minimum += interval)
        {
        this->Model->addLabel(minimum);
        }

      // Adding half the interval misses the last value when the interval
      // is an integer of 1.
      if(interval.getType() == pqChartValue::IntValue && interval == 1)
        {
        this->Model->addLabel(minimum);
        }
      }
    else
      {
      this->Model->addLabel(this->Internal->Minimum);
      this->Model->addLabel(this->Internal->Maximum);
      }
    }
  else if(this->Internal->DataAvailable)
    {
    // The best fit range is zero, but there is data available. Use a
    // small interval to place labels around the data.
    this->Model->addLabel(this->Internal->Minimum - 1);
    this->Model->addLabel(this->Internal->Minimum);
    this->Model->addLabel(this->Internal->Minimum + 1);
    }

  this->Model->finishModifyingData();
}

void pqChartAxis::generateLogLabels(const QRect &contents)
{
  if(!this->Internal->UsingBestFit || !this->Model)
    {
    return;
    }

  // Make sure the range is valid for log scale.
  if(!pqChartPixelScale::isLogScaleValid(this->Internal->Minimum,
        this->Internal->Maximum))
    {
    this->generateLabels(contents);
    return;
    }

  // Clear the current labels from the model.
  this->Model->startModifyingData();
  this->Model->removeAllLabels();
  if(this->Internal->Minimum != this->Internal->Maximum)
    {
    int needed = 0;
    int pixelRange = 0;
    if(this->Location == pqChartAxis::Top ||
        this->Location == pqChartAxis::Bottom)
      {
      int labelWidth = this->getLabelWidthGuess();
      needed = labelWidth + this->Internal->FontHeight;

      // The contents width doesn't account for the label width or the
      // neighbor width.
      QRect neighbor;
      pixelRange = contents.width();
      int space = labelWidth;
      if(this->AtMin && !this->AtMin->isSpaceTooSmall())
        {
        this->AtMin->getBounds(neighbor);
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < labelWidth)
          {
          space = labelWidth;
          }
        }

      pixelRange -= space;
      space = labelWidth;
      if(this->AtMax && !this->AtMax->isSpaceTooSmall())
        {
        this->AtMax->getBounds(neighbor);
        space = neighbor.isValid() ? neighbor.width() : 0;
        if(space < labelWidth)
          {
          space = labelWidth;
          }
        }

      pixelRange -= space;
      }
    else
      {
      needed = 2 * this->Internal->FontHeight;
      pixelRange = contents.height();
      }

    // Adjust the min/max to a power of ten.
    int maxExp = -1;
    int minExp = -1;
    double logValue = 0.0;
    if(!(this->Internal->Maximum.getType() == pqChartValue::IntValue &&
        this->Internal->Maximum == 0))
      {
      logValue = log10(this->Internal->Maximum.getDoubleValue());
      maxExp = (int)logValue;
      if(this->Internal->Maximum > this->Internal->Minimum &&
          logValue > (double)maxExp)
        {
        maxExp++;
        }
      }

    if(!(this->Internal->Minimum.getType() == pqChartValue::IntValue &&
        this->Internal->Minimum == 0))
      {
      logValue = log10(this->Internal->Minimum.getDoubleValue());

      // The log10 result can be off for certain values so adjust
      // the result to get a better integer exponent.
      if(logValue < 0.0)
        {
        logValue -= this->Scale->MinLogValue;
        }
      else
        {
        logValue += this->Scale->MinLogValue;
        }

      minExp = (int)logValue;
      if(this->Internal->Minimum > this->Internal->Maximum &&
          logValue > (double)minExp)
        {
        minExp++;
        }
      }

    int allowed = pixelRange / needed;
    int subInterval = 0;
    int intervals = maxExp - minExp;
    pqChartValue value;
    value = pow((double)10.0, (double)minExp);
    value.convertTo(this->Internal->Minimum.getType());
    if(allowed > intervals)
      {
      // If the number of allowed tick marks is greater than the
      // exponent range, there may be space for sub-intervals.
      int remaining = allowed / intervals;
      if(remaining >= 20)
        {
        subInterval = 1;
        }
      else if(remaining >= 10)
        {
        subInterval = 2;
        }
      else if(remaining >= 3)
        {
        subInterval = 5;
        }
      }

    // Place the first value on the list using value min in case
    // the first value is int zero.
    this->Model->addLabel(value);

    // Fill in the data based on the interval.
    pqChartValue subItem;
    for(int i = 1; i <= intervals; i++)
      {
      // Add entries for the sub-intervals if there are any. Don't
      // add sub-intervals for int values less than one.
      if(subInterval > 0 && !(value.getType() == pqChartValue::IntValue &&
          value == 0))
        {
        for(int j = subInterval; j < 10; j += subInterval)
          {
          subItem = value;
          subItem *= j;
          this->Model->addLabel(subItem);
          }
        }

      value = pow((double)10.0, (double)(minExp + i));
      value.convertTo(this->Internal->Minimum.getType());
      this->Model->addLabel(value);
      }
    }
  else if(this->Internal->DataAvailable)
    {
    // The best fit range is zero, but there is data available. Find
    // the closest power of ten around the value.
    int logValue = (int)log10(this->Internal->Maximum.getDoubleValue());
    pqChartValue value = pow((double)10.0, (double)logValue);
    value.convertTo(this->Internal->Minimum.getType());
    this->Model->addLabel(value);

    logValue += 1;
    value = pow((double)10.0, (double)logValue);
    value.convertTo(this->Internal->Minimum.getType());
    this->Model->addLabel(value);
    }

  this->Model->finishModifyingData();
}

QString pqChartAxis::getLabel(const pqChartValue& value) const
{
  if (this->Formatter)
    {
    return this->Formatter->format(value,
      this->Options->getPrecision(),
      this->Options->getNotation());
    }

  return value.getString(this->Options->getPrecision(),
    this->Options->getNotation());
}
