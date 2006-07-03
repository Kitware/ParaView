/*=========================================================================

   Program: ParaView
   Module:    pqChartZoomPanAlt.cxx

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

/*!
 * \file pqChartZoomPanAlt.cxx
 *
 * \brief
 *   An alternative to pqChartZoomPan.  
 *   Used to handle the zoom and
 *   pan interaction for a chart widget by manipulating properties of axes.
 *
 * \author Eric Stanton 
 * \date   June 23, 2006
 */

#include "pqChartZoomPanAlt.h"

#include "pqChartAxis.h"
#include "pqLineChartWidget.h"
#include <QCursor>
#include <QPixmap>
#include <QRect>

// Set zoom/pan factors for key and wheel events.
#define ZOOM_FACTOR_ALT 20.0
#define PAN_FACTOR_ALT 10.0

// Zoom cursor xpm.
#include "zoom.xpm"


pqChartZoomPanAlt::pqChartZoomPanAlt(pqLineChartWidget *p)
  : Last(), Parent(p)
{
  this->Current = pqChartZoomPanAlt::NoMode;
}

pqChartZoomPanAlt::~pqChartZoomPanAlt()
{
}

void pqChartZoomPanAlt::zoomToRectangle(const QRect * /*area*/)
{
/*
  if(!this->Parent || !area || !area->isValid())
    return;

  // Make sure the x,y of the rectangle are valid.
  int x = area->x();
  int y = area->y();
  if(x < 0 || y < 0)
    return;

  // adjust the min/max values of the axes to align with those of the rectangle bounds
  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartAxis &yAxis = this->Parent->getYAxis();
  double newMinX,newMaxX,newMinY,newMaxY;
  QPoint upperLeft = this->Parent->viewport()->mapToGlobal(QPoint(x,y));
  QPoint lowerRight = QPoint(upperLeft.x()+area->width(),upperLeft.y()+area->height());
//this->Parent->mapToGlobal(QPoint(x+area->width(),y+area->height()));

cout << xAxis.getMinPixel() << " " << xAxis.getMaxPixel() << endl;
cout << yAxis.getMinPixel() << " " << yAxis.getMaxPixel() << endl;
  //newMinX = xAxis.getValueMin() + xAxis.getValueRange()*(x-xAxis.getPixelMin())/xAxis.getPixelRange();

  newMinY = yAxis.getValueFor(lowerRight.y()); 
  newMaxY = yAxis.getValueFor(upperLeft.y()); 
  newMinX = xAxis.getValueFor(upperLeft.x()); 
  newMaxX = xAxis.getValueFor(lowerRight.x()); 

cout << x << " " << upperLeft.x() << " " << newMinX << endl;
cout << x+area->width() << " " << lowerRight.x() << " " << newMaxX << endl;
cout << y+area->height() << " " << lowerRight.y() << " " << newMinY << endl;
cout << y << " " << upperLeft.y() << " " << newMaxY << endl;
  xAxis.setValueRange(newMinX,newMaxX);
  yAxis.setValueRange(newMinY,newMaxY);
*/   
}

void pqChartZoomPanAlt::setAxesBounds(double deltaMinX, double deltaMaxX, double deltaMinY, double deltaMaxY)
{

  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartAxis &yAxis = this->Parent->getYAxis();

  xAxis.setMinValue(xAxis.getTrueMinValue() + deltaMinX);
  xAxis.setMaxValue(xAxis.getTrueMaxValue() + deltaMaxX);
  yAxis.setMinValue(yAxis.getTrueMinValue() + deltaMinY);
  yAxis.setMaxValue(yAxis.getTrueMaxValue() + deltaMaxY);
}

void pqChartZoomPanAlt::panUp()
{
  pqChartAxis &yAxis = this->Parent->getYAxis();
  pqChartValue yRange,yNumInts,yInc;
  yRange = yAxis.getValueRange();
  yNumInts = yAxis.getNumberOfIntervals();
  yInc = yRange.getFloatValue()/PAN_FACTOR_ALT;

  this->setAxesBounds(0,0,yInc,yInc);
}

void pqChartZoomPanAlt::panDown()
{
  pqChartAxis &yAxis = this->Parent->getYAxis();
  pqChartValue yRange,yNumInts,yInc;
  yRange = yAxis.getValueRange();
  yNumInts = yAxis.getNumberOfIntervals();
  yInc = yRange.getFloatValue()/PAN_FACTOR_ALT;

  this->setAxesBounds(0,0,-1*yInc,-1*yInc);
}

void pqChartZoomPanAlt::panLeft()
{
  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartValue xRange,xNumInts,xInc;
  xRange = xAxis.getValueRange();
  xNumInts = xAxis.getNumberOfIntervals();
  xInc = xRange.getFloatValue()/PAN_FACTOR_ALT;

  this->setAxesBounds(-1*xInc,-1*xInc,0,0);
}

void pqChartZoomPanAlt::panRight()
{
  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartValue xRange,xNumInts,xInc;
  xRange = xAxis.getValueRange();
  xNumInts = xAxis.getNumberOfIntervals();
  xInc = xRange.getFloatValue()/PAN_FACTOR_ALT;

  this->setAxesBounds(xInc,xInc,0,0);
}

void pqChartZoomPanAlt::zoomIn(InteractFlags flags)
{
  double xRange,xInc,yRange,yInc;
  double factor = -1.0;
  xInc = 0;
  yInc = 0;
  if(flags==pqChartZoomPanAlt::ZoomXOnly || flags==pqChartZoomPanAlt::ZoomBoth)
    {
    pqChartAxis &xAxis = this->Parent->getXAxis();
    xRange = xAxis.getValueRange();
    xInc = xRange/ZOOM_FACTOR_ALT;
    }
  if(flags==pqChartZoomPanAlt::ZoomYOnly || flags==pqChartZoomPanAlt::ZoomBoth)
    {
    pqChartAxis &yAxis = this->Parent->getYAxis();
    yRange = yAxis.getValueRange();
    yInc = yRange/ZOOM_FACTOR_ALT;
    }

  this->setAxesBounds(xInc,xInc*factor,yInc,yInc*factor);
}

void pqChartZoomPanAlt::zoomOut(InteractFlags flags)
{
  double xRange,xInc;
  double yRange,yInc;
  double factor = -1.0;
  xInc = 0;
  yInc = 0;
  if(flags==pqChartZoomPanAlt::ZoomXOnly || flags==pqChartZoomPanAlt::ZoomBoth)
    {
    pqChartAxis &xAxis = this->Parent->getXAxis();
    xRange = xAxis.getValueRange();
    xInc = xRange/ZOOM_FACTOR_ALT;
    }
  if(flags==pqChartZoomPanAlt::ZoomYOnly || flags==pqChartZoomPanAlt::ZoomBoth)
    {
    pqChartAxis &yAxis = this->Parent->getYAxis();
    yRange = yAxis.getValueRange();
    yInc = yRange/ZOOM_FACTOR_ALT;
    }

  this->setAxesBounds(xInc*factor,xInc,yInc*factor,yInc);
}

void pqChartZoomPanAlt::setZoomCursor()
{
  if(this->Parent)
    this->Parent->setCursor(QCursor(QPixmap(zoom_xpm), 11, 11));
}

void pqChartZoomPanAlt::startInteraction(InteractMode mode)
{
  if(this->Current == pqChartZoomPanAlt::NoMode)
    {
    this->Current = mode;
    if(this->Parent)
      {
      if(this->Current == pqChartZoomPanAlt::Zoom)
        this->setZoomCursor();
      else if(this->Current == pqChartZoomPanAlt::Pan)
        this->Parent->setCursor(Qt::SizeAllCursor);
      }
    }

  // We want to fix the number of intervals on the axes
  // during the interaction, so we store the current layout types
  // so we can restore them afterwards
  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartAxis &yAxis = this->Parent->getYAxis();
  this->oldXAxisLayout = xAxis.getLayoutType(); 
  this->oldYAxisLayout = yAxis.getLayoutType(); 
  xAxis.setLayoutType(pqChartAxis::FixedInterval);
  yAxis.setLayoutType(pqChartAxis::FixedInterval);
}

void pqChartZoomPanAlt::interact(const QPoint &pos, InteractFlags flags)
{
  if(!this->Parent)
    return;

  if(flags == pqChartZoomPanAlt::NoFlags)
    {
    // Convert the distance the mouse has been moved from display coords to axis units
    double deltaY = this->Parent->getYAxis().getValueRange()*(pos.y()-this->Last.y())/this->Parent->viewport()->height();
    double deltaX = this->Parent->getXAxis().getValueRange()*(this->Last.x()-pos.x())/this->Parent->viewport()->width();
    this->setAxesBounds(deltaX,deltaX,deltaY,deltaY);
    }
  else
    {
    // Zoom in or out based on the mouse movement up or down.
    if(this->Last.y()<pos.y())
      {
      this->zoomOut(flags);
      }
    else if(this->Last.y()>pos.y())
      {
      this->zoomIn(flags);
      }
    }
    
  this->Last = pos;
}

bool pqChartZoomPanAlt::handleWheelZoom(int /*delta*/, const QPoint & /*pos*/,
    InteractFlags /*flags*/)
{
/*
  if(!this->Parent)
    return false;

  // If the wheel event delta is positive, zoom in. Otherwise,
  // zoom out.
  int factorChange = ZOOM_FACTOR_ALT;
  if(delta < 0)
    factorChange *= -1;

  int x = pos.x();
  int y = pos.y();
  int newXZoom = this->XZoomFactor;
  int newYZoom = this->YZoomFactor;
  bool changeInX = true;
  bool changeInY = true;
  if(flags == pqChartZoomPanAlt::ZoomXOnly)
    changeInY = false;
  else if(flags == pqChartZoomPanAlt::ZoomYOnly)
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
*/

  return false;
}

void pqChartZoomPanAlt::finishInteraction()
{
  if(this->Current == pqChartZoomPanAlt::Zoom)
    {
    this->Current = pqChartZoomPanAlt::NoMode;
    if(this->Parent)
      {
      this->Parent->setCursor(Qt::ArrowCursor);
      }
    }
  else if(this->Current == pqChartZoomPanAlt::Pan)
    {
    this->Current = pqChartZoomPanAlt::NoMode;
    if(this->Parent)
      this->Parent->setCursor(Qt::ArrowCursor);
    }

  // Restore the axes layout types to what they were before the interaction began
  pqChartAxis &xAxis = this->Parent->getXAxis();
  pqChartAxis &yAxis = this->Parent->getYAxis();
  xAxis.setLayoutType(this->oldXAxisLayout);
  yAxis.setLayoutType(this->oldYAxisLayout);
}

