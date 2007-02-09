/*=========================================================================

   Program: ParaView
   Module:    pqVTKHistogramModel.h

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

#ifndef _pqVTKHistogramModel_h
#define _pqVTKHistogramModel_h


#include "pqHistogramModel.h"
#include "pqCoreExport.h"

class pqBarChartDisplay;
class pqDisplay;
class pqHistogramColor;
class pqVTKHistogramModelInternal;
class vtkDataArray;

/// Concrete implementation for the pqHistogramModel for 
/// vtkRectilinearGrid.
class PQCORE_EXPORT pqVTKHistogramModel : public pqHistogramModel
{
  Q_OBJECT
public:
  pqVTKHistogramModel(QObject *parent=0);
  virtual ~pqVTKHistogramModel();

  /// \name pqHistogramModel Methods
  //@{
  virtual int getNumberOfBins() const;
  virtual void getBinValue(int index, pqChartValue &bin) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// Returns the color scheme to be used by the chart.
  pqHistogramColor* getColorScheme() const;

public slots:
  /// Add display to the view. Although this model supports adding
  /// more than 1 display, it shows only 1 plot at a time.
  void addDisplay(pqDisplay*);

  /// Remove a display.
  void removeDisplay(pqDisplay*);

  /// Remove all displays.
  void removeAllDisplays();

  /// Equivalent to "render". Leads to the updating of the widget.
  /// update leads to a call to forceUpdate only it anything
  /// has been modified since last update.
  void update();

  /// Forces update of the widget.
  void forceUpdate();

protected:
  /// Called by forceUpdate when the data is empty.
  void forceUpdateEmptyData();

  /// Set the display that is being currently displayed.
  void setCurrentDisplay(pqBarChartDisplay* display);

  /// Returns the array to be plotted on x axis.
  vtkDataArray* getXArray(pqBarChartDisplay* display) const;

  /// Returns the array to be plotted on y axis.
  vtkDataArray* getYArray(pqBarChartDisplay* display) const;

  /// updates the color scheme.
  void updateColorScheme();

private:
  pqVTKHistogramModelInternal *Internal; ///< Stores the data bounds.
};

#endif

