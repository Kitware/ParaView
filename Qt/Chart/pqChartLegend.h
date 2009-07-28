/*=========================================================================

   Program: ParaView
   Module:    pqChartLegend.h

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

#ifndef _pqChartLegend_h
#define _pqChartLegend_h


#include "QtChartExport.h"
#include <QWidget>

class pqChartLegendInternal;
class pqChartLegendModel;
class QFont;
class QPainter;
class QPoint;
class QRect;


/// \class pqChartLegend
/// \brief
///   The pqChartLegend class displays a chart legend.
///
/// A pqChartLegendModel is used to describe the entries. Each entry
/// can have an icon and a label. The icon is used to visually
/// identify the series on the chart. For a line chart series, the
/// image should be drawn in the same color and line style.
class QTCHART_EXPORT pqChartLegend : public QWidget
{
  Q_OBJECT

public:
  enum LegendLocation
    {
    Left = 0,  ///< Place the legend on the left of the chart.
    Top,       ///< Place the legend on the top of the chart.
    Right,     ///< Place the legend on the right of the chart.
    Bottom     ///< Place the legend on the bottom of the chart.
    };

  enum ItemFlow
    {
    LeftToRight = 0, ///< Items are arranged left to right.
    TopToBottom      ///< Items are arranged top to bottom.
    };

public:
  /// \brief
  ///   Creates a chart legend instance.
  /// \param parent The parent widget.
  pqChartLegend(QWidget *parent=0);
  virtual ~pqChartLegend();

  /// \name Setup Methods
  //@{
  /// \brief
  ///   Gets the legend model.
  /// \return
  ///   A pointer to the legend model.
  pqChartLegendModel *getModel() const {return this->Model;}

  /// \brief
  ///   Sets the legend model.
  /// \param model The new legend model.
  void setModel(pqChartLegendModel *model);

  /// \brief
  ///   Gets the legend location.
  /// \return
  ///   The legend location.
  LegendLocation getLocation() const {return this->Location;}

  /// \brief
  ///   Sets the legend location.
  ///
  /// The chart uses the location to place the legend in the
  /// appropriate place. The combination of location and flow
  /// determine how the legend looks.
  ///
  /// \param location The new legend location.
  void setLocation(LegendLocation location);

  /// \brief
  ///   Gets the legend item flow.
  /// \return
  ///   The legend item flow.
  ItemFlow getFlow() const {return this->Flow;}

  /// \brief
  ///   Sets the legend item flow.
  ///
  /// The flow is used to determine the layout direction of the
  /// legend entries. Depending on the location, the same flow type
  /// can look different.
  ///
  /// \param flow The new item flow.
  void setFlow(ItemFlow flow);
  //@}

  /// \brief
  ///   Gets the preferred size of the chart legend.
  /// \return
  ///   The preferred size of the chart legend.
  virtual QSize sizeHint() const {return this->Bounds;}

  /// \brief
  ///   Draws the legend using the given painter.
  /// \param painter The painter to use.
  void drawLegend(QPainter &painter);

signals:
  /// Emitted when the legend location is changed.
  void locationChanged();

public slots:
  /// Resets the chart legend.
  void reset();

protected slots:
  /// \brief
  ///   Inserts a new entry in the legend.
  /// \param index Where to insert the entry.
  void insertEntry(int index);

  /// \brief
  ///   Starts the entry removal process.
  /// \param index The entry being removed.
  void startEntryRemoval(int index);

  /// \brief
  ///   Finishes the entry removal process.
  /// \param index The entry that was removed.
  void finishEntryRemoval(int index);

  /// \brief
  ///   Updates the text for the given entry.
  /// \param index The index of the modified entry.
  void updateEntryText(int index);

protected:
  /// \brief
  ///   Updates the layout when the font changes.
  /// \param e Event specific information.
  /// \return
  ///   True if the event was handled.
  virtual bool event(QEvent *e);

  /// \brief
  ///   Draws the chart title.
  /// \param e Event specific information.
  virtual void paintEvent(QPaintEvent *e);

private:
  /// Calculates the preferred size of the chart legend.
  void calculateSize();

private:
  pqChartLegendInternal *Internal; ///< Stores the graphical items.
  pqChartLegendModel *Model;       ///< A pointer to the model.
  LegendLocation Location;         ///< Stores the legend location.
  ItemFlow Flow;                   ///< Stores the order of the items.
  QSize Bounds;                    ///< Stores the prefered size.
  int IconSize;                    ///< Stores the icon size.
  int TextSpacing;                 ///< The space between icon and text.
  int Margin;                      ///< The margin around the entries.
};

#endif
