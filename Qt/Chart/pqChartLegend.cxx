/*=========================================================================

   Program: ParaView
   Module:    pqChartLegend.cxx

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

#include "pqChartLegend.h"

#include "pqChartLegendModel.h"
#include <QEvent>
#include <QFont>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>


class pqChartLegendInternal
{
public:
  pqChartLegendInternal();
  ~pqChartLegendInternal() {}

  QList<int> Entries;
  int EntryHeight;
  bool FontChanged;
};


//----------------------------------------------------------------------------
pqChartLegendInternal::pqChartLegendInternal()
  : Entries()
{
  this->EntryHeight = 0;
  this->FontChanged = false;
}


//----------------------------------------------------------------------------
pqChartLegend::pqChartLegend(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->Internal = new pqChartLegendInternal();
  this->Model = 0;
  this->Location = pqChartLegend::Right;
  this->Flow = pqChartLegend::TopToBottom;
  this->IconSize = 16;
  this->TextSpacing = 4;
  this->Margin = 5;

  // Set the size policy to go with the default location.
  this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

pqChartLegend::~pqChartLegend()
{
  delete this->Internal;
}

void pqChartLegend::setModel(pqChartLegendModel *model)
{
  if(this->Model)
    {
    this->disconnect(this->Model, 0, this, 0);
    }

  this->Model = model;
  if(this->Model)
    {
    this->connect(this->Model, SIGNAL(entriesReset()), this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(entryInserted(int)),
        this, SLOT(insertEntry(int)));
    this->connect(this->Model, SIGNAL(removingEntry(int)),
        this, SLOT(startEntryRemoval(int)));
    this->connect(this->Model, SIGNAL(entryRemoved(int)),
        this, SLOT(finishEntryRemoval(int)));
    this->connect(this->Model, SIGNAL(iconChanged(int)),
        this, SLOT(update()));
    this->connect(this->Model, SIGNAL(textChanged(int)),
        this, SLOT(updateEntryText(int)));
    }

  this->reset();
}

void pqChartLegend::setLocation(pqChartLegend::LegendLocation location)
{
  if(this->Location != location)
    {
    this->Location = location;
    if(this->Location == pqChartLegend::Top ||
        this->Location == pqChartLegend::Bottom)
      {
      this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      }
    else
      {
      this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
      }

    this->calculateSize();
    emit this->locationChanged();
    }
}

void pqChartLegend::setFlow(pqChartLegend::ItemFlow flow)
{
  if(this->Flow != flow)
    {
    this->Flow = flow;
    this->calculateSize();
    this->update();
    }
}

void pqChartLegend::drawLegend(QPainter &painter)
{
  // Set up the painter for the location and flow. Some combinations
  // may require the painter to be rotated.
  QSize area = this->size();
  QSize bounds = this->Bounds;
  if((this->Flow == pqChartLegend::LeftToRight &&
      (this->Location == pqChartLegend::Left ||
      this->Location == pqChartLegend::Right)) ||
      (this->Flow == pqChartLegend::TopToBottom &&
      (this->Location == pqChartLegend::Top ||
      this->Location == pqChartLegend::Bottom)))
    {
    painter.translate(QPoint(0, this->height() - 1));
    painter.rotate(-90.0);
    area.transpose();
    bounds.transpose();
    }

  // TODO: Allow the user to pan the contents when they are too big
  // to be seen in the viewport.

  int offset = 0;
  int index = 0;
  QFontMetrics fm = this->fontMetrics();
  painter.setPen(QColor(Qt::black));
  if(this->Flow == pqChartLegend::LeftToRight)
    {
    // Determine the offset. Then, draw the outline.
    offset = area.width() - bounds.width();
    offset = offset > 0 ? offset / 2 : 0;
    painter.drawRect(offset, 0, bounds.width() - 1, bounds.height() - 1);

    // Determine the icon and text y-position.
    int iconY = bounds.height() - this->IconSize;
    if(iconY != 0)
      {
      iconY = iconY / 2;
      }

    int textY = bounds.height() - fm.height();
    if(textY != 0)
      {
      textY = textY / 2;
      }

    textY += fm.ascent() + 1;

    // Draw all the entries.
    offset += this->Margin;
    QList<int>::Iterator iter = this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter, ++index)
      {
      int px = offset;
      QPixmap icon = this->Model->getIcon(index);
      if(!icon.isNull())
        {
        // Make sure the pixmap is sized properly.
        icon = icon.scaled(QSize(this->IconSize, this->IconSize),
            Qt::KeepAspectRatio);
        painter.drawPixmap(px, iconY, icon);
        px += this->IconSize + this->TextSpacing;
        }

      painter.drawText(px, textY, this->Model->getText(index));
      offset += *iter + this->TextSpacing;
      }
    }
  else
    {
    // Determine the offset. Then, draw the outline.
    offset = area.height() - bounds.height();
    offset = offset > 0 ? offset / 2 : 0;
    painter.drawRect(0, offset, bounds.width() - 1, bounds.height() - 1);

    // find the lengths needed to center the icon and text.
    int iconY = this->Internal->EntryHeight - this->IconSize;
    if(iconY != 0)
      {
      iconY = iconY / 2;
      }

    int textY = this->Internal->EntryHeight - fm.height();
    if(textY != 0)
      {
      textY = textY / 2;
      }

    textY += fm.ascent() + 1;

    // Draw all the entries.
    offset += this->Margin;
    for( ; index < this->Internal->Entries.size(); ++index)
      {
      int px = this->Margin;
      QPixmap icon = this->Model->getIcon(index);
      if(!icon.isNull())
        {
        // Make sure the pixmap is sized properly.
        icon = icon.scaled(QSize(this->IconSize, this->IconSize),
            Qt::KeepAspectRatio);
        painter.drawPixmap(px, offset + iconY, icon);
        px += this->IconSize + this->TextSpacing;
        }

      painter.drawText(px, offset + textY, this->Model->getText(index));
      offset += this->Internal->EntryHeight + this->TextSpacing;
      }
    }
}

void pqChartLegend::reset()
{
  this->Internal->Entries.clear();
  if(this->Model)
    {
    for(int i = this->Model->getNumberOfEntries(); i > 0; i--)
      {
      this->Internal->Entries.append(0);
      }
    }

  this->calculateSize();
  this->update();
}

void pqChartLegend::insertEntry(int index)
{
  this->Internal->Entries.insert(index, 0);
  this->calculateSize();
  this->update();
}

void pqChartLegend::startEntryRemoval(int index)
{
  this->Internal->Entries.removeAt(index);
}

void pqChartLegend::finishEntryRemoval(int)
{
  this->calculateSize();
  this->update();
}

void pqChartLegend::updateEntryText(int index)
{
  this->Internal->Entries[index] = 0;
  this->calculateSize();
  this->update();
}

bool pqChartLegend::event(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    this->Internal->FontChanged = true;
    this->calculateSize();
    this->Internal->FontChanged = false;
    this->update();
    }

  return QWidget::event(e);
}

void pqChartLegend::paintEvent(QPaintEvent *e)
{
  if(!this->Model || !this->Bounds.isValid() || !e->rect().isValid() ||
      this->Internal->Entries.size() == 0)
    {
    return;
    }

  QPainter painter(this);
  this->drawLegend(painter);
}

void pqChartLegend::calculateSize()
{
  QSize bounds;
  if(this->Internal->Entries.size() > 0)
    {
    // Get the font height for the entries. For now, all the entries
    // use the same font.
    QFontMetrics fm = this->fontMetrics();
    this->Internal->EntryHeight = fm.height();
    if(this->Internal->EntryHeight < this->IconSize)
      {
      this->Internal->EntryHeight = this->IconSize;
      }

    // Find the width needed for each entry. Use the width to determine
    // the necessary space.
    int total = 0;
    int maxWidth = 0;
    QList<int>::Iterator iter = this->Internal->Entries.begin();
    for(int i = 0; iter != this->Internal->Entries.end(); ++iter, ++i)
      {
      if(this->Model && (this->Internal->FontChanged || *iter == 0))
        {
        QString text = this->Model->getText(i);
        *iter = fm.width(text);
        QPixmap icon = this->Model->getIcon(i);
        if(!icon.isNull())
          {
          *iter += this->IconSize + this->TextSpacing;
          }
        }

      // Sum up the entry widths for left-to-right. In top-to-bottom
      // mode, find the max width.
      if(this->Flow == pqChartLegend::LeftToRight)
        {
        total += *iter;
        if(i > 0)
          {
          total += this->TextSpacing;
          }
        }
      else if(*iter > maxWidth)
        {
        maxWidth = *iter;
        }
      }

    // Add space around the entries for the outline.
    int padding = 2 * this->Margin;
    if(this->Flow == pqChartLegend::LeftToRight)
      {
      bounds.setHeight(total + padding);
      bounds.setWidth(this->Internal->EntryHeight + padding);
      if(this->Location == pqChartLegend::Top ||
          this->Location == pqChartLegend::Bottom)
        {
        bounds.transpose();
        }
      }
    else
      {
      total = this->Internal->EntryHeight * this->Internal->Entries.size();
      total += padding;
      if(this->Internal->Entries.size() > 1)
        {
        total += (this->Internal->Entries.size() - 1) * this->TextSpacing;
        }

      bounds.setWidth(maxWidth + padding);
      bounds.setHeight(total);
      if(this->Location == pqChartLegend::Top ||
          this->Location == pqChartLegend::Bottom)
        {
        bounds.transpose();
        }
      }
    }

  if(bounds != this->Bounds)
    {
    this->Bounds = bounds;
    this->updateGeometry();
    }
}


