/*=========================================================================

   Program: ParaView
   Module:    pqChartLegendModel.cxx

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

/// \file pqChartLegendModel.cxx
/// \date 11/14/2006

#include "pqChartLegendModel.h"

#include "pqPointMarker.h"
#include <QList>
#include <QPainter>
#include <QPen>


class pqChartLegendModelItem
{
public:
  pqChartLegendModelItem(const QPixmap &icon, const QString &text);
  ~pqChartLegendModelItem() {}

  QPixmap Icon;
  QString Text;
};


class pqChartLegendModelInternal : public QList<pqChartLegendModelItem *> {};


//----------------------------------------------------------------------------
pqChartLegendModelItem::pqChartLegendModelItem(const QPixmap &icon,
    const QString &text)
  : Icon(icon), Text(text)
{
}


//----------------------------------------------------------------------------
pqChartLegendModel::pqChartLegendModel(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqChartLegendModelInternal();
  this->InModify = false;
}

pqChartLegendModel::~pqChartLegendModel()
{
  QList<pqChartLegendModelItem *>::Iterator iter = this->Internal->begin();
  for( ; iter != this->Internal->end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
}

void pqChartLegendModel::addEntry(const QPixmap &icon, const QString &text)
{
  this->Internal->append(new pqChartLegendModelItem(icon, text));
  if(!this->InModify)
    {
    emit this->entryInserted(this->Internal->size() - 1);
    }
}

void pqChartLegendModel::insertEntry(int index, const QPixmap &icon,
    const QString &text)
{
  if(index < 0)
    {
    index = 0;
    }

  if(index < this->Internal->size())
    {
    this->Internal->insert(index, new pqChartLegendModelItem(icon, text));
    if(!this->InModify)
      {
      emit this->entryInserted(index);
      }
    }
  else
    {
    this->addEntry(icon, text);
    }
}

void pqChartLegendModel::removeEntry(int index)
{
  if(index >= 0 && index < this->Internal->size())
    {
    if(!this->InModify)
      {
      emit this->removingEntry(index);
      }

    delete this->Internal->takeAt(index);
    if(!this->InModify)
      {
      emit this->entryRemoved(index);
      }
    }
}

void pqChartLegendModel::removeAllEntries()
{
  if(this->Internal->size() > 0)
    {
    QList<pqChartLegendModelItem *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->clear();
    if(!this->InModify)
      {
      emit this->entriesReset();
      }
    }
}

void pqChartLegendModel::startModifyingData()
{
  this->InModify = true;
}

void pqChartLegendModel::finishModifyingData()
{
  if(this->InModify)
    {
    this->InModify = false;
    emit this->entriesReset();
    }
}

int pqChartLegendModel::getNumberOfEntries() const
{
  return this->Internal->size();
}

QPixmap pqChartLegendModel::getIcon(int index) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    return this->Internal->at(index)->Icon;
    }

  return QPixmap();
}

QString pqChartLegendModel::getText(int index) const
{
  if(index >= 0 && index < this->Internal->size())
    {
    return this->Internal->at(index)->Text;
    }

  return QString();
}

QPixmap pqChartLegendModel::generateLineIcon(const QPen &pen,
    pqPointMarker *marker, const QPen *pointPen)
{
  // Create a blank pixmap of the appropriate size.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Draw a line on the pixmap.
  QPainter painter(&icon);
  painter.setPen(pen);
  painter.drawLine(1, 15, 14, 0);

  // Draw a point in the middle of the line.
  if(marker)
    {
    if(pointPen)
      {
      painter.setPen(*pointPen);
      }

    painter.translate(QPoint(7, 7));
    marker->drawMarker(painter);
    }

  return icon;
}

QPixmap pqChartLegendModel::generateColorIcon(const QColor &color)
{
  // Create a blank pixmap of the appropriate size.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Fill a small rectangle using the color given.
  QPainter painter(&icon);
  painter.fillRect(3, 3, 10, 10, QBrush(color));

  return icon;
}


