/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewHistogram.h

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

/// \file pqPlotViewHistogram.h
/// \date 7/13/2007

#ifndef _pqPlotViewHistogram_h
#define _pqPlotViewHistogram_h


#include "pqCoreExport.h"
#include <QObject>

class pqBarChartRepresentation;
class pqChartArea;
class pqHistogramChart;
class pqPlotViewHistogramInternal;
class pqRepresentation;


class PQCORE_EXPORT pqPlotViewHistogram : public QObject
{
  Q_OBJECT

public:
  pqPlotViewHistogram(QObject *parent=0);
  virtual ~pqPlotViewHistogram();

  void initialize(pqChartArea *chartArea);

  pqHistogramChart *getChartLayer() const;

  pqBarChartRepresentation *getCurrentRepresentation() const;
  void setCurrentRepresentation(pqBarChartRepresentation *display);

  void update(bool force=false);
  bool isUpdateNeeded();

  void addRepresentation(pqBarChartRepresentation *histogram);
  void removeRepresentation(pqBarChartRepresentation *histogram);
  void removeAllRepresentations();

private slots:
  void updateVisibility(pqRepresentation* display);

private:
  pqPlotViewHistogramInternal *Internal;
};

#endif
