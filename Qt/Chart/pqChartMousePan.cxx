/*=========================================================================

   Program: ParaView
   Module:    pqChartMousePan.cxx

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

/// \file pqChartMousePan.cxx
/// \date 6/25/2007

#include "pqChartMousePan.h"

#include "pqChartContentsSpace.h"
#include <QCursor>
#include <QMouseEvent>
#include <QPoint>


class pqChartMousePanInternal
{
public:
  pqChartMousePanInternal();
  ~pqChartMousePanInternal() {}

  QPoint Last;
  bool LastSet;
};


//----------------------------------------------------------------------------
pqChartMousePanInternal::pqChartMousePanInternal()
  : Last()
{
  this->LastSet = false;
}


//----------------------------------------------------------------------------
pqChartMousePan::pqChartMousePan(QObject *parentObject)
  : pqChartMouseFunction(parentObject)
{
  this->Internal = new pqChartMousePanInternal();
}

pqChartMousePan::~pqChartMousePan()
{
  delete this->Internal;
}

void pqChartMousePan::setMouseOwner(bool owns)
{
  pqChartMouseFunction::setMouseOwner(owns);
  if(owns)
    {
    emit this->cursorChangeRequested(QCursor(Qt::ClosedHandCursor));
    }
  else
    {
    emit this->cursorChangeRequested(QCursor(Qt::ArrowCursor));
    }
}

bool pqChartMousePan::mousePressEvent(QMouseEvent *e, pqChartContentsSpace *)
{
  this->Internal->Last = e->globalPos();
  this->Internal->LastSet = true;
  return false;
}

bool pqChartMousePan::mouseMoveEvent(QMouseEvent *e,
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
      QPoint pos = e->globalPos();
      int xOffset = contents->getXOffset();
      int yOffset = contents->getYOffset();
      contents->setXOffset(this->Internal->Last.x() - pos.x() + xOffset);
      contents->setYOffset(this->Internal->Last.y() - pos.y() + yOffset);
      this->Internal->Last = pos;
      }
    else
      {
      this->Internal->Last = e->globalPos();
      this->Internal->LastSet = true;
      }
    }

  return true;
}

bool pqChartMousePan::mouseReleaseEvent(QMouseEvent *,
    pqChartContentsSpace *)
{
  if(this->isMouseOwner())
    {
    emit this->interactionFinished(this);
    }

  this->Internal->LastSet = false;
  return true;
}

bool pqChartMousePan::mouseDoubleClickEvent(QMouseEvent *,
    pqChartContentsSpace *)
{
  return false;
}


