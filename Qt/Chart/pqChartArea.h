/*=========================================================================

   Program: ParaView
   Module:    pqChartArea.h

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

/// \file pqChartArea.h
/// \date 11/20/2006

#ifndef _pqChartArea_h
#define _pqChartArea_h


#include "QtChartExport.h"
#include <QWidget>
#include "pqChartAxis.h" // Needed for enum

class pqChartAreaInternal;
class pqChartAxisLayer;
class pqChartContentsSpace;
class pqChartGridLayer;
class pqChartInteractor;
class pqChartMouseBox;
class pqChartLayer;
class QPainter;
class QPrinter;
class QRect;


/// \class pqChartArea
/// \brief
///   The pqChartArea class manages the chart axes and layers.
class QTCHART_EXPORT pqChartArea : public QWidget
{
  Q_OBJECT

public:
  enum AxisBehavior
    {
    ChartSelect = 0, ///< The axis labels are determined by the charts.
    BestFit,         ///< The axis labels are determined by space.
    FixedInterval    ///< The axis labels are fixed.
    };

public:
  /// \brief
  ///   Creates a chart area instance.
  /// \param parent The parent widget.
  pqChartArea(QWidget *parent=0);
  virtual ~pqChartArea();

  /// \brief
  ///   Gets the preferred size of the chart area.
  /// \return
  ///   The preferred size of the chart area.
  virtual QSize sizeHint() const;

  /// \name Axis Methods
  //@{
  /// \brief
  ///   Gets the axis in the specified location.
  /// \param location The location of the axis.
  /// \return
  ///   A pointer to the specified axis.
  pqChartAxis *getAxis(pqChartAxis::AxisLocation location) const;

  /// \brief
  ///   Gets the layout behavior for the specified axis.
  /// \param location The location of the axis.
  /// \return
  ///   The layout behavior for the specified axis.
  AxisBehavior getAxisBehavior(pqChartAxis::AxisLocation location) const;

  /// \brief
  ///   Sets the layout behavior for the specified axis.
  /// \param location The location of the axis.
  /// \param behavior The new layout behavior.
  void setAxisBehavior(pqChartAxis::AxisLocation location,
      AxisBehavior behavior);
  //@}

  /// \name Layer Methods
  //@{
  /// \brief
  ///   Adds a chart layer to the top of the layer list.
  /// \param chart The chart to add to the list.
  /// \sa pqChartArea::insertLayer(int, pqChartLayer *)
  void addLayer(pqChartLayer *chart);

  /// \brief
  ///   Inserts a chart layer in the layer list.
  ///
  /// The chart layers are drawn in the order they reside in the list
  /// of layers. A chart at the beginning of the list is drawn
  /// underneath the other layers. A chart at the end of the list is
  /// drawn on top of the other layers.
  ///
  /// The chart area has two built in chart layers: the grid and axis
  /// layers. By default, the grid layer is on the bottom and the axis
  /// layer is on the next layer. The index of these layers can be
  /// used to place other layers in the chart.
  ///
  /// \param index Where to insert the chart.
  /// \param chart The chart to insert in the list.
  /// \sa
  ///   pqChartArea::getGridLayerIndex(),
  ///   pqChartArea::getAxisLayerIndex()
  void insertLayer(int index, pqChartLayer *chart);

  /// \brief
  ///   Removes a chart layer from the layer list.
  /// \param chart The chart to remove from the list.
  void removeLayer(pqChartLayer *chart);

  /// \brief
  ///   Gets the chart layer at the specified index.
  /// \param index The index of the layer.
  /// \return
  ///   A pointer to the chart layer at the specified index.
  pqChartLayer *getLayer(int index) const;

  /// \brief
  ///   Gets the index for the grid layer.
  /// \return
  ///   The index for the grid layer.
  int getGridLayerIndex() const;

  /// \brief
  ///   Gets the index for the axis layer.
  /// \return
  ///   The index for the axis layer.
  int getAxisLayerIndex() const;
  //@}

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Gets the current chart interactor.
  /// \return
  ///   A pointer to the current chart interactor.
  pqChartInteractor *getInteractor() const {return this->Interactor;}

  /// \brief
  ///   Sets the chart interactor.
  ///
  /// This method sets up the interactor to work with the chart. The
  /// contents space and mouse box are set on the interactor.
  ///
  /// \param interactor The new chart interactor.
  void setInteractor(pqChartInteractor *interactor);

  /// \brief
  ///   Gets the contents space object.
  /// \return
  ///   A pointer to the contents space object.
  pqChartContentsSpace *getContentsSpace() const {return this->Contents;}
  //@}

  /// \name Printing Methods
  //@{
  /// \brief
  ///   Prints the chart using the given printer.
  /// \param printer The printer to use.
  void printChart(QPrinter &printer);

  /// \brief
  ///   Draws the chart axes and layers using the given painter.
  /// \param painter The painter to use.
  /// \param area The area that need painting.
  void drawChart(QPainter &painter, const QRect &area);
  //@}

public slots:
  /// Calculates the axis and chart layout.
  void layoutChart();

  /// Merges layout requests into one delayed layout event.
  void updateLayout();

protected:
  /// \brief
  ///   Updates the layout when the font changes.
  /// \param e Event specific information.
  /// \return
  ///   True if the event was handled.
  virtual bool event(QEvent *e);

  /// \brief
  ///   Updates the layout when the size changes.
  /// \param e Event specific information.
  virtual void resizeEvent(QResizeEvent *e);

  /// \brief
  ///   Draws the chart area.
  /// \param e Event specific information.
  virtual void paintEvent(QPaintEvent *e);

  /// \name Interaction Methods
  //@{
  /// \brief
  ///   Handles the key press events for the chart.
  ///
  /// All the interaction events are forwarded to the pqChartInteractor.
  /// It is up to the interactor object to accept or ignore the events.
  ///
  /// \param e Event specific information.
  virtual void keyPressEvent(QKeyEvent *e);

  /// \brief
  ///   Handles the mouse press events for the chart.
  /// \param e Event specific information.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse move events for the chart.
  /// \param e Event specific information.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse release events for the chart.
  /// \param e Event specific information.
  virtual void mouseReleaseEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse double click events for the chart.
  /// \param e Event specific information.
  virtual void mouseDoubleClickEvent(QMouseEvent *e);

  /// \brief
  ///   Handles the mouse wheel events for the chart.
  /// \param e Event specific information.
  virtual void wheelEvent(QWheelEvent *e);
  //@}

signals:
  /// Emitted when a delayed chart layout is needed.
  void delayedLayoutNeeded();

private slots:
  /// Sets a flag to recalculate the overall range.
  void handleChartRangeChange();

  /// Updates the layout after a zoom change.
  void handleZoomChange();

  /// \brief
  ///   Changes the mouse cursor to the specified cursor.
  /// \param cursor The new mouse cursor.
  void changeCursor(const QCursor &cursor);

  /// \brief
  ///   Calls for a repaint on the given area.
  /// \param area The area to repaint.
  void updateArea(const QRect &area);

private:
  /// Creates an axis in each of the chart axis locations.
  void setupAxes();

private:
  pqChartAreaInternal *Internal;  ///< Stores the list of layers.
  pqChartGridLayer *GridLayer;    ///< Stores the grid layer.
  pqChartAxisLayer *AxisLayer;    ///< Stores the axis layer.
  pqChartContentsSpace *Contents; ///< Stores the contents space.
  pqChartMouseBox *MouseBox;      ///< Stores the mouse box.
  pqChartInteractor *Interactor;  ///< Stores the chart interactor.
};

#endif
