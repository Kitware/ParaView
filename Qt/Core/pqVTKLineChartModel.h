/*=========================================================================

   Program: ParaView
   Module:    pqVTKLineChartModel.h

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
#ifndef __pqVTKLineChartModel_h
#define __pqVTKLineChartModel_h

#include "pqLineChartModel.h"
#include "pqCoreExport.h"

class pqDisplay;
class pqLineChartDisplay;
class pqVTKLineChartModelInternal;

/// pqVTKLineChartModelInternal is a concrete implementation for
/// pqLineChartModel that plots XYPlotDisplay2 proxies.
class PQCORE_EXPORT pqVTKLineChartModel : public pqLineChartModel
{
  Q_OBJECT
public:
  typedef pqLineChartModel Superclass;

  pqVTKLineChartModel(QObject* parent=0);
  virtual ~pqVTKLineChartModel();

public slots:
  /// Add a display to the view.
  void addDisplay(pqDisplay*);

  /// Remove a display from the view.
  void removeDisplay(pqDisplay*);

  /// Equivalent to render on render views. It checks is
  /// the plot data/displays have modified since last update.
  /// If so, it will rebuilt the plots.
  void update();

  /// Removes all the plots from the model.
  virtual void clearPlots();

private:
  pqVTKLineChartModel(const pqVTKLineChartModel&); // Not implemented.
  void operator=(const pqVTKLineChartModel&); // Not implemented.

  pqVTKLineChartModelInternal* Internal;

};

#endif

