/*=========================================================================

   Program: ParaView
   Module:    pqParallelCoordinatesChartDisplayPanel.h

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
#ifndef __pqParallelCoordinatesChartDisplayPanel_h
#define __pqParallelCoordinatesChartDisplayPanel_h

#include "pqDisplayPanel.h"

class pqRepresentation;
class pqDataInformationModel;
class QModelIndex;

/// Editor widget for XY chart displays.
class PQCOMPONENTS_EXPORT pqParallelCoordinatesChartDisplayPanel : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqParallelCoordinatesChartDisplayPanel(pqRepresentation* display, QWidget* parent=0);
  virtual ~pqParallelCoordinatesChartDisplayPanel();

public slots:
  /// Reloads the series list from the display.
  void reloadSeries();

protected slots:
  /// Slot to listen to clicks for changing color.
  void activateItem(const QModelIndex &index);

  void updateOptionsWidgets();

  void setSeriesColor(const QColor &color);

  void setSeriesOpacity(double opacity);

  void setSeriesThickness(int thickness);

  void setSeriesStyle(int listIndex);

private:
  pqParallelCoordinatesChartDisplayPanel(const pqParallelCoordinatesChartDisplayPanel&); // Not implemented.
  void operator=(const pqParallelCoordinatesChartDisplayPanel&); // Not implemented.

  /// Set the display whose properties this editor is editing.
  /// This call will raise an error if the display is not
  /// an XYChartRepresentation proxy.
  void setDisplay(pqRepresentation* display);

  Qt::CheckState getEnabledState() const;

  class pqInternal;
  pqInternal* Internal;

  pqDataInformationModel* Model;
};

#endif
