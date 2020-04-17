/*=========================================================================

   Program: ParaView
   Module:  pqSeriesEditorPropertyWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef pqSeriesEditorPropertyWidget_h
#define pqSeriesEditorPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

class QModelIndex;
class vtkObject;
class vtkSMPropertyGroup;

/**
 * pqSeriesEditorPropertyWidget is the pqPropertyWidget used to edit
 * plot-options for a xy-series. It works with a property-group with properties
 * for various functions, including required properties (SeriesVisibility)
 * and optional properties (SeriesLabel, SeriesColor, SeriesLineThickness,
 * SeriesLineStyle, SeriesMarkerStyle, and SeriesPlotCorner).
 * If any of optional property functions are missing from the group, the
 * corresponding widgets will be hidden.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSeriesEditorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> seriesVisibility READ seriesVisibility WRITE setSeriesVisibility NOTIFY
      seriesVisibilityChanged)

  Q_PROPERTY(
    QList<QVariant> presetLabel READ presetLabel WRITE setPresetLabel NOTIFY presetLabelChanged)

  Q_PROPERTY(
    QList<QVariant> seriesLabel READ seriesLabel WRITE setSeriesLabel NOTIFY seriesLabelChanged)

  Q_PROPERTY(
    QList<QVariant> seriesColor READ seriesColor WRITE setSeriesColor NOTIFY seriesColorChanged)

  Q_PROPERTY(
    QList<QVariant> presetColor READ presetColor WRITE setPresetColor NOTIFY presetColorChanged)

  Q_PROPERTY(QList<QVariant> seriesLineThickness READ seriesLineThickness WRITE
      setSeriesLineThickness NOTIFY seriesLineThicknessChanged)

  Q_PROPERTY(QList<QVariant> seriesLineStyle READ seriesLineStyle WRITE setSeriesLineStyle NOTIFY
      seriesLineStyleChanged)

  Q_PROPERTY(QList<QVariant> seriesMarkerStyle READ seriesMarkerStyle WRITE setSeriesMarkerStyle
      NOTIFY seriesMarkerStyleChanged)

  Q_PROPERTY(QList<QVariant> seriesMarkerSize READ seriesMarkerSize WRITE setSeriesMarkerSize NOTIFY
      seriesMarkerSizeChanged)

  Q_PROPERTY(QList<QVariant> seriesPlotCorner READ seriesPlotCorner WRITE setSeriesPlotCorner NOTIFY
      seriesPlotCornerChanged)

  typedef pqPropertyWidget Superclass;

public:
  pqSeriesEditorPropertyWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = 0);
  ~pqSeriesEditorPropertyWidget() override;

  //@{
  /**
   * Get/Set the visibility for series.
   */
  QList<QVariant> seriesVisibility() const;
  void setSeriesVisibility(const QList<QVariant>&);
  //@}

  //@{
  /**
   * Get/Set the color for each of the series.
   */
  QList<QVariant> seriesColor() const;
  void setSeriesColor(const QList<QVariant>&);
  //@}

  QList<QVariant> presetColor() const;
  void setPresetColor(const QList<QVariant>&);
  //@{
  /**
   * Get/Set the label for each of the series.
   */
  QList<QVariant> seriesLabel() const;
  void setSeriesLabel(const QList<QVariant>&);
  //@}

  QList<QVariant> presetLabel() const;
  void setPresetLabel(const QList<QVariant>&);

  //@{
  /**
   * Get/Set the line-thickness for each of the series.
   */
  QList<QVariant> seriesLineThickness() const;
  void setSeriesLineThickness(const QList<QVariant>&);
  //@}

  //@{
  /**
   * Get/Set the line-style for each of the series.
   */
  QList<QVariant> seriesLineStyle() const;
  void setSeriesLineStyle(const QList<QVariant>&);
  //@}

  //@{
  /**
   * Get/Set the marker-style for each of the series.
   */
  QList<QVariant> seriesMarkerStyle() const;
  void setSeriesMarkerStyle(const QList<QVariant>&);
  //@}

  //@{
  /**
   * Get/Set the marker-size for each of the series.
   */
  QList<QVariant> seriesMarkerSize() const;
  void setSeriesMarkerSize(const QList<QVariant>&);
  //@}

  //@{
  /**
   * Get/Set the plot-corner for each of the series.
   */
  QList<QVariant> seriesPlotCorner() const;
  void setSeriesPlotCorner(const QList<QVariant>&);
  //@}

Q_SIGNALS:
  //@{
  /**
   * Fired when the series visibility changes.
   */
  void seriesVisibilityChanged();
  //@}

  //@{
  /**
   * Fired when the series labels change.
   */
  void seriesLabelChanged();
  //@}

  void presetLabelChanged();

  //@{
  /**
   * Fired when the series colors change.
   */
  void seriesColorChanged();
  //@}

  void presetColorChanged();

  //@{
  /**
   * Fired when the series line thickness changes
   */
  void seriesLineThicknessChanged();
  //@}

  //@{
  /**
   * Fired when the series line style changes
   */
  void seriesLineStyleChanged();
  //@}

  //@{
  /**
   * Fired when the series marker style changes
   */
  void seriesMarkerStyleChanged();
  //@}

  //@{
  /**
   * Fired when the series marker size changes
   */
  void seriesMarkerSizeChanged();
  //@}

  //@{
  /**
   * Fired when the series plot corners change
   */
  void seriesPlotCornerChanged();
  //@}

private Q_SLOTS:
  //@{
  /**
   * update all series-properties widgets using the "current" series.
   */
  void refreshPropertiesWidgets();
  //@}

  //@{
  /**
   * update all selected series with the value from the sender widget.
   */
  void savePropertiesWidgets();
  //@}

  //@{
  /**
   * called when the vtkSMProperty fires a vtkCommand::DomainModifiedEvent.
   */
  void domainModified(vtkObject* sender);
  //@}

  /**
   * called when the color preset is modified.
   */
  void onPresetChanged(const QString& name);

private:
  Q_DISABLE_COPY(pqSeriesEditorPropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
