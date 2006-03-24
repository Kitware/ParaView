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
 * \file pqChartMouseBox.cxx
 *
 * \brief
 *   The pqChartMouseBox class stores the data for a mouse zoom or
 *   selection box.
 *
 * \author Mark Richardson
 * \date   September 28, 2005
 */

#include "pqChartMouseBox.h"


pqChartMouseBox::pqChartMouseBox()
  : Box(), Last()
{
}

void pqChartMouseBox::adjustBox(const QPoint &current)
{
  // Determine the new area. The last point should be kept as one
  // of the corners.
  if(current.x() < this->Last.x())
    {
    if(current.y() < this->Last.y())
      {
      this->Box.setTopLeft(current);
      this->Box.setBottomRight(this->Last);
      }
    else
      {
      this->Box.setBottomLeft(current);
      this->Box.setTopRight(this->Last);
      }
    }
  else
    {
    if(current.y() < this->Last.y())
      {
      this->Box.setTopRight(current);
      this->Box.setBottomLeft(this->Last);
      }
    else
      {
      this->Box.setBottomRight(current);
      this->Box.setTopLeft(this->Last);
      }
    }
}

void pqChartMouseBox::resetBox()
{
  this->Box.setCoords(0, 0, 0, 0);
}


