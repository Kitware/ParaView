/*=========================================================================

   Program: ParaView
   Module:    pqChartContentsSpace.cxx

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

/// \file pqChartContentsSpace.cxx
/// \date 3/1/2007

#include "pqChartContentsSpace.h"

#include "pqChartZoomHistory.h"
#include <QPoint>
#include <QRect>


// Set a maximum zoom factor (in percentage) to prevent overflow
// problems while zooming.
#define MAX_ZOOM 1600


class pqChartContentsSpaceInternal
{
public:
  pqChartContentsSpaceInternal();
  ~pqChartContentsSpaceInternal() {}

  QRect Layer;                ///< Stores the chart layer viewport.
  pqChartZoomHistory History; ///< Stores the viewport zoom history.
  bool InHistory;             ///< Used for zoom history processing.
  bool InInteraction;         ///< Used for interactive zoom.
};


//----------------------------------------------------------------------------
pqChartContentsSpaceInternal::pqChartContentsSpaceInternal()
  : Layer(), History()
{
  this->InHistory = false;
  this->InInteraction = false;
}


//----------------------------------------------------------------------------
int pqChartContentsSpace::ZoomFactorStep = 10;
int pqChartContentsSpace::PanStep = 15;


//----------------------------------------------------------------------------
pqChartContentsSpace::pqChartContentsSpace(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqChartContentsSpaceInternal();
  this->OffsetX = 0;
  this->OffsetY = 0;
  this->MaximumX = 0;
  this->MaximumY = 0;
  this->Width = 0;
  this->Height = 0;
  this->ZoomFactorX = 100;
  this->ZoomFactorY = 100;

  // Set up the original zoom position in the history.
  this->Internal->History.addHistory(0, 0, 100, 100);
}

pqChartContentsSpace::~pqChartContentsSpace()
{
  delete this->Internal;
}

int pqChartContentsSpace::getContentsWidth() const
{
  return this->Width + this->MaximumX;
}

int pqChartContentsSpace::getContentsHeight() const
{
  return this->Height + this->MaximumY;
}

void pqChartContentsSpace::translateToContents(QPoint &point) const
{
  point.rx() += this->OffsetX;
  point.ry() += this->OffsetY;
}

void pqChartContentsSpace::translateToContents(QRect &area) const
{
  area.translate(this->OffsetX, this->OffsetY);
}

void pqChartContentsSpace::translateFromContents(QPoint &point) const
{
  point.rx() -= this->OffsetX;
  point.ry() -= this->OffsetY;
}

void pqChartContentsSpace::translateFromContents(QRect &area) const
{
  area.translate(-this->OffsetX, -this->OffsetY);
}

void pqChartContentsSpace::setChartSize(int width, int height)
{
  if(this->Width != width || this->Height != height)
    {
    // Scale the offsets for the new size.
    bool changeXOffset = this->Width != 0 && this->OffsetX != 0;
    if(changeXOffset)
      {
      this->OffsetX = (this->OffsetX * width) / this->Width;
      }

    bool changeYOffset = this->Height != 0 && this->OffsetY != 0;
    if(changeYOffset)
      {
      this->OffsetY = (this->OffsetY * height) / this->Height;
      }

    // Use the xoom factors to determine the new maximum offsets.
    bool xShrinking = width < this->Width;
    this->Width = width;
    if(this->ZoomFactorX > 100)
      {
      this->MaximumX = (this->Width * this->ZoomFactorX) / 100;
      this->MaximumX -= this->Width;
      }

    bool yShrinking = height < this->Height;
    this->Height = height;
    if(this->ZoomFactorY > 100)
      {
      this->MaximumY = (this->Height * this->ZoomFactorY) / 100;
      this->MaximumY -= this->Height;
      }

    if(xShrinking && changeXOffset)
      {
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(yShrinking && changeYOffset)
      {
      emit this->yOffsetChanged(this->OffsetY);
      }

    if(this->ZoomFactorX > 100 || this->ZoomFactorY > 100)
      {
      emit this->maximumChanged(this->MaximumX, this->MaximumY);
      }

    if(!xShrinking && changeXOffset)
      {
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(!yShrinking && changeYOffset)
      {
      emit this->yOffsetChanged(this->OffsetY);
      }
    }
}

void pqChartContentsSpace::setChartLayerBounds(const QRect &bounds)
{
  this->Internal->Layer = bounds;
}

void pqChartContentsSpace::zoomToPercent(int percent)
{
  this->zoomToPercent(percent, percent);
}

void pqChartContentsSpace::zoomToPercent(int xPercent, int yPercent)
{
  if(xPercent < 100)
    {
    xPercent = 100;
    }
  else if(xPercent > MAX_ZOOM)
    {
    xPercent = MAX_ZOOM;
    }

  if(yPercent < 100)
    {
    yPercent = 100;
    }
  else if(yPercent > MAX_ZOOM)
    {
    yPercent = MAX_ZOOM;
    }

  if(this->ZoomFactorX != xPercent || this->ZoomFactorY != yPercent)
    {
    this->ZoomFactorX = xPercent;
    this->ZoomFactorY = yPercent;
    if(this->Width != 0 || this->Height != 0)
      {
      if(!this->Internal->InHistory && !this->Internal->InInteraction)
        {
        // Add the new zoom location to the zoom history.
        this->Internal->History.addHistory(this->OffsetX, this->OffsetY,
            this->ZoomFactorX, this->ZoomFactorY);
        emit this->historyPreviousAvailabilityChanged(
            this->Internal->History.isPreviousAvailable());
        emit this->historyNextAvailabilityChanged(
            this->Internal->History.isNextAvailable());
        }

      this->MaximumX = (this->Width * this->ZoomFactorX) / 100;
      this->MaximumX -= this->Width;
      this->MaximumY = (this->Height * this->ZoomFactorY) / 100;
      this->MaximumY -= this->Height;

      // Make sure the offsets fit in the new space.
      this->setXOffset(this->OffsetX);
      this->setYOffset(this->OffsetY);

      emit this->maximumChanged(this->MaximumX, this->MaximumY);
      }
    }
}

void pqChartContentsSpace::zoomToPercentX(int percent)
{
  this->zoomToPercent(percent, this->ZoomFactorY);
}

void pqChartContentsSpace::zoomToPercentY(int percent)
{
  this->zoomToPercent(this->ZoomFactorX, percent);
}

void pqChartContentsSpace::zoomToRectangle(const QRect &area)
{
  // Make sure the rectangle is valid and the chart size has been
  // assigned.
  if(!area.isValid() || this->Width == 0 || this->Height == 0 ||
      !this->Internal->Layer.isValid())
    {
    return;
    }

  // Make sure the x,y of the rectangle are valid.
  int x = area.x();
  int y = area.y();
  if(x < 0 || y < 0)
    {
    return;
    }

  // Adjust the top-left corner coordinates for the chart layer
  // viewport and the current offset.
  x = x - this->Internal->Layer.x() + this->OffsetX;
  y = y - this->Internal->Layer.y() + this->OffsetY;

  // Find the new zoom factors using the zoom factors for the chart
  // layer viewport.
  int xZoom1 = this->Width * (this->ZoomFactorX - 100);
  xZoom1 = (xZoom1 / this->Internal->Layer.width()) + 100;
  int xZoom2 = (xZoom1 * this->Internal->Layer.width()) / area.width();
  int xFactor = this->Internal->Layer.width() * (xZoom2 - 100);
  xFactor = (xFactor / this->Width) + 100;

  int yZoom1 = this->Height * (this->ZoomFactorY - 100);
  yZoom1 = (yZoom1 / this->Internal->Layer.height()) + 100;
  int yZoom2 = (yZoom1 * this->Internal->Layer.height()) / area.height();
  int yFactor = this->Internal->Layer.height() * (yZoom2 - 100);
  yFactor = (yFactor / this->Height) + 100;

  // Set the new zoom factors.
  this->zoomToPercent(xFactor, yFactor);

  // Re-calculate the second zoom factors.
  xZoom2 = this->Width * (this->ZoomFactorX - 100);
  xZoom2 = (xZoom2 / this->Internal->Layer.width()) + 100;
  yZoom2 = this->Height * (this->ZoomFactorY - 100);
  yZoom2 = (yZoom2 / this->Internal->Layer.height()) + 100;

  // Set the offset to match the original zoom area.
  this->setXOffset((xZoom2 * x) / xZoom1);
  this->setYOffset((yZoom2 * y) / yZoom1);
}

void pqChartContentsSpace::zoomIn(pqChartContentsSpace::InteractFlags flags)
{
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartContentsSpace::ZoomXOnly)
    {
    changeInY = false;
    }
  else if(flags == pqChartContentsSpace::ZoomYOnly)
    {
    changeInX = false;
    }

  int x = this->ZoomFactorX;
  int y = this->ZoomFactorY;
  if(changeInX)
    {
    x += pqChartContentsSpace::ZoomFactorStep;
    }

  if(changeInY)
    {
    y += pqChartContentsSpace::ZoomFactorStep;
    }

  this->zoomToPercent(x, y);
}

void pqChartContentsSpace::zoomOut(pqChartContentsSpace::InteractFlags flags)
{
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartContentsSpace::ZoomXOnly)
    {
    changeInY = false;
    }
  else if(flags == pqChartContentsSpace::ZoomYOnly)
    {
    changeInX = false;
    }

  int x = this->ZoomFactorX;
  int y = this->ZoomFactorY;
  if(changeInX)
    {
    x -= pqChartContentsSpace::ZoomFactorStep;
    }

  if(changeInY)
    {
    y -= pqChartContentsSpace::ZoomFactorStep;
    }

  this->zoomToPercent(x, y);
}

void pqChartContentsSpace::startInteraction()
{
  this->Internal->InInteraction = true;
}

bool pqChartContentsSpace::isInInteraction() const
{
  return this->Internal->InInteraction;
}

void pqChartContentsSpace::finishInteraction()
{
  if(this->Internal->InInteraction)
    {
    this->Internal->InInteraction = false;

    // If the zoom factors have changed, update the history.
    const pqChartZoomViewport *current = this->Internal->History.getCurrent();
    if(!current || (current->getXZoom() != this->ZoomFactorX ||
        current->getYZoom() != this->ZoomFactorY))
      {
      this->Internal->History.addHistory(this->OffsetX, this->OffsetY,
          this->ZoomFactorX, this->ZoomFactorY);
      emit this->historyPreviousAvailabilityChanged(
          this->Internal->History.isPreviousAvailable());
      emit this->historyNextAvailabilityChanged(
          this->Internal->History.isNextAvailable());
      }
    }
}

void pqChartContentsSpace::handleWheelZoom(int delta, const QPoint &pos,
    pqChartContentsSpace::InteractFlags flags)
{
  // If the wheel event delta is positive, zoom in. Otherwise, zoom
  // out.
  int factorChange = pqChartContentsSpace::ZoomFactorStep;
  if(delta < 0)
    {
    factorChange *= -1;
    }

  int x = pos.x() + this->OffsetX;
  int y = pos.y() + this->OffsetY;
  int newXZoom = this->ZoomFactorX;
  int newYZoom = this->ZoomFactorY;
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartContentsSpace::ZoomXOnly)
    {
    changeInY = false;
    }
  else if(flags == pqChartContentsSpace::ZoomYOnly)
    {
    changeInX = false;
    }

  if(changeInX)
    {
    newXZoom += factorChange;
    if(newXZoom < 100)
      {
      newXZoom = 100;
      }
    else if(newXZoom > MAX_ZOOM)
      {
      newXZoom = MAX_ZOOM;
      }

    if(newXZoom != this->ZoomFactorX)
      {
      x = (newXZoom * x)/(this->ZoomFactorX);
      }
    }

  if(changeInY)
    {
    newYZoom += factorChange;
    if(newYZoom < 100)
      {
      newYZoom = 100;
      }
    else if(newYZoom > MAX_ZOOM)
      {
      newYZoom = MAX_ZOOM;
      }

    if(newYZoom != this->ZoomFactorY)
      {
      y = (newYZoom * y)/(this->ZoomFactorY);
      }
    }

  // Set the new zoom factor(s).
  this->zoomToPercent(newXZoom, newYZoom);

  // Keep the same position under the point if possible.
  x -= pos.x();
  this->setXOffset(x);

  y -= pos.y();
  this->setYOffset(y);
}

bool pqChartContentsSpace::isHistoryPreviousAvailable() const
{
  return this->Internal->History.isPreviousAvailable();
}

bool pqChartContentsSpace::isHistoryNextAvailable() const
{
  return this->Internal->History.isNextAvailable();
}

void pqChartContentsSpace::setXOffset(int offset)
{
  if(offset < 0)
    {
    offset = 0;
    }
  else if(offset > this->MaximumX)
    {
    offset = this->MaximumX;
    }

  if(this->OffsetX != offset)
    {
    this->OffsetX = offset;
    if(!this->Internal->InHistory)
      {
      this->Internal->History.updatePosition(this->OffsetX, this->OffsetY);
      }
      
    emit this->xOffsetChanged(this->OffsetX);
    }
}

void pqChartContentsSpace::setYOffset(int offset)
{
  if(offset < 0)
    {
    offset = 0;
    }
  else if(offset > this->MaximumY)
    {
    offset = this->MaximumY;
    }

  if(this->OffsetY != offset)
    {
    this->OffsetY = offset;
    if(!this->Internal->InHistory)
      {
      this->Internal->History.updatePosition(this->OffsetX, this->OffsetY);
      }
      
    emit this->yOffsetChanged(this->OffsetY);
    }
}

void pqChartContentsSpace::setMaximumXOffset(int maximum)
{
  if(this->MaximumX != maximum && maximum >= 0)
    {
    this->MaximumX = maximum;
    if(this->OffsetX > this->MaximumX)
      {
      this->OffsetX = this->MaximumX;
      emit this->xOffsetChanged(this->OffsetX);
      }

    if(this->Width != 0)
      {
      this->ZoomFactorX = (this->Width + this->MaximumX) * 100;
      this->ZoomFactorX /= this->Width;
      }

    emit this->maximumChanged(this->MaximumX, this->MaximumY);
    }
}

void pqChartContentsSpace::setMaximumYOffset(int maximum)
{
  if(this->MaximumY != maximum && maximum >= 0)
    {
    this->MaximumY = maximum;
    if(this->OffsetY > this->MaximumY)
      {
      this->OffsetY = this->MaximumY;
      emit this->yOffsetChanged(this->OffsetY);
      }

    if(this->Height != 0)
      {
      this->ZoomFactorY = (this->Height + this->MaximumY) * 100;
      this->ZoomFactorY /= this->Height;
      }

    emit this->maximumChanged(this->MaximumX, this->MaximumY);
    }
}

void pqChartContentsSpace::panUp()
{
  this->setYOffset(this->OffsetY - pqChartContentsSpace::PanStep);
}

void pqChartContentsSpace::panDown()
{
  this->setYOffset(this->OffsetY + pqChartContentsSpace::PanStep);
}

void pqChartContentsSpace::panLeft()
{
  this->setXOffset(this->OffsetX - pqChartContentsSpace::PanStep);
}

void pqChartContentsSpace::panRight()
{
  this->setXOffset(this->OffsetX + pqChartContentsSpace::PanStep);
}

void pqChartContentsSpace::resetZoom()
{
  this->zoomToPercent(100, 100);
}

void pqChartContentsSpace::historyNext()
{
  const pqChartZoomViewport *zoom = this->Internal->History.getNext();
  if(zoom)
    {
    this->Internal->InHistory = true;
    this->zoomToPercent(zoom->getXZoom(), zoom->getYZoom());
    this->setXOffset(zoom->getXPosition());
    this->setYOffset(zoom->getYPosition());
    this->Internal->InHistory = false;

    emit this->historyPreviousAvailabilityChanged(
        this->Internal->History.isPreviousAvailable());
    emit this->historyNextAvailabilityChanged(
        this->Internal->History.isNextAvailable());
    }
}

void pqChartContentsSpace::historyPrevious()
{
  const pqChartZoomViewport *zoom = this->Internal->History.getPrevious();
  if(zoom)
    {
    this->Internal->InHistory = true;
    this->zoomToPercent(zoom->getXZoom(), zoom->getYZoom());
    this->setXOffset(zoom->getXPosition());
    this->setYOffset(zoom->getYPosition());
    this->Internal->InHistory = false;

    emit this->historyPreviousAvailabilityChanged(
        this->Internal->History.isPreviousAvailable());
    emit this->historyNextAvailabilityChanged(
        this->Internal->History.isNextAvailable());
    }
}

int pqChartContentsSpace::getZoomFactorStep()
{
  return pqChartContentsSpace::ZoomFactorStep;
}

void pqChartContentsSpace::setZoomFactorStep(int step)
{
  pqChartContentsSpace::ZoomFactorStep = step;
}

int pqChartContentsSpace::getPanStep()
{
  return pqChartContentsSpace::PanStep;
}

void pqChartContentsSpace::setPanStep(int step)
{
  pqChartContentsSpace::PanStep = step;
}


