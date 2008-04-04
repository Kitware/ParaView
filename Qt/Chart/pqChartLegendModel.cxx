/*=========================================================================

   Program: ParaView
   Module:    pqChartLegendModel.cxx

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
  unsigned int Id;
};


class pqChartLegendModelInternal
{
public:
  pqChartLegendModelInternal();
  ~pqChartLegendModelInternal() {}

  QList<pqChartLegendModelItem *> Entries;
  unsigned int NextId;
};


//----------------------------------------------------------------------------
pqChartLegendModelItem::pqChartLegendModelItem(const QPixmap &icon,
    const QString &text)
  : Icon(icon), Text(text)
{
  this->Id = 0;
}


//----------------------------------------------------------------------------
pqChartLegendModelInternal::pqChartLegendModelInternal()
  : Entries()
{
  this->NextId = 1;
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
  QList<pqChartLegendModelItem *>::Iterator iter =
      this->Internal->Entries.begin();
  for( ; iter != this->Internal->Entries.end(); ++iter)
    {
    delete *iter;
    }

  delete this->Internal;
}

int pqChartLegendModel::addEntry(const QPixmap &icon, const QString &text)
{
  return this->insertEntry(this->Internal->Entries.size(), icon, text);
}

int pqChartLegendModel::insertEntry(int index, const QPixmap &icon,
    const QString &text)
{
  if(index < 0)
    {
    index = 0;
    }

  pqChartLegendModelItem *item = new pqChartLegendModelItem(icon, text);
  item->Id = this->Internal->NextId++;
  if(index < this->Internal->Entries.size())
    {
    this->Internal->Entries.insert(index, item);
    }
  else
    {
    this->Internal->Entries.append(item);
    }

  if(!this->InModify)
    {
    emit this->entryInserted(index);
    }

  return item->Id;
}

void pqChartLegendModel::removeEntry(int index)
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    if(!this->InModify)
      {
      emit this->removingEntry(index);
      }

    delete this->Internal->Entries.takeAt(index);
    if(!this->InModify)
      {
      emit this->entryRemoved(index);
      }
    }
}

void pqChartLegendModel::removeAllEntries()
{
  if(this->Internal->Entries.size() > 0)
    {
    QList<pqChartLegendModelItem *>::Iterator iter =
        this->Internal->Entries.begin();
    for( ; iter != this->Internal->Entries.end(); ++iter)
      {
      delete *iter;
      }

    this->Internal->Entries.clear();
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
  return this->Internal->Entries.size();
}

int pqChartLegendModel::getIndexForId(unsigned int id) const
{
  QList<pqChartLegendModelItem *>::Iterator iter =
      this->Internal->Entries.begin();
  for(int index = 0; iter != this->Internal->Entries.end(); ++iter, ++index)
    {
    if((*iter)->Id == id)
      {
      return index;
      }
    }

  return -1;
}

QPixmap pqChartLegendModel::getIcon(int index) const
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    return this->Internal->Entries[index]->Icon;
    }

  return QPixmap();
}

void pqChartLegendModel::setIcon(int index, const QPixmap &icon)
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    this->Internal->Entries[index]->Icon = icon;
    emit this->iconChanged(index);
    }
}

QString pqChartLegendModel::getText(int index) const
{
  if(index >= 0 && index < this->Internal->Entries.size())
    {
    return this->Internal->Entries[index]->Text;
    }

  return QString();
}

void pqChartLegendModel::setText(int index, const QString &text)
{
  if(index >= 0 && index < this->Internal->Entries.size() &&
    text != this->Internal->Entries[index]->Text)
    {
    this->Internal->Entries[index]->Text = text;
    emit this->textChanged(index);
    }
}

QPixmap pqChartLegendModel::generateLineIcon(const QPen &pen,
    pqPointMarker *marker, const QPen *pointPen)
{
  // Create a blank pixmap of the appropriate size.
  QPixmap icon(16, 16);
  icon.fill(QColor(255, 255, 255, 0));

  // Draw a line on the pixmap.
  QPainter painter(&icon);
  painter.setRenderHint(QPainter::Antialiasing, true);
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


