/*=========================================================================

   Program: ParaView
   Module:    pqLineChartRepresentation.h

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
#ifndef __pqLineChartRepresentation_h
#define __pqLineChartRepresentation_h

#include "pqDataRepresentation.h"

class vtkSMProperty;
class vtkDataArray;
class vtkTable;
class QColor;

/// pqLineChartRepresentation is a pqRepresentation for "XYPlotRepresentation" proxy.
/// It adds logic to initialize default state as well as access
/// get information about the plot parameters from the proxy.
class PQCORE_EXPORT pqLineChartRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;

public:
  pqLineChartRepresentation(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server, QObject* parent=0);
  virtual ~pqLineChartRepresentation();

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
  vtkTable* getClientSideData() const;
  bool isDataModified() const;

  vtkDataArray* getArray(const QString &arrayName) const;

  /// Returns the array used for x-axis.
  vtkDataArray* getXArray() const;

  /// Returns the array used for y axis at the given index.
  vtkDataArray* getYArray(int index) const;

  /// Returns the array used for masking.
  vtkDataArray* getMaskArray();

  /// Returns the value of the "CompositeDataSetIndex" property.
  unsigned int getCompositeDataSetIndex() const;

  bool isUpdateNeeded() const;
  bool isArrayUpdateNeeded(int attributeType) const;

  int getAttributeType() const;

  int getNumberOfSeries() const;

  int getSeriesIndex(const QString &name, int component) const;

  bool isSeriesEnabled(int series) const;
  void setSeriesEnabled(int series, bool enabled);

  void getSeriesName(int series, QString &name) const;
  void setSeriesName(int series, const QString &name);

  bool isSeriesInLegend(int series) const;
  void setSeriesInLegend(int series, bool inLegend);

  void getSeriesLabel(int series, QString &label) const;
  void setSeriesLabel(int series, const QString &label);

  void getSeriesColor(int series, QColor &color) const;
  void setSeriesColor(int series, const QColor &color);
  bool isSeriesColorSet(int series) const;

  int getSeriesThickness(int series) const;
  void setSeriesThickness(int series, int thickness);

  Qt::PenStyle getSeriesStyle(int series) const;
  void setSeriesStyle(int series, Qt::PenStyle style);
  bool isSeriesStyleSet(int series) const;

  int getSeriesAxesIndex(int series) const;
  void setSeriesAxesIndex(int series, int index);

  int getSeriesComponent(int series) const;
  void addComponentLabel(QString &name, int component, int numComponents) const;

  void beginSeriesChanges();
  void endSeriesChanges();

public slots:
  void startSeriesUpdate(bool force=false);
  void finishSeriesUpdate();
  void setAttributeType(int attr);
  void markAsModified();

signals:
  /// Emitted when the series list has changed.
  void seriesListChanged();

  /// \brief
  ///   Emitted when the enabled state for a series has changed.
  /// \param series The index of the series.
  /// \param enabled True if the series is enabled.
  void enabledStateChanged(int series, bool enabled);

  /// \brief
  ///   Emitted when the legend state for a series has changed.
  /// \param series The index of the series.
  /// \param inLegend True if the series is in the legend.
  void legendStateChanged(int series, bool inLegend);

  /// \brief
  ///   Emitted when the color for a series has changed.
  /// \param series The index of the series.
  /// \param color The new color for the series.
  void colorChanged(int series, const QColor &color);

  /// \brief
  ///   Emitted when the style for a series has changed.
  /// \param series The index of the series.
  /// \param style The new line style for the series.
  void styleChanged(int series, Qt::PenStyle style);

protected:
  bool getXArrayDefault(vtkSMProperty* prop, QString &arrayName);

private slots:
  void changeSeriesList();
  void markListModified();

private:
  pqLineChartRepresentation(const pqLineChartRepresentation&); // Not implemented.
  void operator=(const pqLineChartRepresentation&); // Not implemented.

  int isEnabledByDefault(const QString &arrayName) const;

  void saveSeriesChanges();

  class pqInternals;
  pqInternals *Internals;
};


#endif

