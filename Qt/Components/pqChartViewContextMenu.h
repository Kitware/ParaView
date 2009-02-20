/*=========================================================================

   Program: ParaView
   Module:    pqChartViewContextMenu.h

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

#ifndef _pqChartViewContextMenu_h
#define _pqChartViewContextMenu_h


#include "pqComponentsExport.h"
#include <QObject>
#include "vtkQtChartAxis.h" // needed for enum

class pqActiveViewOptionsManager;
class pqView;
class QPoint;
class QString;
class QWidget;
class vtkQtChartTitle;
class vtkQtChartLegend;
class vtkQtChartWidget;


/// \class pqChartViewContextMenu
/// \brief
///   The pqChartViewContextMenu class handles the context menu
///   requests for chart views.
class PQCOMPONENTS_EXPORT pqChartViewContextMenu : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart view context menu.
  ///
  /// The \c manager is used to open the chart options dialog when
  /// the "Properties" menu item is activated.
  ///
  /// \param view The chart view, which is used as the parent object.
  /// \param manager The options dialog manager.
  pqChartViewContextMenu(pqView *view, pqActiveViewOptionsManager *manager);
  virtual ~pqChartViewContextMenu();

  const QString &getChartLayerPage() const;
  void setChartLayerPage(const QString &page);

signals:
  /// Emitted when a screenshot has been requested.
  void screenshotRequested();

protected slots:
  virtual void showContextMenu(const QPoint &location);
  virtual void setupTitleMenu(vtkQtChartTitle *title);
  virtual void setupLegendMenu(vtkQtChartLegend *legend);
  virtual void setupAxisTitleMenu(vtkQtChartAxis::AxisLocation axis,
      vtkQtChartTitle *title);
  void showChartAreaProperties();
  void showOtherProperties();

protected:
  virtual const QString &getChartOptionsPage(const QPoint &location);
  virtual void addCommonActions(QWidget *widget);

protected:
  pqView *View;                        ///< Stores the chart view.
  vtkQtChartWidget *Chart;             ///< Stores the chart widget.
  pqActiveViewOptionsManager *Manager; ///< Stores the options manager.

private:
  QPoint *Point;                       ///< Stores the menu location.
  QString *Page;                       ///< Stores the chart layer page.
};

#endif
