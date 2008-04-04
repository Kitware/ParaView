/*=========================================================================

   Program: ParaView
   Module:    pqChartAxisLayer.h

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

/// \file pqChartAxisLayer.h
/// \date 2/9/2007

#ifndef _pqChartAxisLayer_h
#define _pqChartAxisLayer_h


#include "QtChartExport.h"
#include "pqChartLayer.h"

class QRect;


/// \class pqChartAxisLayer
/// \brief
///   The pqChartAxisLayer class draws all the chart axis objects.
class QTCHART_EXPORT pqChartAxisLayer : public pqChartLayer
{
public:
  /// \brief
  ///   Builds a chart axis layer.
  /// \param parent The parent object.
  pqChartAxisLayer(QObject *parent=0);
  virtual ~pqChartAxisLayer();

  /// \name pqChartLayer Methods
  //@{
  virtual void layoutChart(const QRect &area);
  virtual void drawChart(QPainter &painter, const QRect &area);
  //@}

private:
  QRect *Bounds; ///< Stores the boundary.
};

#endif
