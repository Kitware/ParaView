/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractor.cxx

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

/// \file pqChartInteractor.cxx
/// \date 5/2/2007

#include "pqChartInteractor.h"

#include "pqChartContentsSpace.h"
#include "pqChartMouseBox.h"

#include <QCursor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRect>
#include <QWheelEvent>


pqChartInteractor::pqChartInteractor(QObject *parentObject)
  : QObject(parentObject)
{
  this->Contents = 0;
  this->MouseBox = 0;
  this->Mode = pqChartInteractor::NoMode;
  this->PanButton = Qt::RightButton;
  this->ZoomButton = Qt::MidButton;
  this->BoxModifier = Qt::ShiftModifier;
  this->XModifier = Qt::ControlModifier;
  this->YModifier = Qt::AltModifier;
}

void pqChartInteractor::setContentsSpace(pqChartContentsSpace *space)
{
  this->Contents = space;
}

void pqChartInteractor::setMouseBox(pqChartMouseBox *box)
{
  this->MouseBox = box;
}

bool pqChartInteractor::keyPressEvent(QKeyEvent *e)
{
  if(!this->Contents)
    {
    return false;
    }

  bool handled = true;
  if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus ||
      e->key() == Qt::Key_Equal)
    {
    // If only the ctrl key is down, zoom only in the x. If only
    // the alt key is down, zoom only in the y. Otherwise, zoom
    // both axes by the same amount. Mask off the shift key since
    // it is needed to press the plus key.
    pqChartContentsSpace::InteractFlags flags = pqChartContentsSpace::ZoomBoth;
    int state = e->modifiers() & (Qt::ControlModifier | Qt::AltModifier |
        Qt::MetaModifier);
    if(state == this->XModifier)
      {
      flags = pqChartContentsSpace::ZoomXOnly;
      }
    else if(state == this->YModifier)
      {
      flags = pqChartContentsSpace::ZoomYOnly;
      }

    // Zoom in for the plus/equal key and out for the minus key.
    if(e->key() == Qt::Key_Minus)
      {
      this->Contents->zoomOut(flags);
      }
    else
      {
      this->Contents->zoomIn(flags);
      }
    }
  else if(e->key() == Qt::Key_Up)
    {
    this->Contents->panUp();
    }
  else if(e->key() == Qt::Key_Down)
    {
    this->Contents->panDown();
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(e->modifiers() == Qt::AltModifier)
      {
      this->Contents->historyPrevious();
      }
    else
      {
      this->Contents->panLeft();
      }
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(e->modifiers() == Qt::AltModifier)
      {
      this->Contents->historyNext();
      }
    else
      {
      this->Contents->panRight();
      }
    }
  else
    {
    handled = false;
    }

  return handled;
}

void pqChartInteractor::mousePressEvent(QMouseEvent *e)
{
  // TODO: Handle selection.
  e->ignore();
}

void pqChartInteractor::mouseMoveEvent(QMouseEvent *e)
{
  // If the interactor is not already in a mouse mode, see which mode
  // to enter.
  if(this->Mode == pqChartInteractor::NoMode)
    {
    if(e->buttons() == this->ZoomButton)
      {
      if(e->modifiers() == this->BoxModifier)
        {
        this->Mode = pqChartInteractor::ZoomBox;
        if(this->Contents)
          {
          emit this->cursorChangeNeeded(this->Contents->getZoomCursor());
          }
        }
      else 
        {
        this->Mode = pqChartInteractor::Zoom;
        if(this->Contents)
          {
          this->Contents->startInteraction(pqChartContentsSpace::Zoom);
          }
        }
      }
    else if(e->buttons() == this->PanButton)
      {
      this->Mode = pqChartInteractor::Pan;
      if(this->Contents)
        {
        this->Contents->startInteraction(pqChartContentsSpace::Pan);
        }
      }
    }

  if(!this->Contents)
    {
    e->ignore();
    return;
    }

  // Interact with the chart based on the mouse mode.
  if(this->Mode == pqChartInteractor::Zoom)
    {
    pqChartContentsSpace::InteractFlags flags = pqChartContentsSpace::ZoomBoth;
    if(e->modifiers() == this->XModifier)
      {
      flags = pqChartContentsSpace::ZoomXOnly;
      }
    else if(e->modifiers() == this->YModifier)
      {
      flags = pqChartContentsSpace::ZoomYOnly;
      }

    this->Contents->interact(e->globalPos(), flags);
    }
  else if(this->Mode == pqChartInteractor::Pan)
    {
    this->Contents->interact(e->globalPos(), pqChartContentsSpace::NoFlags);
    }
  else if(this->MouseBox && this->Mode == pqChartInteractor::ZoomBox)
    {
    // Get the current mouse position in contents space.
    QPoint point = e->pos();
    this->Contents->translateToContents(point);

    QRect area = this->MouseBox->Box;
    this->MouseBox->adjustBox(point);

    // Repaint the zoom box. Unite the previous area with the new
    // area to ensure all the changes get repainted.
    if(area.isValid())
      {
      area = area.unite(this->MouseBox->Box);
      }
    else
      {
      area = this->MouseBox->Box;
      }

    this->Contents->translateFromContents(area);
    emit this->repaintNeeded(area);
    }

  e->accept();
}

void pqChartInteractor::mouseReleaseEvent(QMouseEvent *e)
{
  if(!this->Contents)
    {
    e->ignore();
    return;
    }

  if(this->Mode == pqChartInteractor::Zoom ||
      this->Mode == pqChartInteractor::Pan)
    {
    this->Mode = pqChartInteractor::NoMode;
    this->Contents->finishInteraction();
    }
  else if(this->MouseBox && this->Mode == pqChartInteractor::ZoomBox)
    {
    this->Mode = pqChartInteractor::NoMode;
    QPoint point = e->pos();
    this->Contents->translateToContents(point);
    this->MouseBox->adjustBox(point);
    this->Contents->zoomToRectangle(this->MouseBox->Box);
    emit this->cursorChangeNeeded(Qt::ArrowCursor);
    }

  e->accept();
}

void pqChartInteractor::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(this->Contents && e->button() == this->ZoomButton)
    {
    this->Contents->resetZoom();
    }

  e->accept();
}

void pqChartInteractor::wheelEvent(QWheelEvent *e)
{
  if(this->Contents)
    {
    pqChartContentsSpace::InteractFlags flags = pqChartContentsSpace::ZoomBoth;
    if(e->modifiers() == this->XModifier)
      {
      flags = pqChartContentsSpace::ZoomXOnly;
      }
    else if(e->modifiers() == this->YModifier)
      {
      flags = pqChartContentsSpace::ZoomYOnly;
      }

    // Get the current mouse position and convert it to contents coords.
    this->Contents->handleWheelZoom(e->delta(), e->pos(), flags);
    }

  e->accept();
}

void pqChartInteractor::setMouseMode(int mode)
{
  this->Mode = mode;
}


