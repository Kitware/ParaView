/*=========================================================================

   Program: ParaView
   Module:    pqHistogramColor.cxx

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

/*!
 * \file pqHistogramColor.cxx
 *
 * \brief
 *   The pqHistogramColor and QHistogramColorParams classes are used to
 *   define the colors used on a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   May 18, 2005
 */

#include "pqHistogramColor.h"


QColor pqHistogramColor::getColor(int index, int total) const
{
  // Get a color that is on the pure hue line of the rgb color
  // cube. Use the index/total ratio to find the color. Only use
  // the hues from red to blue.
  QColor color;
  if(--total > 0)
    {
    int hueTotal = 1020; // 255 * 4
    int hueValue = (hueTotal * index)/total;
    int section = hueValue/255;
    int value = hueValue % 255;
    if(section == 0)
      color.setRgb(255, value, 0);
    else if(section == 1)
      color.setRgb(255 - value, 255, 0);
    else if(section == 2)
      color.setRgb(0, 255, value);
    else if(section == 3)
      color.setRgb(0, 255 - value, 255);
    else
      color.setRgb(value, 0, 255);
    }
  else
    color = Qt::red;

  return color;
}


