/*!
 * \file pqLineChartWidget.h
 *
 * \brief
 *   The pqLineChartWidget class is used to display and interact with
 *   a line chart.
 *
 * \author Mark Richardson
 * \date   September 27, 2005
 */

#ifndef _pqLineChartWidget_h
#define _pqLineChartWidget_h


#include "pqChartExport.h"
#include <QAbstractScrollArea>

class pqChartAxis;
class pqChartLabel;
class pqChartLegend;
class pqChartMouseBox;
class pqChartZoomPan;
class pqLineChart;


/// \class pqLineChartWidget
/// \brief
///   The pqLineChartWidget class is used to display and interact with
///   a line chart.
class QTCHART_EXPORT pqLineChartWidget : public QAbstractScrollArea
{
  Q_OBJECT

public:
  enum MouseMode {
    NoMode,
    MoveWait,
    Pan,
    Zoom,
    ZoomBox,
    SelectBox,
  };

public:
  /// \brief
  ///   Creates an interactive line chart.
  /// \param parent The parent widget to place the chart in.
  pqLineChartWidget(QWidget *parent=0);
  virtual ~pqLineChartWidget();

  /// \brief
  ///   Sets the background color for the widget.
  ///
  /// All chart components are drawn on-top-of this color.
  ///
  /// \param color The color to use for the background.
  void setBackgroundColor(const QColor &color);

  /// \brief
  ///   Sets the font for the widget.
  ///
  /// The font is used for the labels on the histogram axes.
  ///
  /// \param font The font to use.
  virtual void setFont(const QFont &font);

  /// Returns the chart title object
  pqChartLabel& getTitle() {return *this->Title;}

  /// Returns the chart legend object
  pqChartLegend& getLegend() {return *this->Legend;}

  /// \name Interface Methods
  //@{
  /// \brief
  ///   Gets the line chart object.
  /// \return
  ///   A pointer to the line chart object.
  pqLineChart *getLineChart() {return this->LineChart;}

  /// \brief
  ///   Returns the chart X-axis.
  /// \return
  ///   A pointer to the axis.
  pqChartAxis *getXAxis() {return this->XAxis;}

  /// \brief
  ///   Returns the chart Y-axis.
  /// \return
  ///   A pointer to the axis.
  pqChartAxis *getYAxis() {return this->YAxis;}

  /// \brief
  ///   Gets the zoom/pan handler for the widget.
  /// \return
  ///   The widget zoom/pan handler.
  pqChartZoomPan *getZoomPanHandler() const {return this->ZoomPan;}
  //@}

public slots:
  /// \brief
  ///   Updates the chart layout when a change is made.
  void updateLayout();

  /// \brief
  ///   Initiates a repaint of the chart when a change is made.
  void repaintChart();

private slots:
  /// \brief
  ///   Used to layout the line chart.
  /// \param width The contents width.
  /// \param height The contents height.
  void layoutChart(int width, int height);

public:
  /// \brief
  ///   Used to determine the prefered size of the widget.
  /// \return
  ///   The prefered size of the widget.
  virtual QSize sizeHint() const;

protected:
  /// Called to handle tooltip events
  bool event(QEvent *e);

  /// \brief
  ///   Called to handle user key press events.
  ///
  /// The key press handler allows the user to zoom, pan, and
  /// navigate the zoom history. See the class detail description
  /// for the specific keys.
  ///
  /// \param e Event specific data.
  virtual void keyPressEvent(QKeyEvent *e);

  /// \brief
  ///   Called when the widget is shown.
  /// \param e Event specific data.
  virtual void showEvent(QShowEvent *e);

  /// \brief
  ///   Called to draw the contents of the scroll view.
  ///
  /// This method handles the drawing of the line chart. It is
  /// called whenever the data or the size changes. The clip
  /// region coordinates are used to determine which portion of
  /// the chart needs to be redrawn.
  ///
  /// \param e Event specific data.
  virtual void paintEvent(QPaintEvent *e);

  /// \brief
  ///   Called to handle a mouse press event.
  /// \param e Event specific data.
  virtual void mousePressEvent(QMouseEvent *e);

  /// \brief
  ///   Called to handle a mouse release event.
  /// \param e Event specific data.
  virtual void mouseReleaseEvent(QMouseEvent *e);

  /// \brief
  ///   Called to handle a mouse double click event.
  /// \param e Event specific data.
  virtual void mouseDoubleClickEvent(QMouseEvent *e);

  /// \brief
  ///   Called to handle a mouse move event.
  /// \param e Event specific data.
  virtual void mouseMoveEvent(QMouseEvent *e);

  /// \brief
  ///   Called to handle a mouse wheel event.
  /// \param e Event specific data.
  virtual void wheelEvent(QWheelEvent *e);

  /// \brief
  ///   Called when the viewport is resized.
  /// \param e Event specific data.
  virtual void resizeEvent(QResizeEvent *e);

  /// \brief
  ///   Displays the default context menu.
  /// \param e Event specific data.
  virtual void contextMenuEvent(QContextMenuEvent *e);

private:
  /// \brief
  ///   Called to handle viewport events.
  ///
  /// The mouse trigger for the context menu must always be the
  /// mouse release event. The mouse interactions for panning
  /// prevent the context menu from happening on the mouse down
  /// event.
  ///
  /// \param e Event specific data.
  virtual bool viewportEvent(QEvent *e);

private:
  QColor BackgroundColor;  ///< Stores the background color.
  MouseMode Mode;          ///< Stores the current mouse state.
  pqChartMouseBox *Mouse;  ///< Stores the mouse drag box.
  pqChartZoomPan *ZoomPan; ///< Handles the zoom/pan interaction.
  pqChartLabel* const Title;     ///< Used to draw the chart title.
  pqChartAxis *XAxis;      ///< Used to draw the x-axis.
  pqChartAxis *YAxis;      ///< Used to draw the y-axis.
  pqChartLegend* const Legend; ///< Used to draw the chart legend.
  pqLineChart *LineChart;  ///< Used to draw the line chart.
  bool MouseDown;          ///< Used for mouse interactions.
};

#endif
