/*=========================================================================

   Program: ParaView
   Module:    pqLineChartDisplay.h

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
#ifndef __pqLineChartDisplay_h
#define __pqLineChartDisplay_h

#include "pqConsumerDisplay.h"

class vtkSMProperty;
class vtkDataArray;
class vtkRectilinearGrid;
class QColor;

/// pqLineChartDisplay is a pqDisplay for "XYPlotDisplay2" proxy.
/// It adds logic to initialize default state as well as access
/// get information about the plot parameters from the proxy.
class PQCORE_EXPORT pqLineChartDisplay : public pqConsumerDisplay
{
  Q_OBJECT
  typedef pqConsumerDisplay Superclass;

public:
  pqLineChartDisplay(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server, QObject* parent=0);
  virtual ~pqLineChartDisplay();

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values. 
  virtual void setDefaultPropertyValues();

  /// Returns the client-side rectilinear grid. 
  /// Note that this method does not update the pipeline.
  vtkRectilinearGrid* getClientSideData() const;

  /// Returns the array used for x-axis.
  vtkDataArray* getXArray();

  /// Returns the array used for y axis at the given index.
  vtkDataArray* getYArray(int index);

  int getAttributeType() const;

  int getNumberOfSeries() const;

  int getSeriesIndex(const QString &name) const;

  bool isSeriesEnabled(int series) const;
  void setSeriesEnabled(int series, bool enabled);

  void getSeriesName(int series, QString &name) const;
  void setSeriesName(int series, const QString &name);

  void getSeriesColor(int series, QColor &color) const;
  void setSeriesColor(int series, const QColor &color);
  bool isSeriesColorSet(int series) const;

  void beginSeriesChanges();
  void endSeriesChanges();

public slots:
  void updateSeries();
  void setAttributeType(int attr);

signals:
  /// Emitted when the series list has changed.
  void seriesListChanged();

  /// \brief
  ///   Emitted when the color for a series has changed.
  /// \param series The index of the series.
  void colorChanged(int series, const QColor &color);

protected:
  /// method to set default values for the status property.
  void setStatusDefaults(vtkSMProperty* prop);

private slots:
  void changeSeriesList();

private:
  pqLineChartDisplay(const pqLineChartDisplay&); // Not implemented.
  void operator=(const pqLineChartDisplay&); // Not implemented.

  int isEnabledByDefault(const QString &arrayName) const;

  void saveSeriesChanges();

  class pqInternals;
  pqInternals *Internals;
};


#endif

