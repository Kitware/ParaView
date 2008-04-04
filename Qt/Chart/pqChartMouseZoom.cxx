/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseZoom.cxx

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

/// \file pqChartMouseZoom.cxx
/// \date 6/25/2007

#include "pqChartMouseZoom.h"

#include "pqChartContentsSpace.h"
#include "pqChartMouseBox.h"

#include <QCursor>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>
#include <QRect>

// TODO: Use Qt resource instead of including directly.
// Zoom cursor xpm.
#include "zoom.xpm"


class pqChartMouseZoomInternal
{
public:
  pqChartMouseZoomInternal();
  ~pqChartMouseZoomInternal() {}

  QCursor ZoomCursor;
  QPoint Last;
  bool LastSet;
};


//----------------------------------------------------------------------------
pqChartMouseZoomInternal::pqChartMouseZoomInternal()
  : ZoomCursor(QPixmap(zoom_xpm), 11, 11), Last()
{
  this->LastSet = false;
}


//----------------------------------------------------------------------------
pqChartMouseZoom::pqChartMouseZoom(QObject *parentObject)
  : pqChartMouseFunction(parentObject)
{
  this->Internal = new pqChartMouseZoomInternal();
  this->Flags = pqChartMouseZoom::ZoomBoth;
}

pqChartMouseZoom::~pqChartMouseZoom()
{
  delete this->Internal;
}

void pqChartMouseZoom::setMouseOwner(bool owns)
{
  pqChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(this->Internal->ZoomCursor);
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool pqChartMouseZoom::mousePressEvent(QMouseEvent *e, pqChartContentsSpace *)
{
  this->Internal->Last = e->globalPos();
  this->Internal->LastSet = true;
  return false;
}

bool pqChartMouseZoom::mouseMoveEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  if(!this->isMouseOwner())
    {
    emit this->interactionStarted(this);
    }

  if(this->isMouseOwner())
    {
    if(this->Internal->LastSet)
      {
      if(!contents->isInInteraction())
        {
        contents->startInteraction();
        }

      // Zoom in or out based on the mouse movement up or down.
      QPoint pos = e->globalPos();
      int delta = (this->Internal->Last.y() - pos.y())/4;
      if(delta != 0)
        {
        bool changeInX = true;
        bool changeInY = true;
        if(this->Flags == pqChartMouseZoom::ZoomXOnly)
          {
          changeInY = false;
          }
        else if(this->Flags == pqChartMouseZoom::ZoomYOnly)
          {
          changeInX = false;
          }

        int x = contents->getXZoomFactor();
        int y = contents->getYZoomFactor();
        if(changeInX)
          {
          x += delta;
          }

        if(changeInY)
          {
          y += delta;
          }

        this->Internal->Last = pos;
        contents->zoomToPercent(x, y);
        }
      }
    else
      {
      this->Internal->Last = e->globalPos();
      this->Internal->LastSet = true;
      }
    }

  return true;
}

bool pqChartMouseZoom::mouseReleaseEvent(QMouseEvent *,
    pqChartContentsSpace *contents)
{
  if(this->isMouseOwner())
    {
    contents->finishInteraction();
    emit this->interactionFinished(this);
    }

  return true;
}

bool pqChartMouseZoom::mouseDoubleClickEvent(QMouseEvent *,
    pqChartContentsSpace *contents)
{
  contents->resetZoom();
  return true;
}


//----------------------------------------------------------------------------
pqChartMouseZoomX::pqChartMouseZoomX(QObject *parentObject)
  : pqChartMouseZoom(parentObject)
{
  this->setFlags(pqChartMouseZoom::ZoomXOnly);
}


//----------------------------------------------------------------------------
pqChartMouseZoomY::pqChartMouseZoomY(QObject *parentObject)
  : pqChartMouseZoom(parentObject)
{
  this->setFlags(pqChartMouseZoom::ZoomYOnly);
}


//----------------------------------------------------------------------------
pqChartMouseZoomBox::pqChartMouseZoomBox(QObject *parentObject)
  : pqChartMouseFunction(parentObject)
{
  this->MouseBox = 0;
  this->ZoomCursor = new QCursor(QPixmap(zoom_xpm), 11, 11);
}

pqChartMouseZoomBox::~pqChartMouseZoomBox()
{
  delete this->ZoomCursor;
}

void pqChartMouseZoomBox::setMouseOwner(bool owns)
{
  pqChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(*this->ZoomCursor);
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool pqChartMouseZoomBox::mousePressEvent(QMouseEvent *,
    pqChartContentsSpace *)
{
  return false;
}

bool pqChartMouseZoomBox::mouseMoveEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  if(!this->isMouseOwner() && this->MouseBox)
    {
    emit this->interactionStarted(this);
    }

  if(this->isMouseOwner())
    {
    // Get the current mouse position in contents space.
    QPoint point = e->pos();
    contents->translateToContents(point);

    QRect area;
    this->MouseBox->getRectangle(area);
    this->MouseBox->adjustRectangle(point);

    // Repaint the zoom box. Unite the previous area with the new
    // area to ensure all the changes get repainted.
    this->MouseBox->getUnion(area);
    contents->translateFromContents(area);
    emit this->repaintNeeded(area);
    }

  return true;
}

bool pqChartMouseZoomBox::mouseReleaseEvent(QMouseEvent *e,
    pqChartContentsSpace *contents)
{
  if(this->isMouseOwner())
    {
    QPoint point = e->pos();
    contents->translateToContents(point);
    this->MouseBox->adjustRectangle(point);
    QRect area;
    this->MouseBox->getRectangle(area);
    contents->zoomToRectangle(area);
    emit this->interactionFinished(this);
    }

  return true;
}

bool pqChartMouseZoomBox::mouseDoubleClickEvent(QMouseEvent *,
    pqChartContentsSpace *)
{
  return false;
}


