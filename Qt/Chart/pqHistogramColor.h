/*=========================================================================

   Program: ParaView
   Module:    pqHistogramColor.h

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
 * \file pqHistogramColor.h
 *
 * \brief
 *   The pqHistogramColor class is used to control the bar colors on
 *   a pqHistogramChart.
 *
 * \author Mark Richardson
 * \date   May 18, 2005
 */

#ifndef _pqHistogramColor_h
#define _pqHistogramColor_h

#include "QtChartExport.h"
#include <QColor> // Needed for QColor return type.

/*!
 *  \class pqHistogramColor
 *  \brief
 *    The pqHistogramColor class is used to control the bar colors on
 *    a pqHistogramChart.
 * 
 *  The \c getColor method is used to get the color for a specific
 *  histogram bar. The default implementation returns a color from
 *  red to blue based on the bar's index. To get a different color
 *  scheme, create a new class that inherits from pqHistogramColor
 *  and overload the \c getColor method. The following example shows
 *  how to make all the histogram bars the same color.
 *  \code
 *  class OneColor : public pqHistogramColor
 *  {
 *     public:
 *        OneColor();
 *        virtual ~OneColor() {}
 * 
 *        virtual QColor getColor(int index, int total) const;
 *        void setColor(QColor color) {this->color = color;}
 * 
 *     private:
 *        QColor color;
 *  };
 * 
 *  OneColor::OneColor()
 *     : color(Qt::red)
 *  {
 *  }
 * 
 *  QColor OneColor::getColor(int, int) const
 *  {
 *     return this->color;
 *  }
 *  \endcode
 */
class QTCHART_EXPORT pqHistogramColor
{
public:
  pqHistogramColor() {}
  virtual ~pqHistogramColor() {}

  /// \brief
  ///   Called to get the color for a specified histogram bar.
  ///
  /// The default implementation returns a color from red to blue
  /// based on the index of the bar. Overload this method to change
  /// the default behavior.
  ///
  /// \param index The index of the bar.
  /// \param total The total number of bars in the histogram.
  /// \return
  ///   The color to paint the specified bar.
  virtual QColor getColor(int index, int total) const;
};

#endif
