/*=========================================================================

   Program: ParaView
   Module:    pqSourceInfoIcons.cxx

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

========================================================================*/

/// \file pqSourceInfoIcons.cxx
/// \date 6/9/2006

#include "pqSourceInfoIcons.h"

#include <QMap>
#include <QString>


class pqSourceInfoIconsInternal : public QMap<QString, QString> {};


pqSourceInfoIcons::pqSourceInfoIcons(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqSourceInfoIconsInternal();
}

pqSourceInfoIcons::~pqSourceInfoIcons()
{
  delete this->Internal;
}

QPixmap pqSourceInfoIcons::getPixmap(const QString &source,
    pqSourceInfoIcons::DefaultPixmap alternate) const
{
  QMap<QString, QString>::Iterator iter = this->Internal->find(source);
  if(iter != this->Internal->end())
    {
    return QPixmap(*iter);
    }

  // If the source does not have a special icon, use the default.
  if(alternate == pqSourceInfoIcons::Source)
    {
    return QPixmap(":/pqWidgets/pqSource16.png");
    }
  else if(alternate == pqSourceInfoIcons::Filter)
    {
    return QPixmap(":/pqWidgets/pqFilter16.png");
    }
  else if(alternate == pqSourceInfoIcons::CustomFilter)
    {
    return QPixmap(":/pqWidgets/pqBundle16.png");
    }

  return QPixmap();
}

void pqSourceInfoIcons::setPixmap(const QString &source,
    const QString &fileName)
{
  // Insert the new mapping. This will overwrite any previous icon
  // for the given source.
  this->Internal->insert(source, fileName);
  emit this->pixmapChanged(source);
}

void pqSourceInfoIcons::clearPixmaps()
{
  this->Internal->clear();
}


