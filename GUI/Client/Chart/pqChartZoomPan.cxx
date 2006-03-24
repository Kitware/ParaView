/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

/*!
 * \file pqChartZoomPan.cxx
 *
 * \brief
 *   The pqChartZoomPan and class is used to handle the zoom and
 *   pan interaction for a chart widget.
 *
 * \author Mark Richardson
 * \date   August 9, 2005
 */

#include "pqChartZoomPan.h"
#include "pqChartZoomHistory.h"

#include <QAbstractScrollArea>
#include <QCursor>
#include <QPixmap>
#include <QRect>
#include <QScrollBar>


// Set a maximum zoom factor (in percentage) to prevent overflow
// problems while zooming.
#define MAX_ZOOM 1600

// Set zoom/pan factors for key and wheel events.
#define ZOOM_FACTOR 10
#define PAN_FACTOR 15

// Zoom cursor xpm.
#include "zoom.xpm"


pqChartZoomPan::pqChartZoomPan(QAbstractScrollArea *p)
  : QObject(p), Last()
{
  this->Current = pqChartZoomPan::NoMode;
  this->Parent = p;
  this->History = new pqChartZoomHistory();
  this->ContentsX = 0;
  this->ContentsY = 0;
  this->ContentsWidth = 0;
  this->ContentsHeight = 0;
  this->XZoomFactor = 100;
  this->YZoomFactor = 100;
  this->InHistory = false;
  this->InPosition = false;

  // Set up the original zoom position in the history.
  if(this->History)
    this->History->addHistory(0, 0, 100, 100);

  // Turn off the scrollbars in the scrollview. They are not needed
  // until there is a zoom factor greater than 100.
  if(this->Parent)
    {
    this->Parent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->Parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this->Parent->horizontalScrollBar(), SIGNAL(valueChanged(int)),
        this, SLOT(setContentsX(int)));
    connect(this->Parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
        this, SLOT(setContentsY(int)));
    }
}

pqChartZoomPan::~pqChartZoomPan()
{
  if(this->History)
    delete this->History;
}

void pqChartZoomPan::zoomToPercent(int percent)
{
  this->zoomToPercent(percent, percent);
}

void pqChartZoomPan::zoomToPercent(int percentX, int percentY)
{
  if(percentX < 100)
    percentX = 100;
  else if(percentX > MAX_ZOOM)
    percentX = MAX_ZOOM;
  if(percentY < 100)
    percentY = 100;
  else if(percentY > MAX_ZOOM)
    percentY = MAX_ZOOM;

  if(this->XZoomFactor != percentX || this->YZoomFactor != percentY)
    {
    this->XZoomFactor = percentX;
    this->YZoomFactor = percentY;
    if(this->History && !this->InHistory)
      {
      // Add the new zoom location to the zoom history.
      this->History->addHistory(this->ContentsX, this->ContentsY,
          this->XZoomFactor, this->YZoomFactor);
      }

    // During a shrinking resize event, the scrollbars can flash on
    // for a moment. To avoid this, turn the scrollbars off unless
    // there is a zoom factor.
    if(this->Parent)
      {
      if(this->XZoomFactor > 100)
        this->Parent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      else
        this->Parent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      if(this->YZoomFactor > 100)
        this->Parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      else
        this->Parent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      }

    this->updateContentSize();
    }
}

void pqChartZoomPan::zoomToPercentX(int percent)
{
  this->zoomToPercent(percent, this->YZoomFactor);
}

void pqChartZoomPan::zoomToPercentY(int percent)
{
  this->zoomToPercent(this->XZoomFactor, percent);
}

void pqChartZoomPan::zoomToRectangle(const QRect *area)
{
  if(!this->Parent || !area || !area->isValid())
    return;

  // Make sure the x,y of the rectangle are valid.
  int x = area->x();
  int y = area->y();
  if(x < 0 || y < 0)
    return;

  // Make sure the viewport size has been assigned.
  int vpWidth = this->Parent->viewport()->width();
  int vpHeight = this->Parent->viewport()->height();
  if(vpWidth == 0 || vpHeight == 0)
    return;

  // Adjust the width or height of the area to match the viewport
  // aspect ratio. Use the larger ratio when converting.
  int aWidth = area->width();
  int aHeight = area->height();
  int width = vpWidth > aWidth ? vpWidth/aWidth : aWidth/vpWidth;
  int height = vpHeight > aHeight ? vpHeight/aHeight : aHeight/vpHeight;
  if(height < width)
    aWidth = (vpWidth * aHeight)/vpHeight;
  else
    aHeight = (vpHeight * aWidth)/vpWidth;

  // Use the width and height ratios to determine the new zoom
  // factors. Make sure the resulting size is not too big.
  width = this->ContentsWidth;
  height = this->ContentsHeight;
  this->zoomToPercent((width * 100)/aWidth, (height * 100)/aHeight);

  // Move the viewport to show the rectangle.
  x = (this->ContentsWidth * x)/width;
  if(x < 0)
    x = 0;
  y = (this->ContentsHeight * y)/height;
  if(y < 0)
    y = 0;
  this->setContentsPos(x, y);
}

void pqChartZoomPan::zoomIn(InteractFlags flags)
{
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartZoomPan::ZoomXOnly)
    changeInY = false;
  else if(flags == pqChartZoomPan::ZoomYOnly)
    changeInX = false;

  int x = this->XZoomFactor;
  int y = this->YZoomFactor;
  if(changeInX)
    x += ZOOM_FACTOR;
  if(changeInY)
    y += ZOOM_FACTOR;
  this->zoomToPercent(x, y);
}

void pqChartZoomPan::zoomOut(InteractFlags flags)
{
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartZoomPan::ZoomXOnly)
    changeInY = false;
  else if(flags == pqChartZoomPan::ZoomYOnly)
    changeInX = false;

  int x = this->XZoomFactor;
  int y = this->YZoomFactor;
  if(changeInX)
    x -= ZOOM_FACTOR;
  if(changeInY)
    y -= ZOOM_FACTOR;
  this->zoomToPercent(x, y);
}

void pqChartZoomPan::setZoomCursor()
{
  if(this->Parent)
    this->Parent->setCursor(QCursor(QPixmap(zoom_xpm), 11, 11));
}

void pqChartZoomPan::startInteraction(InteractMode mode)
{
  if(this->Current == pqChartZoomPan::NoMode)
    {
    this->Current = mode;
    if(this->Parent)
      {
      if(this->Current == pqChartZoomPan::Zoom)
        this->setZoomCursor();
      else if(this->Current == pqChartZoomPan::Pan)
        this->Parent->setCursor(Qt::SizeAllCursor);
      }
    }
}

void pqChartZoomPan::interact(const QPoint &pos, InteractFlags flags)
{
  if(!this->Parent)
    return;

  if(this->Current == pqChartZoomPan::Zoom)
    {
    // Zoom in or out based on the mouse movement up or down.
    int delta = (this->Last.y() - pos.y())/4;
    if(delta != 0)
      {
      bool changeInX = true;
      bool changeInY = true;
      if(flags == pqChartZoomPan::ZoomXOnly)
        changeInY = false;
      else if(flags == pqChartZoomPan::ZoomYOnly)
        changeInX = false;

      int x = this->XZoomFactor;
      int y = this->YZoomFactor;
      if(changeInX)
        x += delta;
      if(changeInY)
        y += delta;
      this->InHistory = true;
      this->zoomToPercent(x, y);
      this->InHistory = false;
      this->Last = pos;
      }
    }
  else if(this->Current == pqChartZoomPan::Pan)
    {
    int x = this->Last.x() - pos.x() + this->ContentsX;
    int y = this->Last.y() - pos.y() + this->ContentsY;
    this->setContentsPos(x, y);
    this->Last = pos;
    }
}

bool pqChartZoomPan::handleWheelZoom(int delta, const QPoint &pos,
    InteractFlags flags)
{
  if(!this->Parent)
    return false;

  // If the wheel event delta is positive, zoom in. Otherwise,
  // zoom out.
  int factorChange = ZOOM_FACTOR;
  if(delta < 0)
    factorChange *= -1;

  int x = pos.x();
  int y = pos.y();
  int newXZoom = this->XZoomFactor;
  int newYZoom = this->YZoomFactor;
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartZoomPan::ZoomXOnly)
    changeInY = false;
  else if(flags == pqChartZoomPan::ZoomYOnly)
    changeInX = false;

  if(changeInX)
    {
    newXZoom += factorChange;
    if(newXZoom < 100)
      newXZoom = 100;
    else if(newXZoom > MAX_ZOOM)
      newXZoom = MAX_ZOOM;

    if(newXZoom != this->XZoomFactor)
      x = (newXZoom * x)/(this->XZoomFactor);
    }

  if(changeInY)
    {
    newYZoom += factorChange;
    if(newYZoom < 100)
      newYZoom = 100;
    else if(newYZoom > MAX_ZOOM)
      newYZoom = MAX_ZOOM;

    if(newYZoom != this->YZoomFactor)
      y = (newYZoom * y)/(this->YZoomFactor);
    }

  // Set the new zoom factor(s).
  this->zoomToPercent(newXZoom, newYZoom);

  // Center the viewport on the wheel event position.
  x = x - (this->Parent->viewport()->width()/2);
  if(x < 0)
    x = 0;
  else if(x > this->ContentsWidth)
    x = this->ContentsWidth;

  y = y - (this->Parent->viewport()->height()/2);
  if(y < 0)
    y = 0;
  else if(y > this->ContentsHeight)
    y = this->ContentsHeight;

  this->setContentsPos(x, y);
  return true;
}

void pqChartZoomPan::finishInteraction()
{
  if(this->Current == pqChartZoomPan::Zoom)
    {
    this->Current = pqChartZoomPan::NoMode;
    if(this->Parent)
      {
      this->Parent->setCursor(Qt::ArrowCursor);
      if(this->History)
        {
        this->History->addHistory(this->ContentsX, this->ContentsY,
          this->XZoomFactor, this->YZoomFactor);
        }
      }
    }
  else if(this->Current == pqChartZoomPan::Pan)
    {
    this->Current = pqChartZoomPan::NoMode;
    if(this->Parent)
      this->Parent->setCursor(Qt::ArrowCursor);
    }
}

void pqChartZoomPan::setContentsX(int x)
{
  this->setContentsPos(x, this->ContentsY);
}

void pqChartZoomPan::setContentsY(int y)
{
  this->setContentsPos(this->ContentsX, y);
}

void pqChartZoomPan::setContentsPos(int x, int y)
{
  if(!this->Parent || this->InPosition)
    return;

  if(this->ContentsX == x && this->ContentsY == y)
    return;

  // Update the scrollbar positions.
  this->InPosition = true;
  QScrollBar *scroll = this->Parent->horizontalScrollBar();
  scroll->setValue(x);
  this->ContentsX = scroll->value();
  scroll = this->Parent->verticalScrollBar();
  scroll->setValue(y);
  this->ContentsY = scroll->value();
  this->InPosition = false;

  // Make sure the current history is up to date.
  if(this->History && !this->InHistory)
    this->History->updatePosition(this->ContentsX, this->ContentsY);

  // Repaint the viewport for the scrolling changes.
  this->Parent->viewport()->update();
}

void pqChartZoomPan::updateContentSize()
{
  if(!this->Parent)
    return;

  // Don't calculate the content size if the viewport size is invalid.
  int vpwidth = this->Parent->viewport()->width();
  int vpheight = this->Parent->viewport()->height();
  if(vpwidth <= 0 || vpheight <= 0)
    return;

  int width = (vpwidth * this->XZoomFactor)/100;
  int height = (vpheight * this->YZoomFactor)/100;
  if(width != this->ContentsWidth || height != this->ContentsHeight)
    {
    // Let the chart know that the content size is changing. Some
    // charts need to layout with the new size before being painted.
    emit this->contentsSizeChanging(width, height);

    this->ContentsWidth = width;
    this->ContentsHeight = height;

    // Adjust the scrollbars for the new content size.
    this->InPosition = true;
    QScrollBar *scroll = this->Parent->horizontalScrollBar();
    scroll->setMinimum(0);
    scroll->setMaximum(this->ContentsWidth - vpwidth);
    int x = scroll->value();
    scroll = this->Parent->verticalScrollBar();
    scroll->setMinimum(0);
    scroll->setMaximum(this->ContentsHeight - vpheight);
    int y = scroll->value();
    this->InPosition = false;

    // Adjusting the scroll bars might change the position. setting
    // the content position will initiate a repaint. If the position
    // hasn't changed, initiate the update for the size changes.
    if(x != this->ContentsX || y != this->ContentsY)
      this->setContentsPos(x, y);
    else
      this->Parent->viewport()->update();

    // Let interrested objects know the content size changed.
    emit this->contentsSizeChanged(width, height);
    }
}

void pqChartZoomPan::panUp()
{
  this->setContentsPos(this->ContentsX, this->ContentsY - PAN_FACTOR);
}

void pqChartZoomPan::panDown()
{
  this->setContentsPos(this->ContentsX, this->ContentsY + PAN_FACTOR);
}

void pqChartZoomPan::panLeft()
{
  this->setContentsPos(this->ContentsX - PAN_FACTOR, this->ContentsY);
}

void pqChartZoomPan::panRight()
{
  this->setContentsPos(this->ContentsX + PAN_FACTOR, this->ContentsY);
}

void pqChartZoomPan::resetZoom()
{
  this->zoomToPercent(100, 100);
}

void pqChartZoomPan::historyNext()
{
  if(!this->History)
    return;

  const pqChartZoomItem *zoom = this->History->getNext();
  if(zoom)
    {
    this->InHistory = true;
    this->zoomToPercent(zoom->getXZoom(), zoom->getYZoom());
    this->setContentsPos(zoom->getXPosition(), zoom->getYPosition());
    this->InHistory = false;
    }
}

void pqChartZoomPan::historyPrevious()
{
  if(!this->History)
    return;

  const pqChartZoomItem *zoom = this->History->getPrevious();
  if(zoom)
    {
    this->InHistory = true;
    this->zoomToPercent(zoom->getXZoom(), zoom->getYZoom());
    this->setContentsPos(zoom->getXPosition(), zoom->getYPosition());
    this->InHistory = false;
    }
}


