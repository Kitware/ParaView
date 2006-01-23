/*!
 * \file pqHistogramWidget.cxx
 *
 * \brief
 *   The pqHistogramWidget class is used to display and interact with
 *   a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 10, 2005
 */

#include "pqHistogramWidget.h"
#include "pqChartMouseBox.h"
#include "pqChartValue.h"
#include "pqChartZoomPan.h"
#include "pqHistogramChart.h"
#include "pqHistogramColor.h"
#include "pqHistogramSelection.h"
#include "pqLineChart.h"
#include "pqChartAxis.h"
#include "pqChartLabel.h"

#include <QCursor>
#include <QEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

// Set up a margin around the histogram.
#define MARGIN 3
#define DBL_MARGIN 6


/// \class pqHistogramWidgetData
/// \brief
///   The pqHistogramWidgetData class stores information needed for
///   the mouse interactions.
class pqHistogramWidgetData
{
public:
  pqHistogramWidgetData();
  ~pqHistogramWidgetData();

  /// Cleans up the memory for the previous selection list.
  void cleanSelectionList();

public:
  /// Stores the previously used list of selected ranges.
  pqHistogramSelectionList Selection;
};


pqHistogramWidgetData::pqHistogramWidgetData()
  : Selection()
{
}

pqHistogramWidgetData::~pqHistogramWidgetData()
{
  cleanSelectionList();
}

void pqHistogramWidgetData::cleanSelectionList()
{
  pqHistogramSelectionList::Iterator iter = this->Selection.begin();
  for( ; iter != this->Selection.end(); iter++)
    {
    if(*iter)
      delete *iter;
    }

  this->Selection.clear();
}


pqHistogramWidget::pqHistogramWidget(QWidget *p)
  : QAbstractScrollArea(p)
{
  this->BackgroundColor = Qt::white;
  this->Mode = pqHistogramWidget::NoMode;
  this->Interact = pqHistogramWidget::Bin;
  this->EasyBinSelection = true;
  this->SelectMode = pqHistogramWidget::Bin;
  this->Title = 0;
  this->XAxis = 0;
  this->YAxis = 0;
  this->FAxis = 0;
  this->Histogram = 0;
  this->LineChart = 0;
  this->Data = new pqHistogramWidgetData();
  this->Mouse = new pqChartMouseBox();
  this->ZoomPan = new pqChartZoomPan(this);
  this->MoveTimer = 0;
  this->LastBin = -1;
  this->LastValueX = -1;
  this->MouseDown = false;

  // Set up the default Qt properties.
  this->setFocusPolicy(Qt::ClickFocus);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QPalette newPalette = this->viewport()->palette();
  newPalette.setColor(QPalette::Background, QColor(Qt::white));
  this->viewport()->setPalette(newPalette);
  this->setAttribute(Qt::WA_KeyCompression);

  // Setup the chart title
  this->Title = new pqChartLabel();
  connect(this->Title, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->Title, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  // Set up the histogram and its parameters.
  QFont myFont = font();
  this->XAxis = new pqChartAxis(pqChartAxis::Bottom);
  this->YAxis = new pqChartAxis(pqChartAxis::Left);
  this->FAxis = new pqChartAxis(pqChartAxis::Right);
  if(this->XAxis)
    {
    this->XAxis->setNeigbors(this->YAxis, this->FAxis);
    this->XAxis->setTickLabelFont(myFont);
    connect(this->XAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
    connect(this->XAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));
    }

  if(this->YAxis)
    {
    this->YAxis->setNeigbors(this->XAxis, 0);
    this->YAxis->setTickLabelFont(myFont);
    connect(this->YAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
    connect(this->YAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));
    }

  if(this->FAxis)
    {
    this->FAxis->setNeigbors(this->XAxis, 0);
    this->FAxis->setTickLabelFont(myFont);
    this->FAxis->setAxisColor(Qt::darkBlue); // Differentiate the function grid.
    connect(this->FAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
    connect(this->FAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));
    }

  this->Histogram = new pqHistogramChart();
  if(this->Histogram)
    {
    this->Histogram->setAxes(this->XAxis, this->YAxis);
    connect(this->Histogram, SIGNAL(repaintNeeded()), this,
        SLOT(repaintChart()));
    connect(this->Histogram,
        SIGNAL(selectionChanged(const pqHistogramSelectionList &)),
        this, SLOT(changeSelection(const pqHistogramSelectionList &)));
    }

  this->LineChart = new pqLineChart();
  if(this->LineChart)
    {
    this->LineChart->setAxes(this->XAxis, this->FAxis);
    connect(this->LineChart, SIGNAL(repaintNeeded()), this,
        SLOT(repaintChart()));
    }

  // Set up the widget for keyboard input.
  this->setAttribute(Qt::WA_InputMethodEnabled);

  // Connect to the zoom/pan object signal.
  if(this->ZoomPan)
    {
    this->ZoomPan->setObjectName("ZoomPan");
    connect(this->ZoomPan, SIGNAL(contentsSizeChanging(int, int)),
        this, SLOT(layoutChart(int, int)));
    }
}

pqHistogramWidget::~pqHistogramWidget()
{
  delete this->Title;

  if(this->Histogram)
    delete this->Histogram;
  if(this->LineChart)
    delete this->LineChart;
  if(this->XAxis)
    delete this->XAxis;
  if(this->YAxis)
    delete this->YAxis;
  if(this->FAxis)
    delete this->FAxis;

  if(this->Data)
    delete this->Data;
  if(this->ZoomPan)
    delete this->ZoomPan;
  if(this->Mouse)
    delete this->Mouse;
}

void pqHistogramWidget::setBackgroundColor(const QColor& color)
{
  this->BackgroundColor = color;

  if(this->ZoomPan)
    {
    this->layoutChart(this->ZoomPan->contentsWidth(),
        this->ZoomPan->contentsHeight());
    }
}

void pqHistogramWidget::setFont(const QFont &f)
{
  QAbstractScrollArea::setFont(f);

  // Block the axis update signals until all the changes are
  // made. This avoids laying out the chart for each individual
  // change.
  QFontMetrics fm(f);
  if(this->XAxis)
    {
    this->XAxis->blockSignals(true);
    this->XAxis->setTickLabelFont(f);
    this->XAxis->blockSignals(false);
    }

  if(this->YAxis)
    {
    this->YAxis->blockSignals(true);
    this->YAxis->setTickLabelFont(f);
    this->YAxis->blockSignals(false);
    }

  if(this->FAxis)
    {
    this->FAxis->blockSignals(true);
    this->FAxis->setTickLabelFont(f);
    this->FAxis->blockSignals(false);
    }

  if(this->ZoomPan)
    {
    this->layoutChart(this->ZoomPan->contentsWidth(),
        this->ZoomPan->contentsHeight());
    }
}

pqChartAxis *pqHistogramWidget::getAxis(AxisName name)
{
  if(name == pqHistogramWidget::HorizontalAxis)
    {
    return this->XAxis;
    }
  else if(name == pqHistogramWidget::HistogramAxis)
    {
    return this->YAxis;
    }
  else if(name == pqHistogramWidget::LineChartAxis)
    {
    return this->FAxis;
    }

  return 0;
}

void pqHistogramWidget::setInteractMode(InteractMode mode)
{
  if(mode != this->Interact)
    {
    if(this->Interact == pqHistogramWidget::ValueMove)
      this->LastValueX = -1;
    this->Interact = mode;
    if(this->Interact != pqHistogramWidget::Function)
      {
      // If the user is switching between bin and value selection,
      // clear the selection.
      if(this->SelectMode != this->Interact && (this->Interact ==
          pqHistogramWidget::Bin || this->SelectMode ==
          pqHistogramWidget::Bin))
        {
        this->selectNone();
        }

      this->SelectMode = this->Interact;
      }

    emit this->interactModeChanged(this->Interact);
    }
}

void pqHistogramWidget::setEasyBinSelection(bool Enable)
{
  this->EasyBinSelection = Enable;
}

void pqHistogramWidget::selectAll()
{
  if(this->Histogram)
    {
    if(this->Interact == pqHistogramWidget::Bin)
      this->Histogram->selectAllBins();
    else if(this->Interact == pqHistogramWidget::Value)
      this->Histogram->selectAllValues();
    }
}

void pqHistogramWidget::selectNone()
{
  if(this->Histogram)
    this->Histogram->selectNone();
}

void pqHistogramWidget::selectInverse()
{
  if(this->Histogram)
    {
    if(this->Histogram->hasSelection())
      this->Histogram->selectInverse();
    else
      this->selectAll();
    }
}

void pqHistogramWidget::updateLayout()
{
  // All of the chart members' layouts are interrelated. When one
  // of them needs to be updated, they all need to be updated.
  if(this->ZoomPan)
    {
    this->layoutChart(this->ZoomPan->contentsWidth(),
        this->ZoomPan->contentsHeight());
    this->viewport()->update();
    }
}

void pqHistogramWidget::repaintChart()
{
  this->viewport()->update();
}

void pqHistogramWidget::layoutChart(int w, int h)
{
  QRect area(MARGIN, MARGIN, w - DBL_MARGIN, h - DBL_MARGIN);
  
  const QRect title_request = this->Title->getSizeRequest();
  this->Title->setBounds(QRect(area.left(), area.top(), area.width(), title_request.height()));
  area = QRect(area.left(), area.top() + title_request.height(), area.right(), area.height() - title_request.height());
  
  if(this->XAxis)
    this->XAxis->layoutAxis(area);
  if(this->YAxis)
    this->YAxis->layoutAxis(area);
  if(this->FAxis)
    this->FAxis->layoutAxis(area);
  if(this->Histogram)
    this->Histogram->layoutChart();
  if(this->LineChart)
    this->LineChart->layoutChart();
}

void pqHistogramWidget::changeSelection(const pqHistogramSelectionList &list)
{
  emit this->selectionChanged(list);
  this->viewport()->update();
}

void pqHistogramWidget::moveTimeout()
{
  this->sendSelectionNotification();
}

QSize pqHistogramWidget::sizeHint() const
{
  this->ensurePolished();
  int f = 150 + 2*this->frameWidth();
  return QSize(f, f);
}

void pqHistogramWidget::keyPressEvent(QKeyEvent *e)
{
  bool handled = true;
  if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus ||
      e->key() == Qt::Key_Equal)
    {
    if(this->ZoomPan)
      {
      // If only the ctrl key is down, zoom only in the x. If only
      // the alt key is down, zoom only in the y. Otherwise, zoom
      // both axes by the same amount. Mask off the shift key since
      // it is needed to press the plus key.
      pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
      int state = e->modifiers() & (Qt::ControlModifier | Qt::AltModifier |
          Qt::MetaModifier);
      if(state == Qt::ControlModifier)
        flags = pqChartZoomPan::ZoomXOnly;
      else if(state == Qt::AltModifier)
        flags = pqChartZoomPan::ZoomYOnly;

      // Zoom in for the plus/equal key and out for the minus key.
      if(e->key() == Qt::Key_Minus)
        this->ZoomPan->zoomOut(flags);
      else
        this->ZoomPan->zoomIn(flags);
      }
    }
  else if(e->key() == Qt::Key_Up)
    {
    if(this->ZoomPan)
      this->ZoomPan->panUp();
    }
  else if(e->key() == Qt::Key_Down)
    {
    if(this->ZoomPan)
      this->ZoomPan->panDown();
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(this->ZoomPan)
      {
      if(e->modifiers() == Qt::AltModifier)
        this->ZoomPan->historyPrevious();
      else
        this->ZoomPan->panLeft();
      }
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(this->ZoomPan)
      {
      if(e->modifiers() == Qt::AltModifier)
        this->ZoomPan->historyNext();
      else
        this->ZoomPan->panRight();
      }
    }
  else if(e->key() == Qt::Key_M)
    {
    if(this->Interact == pqHistogramWidget::Value)
      setInteractMode(pqHistogramWidget::ValueMove);
    else if(this->Interact == pqHistogramWidget::ValueMove)
      setInteractMode(pqHistogramWidget::Value);
    else
      handled = false;
    }
  else
    handled = false;

  if(handled)
    e->accept();
  else
    QAbstractScrollArea::keyPressEvent(e);
}

void pqHistogramWidget::showEvent(QShowEvent *e)
{
  QAbstractScrollArea::showEvent(e);
  if(this->ZoomPan)
    this->ZoomPan->updateContentSize();
}

void pqHistogramWidget::paintEvent(QPaintEvent *e)
{
  if(!this->ZoomPan)
    return;

  // Qt4 now handles the double buffering. Create a QPainter to
  // handle the drawing.
  QPainter *painter = new QPainter(this->viewport());
  if(!painter)
    return;

  // Make sure the painter is usable.
  if(!painter->isActive())
    {
    delete painter;
    return;
    }

  // Get the clip area from the paint event. Set the painter to
  // content coordinates.
  QRect area = e->rect();
  if(!area.isValid())
    return;
  painter->translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
  area.translate(this->ZoomPan->contentsX(), this->ZoomPan->contentsY());
  painter->setClipRect(area);

  // Paint the widget background.
  painter->fillRect(area, this->BackgroundColor);

  // Paint the highlight background.
  if(this->Histogram)
    this->Histogram->drawBackground(painter, area);

  // Draw in the axes and grid.
  if(this->FAxis)
    this->FAxis->drawAxis(painter, area);
  if(this->YAxis)
    this->YAxis->drawAxis(painter, area);
  if(this->XAxis)
    this->XAxis->drawAxis(painter, area);

  // Paint the histogram.
  if(this->Histogram)
    this->Histogram->drawChart(painter, area);

  // Paint the line chart on top of the histogram.
  if(this->LineChart)
    this->LineChart->drawChart(painter, area);

  // Draw in the axis lines again to ensure they are on top.
  if(this->FAxis)
    this->FAxis->drawAxisLine(painter);
  if(this->YAxis)
    this->YAxis->drawAxisLine(painter);
  if(this->XAxis)
    this->XAxis->drawAxisLine(painter);

  // Draw the chart title
  this->Title->draw(*painter, area);

  if(this->Mouse && this->Mouse->Box.isValid())
    {
    // Draw in mouse box selection or zoom if needed.
    painter->setPen(Qt::black);
    painter->setPen(Qt::DotLine);
    if(this->Mode == pqHistogramWidget::ZoomBox ||
        this->Mode == pqHistogramWidget::SelectBox)
      {
      painter->drawRect(this->Mouse->Box.x(), this->Mouse->Box.y(),
          this->Mouse->Box.width() - 1, this->Mouse->Box.height() - 1);
      }
    }

  // Clean up the painter.
  delete painter;
}

void pqHistogramWidget::mousePressEvent(QMouseEvent *e)
{
  if(!this->ZoomPan)
    return;

  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = true;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  // Save the necessary coordinates.
  if(this->Mouse)
    this->Mouse->Last = point;
  this->ZoomPan->Last = e->globalPos();

  // Make sure the timer is allocated and connected.
  if(!this->MoveTimer)
    {
    this->MoveTimer = new QTimer(this);
    this->MoveTimer->setObjectName("MouseMoveTimeout");
    this->MoveTimer->setSingleShot(true);
    connect(this->MoveTimer, SIGNAL(timeout()), this, SLOT(moveTimeout()));
    }

  if(this->Histogram && e->button() == Qt::LeftButton)
    {
    // Block the selection changed signals in case the user is going to
    // drag the mouse. Use a timer to delay the signal if the user is
    // simply picking. Only block the signals if the timer was allocated.
    bool delaySignal = false;
    if(this->MoveTimer)
      this->Histogram->blockSignals(true);

    pqHistogramSelection range;
    if(this->Interact == pqHistogramWidget::Bin)
      {
      int bin = this->Histogram->getBinAt(point.x(), point.y(), this->EasyBinSelection);
      range.setType(pqHistogramSelection::Bin);
      range.setRange(bin, bin);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(bin != -1)
          {
          delaySignal = true;
          if(this->LastBin == -1)
            {
            this->Histogram->setSelection(&range);
            this->LastBin = bin;
            }
          else
            {
            range.setFirst(this->LastBin);
            this->Histogram->setSelection(&range);
            }
          }
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        if(bin != -1)
          {
          delaySignal = true;
          this->Histogram->xorSelection(&range);
          this->LastBin = bin;

          // Set up the selection list so the first click doesn't
          // get changed.
          if(this->Data)
            {
            this->Data->cleanSelectionList();
            pqHistogramSelection *selection = new pqHistogramSelection(range);
            if(selection)
              this->Data->Selection.pushBack(selection);
            }
          }
        else if(this->Data)
          this->Data->cleanSelectionList();
        }
      else
        {
        delaySignal = true;
        this->LastBin = bin;
        if(bin == -1)
          this->Histogram->selectNone();
        else
          this->Histogram->setSelection(&range);
        }
      }
    else if(this->Interact == pqHistogramWidget::Value)
      {
      pqChartValue val;
      bool valid = this->Histogram->getValueAt(point.x(), point.y(), val);
      range.setType(pqHistogramSelection::Value);
      range.setRange(val, val);
      if(e->modifiers() & Qt::ShiftModifier)
        {
        if(valid)
          {
          delaySignal = true;
          if(this->LastValueX == -1)
            {
            this->LastValueX = point.x();
            this->Histogram->setSelection(&range);
            }
          else
            {
            pqChartValue last;
            if(this->Histogram->getValueAt(this->LastValueX, point.y(), last))
              {
              range.setFirst(last);
              this->Histogram->setSelection(&range);
              }
            }
          }
        }
      else if(e->modifiers() & Qt::ControlModifier)
        {
        if(valid)
          {
          delaySignal = true;
          this->LastValueX = point.x();
          this->Histogram->xorSelection(&range);

          // Set up the selection list so the first click doesn't
          // get changed.
          if(this->Data)
            {
            this->Data->cleanSelectionList();
            pqHistogramSelection *selection = new pqHistogramSelection(range);
            if(selection)
              this->Data->Selection.pushBack(selection);
            }
          }
        else if(this->Data)
          this->Data->cleanSelectionList();
        }
      else
        {
        delaySignal = true;
        if(valid)
          {
          this->LastValueX = point.x();
          this->Histogram->setSelection(&range);
          }
        else
          {
          this->LastValueX = -1;
          this->Histogram->selectNone();
          }
        }
      }
    else if(this->Interact == pqHistogramWidget::ValueMove)
      {
      bool valid = this->Histogram->getValueRangeAt(point.x(), point.y(), range);
      if(valid)
        this->LastValueX = point.x();
      else
        this->LastValueX = -1;
      }

    this->Histogram->blockSignals(false);
    if(this->MoveTimer && delaySignal)
      {
      this->Mode = pqHistogramWidget::MoveWait;
      this->MoveTimer->start(200);
      this->viewport()->update();
      }
    }

  e->accept();
}

void pqHistogramWidget::mouseReleaseEvent(QMouseEvent *e)
{
  if(!this->ZoomPan)
    return;

  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = false;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  if(this->Mode == pqHistogramWidget::ZoomBox)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    if(this->Mouse)
      {
      this->Mouse->adjustBox(point);
      this->ZoomPan->zoomToRectangle(&this->Mouse->Box);
      this->Mouse->resetBox();
      }
    }
  else if(this->Mode == pqHistogramWidget::SelectBox)
    {
    this->Mode = pqHistogramWidget::NoMode;
    if(this->Data)
      {
      // Send off a selection changed signal.
      sendSelectionNotification();
      this->Data->cleanSelectionList();
      }

    if(this->Mouse)
      {
      QRect area = this->Mouse->Box;
      this->Mouse->resetBox();
      if(area.isValid())
        {
        // Translate the area to viewport coordinates.
        area.translate(-this->ZoomPan->contentsX(),
            -this->ZoomPan->contentsY());
        this->viewport()->update(area);
        }
      }
    }
  else if(this->Mode == pqHistogramWidget::ValueDrag)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    if(this->Data)
      {
      // Send off a selection changed signal.
      sendSelectionNotification();
      this->Data->cleanSelectionList();
      }
    }
  else if(this->Mode == pqHistogramWidget::Zoom ||
      this->Mode == pqHistogramWidget::Pan)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->ZoomPan->finishInteraction();
    }
  else if(this->Mode == pqHistogramWidget::MoveWait)
    {
    // Cancel the timer and send the selection changed signal.
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    if(this->MoveTimer)
      this->MoveTimer->stop();
    this->sendSelectionNotification();
    }
  else if(this->Mode != pqHistogramWidget::NoMode)
    {
    this->Mode = pqHistogramWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    }
  else if(e->button() == Qt::RightButton)
    {
    // Display the context menu.
    QContextMenuEvent cme(QContextMenuEvent::Mouse, e->pos(), e->globalPos());
    QAbstractScrollArea::viewportEvent(&cme);
    }

  e->accept();
}

void pqHistogramWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(e->button() == Qt::MidButton && this->ZoomPan)
    this->ZoomPan->resetZoom();

  e->accept();
}

void pqHistogramWidget::mouseMoveEvent(QMouseEvent *e)
{
  if(!this->ZoomPan || !this->MouseDown)
    return;

  // Get the current mouse position and convert it to contents coords.
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  // Check for the move wait timer. If it is active, cancel
  // the timer so it does not send a selection update.
  if(this->Mode == pqHistogramWidget::MoveWait)
    {
    if(this->MoveTimer)
      this->MoveTimer->stop();
    this->Mode = pqHistogramWidget::NoMode;
    }

  bool handled = true;
  if(this->Mode == pqHistogramWidget::NoMode)
    {
    // Change the cursor for the mouse mode.
    if(e->buttons() == Qt::LeftButton)
      {
      if(this->Interact == pqHistogramWidget::Value)
        {
        if(this->LastValueX != -1)
          this->Mode = pqHistogramWidget::ValueDrag;
        }
      else if(this->Interact == pqHistogramWidget::ValueMove)
        {
        if(this->LastValueX != -1)
          {
          this->Mode = pqHistogramWidget::ValueDrag;
          this->setCursor(Qt::SizeAllCursor);
          }
        }
      else
        this->Mode = pqHistogramWidget::SelectBox;
      }
    else if(e->buttons() == Qt::MidButton)
      {
      if(e->modifiers() & Qt::ShiftModifier)
        {
        this->Mode = pqHistogramWidget::ZoomBox;
        this->ZoomPan->setZoomCursor();
        }
      else
        {
        this->Mode = pqHistogramWidget::Zoom;
        this->ZoomPan->startInteraction(pqChartZoomPan::Zoom);
        }
      }
    else if(e->buttons() == Qt::RightButton)
      {
      this->Mode = pqHistogramWidget::Pan;
      this->ZoomPan->startInteraction(pqChartZoomPan::Pan);
      }
    else
      handled = false;
    }

  if(this->Data)
    {
    if(this->Mode == pqHistogramWidget::ZoomBox)
      {
      QRect area = this->Mouse->Box;
      this->Mouse->adjustBox(point);

      // Repaint the zoom box. Unite the previous area with the new
      // area to ensure all the changes get repainted.
      if(area.isValid())
        area = area.unite(this->Mouse->Box);
      else
        area = this->Mouse->Box;

      area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
      this->viewport()->update(area);
      }
    else if(this->Mode == pqHistogramWidget::SelectBox)
      {
      // Save a copy of the old area. The union of the old and new
      // areas need to be repainted.
      QRect area = this->Mouse->Box;
      this->Mouse->adjustBox(point);
      if(area.isValid())
        area = area.unite(this->Mouse->Box);
      else
        area = this->Mouse->Box;

      // Update the selection based on the new selection box.
      if(this->Histogram && this->Interact == pqHistogramWidget::Bin)
        {
        // Block the selection changed signals. A single signal will
        // get sent when the mouse is released.
        this->Histogram->blockSignals(true);
        pqHistogramSelectionList newSelection;
        this->Histogram->getBinsIn(this->Mouse->Box, newSelection, this->EasyBinSelection);
        if(e->modifiers() & Qt::ShiftModifier)
          {
          if(!this->Data->Selection.isEmpty())
            this->Histogram->subtractSelection(this->Data->Selection);
          this->Histogram->addSelection(newSelection);
          }
        else if(e->modifiers() & Qt::ControlModifier)
          {
          // Make a second copy of the new selection list so it doesn't
          // get lost when finding the intersection.
          pqHistogramSelectionList toDelete;
          pqHistogramSelectionList copy;
          copy.makeNewCopy(newSelection);
          this->Data->Selection.Xor(copy, toDelete);
          this->Histogram->xorSelection(this->Data->Selection);
          this->Data->Selection += toDelete;
          }
        else
          this->Histogram->setSelection(newSelection);
        this->Histogram->blockSignals(false);

        // Adjust the repaint area to include the highlight;
        int extra = this->Histogram->getBinWidth() + 1;
        area.setRight(area.right() + extra);
        area.setLeft(area.left() - extra);
        area.setTop(0);
        area.setBottom(this->ZoomPan->contentsHeight());

        // Save the new selection in place of the old one.
        this->Data->cleanSelectionList();
        this->Data->Selection = newSelection;
        }

      // Repaint the affected area.
      area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
      this->viewport()->update(area);
      }
    else if(this->Mode == pqHistogramWidget::ValueDrag)
      {
      if(this->Histogram && this->Interact == pqHistogramWidget::Value)
        {
        QRect area;
        area.setTop(0);
        area.setBottom(this->ZoomPan->contentsHeight());
        if(this->LastValueX < point.x())
          {
          area.setLeft(this->LastValueX);
          area.setRight(point.x());
          }
        else
          {
          area.setRight(this->LastValueX);
          area.setLeft(point.x());
          }

        pqHistogramSelectionList newSelection;
        this->Histogram->getValuesIn(area, newSelection);
        this->Histogram->blockSignals(true);
        if((e->modifiers() & Qt::ControlModifier) &&
            !(e->modifiers() & Qt::ShiftModifier))
          {
          // Make a second copy of the new selection list so it doesn't
          // get lost when finding the intersection.
          pqHistogramSelectionList toDelete;
          pqHistogramSelectionList copy;
          copy.makeNewCopy(newSelection);
          this->Data->Selection.Xor(copy, toDelete);
          this->Histogram->xorSelection(this->Data->Selection);
          this->Data->Selection += toDelete;
          }
        else
          this->Histogram->setSelection(newSelection);
        this->Histogram->blockSignals(false);

        // Adjust the repaint area to include the highlight;
        int extra = this->Histogram->getBinWidth() + 1;
        area.setRight(area.right() + extra);
        area.setLeft(area.left() - extra);

        // Save the new selection in place of the old one.
        this->Data->cleanSelectionList();
        this->Data->Selection = newSelection;

        area.translate(-this->ZoomPan->contentsX(),
            -this->ZoomPan->contentsY());
        this->viewport()->update(area);
        }
      else if(this->Histogram && this->LastValueX != point.x())
        {
        // Get the selection range under the previous mouse position
        // and move it to the new position.
        pqHistogramSelection range;
        if(this->Histogram->getValueRangeAt(this->LastValueX, point.y(), range))
          {
          // Get the value offset.
          pqChartValue offset = this->XAxis->getValueFor(point.x());
          offset -= this->XAxis->getValueFor(this->LastValueX);
          if(offset != 0)
            {
            // Save the last position and move the selection.
            this->Histogram->blockSignals(true);
            this->Histogram->moveSelection(range, offset);
            this->Histogram->blockSignals(false);
            if(range.getFirst() == range.getSecond())
              {
              range.moveRange(offset);
              this->LastValueX = this->XAxis->getPixelFor(range.getFirst());
              }
            else
              this->LastValueX = point.x();

            this->viewport()->update();
            }
          }
        }
      }
    else if(this->Mode == pqHistogramWidget::Zoom)
      {
      pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
      if(e->modifiers() == Qt::ControlModifier)
        flags = pqChartZoomPan::ZoomXOnly;
      else if(e->modifiers() == Qt::AltModifier)
        flags = pqChartZoomPan::ZoomYOnly;
      this->ZoomPan->interact(e->globalPos(), flags);
      }
    else if(this->Mode == pqHistogramWidget::Pan)
      this->ZoomPan->interact(e->globalPos(), pqChartZoomPan::NoFlags);
    else
      handled = false;
    }

  if(handled)
    e->accept();
  else
    e->ignore();
}

void pqHistogramWidget::wheelEvent(QWheelEvent *e)
{
  if(this->ZoomPan)
    {
    pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
    if(e->modifiers() == Qt::ControlModifier)
      flags = pqChartZoomPan::ZoomXOnly;
    else if(e->modifiers() == Qt::AltModifier)
      flags = pqChartZoomPan::ZoomYOnly;

    // Get the current mouse position and convert it to contents coords.
    QPoint point = e->pos();
    point.rx() += this->ZoomPan->contentsX();
    point.ry() += this->ZoomPan->contentsY();
    this->ZoomPan->handleWheelZoom(e->delta(), point, flags);
    }

  e->accept();
}

void pqHistogramWidget::resizeEvent(QResizeEvent *)
{
  if(this->ZoomPan)
    this->ZoomPan->updateContentSize();
}

void pqHistogramWidget::contextMenuEvent(QContextMenuEvent *e)
{
  // TODO: Display the default context menu.
  e->accept();
}

bool pqHistogramWidget::viewportEvent(QEvent *e)
{
  // Make sure that the context menu is not called for a mouse down.
  if(e->type() == QEvent::ContextMenu)
    {
    QContextMenuEvent *cme = static_cast<QContextMenuEvent *>(e);
    if(cme->reason() == QContextMenuEvent::Mouse)
      return false;
    }

  return QAbstractScrollArea::viewportEvent(e);
}

void pqHistogramWidget::sendSelectionNotification()
{
  if(this->Histogram)
    {
    pqHistogramSelectionList list;
    this->Histogram->getSelection(list);
    emit selectionChanged(list);
    pqHistogramSelectionList::Iterator iter = list.begin();
    for( ; iter != list.end(); ++iter)
      {
      if(*iter)
        delete *iter;
      }
    }
}


