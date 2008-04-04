/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewContextMenu.h

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

/// \file pqPlotViewContextMenu.h
/// \date 9/18/2007

#ifndef _pqPlotViewContextMenu_h
#define _pqPlotViewContextMenu_h


#include "pqComponentsExport.h"
#include <QObject>
#include "pqChartAxis.h" // Needed for enum

class pqActiveViewOptionsManager;
class pqChartTitle;
class pqChartLegend;
class pqPlotView;
class QPoint;
class QString;
class QWidget;


/// \class pqPlotViewContextMenu
/// \brief
///   The pqPlotViewContextMenu class handles the context menu
///   requests for the plot view.
class PQCOMPONENTS_EXPORT pqPlotViewContextMenu : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a plot view context menu.
  ///
  /// The \c manager is used to open the chart options dialog when
  /// the "Properties" menu item is activated.
  ///
  /// \param view The plot view, which is used as the parent object.
  /// \param manager The options dialog manager.
  pqPlotViewContextMenu(pqPlotView *view, pqActiveViewOptionsManager *manager);
  virtual ~pqPlotViewContextMenu();

signals:
  /// Emitted when a screenshot has been requested.
  void screenshotRequested();

private slots:
  void showContextMenu(const QPoint &location);
  void showChartAreaProperties();
  void showOtherProperties();
  void setupChartTitle(pqChartTitle *title);
  void setupChartLegend(pqChartLegend *legend);
  void setupAxisTitle(pqChartAxis::AxisLocation axis, pqChartTitle *title);

private:
  void addCommonActions(QWidget *widget);

private:
  pqPlotView *View;                    ///< Stores the plot view.
  pqActiveViewOptionsManager *Manager; ///< Stores the options manager.
  QPoint *Point;                       ///< Stores the menu location.
};

#endif
