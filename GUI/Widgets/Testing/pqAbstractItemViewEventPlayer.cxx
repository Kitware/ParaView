/*=========================================================================

   Program: ParaView
   Module:    pqAbstractItemViewEventPlayer.cxx

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

#include "pqAbstractItemViewEventPlayer.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QMouseEvent>
#include <QTime>
#include <QtDebug>

/// Converts a string representation of a model index into the real thing
static QModelIndex GetIndex(QAbstractItemView& View, const QString& Name)
{
    QStringList rows = Name.split('/', QString::SkipEmptyParts);
    QString column;
    
    if(rows.size())
      {
      column = rows.back().split('|').at(1);
      rows.back() = rows.back().split('|').at(0);
      }
    
    QModelIndex index;
    for(int i = 0; i != rows.size(); ++i)
      {
      index = View.model()->index(rows[i].toInt(), column.toInt(), index);
      }
      
    return index;
}

///////////////////////////////////////////////////////////////////////////////
// pqAbstractItemViewEventPlayer

pqAbstractItemViewEventPlayer::pqAbstractItemViewEventPlayer()
{
}

bool pqAbstractItemViewEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  QAbstractItemView* const object = qobject_cast<QAbstractItemView*>(Object);
  if(!object)
    {
    return false;
    }
    
  if(Command == "currentChanged")
    {
    const QModelIndex index = GetIndex(*object, Arguments);
    if(!index.isValid())
      return false;
      
    object->setCurrentIndex(index);
    return true;
    }
    
  qCritical() << "Unknown abstract item command: " << Command << "\n";
  Error = true;
  return true;
}
