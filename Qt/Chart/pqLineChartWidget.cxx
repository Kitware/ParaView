/*=========================================================================

   Program: ParaView
   Module:    pqLineChartWidget.cxx

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

/*!
 * \file pqLineChartWidget.cxx
 *
 * \brief
 *   The pqLineChartWidget class is used to display and interact with
 *   a line chart.
 *
 * \author Mark Richardson
 * \date   September 27, 2005
 */

#include "pqChartAxis.h"
#include "pqChartLabel.h"
#include "pqChartLegend.h"
#include "pqChartMouseBox.h"
#include "pqChartZoomPan.h"
#include "pqChartZoomPanAlt.h"
#include "pqLineChart.h"
#include "pqLineChartWidget.h"

#include <QCursor>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPoint>
#include <QPrinter>
#include <QRect>
#include <QToolTip>

// Set up a margin around the chart.
#define MARGIN 3
#define DBL_MARGIN 6


pqLineChartWidget::pqLineChartWidget(QWidget *p) :
  QAbstractScrollArea(p),
  BackgroundColor(Qt::white),
  Mode(pqLineChartWidget::NoMode),
  Mouse(new pqChartMouseBox()),
  ZoomPan(new pqChartZoomPan(this)),
  ZoomPanAlt(new pqChartZoomPanAlt(this)),
  Title(new pqChartLabel()),
  XAxis(new pqChartAxis(pqChartAxis::Bottom)),
  YAxis(new pqChartAxis(pqChartAxis::Left)),
  Legend(new pqChartLegend()),
  LineChart(new pqLineChart()),
  MouseDown(false),
  SkipContextMenu(false),
  RightYAxis(new pqChartAxis(pqChartAxis::Right)),
  SecondLineChart(new pqLineChart())
{
  // Set up the default Qt properties.
  this->setFocusPolicy(Qt::ClickFocus);
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QPalette newPalette = this->viewport()->palette();
  newPalette.setColor(QPalette::Background, QColor(Qt::white));
  this->viewport()->setPalette(newPalette);
  this->setAttribute(Qt::WA_KeyCompression);

  // Set up the widget for keyboard input.
  this->setAttribute(Qt::WA_InputMethodEnabled);

  this->useAlternateZoomPan = false;

  // Connect to the zoom/pan object signal.
  this->ZoomPan->setObjectName("ZoomPan");
  connect(this->ZoomPan, SIGNAL(contentsSizeChanging(int, int)),
      this, SLOT(layoutChart(int, int)));

  // Connect to the zoom/pan alternative object signal.
  this->ZoomPanAlt->setObjectName("ZoomPanAlt");

  // Setup the chart title
  connect(this->Title, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->Title, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  // Setup the chart legend
  connect(this->Legend, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->Legend, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  // Set up the line chart and the axes.
  QFont myFont = font();
  this->XAxis->setNeigbors(this->YAxis, this->RightYAxis);
  this->XAxis->setTickLabelFont(myFont);
  connect(this->XAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->XAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  this->YAxis->setNeigbors(this->XAxis, 0);
  this->YAxis->setTickLabelFont(myFont);
  connect(this->YAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->YAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));

  this->LineChart->setAxes(this->XAxis, this->YAxis);
  connect(this->LineChart, SIGNAL(layoutNeeded()), this,
      SLOT(updateLayout()));
  connect(this->LineChart, SIGNAL(repaintNeeded()), this,
      SLOT(repaintChart()));

  this->RightYAxis->setNeigbors(this->XAxis, 0);
  this->RightYAxis->setTickLabelFont(myFont);
  connect(this->RightYAxis, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  connect(this->RightYAxis, SIGNAL(repaintNeeded()), this, SLOT(repaintChart()));
  // turn off axis by default
  this->RightYAxis->setVisible(false);

  this->SecondLineChart->setAxes(this->XAxis, this->RightYAxis);
  connect(this->SecondLineChart, SIGNAL(layoutNeeded()), this,
      SLOT(updateLayout()));
  connect(this->SecondLineChart, SIGNAL(repaintNeeded()), this,
      SLOT(repaintChart()));
}

pqLineChartWidget::~pqLineChartWidget()
{
  delete this->LineChart;
  delete this->SecondLineChart;
  delete this->Legend;
  delete this->YAxis;
  delete this->RightYAxis;
  delete this->XAxis;
  delete this->Title;
  delete this->ZoomPan;
  delete this->ZoomPanAlt;
  delete this->Mouse;
}

void pqLineChartWidget::setBackgroundColor(const QColor& color)
{
  this->BackgroundColor = color;

  this->layoutChart(this->ZoomPan->contentsWidth(),
      this->ZoomPan->contentsHeight());
}

void pqLineChartWidget::setFont(const QFont &f)
{
  QAbstractScrollArea::setFont(f);

  // Block the axis update signals until all the changes are
  // made. This avoids laying out the chart for each individual
  // change.
  QFontMetrics fm(f);

  this->XAxis->blockSignals(true);
  this->XAxis->setTickLabelFont(f);
  this->XAxis->blockSignals(false);

  this->YAxis->blockSignals(true);
  this->YAxis->setTickLabelFont(f);
  this->YAxis->blockSignals(false);

  this->RightYAxis->blockSignals(true);
  this->RightYAxis->setTickLabelFont(f);
  this->RightYAxis->blockSignals(false);

  this->layoutChart(this->ZoomPan->contentsWidth(),
      this->ZoomPan->contentsHeight());
}

void pqLineChartWidget::updateLayout()
{
  // All of the chart members' layouts are interrelated. When one
  // of them needs to be updated, they all need to be updated.
  this->layoutChart(this->ZoomPan->contentsWidth(),
      this->ZoomPan->contentsHeight());
  this->viewport()->update();
}

void pqLineChartWidget::repaintChart()
{
  this->viewport()->update();
}

void pqLineChartWidget::layoutChart(int w, int h)
{
  QRect area(MARGIN, MARGIN, w - DBL_MARGIN, h - DBL_MARGIN);
  
  // Leave space for the title ...
  const QRect title_request = this->Title->getSizeRequest();
  const QRect title_bounds = QRect(area.left(), area.top(), area.width(), title_request.height());
  this->Title->setBounds(title_bounds);
  area.setTop(title_bounds.bottom());
  
  // Leave space for the legend ...
  const QRect legend_request = this->Legend->getSizeRequest();
  const QRect legend_bounds = QRect(
    area.right() - legend_request.width(),
    area.center().y() - (legend_request.height() / 2),
    legend_request.width(),
    legend_request.height());
  this->Legend->setBounds(legend_bounds);
  area.setRight(legend_bounds.left());
  
  this->XAxis->layoutAxis(area);
  this->YAxis->layoutAxis(area);
  this->RightYAxis->layoutAxis(area);
  this->LineChart->layoutChart();
  this->SecondLineChart->layoutChart();
}

QSize pqLineChartWidget::sizeHint() const
{
  this->ensurePolished();
  int f = 150 + 2*this->frameWidth();
  return QSize(f, f);
}

bool pqLineChartWidget::event(QEvent *e)
{
  if(e->type() == QEvent::ToolTip)
    this->LineChart->showTooltip(*static_cast<QHelpEvent*>(e));
  
  return QAbstractScrollArea::event(e);
}
    
void pqLineChartWidget::keyPressEvent(QKeyEvent *e)
{
  bool handled = true;
  if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus ||
      e->key() == Qt::Key_Equal)
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
  else if(e->key() == Qt::Key_Up)
    {
    this->ZoomPan->panUp();
    }
  else if(e->key() == Qt::Key_Down)
    {
    this->ZoomPan->panDown();
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(e->modifiers() == Qt::AltModifier)
      this->ZoomPan->historyPrevious();
    else
      this->ZoomPan->panLeft();
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(e->modifiers() == Qt::AltModifier)
      this->ZoomPan->historyNext();
    else
      this->ZoomPan->panRight();
    }
  else if(e->key() == Qt::Key_A)
    {
    this->useAlternateZoomPan = true;
    }
  else
    handled = false;

  if(handled)
    e->accept();
  else
    QAbstractScrollArea::keyPressEvent(e);
}

void pqLineChartWidget::showEvent(QShowEvent *e)
{
  QAbstractScrollArea::showEvent(e);
  this->ZoomPan->updateContentSize();
}

void pqLineChartWidget::paintEvent(QPaintEvent *e)
{
  // Get the clip area from the paint event. Set the painter to
  // content coordinates.
  QRect area = e->rect();
  if(!area.isValid())
    return;
    
  QPainter painter(this->viewport());
    
  this->draw(painter, area);
  
  e->accept();
}

void pqLineChartWidget::mousePressEvent(QMouseEvent *e)
{
  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = true;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  this->Mouse->Last = point;
  this->ZoomPan->Last = e->globalPos();
  this->ZoomPanAlt->Last = e->globalPos();

  e->accept();
}

void pqLineChartWidget::mouseReleaseEvent(QMouseEvent *e)
{
  // Get the current mouse position and convert it to contents coords.
  this->MouseDown = false;
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  if(this->Mode == pqLineChartWidget::ZoomBox)
    {
    this->Mode = pqLineChartWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    if(this->Mouse)
      {
      this->Mouse->adjustBox(point);
      this->ZoomPan->zoomToRectangle(&this->Mouse->Box);
      this->ZoomPanAlt->zoomToRectangle(&this->Mouse->Box);
      this->Mouse->resetBox();
      }
    }
  else if(this->Mode == pqLineChartWidget::Zoom ||
      this->Mode == pqLineChartWidget::Pan)
    {
    this->Mode = pqLineChartWidget::NoMode;
    if(this->useAlternateZoomPan)
      this->ZoomPanAlt->finishInteraction();
    else
      this->ZoomPan->finishInteraction();
    }
  else if(this->Mode != pqLineChartWidget::NoMode)
    {
    this->Mode = pqLineChartWidget::NoMode;
    this->setCursor(Qt::ArrowCursor);
    }

  this->useAlternateZoomPan = false;
    
  e->accept();
}

void pqLineChartWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(e->button() == Qt::MidButton)
    this->ZoomPan->resetZoom();

  e->accept();
}

void pqLineChartWidget::mouseMoveEvent(QMouseEvent *e)
{
  if(!this->MouseDown)
    return;

  // Get the current mouse position and convert it to contents coords.
  QPoint point = e->pos();
  point.rx() += this->ZoomPan->contentsX();
  point.ry() += this->ZoomPan->contentsY();

  // Check for the move wait timer. If it is active, cancel
  // the timer so it does not send a selection update.
  if(this->Mode == pqLineChartWidget::MoveWait)
    {
    this->Mode = pqLineChartWidget::NoMode;
    }

  bool handled = true;
  if(this->Mode == pqLineChartWidget::NoMode)
    {
    // Change the cursor for the mouse mode.
    if(e->buttons() == Qt::MidButton)
      {
      if(e->modifiers() == Qt::ShiftModifier)
        {
        this->Mode = pqLineChartWidget::ZoomBox;
        this->ZoomPan->setZoomCursor();
        this->ZoomPanAlt->setZoomCursor();
        }
      else 
        {
        this->Mode = pqLineChartWidget::Zoom;
        if(this->useAlternateZoomPan)
          this->ZoomPanAlt->startInteraction(pqChartZoomPanAlt::Zoom);
        else
          this->ZoomPan->startInteraction(pqChartZoomPan::Zoom);
        }
      }
    else if(e->buttons() == Qt::RightButton)
      {
      this->SkipContextMenu = true;
      this->Mode = pqLineChartWidget::Pan;
      if(this->useAlternateZoomPan)
        this->ZoomPanAlt->startInteraction(pqChartZoomPanAlt::Pan);
      else
        this->ZoomPan->startInteraction(pqChartZoomPan::Pan);
      }
    else
      handled = false;
    }

  if(this->Mouse)
    {
    if(this->Mode == pqLineChartWidget::ZoomBox)
      {
      QRect area = this->Mouse->Box;
      this->Mouse->adjustBox(point);

      // Repaint the zoom box. Unite the previous area with the new
      // area to ensure all the changes get repainted.
      if(area.isValid())
        area = area.unite(this->Mouse->Box);
      else
        area = this->Mouse->Box;

      // Translate the area to viewport coordinates.
      area.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
      this->viewport()->update(area);
      }
    else if(this->Mode == pqLineChartWidget::Zoom)
      {
      if(this->useAlternateZoomPan)
        {
        pqChartZoomPanAlt::InteractFlags altflags = pqChartZoomPanAlt::ZoomBoth;
        if(e->modifiers() == Qt::ControlModifier)
          altflags = pqChartZoomPanAlt::ZoomXOnly;
        else if(e->modifiers() == Qt::AltModifier)
          altflags = pqChartZoomPanAlt::ZoomYOnly;
        this->ZoomPanAlt->interact(e->globalPos(), altflags);
        }
      else
        {
        pqChartZoomPan::InteractFlags flags = pqChartZoomPan::ZoomBoth;
        if(e->modifiers() == Qt::ControlModifier)
          flags = pqChartZoomPan::ZoomXOnly;
        else if(e->modifiers() == Qt::AltModifier)
          flags = pqChartZoomPan::ZoomYOnly;
        this->ZoomPan->interact(e->globalPos(), flags);
        }
      }
    else if(this->Mode == pqLineChartWidget::Pan)
      {
      if(this->useAlternateZoomPan)
        this->ZoomPanAlt->interact(e->globalPos(), pqChartZoomPanAlt::NoFlags);
      else
        this->ZoomPan->interact(e->globalPos(), pqChartZoomPan::NoFlags);
      }
    else
      handled = false;
    }

  if(handled)
    e->accept();
  else
    e->ignore();
}

void pqLineChartWidget::wheelEvent(QWheelEvent *e)
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

  e->accept();
}

void pqLineChartWidget::resizeEvent(QResizeEvent *)
{
  this->ZoomPan->updateContentSize();
}

void pqLineChartWidget::contextMenuEvent(QContextMenuEvent *e)
{
  e->accept();
}

bool pqLineChartWidget::viewportEvent(QEvent *e)
{
  if(e->type() == QEvent::ContextMenu)  
    {
    if(this->SkipContextMenu)
      {
      this->SkipContextMenu = false;
      e->accept();
      return true;
      }
    }
    
  return QAbstractScrollArea::viewportEvent(e);
}

void pqLineChartWidget::draw(QPainter& painter, QRect area)
{
  painter.translate(-this->ZoomPan->contentsX(), -this->ZoomPan->contentsY());
  area.translate(this->ZoomPan->contentsX(), this->ZoomPan->contentsY());
//  painter.setClipRect(area);

  // Set the widget font.
  painter.setFont(font());

  // Paint the widget background.
  painter.fillRect(area, this->BackgroundColor);

  // Draw in the axes and grid.
  this->YAxis->drawAxis(&painter, area);
  this->RightYAxis->drawAxis(&painter, area);
  this->XAxis->drawAxis(&painter, area);

  // Paint the chart.
  this->LineChart->drawChart(painter, area);
  this->SecondLineChart->drawChart(painter, area);

  // Draw in the axis lines again to ensure they are on top.
  this->YAxis->drawAxisLine(&painter);
  this->RightYAxis->drawAxisLine(&painter);
  this->XAxis->drawAxisLine(&painter);

  // Draw the chart title
  this->Title->draw(painter, area);

  // Draw the chart legend
  this->Legend->draw(painter, area);

  if(this->Mouse->Box.isValid())
    {
    // Draw in mouse box selection or zoom if needed.
    painter.setPen(Qt::black);
    painter.setPen(Qt::DotLine);
    if(this->Mode == pqLineChartWidget::ZoomBox)
      {
      painter.drawRect(this->Mouse->Box.x(), this->Mouse->Box.y(),
          this->Mouse->Box.width() - 1, this->Mouse->Box.height() - 1);
      }
    }
}

void pqLineChartWidget::printChart(QPrinter& printer)
{
  QSize viewport_size(this->rect().size());
  viewport_size.scale(printer.pageRect().size(), Qt::KeepAspectRatio);

  QPainter painter(&printer);
  painter.setWindow(this->rect());
  painter.setViewport(QRect(0, 0, viewport_size.width(), viewport_size.height()));

  this->draw(painter, this->rect());
}

void pqLineChartWidget::savePDF(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(files[i]);
    
    this->printChart(printer);
    }
}

void pqLineChartWidget::savePNG(const QStringList& files)
{
  const QPixmap grab = QPixmap::grabWidget(this);
  for(int i = 0; i != files.size(); ++i)
    {
    grab.save(files[i], "PNG");
    }
}
