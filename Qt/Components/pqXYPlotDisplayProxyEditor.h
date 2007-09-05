/*=========================================================================

   Program: ParaView
   Module:    pqXYPlotDisplayProxyEditor.h

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
#ifndef __pqXYPlotDisplayProxyEditor_h
#define __pqXYPlotDisplayProxyEditor_h

#include "pqDisplayPanel.h"

class pqRepresentation;
class QModelIndex;

/// Editor widget for XY plot displays.
class PQCOMPONENTS_EXPORT pqXYPlotDisplayProxyEditor : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqXYPlotDisplayProxyEditor(pqRepresentation* display, QWidget* parent=0);
  virtual ~pqXYPlotDisplayProxyEditor();

public slots:
  /// Reloads the series list from the display.
  void reloadSeries();

protected slots:
  /// Called when the attribute mode selection changes.
  void onAttributeModeChanged();

  void onUseIndexToggled(bool checked);

  void onXArrayNameChanged(const QString &arrayName);

  /// Slot to listen to clicks for changing color.
  void activateItem(const QModelIndex &index);

  void updateOptionsWidgets();

  void setCurrentSeriesEnabled(int state);

  void setCurrentSeriesColor(const QColor &color);

  void setCurrentSeriesThickness(int thickness);

  void setCurrentSeriesStyle(int listIndex);

  void setCurrentSeriesAxes(int listIndex);

  /// \brief
  ///   Updates the enabled state for the given series.
  /// \param index The index of the line series.
  void updateItemEnabled(int index);

  /// \brief
  ///   Updates the color for the given series index.
  /// \param index The index of the line series.
  /// \param color The new series color.
  void updateItemColor(int index, const QColor &color);

  /// \brief
  ///   Updates the style for the given series index.
  /// \param index The index of the line series.
  /// \param lineStyle The new series style.
  void updateItemStyle(int index, Qt::PenStyle lineStyle);

private:
  pqXYPlotDisplayProxyEditor(const pqXYPlotDisplayProxyEditor&); // Not implemented.
  void operator=(const pqXYPlotDisplayProxyEditor&); // Not implemented.

  /// Set the display whose properties this editor is editing.
  /// This call will raise an error is the display is not
  /// a XYPlotDisplay2 proxy.
  void setDisplay(pqRepresentation* display);

  void switchXAxisProperties();
  void reloadXComponentList(const QString &arrayName);

  Qt::CheckState getEnabledState() const;

  class pqInternal;
  pqInternal* Internal;
};

#endif

