/*=========================================================================

   Program: ParaView
   Module:    pqChartMouseBox.cxx

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

/// \file pqChartMouseBox.cxx
/// \date 9/28/2005

#include "pqChartMouseBox.h"

#include <QRect>
#include <QPoint>


class pqChartMouseBoxInternal
{
public:
  pqChartMouseBoxInternal();
  ~pqChartMouseBoxInternal() {}

  QRect Box;
  QPoint Last;
};


//----------------------------------------------------------------------------
pqChartMouseBoxInternal::pqChartMouseBoxInternal()
  : Box(), Last()
{
}


//----------------------------------------------------------------------------
pqChartMouseBox::pqChartMouseBox()
{
  this->Internal = new pqChartMouseBoxInternal();
}

pqChartMouseBox::~pqChartMouseBox()
{
  delete this->Internal;
}

bool pqChartMouseBox::isValid() const
{
  return this->Internal->Box.isValid();
}

void pqChartMouseBox::setStartingPosition(const QPoint &start)
{
  this->Internal->Last = start;
}

void pqChartMouseBox::getRectangle(QRect &area) const
{
  area = this->Internal->Box;
}

void pqChartMouseBox::getPaintRectangle(QRect &area) const
{
  area.setRect(this->Internal->Box.x(), this->Internal->Box.y(),
      this->Internal->Box.width() - 1, this->Internal->Box.height() - 1);
}

void pqChartMouseBox::getUnion(QRect &area) const
{
  if(this->Internal->Box.isValid())
    {
    if(area.isValid())
      {
      area = area.unite(this->Internal->Box);
      }
    else
      {
      area = this->Internal->Box;
      }
    }
}

void pqChartMouseBox::adjustRectangle(const QPoint &current)
{
  // Determine the new area. The last point should be kept as one
  // of the corners.
  if(current.x() < this->Internal->Last.x())
    {
    if(current.y() < this->Internal->Last.y())
      {
      this->Internal->Box.setTopLeft(current);
      this->Internal->Box.setBottomRight(this->Internal->Last);
      }
    else
      {
      this->Internal->Box.setBottomLeft(current);
      this->Internal->Box.setTopRight(this->Internal->Last);
      }
    }
  else
    {
    if(current.y() < this->Internal->Last.y())
      {
      this->Internal->Box.setTopRight(current);
      this->Internal->Box.setBottomLeft(this->Internal->Last);
      }
    else
      {
      this->Internal->Box.setBottomRight(current);
      this->Internal->Box.setTopLeft(this->Internal->Last);
      }
    }
}

void pqChartMouseBox::resetRectangle()
{
  this->Internal->Box = QRect();
}


