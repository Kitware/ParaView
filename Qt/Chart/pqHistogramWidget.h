/*!
 * \file pqHistogramWidget.h
 *
 * \brief
 *   The pqHistogramWidget class is used to display and interact with
 *   a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 10, 2005
 */

#ifndef _pqHistogramWidget_h
#define _pqHistogramWidget_h


#include "pqChartExport.h"
#include <QAbstractScrollArea>
#include <QSize> // Needed for QSize return type.

class pqChartAxis;
class pqChartLabel;
class pqChartMouseBox;
class pqChartValue;
class pqChartValueList;
class pqChartZoomPan;
class pqHistogramChart;
class pqHistogramColor;
class pqHistogramSelectionList;
class pqHistogramWidgetData;
class pqLineChart;

class QContextMenuEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QPoint;
class QPopupMenu;
class QPrinter;
class QRect;
class QResizeEvent;
class QTimer;
class QWheelEvent;


/// \class pqHistogramWidget
/// \brief
///   The pqHistogramWidget class is used to display an interactive
///   histogram chart.
///
/// The histogram is displayed in a scroll view to allow the chart
/// to be zoomed in and out. The axes can be zoomed independently as
/// well. The zoom factors and viewport location can be set
/// programatically. The user can use the mouse and keyboard to zoom
/// and pan. The keyboard shortcuts are as follows:
/// \code
/// Plus...................Zoom in.
/// Minus..................Zoom out.
/// Ctrl+Plus..............Horizontally zoom in.
/// Ctrl+minus.............Horizontally zoom out.
/// Alt+Plus...............Vertically zoom in.
/// Alt+minus..............Vertically zoom out.
/// Up.....................Pan up.
/// Down...................Pan down.
/// Left...................Pan left.
/// Right..................Pan right.
/// Alt+Left...............Go to previous view in the history.
/// Alt+Right..............Go to next view in the history.
/// \endcode
///
/// Based on the interaction mode, the histogram can highlight values
/// or bins. When switching between selection modes, the current
/// selection can be erased. This prevents errors when combining bin
/// and value selection. When calling any of the selection methods,
/// the selection mode will be maintained. If you try to set a value
/// selection during bin mode, it will be ignored.
class QTCHART_EXPORT pqHistogramWidget : public QAbstractScrollArea
{
  Q_OBJECT

  Q_ENUMS(InteractMode)
  Q_PROPERTY(InteractMode interactMode READ getInteractMode WRITE setInteractMode)
  //Q_PROPERTY(QColor axisColor READ getAxisColor WRITE setAxisColor)
  //Q_PROPERTY(QColor gridColor READ getGridColor WRITE setGridColor)

public:
  enum InteractMode {
    Bin,
    Value,
    ValueMove,
    Function
  };

  /// \brief
  ///   Creates an interactive histogram chart.
  /// \param parent The parent widget to place the histogram in.
  pqHistogramWidget(QWidget *parent=0);
  virtual ~pqHistogramWidget();

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

  /// \brief
  ///   Gets the current interaction mode.
  /// \return
  ///   The current interaction mode.
  /// \sa pqHistogramWidget::setInteractMode(InteractMode)
  InteractMode getInteractMode() const {return this->Interact;}

  /// Sets whether the user has to hit the "filled" portion of a bin to select it or not
  void setEasyBinSelection(bool Enable = true);
  /// Returns whether the user has to hit the "filled" portion of a bin to select it or not
  const bool getEasyBinSelection() { return this->EasyBinSelection; }

  /// Returns the chart title object
  pqChartLabel& getTitle() {return *this->Title;}

  /// \name Interface Methods
  //@{
  /// \brief
  ///   Gets the histogram chart object.
  /// \return
  ///   A pointer to the histogram chart object.
  pqHistogramChart& getHistogram() {return *this->Histogram;}

  /// \brief
  ///   Gets the line chart object.
  /// \return
  ///   A pointer to the line chart object.
  pqLineChart& getLineChart() {return *this->LineChart;}

  /// Returns the horizontal (histogram) axis
  pqChartAxis& getHorizontalAxis() {return *this->XAxis;}
  /// Returns the vertical (histogram) axis
  pqChartAxis& getHistogramAxis() {return *this->YAxis;}
  /// Returns the vertical (line chart) axis
  pqChartAxis& getLineChartAxis() {return *this->FAxis;}

  /// \brief
  ///   Gets the zoom/pan handler for the widget.
  /// \return
  ///   The widget zoom/pan handler.
  pqChartZoomPan& getZoomPanHandler() const {return *this->ZoomPan;}
  //@}

  /// Prints the chart to the given device (can also be used to generate PDF output, see QPrinter for details)
  void printChart(QPrinter& printer);

public slots:
  /// \brief
  ///   Sets the interaction mode.
  /// \param mode The new interaction mode.
  void setInteractMode(InteractMode mode);

  /// \brief
  ///   Selects all the bins or values on the chart.
  ///
  /// Whether bins or values are selected depends on the interaction
  /// mode.
  void selectAll();

  /// Removes the selection.
  void selectNone();

  /// Inverts the selection.
  void selectInverse();

  /// \brief
  ///   Updates the chart layout when a change is made.
  void updateLayout();

  /// \brief
  ///   Initiates a repaint of the chart when a change is made.
  void repaintChart();

private slots:
  /// \brief
  ///   Used to layout the histogram.
  /// \param width The contents width.
  /// \param height The contents height.
  void layoutChart(int width, int height);

  /// \brief
  ///   Called when the histogram chart's selection has changed.
  ///
  /// The chart will be repainted and the signal will be re-emitted.
  ///
  /// \param list The new selection list.
  void changeSelection(const pqHistogramSelectionList &list);

  /// Called when the mouse move timer expires.
  void moveTimeout();

signals:
  /// \brief
  ///   Called when the interaction mode has changed.
  /// \param mode The new interaction mode.
  void interactModeChanged(pqHistogramWidget::InteractMode mode);

  /// \brief
  ///   Called when the selection has changed.
  /// \param list The new selection.
  void selectionChanged(const pqHistogramSelectionList &list);

public:
  /// \brief
  ///   Used to determine the prefered size of the widget.
  /// \return
  ///   The prefered size of the widget.
  virtual QSize sizeHint() const;

protected:
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
  /// This method handles the drawing of the histogram chart. It is
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
  /// Draws the chart to the given painter
  void draw(QPainter& painter, QRect area);

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

  /// Does the work of sending a selection changed notification.
  void sendSelectionNotification();

  enum MouseMode {
    NoMode,
    MoveWait,
    Pan,
    Zoom,
    ZoomBox,
    SelectBox,
    ValueDrag
  };

  QColor BackgroundColor;      ///< Stores the current background color.
  MouseMode Mode;              ///< Stores the current mouse state.
  InteractMode Interact;       ///< Stores the current interaction mode.
  bool EasyBinSelection;       ///< Stores whether the user has to hit the "filled" portion of a bin to select it
  InteractMode SelectMode;     ///< Stores the current selection type.
  pqChartLabel* const Title;         ///< Used to draw the chart title.
  pqChartAxis* const XAxis;          ///< Used to draw the x-axis.
  pqChartAxis* const YAxis;          ///< Used to draw the y-axis.
  pqChartAxis* const FAxis;          ///< Used to draw the function axis.
  pqHistogramChart* const Histogram; ///< Used to draw the histogram.
  pqLineChart* const LineChart;      ///< Used to draw the line chart.
  pqHistogramWidgetData* const Data; ///< Used in the selection interaction.
  pqChartMouseBox* const Mouse;      ///< Stores the mouse drag box.
  pqChartZoomPan* const ZoomPan;     ///< Handles the zoom/pan interaction.
  QTimer *MoveTimer;           ///< Used for the mouse interaction.
  int LastBin;                 ///< Stores the last bin click.
  int LastValueX;              ///< Stores the last value click.
  bool MouseDown;              ///< Used for mouse interactions.
};

#endif
